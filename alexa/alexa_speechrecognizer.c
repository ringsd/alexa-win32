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

enum WAKE_UP_SOURCE_ENUM{
    WAKE_UP_BY_NONE = 0,
    WAKE_UP_BY_USER,
    WAKE_UP_BY_DIRECTIVE,
    WAKE_UP_BY_EXIT,
};

enum SPEECH_RECOGINZER_STATE{
    IDLE, //the host is idle, recognizer finish or timeout
    RECOGNIZING, //the host start record data, and send data to avs
    BUSY, //the avs recognizing, the host need waiting
    EXPECTING_SPEECH, //expect state
};

struct alexa_speechrecognizer{
    WAKE_UP_SOURCE_ENUM     wakeup_source;
    SPEECH_RECOGINZER_STATE state;
    
    pthread_cond_t          cond;
    pthread_mutex_t         mutex;
};

static pthread_cond_t   alexa_speechrecognizer_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t  alexa_speechrecognizer_mutex = PTHREAD_MUTEX_INITIALIZER;

//wake up the thread by the key or by the directive
void alexa_speechrecognizer_user_wake_up( struct alexa_service* as )
{
    struct alexa_speechrecognizer* sr = &as->sr;
    ALEXA_MUTEX_LOCK(&sr->mutex);
    if( sr->state == IDLE )
    {
        sr->wakeup_source = WAKE_UP_BY_USER;
        pthread_cond_signal(&sr->cond);
    }
    ALEXA_MUTEX_UNLOCK(&sr->mutex);
}

void alexa_speechrecognizer_directive_wake_up( struct alexa_service* as )
{
    struct alexa_speechrecognizer* sr = &as->sr;
    ALEXA_MUTEX_LOCK(&sr->mutex);
    as->sr.wakeup_source = WAKE_UP_BY_DIRECTIVE;
    pthread_cond_signal(&sr->cond);
    ALEXA_MUTEX_UNLOCK(&sr->mutex);
}

void alexa_speechrecognizer_exit_wake_up( struct alexa_service* as )
{
    struct alexa_speechrecognizer* sr = &as->sr;
    ALEXA_MUTEX_LOCK(&sr->mutex);
    as->sr.wakeup_source = WAKE_UP_BY_EXIT;
    pthread_cond_signal(&sr->cond);
    ALEXA_MUTEX_UNLOCK(&sr->mutex);
}

void alexa_speechrecognizer_process(struct alexa_service* as)
{
    struct alexa_speechrecognizer* sr = &(as->sr);
    WAKE_UP_SOURCE_ENUM wakeup_source = sr->wakeup_source;
    
    while(1)
    {
        //exit from this function
        if( sr->wakeup_source == WAKE_UP_BY_EXIT )
        {
            break;
        }
        
        switch( sr->state )
        {
            case IDLE:
            {
                //wait user wake up or wait the directive
                ALEXA_MUTEX_LOCK(&sr->mutex);
                while(1)
                {
                    wakeup_source = sr->wakeup_source;
                    //clear the wake up state
                    sr->wakeup_source = WAKE_UP_BY_NONE;
                    
                    if( wakeup_source == WAKE_UP_BY_EXIT )
                    {
                        break;
                    }

                    if( wakeup_source == WAKE_UP_BY_USER )
                    {
                        //user wake up
                        sr->state = RECOGNIZING;
                        break;
                    }

                    directive = alexa_directive_get( as );
                    if( directive )
                    {
                        //get the directive
                        break;
                    }

                    pthread_cond_wait(&sr->cond, &sr->mutex);
                }
                ALEXA_MUTEX_UNLOCK(&sr->mutex);
                
                if( wakeup_source == WAKE_UP_BY_USER )
                {
                    //has been change state need lock, just break
                }
                else if( directive )
                {
                    alexa_directive_process( as, directive );
                    alexa_directive_free( as, directive );
                    directive = NULL;
                }
                sr->wakeup_source = WAKE_UP_BY_NONE;
                break;
            }
            case RECOGNIZING:
            {
                //record data
                //send Recognize Event to avs
                //change state to BUSY

                //how to implement the NEAR_FIELD FAR_FIELD profile

                sr_recognize_event( as );

                sr->state = BUSY;
                break;
            }
            case BUSY:
            {
                //in busy state, we ignore the user wake up 
                //just wait directive
                ALEXA_MUTEX_LOCK(&sr->mutex);
                while(1)
                {
                    const struct timespec * abstime;
                    directive = alexa_directive_get( as );
                    if( directive )
                    {
                        //get the directive
                        break;
                    }

                    if( ETIMEDOUT == pthread_cond_timedwait(&sr->cond, &sr->mutex, abstime) )
                    {
                        //timeout
                         break;
                    }
                }
                ALEXA_MUTEX_UNLOCK(&sr->mutex);                
                
                if( directive )
                {
                    alexa_directive_process( as, directive );
                    alexa_directive_free( as, directive );
                    directive = NULL;
                    if( msg id is equal )
                    {
                        //msg id is equal
                        sr->state = IDLE;
                    }
                }
                else if( timeout )
                {
                    //timeout, maybe you need clear some state
                    sr->state = IDLE;
                }
                break;
            }
            case EXPECTING_SPEECH:
            {
                if( can change state to RECOGNIZING )
                {
                    sr->state = IDLE;
                    //ExpectSpeechTimedOut 
                }
                else
                {
                    sr->state = RECOGNIZING;
                }
                break;
            }
            default:
            {
                //unknown state
                break;
            }
        }
    }
    return;
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

static struct alexa_speechrecognizer* alexa_speechrecognizer_construct(void)
{
    struct alexa_speechrecognizer* sr = alexa_new( struct alexa_speechrecognizer )
    if( sr )
    {
        sr->wakeup_source = WAKE_UP_BY_NONE;
        
        pthread_cond_init( &sr->cond, NULL );
        pthread_mutex_init( &sr->mutex, NULL )
    }
}

static void alexa_speechrecognizer_destruct(struct alexa_speechrecognizer* sr)
{
    if( sr )
    {
        pthread_cond_destroy( &sr->cond );
        pthread_mutex_destroy( &sr->mutex );
        
        alexa_delete( sr );
    }
}

int alexa_speechrecognizer_init(alexa_service* as)
{
    as->sr = alexa_speechrecognizer_construct();
    if( !as->sr )
    {
        sys_log_e( TAG, "construct speechrecognizer fail.\n" );
    }
    alexa_directive_register(NAMESPACE, directive_process );
    return 0;
}

int alexa_audioplayer_done(alexa_service* as)
{
    alexa_directive_unregister(NAMESPACE);    
    alexa_speechrecognizer_destruct( as->sr );
    return 0;
}

/*******************************************************************************
	END OF FILE
*******************************************************************************/
