/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_service.h

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/14 16:10:16

*******************************************************************************/

#ifndef _alexa_service_h_
#define _alexa_service_h_

#ifdef __cplusplus
extern "C" {
#endif

#include    <string.h>
#include    "cjson/cjson.h"
#include    "list.h"
#include    "sys_log.h"

#include    "alexa_platform.h"
#include    "alexa_base.h"

typedef struct alexa_service alexa_service;

#include    "alexa_context.h"
#include    "alexa_speechrecognizer.h"
#include    "alexa_speechsynthesizer.h"
#include    "alexa_alerts.h"
#include    "alexa_audioplayer.h"
#include    "alexa_playbackcontroller.h"
#include    "alexa_speaker.h"
#include    "alexa_system.h"
#include    "alexa_directive.h"

#include    "alexa_auth.h"
#include    "alexa_http2.h"

struct alexa_service{
    struct alexa_http2* http2;
    //SpeechRecognizer
    struct alexa_speechrecognizer* sr;

    //SpeechSynthesizer
    struct alexa_speechsynthesizer* ss;
    
    //Alerts
    struct alexa_alerts* alerts;
    
    //AudioPlayer
    struct alexa_audioplayer* ap;
    
    //PlayerController
    struct alexa_playbackcontroller* pc;
    
    //Speaker
    struct alexa_speaker* speaker;

    //System
    struct alexa_system* system;

    struct alexa_directive* directive;
};


struct alexa_service* alexa_service_init(void);

void alexa_service_process(struct alexa_service* as, struct alexa_http2* http2);

void alexa_service_done(struct alexa_service* as);

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
