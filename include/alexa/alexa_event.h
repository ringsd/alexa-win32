/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_event.h

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/2/14 14:25:11

*******************************************************************************/

#ifndef _alexa_event_h_
#define _alexa_event_h_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct alexa_event alexa_event;

struct alexa_event_item{
    struct list_head    list;
    char*               event_data;
    int                 event_len;
    char*               audio_data;
    int                 audio_len;
};

int alexa_event_item_add_event_data(struct alexa_event* event, const char* event_data, int event_len, const char* audio_data, int audio_len);

int alexa_event_item_add_event(struct alexa_event* event, const char* event_data);

struct alexa_event_item* alexa_event_item_get( struct alexa_event* event );

void alexa_event_item_free( struct alexa_event_item* item );

struct alexa_event* alexa_event_init( void );

void alexa_event_done( struct alexa_event* event );


#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
