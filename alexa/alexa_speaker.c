/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.

    File: alexa_speaker.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/13 16:14:32

*******************************************************************************/

#include "alexa_service.h"

#define NAMESPACE       "Speaker"

#define MAX_VOLUME_VAL    100
#define MIN_VOLUME_VAL    0

struct alexa_speaker{
    char* messageId;
    int   volume;
    int   muted;
};

enum SPEAKER_EVENT_ENUM{
    VOLUMECHANGED_EVENT,
    MUTECHANGED_EVENT,
    
    VOLUMESTATE_EVENT,
};

static const char* speaker_event[] = {
    "VolumeChanged",
    "MuteChanged",
    
    "VolumeState",
};

cJSON* speaker_volume_state( alexa_service* as )
{
    struct alexa_speaker* speaker = as->speaker;
    cJSON* cj_volume_state = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();
    
    cJSON_AddItemToObject( cj_volume_state, "header", cj_header );
    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", speaker_event[VOLUMESTATE_EVENT]);
    
    cJSON_AddItemToObject( cj_volume_state, "payload", cj_payload );
    cJSON_AddNumberToObject( cj_payload, "volume", speaker->volume);
    cJSON_AddBoolToObject( cj_payload, "mute", speaker->muted);
    
    return cj_volume_state;
}


static void speaker_event_header_construct( struct alexa_speaker* speaker, cJSON* cj_header, enum SPEAKER_EVENT_ENUM event )
{
    int event_index = (int)event;

    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", speaker_event[event_index]);
    cJSON_AddStringToObject( cj_header, "messageId", speaker->messageId);
    
    return;
}

static void speaker_event_payload_construct(struct alexa_speaker* speaker, cJSON* cj_payload, enum SPEAKER_EVENT_ENUM event)
{
    switch( event )
    {
        case VOLUMECHANGED_EVENT:
        case MUTECHANGED_EVENT:
            cJSON_AddNumberToObject( cj_payload, "volume", speaker->volume);
            cJSON_AddBoolToObject( cj_payload, "muted", speaker->muted);
            break;
        default:
            break;
    }
    return;
}

const char* alexa_speaker_event_construct( alexa_service* as, enum SPEAKER_EVENT_ENUM event )
{
    char* event_json;
    cJSON* cj_root = cJSON_CreateObject();
    cJSON* cj_event = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();

    cJSON_AddItemToObject( cj_root, "event", cj_event );
    
    cJSON_AddItemToObject( cj_event, "header", cj_header );
    cJSON_AddItemToObject( cj_event, "payload", cj_payload );

    speaker_event_header_construct( as->speaker, cj_header, event );
    speaker_event_payload_construct( as->speaker, cj_payload, event );
    
    event_json = cJSON_Print(cj_root);
    cJSON_Delete(cj_root);
    
    sys_log_d( "%s\n", event_json );
    return event_json;
}

static int directive_set_volume( struct alexa_service* as, struct alexa_directive_item* item )
{
    struct alexa_speaker* speaker = as->speaker;
    cJSON* cj_payload = item->payload;
    cJSON* cj_volume;
    
    cj_volume = cJSON_GetObjectItem(cj_payload, "volume");
    if( !cj_volume )
    {
        goto err;
    }
    
    //[0, 100]
    speaker->volume = cj_volume->valueint;
    
    alexa_speaker_event_construct( as, VOLUMECHANGED_EVENT );
    
    return 0;
err:
    return -1;
}

static int directive_adjust_volume( struct alexa_service* as, struct alexa_directive_item* item )
{
    struct alexa_speaker* speaker = as->speaker;
    cJSON* cj_payload = item->payload;
    cJSON* cj_volume;
    
    cj_volume = cJSON_GetObjectItem(cj_payload, "volume");
    if( !cj_volume )
    {
        goto err;
    }
    
    //[-100, 100]
    speaker->volume = speaker->volume + cj_volume->valueint;
    if( speaker->volume >= MAX_VOLUME_VAL ) speaker->volume = MAX_VOLUME_VAL;
    if( speaker->volume <= MIN_VOLUME_VAL ) speaker->volume = MIN_VOLUME_VAL;

    alexa_speaker_event_construct( as, VOLUMECHANGED_EVENT );
    
    return 0;
err:
    return -1;
}

static int directive_set_mute( alexa_service* as, struct alexa_directive_item* item )
{
    cJSON* cj_payload = item->payload;
    cJSON* cj_mute;
    
    cj_mute = cJSON_GetObjectItem(cj_payload, "mute");
    if( !cj_mute )
    {
        goto err;
    }
    
    if( cj_mute->type == cJSON_False )
    {
        //
    }
    else
    {
        //
    }
    
    alexa_speaker_event_construct( as, MUTECHANGED_EVENT );
    
    return 0;
err:
    return -1;
}

static int directive_process( alexa_service* as, struct alexa_directive_item* item )
{
    cJSON* header = item->header;
    cJSON* name;
    
    name = cJSON_GetObjectItem(header, "name");
    if( !name )
    {
        goto err;
    }
    
    if( !strcmp( name->valuestring, "SetVolume" ) )
    {
        directive_set_volume( as, item );
    }
    else if( !strcmp( name->valuestring, "AdjustVolume" ) )
    {
        directive_adjust_volume( as, item );
    }
    else if( !strcmp( name->valuestring, "SetMute" ) )
    {
        directive_set_mute( as, item );
    }
    
    return 0;
    
err:
    return -1;
}

static struct alexa_speaker* speaker_construct(void)
{
    struct alexa_speaker* speaker = alexa_new( struct alexa_speaker );
    if( speaker )
    {
        
    }
    return speaker;
}

static void speaker_destruct(struct alexa_speaker* speaker)
{
    alexa_delete( speaker );
}

int alexa_speaker_init(alexa_service* as)
{
    as->speaker = speaker_construct();
    alexa_directive_register(NAMESPACE, directive_process );
    
    return 0;
} 

int alexa_speaker_done(alexa_service* as)
{
    alexa_directive_unregister(NAMESPACE);    

    speaker_destruct( as->speaker );
    return 0;
}


/*******************************************************************************
    END OF FILE
*******************************************************************************/
