/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.

	File: alexa_playbackcontroller.c

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/01/13 16:27:07
    https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/playbackcontroller

*******************************************************************************/

#define NAMESPACE       "PlaybackController"

enum{
    PLAYCOMMANDISSUED_EVENT = 0,
    PAUSECOMMANDISSUED_EVENT,
    NEXTCOMMANDISSUED_EVENT,
    PREVIOUSCOMMANDISSUED_EVENT,
}PLAYBACKCONTROLLER_EVENT_ENUM;

static const char* playbackcontroller_event[] = {
    "PlayCommandIssued",
    "PauseCommandIssued",
    "NextCommandIssued",
    "PreviousCommandIssued",
};


static void playback_ctrl_event_context_construct( alexa_service* as, cJSON* cj_context)
{
    //AudioPlayer.PlaybackState
    cJSON_AddItemToArray( cj_context, audioplayer_playback_state(as) );
    //Alerts.AlertsState
    //Speaker.VolumeState
    //SpeechSynthesizer.SpeechState    
}

static void playback_ctrl_event_header_construct( alexa_service* as, cJSON* cj_header, SYSTEM_EVENT_ENUM event, const char* msg_id )
{
    int event_index = (int)event;
    int len = 0;
    
    assert( msg_id != NULL );

    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", playbackcontroller_event[event_index]);
    cJSON_AddStringToObject( cj_header, "messageId", msg_id);

    return;
}

static void playback_ctrl_event_payload_construct( alexa_service* as, cJSON* cj_payload, SYSTEM_EVENT_ENUM event )
{
    int event_index = (int)event;

    return;
}

const char* alexa_playback_ctrl_event_construct( alexa_service* as )
{
    char* event_json;
    cJSON* cj_root = cJSON_CreateObject();
    cJSON* cj_context = cJSON_CreateArray();
    cJSON* cj_event = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();
    
    cJSON_AddItemToObject( cj_root, "context", cj_context );
    playback_ctrl_event_context_construct( cj_context );
    
    cJSON_AddItemToObject( cj_root, "event", cj_event );
    
    cJSON_AddItemToObject( cj_event, "header", cj_header );
    cJSON_AddItemToObject( cj_event, "payload", cj_payload );

    //
    playback_ctrl_event_header_construct( cj_header, event, msg_id );
    playback_ctrl_event_payload_construct( cj_payload, event, msg_id );    
    
    return ;
}


/*******************************************************************************
	END OF FILE
*******************************************************************************/
