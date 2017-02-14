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
#include "alexa_auth.h"
#include "alexa_http2.h"
#include <stdio.h>

#define TODO                    1

#define TAG                     "speechrecognizer"

#define NAMESPACE               "SpeechRecognizer"

#define PROFILE_CLOSE_TALK      "CLOSE_TALK"
#define PROFILE_NEAR_FILELD     "NEAR_FILELD"
#define PROFILE_FAR_FIELD       "FAR_FIELD"

#define RECOGNIZER_FORMAT       "AUDIO_L16_RATE_16000_CHANNELS_1"

#define ALEXA_RECORD_TEST_FILE  "16k.raw"

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
    IDLE = 0, //the host is idle, recognizer finish or timeout
    RECOGNIZING, //the host start record data, and send data to avs
    BUSY, //the avs recognizing, the host need waiting
    EXPECTING_SPEECH, //expect state
};

static const char* speech_recoginzer_state[] = 
{
    "IDLE",
    "RECOGNIZING",
    "BUSY",
    "EXPECTING_SPEECH",
};

struct alexa_speechrecognizer{
    enum WAKE_UP_SOURCE_ENUM     wakeup_source;
    enum SPEECH_RECOGINZER_STATE state;

    struct alexa_cond*             cond;
    struct alexa_mutex*          mutex;
    
    char*                        profile;
    char*                        format;
    // bind the device ?? don't have enough info
    char                        messageId[ALEXA_UUID_LENGTH + 1];
    //every event need generate the new request id
    char                        dialogRequestId[ALEXA_UUID_LENGTH + 1];
};

//wake up the thread by the key or by the directive
void alexa_speechrecognizer_user_wake_up( struct alexa_service* as )
{
    struct alexa_speechrecognizer* sr = as->sr;
    alexa_mutex_lock(sr->mutex);
    if( sr->state == IDLE )
    {
        sr->wakeup_source = WAKE_UP_BY_USER;
        alexa_cond_signal(sr->cond);
    }
    alexa_mutex_unlock(sr->mutex);
}

void alexa_speechrecognizer_directive_wake_up( struct alexa_service* as )
{
    struct alexa_speechrecognizer* sr = as->sr;
    alexa_mutex_lock(sr->mutex);
    sr->wakeup_source = WAKE_UP_BY_DIRECTIVE;
    alexa_cond_signal(sr->cond);
    alexa_mutex_unlock(sr->mutex);
}

void alexa_speechrecognizer_exit_wake_up( struct alexa_service* as )
{
    struct alexa_speechrecognizer* sr = as->sr;
    alexa_mutex_lock(sr->mutex);
    sr->wakeup_source = WAKE_UP_BY_EXIT;
    alexa_cond_signal(sr->cond);
    alexa_mutex_unlock(sr->mutex);
}

const char* sr_recognizer_event(struct alexa_service* as);

void sr_generate_request_id( struct alexa_speechrecognizer* sr )
{
    //generate a uuid
    alexa_generate_uuid( sr->dialogRequestId, sizeof( sr->dialogRequestId ) - 1 );
}

static void sr_set_state( struct alexa_speechrecognizer* sr, enum SPEECH_RECOGINZER_STATE state )
{
    if( state != sr->state )
    {
        sys_log_d( TAG, "speech recognizer state %s -> %s\n", speech_recoginzer_state[(int)sr->state], speech_recoginzer_state[(int)state] );
        sr->state = state;
    }
}

