/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.

    File: alexa_settting.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/02/17 09:47:17

*******************************************************************************/


#include "alexa_service.h"

#define TAG             "Setting"
#define NAMESPACE       "Setting"

struct alexa_setting{
    struct alexa_service* as;
    char messageId[ALEXA_UUID_LENGTH + 1];
    
    const char* locale;
};

enum SETTING_EVENT_ENUM{
    SETTINGSUPDATED_EVENT = 0,
};

static const char* setting_event[] = {
    "SettingsUpdated",
};

enum{
    ALEXA_LOCALE_en_US = 0,
    ALEXA_LOCALE_en_GB,
    ALEXA_LOCALE_de_DE,
};

static const char* setting_locale[] = 
{
    "en-US",
    "en-GB",
    "de-DE",
};

static void setting_event_header_construct(struct alexa_setting* setting, cJSON* cj_header, enum SETTING_EVENT_ENUM event)
{
    int event_index = (int)event;
    
    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", setting_event[event_index]);
    alexa_generate_uuid(setting->messageId, sizeof(setting->messageId));
    cJSON_AddStringToObject( cj_header, "messageId", setting->messageId);

    return;
}

static void setting_event_payload_construct(struct alexa_setting* setting, cJSON* cj_payload, enum SETTING_EVENT_ENUM event)
{
    switch( event )
    {
        case SETTINGSUPDATED_EVENT:
            {
                cJSON* cj_setting = cJSON_CreateArray();
                cJSON* cj_setting_item = cJSON_CreateObject();
                
                //locale
                cJSON_AddStringToObject( cj_setting_item, "key", "locale");
                cJSON_AddStringToObject( cj_setting_item, "value", setting->locale);
                
                cJSON_AddItemToArray( cj_setting, cj_setting_item );
                cJSON_AddItemToObject( cj_payload, "setting", cj_setting );
            }
            break;
        default:
            break;
    }
    return;
}

const char* alexa_setting_event_construct(struct alexa_setting* setting, enum SETTING_EVENT_ENUM event)
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
    setting_event_header_construct(setting, cj_header, event);
    setting_event_payload_construct(setting, cj_payload, event);

    event_json = cJSON_Print( cj_root );
    cJSON_Delete( cj_root );
    
    return event_json;    
}


static struct alexa_setting* setting_construct( void )
{
    struct alexa_setting* setting = alexa_new( struct alexa_setting );
    if( setting )
    {
        setting->locale = setting_locale[ALEXA_LOCALE_en_US];
    }
    return setting;
}

static void setting_destruct( struct alexa_setting* setting )
{
    alexa_delete( setting );
}

struct alexa_setting* alexa_setting_init(struct alexa_service* as)
{
    struct alexa_setting* setting = setting_construct();
    if(setting)
    {
        setting->as = as;
    }
    return setting;
}

int alexa_setting_done(struct alexa_setting* setting)
{
    if (setting) setting_destruct(setting);
    return 0;
}


/*******************************************************************************
    END OF FILE
*******************************************************************************/
