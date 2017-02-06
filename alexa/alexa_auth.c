/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.
	
	File: alexa_auth.c

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/01/19 11:28:08

*******************************************************************************/

#include <curl/curl.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>

#include "amazon_alexa_if.h"
#include "cjson/cjson.h"

#define TODO 1

#define TAG	"auth"

#undef DEBUG
#define MAX_BUF 655300
#define TMP_BUF 200
#define RECORD_FRAME_SIZE 320
#define RESPONSE_OUTFILE "response.mp3"

#ifdef WIN32

#define ALEXA_KEY_PATH					"alexa.key"
#define ALEXA_TEST_FILE					"howistheweatherinbeijing.wav"
#define ALEXA_RESPONSE_FILE				"response.mp3"

#define bool                            unsigned char
#define false                           0
#define true                            1

#define alexa_delay( ms )				lg_delay( ms )

#define alexa_mutex						int
#define alexa_mutex_create()			lg_create_sem(1)
#define alexa_mutex_lock(handle)		lg_wait_sem(handle, -1)
#define alexa_mutex_unlock(handle)		lg_give_sem(handle)
#define alexa_mutex_destroy(handle)		lg_destroy_sem(handle)


#define alexa_thread					                int
#define alexa_thread_create(proc, p_data, stack, prio)	lg_begin_thread(proc, p_data, stack, prio)
#define alexa_thread_exit(handle)				        lg_end_thread(handle)

#else

#include <semaphore.h>
#include <errno.h>
    
#define ALEXA_KEY_PATH                  "/usr/resource/alexa.key"
#define AMAZON_ALEXA_TEST_FILE          "/usr/resource/howistheweatherinbeijing.wav"
#define AMAZON_ALEXA_RESPONSE_FILE      "/data/response.mp3"

#define bool                            unsigned char
#define false                           0
#define true                            1

#define alexa_delay( ms )				usleep( ms * 1000 )

#define alexa_mutex						int
#define alexa_mutex_create()			lg_create_mutex()
#define alexa_mutex_lock(handle)		lg_lock_mutex(handle)
#define alexa_mutex_unlock(handle)		lg_unlock_mutex(handle)
#define alexa_mutex_destroy(handle)		lg_destroy_mutex(handle)

#define alexa_thread					                int
#define alexa_thread_create(proc, p_data, stack, prio)	lg_begin_thread(proc, p_data, stack, prio)
#define alexa_thread_exit(handle)				        lg_end_thread(handle)

#endif

#define ALEXA_AHTH_URL  "https://api.amazon.com/auth/o2/token"


static int aa_get_authorization(struct aa_service* aa, const char* file);


/**
 @brief
 from :https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/docs/authorizing-your-alexa-enabled-product-from-an-android-or-ios-mobile-app
 
 this struct like the link 
 */
struct aa_authorization {
	char *authorizationCode;
	char *redirectUri;
	char *clientId;
	char *codeVerifier;
};

/** 
 */
struct aa_token {
	char *access_token;
	char *refresh_token;
	char *token_type;
	time_t	 current_time; //second
	long int expires_in;   //second
};

struct aa_service{
    struct aa_authorization authorization;
    struct aa_token         token;
    bool                    initialized;
	bool					recog_thread_exit;
	bool					recog_exit;
	enum aa_state_type		state;

	alexa_mutex				main_mutex;
	alexa_mutex				token_mutex;

	alexa_thread			thread;
};

static char *record_buf;
static int record_len = RECORD_FRAME_SIZE * 2;
static mozart_amazon_callback vr_callback;
static alexa_thread amazon_thread;
static alexa_mutex amazon_mutex;

static const char *boundary = "BOUNDARY1234";
static const char *metadata_content_disposition="Content-Disposition: form-data; name=\"metadata\"";
static const char *metadata_content_type="Content-Type: application/json; charset=UTF-8";
static const char *metadata="{\"messageHeader\": {},\"messageBody\": {\"profile\": \"alexa-close-talk\",\"locale\": \"en-us\",\"format\": \"audio/L16; rate=16000; channels=1\"}}";
static const char *audio_content_disposition="Content-Disposition: form-data; name=\"audio\"";
static const char *audio_content_type="Content-Type: audio/L16; rate=16000; channels=1";

static char* authmng_cjson_dup_string(cJSON* json, const char* key)
{
	cJSON* item = cJSON_GetObjectItem(json, key);
	if( item )
	{
        int len = strlen( item->valuestring );
        char* actual_value = (char*)alexa_malloc( len + 1 );
        if( actual_value )
        {
            strcpy( actual_value, item->valuestring );
        }
        return actual_value;
	}
	return NULL;
}

static size_t authmng_post_response(void *buffer, size_t size, size_t nmemb, void *userp)
{
	int response_len = 0;
	char *response = *(char **)userp;
	char *new_response;

	if (response)
		response_len = strlen(response);

	new_response = (char*)calloc(response_len + size * nmemb + 1, 1);
	if (new_response == NULL) {
		free(response);
		*(char **)userp = NULL;
		return 0;
	}

	if (response) {
		memcpy(new_response, response, response_len);
		free(response);
	}
	memcpy(new_response + response_len, buffer, size * nmemb);

	*(char **)userp = new_response;

	return size * nmemb;
}