void alexa_speechrecognizer_process(struct alexa_service* as)
{
    struct alexa_speechrecognizer* sr = as->sr;
    enum WAKE_UP_SOURCE_ENUM wakeup_source = sr->wakeup_source;

    for (;;)
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
                struct alexa_directive_item* directive_item = NULL;
                //wait user wake up or wait the directive
                alexa_mutex_lock(sr->mutex);
                for (;;)
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
                        sr_set_state( sr, RECOGNIZING );
                        break;
                    }

                    directive_item = alexa_directive_get(as->directive);
                    if (directive_item)
                    {
                        //get the directive
                        break;
                    }

                    alexa_cond_wait(sr->cond, sr->mutex);
                }
                alexa_mutex_unlock(sr->mutex);
                
                if( wakeup_source == WAKE_UP_BY_USER )
                {
                    //has been change state need lock, just break
                }
                else if (directive_item)
                {
                    alexa_directive_process( as->directive, directive_item);
                    alexa_directive_free(directive_item);
                    directive_item = NULL;
                }
                sr->wakeup_source = WAKE_UP_BY_NONE;
                break;
            }
            case RECOGNIZING:
            {
                const char* event;
                char* audio_data = NULL;
                int audio_read_len = 0;
                int audio_data_len = 16 / 8 * 16000 * 5;
                //record data
                //send Recognize Event to avs
                //change state to BUSY

#if 0
                audio_data = alexa_malloc(audio_data_len);
                if (audio_data)
                {
                    struct alexa_record* record = alexa_record_open(1, 16000, 16);
                    char* audio_data_tmp = audio_data;
                    while (audio_data_len)
                    {
                        int read_len = 0;
                        read_len = alexa_record_read(record, audio_data_tmp, audio_data_len - audio_read_len);
                        if (read_len >= 0)
                        {
                            audio_data_tmp += read_len;
                            audio_read_len += read_len;
                        }
                        else
                        {
                            break;
                        }
                    }

                    //alexa_record_save_file(record, "record.wav", audio_data, audio_read_len);
                    alexa_record_close(record);
                }
                //how to implement the NEAR_FIELD FAR_FIELD profile
#else
                FILE* fp = fopen(ALEXA_RECORD_TEST_FILE, "rb");
                if (fp)
                {
                    fseek( fp, 0, SEEK_END );
                    audio_data_len = ftell(fp);
                    audio_data = alexa_malloc(audio_data_len);
                    fseek(fp, 0, SEEK_SET);
                    audio_read_len = fread(audio_data, 1, audio_data_len, fp);
                    fclose(fp);
                }
#endif

                sr_generate_request_id( sr );
                event = sr_recognizer_event(as);
                if (event)
                {
                    //event + binary audio stream
                    alexa_event_item_add_event_data(as->event, event, strlen(event), audio_data, audio_read_len);
                    sr_set_state(sr, BUSY);
                }
                break;
            }
            case BUSY:
            {
                struct alexa_directive_item* directive_item = NULL;
                int timeout = 0; //timeout flag
                
                //in busy state, we ignore the user wake up 
                //just wait directive
                alexa_mutex_lock(sr->mutex);
                for (;;)
                {
                    //const struct timespec * abstime;
                    
                    wakeup_source = sr->wakeup_source;
                    //clear the wake up state
                    sr->wakeup_source = WAKE_UP_BY_NONE;
                    
                    if( wakeup_source == WAKE_UP_BY_EXIT )
                    {
                        break;
                    }

                    directive_item = alexa_directive_get(as->directive);
                    if (directive_item)
                    {
                        //get the directive
                        break;
                    }

                    if( TODO )// ETIMEDOUT == alexa_cond_timedwait(sr->cond, sr->mutex, abstime) )
                    {
                        timeout = 1;
                        break;
                    }
                }
                alexa_mutex_unlock(sr->mutex);                
                
                if (directive_item)
                {
                    alexa_directive_process(as->directive, directive_item);
                    alexa_directive_free(directive_item);
                    directive_item = NULL;
                    if( 0 )//msg id is equal )
                    {
                        //msg id is equal
                        sr_set_state( sr, IDLE );
                    }
                    else
                    {
                        //state not change from BUSY
                    }
                }
                else if( timeout )
                {
                    //timeout, maybe you need clear some state
                    sr_set_state( sr, IDLE );
                }
                break;
            }
            case EXPECTING_SPEECH:
            {
                if(0)// can change state to RECOGNIZING )
                {
                    sr_set_state( sr, IDLE );
                    //ExpectSpeechTimedOut 
                }
                else
                {
                    sr_set_state( sr, RECOGNIZING );
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
const char* sr_recognizer_event( struct alexa_service* as )
{
    struct alexa_speechrecognizer* sr = as->sr;
    const char* event_string;
    cJSON* cj_root = cJSON_CreateObject();
    cJSON* cj_context = NULL;
    cJSON* cj_event = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();

    cj_context = alexa_context_get_state(as);
    cJSON_AddItemToObject(cj_root, "context", cj_context);
    cJSON_AddItemToObject(cj_root, "event", cj_event);

    cJSON_AddItemToObject( cj_event, "header", cj_header );
    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", "Recognize");
    alexa_generate_uuid(sr->messageId, sizeof(sr->messageId));
    cJSON_AddStringToObject( cj_header, "messageId", sr->messageId);
    cJSON_AddStringToObject( cj_header, "dialogRequestId", sr->dialogRequestId);

    cJSON_AddItemToObject( cj_event, "payload", cj_payload );
    cJSON_AddStringToObject( cj_payload, "profile", sr->profile );
    cJSON_AddStringToObject( cj_payload, "format", sr->format );
    
    event_string = cJSON_Print( cj_root );
    sys_log_d( TAG, "%s\n", event_string );
    
    cJSON_Delete( cj_root );
    
    return event_string;
}

static const char* sr_expect_speech_timedout_event(alexa_service* as)
{
    const char* event_string;
    struct alexa_speechrecognizer* sr = as->sr;
    cJSON* cj_root = cJSON_CreateObject();
    cJSON* cj_event = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();

    cJSON_AddItemToObject( cj_root, "event", cj_event );
    
    cJSON_AddItemToObject( cj_event, "header", cj_header );
    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", "ExpectSpeechTimedOut");
    cJSON_AddStringToObject(cj_header, "messageId", sr->messageId);
    
    cJSON_AddItemToObject( cj_event, "payload", cj_payload );
    
    event_string = cJSON_Print( cj_root );
    sys_log_d( TAG, "%s\n", event_string );
    
    cJSON_Delete( cj_root );
    
    return event_string;
}

static int directive_stop_capture(struct alexa_speechrecognizer* sr, cJSON* root)
{
    sr = sr;
    root = root;

    //stop capture 
    
    //close the micro, stop listen 
    
    return 0;
}


static int directive_expect_speech(struct alexa_speechrecognizer* sr, cJSON* root)
{
    sr = sr;
    root = root;
    //open the micorphone 
    
    //if microphone open time out send ExpectSpeechTimedOut Event

    return 0;
}

static int directive_process(struct alexa_speechrecognizer* sr, struct alexa_directive_item* item)
{
    cJSON* root = item->root;
    cJSON* directive;
    cJSON* header;
    cJSON* name;
    
    directive = cJSON_GetObjectItem(root, "directive");
    if( directive == NULL ) goto err;

    header = cJSON_GetObjectItem(directive, "header");
    if( header == NULL ) goto err;

    name = cJSON_GetObjectItem(header, "name");
    if( !name )
    {
        goto err;
    }
    
    if( !strcmp( name->valuestring, "StopCapture" ) )
    {
        directive_stop_capture(sr, root);
    }
    else if( !strcmp( name->valuestring, "ExpectSpeech" ) )
    {
        directive_expect_speech(sr, root);
    }
    
    return 0;
    
err:
    return -1;
}

static struct alexa_speechrecognizer* sr_construct(void)
{
    struct alexa_speechrecognizer* sr = alexa_new(struct alexa_speechrecognizer);
    if( sr )
    {
        sr->wakeup_source = WAKE_UP_BY_NONE;
        
        sr->mutex = alexa_mutex_create();
        sr->cond = alexa_cond_create();
        
        sr->profile = PROFILE_CLOSE_TALK;
        sr->format = RECOGNIZER_FORMAT;
    }
    return sr;
}

static void sr_destruct(struct alexa_speechrecognizer* sr)
{
    if( sr )
    {
        alexa_cond_destroy(sr->cond);
        alexa_mutex_destroy(sr->mutex);
        
        alexa_delete( sr );
    }
}

struct alexa_speechrecognizer* alexa_speechrecognizer_init(void)
{
    struct alexa_speechrecognizer* sr = sr_construct();
    if( sr != NULL )
    {
        alexa_directive_register(NAMESPACE, directive_process, sr);
    }
    return sr;
}

int alexa_speechrecognizer_done(struct alexa_speechrecognizer* sr)
{
    if (sr)
    {
        alexa_directive_unregister(NAMESPACE);
        sr_destruct(sr);
    }
    return 0;
}

/*******************************************************************************
    END OF FILE
*******************************************************************************/
