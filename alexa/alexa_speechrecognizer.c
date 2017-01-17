/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.

	File: alexa_speechrecognizer.c

	Description:

	TIME LIST:
    CREATE By Ringsd   2017/01/13 11:20:02
    
    From
    https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/speechrecognizer
    

*******************************************************************************/

#define NAMESPACE               "SpeechRecognizer"

#define RECOGNIZER_FORMAT       "AUDIO_L16_RATE_16000_CHANNELS_1"

//profile
//"CLOSE_TALK" "NEAR_FILELD" "FAR_FIELD"
//format
//"AUDIO_L16_RATE_16000_CHANNELS_1"

enum SPPED_RECOGINZER_STATE{
    IDLE, //the host is idle, recognizer finish or timeout
    RECOGNIZING, //the host start record data, and send data to avs
    BUSY, //the avs recognizing, the host need waiting
    EXPECTING_SPEECH, //expect state
};


{
    SPPED_RECOGINZER_STATE state; //spped_recoginzer_state
}

alexa_speechrecognizer(SPPED_RECOGINZER_STATE state)
{
    case IDLE:
        //wait user wake up to RECOGNIZING
        //Direcctive ExpectSpeecd to EXPECTING_SPEECH
        break;
    case RECOGNIZING:
        //record data
        //send data to avs to BUSY
        //Direcctive ExpectSpeecd to EXPECTING_SPEECH
        break;
    case BUSY:
        //wait avs message
        //recognize event completed to IDLE
        //Direcctive ExpectSpeecd to EXPECTING_SPEECH
        break;
    case EXPECTING_SPEECH:
        //user start interaction to RECOGNIZING
        //ExpectSpeecd timeout to IDLE
        break;
    default:
        //unknown state
        break;
}

//has binary audio attachment
static const char* sr_recognize_event( alexa_service* as )
{
    const char* event_string;
    cJSON* cj_root = cJSON_CreateObject();
    cJSON* cj_context = cJSON_CreateArray();
    cJSON* cj_event = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();

    cJSON_AddItemToObject( cj_root, "context", cj_context );
    cJSON_AddItemToObject( cj_root, "event", cj_event );
    
    //AudioPlayer.PlaybackState
    cJSON_AddItemToArray( cj_context, audioplayer_playback_state(as) );
    //Alerts.AlertsState
    //Speaker.VolumeState
    //SpeechSynthesizer.SpeechState
    
    cJSON_AddItemToObject( cj_event, "header", cj_header );
    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", "Recognize");
    cJSON_AddStringToObject( cj_header, "messageId", messageId);
    cJSON_AddStringToObject( cj_header, "dialogRequestId", dialogRequestId);
    
    cJSON_AddItemToObject( cj_event, "payload", cj_payload );
    cJSON_AddStringToObject( cj_payload, "profile", "CLOSE_TALK");
    cJSON_AddStringToObject( cj_payload, "format", RECOGNIZER_FORMAT);
    
    event_string = cJSON_Print( cj_root );
    alexa_log_d( "%s\n", event_string );
    
    cJSON_Delete( cj_root );
    
    return event_string;
}

static const char* sr_expect_speech_timedout_event(alexa_service* as)
{
    const char* event_string;
    cJSON* cj_root = cJSON_CreateObject();
    cJSON* cj_event = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();

    cJSON_AddItemToObject( cj_root, "event", cj_event );
    
    cJSON_AddItemToObject( cj_event, "header", cj_header );
    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", "ExpectSpeechTimedOut");
    cJSON_AddStringToObject( cj_header, "messageId", messageId);
    
    cJSON_AddItemToObject( cj_event, "payload", cj_payload );
    
    event_string = cJSON_Print( cj_root );
    alexa_log_d( "%s\n", event_string );
    
    cJSON_Delete( cj_root );
    
    return event_string;
}

static int directive_stop_capture( alexa_service* as, cJSON* root )
{
    //stop capture 
    
    return 0;
err:
    return -1;
}


static int directive_expect_speech( alexa_service* as, cJSON* root )
{
    //open the micorphone 
    
    //if microphone open time out send ExpectSpeechTimedOut Event
    
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
    
    if( !strcmp( name->valuestring, "StopCapture" ) )
    {
        directive_stop_capture( as, root );
    }
    else if( !strcmp( name->valuestring, "ExpectSpeech" ) )
    {
        directive_expect_speech( as, root );
    }
    
    return 0;
    
err:
    return -1;
}

int alexa_speechrecognizer_init(alexa_service* as)
{
    alexa_directive_register(NAMESPACE, directive_process );
    
    return 0;
}

int alexa_audioplayer_done(alexa_service* as)
{
    alexa_directive_unregister(NAMESPACE);    

    return 0;
}

/*******************************************************************************
	END OF FILE
*******************************************************************************/
