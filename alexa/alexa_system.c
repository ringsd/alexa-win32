/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.

	File: alexa_system.c

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/01/13 16:22:09
    
    https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/system

*******************************************************************************/

#define     NAMESPACE      "System"

enum{
    SYNCHRONIZESTATE_EVENT = 0,
    USERINACTIVITYREPORT_EVENT,
    EXCEPTIONENCOUNTERED_EVENT,
}SYSTEM_EVENT_ENUM;

static const char* system_event[] = {
    "SynchronizeState",
    "UserInactivityReport",
    "ExceptionEncountered",
};

enum{
    INVALID_REQUEST_EXCEPTION=400,	
    UNAUTHORIZED_REQUEST_EXCEPTION=403,
    THROTTLING_EXCEPTION=429,
    INTERNAL_SERVICE_EXCEPTION=500,
    SYSTEM_EXCEPTION_NA=503,
};

struct system_exception{
    int err_code;
    const char* err_desc;
};

static system_exception system_exception_list[] = {
    {INVALID_REQUEST_EXCEPTION,         "The request was malformed."},
    {UNAUTHORIZED_REQUEST_EXCEPTION,    "The request was not authorized."},
    {THROTTLING_EXCEPTION,              "Too many requests to the Alexa Voice Service."},
    {INTERNAL_SERVICE_EXCEPTION,        "Internal service exception."},
    {SYSTEM_EXCEPTION_NA,               "The Alexa Voice Service is unavailable."},
};

static void system_event_context_construct( alexa_service* as, cJSON* cj_context)
{
    //AudioPlayer.PlaybackState
    cJSON_AddItemToArray( cj_context, audioplayer_playback_state(as) );
    //Alerts.AlertsState
    //Speaker.VolumeState
    //SpeechSynthesizer.SpeechState    
}

static void system_event_header_construct( alexa_service* as, cJSON* cj_header, SYSTEM_EVENT_ENUM event, const char* msg_id )
{
    int event_index = (int)event;
    int len = 0;
    
    assert( msg_id != NULL );

    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", system_event[event_index]);
    cJSON_AddStringToObject( cj_header, "messageId", msg_id);

    return;
}

static void system_event_payload_construct( alexa_service* as, cJSON* cj_payload, SYSTEM_EVENT_ENUM event )
{
    int event_index = (int)event;
    
    switch(event)
    {
        case SYNCHRONIZESTATE_EVENT:
            //don't have payload
            break;
        case USERINACTIVITYREPORT_EVENT:
            cJSON_AddNumberToObject( cj_payload, "inactiveTimeInSeconds", inactiveTimeInSeconds);
            break;
        case EXCEPTIONENCOUNTERED_EVENT:
            {
                cJSON* cj_error = cJSON_CreateObject();        
            
                cJSON_AddStringToObject( cj_payload, "unparsedDirective", unparsedDirective);
                cJSON_AddItemToObject( cj_payload, "error", cj_error );                
                
                cJSON_AddStringToObject( cj_error, "type", type);
                cJSON_AddStringToObject( cj_error, "message", message);
            }
            break;
        case SYSTEMEXCEPTION_EVENT:
            cJSON_AddStringToObject( cj_payload, "code", code);
            cJSON_AddStringToObject( cj_payload, "description", description);
            break;
        default:
            break;
    }

    return;
}

const char* alexa_system_event_construct( alexa_service* as )
{
    char* event_json;
    cJSON* cj_root = cJSON_CreateObject();
    cJSON* cj_event = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();
    
    switch(event)
    {
        case SYNCHRONIZESTATE_EVENT:
        case EXCEPTIONENCOUNTERED_EVENT:
            {
                cJSON* cj_context = cJSON_CreateArray();
                cJSON_AddItemToObject( cj_root, "context", cj_context );
                system_event_context_construct( cj_context );
            }
            break;
        
    }

    cJSON_AddItemToObject( cj_root, "event", cj_event );
    
    cJSON_AddItemToObject( cj_event, "header", cj_header );
    cJSON_AddItemToObject( cj_event, "payload", cj_payload );

    //
    system_event_header_construct( cj_header, event, msg_id );
    system_event_payload_construct( cj_payload, event, msg_id );    
    
    return ;
}

//
static int exception_process( alexa_service* as, cJSON* root )
{
    cJSON* header = as->header;
    cJSON* name;
    
    name = cJSON_GetObjectItem(header, "name");
    if( !name )
    {
        goto err;
    }
    
    if( !strcmp( name->valuestring, "Exception" ) )
    {
        //alexa_system_exception( as, root );
    }

    return 0;

err:
    return -1;
}


static int directive_reset_user_inactivity( alexa_service* as, cJSON* root )
{
    //reset the inactivity timer 
    return 0;
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
    
    if( !strcmp( name->valuestring, "ResetUserInactivity" ) )
    {
        directive_reset_user_inactivity( as, root );
    }

    return 0;

err:
    return -1;
}


int alexa_system_init(alexa_service* as)
{
    alexa_directive_register(NAMESPACE, directive_process );
    
    return 0;
}

int alexa_system_done(alexa_service* as)
{
    alexa_directive_unregister(NAMESPACE);    

    return 0;
}

/*******************************************************************************
	END OF FILE
*******************************************************************************/
