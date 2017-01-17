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

#include "alexa_service.h"

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


static pthread_cond_t   alexa_speechrecognizer_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t  alexa_speechrecognizer_mutex = PTHREAD_MUTEX_INITIALIZER;

alexa_speechrecognizer_wake_up()
{
    ALEXA_MUTEX_LOCK(&alexa_speechrecognizer_mutex);
    pthread_cond_signal(&alexa_speechrecognizer_cond);
    ALEXA_MUTEX_UNLOCK(&alexa_speechrecognizer_mutex);
}

alexa_speechrecognizer_directive()
{
}

enum SPPED_RECOGINZER_STATE alexa_speechrecognizer_process(struct alexa_service* as)
{
    while(1)
    {
        switch( as->sr.state )
        {
            case IDLE:
                //wait user wake up or wait the directive
                ALEXA_MUTEX_LOCK(&alexa_speechrecognizer_mutex);
                while(wait_directive() || wait_user_wake_up())
                {
                    pthread_cond_wait(&alexa_speechrecognizer_cond, &alexa_speechrecognizer_mutex);
                }
                ALEXA_MUTEX_UNLOCK(&alexa_speechrecognizer_mutex);
                
                if( user wake up )
                {
                    as->sr.state = RECOGNIZING;
                }
                else
                {
                    directive_process( as );
                }
                break;
            case RECOGNIZING:
                //record data
                //send Recognize Event to avs
                //change state to BUSY

                sr_recognize_event( as );

                as->sr.state = BUSY;
                break;
            case BUSY:
                //wait directive
                while( wait_directive() || timeout )
                {
                    
                }

                if( wait directive success )
                {
                    directive_process( as );
                    if( as->sr.state == BUSY )
                    {
                        as->sr.state = IDLE;
                    }
                }
                else
                {
                    //timeout, maybe you need clear some state
                    as->sr.state = IDLE;
                }
                break;
            case EXPECTING_SPEECH:
                if( can change state to RECOGNIZING )
                {
                    as->sr.state = IDLE;
                    //ExpectSpeechTimedOut 
                }
                else
                {
                    as->sr.state = RECOGNIZING;
                }
                break;
            default:
                //unknown state
                break;
        }
    }
    return new_state;
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
    
    cJSON_AddItemToArray( cj_context, alexa_context_state(as) );
    
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
