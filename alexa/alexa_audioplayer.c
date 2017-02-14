/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.

    File: alexa_audioplayer.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/13 15:39:28
    https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/audioplayer

*******************************************************************************/

#include    <string.h>
#include    "alexa_service.h"

#define NAMESPACE       "AudioPlayer"
#define TAG                "AudioPlayer"

#define URL_CID         "cid:"

#define    TODO            1

enum AUDIOPLAYER_EVENT_ENUM{
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
    
};

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

enum AUDIOPLAYER_STATE_ENUM{
    AUDIOPLAYER_STATE_IDLE = 0, //the AUDIOPLAYER is idle
    AUDIOPLAYER_STATE_PLAYING,
    AUDIOPLAYER_STATE_STOPPED,
    AUDIOPLAYER_STATE_PAUSED,
    AUDIOPLAYER_STATE_BUFFER_UNDERRUN,
    AUDIOPLAYER_STATE_FINISHED,
};

static const char* audioplayer_state[] = {
    "IDLE",
    "PLAYING",
    "STOPPED",
    "PAUSED",
    "BUFFER_UNDERRUN",
    "FINISHED",
};

struct alexa_audioplayer{
    enum AUDIOPLAYER_STATE_ENUM state;
    char*       token;
    char*       messageId;
    int         offsetInMilliseconds;
    const char* playerActivity;
    int         stutterDurationInMilliseconds;
    char*       type;
    char*       message;
};

static void audioplayer_set_state( struct alexa_service* as, enum AUDIOPLAYER_STATE_ENUM state )
{
    struct alexa_audioplayer* ap = as->ap;
    if( state != ap->state )
    {
        sys_log_d( TAG, "audioplayer state: %s -> %s\n", audioplayer_state[ap->state], audioplayer_state[state] );
        ap->state = state;
        ap->playerActivity = audioplayer_state[ap->state];
    }
}


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
    struct alexa_audioplayer* ap = as->ap;

    cJSON* cj_playback_state = cJSON_CreateObject();
    cJSON* cj_header = cJSON_CreateObject();
    cJSON* cj_payload = cJSON_CreateObject();

    cJSON_AddItemToObject( cj_playback_state, "header", cj_header );
    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", audioplayer_event[PLAYBACKSTATE_EVENT]);

    cJSON_AddItemToObject( cj_playback_state, "payload", cj_payload );
    if (ap->token) cJSON_AddStringToObject(cj_payload, "token", ap->token);
    else cJSON_AddStringToObject(cj_payload, "token", "");
    cJSON_AddNumberToObject(cj_payload, "offsetInMilliseconds", ap->offsetInMilliseconds);
    cJSON_AddStringToObject(cj_payload, "playerActivity", ap->playerActivity);

    return cj_playback_state;
}

static void audioplayer_event_header_construct(struct alexa_audioplayer* ap, cJSON* cj_header, enum AUDIOPLAYER_EVENT_ENUM event)
{
    int event_index = (int)event;
    
    cJSON_AddStringToObject( cj_header, "namespace", NAMESPACE);
    cJSON_AddStringToObject( cj_header, "name", audioplayer_event[event_index]);
    cJSON_AddStringToObject(cj_header, "messageId", ap->messageId);
    
    return;
}

