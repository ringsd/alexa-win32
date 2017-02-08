/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_alerts.h

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/16 10:42:09

*******************************************************************************/

#ifndef _alexa_alerts_h_
#define _alexa_alerts_h_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct alexa_alerts alexa_alerts;

cJSON* alerts_alerts_state(struct alexa_service* as);

int alexa_alerts_init(struct alexa_service* as);

int alexa_alerts_done(struct alexa_service* as);

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
