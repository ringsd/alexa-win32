/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.

	File: alexa_audioplayer.c

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/01/13 15:39:28
    https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/audioplayer

*******************************************************************************/

#define NAMESPACE       "AudioPlayer"

#define URL_CID         "cid:"

enum{
    PLAYBACKSTARTED_EVENT = 0,
    PLAYBACKNEARLYFINISHED_EVENT,
    PROGRESSREPORTDELAYELAPSED_EVENT,
    PROGRESSREPORTINTERVALELAPSED_EVENT,
    PLAYBACKSTUTTERSTARTED_EVENT,
    PLAYBACKSTUTTERFINISHED_EVENT,
    PLAYBACKFINISHED_EVENT,
    PLAYBACKFAILED_EVENT,
    
    PLAYBACKSTOPPED_EVENT,
    PLAYBACKPAUSED_EVENT,
    PLAYBACKRESUMED_EVENT,
    
    PLAYBACKQUEUECLEARED_EVENT,
    STREAMMETADATAEXTRACTED_EVENT,
    
    PLAYBACKSTATE_EVENT,
    
}AUDIOPLAYER_EVENT_ENUM;

static const char* audioplayer_event[] = {
    "PlaybackStarted",
    "PlaybackNearlyFinished",
    "ProgressReportDelayElapsed",
    "ProgressReportIntervalElapsed",
    "PlaybackStutterStarted",
    "PlaybackStutterFinished",
    "PlaybackFinished",
    "PlaybackFailed",
    
    "PlaybackStopped",
    "PlaybackPaused",
    "PlaybackResumed",
    
    "PlaybackQueueCleared",
    "StreamMetadataExtracted",
    
    "PlaybackState",
};

enum AUDIOPLAYER_STATE{
    AUDIOPLAYER_STATE_IDLE, //the AUDIOPLAYER is idle
    AUDIOPLAYER_STATE_PLAYING,
    AUDIOPLAYER_STATE_STOPPED,
    AUDIOPLAYER_STATE_PAUSED,
    AUDIOPLAYER_STATE_BUFFER_UNDERRUN,
    AUDIOPLAYER_STATE_FINISHED,
};


/*
 *@brief construct the audio player playback state
 *
 *https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/context
 *
 *@param struct alexa_service* as, the alexa_service object
 *@return AudioPlayer.PlaybackState cJSON object
 */
cJSON* audioplayer_playback_state(struct alexa_service* as )
{
    cJSON* cj_playback_state = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();
    
    cJSON_AddItemToObject( cj_playback_state, "header", cj_header );
    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", audioplayer_event[PLAYBACKSTATE_EVENT]);
    
    cJSON_AddItemToObject( cj_playback_state, "payload", cj_payload );
    cJSON_AddStringToObject( cj_payload, "token", token);
    cJSON_AddNumberToObject( cj_payload, "offsetInMilliseconds", offsetInMilliseconds);
    cJSON_AddStringToObject( cj_payload, "playerActivity", playerActivity);
    
    return cj_playback_state;
}

static void audioplayer_event_header_construct( cJSON* cj_header, AUDIOPLAYER_EVENT_ENUM event, const char* msg_id )
{
    int event_index = (int)event;
    int len = 0;
    
    assert( msg_id != NULL );

    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", audioplayer_event[event_index]);
    cJSON_AddStringToObject( cj_header, "messageId", msg_id);
    
    return;
}

static void audioplayer_event_payload_construct(cJSON* cj_payload, AUDIOPLAYER_EVENT_ENUM event)
{
    int event_index = (int)event;
    int len = 0;
    
    switch( event )
    {
        case PLAYBACKSTARTED_EVENT:
        case PLAYBACKNEARLYFINISHED_EVENT:
        case PROGRESSREPORTDELAYELAPSED_EVENT:
        case PROGRESSREPORTINTERVALELAPSED_EVENT:
        case PLAYBACKSTUTTERSTARTED_EVENT:
        
        case PLAYBACKFINISHED_EVENT:
        
        case PLAYBACKSTOPPED_EVENT:
        case PLAYBACKPAUSED_EVENT:
        case PLAYBACKRESUMED_EVENT:
            cJSON_AddStringToObject( cj_payload, "token", token);
            cJSON_AddNumberToObject( cj_payload, "offsetInMilliseconds", offsetInMilliseconds);
            break;
        case PLAYBACKSTUTTERFINISHED_EVENT:
            cJSON_AddStringToObject( cj_payload, "token", token);
            cJSON_AddNumberToObject( cj_payload, "offsetInMilliseconds", offsetInMilliseconds);
            cJSON_AddNumberToObject( cj_payload, "stutterDurationInMilliseconds", stutterDurationInMilliseconds);
            break;
        case PLAYBACKFAILED_EVENT:
            {
                cJSON* currentPlaybackState = cJSON_CreateObject();
                cJSON* error = cJSON_CreateObject();

                cJSON_AddStringToObject( cj_payload, "token", token);
                cJSON_AddItemToObject( cj_payload, "currentPlaybackState", currentPlaybackState);
                cJSON_AddStringToObject( currentPlaybackState, "token", token);
                cJSON_AddNumberToObject( currentPlaybackState, "offsetInMilliseconds", offsetInMilliseconds);
                cJSON_AddStringToObject( currentPlaybackState, "playerActivity", playerActivity);
                
                cJSON_AddItemToObject( cj_payload, "error", error);
                cJSON_AddStringToObject( currentPlaybackState, "type", type);
                cJSON_AddStringToObject( currentPlaybackState, "message", message);
            }
            break;
        case PLAYBACKQUEUECLEARED_EVENT:
            //don't need payload param
            break;
        case STREAMMETADATAEXTRACTED_EVENT:
            //not implement
            break;
        default:
            break;
    }
    
    return;
}

