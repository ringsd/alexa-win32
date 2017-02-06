/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.

	File: alexa_alerts.c

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/01/14 18:15:56

*******************************************************************************/

#include	<string.h>

#include	"cjson/cjson.h"
#include	"list.h"
#include	"alexa_service.h"
#include	"alexa_directive.h"

#define NAMESPACE       "Alerts"

struct alexa_alerts{
    char*   messageId;
    struct list_head alerts_head;
};

struct alexa_alert_item{
    struct list_head list;
    char* type;
    char* token; //the unique id 
    char* scheduledTime; //The scheduled time for an alert in ISO 8601 format
};

enum{
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


static void alerts_event_header_construct( struct alexa_service* as, cJSON* cj_header, enum ALERTS_EVENT_ENUM event )
{
	struct alexa_alerts* alerts = &(as->alerts);
    int event_index = (int)event;
    int len = 0;
    
    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", alerts_event[event_index]);
    cJSON_AddStringToObject( cj_header, "messageId", alerts->messageId);
    
    return;
}

static void alerts_event_payload_construct(struct alexa_service* as, cJSON* cj_payload, enum ALERTS_EVENT_ENUM event, const char* token)
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
			cJSON_AddStringToObject(cj_payload, "token", token);
            break;
        default:
            break;
    }
    return;
}

const char* alexa_alerts_event_construct( struct alexa_service* as, enum ALERTS_EVENT_ENUM event, const char* token )
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
    alerts_event_header_construct( as, cj_header, event );
    alerts_event_payload_construct( as, cj_payload, event, token );
    
	event_json = cJSON_Print(cj_root);
    alexa_log_d( "%s\n", event_json );
    
	cJSON_Delete(cj_root);
    
    return event_json;
}

static int directive_set_alert( struct alexa_service* as, struct alexa_directive_item* item )
{
    cJSON* cj_payload = item->payload;
    cJSON* cj_token;
    cJSON* cj_type;
    cJSON* cj_scheduledTime;
	struct alexa_alerts* alerts = as->alerts;
    struct alexa_alert_item* alert_item;
	struct list_head* list_item;
	struct list_head* head;

    cj_token = cJSON_GetObjectItem(cj_payload, "token");
    if( cj_token == NULL ) goto err;

    cj_type = cJSON_GetObjectItem(cj_payload, "type");
    if( cj_type == NULL ) goto err;

    cj_scheduledTime = cJSON_GetObjectItem(cj_payload, "scheduledTime");
    if( cj_scheduledTime == NULL ) goto err;
    
	list_for_each(list_item, &alerts->alerts_head)
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

    list_add(&(alert_item->list), &alexas->alerts_head);

    if( strcmp( alert_item->valuestring, "TIMER" ) == 0 )
    {
        const char* event_string = NULL;
        //set timer
    }
    else if( strcmp( alert_item->valuestring, "ALERT" ) == 0 )
    {
        //set alert
    }
    
    //if set timer success
    //send set timer success event
    event_string = alexa_alerts_event_construct( as, SETALERTSUCCEEDED_EVENT, alert_item->token );
    
    //else
    //send set timer success event
    event_string = alexa_alerts_event_construct( as, SETALERTFAILED_EVENT, alert_item->token );
    
    
    return 0;
err:
    return -1;
}

static int directive_delete_alert( alexa_service* as, struct alexa_directive_item* item )
{
    cJSON* cj_payload = item->payload;
    cJSON* cj_token;
    struct alexa_alert_item* alert_item;
    
    cj_token = cJSON_GetObjectItem(cj_payload, "token");
    if( !cj_token ) goto err;
    
    //send the alert stop event
    event_string = alexa_alerts_event_construct( as, ALERTSTOPPED_EVENT, alert_item->token );
    
    list_for_each(alert_item, &alexas->alerts_head)
    {
        if( !strcmp(alert_item->token, cj_token->valuestring) )
        {
            alexa_free( alert_item->token );
            alert_item->token = NULL;
            alexa_free( alert_item->scheduledTime );
            alert_item->scheduledTime = NULL;
            alexa_free( alert_item->type );
            alert_item->type = NULL;
            list_del( alert_item );
            //delete the alert
            
            // alert 
            // if find the alert delete, and report success
            event_string = alexa_alerts_event_construct( as, DELETEALERTSUCCEEDED_EVENT, alert_item->token );
            //
            // else report fail
            event_string = alexa_alerts_event_construct( as, DELETEALERTFAILED_EVENT, alert_item->token );
            //
            break;;
        }
    }

    return 0;
err:
    return -1;
}

static int directive_process( alexa_service* as, struct alexa_directive_item* item )
{
    cJSON* cj_header = item->header;
    cJSON* cj_name;
    
    if( cj_header == NULL ) goto err;
    
    cj_name = cJSON_GetObjectItem(cj_header, "name");
    if( !cj_name ) goto err;
    
    if( !strcmp( cj_name->valuestring, "SetAlert" ) )
    {
        directive_set_alert( as, item );
    }
    else if( !strcmp( cj_name->valuestring, "DeleteAlert" ) )
    {
        directive_delete_alert( as, root );
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
        INIT_LIST_HEAD(&alerts->timer_head);
        INIT_LIST_HEAD(&alerts->alert_head);
    }
    
    return alerts;
}

int alexa_alerts_init(alexa_service* as)
{
    as->alerts = alerts_construct();
    
    //load alert from storage
    alexa_directive_register(NAMESPACE, directive_process );
    return 0;
}

int alexa_alerts_done(alexa_service* as)
{
    alexa_directive_unregister(NAMESPACE);    
    //save alert from storage
    
    alerts_destruct( as->alerts )
    return 0;
}


/*******************************************************************************
	END OF FILE
*******************************************************************************/
