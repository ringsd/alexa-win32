/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.

    File: alexa_alerts.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/14 18:15:56

*******************************************************************************/

#include    <string.h>
#include    "alexa_service.h"

#define TAG             "Alerts"
#define NAMESPACE       "Alerts"

struct alexa_alerts{
    struct alexa_service* as;
    char   messageId[ALEXA_UUID_LENGTH+1];
    struct list_head alerts_head;
};

struct alexa_alert_item{
    struct list_head list;

    int active;

    char* type;
    char* token; //the unique id 
    char* scheduledTime; //The scheduled time for an alert in ISO 8601 format
};

enum ALERTS_EVENT_ENUM{
    SETALERTSUCCEEDED_EVENT,
    SETALERTFAILED_EVENT,
    
    DELETEALERTSUCCEEDED_EVENT,
    DELETEALERTFAILED_EVENT,
    
    ALERTSTARTED_EVENT,
    ALERTSTOPPED_EVENT,
    ALERTENTEREDFOREGROUND_EVENT,
    ALERTENTEREDBACKGROUND_EVENT,

    ALERTSSTATE_EVENT,
};

static const char* alerts_event[] = {
    "SetAlertSucceeded",
    "SetAlertFailed",
    
    "DeleteAlertSucceeded",
    "DeleteAlertFailed",
    
    "AlertStarted",
    "AlertStopped",
    "AlertEnteredForeground",
    "AlertEnteredBackground",

    "AlertsState",
};

/*
*@brief construct the alerts state
*
*https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/context
*
*@param struct alexa_service* as, the alexa_service object
*@return Alerts.AlertsState cJSON object
*/
cJSON* alerts_alerts_state(struct alexa_alerts* alerts)
{
    cJSON* cj_alerts_state = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();
    cJSON* cj_all_alerts = cJSON_CreateArray();
    cJSON* cj_active_alerts = cJSON_CreateArray();
    cJSON* cj_alert_item;

    struct alexa_alert_item* alert_item;

    cJSON_AddItemToObject(cj_alerts_state, "header", cj_header);
    cJSON_AddStringToObject(cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject(cj_header, "name", alerts_event[ALERTSSTATE_EVENT]);

    cJSON_AddItemToObject(cj_alerts_state, "payload", cj_payload);
    cJSON_AddItemToObject(cj_payload, "allAlerts", cj_all_alerts);
    cJSON_AddItemToObject(cj_payload, "activeAlerts", cj_active_alerts);

    list_for_each_entry_type(alert_item, &alerts->alerts_head, struct alexa_alert_item, list)
    {
        cj_alert_item = cJSON_CreateObject();

        cJSON_AddStringToObject(cj_alert_item, "token", alert_item->token);
        cJSON_AddStringToObject(cj_alert_item, "type", alert_item->type);
        cJSON_AddStringToObject(cj_alert_item, "scheduledTime", alert_item->scheduledTime);

        cJSON_AddItemToArray(cj_all_alerts, cj_alert_item);
        if (alert_item->active)
        {
            cj_alert_item = cJSON_Duplicate(cj_alert_item, 1);
            cJSON_AddItemToArray(cj_active_alerts, cj_alert_item);
        }
    }

    return cj_alerts_state;
}

static void alerts_event_header_construct(struct alexa_alerts* alerts, cJSON* cj_header, enum ALERTS_EVENT_ENUM event)
{
    int event_index = (int)event;
    
    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", alerts_event[event_index]);
    cJSON_AddStringToObject( cj_header, "messageId", alerts->messageId);
    
    return;
}

static void alerts_event_payload_construct(struct alexa_alerts* alerts, cJSON* cj_payload, enum ALERTS_EVENT_ENUM event, const char* token)
{
    alerts = alerts;

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
            cJSON_AddStringToObject(cj_payload, "token", token);
            break;
        default:
            break;
    }
    return;
}

static const char* alexa_alerts_event_construct( struct alexa_alerts* alerts, enum ALERTS_EVENT_ENUM event, const char* token )
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
    alerts_event_header_construct(alerts, cj_header, event);
    alerts_event_payload_construct(alerts, cj_payload, event, token);
    
    event_json = cJSON_Print(cj_root);
    sys_log_d(TAG, "%s\n", event_json );
    
    cJSON_Delete(cj_root);
    
    return event_json;
}

