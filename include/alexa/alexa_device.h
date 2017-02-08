/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_device.h

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/2/08 15:18:23

*******************************************************************************/

#ifndef _alexa_device_h_
#define _alexa_device_h_

#ifdef __cplusplus
extern "C" {
#endif

struct alexa_device{
    char* manufacturer; //HiBy Music
    char* product;//HiBy Music Player
    char* model;//HBP

    char* codeVerifier;
    char* codeChallenge;
    char* codeChallengeMethod;
    char* sessionId;
};


void alexa_device_code_set( struct alexa_device* device, const char* code_verifier, const char* code_challenge, const char* code_challenge_method );

void alexa_device_code_regenerate(struct alexa_device* device);

void alexa_device_info_set( struct alexa_device* device, const char* manufacturer, const char* product, const char* model );

void alexa_device_sessionid_set( struct alexa_device* device, const char* sessionid );


struct alexa_device* alexa_device_construct( void );

void alexa_device_destruct( struct alexa_device* device );

void alexa_device_start_discovery( struct alexa_device* device );

void alexa_device_stop_discovery(struct alexa_device* device);

void alexa_device_app_auth( struct alexa_device* device );

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