static int curl_send_post_save_to_file(char *url, char *fields, int len, struct curl_slist *http_headers)
{
	int ret = 0;
	long retcode = 0;
	CURL *curl;
	FILE *fp;
#ifdef WIN32    
	 if((fp = fopen(AMAZON_ALEXA_RESPONSE_FILE, "wb")) == NULL)
		 printf("open response outfile error!\n");
#else
    system("rm "AMAZON_ALEXA_RESPONSE_FILE);
	if((fp = fopen(AMAZON_ALEXA_RESPONSE_FILE, "a+")) == NULL)
		printf("open response outfile error!\n");
#endif
     
	curl = curl_easy_init();
	/* set request header */
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_headers);
	/* open ssl protol */
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

	curl_easy_setopt(curl, CURLOPT_URL, url);

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);
	/* set CURLOPT_POSTFIELDSIZE, or will send poststring as string */
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, len);

	curl_easy_setopt(curl, CURLOPT_HEADER, 0);
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

	if ((ret = curl_easy_perform(curl)) != CURLE_OK) {
		printf("curl_easy_perform errno %d\n", ret);
	}

    ret = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &retcode);
	curl_easy_cleanup(curl);
	fclose(fp);

	if ((ret == CURLE_OK) && retcode == 200)
    {
		printf("server response ok\n");
		return 0;
    }
	else {
		printf("server retcode: %ld\n",retcode);
		return -1;
	}
}

static char *authmng_curl_send_post(const char *url, char *fields)
{
	int ret = 0;
	CURL *curl;
	char *response = NULL;

	curl = curl_easy_init();
	/* open ssl protol */
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, authmng_post_response);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	if ((ret = curl_easy_perform(curl)) != CURLE_OK)
    {
		sys_log_e(TAG, "curl_easy_perform errno %d\n", ret);
	}
	curl_easy_cleanup(curl);

	return response;
}

static int parse_token_response(struct aa_service* aa, char *response)
{
	struct aa_token* token = &aa->token;

	cJSON * root_json;
	cJSON * expires_in_json;
	char*	error;
	char*	error_desc;

	char*	access_token;
	char*	refresh_token;
	char*	token_type;

	int		expires_in = 3600;
	root_json = cJSON_Parse(response);
	if( root_json )
	{
		error_desc = cjson_parse_dup_string(root_json, "error_description");
		error = cjson_parse_dup_string(root_json, "error");

		access_token = cjson_parse_dup_string(root_json, "access_token");
		refresh_token = cjson_parse_dup_string(root_json, "refresh_token");
		token_type = cjson_parse_dup_string(root_json, "token_type");
		expires_in_json = cJSON_GetObjectItem(root_json, "expires_in");
		if( expires_in_json ) expires_in = expires_in_json->valueint;
		//timestamp
		token->current_time = time( NULL );

		if(error_desc || error) 
		{
			printf( "parse response error:%s desc:%s\n", error == NULL ? "NULL" : error, error_desc == NULL ? "NULL" : error_desc );
			if(error_desc) free(error_desc);
			if(error) free(error);
			goto err;
		}
		else if( access_token && refresh_token && token_type )
		{
			if(access_token) token->access_token = access_token;
			if(refresh_token) token->refresh_token = refresh_token;
			if(token_type) token->token_type = token_type;
			token->expires_in = expires_in;
		}
		cJSON_Delete(root_json);
	}

	return 0;
err:
	return -1;
}


static int aa_service_save2keyfile( cJSON* root_json, const char* file )
{
	FILE* fp;
	char *out = cJSON_Print(root_json);
	if( !out )
	{
		goto err;
	}

	fp = fopen( file, "wb" );
	if (!fp)
	{
		goto err2;
	}

	fwrite( out, 1, strlen(out), fp  );
	fclose( fp );

	free( out );

	return 0;
err2:
	free( out );
err:
	return -1;
}


static int aa_service_save_key( aa_service* aa, const char* file )
{
	struct cJSON* cj_root = cJSON_CreateObject();
	struct cJSON* cj_auth = cJSON_CreateObject();
	struct cJSON* cj_token = cJSON_CreateObject();
    

	struct aa_authorization* auth = (struct aa_authorization*)&aa->authorization;
	struct aa_token* token = (struct aa_token*)&aa->token;

	cJSON_AddItemToObject(cj_root, "authorization", cj_auth);
	cJSON_AddItemToObject(cj_auth, "authorizationCode", cJSON_CreateString(auth->authorizationCode));
	cJSON_AddItemToObject(cj_auth, "redirectUri", cJSON_CreateString(auth->redirectUri));
	cJSON_AddItemToObject(cj_auth, "clientId", cJSON_CreateString(auth->clientId));
	cJSON_AddItemToObject(cj_auth, "codeVerifier", cJSON_CreateString(auth->codeVerifier));

	cJSON_AddItemToObject(cj_root, "token", cj_token);
	cJSON_AddItemToObject(cj_token, "access_token", cJSON_CreateString(token->access_token));
	cJSON_AddItemToObject(cj_token, "refresh_token", cJSON_CreateString(token->refresh_token));
	cJSON_AddItemToObject(cj_token, "token_type", cJSON_CreateString(token->token_type));
	cJSON_AddNumberToObject(cj_token, "expires_in", token->expires_in);
	cJSON_AddNumberToObject(cj_token, "current_time", token->current_time);

	aa_service_save2keyfile(cj_root, file);

	cJSON_Delete(cj_root);
}

