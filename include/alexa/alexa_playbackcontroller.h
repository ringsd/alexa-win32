/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_playbackcontroller.h

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/14 17:00:58

*******************************************************************************/

#ifndef _alexa_playbackcontroller_h_
#define _alexa_playbackcontroller_h_

#ifdef __cplusplus
extern "C" {
#endif

enum PLAYBACKCONTROLLER_EVENT_ENUM{
    PLAYCOMMANDISSUED_EVENT = 0,
    PAUSECOMMANDISSUED_EVENT,
    NEXTCOMMANDISSUED_EVENT,
    PREVIOUSCOMMANDISSUED_EVENT,
};

typedef struct alexa_playbackcontroller alexa_playbackcontroller;


int alexa_pc_init(struct alexa_service* as);

int alexa_pc_done(struct alexa_service* as);

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
