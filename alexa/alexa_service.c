/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.

	File: alexa_service.c

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/01/16 17:14:18

*******************************************************************************/

static void* alexa_service_process(void*)
{
    
    return NULL;
}

void alexa_service_init()
{
    //
    struct alexa_service* as = alexa_malloc( sizeof( struct alexa_service ) );
    if( as )
    {
        alexa_speechrecognizer_init( as );
        alexa_speechsynthesizer_init( as );
        alexa_alerts_init( as );
        alexa_audioplayer_init( as );
        alexa_playcontroller_init( as );
        alexa_speaker_init( as );
        alexa_system_init( as );
        
        //create thread to enable the alexa service
    }
    else
    {
        alexa_log_e( TAG, "as don't have enough memory.\n" );
    }
    
    
    return ;
}

void alexa_service_done()
{
    return ;
}

/*******************************************************************************
	END OF FILE
*******************************************************************************/
