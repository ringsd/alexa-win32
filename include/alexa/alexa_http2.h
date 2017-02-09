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

struct alexa_http2* alexa_http2_init(struct alexa_service* service, struct alexa_authmng* authmng);

void alexa_http2_done(struct alexa_http2* http2);

int alexa_http2_event_audio_add(struct alexa_http2* http2, const char* event, int event_len, const char* audio_data, int audio_data_len);

int alexa_http2_event_add(struct alexa_http2* http2, const char* event, int event_len);

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