static void aa_free_authorization(struct aa_service* aa)
{
	if (aa->authorization.authorizationCode) {
		free(aa->authorization.authorizationCode);
		aa->authorization.authorizationCode = NULL;
	}
	if (aa->authorization.redirectUri) {
		free(aa->authorization.redirectUri);
		aa->authorization.redirectUri = NULL;
	}
	if (aa->authorization.clientId) {
		free(aa->authorization.clientId);
		aa->authorization.clientId = NULL;
	}
	if (aa->authorization.codeVerifier) {
		free(aa->authorization.codeVerifier);
		aa->authorization.codeVerifier = NULL;
	}
}

static int amazon_set_authorization(void)
{
	//get the authorization 

	//authcode=ANuqzjGudhuJBJVIlQvv, rediuri=amzn://com.ingenic.music, clientId=amzn1.application-oa2-client.0e8cd334679f4c9d9d9bdf15f78d58f2, 
	//codeVerifier=eh+RtQ/yZUDdTLFUrtjYmrFit8pZc5V942qwBXTMIM8=

	/*
	authorization.authorizationCode = (char *)malloc(256);
	if (authorization.authorizationCode == NULL) {
		printf("%s:line%d: data buf malloc fail!\n", __func__,__LINE__);
		goto err;
	}
	if (mozart_ini_getkey("/usr/data/z.ini", "authorization", "authorizationCode", authorization.authorizationCode))
		goto err;

	authorization.redirectUri = (char *)malloc(256);
	if (authorization.redirectUri == NULL) {
		printf("%s:line%d: data buf malloc fail!\n", __func__,__LINE__);
		goto err;
	}
	if (mozart_ini_getkey("/usr/data/z.ini", "authorization", "redirectUri", authorization.redirectUri))
		goto err;

	authorization.clientId = (char *)malloc(256);
	if (authorization.clientId == NULL) {
		printf("%s:line%d: data buf malloc fail!\n", __func__,__LINE__);
		goto err;
	}
	if (mozart_ini_getkey("/usr/data/z.ini", "authorization", "clientId", authorization.clientId))
		goto err;

	authorization.codeVerifier = (char *)malloc(256);
	if (authorization.codeVerifier == NULL) {
		printf("%s:line%d: data buf malloc fail!\n", __func__,__LINE__);
		goto err;
	}
	if (mozart_ini_getkey("/usr/data/z.ini", "authorization", "codeVerifier", authorization.codeVerifier))
		goto err;
	*/

	return 0;
err:
	//aa_free_authorization(aa);
	return -1;
}

static void aa_free_token(struct aa_service* aa)
{
    struct aa_token* token = &aa->token;
    
	if(token->refresh_token) {
		free(token->refresh_token);
		token->refresh_token = NULL;
	}
	if(token->access_token) {
		free(token->access_token);
		token->access_token = NULL;
	}
	if(token->token_type) {
		free(token->token_type);
		token->token_type = NULL;
	}
	token->expires_in = 0;
}

static int aa_service_refresh_token(struct aa_service* aa)
{
	struct aa_authorization *authorization = (struct aa_authorization *)&aa->authorization;
	struct aa_token *token = (struct aa_token *)&aa->token;

	char *response = NULL;
	char *cmd_str = NULL;
	int cmd_len = 0, ret = 0;

	if(!token->refresh_token || !authorization->clientId || !authorization->codeVerifier || !authorization->redirectUri) {
		printf("why don't have the token and auth value, need the app send the auth to device!\n");
		return -1;
	}

	cmd_len = 0;
	cmd_len += strlen(token->refresh_token);
	cmd_len += strlen(authorization->clientId);
	cmd_len += strlen(authorization->codeVerifier);
	cmd_len += strlen(authorization->redirectUri);
	cmd_len += 256;

	cmd_str = (char*)malloc(cmd_len);
	if ( cmd_str )
	{
		memset(cmd_str, 0, cmd_len);
		sprintf(cmd_str, "grant_type=refresh_token&refresh_token=%s&client_id=%s&code_verifier=%s&redirect_uri=%s", 
			token->refresh_token, authorization->clientId, authorization->codeVerifier, authorization->redirectUri);

		response = curl_send_post("https://api.amazon.com/auth/o2/token", cmd_str);
		if (response == NULL) {
			printf("refresh token: no response detected!\n");
			free(cmd_str);
			return -1;
		}
		ret = parse_token_response(aa, response);
		if(ret < 0)
		{
			printf("parse refresh token fail\n");
			ret = -1;
		}

		aa_service_save_key( aa, AMAZON_ALEXA_KEY_PATH );

		free(cmd_str);
		free(response);
	}
	return ret;
}

static char *aa_get_access_token(struct aa_service* aa)
{
	time_t current_time = time(NULL);
	struct aa_token* token = (struct aa_token*)&aa->token;

	alexa_mutex_lock(aa->token_mutex);

	if (token->access_token && current_time - token->current_time < token->expires_in) {
		alexa_mutex_unlock(aa->token_mutex);
		return token->access_token;
	}
	if(token->access_token) {
		free(token->access_token);
		token->access_token = NULL;
	}
	if (aa_service_refresh_token(aa) == 0) {
		alexa_mutex_unlock(aa->token_mutex);
		return token->access_token;
	} else {
		alexa_mutex_unlock(aa->token_mutex);
		return NULL;
	}
}

