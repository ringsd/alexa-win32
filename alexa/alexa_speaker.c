/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.

    File: alexa_speaker.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/13 16:14:32

*******************************************************************************/

#include "alexa_service.h"

#define TAG             "Speaker"
#define NAMESPACE       "Speaker"

#define MAX_VOLUME_VAL    100
#define MIN_VOLUME_VAL    0

struct alexa_speaker{
    struct alexa_service* as;
    char  messageId[ALEXA_UUID_LENGTH + 1];
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

cJSON* speaker_volume_state(struct alexa_speaker* speaker)
{
    cJSON* cj_volume_state = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();
    
    cJSON_AddItemToObject( cj_volume_state, "header", cj_header );
    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", speaker_event[VOLUMESTATE_EVENT]);
    
    cJSON_AddItemToObject( cj_volume_state, "payload", cj_payload );
    cJSON_AddNumberToObject( cj_payload, "volume", speaker->volume);
    cJSON_AddBoolToObject( cj_payload, "muted", speaker->muted);
    
    return cj_volume_state;
}

static void speaker_send_event(struct alexa_speaker* speaker, const char* event_string)
{
    alexa_event_item_add_event(alexa_service_get_event(speaker->as), event_string);
}

static void speaker_event_header_construct( struct alexa_speaker* speaker, cJSON* cj_header, enum SPEAKER_EVENT_ENUM event )
{
    int event_index = (int)event;

    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", speaker_event[event_index]);
    alexa_generate_uuid(speaker->messageId, sizeof(speaker->messageId));
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

const char* alexa_speaker_event_construct(struct alexa_speaker* speaker, enum SPEAKER_EVENT_ENUM event)
{
    char* event_json;
    cJSON* cj_root = cJSON_CreateObject();
    cJSON* cj_event = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();

    cJSON_AddItemToObject( cj_root, "event", cj_event );
    
    cJSON_AddItemToObject( cj_event, "header", cj_header );
    cJSON_AddItemToObject( cj_event, "payload", cj_payload );

    speaker_event_header_construct(speaker, cj_header, event);
    speaker_event_payload_construct(speaker, cj_payload, event);
    
    event_json = cJSON_Print(cj_root);
    cJSON_Delete(cj_root);
    
    sys_log_d(TAG, "%s\n", event_json );
    return event_json;
}

static int directive_set_volume(struct alexa_speaker* speaker, struct alexa_directive_item* item)
{
    const char* event_string = NULL;
    cJSON* cj_payload = item->payload;
    cJSON* cj_volume;
    
    cj_volume = cJSON_GetObjectItem(cj_payload, "volume");
    if( !cj_volume )
    {
        goto err;
    }
    
    //[0, 100]
    speaker->volume = cj_volume->valueint;
    
    event_string = alexa_speaker_event_construct(speaker, VOLUMECHANGED_EVENT);
    speaker_send_event(speaker, event_string);
    
    return 0;
err:
    return -1;
}

static int directive_adjust_volume(struct alexa_speaker* speaker, struct alexa_directive_item* item)
{
    cJSON* cj_payload = item->payload;
    cJSON* cj_volume;
    const char* event_string = NULL;
    
    cj_volume = cJSON_GetObjectItem(cj_payload, "volume");
    if( !cj_volume )
    {
        goto err;
    }
    
    //[-100, 100]
    speaker->volume = speaker->volume + cj_volume->valueint;
    if( speaker->volume >= MAX_VOLUME_VAL ) speaker->volume = MAX_VOLUME_VAL;
    if( speaker->volume <= MIN_VOLUME_VAL ) speaker->volume = MIN_VOLUME_VAL;

    event_string = alexa_speaker_event_construct(speaker, VOLUMECHANGED_EVENT);
    speaker_send_event(speaker, event_string);

    return 0;
err:
    return -1;
}

static int directive_set_mute(struct alexa_speaker* speaker, struct alexa_directive_item* item)
{
    cJSON* cj_payload = item->payload;
    cJSON* cj_mute;
    const char* event_string;
    
    cj_mute = cJSON_GetObjectItem(cj_payload, "mute");
    if( !cj_mute )
    {
        goto err;
    }
    
    if( cj_mute->type == cJSON_False )
    {
        speaker->muted = 0;
    }
    else
    {
        speaker->muted = 1;
    }
    
    event_string = alexa_speaker_event_construct(speaker, MUTECHANGED_EVENT);
    speaker_send_event(speaker, event_string);

    return 0;
err:
    return -1;
}

static int directive_process( struct alexa_speaker* speaker, struct alexa_directive_item* item )
{
    cJSON* cj_header = item->header;
    cJSON* cj_dialogRequestId;
    cJSON* cj_name;
    
    cj_name = cJSON_GetObjectItem(cj_header, "name");
    if (!cj_name)
    {
        goto err;
    }
    
    cj_dialogRequestId = cJSON_GetObjectItem(cj_header, "dialogRequestId");
    if (cj_dialogRequestId)
    {
        sys_log_i(TAG, "directive from the speech recognizer.\n");
        cj_dialogRequestId->valuestring;
    }

    if (!strcmp(cj_name->valuestring, "SetVolume"))
    {
        directive_set_volume(speaker, item);
    }
    else if (!strcmp(cj_name->valuestring, "AdjustVolume"))
    {
        directive_adjust_volume(speaker, item);
    }
    else if (!strcmp(cj_name->valuestring, "SetMute"))
    {
        directive_set_mute(speaker, item);
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

struct alexa_speaker* alexa_speaker_init(alexa_service* as)
{
    struct alexa_speaker* speaker = speaker_construct();
    if (speaker)
    {
        alexa_directive_register(NAMESPACE, directive_process, (void*)speaker);
        speaker->as = as;
    }
    return speaker;
}

int alexa_speaker_done(struct alexa_speaker* speaker)
{
    alexa_directive_unregister(NAMESPACE);    
    if (speaker) speaker_destruct(speaker);
    return 0;
}


/*******************************************************************************
    END OF FILE
*******************************************************************************/
