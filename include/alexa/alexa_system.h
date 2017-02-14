/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_system.h

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/14 16:43:39

*******************************************************************************/

#ifndef _alexa_system_h_
#define _alexa_system_h_

#ifdef __cplusplus
extern "C" {
#endif

struct alexa_system* alexa_system_init(struct alexa_service* as);
int alexa_system_done(struct alexa_system* system);

const char* alexa_system_synchronizestate_construct(struct alexa_system* system);


const char* alexa_system_synchronizestate_event(struct alexa_system* system);

const char* alexa_system_userinactivityreport_event(struct alexa_system* system);

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
