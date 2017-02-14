/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_speaker.h

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/1/18 21:47:44

*******************************************************************************/

#ifndef _alexa_speaker_h_
#define _alexa_speaker_h_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct alexa_speaker alexa_speaker;

cJSON* speaker_volume_state(struct alexa_speaker* speaker);

struct alexa_speaker* alexa_speaker_init(alexa_service* as);

int alexa_speaker_done(struct alexa_speaker* speaker);

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
