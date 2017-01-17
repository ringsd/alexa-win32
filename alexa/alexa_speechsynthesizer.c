/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.

	File: alexa_speechsynthesizer.c

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/01/14 17:53:05

    https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/speechsynthesizer
    
*******************************************************************************/

#define NAMESPACE               "SpeechSynthesizer"

#define RECOGNIZER_FORMAT       "AUDIO_L16_RATE_16000_CHANNELS_1"

enum{
    SPEECHSTARTED_EVENT = 0,
    SPEECHFINISHED_EVENT,
    
    SPEECHSTATE_EVENT, //for context
}SPEECHSYNTHESIZER_EVENT_ENUM;

static const char* speechsynthesizer_event[] = {
    "SpeechStarted",
    "SpeechFinished",
    
    "SpeechState",
};

cJSON* speechsynthesizer_speech_state( alexa_service* as )
{
    cJSON* cj_speech_state = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();
    
    cJSON_AddItemToObject( cj_speech_state, "header", cj_header );
    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", speechsynthesizer_event[SPEECHSTATE_EVENT]);
    
    cJSON_AddItemToObject( cj_speech_state, "payload", cj_payload );
    cJSON_AddNumberToObject( cj_payload, "offsetInMilliseconds", offsetInMilliseconds);
    cJSON_AddBoolToObject( cj_payload, "playerActivity", playerActivity);
    
    return cj_speech_state;
}

static void ss_event_header_construct( cJSON* cj_header, SPEECHSYNTHESIZER_EVENT_ENUM event, const char* msg_id )
{
    int event_index = (int)event;
    int len = 0;

    assert( msg_id != NULL );

    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", speechsynthesizer_event[event_index]);
    cJSON_AddStringToObject( cj_header, "messageId", msg_id);
    
    return;
}

//has binary audio attachment
static const char* ss_event_construct( alexa_service* as )
{
    const char* event_string;
    cJSON* cj_root = cJSON_CreateObject();
    cJSON* cj_event = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();

    cJSON_AddItemToObject( cj_root, "event", cj_event );
    
    cJSON_AddItemToObject( cj_event, "header", cj_header );
    
    ss_event_header_construct( as, cj_header, msg_id );
    
    cJSON_AddItemToObject( cj_event, "payload", cj_payload );
    cJSON_AddStringToObject( cj_payload, "token", token);
    
    event_string = cJSON_Print( cj_root );
    alexa_log_d( "%s\n", event_string );
    
    cJSON_Delete( cj_root );
    
    return event_string;
}

static int directive_speak( alexa_service* as, cJSON* root )
{
    cJSON* cj_payload = as->payload;
    cJSON* cj_url = as->payload;
    
    cj_url = cJSON_GetObjectItem(cj_payload, "url");
    if( !cj_url )
    {
        goto err;
    }    
    
    // play the sound
    
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
    
    if( !strcmp( name->valuestring, "Speak" ) )
    {
        directive_speak( as, root );
    }
    
    return 0;
    
err:
    return -1;
}

int alexa_speechsynthesizer_init(alexa_service* as)
{
    alexa_directive_register(NAMESPACE, directive_process );
    
    return 0;
}

int alexa_speechsynthesizer_done(alexa_service* as)
{
    alexa_directive_unregister(NAMESPACE);    

    return 0;
}

/*******************************************************************************
	END OF FILE
*******************************************************************************/
