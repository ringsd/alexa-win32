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

#define NAMESPACE               "SpeechSynthesizer"

#define SPEECHSYNTHESIZER_STATE_PLAYING         "PLAYING"
#define SPEECHSYNTHESIZER_STATE_FINISHED        "FINISHED"

struct alexa_speechsynthesizer{
    char*                     messageId;
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

cJSON* speechsynthesizer_speech_state( alexa_service* as )
{
    struct alexa_speechsynthesizer* ss = as->ss;
    
    cJSON* cj_speech_state = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();
    
    cJSON_AddItemToObject( cj_speech_state, "header", cj_header );
    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", speechsynthesizer_event[SPEECHSTATE_EVENT]);
    
    cJSON_AddItemToObject( cj_speech_state, "payload", cj_payload );
    cJSON_AddNumberToObject( cj_payload, "offsetInMilliseconds", ss->offsetInMilliseconds);
    cJSON_AddStringToObject( cj_payload, "playerActivity", ss->playerActivity );
    
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
static const char* ss_event_construct( alexa_service* as, enum SPEECHSYNTHESIZER_EVENT_ENUM event )
{
    struct alexa_speechsynthesizer* ss = as->ss;
    const char* event_string;
    cJSON* cj_root = cJSON_CreateObject();
    cJSON* cj_event = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();

    cJSON_AddItemToObject( cj_root, "event", cj_event );
    cJSON_AddItemToObject( cj_event, "header", cj_header );
    
    ss_event_header_construct( cj_header, event, (const char*)ss->messageId );

    cJSON_AddItemToObject( cj_event, "payload", cj_payload );
    cJSON_AddStringToObject( cj_payload, "token", ss->token);
    
    event_string = cJSON_Print( cj_root );
    
    cJSON_Delete( cj_root );

    sys_log_d( "%s\n", event_string );
    return event_string;
}

void alexa_speechsynthesizer_set_event( alexa_service* as, const char* playerActivity )
{
    struct alexa_speechsynthesizer* ss = as->ss;
    ss->playerActivity = (char*)playerActivity;
}

const char* alexa_speechsynthesizer_event_construct( alexa_service* as )
{
    return ss_event_construct(as, SPEECHSTARTED_EVENT);
}

static int directive_speak( alexa_service* as, struct alexa_directive_item* item )
{
    cJSON* cj_payload = item->payload;
    cJSON* cj_url;
    
    cj_url = cJSON_GetObjectItem(cj_payload, "url");
    if( !cj_url )
    {
        goto err;
    }

    // sync
    alexa_speechsynthesizer_set_event( as, SPEECHSYNTHESIZER_STATE_PLAYING );
    alexa_speechsynthesizer_event_construct( as );
    
    // play the sound
    
    // play the sound end 

    alexa_speechsynthesizer_set_event( as, SPEECHSYNTHESIZER_STATE_FINISHED );
    alexa_speechsynthesizer_event_construct( as );

    return 0;
err:
    return -1;
}


static int directive_process( alexa_service* as, struct alexa_directive_item* item )
{
    cJSON* cj_header = item->header;
    cJSON* cj_name;
    
    if( !cj_header ) goto err;
    
    cj_name = cJSON_GetObjectItem(cj_header, "name");
    if( !cj_name ) goto err;
    
    if( !strcmp( cj_name->valuestring, "Speak" ) )
    {
        directive_speak( as, item );
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
    alexa_delete( ss );
}

int alexa_speechsynthesizer_init(alexa_service* as)
{
    as->ss = ss_construct();
    alexa_directive_register(NAMESPACE, directive_process );
    
    return 0;
}

int alexa_speechsynthesizer_done(alexa_service* as)
{
    ss_destruct( as->ss );
    alexa_directive_unregister(NAMESPACE);    
    return 0;
}

/*******************************************************************************
    END OF FILE
*******************************************************************************/