static int aa_service_search_music( struct aa_service* aa ) {
	//record_param param = {16, 16000, 1, 100};
	struct mic_record *record_info;
	int record_maxlen = 16000 * 2 * 10;
	char *wr_buf;
	char tmp[TMP_BUF] = {0};
	char *wr_buf_fp;
	int body_len = 0, read_ret = 0, ret = 0;
	struct curl_slist *headers = NULL;
	char *cmd = NULL;
	char *cmd_str = NULL;
	int cmd_len = 0;
	char *access_token = aa_get_access_token( aa );
	if(access_token == NULL) {
		printf("get access_token fail!\n");
		return -1;
	}

    wr_buf = malloc( MAX_BUF );
    if( !wr_buf )
    {
		printf("malloc the wr_buf fail!\n");
        return -1;
    }
    memset( wr_buf, 0, MAX_BUF );
    
    wr_buf_fp = wr_buf;
    
	/* init cound card */
	//get the source stream
#if 0
	record_info = mozart_soundcard_init(param);
	if (!record_info) {
		printf("mozart_soundcard_init failed\n");
		return -1;
	}
	mozart_play_key_sync((char *)"welcome");

	cmd_len = strlen(access_token) + 128;
	cmd_str = malloc(cmd_len);
	memset(cmd_str, 0, cmd_len);
	sprintf(cmd_str,"Authorization: Bearer %s",access_token);

	sprintf(wr_buf, "--%s\r\n%s\r\n%s\r\n\r\n%s\r\n\r\n--%s\r\n%s\r\n%s\r\n\r\n",
			boundary, metadata_content_disposition, metadata_content_type, metadata, boundary, audio_content_disposition,
			audio_content_type);
	body_len = strlen(wr_buf);
	wr_buf_fp += body_len;

	while (!recog_exit && (record_maxlen > record_len)) {
		read_ret = mozart_record(record_info, record_buf, record_len);
		if (read_ret < 0) {
			printf("mozart_record failed\n");
			break;
		}
		memcpy(wr_buf_fp, record_buf, read_ret);
		body_len += read_ret;
		wr_buf_fp += read_ret;
		record_maxlen -= read_ret;
	}
	mozart_soundcard_uninit(record_info);
	mozart_play_key_sync((char *)"welcome");
#else

    printf( "%s %d\n", __FUNCTION__, __LINE__ );

	cmd_len = strlen(access_token) + 128;
	cmd_str = (char*)malloc(cmd_len);
	memset(cmd_str, 0, cmd_len);
	sprintf(cmd_str,"Authorization: Bearer %s",access_token);

    printf( "%s %d\n", __FUNCTION__, __LINE__ );
	sprintf(wr_buf, "--%s\r\n%s\r\n%s\r\n\r\n%s\r\n\r\n--%s\r\n%s\r\n%s\r\n\r\n",
		boundary, metadata_content_disposition, metadata_content_type, metadata, boundary, audio_content_disposition,
		audio_content_type);
	body_len = strlen(wr_buf);
	wr_buf_fp += body_len;

    printf( "%s %d\n", __FUNCTION__, __LINE__ );
	{
		FILE * fp = fopen( AMAZON_ALEXA_TEST_FILE, "rb" );
		int recode_file_length = 0;
		int read_ret = 0;
		if ( fp )
		{
			//wav file skip the header
			fseek( fp, 0, SEEK_END );
			recode_file_length = ftell( fp );
			fseek( fp, 0, SEEK_SET );

			recode_file_length -= 44;

			if ( recode_file_length > record_maxlen )
			{
				recode_file_length = record_maxlen;
			}
			read_ret = fread(wr_buf_fp, 1, recode_file_length, fp);
			body_len += read_ret;
			wr_buf_fp += read_ret;
			record_maxlen -= read_ret;
			fclose(fp);
		}
	}
    printf( "%s %d\n", __FUNCTION__, __LINE__ );

#endif
	printf("searching...\n");

	/*add boundary*/
	memset(tmp, 0, TMP_BUF);
	sprintf(tmp, "\r\n\r\n--%s--\r\n", boundary);
	memcpy(wr_buf_fp, tmp, strlen(tmp));
	body_len = body_len + strlen(tmp);

    printf( "%s %d\n", __FUNCTION__, __LINE__ );
	/* rm response.mp3 */
	cmd = (char*)malloc(strlen(RESPONSE_OUTFILE) + 128);
	free(cmd);

    printf( "%s %d\n", __FUNCTION__, __LINE__ );
	headers = curl_slist_append(headers, cmd_str);
	headers = curl_slist_append(headers, "Content-Type: multipart/form-data; boundary=BOUNDARY1234");
	ret = curl_send_post_save_to_file("https://access-alexa-na.amazon.com/v1/avs/speechrecognizer/recognize", wr_buf, body_len, headers);
    /* free slist */
    curl_slist_free_all(headers);
	free(cmd_str);
    free(wr_buf);
    printf( "%s %d\n", __FUNCTION__, __LINE__ );

	return ret;
}

