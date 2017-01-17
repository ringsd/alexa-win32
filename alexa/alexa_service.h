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

#include "cjson.h"

struct alexa_service{
    //SpeechRecognizer
    struct alexa_speechrecoginzer sr;

    //SpeechSynthesizer
    struct alexa_speechsynthesizer ss;
    
    //Alerts
    struct alexa_alerts alerts;
    
    //AudioPlayer
    struct alexa_audioplayer ap;
    
    //PlayerController
    struct alexa_playercontroller playctrl;
    
    //Speaker
    struct alexa_playercontroller speaker;

    //System
    struct alexa_system system;
};

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
	END OF FILE
*******************************************************************************/
