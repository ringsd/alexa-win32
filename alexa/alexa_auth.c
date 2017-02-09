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
#include "alexa_base.h"
#include "alexa_platform.h"
#include "alexa_device.h"

#include "sys_log.h"

#define TODO 1

#define TAG    "auth"

#ifdef WIN32
#define ALEXA_CONFIG_PATH                    "alexa.json"
#else
#define ALEXA_CONFIG_PATH                  "/usr/resource/alexa.json"
#endif

#define ALEXA_AHTH_URL  "https://api.amazon.com/auth/o2/token"

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
    int expires_in;   //second
    time_t   current_time; //second
};

struct alexa_authmng{
    struct alexa_device*       device;
    struct alexa_authorization auth;
    struct alexa_token         token;
};

static char* authmng_cjson_get_string(cJSON* json, const char* key)
{
    cJSON* item = cJSON_GetObjectItem(json, key);
    if( item )
    {
        return item->valuestring;
    }
    return NULL;
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

    new_response = (char*)alexa_malloc(response_len + size * nmemb + 1);
    if (new_response == NULL) {
        alexa_free(response);
        *(char **)userp = NULL;
        return 0;
    }

    if (response) {
        memcpy(new_response, response, response_len);
        alexa_free(response);
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

static int authmng_json_save2file( cJSON* root_json, const char* file )
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

static int authmng_save_config_file( struct alexa_authmng* authmng, const char* file )
{
    int ret = 0;
    struct cJSON* cj_root = cJSON_CreateObject();
    struct cJSON* cj_device = cJSON_CreateObject();
    struct cJSON* cj_auth = cJSON_CreateObject();
    struct cJSON* cj_token = cJSON_CreateObject();
    struct alexa_authorization* auth = &authmng->auth;
    struct alexa_token* token = &authmng->token;
    struct alexa_device* device = authmng->device;

    cJSON_AddItemToObject(cj_root, "device", cj_device);
    cJSON_AddItemToObject(cj_device, "manufacturer", cJSON_CreateString(device->manufacturer));
    cJSON_AddItemToObject(cj_device, "product", cJSON_CreateString(device->product));
    cJSON_AddItemToObject(cj_device, "model", cJSON_CreateString(device->model));
    cJSON_AddItemToObject(cj_device, "codeVerifier", cJSON_CreateString(device->codeVerifier));
    cJSON_AddItemToObject(cj_device, "codeChallenge", cJSON_CreateString(device->codeChallenge));
    cJSON_AddItemToObject(cj_device, "codeChallengeMethod", cJSON_CreateString(device->codeChallengeMethod));
    cJSON_AddItemToObject(cj_device, "sessionId", cJSON_CreateString(device->sessionId));

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
    cJSON_AddNumberToObject(cj_token, "current_time", (double)token->current_time);

    if (authmng_json_save2file(cj_root, file) < 0)
    {
        sys_log_e( TAG, "save the auth fail.\n" );
        ret = -1;
    }

    cJSON_Delete(cj_root);
    return ret;
}

static void authmng_auth_free(struct alexa_authorization* auth)
{
    ALEXA_SAFE_FREE(auth->authorizationCode);
    ALEXA_SAFE_FREE(auth->redirectUri);
    ALEXA_SAFE_FREE(auth->clientId);
    ALEXA_SAFE_FREE(auth->codeVerifier);
}

static void authmng_token_free(struct alexa_token* token)
{
    ALEXA_SAFE_FREE(token->access_token);
    ALEXA_SAFE_FREE(token->refresh_token);
    ALEXA_SAFE_FREE(token->token_type);
}

static void authmng_parse_device(struct alexa_device* device, cJSON* cj_device)
{
    //device information
    alexa_device_info_set(device, 
                        authmng_cjson_get_string(cj_device, "manufacturer"), 
                        authmng_cjson_get_string(cj_device, "product"), 
                        authmng_cjson_get_string(cj_device, "model"));

    //device auth information
    alexa_device_code_set(device, 
                        authmng_cjson_get_string(cj_device, "codeVerifier"), 
                        authmng_cjson_get_string(cj_device, "codeChallenge"), 
                        authmng_cjson_get_string(cj_device, "codeChallengeMethod"));
    
    alexa_device_sessionid_set(device, 
                        authmng_cjson_get_string(cj_device, "sessionId"));

    return ;
}

static void authmng_parse_auth(struct alexa_authorization* authorization, cJSON* cj_auth)
{
    authorization->authorizationCode = authmng_cjson_dup_string(cj_auth, "authorizationCode");
    if( authorization->authorizationCode ) sys_log_i( TAG, "authorizationCode:%s\n", authorization->authorizationCode );

    authorization->redirectUri = authmng_cjson_dup_string(cj_auth, "redirectUri");
    if( authorization->redirectUri ) sys_log_i( TAG, "redirectUri:%s\n", authorization->redirectUri );

    authorization->clientId = authmng_cjson_dup_string(cj_auth, "clientId");
    if( authorization->clientId ) sys_log_i( TAG, "clientId:%s\n", authorization->clientId );

    authorization->codeVerifier = authmng_cjson_dup_string(cj_auth, "codeVerifier");
    if( authorization->codeVerifier ) sys_log_i( TAG, "codeVerifier:%s\n", authorization->codeVerifier );

    return ;
}

static void authmng_parse_token(struct alexa_token* token, cJSON* cj_token)
{
    cJSON* cj_expires_in;
    cJSON* cj_current_time;

    token->access_token = authmng_cjson_dup_string(cj_token, "access_token");
    if( token->access_token ) sys_log_i( TAG, "access_token:%s\n", token->access_token );
    
    token->refresh_token = authmng_cjson_dup_string(cj_token, "refresh_token");
    if( token->refresh_token ) sys_log_i( TAG, "refresh_token:%s\n", token->refresh_token );
    
    token->token_type = authmng_cjson_dup_string(cj_token, "token_type");
    if( token->token_type ) sys_log_i( TAG, "token_type:%s\n", token->token_type );
    
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

static int authmng_load_config_file( struct alexa_authmng* authmng, const char* file )
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
        cJSON* cj_device = cJSON_GetObjectItem(cj_root, "device");
        cJSON* cj_auth = cJSON_GetObjectItem(cj_root, "authorization");
        cJSON* cj_token = cJSON_GetObjectItem(cj_root, "token");

        authmng_parse_device( authmng->device, cj_device );
        authmng_parse_auth( &authmng->auth, cj_auth );
        authmng_parse_token( &authmng->token, cj_token );

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
    char*    error;
    char*    error_desc;

    char*    access_token;
    char*    refresh_token;
    char*    token_type;

    int        expires_in = 3600;
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
            ALEXA_SAFE_FREE(error_desc);
            ALEXA_SAFE_FREE(error);
            cJSON_Delete(cj_root);
            goto err;
        }
        else if( access_token && refresh_token && token_type )
        {
            authmng_token_free(token);
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

        authmng_save_config_file(authmng, ALEXA_CONFIG_PATH);

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

static void authmng_auth_set(struct alexa_authorization* auth, const char* authorizationCode, const char* redirectUri, const char* clientId, const char* codeVerifier)
{
    auth->authorizationCode = alexa_strdup(authorizationCode);
    auth->redirectUri = alexa_strdup(redirectUri);
    auth->clientId = alexa_strdup(clientId);
    auth->codeVerifier = alexa_strdup(codeVerifier);
}

static void authmng_token_set(struct alexa_token* token, const char* access_token, const char* refresh_token, const char* token_type)
{
    token->access_token = alexa_strdup(access_token);
    token->refresh_token = alexa_strdup(refresh_token);
    token->token_type = alexa_strdup(token_type);
    token->expires_in = 0;
    token->current_time = 0;
}

const char*alexa_authmng_get_access_token(struct alexa_authmng* authmng)
{
    return authmng->token.access_token;
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


static void alexa_authmng_destruct(struct alexa_authmng* authmng)
{
    authmng_auth_free(&authmng->auth);
    authmng_token_free(&authmng->token);
    alexa_delete(authmng);
}

struct alexa_authmng* alexa_authmng_init(void)
{
    struct alexa_authmng* authmng;
    struct alexa_device* device;

    authmng = alexa_authmng_construct();
    if( authmng == NULL ) goto err;
    
    device = alexa_device_construct();
    if( device == NULL ) goto err2;
    
    authmng->device = device;

    //load alexa auth information
    if (authmng_load_config_file(authmng, ALEXA_CONFIG_PATH) < 0)
    {
        //first load or some error happen
        sys_log_e( TAG, "auth chenk config fail.\n" );

        //generate the device code for
        //set device info
        alexa_device_code_regenerate(device);
        alexa_device_info_set(device, "HiBy Music", "HiBy Music Player", "G10" );
        alexa_device_sessionid_set(device, "HiBy Music");

        //char* authcode = "ANsdtryAfaZSPGbuKBYF";
        //char* redirectUri = "amzn://com.amazon.alexa.avs.companion";
        //char* clientID = "amzn1.application-oa2-client.287ac689b0114220b68ff67ffefac322";
        //char* codeVerifier = "TkpLH7myIpBE5M4MVjRoRgENnNRaf4VjAxffQ9u1Jt4";
        authmng_auth_set(&authmng->auth, 
                        "ANsJYGGnTIkLHWtIFawt", 
                        "amzn://com.amazon.alexa.avs.companion", 
                        "amzn1.application-oa2-client.287ac689b0114220b68ff67ffefac322", 
                        "F3c7aEv0MnirJjiS0N9IPN30PlbxqU_TzIbmVps0LyA");

        alexa_device_start_discovery(device);

        //wait the app auth the device
        alexa_device_app_auth(device);
        
        //get the authorization code
        authmng_authorization_code(authmng);
    }
    else
    {
        alexa_device_start_discovery(device);
        
        //get the refresh code
        authmng_refresh_token(authmng);
    }

    return authmng;

err2:
    alexa_authmng_destruct(authmng);
err:
    return NULL;
}

void alexa_authmng_done(struct alexa_authmng* authmng)
{
    alexa_device_stop_discovery(authmng->device);
    alexa_device_destruct(authmng->device);

    alexa_delete(authmng);
}

#ifdef ALEXA_UNIT_TEST

const char* sessionIdTest = "123456";
const char* clientIdTest = "amzn1.application-oa2-client.287ac689b0114220b68ff67ffefac322";
const char* redirectUriTest = "amzn://com.amazon.alexa.avs.companion";
const char* authorizationCodeTest = "ANsJYGGnTIkLHWtIFawt";
const char* codeVerifierTest = "F3c7aEv0MnirJjiS0N9IPN30PlbxqU_TzIbmVps0LyA";
const char* codeChallengeTest = "2uc6zTIJz9Vpmgybc-eoA8TcNs8bA1C3QONL0G7QTak";
const char* codeChallengeMethodTest = "S256";

void alexa_authmng_test(void)
{
    struct alexa_authmng* authmng;
    struct alexa_device* device;

    authmng = alexa_authmng_construct();
    device = alexa_device_construct();
    authmng->device = device;

    authmng_load_config_file(authmng, ALEXA_CONFIG_PATH);

    authmng_auth_free( &authmng->auth);
    authmng_token_free(&authmng->token);

    if (authmng->device->manufacturer == NULL)
    {
        //set device info
        alexa_device_code_regenerate(authmng->device);
        alexa_device_info_set(authmng->device, "HiBy Music", "HiBy Music Player", "G10");
        alexa_device_sessionid_set(authmng->device, "HiBy Music");
    }
    authmng_auth_set(&authmng->auth, "ANsdtryAfaZSPGbuKBYF", "amzn://com.amazon.alexa.avs.companion", "amzn1.application-oa2-client.287ac689b0114220b68ff67ffefac322", "TkpLH7myIpBE5M4MVjRoRgENnNRaf4VjAxffQ9u1Jt4");
    authmng_token_set(&authmng->token, "access_token", "refresh_token", "token_type");

    authmng_save_config_file(authmng, ALEXA_CONFIG_PATH);

    alexa_device_destruct(authmng->device);
    alexa_authmng_destruct(authmng);
}
#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