static void *aa_service_recog(void *args)
{
	struct aa_service* aa = (struct aa_service*)args;
	int ret = 0;
    
	while (!aa->recog_thread_exit) {
		if(aa->state == AA_STATE_RECOG) {
			ret = aa_service_search_music(aa);
			if (ret == 0) {
				//play the audio sound
#if 0
				if (mozart_player_playurl(amazon_handler, RESPONSE_OUTFILE)) {
					printf("player play url failed\n");
					return NULL;
				}
#endif
			} else {
				printf("amazon recognize fail!\n");
			}
			aa->state = AA_STATE_IDLE;
		} else {
			alexa_delay(50);
		}
	}

	aa->state = AA_STATE_QUIT;
	return NULL;
}

struct aa_service* aa_service_startup(mozart_amazon_callback callback)
{
	struct aa_service* aa = (struct aa_service*)malloc( sizeof(struct aa_service) );
	if ( !aa )
	{
		goto err;
	}

	memset( aa, 0, sizeof(struct aa_service) );

	//aa->main_mutex = alexa_mutex_create();
	aa->main_mutex = alexa_mutex_create();
    aa->token_mutex = alexa_mutex_create();
    printf( "%s %d 0x%08x\n", __FUNCTION__, __LINE__, aa->main_mutex );
    
	if( aa_get_authorization(aa, ALEXA_KEY_PATH) < 0 )
	{
		if (aa_service_refresh_token(aa) < 0)
			printf("refresh token fail!\n");
	}
    printf( "%s %d\n", __FUNCTION__, __LINE__ );

	alexa_mutex_lock(aa->main_mutex);
	if (record_buf) {
		free(record_buf);
		record_buf = NULL;
	}
	record_buf = (char *)malloc(record_len);
	if (record_buf == NULL) {
		printf("%s:line%d: data buf malloc fail!\n", __FUNCTION__,__LINE__);
		goto err;
	}
    printf( "%s %d\n", __FUNCTION__, __LINE__ );

	vr_callback = callback;
	aa->recog_thread_exit = false;
	aa->recog_exit = false;
	aa->state = AA_STATE_IDLE;

	if (aa->thread = alexa_thread_create(aa_service_recog, aa, NULL, 0) < 0) {
		printf("%s: Can't create amazon thread: %s!\n",
		       __FUNCTION__, strerror(errno));
		goto err_pthread_create;
	}

	aa->initialized = true;
	alexa_mutex_unlock(aa->main_mutex);
    printf( "%s %d\n", __FUNCTION__, __LINE__ );

	return aa;

err_pthread_create:
	if (record_buf) {
		free(record_buf);
		record_buf = NULL;
	}
err:
	aa_free_authorization(aa);
	aa_free_token(aa);
	alexa_mutex_unlock(amazon_mutex);

	return NULL;
}

int aa_service_shutdown(struct aa_service* aa)
{
	char *uuid = NULL;
	alexa_mutex_lock(aa->main_mutex);

	if (aa->initialized) {
		aa->recog_thread_exit = true;
		aa->recog_exit = true;
		alexa_thread_exit(aa->thread);
		aa->initialized = false;
	}

#if 0
	uuid = mozart_player_getuuid(amazon_handler);
	if (uuid) {
		while(!strcmp(amazon_handler->uuid, uuid) &&
				mozart_player_getstatus(amazon_handler) != PLAYER_STOPPED)
			usleep(100);
		free(uuid);
	}
	mozart_player_handler_put(amazon_handler);
	amazon_handler = NULL;
#endif

	if (record_buf) {
		free(record_buf);
		record_buf = NULL;
	}
	aa_free_authorization( aa );
	aa_free_token( aa );
	aa->state = AA_STATE_SHUTDOWN;
	alexa_mutex_unlock(aa->main_mutex);

	return 0;
}

int aa_service_wakeup(struct aa_service* aa)
{
	int ret = 0;
	alexa_mutex_lock(aa->main_mutex);
	if (aa->initialized) {
		if (aa->state == AA_STATE_IDLE) {
			aa->state = AA_STATE_RECOG;
			aa->recog_exit = false;
		}
	} else {
		ret = -1;
	}
	alexa_mutex_unlock(aa->main_mutex);
	return ret;
}

void aa_service_cancel(struct aa_service* aa)
{
	alexa_mutex_lock(aa->main_mutex);
	aa->recog_exit = true;
	alexa_mutex_unlock(aa->main_mutex);
}

enum aa_state_type aa_get_status(struct aa_service* aa)
{
	return aa->state;
}

static void aa_parse_auth(struct aa_service* aa, cJSON* auth_json)
{
    struct aa_authorization* authorization = &aa->authorization;

    authorization->authorizationCode = cjson_parse_dup_string(auth_json, "authorizationCode");
    if( authorization->authorizationCode ) printf( "authorization->authorizationCode:%s\n", authorization->authorizationCode );
    authorization->redirectUri = cjson_parse_dup_string(auth_json, "redirectUri");
    if( authorization->redirectUri ) printf( "authorization->redirectUri:%s\n", authorization->redirectUri );
    authorization->clientId = cjson_parse_dup_string(auth_json, "clientId");
    if( authorization->clientId ) printf( "authorization->clientId:%s\n", authorization->clientId );
    authorization->codeVerifier = cjson_parse_dup_string(auth_json, "codeVerifier");
    if( authorization->codeVerifier ) printf( "authorization->codeVerifier:%s\n", authorization->codeVerifier );
}