static void audioplayer_event_payload_construct(struct alexa_audioplayer* ap, cJSON* cj_payload, enum AUDIOPLAYER_EVENT_ENUM event)
{
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
            cJSON_AddStringToObject( cj_payload, "token", ap->token);
            cJSON_AddNumberToObject(cj_payload, "offsetInMilliseconds", ap->offsetInMilliseconds);
            break;
        case PLAYBACKSTUTTERFINISHED_EVENT:
            cJSON_AddStringToObject(cj_payload, "token", ap->token);
            cJSON_AddNumberToObject(cj_payload, "offsetInMilliseconds", ap->offsetInMilliseconds);
            cJSON_AddNumberToObject(cj_payload, "stutterDurationInMilliseconds", ap->stutterDurationInMilliseconds);
            break;
        case PLAYBACKFAILED_EVENT:
            {
                cJSON* currentPlaybackState = cJSON_CreateObject();
                cJSON* error = cJSON_CreateObject();

                cJSON_AddStringToObject(cj_payload, "token", ap->token);
                cJSON_AddItemToObject(cj_payload, "currentPlaybackState", currentPlaybackState);
                cJSON_AddStringToObject(currentPlaybackState, "token", ap->token);
                cJSON_AddNumberToObject(currentPlaybackState, "offsetInMilliseconds", ap->offsetInMilliseconds);
                cJSON_AddStringToObject(currentPlaybackState, "playerActivity", ap->playerActivity);
                
                cJSON_AddItemToObject( cj_payload, "error", error);
                cJSON_AddStringToObject(currentPlaybackState, "type", ap->type);
                cJSON_AddStringToObject(currentPlaybackState, "message", ap->message);
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

const char* alexa_audioplayer_event_construct(struct alexa_service* as, enum AUDIOPLAYER_EVENT_ENUM event )
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
    audioplayer_event_header_construct( as->ap, cj_header, event );
    audioplayer_event_payload_construct(as->ap, cj_payload, event);
    
    event_json = cJSON_Print(cj_root);
    sys_log_d(TAG, "%s\n", event_json);
    
    cJSON_Delete(cj_root);
    
    return event_json;
}

static void alexa_audioplayer_state_process(struct alexa_service* as, enum AUDIOPLAYER_STATE_ENUM state )
{
    switch( state )
    {
        case AUDIOPLAYER_STATE_IDLE:
        {
            //recive the play directive_play

            audioplayer_set_state( as, AUDIOPLAYER_STATE_PLAYING );
            alexa_audioplayer_event_construct( as, PLAYBACKSTARTED_EVENT );

            break;
        }
        case AUDIOPLAYER_STATE_PLAYING:
            //playing has some issue
            if(TODO)
            {
                audioplayer_set_state( as, AUDIOPLAYER_STATE_STOPPED );
                alexa_audioplayer_event_construct( as, PLAYBACKFAILED_EVENT );
            }
                
            //directive_stop or directive_clearqueue
            if(TODO)
            {
                audioplayer_set_state( as, AUDIOPLAYER_STATE_STOPPED );
                alexa_audioplayer_event_construct( as, PLAYBACKSTOPPED_EVENT );
            }
            
            // alert or timer 
            if(TODO)
            {
                audioplayer_set_state( as, AUDIOPLAYER_STATE_PAUSED );
                alexa_audioplayer_event_construct( as, PLAYBACKPAUSED_EVENT );
            }
        
            //ran out of buffered bytes
            if(TODO)
            {
                audioplayer_set_state( as, AUDIOPLAYER_STATE_BUFFER_UNDERRUN );
                alexa_audioplayer_event_construct( as, PLAYBACKSTUTTERSTARTED_EVENT );
            }

            //play finish
            if(TODO)
            {
                audioplayer_set_state( as, AUDIOPLAYER_STATE_FINISHED );
                alexa_audioplayer_event_construct( as, PLAYBACKFINISHED_EVENT );
            }
            
            break;
        case AUDIOPLAYER_STATE_STOPPED:
        {
            //recive the play directive_play

            audioplayer_set_state( as, AUDIOPLAYER_STATE_PLAYING );
            alexa_audioplayer_event_construct( as, PLAYBACKSTARTED_EVENT );
            
            break;
        }
        case AUDIOPLAYER_STATE_PAUSED:
        {
            //play resume
            if(TODO)
            {
                audioplayer_set_state( as, AUDIOPLAYER_STATE_PLAYING );
                alexa_audioplayer_event_construct( as, PLAYBACKRESUMED_EVENT );
            }

            //directive_clearqueue
            if(TODO)
            {
                audioplayer_set_state( as, AUDIOPLAYER_STATE_STOPPED );
                alexa_audioplayer_event_construct( as, PLAYBACKSTOPPED_EVENT );
            }

            break;
        }
        case AUDIOPLAYER_STATE_BUFFER_UNDERRUN:
        {
            //buffering finished playback resumed
            if(TODO)
            {
                audioplayer_set_state( as, AUDIOPLAYER_STATE_PLAYING );
                alexa_audioplayer_event_construct( as, PLAYBACKSTUTTERFINISHED_EVENT );
            }
            
            //directive_clearqueue
            if(TODO)
            {
                audioplayer_set_state( as, AUDIOPLAYER_STATE_STOPPED );
                alexa_audioplayer_event_construct( as, PLAYBACKSTOPPED_EVENT );
            }

            break;
        }
        case AUDIOPLAYER_STATE_FINISHED:
        {
            //recive the play directive_play

            audioplayer_set_state( as, AUDIOPLAYER_STATE_PLAYING );
            alexa_audioplayer_event_construct( as, PLAYBACKSTARTED_EVENT );
            
            break;
        }
        default:
            break;
    }
}


static int directive_play( struct alexa_service* as, struct alexa_directive_item* item )
{
    struct alexa_audioplayer* ap = as->ap;

    cJSON* cj_payload = item->payload;
    cJSON* cj_playBehavior;
    cJSON* cj_audioItem;
    cJSON* cj_audioItemId;
    cJSON* cj_stream;
    cJSON* cj_url;
    cJSON* cj_streamFormat;
    cJSON* cj_offsetInMilliseconds;
    cJSON* cj_expiryTime;
    cJSON* cj_progressReport;
    cJSON* cj_progressReportDelayInMilliseconds;
    cJSON* cj_progressReportIntervalInMilliseconds;

    cJSON* cj_token;
    cJSON* cj_expectedPreviousToken;

    as = as;

    cj_playBehavior = cJSON_GetObjectItem(cj_payload, "playBehavior");
    if( !cj_playBehavior ) goto err;
    
    cj_audioItem = cJSON_GetObjectItem(cj_payload, "audioItem");
    if( !cj_audioItem ) goto err;
    
    cj_audioItemId = cJSON_GetObjectItem(cj_audioItem, "audioItemId");
    if( !cj_audioItemId ) goto err;

    cj_audioItemId->valuestring;
    
    cj_stream = cJSON_GetObjectItem(cj_audioItem, "stream");
    if( !cj_stream )
    {
        goto err;
    }

    cj_url = cJSON_GetObjectItem(cj_stream, "url");
    cj_streamFormat = cJSON_GetObjectItem(cj_stream, "streamFormat");
    cj_offsetInMilliseconds = cJSON_GetObjectItem(cj_stream, "offsetInMilliseconds");
    cj_expiryTime = cJSON_GetObjectItem(cj_stream, "expiryTime");
    
    cj_progressReport = cJSON_GetObjectItem(cj_stream, "progressReport");
    if( cj_progressReport )
    {
        cj_progressReportDelayInMilliseconds = cJSON_GetObjectItem(cj_progressReport, "progressReportDelayInMilliseconds");
        cj_progressReportIntervalInMilliseconds = cJSON_GetObjectItem(cj_progressReport, "progressReportIntervalInMilliseconds");
    }
    cj_token = cJSON_GetObjectItem(cj_stream, "token");
    cj_expectedPreviousToken = cJSON_GetObjectItem(cj_stream, "expectedPreviousToken");
    
    ALEXA_SAFE_FREE(ap->token);
    ap->token = alexa_strdup(cj_token->valuestring);

    if( !strncmp(cj_url->valuestring, URL_CID, strlen(URL_CID) ) )
    {
        //cid
        
        //has some audio data
    }
    else
    {
        //http or https  

        //has new audio item or has a playlist
    }

    //build new item

    if( !strcmp( cj_playBehavior->valuestring, "REPLACE_ALL" ) )
    {
        //play the new item 

        //clear queue

        //add the new item to the queueu
        
        //send PlaybackStopped Event
    }
    else if( !strcmp( cj_playBehavior->valuestring, "ENQUEUE" ) )
    {
        //add the new item to the queue
        
    }
    else if( !strcmp( cj_playBehavior->valuestring, "REPLACE_ENQUEUED" ) )
    {
        //clear not play item

        //add the new item to the queue
    }

    return 0;

err:
    return -1;
}

static int directive_stop(struct alexa_service* as, struct alexa_directive_item* item)
{
    //send PlaybackStopped Event
    
    as = as;
    item = item;

    return 0;
}


static int directive_clearqueue(struct alexa_service* as, struct alexa_directive_item* item)
{
    cJSON* payload = item->payload;
    cJSON* clearBehavior;
    
    as = as;

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


static int directive_process( struct alexa_service* as, struct alexa_directive_item* item )
{
    cJSON* cj_header = item->header;
    cJSON* cj_name;
    
    as = as;

    cj_name = cJSON_GetObjectItem(cj_header, "name");
    if( !cj_name )
    {
        goto err;
    }
    
    if( !strcmp( cj_name->valuestring, "Play" ) )
    {
        directive_play( as, item );
    }
    else if( !strcmp( cj_name->valuestring, "Stop" ) )
    {
        directive_stop( as, item );
    }
    else if( !strcmp( cj_name->valuestring, "ClearQueue" ) )
    {
        directive_clearqueue( as, item );
    }

    return 0;
    
err:
    return -1;
}

static struct alexa_audioplayer* audioplayer_construct(void)
{
    struct alexa_audioplayer* ap = alexa_new( struct alexa_audioplayer );
    if (ap)
    {
        ALEXA_SAFE_FREE(ap->token);
        ap->state = AUDIOPLAYER_STATE_IDLE;
        ap->playerActivity = audioplayer_state[ap->state];
    }

    return ap;
}


static void audioplayer_destruct(struct alexa_audioplayer* ap)
{
    alexa_delete(ap);
    return ;
}


/*
 *@brief init the alexa audio player
 *@param struct alexa_service* as, the alexa_service object
 *@return 0 success, otherwise fail
 */
int alexa_audioplayer_init(struct alexa_service* as)
{
    as->ap = audioplayer_construct();
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
    audioplayer_destruct(as->ap);
    alexa_directive_unregister(NAMESPACE);
    return 0;
}

/*******************************************************************************
    END OF FILE
*******************************************************************************/
