/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.
	
	File: alexa_context.c

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/01/17 14:56:19
    https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/context    

*******************************************************************************/

/*
 *@brief construct the alexa context
 *@param struct alexa_service* as, the alexa_service object
 *@return alexa_context json object
 */
cJSON* alexa_context_get_state( struct alexa_service* as )
{
    cJSON* cj_context = cJSON_CreateArray();

    if( !cj_context )
    {
        sys_log_e( TAG, "memory leak" );
        goto err;
    }
    
    //AudioPlayer.PlaybackState
    cj_state = audioplayer_playback_state( as );
    if(cj_state) cJSON_AddItemToArray( cj_context, cj_state );
    
    //Alerts.AlertsState
    cj_state = alerts_alerts_state( as );
    if(cj_state) cJSON_AddItemToArray( cj_context, cj_state );

    //Speaker.VolumeState
    cj_state = speaker_volume_state( as );
    if(cj_state) cJSON_AddItemToArray( cj_context, cj_state );

    //SpeechSynthesizer.SpeechState
    cj_state = speechsynthesizer_speech_state( as );
    if(cj_state) cJSON_AddItemToArray( cj_context, cj_state );
    
err:    
    return cj_context;
}




/*******************************************************************************
	END OF FILE
*******************************************************************************/