static void aa_parse_token(struct aa_service* aa, cJSON* token_json)
{
    struct aa_token* token = &aa->token;
    cJSON* expires_in_json;
	cJSON* current_time_json;

    token->access_token = cjson_parse_dup_string(token_json, "access_token");
    if( token->access_token ) printf( "token->access_token:%s\n", token->access_token );
    token->refresh_token = cjson_parse_dup_string(token_json, "refresh_token");
    if( token->refresh_token ) printf( "token->refresh_token:%s\n", token->refresh_token );
    token->token_type = cjson_parse_dup_string(token_json, "token_type");
    if( token->token_type ) printf( "token->token_type:%s\n", token->token_type );
    expires_in_json = cJSON_GetObjectItem(token_json, "expires_in");
    if( expires_in_json )
    {
        int expires_in = expires_in_json->valueint;    
        token->expires_in = expires_in;
    }
	current_time_json = cJSON_GetObjectItem(token_json, "current_time");
	if( current_time_json )
	{
		int current_time = current_time_json->valueint;
		token->current_time = current_time;
	}
}


/**
 @brief get the authorization from the exist alexa.key file
 */
static int aa_get_authorization(struct aa_service* aa, const char* file)
{
    FILE* fp;
    int length;
    char* buf = NULL;
    
    cJSON* root_json;
    
	aa_free_authorization(aa);
    aa_free_token(aa);
    
	fp = fopen( file, "rb" );
    if ( fp )
    {
        fseek( fp, 0, SEEK_END );
        length = ftell( fp );
        fseek( fp, 0, SEEK_SET );

        buf = (char*)malloc( length + 1 );

        fread( buf , 1, length, fp );
        fclose(fp);
    }
    else
    {
        goto err;
    }

    if ( !buf )
    {
        goto err;
    }

    root_json = cJSON_Parse(buf);
    if( root_json )
    {
        cJSON* auth_json = cJSON_GetObjectItem(root_json, "authorization");
        cJSON* token_json = cJSON_GetObjectItem(root_json, "token");

        aa_parse_auth( aa, auth_json );
        aa_parse_token( aa, token_json );

        free(buf);
        cJSON_Delete(root_json);

		//need check the expires_in
		if( aa->token.expires_in )
		{
		}
    }
    else
    {
        free(buf);
        goto err;
    }

	return 0;

err:
	return -1;
}

void aa_set_authorization(char *authorizationCode, char *redirectUri, 
        char *clientId, char *codeVerifier)
{
	char *cmd_str = NULL;
	int cmd_len = 0;
	char *response = NULL;

	struct aa_service* aa = (struct aa_service*)malloc( sizeof(struct aa_service) );
	if( !aa )
	{
		goto err;
	}

	cmd_len = strlen(authorizationCode) + strlen(redirectUri) + strlen(clientId) + strlen(codeVerifier) + 256;
	cmd_str = (char*)malloc(cmd_len);
	memset(cmd_str, 0, cmd_len);
	sprintf(cmd_str, "grant_type=authorization_code&code=%s&code_verifier=%s&client_id=%s&redirect_uri=%s", authorizationCode, codeVerifier, clientId, redirectUri);

	response = curl_send_post("https://api.amazon.com/auth/o2/token", cmd_str);
	if (response == NULL) {
		printf("no response detected!\n");
		goto err_response;
	}
	parse_token_response( aa, response );

	{
		struct aa_authorization* auth = (struct aa_authorization*)&(aa->authorization);

		auth->authorizationCode = strdup(authorizationCode);
		auth->redirectUri = strdup(redirectUri);
		auth->clientId = strdup(clientId);
		auth->codeVerifier = strdup(codeVerifier);

		aa_service_save_key( aa, ALEXA_KEY_PATH );
	}

err_response:
	free(cmd_str);
	free(response);

err:

	return;
}


/**
 @brief
 from :https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/docs/authorizing-your-alexa-enabled-product-from-an-android-or-ios-mobile-app
 
 this struct like the link 
 */
struct alexa_authorization{
	char *authorizationCode;
	char *redirectUri;
	char *clientId;
	char *codeVerifier;
};

/** 
 */
struct alexa_token {
	char *access_token;
	char *refresh_token;
	char *token_type;
	time_t	 current_time; //second
	long int expires_in;   //second
};

struct alexa_authmng{
    struct alexa_authorization auth;
    struct alexa_token         token;
};

static int authmng_save2file( cJSON* root_json, const char* file )
{
	FILE* fp;
	char *out = cJSON_Print(root_json);
	if( !out )
	{
		goto err;
	}

	fp = fopen( file, "wb" );
	if (!fp)
	{
		goto err2;
	}

	fwrite( out, 1, strlen(out), fp  );
	fclose( fp );

	alexa_free( out );

	return 0;
err2:
	alexa_free( out );
err:
	return -1;
}

