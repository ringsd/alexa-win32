/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_event.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/02/14 14:25:05

*******************************************************************************/

#include <string.h>
#include "sys_log.h"
#include "list.h"
#include "alexa_platform.h"
#include "alexa_event.h"

#define TAG "alexa_event"

struct alexa_event{
    struct list_head        head;
    struct alexa_mutex*     mutex;
};

int alexa_event_item_add_event_data(struct alexa_event* event, const char* event_data, int event_len, const char* audio_data, int audio_len)
{
    struct alexa_event_item* item;
    
    item = alexa_new(struct alexa_event_item);
    if( !item )
    {
        sys_log_e( TAG, "don't have enough mem\n" );
        goto err;
    }

    sys_log_i( TAG, "%s\n", event_data );

    item->event_data = (char*)event_data;
    item->event_len = event_len;

    item->audio_data = (char*)audio_data;
    item->audio_len = audio_len;

    //add the event to the events list
    alexa_mutex_lock(event->mutex);
    list_add_tail(&(item->list), &event->head);
    alexa_mutex_unlock(event->mutex);

    return 0;

err:
    return -1;
}

int alexa_event_item_add_event(struct alexa_event* event, const char* event_data)
{
    if (event_data)
    {
        return alexa_event_item_add_event_data(event, event_data, strlen(event_data), NULL, 0);
    }
    else
    {
        sys_log_e( TAG, "Add event error: event_data is NLLL.\n" );
        return -1;
    }
}

struct alexa_event_item* alexa_event_item_get( struct alexa_event* event )
{
    struct alexa_event_item* item = NULL;
    //find the first event, and remove from the new list
    alexa_mutex_lock(event->mutex);
    if( !list_empty(&event->head) )
    {
        item = list_first_entry(&event->head, struct alexa_event_item, list);
        list_del(&(item->list));
    }
    alexa_mutex_unlock(event->mutex);
    
    return item;
}

void alexa_event_item_free( struct alexa_event_item* item )
{
    ALEXA_SAFE_FREE(item->event_data);
    ALEXA_SAFE_FREE(item->audio_data);
    alexa_delete(item);
    
    return;
}

static struct alexa_event* event_construct( void )
{
    struct alexa_event* event = alexa_new( struct alexa_event );
    if( event )
    {
        INIT_LIST_HEAD(&event->head);
        event->mutex = alexa_mutex_create();
    }
    return event;
}

static void event_destruct( struct alexa_event* event )
{
    //clear event list
    alexa_mutex_lock(event->mutex);
    for (;;)
    {
        struct alexa_event_item* item = NULL;
        item = alexa_event_item_get(event);
        if (item)
        {
            alexa_event_item_free(item);
        }
        {
            break;
        }
    }
    alexa_mutex_unlock(event->mutex);
    alexa_mutex_destroy(event->mutex);
    alexa_delete(event);
}

struct alexa_event* alexa_event_init( void )
{
    struct alexa_event* event = event_construct();
    if( event == NULL )
    {
        sys_log_e( TAG, "construct alexa event fail.\n" );
    }
    return event;
}

void alexa_event_done( struct alexa_event* event )
{
    event_destruct( event );
    return;
}

/*******************************************************************************
    END OF FILE
*******************************************************************************/
