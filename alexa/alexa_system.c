/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.

    File: alexa_system.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/13 16:22:09
    
    https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/system

*******************************************************************************/

#include    "alexa_service.h"

#define     TAG            "system"
#define     NAMESPACE      "System"

struct alexa_system{
    char     messageId[48];
    int      inactiveTimeInSeconds;
    char*    unparsedDirective;
    char*    type;
    char*    message;
    char*    code;
    char*    description;
};

enum SYSTEM_EVENT_ENUM{
    SYNCHRONIZESTATE_EVENT = 0,
    USERINACTIVITYREPORT_EVENT,
    EXCEPTIONENCOUNTERED_EVENT,
    SYSTEMEXCEPTION_EVENT,
};

static const char* system_event[] = {
    "SynchronizeState",
    "UserInactivityReport",
    "ExceptionEncountered",
    "SYSTEMEXCEPTION_EVENT",
};

enum{
    INVALID_REQUEST_EXCEPTION=400,
    UNAUTHORIZED_REQUEST_EXCEPTION=403,
    THROTTLING_EXCEPTION=429,
    INTERNAL_SERVICE_EXCEPTION=500,
    SYSTEM_EXCEPTION_NA=503,
};

struct system_exception{
    int         err_code;
    const char* err_desc;
};

static struct system_exception system_exception_list[] = {
    {INVALID_REQUEST_EXCEPTION,         "The request was malformed."},
    {UNAUTHORIZED_REQUEST_EXCEPTION,    "The request was not authorized."},
    {THROTTLING_EXCEPTION,              "Too many requests to the Alexa Voice Service."},
    {INTERNAL_SERVICE_EXCEPTION,        "Internal service exception."},
    {SYSTEM_EXCEPTION_NA,               "The Alexa Voice Service is unavailable."},
};

static cJSON* system_event_context_construct( struct alexa_service* as )
{
    return alexa_context_get_state(as);
}

static void system_event_header_construct( struct alexa_system* system, cJSON* cj_header, enum SYSTEM_EVENT_ENUM event )
{
    int event_index = (int)event;
    
    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", system_event[event_index]);
    alexa_generate_uuid(system->messageId, sizeof(system->messageId));
    cJSON_AddStringToObject( cj_header, "messageId", system->messageId);

    return;
}

static void system_event_payload_construct( struct alexa_system* system, cJSON* cj_payload, enum SYSTEM_EVENT_ENUM event )
{
    switch(event)
    {
        case SYNCHRONIZESTATE_EVENT:
            //don't have payload
            break;
        case USERINACTIVITYREPORT_EVENT:
            cJSON_AddNumberToObject( cj_payload, "inactiveTimeInSeconds", system->inactiveTimeInSeconds);
            break;
        case EXCEPTIONENCOUNTERED_EVENT:
            {
                cJSON* cj_error = cJSON_CreateObject();        
            
                cJSON_AddStringToObject(cj_payload, "unparsedDirective", system->unparsedDirective);
                cJSON_AddItemToObject( cj_payload, "error", cj_error );                
                
                cJSON_AddStringToObject(cj_error, "type", system->type);
                cJSON_AddStringToObject(cj_error, "message", system->message);
            }
            break;
        case SYSTEMEXCEPTION_EVENT:
            cJSON_AddStringToObject(cj_payload, "code", system->code);
            cJSON_AddStringToObject(cj_payload, "description", system->description);
            break;
        default:
            break;
    }

    return;
}

const char* alexa_system_event_construct( alexa_service* as, enum SYSTEM_EVENT_ENUM event )
{
    struct alexa_system* system = as->system;
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
            cJSON* cj_context = system_event_context_construct( as );
            cJSON_AddItemToObject( cj_root, "context", cj_context );
            break;
        }
        default:
            break;
    }

    cJSON_AddItemToObject( cj_root, "event", cj_event );
    
    cJSON_AddItemToObject( cj_event, "header", cj_header );
    cJSON_AddItemToObject( cj_event, "payload", cj_payload );

    //
    system_event_header_construct( system, cj_header, event );
    system_event_payload_construct( system, cj_payload, event );    

    event_json = cJSON_Print( cj_root );
    sys_log_d( TAG, "%s\n", event_json );
    
    cJSON_Delete( cj_root );    
    
    return event_json;
}

const char* alexa_system_synchronizestate_construct(alexa_service* as)
{
    return alexa_system_event_construct(as, SYNCHRONIZESTATE_EVENT);
}

//
static int exception_process( struct alexa_service* as, struct alexa_directive_item* item )
{
    cJSON* cj_payload = item->payload;
    cJSON* cj_code;
    cJSON* cj_description;
    
    as = as;

    cj_code = cJSON_GetObjectItem(cj_payload, "code");
    if (!cj_code)
    {
        goto err;
    }

    cj_description = cJSON_GetObjectItem(cj_payload, "description");
    if (!cj_description)
    {
        goto err;
    }

    sys_log_e(TAG, "Has exception code %s -> %s\n", cj_code->valuestring, cj_description->valuestring);

    return 0;

err:
    return -1;
}


static int directive_reset_user_inactivity( struct alexa_service* as, struct alexa_directive_item* item )
{
    as = as;
    item = item;
    //reset the inactivity timer 
    return 0;
}

static int directive_process(alexa_service* as, struct alexa_directive_item* item)
{
    cJSON* cj_header = item->header;
    cJSON* cj_name;
    
    cj_name = cJSON_GetObjectItem(cj_header, "name");
    if (!cj_name)
    {
        goto err;
    }
    
    if (!strcmp(cj_name->valuestring, "ResetUserInactivity"))
    {
        directive_reset_user_inactivity( as, item );
    }
    else if (!strcmp(cj_name->valuestring, "Exception"))
    {
        exception_process(as, item);
    }
    
    return 0;

err:
    return -1;
}

static struct alexa_system* system_construct( void )
{
    struct alexa_system* system = alexa_new( struct alexa_system );
    if( system )
    {
        
    }
    return system;
}

static void system_destruct( struct alexa_system* system )
{
    alexa_delete( system );
}

int alexa_system_init(alexa_service* as)
{
    as->system = system_construct();
    alexa_directive_register(NAMESPACE, directive_process );
    
    return 0;
}

int alexa_system_done(alexa_service* as)
{
    system_destruct(as->system);
    alexa_directive_unregister(NAMESPACE);    

    return 0;
}

/*******************************************************************************
    END OF FILE
*******************************************************************************/
