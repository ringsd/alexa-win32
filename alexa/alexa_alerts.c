/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.

	File: alexa_alerts.c

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/01/14 18:15:56

*******************************************************************************/

#define NAMESPACE       "Alerts"

enum{
    SETALERTSUCCEEDED_EVENT,
    MUTECHANGED_EVENT,
    
    SETALERTSUCCEEDED_EVENT,
    SETALERTFAILED_EVENT,
    
    DELETEALERTSUCCEEDED_EVENT,
    DELETEALERTFAILED_EVENT,
    
    ALERTSTARTED_EVENT,
    ALERTSTOPPED_EVENT,
    ALERTENTEREDFOREGROUND_EVENT,
    ALERTENTEREDBACKGROUND_EVENT,
}ALERTS_EVENT_ENUM;

static const char* alerts_event[] = {
    "SetAlertSucceeded",
    "SetAlertFailed",
    
    "DeleteAlertSucceeded",
    "DeleteAlertFailed",
    
    "AlertStarted",
    "AlertStopped",
    "AlertEnteredForeground",
    "AlertEnteredBackground",
};

static void alerts_event_header_construct( cJSON* cj_header, ALERTS_EVENT_ENUM event, const char* msg_id )
{
    int event_index = (int)event;
    int len = 0;
    
    assert( msg_id != NULL );

    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", alerts_event[event_index]);
    cJSON_AddStringToObject( cj_header, "messageId", msg_id);
    
    return;
}

static void alerts_event_payload_construct(cJSON* cj_payload, ALERTS_EVENT_ENUM event)
{
    int event_index = (int)event;
    int len = 0;
    
    switch( event )
    {
        case SETALERTSUCCEEDED_EVENT:
        case SETALERTFAILED_EVENT:
        
        case DELETEALERTSUCCEEDED_EVENT:
        case DELETEALERTFAILED_EVENT:
        
        case ALERTSTARTED_EVENT:
        case ALERTSTOPPED_EVENT:
        case ALERTENTEREDFOREGROUND_EVENT:
        case ALERTENTEREDBACKGROUND_EVENT:
            cJSON_AddNumberToObject( cj_payload, "token", token);
            break;
        default:
            break;
    }
    return;
}

const char* alexa_alerts_event_construct( alexa_service* as )
{
    char* event_json;
    cJSON* cj_root = cJSON_CreateObject();
    cJSON* cj_event = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();

    cJSON_AddItemToObject( cj_root, "event", cj_event );
    
    cJSON_AddItemToObject( cj_event, "header", cj_header );
    cJSON_AddItemToObject( cj_event, "payload", cj_payload );

    //
    alerts_event_header_construct( cj_header, event, msg_id );
    alerts_event_payload_construct( cj_payload, event, msg_id );
    
    event_json = cJSON_Print( root );
    alexa_log_d( "%s\n", event_json );
    
    cJSON_Delete( root );
    
    return event_json;
}

static int directive_set_alert( alexa_service* as, cJSON* root )
{
    cJSON* payload = as->payload;
    cJSON* cj_token;
    cJSON* cj_type;
    cJSON* cj_scheduledTime;
    
    cj_token = cJSON_GetObjectItem(payload, "token");
    if( !cj_token )
    {
        goto err;
    }

    cj_type = cJSON_GetObjectItem(payload, "type");
    if( !cj_type )
    {
        goto err;
    }

    cj_scheduledTime = cJSON_GetObjectItem(payload, "scheduledTime");
    if( !cj_scheduledTime )
    {
        goto err;
    }
    
    if( !strcmp(cj_type->valuestring, "TIMER") )
    {
        // timer 
        // find the timer cj_token->valuestring
        // if has this timer reset timer
        // else add the new timer
    }
    else if( !strcmp(cj_type->valuestring, "ALARM") )
    {
        // alarm
        // find the alarm cj_token->valuestring
        // if has this alarm reset alarm
        // else add the new alarm
    }
    
    return 0;
err:
    return -1;
}

static int directive_delete_alert( alexa_service* as, cJSON* root )
{
    cJSON* cj_payload = as->cj_payload;
    cJSON* cj_token;
    
    cj_token = cJSON_GetObjectItem(cj_payload, "token");
    if( !cj_token )
    {
        goto err;
    }
    
    // alert 
    // find the alert by cj_token->valuestring
    // if find the alert delete, and report success
    // else report fail
    
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
    
    if( !strcmp( name->valuestring, "SetAlert" ) )
    {
        directive_set_alert( as, root );
    }
    else if( !strcmp( name->valuestring, "DeleteAlert" ) )
    {
        directive_delete_alert( as, root );
    }
    return 0;
    
err:
    return -1;
}

int alexa_alerts_init(alexa_service* as)
{
    alexa_directive_register(NAMESPACE, directive_process );
    
    return 0;
}

int alexa_alerts_done(alexa_service* as)
{
    alexa_directive_unregister(NAMESPACE);    

    return 0;
}


/*******************************************************************************
	END OF FILE
*******************************************************************************/
