/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.

    File: alexa_system.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/13 16:22:09
    
    https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/system

*******************************************************************************/

#include <stdio.h>
#include "alexa_service.h"
#include "alexa_platform.h"
#include "alexa_base.h"

#define     TAG            "system"
#define     NAMESPACE      "System"

struct alexa_system{
    struct alexa_service* as;
    char     messageId[ALEXA_UUID_LENGTH + 1];
    time_t   last_user_active;
    char     inactiveTimeInSeconds[32];
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

static cJSON* system_event_context_construct(struct alexa_system* system)
{
    return alexa_context_get_state(system->as);
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
        {
            time_t current_time;
            time(&current_time);
            //sprintf(system->inactiveTimeInSeconds, "%lld", current_time);
            cJSON_AddNumberToObject(cj_payload, "inactiveTimeInSeconds", difftime(current_time, system->last_user_active));
            break;
        }
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

static const char* alexa_system_event_construct(struct alexa_system* system, enum SYSTEM_EVENT_ENUM event)
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
            cJSON* cj_context = system_event_context_construct(system);
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

const char* alexa_system_synchronizestate_event(struct alexa_system* system)
{
    return alexa_system_event_construct(system, SYNCHRONIZESTATE_EVENT);
}

const char* alexa_system_userinactivityreport_event(struct alexa_system* system)
{
    return alexa_system_event_construct(system, USERINACTIVITYREPORT_EVENT);
}

//
static int exception_process( struct alexa_system* system, struct alexa_directive_item* item )
{
    cJSON* cj_payload = item->payload;
    cJSON* cj_code;
    cJSON* cj_description;
    
    system = system;

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


static int directive_reset_user_inactivity( struct alexa_system* system, struct alexa_directive_item* item )
{
    item = item;

    //reset the inactivity timer 
    time(&system->last_user_active);

    return 0;
}

static int directive_process(struct alexa_system* system, struct alexa_directive_item* item)
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
        directive_reset_user_inactivity(system, item);
    }
    else if (!strcmp(cj_name->valuestring, "Exception"))
    {
        exception_process(system, item);
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
        time(&system->last_user_active);
    }
    return system;
}

static void system_destruct( struct alexa_system* system )
{
    alexa_delete( system );
}

struct alexa_system* alexa_system_init(struct alexa_service* as)
{
    struct alexa_system* system = system_construct();
    if (system != NULL)
    {
        system->as = as;
        alexa_directive_register(NAMESPACE, (directive_process_func)directive_process, system);
    }
    else
    {
    }
    return system;
}

int alexa_system_done(struct alexa_system* system)
{
    if (system != NULL)
    {
        system_destruct(system);
        alexa_directive_unregister(NAMESPACE);
    }

    return 0;
}

/*******************************************************************************
    END OF FILE
*******************************************************************************/
