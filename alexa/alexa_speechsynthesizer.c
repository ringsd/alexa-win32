/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.

    File: alexa_speechsynthesizer.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/14 17:53:05

    https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/speechsynthesizer
    
*******************************************************************************/

#include "alexa_service.h"

#define TAG                     "SpeechSynthesizer"
#define NAMESPACE               "SpeechSynthesizer"

#define SPEECHSYNTHESIZER_STATE_PLAYING         "PLAYING"
#define SPEECHSYNTHESIZER_STATE_FINISHED        "FINISHED"

struct alexa_speechsynthesizer{
    struct alexa_service*     as;
    char                      messageId[ALEXA_UUID_LENGTH + 1];
    char*                     token;
    
    char*                     playerActivity;
    int                       offsetInMilliseconds;
};

enum SPEECHSYNTHESIZER_EVENT_ENUM{
    SPEECHSTARTED_EVENT = 0,
    SPEECHFINISHED_EVENT,
    
    SPEECHSTATE_EVENT, //for context
};

static const char* speechsynthesizer_event[] = {
    "SpeechStarted",
    "SpeechFinished",
    
    "SpeechState",
};

cJSON* speechsynthesizer_speech_state(struct alexa_speechsynthesizer* ss)
{
    cJSON* cj_speech_state = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();
    
    cJSON_AddItemToObject( cj_speech_state, "header", cj_header );
    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", speechsynthesizer_event[SPEECHSTATE_EVENT]);
    
    cJSON_AddItemToObject( cj_speech_state, "payload", cj_payload );
    if (ss->token) cJSON_AddStringToObject(cj_payload, "token", ss->token);
    else cJSON_AddStringToObject(cj_payload, "token", "");
    cJSON_AddNumberToObject(cj_payload, "offsetInMilliseconds", ss->offsetInMilliseconds);
    cJSON_AddStringToObject(cj_payload, "playerActivity", ss->playerActivity);
    return cj_speech_state;
}

static void ss_event_header_construct( cJSON* cj_header, enum SPEECHSYNTHESIZER_EVENT_ENUM event, const char* messageId )
{
    int event_index = (int)event;

    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", speechsynthesizer_event[event_index]);
    cJSON_AddStringToObject( cj_header, "messageId", messageId);
    
    return;
}

//has binary audio attachment
static const char* ss_event_construct(struct alexa_speechsynthesizer* ss, enum SPEECHSYNTHESIZER_EVENT_ENUM event, const char* token)
{
    const char* event_string;
    cJSON* cj_root = cJSON_CreateObject();
    cJSON* cj_event = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();

    ss = ss;

    cJSON_AddItemToObject( cj_root, "event", cj_event );
    cJSON_AddItemToObject( cj_event, "header", cj_header );
    
    alexa_generate_uuid(ss->messageId, sizeof(ss->messageId));
    ss_event_header_construct(cj_header, event, ss->messageId);

    cJSON_AddItemToObject( cj_event, "payload", cj_payload );
    cJSON_AddStringToObject( cj_payload, "token", token);
    
    event_string = cJSON_Print( cj_root );
    
    cJSON_Delete( cj_root );

    sys_log_d( TAG, "%s\n", event_string );
    return event_string;
}

void alexa_speechsynthesizer_set_event(struct alexa_speechsynthesizer* ss, const char* playerActivity)
{
    ss->playerActivity = (char*)playerActivity;
}

const char* alexa_speechsynthesizer_event_construct(struct alexa_speechsynthesizer* ss, enum SPEECHSYNTHESIZER_EVENT_ENUM event, const char* token)
{
    return ss_event_construct(ss, event, token);
}

static int directive_speak(struct alexa_speechsynthesizer* ss, struct alexa_directive_item* item)
{
    cJSON* cj_payload = item->payload;
    cJSON* cj_url;
    cJSON* cj_token;
    const char* event_string;
    
    cj_url = cJSON_GetObjectItem(cj_payload, "url");
    if( !cj_url )
    {
        goto err;
    }

    cj_token = cJSON_GetObjectItem(cj_payload, "token");
    if (!cj_token)
    {
        goto err;
    }
    ALEXA_SAFE_FREE(ss->token);
    ss->token = alexa_strdup(cj_token->valuestring);

    // sync
    alexa_speechsynthesizer_set_event( ss, SPEECHSYNTHESIZER_STATE_PLAYING );
    event_string = alexa_speechsynthesizer_event_construct(ss, SPEECHSTARTED_EVENT, cj_token->valuestring);
    alexa_event_item_add_event(ss->as->event, event_string);

    // play the sound
    alexa_delay(1000);

    // play the sound end 

    alexa_speechsynthesizer_set_event( ss, SPEECHSYNTHESIZER_STATE_FINISHED );
    event_string = alexa_speechsynthesizer_event_construct(ss, SPEECHFINISHED_EVENT, cj_token->valuestring);
    alexa_event_item_add_event(ss->as->event, event_string);

    return 0;
err:
    return -1;
}


static int directive_process( struct alexa_speechsynthesizer* ss, struct alexa_directive_item* item )
{
    cJSON* cj_header = item->header;
    cJSON* cj_name;
    
    if( !cj_header ) goto err;
    
    cj_name = cJSON_GetObjectItem(cj_header, "name");
    if( !cj_name ) goto err;
    
    if( !strcmp( cj_name->valuestring, "Speak" ) )
    {
        directive_speak( ss, item );
    }
    
    return 0;
    
err:
    return -1;
}

static struct alexa_speechsynthesizer* ss_construct(void)
{
    struct alexa_speechsynthesizer* ss = alexa_new( struct alexa_speechsynthesizer );
    if( ss )
    {
        ss->playerActivity = SPEECHSYNTHESIZER_STATE_FINISHED;
        ss->offsetInMilliseconds = 0;
    }

    return ss;
}


static void ss_destruct(struct alexa_speechsynthesizer* ss)
{
    ALEXA_SAFE_FREE(ss->token);
    alexa_delete(ss);
}

struct alexa_speechsynthesizer* alexa_speechsynthesizer_init(alexa_service* as)
{
    struct alexa_speechsynthesizer* ss = ss_construct();
    if (ss)
    {
        alexa_directive_register(NAMESPACE, directive_process, (void*)ss);
        ss->as = as;
    }
    return ss;
}

int alexa_speechsynthesizer_done(struct alexa_speechsynthesizer* ss)
{
    alexa_directive_unregister(NAMESPACE);
    if (ss) ss_destruct(ss);
    return 0;
}

/*******************************************************************************
    END OF FILE
*******************************************************************************/