const char* alexa_audioplayer_event_construct(struct alexa_service* as )
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
    audioplayer_event_header_construct( cj_header, event, msg_id );
    audioplayer_event_payload_construct( cj_payload, event, msg_id );
    
    event_json = cJSON_Print( root );
    alexa_log_d( "%s\n", event_json );
    
    cJSON_Delete( root );
    
    return event_json;
}

static int directive_play(struct alexa_service* as, cJSON* root )
{
    cJSON* payload = as->payload;
    
    playBehavior = cJSON_GetObjectItem(payload, "playBehavior");
    if( !playBehavior )
    {
        goto err;
    }
    
    audioItem = cJSON_GetObjectItem(payload, "audioItem");
    if( !audioItem )
    {
        goto err;
    }
    
    audioItemId = cJSON_GetObjectItem(audioItem, "audioItemId");
    if( !audioItemId )
    {
        goto err;
    }
    audioItemId->valuestring;
    
    stream = cJSON_GetObjectItem(audioItem, "stream");
    if( !stream )
    {
        goto err;
    }

    url = cJSON_GetObjectItem(stream, "url");
    streamFormat = cJSON_GetObjectItem(stream, "streamFormat");
    offsetInMilliseconds = cJSON_GetObjectItem(stream, "offsetInMilliseconds");
    expiryTime = cJSON_GetObjectItem(stream, "expiryTime");
    
    progressReport = cJSON_GetObjectItem(stream, "progressReport");
    if( progressReport )
    {
        expiryTime = cJSON_GetObjectItem(progressReport, "progressReportDelayInMilliseconds");
        expiryTime = cJSON_GetObjectItem(progressReport, "progressReportIntervalInMilliseconds");
    }
    token = cJSON_GetObjectItem(stream, "token");
    expectedPreviousToken = cJSON_GetObjectItem(stream, "expectedPreviousToken");
    
    
    if( !strncmp(url->valuestring, URL_CID, strlen(URL_CID) ) )
    {
        //cid
        
        //has some audio data
    }
    else
    {
        //http or https  
        
        //has new url
    }

    //build new item

    if( !strcmp( playBehavior->valuestring, "REPLACE_ALL" ) )
    {
        //play the new item 
        
        //clear queue
        
        //add the new item to the queueu
        
        //send PlaybackStopped Event        
    }
    else if( !strcmp( playBehavior->valuestring, "ENQUEUE" ) )
    {
        //add the new item to the queue
    }
    else if( !strcmp( playBehavior->valuestring, "REPLACE_ENQUEUED" ) )
    {
        //clear not play item

        //add the new item to the queue
    }

    return 0;

err:
    return -1;
}


static int directive_stop(struct alexa_service* as, cJSON* root )
{
    //send PlaybackStopped Event
    
    return 0;
err:
    return -1;
}


static int directive_clearqueue(struct alexa_service* as, cJSON* root )
{
    cJSON* payload = as->payload;
    cJSON* clearBehavior;
    
    clearBehavior = cJSON_GetObjectItem(payload, "clearBehavior");
    if( !clearBehavior )
    {
        goto err;
    }

    if( !strcmp( clearBehavior->valuestring, "CLEAR_ENQUEUED" ) )
    {
        //clear queue
        //continue play the exist stream
    }
    else if( !strcmp( clearBehavior->valuestring, "CLEAR_ALL" ) )
    {
        //clear queue
        //stop play the exist stream
        
        //send PlaybackStopped Event        
        
    }
    
    return 0;
err:
    return -1;
}


static int directive_process(struct alexa_service* as, cJSON* root )
{
    cJSON* header = as->header;
    cJSON* name;
    
    alexa_log_d( TAG, "directive %s\n", NAMESPACE );
    
    name = cJSON_GetObjectItem(header, "name");
    if( !name )
    {
        goto err;
    }
    
    if( !strcmp( name->valuestring, "Play" ) )
    {
        directive_play( as, root );
    }
    else if( !strcmp( name->valuestring, "Stop" ) )
    {
        directive_stop( as, root );
    }
    else if( !strcmp( name->valuestring, "ClearQueue" ) )
    {
        directive_clearqueue( as, root );
    }
    
    return 0;
    
err:
    return -1;
}


/*
 *@brief init the alexa audio player
 *@param struct alexa_service* as, the alexa_service object
 *@return 0 success, otherwise fail
 */
int alexa_audioplayer_init(struct alexa_service* as)
{
    alexa_directive_register(NAMESPACE, directive_process );
    
    return 0;
}

/*
 *@brief done the alexa audio player
 *@param struct alexa_service* as, the alexa_service object
 *@return 0 success, otherwise fail
 */
int alexa_audioplayer_done(struct alexa_service* as)
{
    alexa_directive_unregister(NAMESPACE);
    return 0;
}

/*******************************************************************************
	END OF FILE
*******************************************************************************/
