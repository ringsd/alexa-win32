/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.

	File: alexa_speaker.c

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/01/13 16:14:32

*******************************************************************************/

#define NAMESPACE       "Speaker"

enum{
    VOLUMECHANGED_EVENT,
    MUTECHANGED_EVENT,
    
    VOLUMESTATE_EVENT,
}SPEAKER_EVENT_ENUM;

static const char* speaker_event[] = {
    "VolumeChanged",
    "MuteChanged",
    
    "VolumeState",
};

cJSON* speaker_volume_state( alexa_service* as )
{
    cJSON* cj_volume_state = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();
    
    cJSON_AddItemToObject( cj_volume_state, "header", cj_header );
    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", speaker_event[VOLUMESTATE_EVENT]);
    
    cJSON_AddItemToObject( cj_volume_state, "payload", cj_payload );
    cJSON_AddNumberToObject( cj_payload, "volume", volume);
    cJSON_AddBoolToObject( cj_payload, "mute", mute);
    
    return cj_volume_state;
}


static void speaker_event_header_construct( cJSON* cj_header, AUDIOPLAYER_EVENT_ENUM event, const char* msg_id )
{
    int event_index = (int)event;
    int len = 0;
    
    assert( msg_id != NULL );

    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", speaker_event[event_index]);
    cJSON_AddStringToObject( cj_header, "messageId", msg_id);
    
    return;
}

static void speaker_event_payload_construct(cJSON* cj_payload, AUDIOPLAYER_EVENT_ENUM event)
{
    int event_index = (int)event;
    int len = 0;
    
    switch( event )
    {
        case VOLUMECHANGED_EVENT:
        case MUTECHANGED_EVENT:
            cJSON_AddNumberToObject( cj_payload, "volume", volume);
            cJSON_AddBoolToObject( cj_payload, "muted", muted);
            break;
        default:
            break;
    }
    return;
}

const char* alexa_speaker_event_construct( alexa_service* as )
{
    char* event_json;
    cJSON* cj_root = cJSON_CreateObject();
    cJSON* cj_event = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();

    cJSON_AddItemToObject( cj_root, "event", cj_event );
    
    cJSON_AddItemToObject( cj_event, "header", cj_header );
    cJSON_AddItemToObject( cj_event, "payload", cj_payload );

    //
    speaker_event_header_construct( cj_header, event, msg_id );
    speaker_event_payload_construct( cj_payload, event, msg_id );
    
    event_json = cJSON_Print( root );
    alexa_log_d( "%s\n", event_json );
    
    cJSON_Delete( root );
    
    return event_json;
}

static int directive_set_volume( alexa_service* as, cJSON* root )
{
    cJSON* payload = as->payload;
    cJSON* volume;
    
    volume = cJSON_GetObjectItem(payload, "volume");
    if( !volume )
    {
        goto err;
    }
    
    //[0, 100]
    volume->valueint;
    
    return 0;
err:
    return -1;
}

static int directive_adjust_volume( alexa_service* as, cJSON* root )
{
    cJSON* payload = as->payload;
    cJSON* volume;
    
    volume = cJSON_GetObjectItem(payload, "volume");
    if( !volume )
    {
        goto err;
    }
    
    //[-100, 100]
    volume->valueint;
    
    return 0;
err:
    return -1;
}

static int directive_set_mute( alexa_service* as, cJSON* root )
{
    cJSON* payload = as->payload;
    cJSON* mute;
    
    mute = cJSON_GetObjectItem(payload, "mute");
    if( !mute )
    {
        goto err;
    }
    
    if( mute->type == cJSON_False )
    {
        //
    }
    else
    {
        //
    }
    
    return 0;
err:
    return -1;
}

static int directive_process( alexa_service* as, cJSON* root )
{
    cJSON* header = as->header;
    cJSON* name;
    
    name = cJSON_GetObjectItem(header, "name");
    if( !name )
    {
        goto err;
    }
    
    if( !strcmp( name->valuestring, "SetVolume" ) )
    {
        directive_set_volume( as, root );
    }
    else if( !strcmp( name->valuestring, "AdjustVolume" ) )
    {
        directive_adjust_volume( as, root );
    }
    else if( !strcmp( name->valuestring, "SetMute" ) )
    {
        directive_set_mute( as, root );
    }
    
    return 0;
    
err:
    return -1;
}

int alexa_speaker_init(alexa_service* as)
{
    alexa_directive_register(NAMESPACE, directive_process );
    
    return 0;
}

int alexa_speaker_done(alexa_service* as)
{
    alexa_directive_unregister(NAMESPACE);    

    return 0;
}


/*******************************************************************************
	END OF FILE
*******************************************************************************/