static int directive_set_alert(struct alexa_alerts* alerts, struct alexa_directive_item* item)
{
    cJSON* cj_payload = item->payload;
    cJSON* cj_token;
    cJSON* cj_type;
    cJSON* cj_scheduledTime;
    struct alexa_alert_item* alert_item;

    const char* event_string;

    cj_token = cJSON_GetObjectItem(cj_payload, "token");
    if( cj_token == NULL ) goto err;

    cj_type = cJSON_GetObjectItem(cj_payload, "type");
    if( cj_type == NULL ) goto err;

    cj_scheduledTime = cJSON_GetObjectItem(cj_payload, "scheduledTime");
    if( cj_scheduledTime == NULL ) goto err;
    
    list_for_each_entry_type(alert_item, &alerts->alerts_head, struct alexa_alert_item, list)
    {
        if( !strcmp(alert_item->token, cj_token->valuestring) )
        {
            alexa_free( alert_item->token );
            alert_item->token = NULL;
            alexa_free( alert_item->scheduledTime );
            alert_item->scheduledTime = NULL;
            alexa_free( alert_item->type );
            alert_item->type = NULL;
            break;;
        }
    }

    if( alert_item == NULL ) 
    {
        alert_item = alexa_new( struct alexa_alert_item );
    }
    
    if( alert_item == NULL ) goto err;
    
    alert_item->type = alexa_strdup( cj_type->valuestring );
    alert_item->token = alexa_strdup( cj_token->valuestring );
    alert_item->scheduledTime = alexa_strdup( cj_scheduledTime->valuestring );

    list_add(&(alert_item->list), &alerts->alerts_head);

    if( strcmp( alert_item->type, "TIMER" ) == 0 )
    {
        //set timer
    }
    else if (strcmp(alert_item->type, "ALERT") == 0)
    {
        //set alert
    }
    
    //if set timer success
    //send set timer success event
    event_string = alexa_alerts_event_construct(alerts, SETALERTSUCCEEDED_EVENT, alert_item->token);
    
    //else
    //send set timer success event
    event_string = alexa_alerts_event_construct(alerts, SETALERTFAILED_EVENT, alert_item->token);
    
    
    return 0;
err:
    return -1;
}

static int directive_delete_alert( struct alexa_alerts* alerts, struct alexa_directive_item* item )
{
    cJSON* cj_payload = item->payload;
    cJSON* cj_token;
    struct alexa_alert_item* alert_item;

    const char* event_string;
    
    cj_token = cJSON_GetObjectItem(cj_payload, "token");
    if( !cj_token ) goto err;
    
    //send the alert stop event
    event_string = alexa_alerts_event_construct(alerts, ALERTSTOPPED_EVENT, (const char*)cj_token->valuestring);
    
    list_for_each_entry_type(alert_item, &alerts->alerts_head, struct alexa_alert_item, list)
    {
        if( !strcmp(alert_item->token, cj_token->valuestring) )
        {
            alexa_free( alert_item->token );
            alert_item->token = NULL;
            alexa_free( alert_item->scheduledTime );
            alert_item->scheduledTime = NULL;
            alexa_free( alert_item->type );
            alert_item->type = NULL;
            list_del(&(alert_item->list));
            //delete the alert

            // alert 
            // if find the alert delete, and report success
            event_string = alexa_alerts_event_construct(alerts, DELETEALERTSUCCEEDED_EVENT, (const char*)alert_item->token);
            //
            // else report fail
            event_string = alexa_alerts_event_construct(alerts, DELETEALERTFAILED_EVENT, (const char*)alert_item->token);
            //
            break;;
        }
    }

    return 0;
err:
    return -1;
}

static int directive_process(struct alexa_alerts* alerts, struct alexa_directive_item* item)
{
    cJSON* cj_header = item->header;
    cJSON* cj_name;
    
    if( cj_header == NULL ) goto err;
    
    cj_name = cJSON_GetObjectItem(cj_header, "name");
    if( !cj_name ) goto err;
    
    if( !strcmp( cj_name->valuestring, "SetAlert" ) )
    {
        directive_set_alert(alerts, item);
    }
    else if( !strcmp( cj_name->valuestring, "DeleteAlert" ) )
    {
        directive_delete_alert(alerts, item);
    }
    return 0;
    
err:
    return -1;
}

static struct alexa_alerts* alerts_construct( void )
{
    struct alexa_alerts* alerts = alexa_new( struct alexa_alerts );
    if( alerts != NULL )
    {
        INIT_LIST_HEAD(&alerts->alerts_head);
    }
    
    return alerts;
}

static void alerts_destruct(struct alexa_alerts* alerts)
{
    //for each stop the alert or timer
    alexa_delete(alerts);
    return;
}

struct alexa_alerts* alexa_alerts_init(struct alexa_service* as)
{
    struct alexa_alerts* alerts = alerts_construct();
    //load alert from storage
    if (alerts)
    {
        alexa_directive_register(NAMESPACE, (directive_process_func)directive_process, alerts);
        alerts->as = as;
    }

    return alerts;
}

int alexa_alerts_done(struct alexa_alerts* alerts)
{
    alexa_directive_unregister(NAMESPACE);

    //save alert from storage

    if (alerts) alerts_destruct(alerts);
    return 0;
}


/*******************************************************************************
    END OF FILE
*******************************************************************************/