static int authmng_save_key_file( struct alexa_authmng* authmng, const char* file )
{
    //ignore the cJSON error
	struct cJSON* cj_root = cJSON_CreateObject();
	struct cJSON* cj_auth = cJSON_CreateObject();
	struct cJSON* cj_token = cJSON_CreateObject();
	struct alexa_authorization* auth = &authmng->auth;
	struct alexa_token* token = &authmng->token;

	cJSON_AddItemToObject(cj_root, "authorization", cj_auth);
	cJSON_AddItemToObject(cj_auth, "authorizationCode", cJSON_CreateString(auth->authorizationCode));
	cJSON_AddItemToObject(cj_auth, "redirectUri", cJSON_CreateString(auth->redirectUri));
	cJSON_AddItemToObject(cj_auth, "clientId", cJSON_CreateString(auth->clientId));
	cJSON_AddItemToObject(cj_auth, "codeVerifier", cJSON_CreateString(auth->codeVerifier));

	cJSON_AddItemToObject(cj_root, "token", cj_token);
	cJSON_AddItemToObject(cj_token, "access_token", cJSON_CreateString(token->access_token));
	cJSON_AddItemToObject(cj_token, "refresh_token", cJSON_CreateString(token->refresh_token));
	cJSON_AddItemToObject(cj_token, "token_type", cJSON_CreateString(token->token_type));
	cJSON_AddNumberToObject(cj_token, "expires_in", token->expires_in);
	cJSON_AddNumberToObject(cj_token, "current_time", token->current_time);

    if( auth_save2file( cj_root, file ) < 0 )
    {
        sys_log_e( TAG, "save the auth fail.\n" );
    }

	cJSON_Delete(cj_root);
}

static void authmng_parse_auth(struct alexa_authmng* authmng, cJSON* cj_auth)
{
    struct alexa_authorization* authorization = &authmng->auth;

    authorization->authorizationCode = authmng_cjson_dup_string(cj_auth, "authorizationCode");
    if( authorization->authorizationCode )
    {
        sys_log_i( TAG, "authorizationCode:%s\n", authorization->authorizationCode );
    }
    
    authorization->redirectUri = authmng_cjson_dup_string(cj_auth, "redirectUri");
    if( authorization->redirectUri )
    {
        sys_log_i( TAG, "redirectUri:%s\n", authorization->redirectUri );
    
    }
    
    authorization->clientId = authmng_cjson_dup_string(cj_auth, "clientId");
    if( authorization->clientId ) 
    {
        sys_log_i( TAG, "clientId:%s\n", authorization->clientId );
    }
    
    authorization->codeVerifier = authmng_cjson_dup_string(cj_auth, "codeVerifier");
    if( authorization->codeVerifier ) 
    {
        sys_log_i( TAG, "codeVerifier:%s\n", authorization->codeVerifier );
    }
    return ;
}

static void authmng_parse_token(struct alexa_authmng* authmng, cJSON* cj_token)
{
    struct alexa_token* token = &authmng->token;
    cJSON* cj_expires_in;
	cJSON* cj_current_time;

    token->access_token = authmng_cjson_dup_string(cj_token, "access_token");
    if( token->access_token )
    {
        sys_log_i( TAG, "access_token:%s\n", token->access_token );
    }
    
    token->refresh_token = authmng_cjson_dup_string(cj_token, "refresh_token");
    if( token->refresh_token )
    {
        sys_log_i( TAG, "refresh_token:%s\n", token->refresh_token );
    }
    
    token->token_type = authmng_cjson_dup_string(cj_token, "token_type");
    if( token->token_type )
    {
        sys_log_i( TAG, "token_type:%s\n", token->token_type );
    }
    
    cj_expires_in = cJSON_GetObjectItem(cj_token, "expires_in");
    if( cj_expires_in )
    {
        int expires_in = cj_expires_in->valueint;    
        token->expires_in = expires_in;
    }
    
	cj_current_time = cJSON_GetObjectItem(cj_token, "current_time");
	if( cj_current_time )
	{
		int current_time = cj_current_time->valueint;
		token->current_time = current_time;
	}
    
    return;
}

static int authmng_load_key( struct alexa_authmng* authmng, const char* file )
{
    FILE* fp;
    int length;
    char* buf = NULL;
    cJSON* cj_root;
    
	fp = fopen( file, "rb" );
    if ( fp )
    {
        fseek( fp, 0, SEEK_END );
        length = ftell( fp );
        fseek( fp, 0, SEEK_SET );

        buf = (char*)alexa_malloc( length + 1 );

        fread( buf , 1, length, fp );
        fclose(fp);
    }
    else
    {
        goto err;
    }

    if ( !buf )
    {
        goto err;
    }

    cj_root = cJSON_Parse(buf);
    if( cj_root )
    {
        cJSON* cj_auth = cJSON_GetObjectItem(cj_root, "authorization");
        cJSON* cj_token = cJSON_GetObjectItem(cj_root, "token");

        authmng_parse_auth( authmng, cj_auth );
        authmng_parse_token( authmng, cj_token );

        alexa_free(buf);
        cJSON_Delete(cj_root);

		//need check the expires_in
		if (TODO && authmng->token.expires_in)
		{
		}
    }
    else
    {
        alexa_free(buf);
        goto err;
    }

	return 0;

err:
	return -1;
}


