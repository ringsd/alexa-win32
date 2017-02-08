/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_device.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/02/07 14:24:18

*******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include "base64.h"
#include "alexa_platform.h"
#include "alexa_base.h"
#include "alexa_device.h"
#include "openssl/sha.h"

#define ALEXA_CODE_CHALLENGE_METHOD_S256    "S256"
#define ALEXA_CODE_CHALLENGE_METHOD         ALEXA_CODE_CHALLENGE_METHOD_S256

//generate a code verifier
//need release by user
static char* generate_code_verifier(void)
{
    int i = 0;
    int encode_len;
    unsigned char code_verifier_number[32];
    char* code_verifier;
    //random generate 32 byte number
    for (i = 0; i < sizeof(code_verifier_number); i++)
    {
        code_verifier_number[i] = rand();
    }

    //base 64 url encode 
    encode_len = base64_encode_len(sizeof(code_verifier_number));
    code_verifier = alexa_malloc(encode_len);
	encode_len = base64_encode_urlsafe(code_verifier, code_verifier_number, sizeof(code_verifier_number));
    return code_verifier;
}

static char* generate_code_challenge(const char* code_verifier, const char* code_challenge_method)
{
    int encode_len;
    unsigned char code_challenge_number[32];
    char *code_challenge;
	if (strcmp(code_challenge_method, ALEXA_CODE_CHALLENGE_METHOD_S256) == 0)
    {
		SHA256(code_verifier, strlen(code_verifier), code_challenge_number);
    }
    else
    {
        //error
    }

    //base 64 url encode 
    encode_len = base64_encode_len(sizeof(code_challenge_number));
    code_challenge = alexa_malloc(encode_len);
	encode_len = base64_encode_urlsafe(code_challenge, code_challenge_number, sizeof(code_challenge_number));

    return code_challenge;
}

void alexa_device_code_set( struct alexa_device* device, const char* code_verifier, const char* code_challenge, const char* code_challenge_method )
{
    device->codeVerifier = alexa_strdup(code_verifier);
    device->codeChallenge = alexa_strdup(code_challenge);
    device->codeChallengeMethod = alexa_strdup(code_challenge_method);
}

void alexa_device_code_regenerate(struct alexa_device* device)
{
    device->codeVerifier = generate_code_verifier();
    device->codeChallengeMethod = alexa_strdup(ALEXA_CODE_CHALLENGE_METHOD);
    device->codeChallenge = generate_code_challenge(device->codeVerifier, device->codeChallengeMethod);
}

void alexa_device_info_set( struct alexa_device* device, const char* manufacturer, const char* product, const char* model )
{
    device->manufacturer = alexa_strdup(manufacturer);
    device->product = alexa_strdup(product);
    device->model = alexa_strdup(model);
}

void alexa_device_sessionid_set( struct alexa_device* device, const char* sessionid )
{
    device->sessionId = alexa_strdup(sessionid);
}

struct alexa_device* alexa_device_construct( void )
{
    struct alexa_device* device = alexa_new(struct alexa_device);
    return device;
}

void alexa_device_destruct( struct alexa_device* device )
{
    if( device )
    {
		ALEXA_SAFE_FREE(device->manufacturer);
		ALEXA_SAFE_FREE(device->product);
		ALEXA_SAFE_FREE(device->model);
        
		ALEXA_SAFE_FREE(device->codeVerifier);
		ALEXA_SAFE_FREE(device->codeChallenge);
		ALEXA_SAFE_FREE(device->codeChallengeMethod);
		ALEXA_SAFE_FREE(device->sessionId);
        
        alexa_delete(device);
    }
}

void alexa_device_start_discovery( struct alexa_device* device )
{
    //device broadcast online, wait app find devcie
    
    //app connect device
    
    //app get the device infomation
    
    //wait app send token
    
    //set the token to the alexa_service
}

void alexa_device_stop_discovery(struct alexa_device* device)
{
	//disconnect the app

	//stop broadcast online
}

void alexa_device_app_auth( struct alexa_device* device )
{
}


/*******************************************************************************
    END OF FILE
*******************************************************************************/
