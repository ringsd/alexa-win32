/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_http2.h

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/1/18 16:39:09

*******************************************************************************/

#ifndef _alexa_http2_h_
#define _alexa_http2_h_

#ifdef __cplusplus
extern "C" {
#endif

int alexa_http2_init(struct alexa_service* as);

void alexa_http2_done(struct alexa_service* as);

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
