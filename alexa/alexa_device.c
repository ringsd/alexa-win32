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

#define ALEXA_CODE_CHALLENGE_METHOD_S256    "S256"
#define ALEXA_CODE_CHALLENGE_METHOD         ALEXA_CODE_CHALLENGE_METHOD_S256

struct alexa_device{
    char* name;
    char* manufacturer; //HiBy Music
    char* product;//HiBy Music Player
    char* model;//HBP

    char* codeVerifier;
    char* codeChallenge;
    char* codeChallengeMethod;
    char* sessionId;
};

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
    encode_len = base64_encode_urlsafe(code_verifier_number, code_verifier, sizeof(code_verifier_number));
    return code_verifier;
}

static char* generate_code_challenge(const char* code_verifier, const char* code_challenge_method)
{
    int encode_len;
    unsigned char *code_verifier_s256 = NULL;
    unsigned char *code_challenge_number = NULL;
    char *code_challenge;
    if( code_challenge_method == "S256" )
    {
        //
        //sha256 code_verifier
        //code_challenge = code_verifier_s256;
		code_verifier_s256 = code_verifier_s256;
    }
    else
    {
        //error
    }

    //base 64 url encode 
    encode_len = base64_encode_len(sizeof(code_challenge_number));
    code_challenge = alexa_malloc(encode_len);
    encode_len = base64_encode_urlsafe(code_challenge_number, code_challenge, sizeof(code_challenge_number));

    return code_challenge;
}

struct alexa_device* alexa_device_new( void )
{
    struct alexa_device* device = alexa_new(struct alexa_device);
    if (device != NULL )
    {
        device->codeVerifier = generate_code_verifier();
		device->codeChallengeMethod = ALEXA_CODE_CHALLENGE_METHOD;
		device->codeChallenge = generate_code_challenge(device->codeVerifier, device->codeChallengeMethod);
    }
    return device;
}

void alexa_device_delete( struct alexa_device* device )
{
    if( device )
    {
        alexa_delete(device);
    }
}

void alexa_device_discovery( void )
{
    //device broadcast online, wait app find devcie
    
    //app connect device
    
    //app get the device infomation
    
    //wait app send token
    
    //set the token to the alexa_service
}


/*******************************************************************************
    END OF FILE
*******************************************************************************/
