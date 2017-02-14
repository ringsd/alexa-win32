/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_speechsynthesizer.h

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/14 18:01:10

*******************************************************************************/

#ifndef _alexa_speechsynthesizer_h_
#define _alexa_speechsynthesizer_h_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct alexa_speechsynthesizer alexa_speechsynthesizer;

cJSON* speechsynthesizer_speech_state(struct alexa_speechsynthesizer* as);

struct alexa_speechsynthesizer* alexa_speechsynthesizer_init(alexa_service* as);

int alexa_speechsynthesizer_done(struct alexa_speechsynthesizer* ss);

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
