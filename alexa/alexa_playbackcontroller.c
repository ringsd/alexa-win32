/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.

    File: alexa_playbackcontroller.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/13 16:27:07
    https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/playbackcontroller

*******************************************************************************/

#include "alexa_service.h"

#define TAG             "PlaybackController"
#define NAMESPACE       "PlaybackController"

struct alexa_playbackcontroller{
    struct alexa_service* as;
    char messageId[ALEXA_UUID_LENGTH];
};

static const char* playbackcontroller_event[] = {
    "PlayCommandIssued",
    "PauseCommandIssued",
    "NextCommandIssued",
    "PreviousCommandIssued",
};

static cJSON* pc_event_context_construct( struct alexa_playbackcontroller* pc )
{
    return alexa_context_get_state(pc->as);
}

static void pc_event_header_construct(struct alexa_playbackcontroller* pc, cJSON* cj_header, enum PLAYBACKCONTROLLER_EVENT_ENUM event)
{
    int event_index = (int)event;
    
    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", playbackcontroller_event[event_index]);
    cJSON_AddStringToObject( cj_header, "messageId", pc->messageId);

    return;
}

static void pc_event_payload_construct(struct alexa_playbackcontroller* pc, cJSON* cj_payload, enum PLAYBACKCONTROLLER_EVENT_ENUM event)
{
    pc = pc;
    cj_payload = cj_payload;
    event = event;
    return;
}

const char* alexa_pc_event_construct(struct alexa_playbackcontroller* pc, enum PLAYBACKCONTROLLER_EVENT_ENUM event)
{
    char* event_json;
    cJSON* cj_root = cJSON_CreateObject();
    cJSON* cj_context;
    cJSON* cj_event = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();
    
    cj_context = pc_event_context_construct(pc);
    cJSON_AddItemToObject( cj_root, "context", cj_context );
    
    cJSON_AddItemToObject( cj_root, "event", cj_event );
    
    cJSON_AddItemToObject( cj_event, "header", cj_header );
    cJSON_AddItemToObject( cj_event, "payload", cj_payload );

    //
    pc_event_header_construct(pc, cj_header, event);
    pc_event_payload_construct(pc, cj_payload, event);

    event_json = cJSON_Print( cj_root );
    cJSON_Delete( cj_root );

    sys_log_d(TAG, "%s\n", event_json );
    
    return event_json;    
}


static struct alexa_playbackcontroller* pc_construct( void )
{
    struct alexa_playbackcontroller* pc = alexa_new( struct alexa_playbackcontroller );
    if( pc )
    {
        
    }
    return pc;
}

static void pc_destruct( struct alexa_playbackcontroller* pc )
{
    alexa_delete( pc );
}

struct alexa_playbackcontroller* alexa_pc_init(struct alexa_service* as)
{
    struct alexa_playbackcontroller* pc = pc_construct();
    if(pc)
    {
        pc->as = as;
    }
    return pc;
}

int alexa_pc_done(struct alexa_playbackcontroller* pc)
{
    if (pc) pc_destruct(pc);
    return 0;
}

/*******************************************************************************
    END OF FILE
*******************************************************************************/