static int authmng_parse_token_response(struct alexa_authmng* authmng, char *response)
{
	struct alexa_token* token = &authmng->token;

	cJSON * cj_root;
	cJSON * cj_expires_in;
	char*	error;
	char*	error_desc;

	char*	access_token;
	char*	refresh_token;
	char*	token_type;

	int		expires_in = 3600;
	cj_root = cJSON_Parse(response);
	if( cj_root )
	{
		error_desc = authmng_cjson_dup_string(cj_root, "error_description");
		error = authmng_cjson_dup_string(cj_root, "error");

		access_token = authmng_cjson_dup_string(cj_root, "access_token");
		refresh_token = authmng_cjson_dup_string(cj_root, "refresh_token");
		token_type = authmng_cjson_dup_string(cj_root, "token_type");
		cj_expires_in = cJSON_GetObjectItem(cj_root, "expires_in");
		if( cj_expires_in ) expires_in = cj_expires_in->valueint;
		//timestamp
		token->current_time = time( NULL );

		if(error_desc || error) 
		{
			sys_log_e( TAG, "parse response error:%s desc:%s\n", error == NULL ? "NULL" : error, error_desc == NULL ? "NULL" : error_desc );
			if(error_desc) alexa_free(error_desc);
			if(error) alexa_free(error);
			goto err;
		}
		else if( access_token && refresh_token && token_type )
		{
			if(access_token) token->access_token = access_token;
			if(refresh_token) token->refresh_token = refresh_token;
			if(token_type) token->token_type = token_type;
			token->expires_in = expires_in;
		}
		cJSON_Delete(cj_root);
	}

	return 0;
err:
	return -1;
}

/*
 *@brief
 *
 *@param struct authmng* the auth manager object
 *@param
 *      authorization_code
 *      refresh_token
 *@return 
 */

static int authmng_get_token(struct alexa_authmng* authmng, const char* grant_type)
{
	struct alexa_authorization *authorization = &authmng->auth;
	struct alexa_token *token = &authmng->token;

	char *response = NULL;
	char *cmd_str = NULL;
	int cmd_len = 0, ret = 0;

	if( !authorization->clientId || 
        !authorization->codeVerifier || 
        !authorization->redirectUri ) 
    {
        sys_log_e( TAG, "Don't have the auth value\n" );
        ret = -1;
        goto err;
	}

	cmd_len = 0;
    if( !strcmp( grant_type, "authorization_code" ) )
    {
        if( !authorization->authorizationCode )
        {
            sys_log_e( TAG, "Don't have the authorizationCode value\n" );
            ret = -1;
            goto err;
        }
        
        cmd_len += strlen(authorization->authorizationCode);
    }
    else
    {
        if( !token->refresh_token )
        {
            sys_log_e( TAG, "Don't have the refresh_token value\n" );
            ret = -1;
            goto err;
        }
        
        cmd_len += strlen(token->refresh_token);
    }
	cmd_len += strlen(authorization->clientId);
	cmd_len += strlen(authorization->codeVerifier);
	cmd_len += strlen(authorization->redirectUri);
	cmd_len += 256;

	cmd_str = (char*)alexa_malloc(cmd_len);
	if ( cmd_str )
	{
        char* cmd_str_tmp = cmd_str;
		memset(cmd_str, 0, cmd_len);

        cmd_str_tmp += sprintf( cmd_str_tmp, "grant_type=%s", grant_type );
        if( !strcmp( grant_type, "authorization_code" ) )
        {
            //authorization_code
            cmd_str_tmp += sprintf( cmd_str_tmp, "&code=%s", authorization->authorizationCode );
        }
        else
        {
            //refresh_token
            cmd_str_tmp += sprintf( cmd_str_tmp, "&refresh_token=%s", token->refresh_token );
        }
        
        cmd_str_tmp += sprintf( cmd_str_tmp, "&client_id=%s", authorization->clientId );
        cmd_str_tmp += sprintf( cmd_str_tmp, "&code_verifier=%s", authorization->codeVerifier );
        cmd_str_tmp += sprintf( cmd_str_tmp, "&redirect_uri=%s", authorization->redirectUri );

		response = authmng_curl_send_post(ALEXA_AHTH_URL, cmd_str);
		if (response == NULL) {
			sys_log_e(TAG, "refresh token: no response detected!\n");
            ret = -2;
            goto err2;
		}
        
		ret = authmng_parse_token_response(authmng, response);
		if(ret < 0)
		{
			printf("parse refresh token fail\n");
            ret = -3;
            goto err3;
		}

		authmng_save_key_file(authmng, ALEXA_KEY_PATH);

		alexa_free(cmd_str);
		alexa_free(response);
	}

    return 0;

err3:
    alexa_free( response );
err2:
    alexa_free( cmd_str );
err:
	return ret;
}

static int authmng_refresh_token(struct alexa_authmng* authmng)
{
    return authmng_get_token( authmng, "refresh_token" );
}

static int authmng_authorization_code(struct alexa_authmng* authmng)
{
    return authmng_get_token( authmng, "authorization_code" );
}

alexa_authmng_cancel()
{
    
}

alexa_authmng_init()
{
	struct alexa_authmng* authmng;

    //
    if( authmng_load_key( authmng, ALEXA_KEY_PATH ) < 0 )
    {
        sys_log_e( TAG, "auth chenk key fail.\n" );
        
        //generate the auth infomation
        //codeVerifier
        //codeChallenge
        //codeChallengeMethod
        //sessionId
        
        
        authmng_authorization_code(authmng);
    }
    
    authmng_refresh_token(authmng);
    
    //make device can discover
    
    //send the device auth infomation
    
    //recive the auth code from phone 
    
    //using the auth code connect with avs

    return 0;
}


/*******************************************************************************
	END OF FILE
*******************************************************************************/
