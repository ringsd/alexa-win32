/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.

	File: alexa_speaker.c

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/01/13 16:14:32

*******************************************************************************/

#define NAMESPACE       "Speaker"


SetVolume Directive

AdjustVolume Directive
VolumeChanged Event
SetMute Directive
MuteChanged Event

static int directive_set_volume( alexa_service* as, cJSON* root )
{
    cJSON* payload = as->payload;
    cJSON* volume;
    
    volume = cJSON_GetObjectItem(payload, "volume");
    if( !volume )
    {
        goto err;
    }
    
    //[0, 100]
    volume->valueint;
    
    return 0;
err:
    return -1;
}

static int directive_adjust_volume( alexa_service* as, cJSON* root )
{
    cJSON* payload = as->payload;
    cJSON* volume;
    
    volume = cJSON_GetObjectItem(payload, "volume");
    if( !volume )
    {
        goto err;
    }
    
    //[-100, 100]
    volume->valueint;
    
    return 0;
err:
    return -1;
}

static int directive_set_mute( alexa_service* as, cJSON* root )
{
    cJSON* payload = as->payload;
    cJSON* mute;
    
    mute = cJSON_GetObjectItem(payload, "mute");
    if( !mute )
    {
        goto err;
    }
    
    if( mute->type == cJSON_False )
    {
        //
    }
    else
    {
        //
    }
    
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
    
    if( !strcmp( name->valuestring, "SetVolume" ) )
    {
        directive_set_volume( as, root );
    }
    else if( !strcmp( name->valuestring, "AdjustVolume" ) )
    {
        directive_adjust_volume( as, root );
    }
    else if( !strcmp( name->valuestring, "SetMute" ) )
    {
        directive_set_mute( as, root );
    }
    
    return 0;
    
err:
    return -1;
}

int alexa_speaker_init(alexa_service* as)
{
    alexa_directive_register(NAMESPACE, directive_process );
    
    return 0;
}

int alexa_speaker_done(alexa_service* as)
{
    alexa_directive_unregister(NAMESPACE);    

    return 0;
}


/*******************************************************************************
	END OF FILE
*******************************************************************************/
