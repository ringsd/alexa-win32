/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_speechrecognizer.h

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/14 17:52:37

*******************************************************************************/

#ifndef _alexa_speechrecognizer_h_
#define _alexa_speechrecognizer_h_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct alexa_speechrecognizer alexa_speechrecognizer;


struct alexa_speechrecognizer* alexa_speechrecognizer_init(void);

int alexa_speechrecognizer_done(struct alexa_speechrecognizer* sr);

void alexa_speechrecognizer_process(struct alexa_service* as);

void alexa_speechrecognizer_user_wake_up(struct alexa_service* as);

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
