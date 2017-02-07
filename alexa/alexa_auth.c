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

#include "cjson/cjson.h"
#include "base64.h"
#include "alexa_platform.h"

#define TODO 1

#define TAG	"auth"

#ifdef WIN32
#define ALEXA_KEY_PATH					"alexa.key"
#else
#define ALEXA_KEY_PATH                  "/usr/resource/alexa.key"
#endif

#define ALEXA_AHTH_URL  "https://api.amazon.com/auth/o2/token"

char* alexa_strdup(const char* str)
{
	int len = strlen(str);
	char* actual_str = (char*)alexa_malloc(len + 1);
	if (actual_str)
	{
		strcpy(actual_str, str);
	}
	return actual_str;
}

static char* authmng_cjson_dup_string(cJSON* json, const char* key)
{
	cJSON* item = cJSON_GetObjectItem(json, key);
	if( item )
	{
		return alexa_strdup(item->valuestring);
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

	if (authmng_save2file(cj_root, file) < 0)
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

void alexa_authmng_cancel(void)
{
    
}

static struct alexa_authmng* alexa_authmng_construct(void)
{
	struct alexa_authmng* authmng = alexa_new(struct alexa_authmng);
	if (authmng)
	{

	}
	return authmng;
}

int alexa_authmng_init(void)
{
	struct alexa_authmng* authmng;

	authmng = alexa_authmng_construct();

	//new alexa device

	//load device information

    //load alexa auth information
    if( authmng_load_key( authmng, ALEXA_KEY_PATH ) < 0 )
    {
        sys_log_e( TAG, "auth chenk key fail.\n" );

		//wait the app auth the device

		//get the authorization code
        authmng_authorization_code(authmng);
    }
    
    authmng_refresh_token(authmng);
    
	//start the alexa service

    return 0;
}


/*******************************************************************************
	END OF FILE
*******************************************************************************/
