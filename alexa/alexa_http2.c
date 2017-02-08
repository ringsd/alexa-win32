/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_http2.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/18 16:38:46

    https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/docs/managing-an-http-2-connection
    
*******************************************************************************/

#include <string.h>
#include "curl/curl.h"
#include "list.h"
#include "sys_log.h"
#include "alexa_service.h"
#include "alexa_platform.h"
#include "alexa_base.h"
#include "alexa_auth.h"

#define TODO    1

#define TAG "http2"

#define ALEXA_BASE_URL      "https://avs-alexa-na.amazon.com"
#define ALEXA_API_VERSION   "v20160207" 
#define ALEXA_BOUNDARY      "BOUNDARY1234"

#define ALEXA_EVENT_JSON_HEADER                                                \
"Content-Disposition: form-data; name=\"metadata\""\
"\r\n"\
"Content-Type: application/json; charset=UTF-8"\
"\r\n"

#define ALEXA_BINARY_AUDIO_HEADER                                              \
"Content-Disposition: form-data; name=\"audio\""\
"\r\n"\
"Content-Type: application/octet-stream"\
"\r\n"

#define ALEXA_MAX_EVENTS_COUNT    5
#define HTTP2_MAX_STREAMS   10

/*
 * establishing HTTP/2 connection
 *   creating an HTTP/2 Connection 
 *     1. Directives GET  base_url/{{API version}}/directives
 *        :method = GET
 *        :scheme = https
 *        :path = /{{API version}}/directives
 *        authorization = Bearer {{YOUR_ACCESS_TOKEN}}
 *
 *     2. Events     POST base_url/{{API version}}/events
 *        :method = POST
 *        :scheme = https
 *        :path = /{{API version}}/events
 *        authorization = Bearer {{YOUR_ACCESS_TOKEN}}
 *        content-type = multipart/form-data; boundary={{BOUNDARY_TERM_HERE}}
 *
 *        --{{BOUNDARY_TERM_HERE}}
 *        Content-Disposition: form-data; name="metadata"
 *        Content-Type: application/json; charset=UTF-8
 *
 *        {
 *            "context": [
 *                {{Alerts.AlertsState}},
 *                {{AudioPlayer.PlaybackState}},
 *                {{Speaker.VolumeState}},
 *                {{SpeechSynthesizer.SpeechState}}
 *            ],
 *            "event": {
 *                "header": {
 *                "namespace": "System",
 *                "name": "SynchronizeState",
 *                "messageId": "{{STRING}}"
 *                },
 *                "payload": {
 *                }
 *            }
 *        }
 *
 *        --{{BOUNDARY_TERM_HERE}}--
 *
 *     3. PING       GET  base_url/ping every 5 minutes
 *        :method = GET
 *        :scheme = https
 *        :path = /ping
 *        authorization = Bearer {{YOUR_ACCESS_TOKEN}}
 */


struct alexa_http2{
    struct alexa_authmng* authmng;
    struct alexa_service* as;
    char*   access_token;
    char*   boundary;
    char*   api_version;
    char*   base_url;
    void*   curl_handle;
    CURL*   curl;

    int     events_count;
    int     max_events_count;

    struct list_head events_head;
    int boundary_len;
    int dummy_len;
    int audio_header_len;
    int event_header_len;
};

struct alexa_event_item{
    struct list_head    list;
    char*               content;
    int                 content_len;
};

int alexa_http2_event_audio_add(struct alexa_http2* http2, const char* event, int event_len, const char* audio_data, int audio_data_len)
{
    int content_len = 0;
    char* content;
    char* content_pos;
    struct alexa_event_item* item;
    
    item = alexa_new(struct alexa_event_item);
    if( !item )
    {
        sys_log_e( TAG, "don't have enough mem\n" );
        goto err;
    }

    content_len += http2->boundary_len + http2->dummy_len;
    content_len += http2->event_header_len;
    content_len += event_len;
    content_len += http2->dummy_len;
    if( audio_data )
    {
        content_len += http2->boundary_len + http2->dummy_len;
        content_len += http2->audio_header_len;
        content_len += audio_data_len;
    }
    content_len += http2->boundary_len + http2->dummy_len;

    content = (char*)alexa_malloc( content_len );
    if( !content )
    {
        sys_log_e( TAG, "don't have enough mem\n" );
        goto err1;
    }
    item->content = content;
    item->content_len = content_len;
    
    content_pos = content;
    
    //first boundary
    content_pos += sprintf( content_pos, "--%s\r\n", http2->boundary );
    
    //event json header
    content_pos += sprintf( content_pos, ALEXA_EVENT_JSON_HEADER );
    //event json
    content_pos += sprintf( content_pos, event );
    content_pos += sprintf( content_pos, "\r\n" );
    
    if( audio_data )
    {
        //boundary
        content_pos += sprintf( content_pos, "--%s\r\n", http2->boundary );
        //binary audio header
        content_pos += sprintf( content_pos, ALEXA_BINARY_AUDIO_HEADER );
        //binary audio data
        memcpy( content_pos, audio_data, audio_data_len );
        content_pos += audio_data_len;
    }
    
    //last boundary
    content_pos += sprintf( content_pos, "--%s--", http2->boundary );

    //add the event to the events list
    list_add(&(item->list), &http2->events_head);

    return 0;
err1:
    alexa_delete( item );
err:
    return -1;
}

int alexa_http2_event_add( struct alexa_http2* http2, const char* event, int event_len )
{
    return alexa_http2_event_audio_add(http2, event, event_len, NULL, 0);
}

int alexa_http2_event_audio_remove( struct alexa_event_item* item )
{
    if( item )
    {
        if( item->content )
        {
            alexa_free( item->content );
        }
        alexa_delete( item );
    }
    return 0;
}

static size_t directives_response(void *buffer, size_t size, size_t nmemb, void *userp)
{
    int response_len = 0;
    char *response = *(char **)userp;
    char *new_response;

    if (response)
        response_len = strlen(response);

    new_response = (char*)calloc(response_len + size * nmemb + 1, 1);
    if (new_response == NULL) {
        free(response);
        *(char **)userp = NULL;
        return 0;
    }

    if (response) {
        memcpy(new_response, response, response_len);
        free(response);
    }
    memcpy(new_response + response_len, buffer, size * nmemb);

    *(char **)userp = new_response;

    return size * nmemb;
}


//directive get request
static void setup_directives(CURL *hnd, struct curl_slist *headers, char** response)
{
    /* set the http header */
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);

    /* set the directives URL */ 
    curl_easy_setopt(hnd, CURLOPT_URL, ALEXA_BASE_URL"/"ALEXA_API_VERSION"/directives" );

    /* send it verbose for max debuggaility */ 
    //curl_easy_setopt(hnd, CURLOPT_VERBOSE, 1L);
    //curl_easy_setopt(hnd, CURLOPT_DEBUGFUNCTION, my_trace);

    /* we use a self-signed test server, skip verification during debugging */ 
    curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 0L);
    
    /* write to this data */ 
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, directives_response);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, response);

    /* HTTP/2 please */ 
    curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);

#if (CURLPIPE_MULTIPLEX > 0)
    /* wait for pipe connection to confirm */ 
    curl_easy_setopt(hnd, CURLOPT_PIPEWAIT, 1L);
#endif
}

//ping request get
static void setup_ping(CURL *hnd, struct curl_slist *headers)
{
    /* set the http header */
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
    
    /* set the ping URL */ 
    curl_easy_setopt(hnd, CURLOPT_URL, ALEXA_BASE_URL"/ping" );

    /* send it verbose for max debuggaility */ 
    //curl_easy_setopt(hnd, CURLOPT_VERBOSE, 1L);
    //curl_easy_setopt(hnd, CURLOPT_DEBUGFUNCTION, my_trace);

    /* HTTP/2 please */ 
    curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);

    /* we use a self-signed test server, skip verification during debugging */ 
    curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 0L);

    #if (CURLPIPE_MULTIPLEX > 0)
    /* wait for pipe connection to confirm */ 
    curl_easy_setopt(hnd, CURLOPT_PIPEWAIT, 1L);
    #endif
}

//post events
static void setup_events(CURL *hnd, struct curl_slist *headers, const char *fields, int len)
{
    /* set the events URL */ 
    curl_easy_setopt(hnd, CURLOPT_URL, ALEXA_BASE_URL"/"ALEXA_API_VERSION"/events" );

    /* send it verbose for max debuggaility */ 
    //curl_easy_setopt(hnd, CURLOPT_VERBOSE, 1L);
    //curl_easy_setopt(hnd, CURLOPT_DEBUGFUNCTION, my_trace);

    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, fields);
    /* set CURLOPT_POSTFIELDSIZE, or will send poststring as string */
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDSIZE, len);
    
    /* HTTP/2 please */ 
    curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);

    /* we use a self-signed test server, skip verification during debugging */ 
    curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 0L);

#if (CURLPIPE_MULTIPLEX > 0)
    /* wait for pipe connection to confirm */ 
    curl_easy_setopt(hnd, CURLOPT_PIPEWAIT, 1L);
#endif
 
}


static CURL* curl_directives_construct(struct alexa_http2* http2, char** response)
{
    //send the directives get request
    CURL* directives_hnd = NULL;
    struct curl_slist *headers = NULL;
    char header_auth[128];

    directives_hnd = curl_easy_init();
    if( directives_hnd == NULL )
    {
        sys_log_e( TAG, "" );
        goto err;
    }
    
    sprintf( header_auth, "authorization = Bearer %s", http2->access_token );
    headers = curl_slist_append(headers, header_auth);
    setup_directives(directives_hnd, headers, response);
    
    return directives_hnd;
    
err:
    return NULL;
}

//every 5 minutes, send the ping get request
static CURL* curl_ping_construct(struct alexa_http2* http2)
{
    CURL* ping_hnd = NULL;
    struct curl_slist *headers = NULL;
    char header_auth[128];
    
    ping_hnd = curl_easy_init();
    if( ping_hnd == NULL )
    {
        sys_log_e( TAG, "" );
        goto err;
    }
    
    sprintf( header_auth, "authorization = Bearer %s", http2->access_token );
    headers = curl_slist_append(headers, header_auth);
    setup_ping(ping_hnd, headers);
    
    return ping_hnd;
    
err:
    return NULL;
}


static CURL* curl_events_construct(struct alexa_http2* http2)
{
    CURL* events_hnd = NULL;
    
    if( !list_empty(&http2->events_head) )
    {
        //send the events post request
        struct curl_slist *headers = NULL;
        char header_auth[128];
        char header_content[128];
        struct alexa_event_item* item;

        events_hnd = curl_easy_init();
        if( events_hnd == NULL )
        {
            sys_log_e( TAG, "" );
            goto err;
        }
        
        item = list_first_entry(&http2->events_head, struct alexa_event_item, list);
        list_del(&(item->list));

        sprintf( header_auth, "authorization = Bearer %s", http2->access_token );
        sprintf( header_content, "content-type = multipart/form-data; boundary=%s", http2->boundary );

        headers = curl_slist_append(headers, header_auth);
        headers = curl_slist_append(headers, header_content);

        setup_events(events_hnd, headers, item->content, item->content_len);

        alexa_http2_event_audio_remove( item );
    }
    
    return events_hnd;
    
err:
    return NULL;
}

#ifdef WIN32
static void alexa_http2_process( void* data )
#else
static void* alexa_http2_process(void* data)
#endif
{
    CURL*    ping_hnd = NULL;
    CURL*    directives_hnd = NULL;
    CURL*    events_hnd = NULL;
    CURLM*   multi_handle;
    char*    directives_response = NULL;
    int still_running; /* keep number of running handles */ 
    struct CURLMsg *m;
    struct alexa_http2* http2 = (struct alexa_http2*)data;
    int ping_hnd_timeout = 0;

    /* init a multi stack */ 
    multi_handle = curl_multi_init();

    directives_hnd = curl_directives_construct(http2, &directives_response);
    if( directives_hnd == NULL )
    {
        sys_log_e( TAG, "construct the directives request error.\n" );
    }
    else
    {
        curl_multi_add_handle(multi_handle, directives_hnd);
    }

    //curl_multi_setopt(multi_handle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);

    /* We do HTTP/2 so let's stick to one connection per host */ 
    curl_multi_setopt(multi_handle, CURLMOPT_MAX_HOST_CONNECTIONS, 1L);    
    
    /* we start some action by calling perform right away */ 
    curl_multi_perform(multi_handle, &still_running);

    do {
        struct timeval timeout;
        int rc; /* select() return code */ 
        CURLMcode mc; /* curl_multi_fdset() return code */ 

        fd_set fdread;
        fd_set fdwrite;
        fd_set fdexcep;
        int maxfd = -1;

        long curl_timeo = -1;

        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);

        /* set a suitable timeout to play around with */ 
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        curl_multi_timeout(multi_handle, &curl_timeo);
        if(curl_timeo >= 0) {
            timeout.tv_sec = curl_timeo / 1000;
            if(timeout.tv_sec > 1)
                timeout.tv_sec = 1;
            else
                timeout.tv_usec = (curl_timeo % 1000) * 1000;
        }

        /* get file descriptors from the transfers */ 
        mc = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

        if(mc != CURLM_OK) {
            fprintf(stderr, "curl_multi_fdset() failed, code %d.\n", mc);
            break;
        }

        /* On success the value of maxfd is guaranteed to be >= -1. We call
           select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
           no fds ready yet so we call select(0, ...) --or Sleep() on Windows--
           to sleep 100ms, which is the minimum suggested value in the
           curl_multi_fdset() doc. */ 

        if(maxfd == -1) {
        #ifdef _WIN32
            Sleep(100);
            rc = 0;
        #else
            /* Portable sleep for platforms other than Windows. */ 
            struct timeval wait = { 0, 100 * 1000 }; /* 100ms */ 
            rc = select(0, NULL, NULL, NULL, &wait);
        #endif
        }
        else {
            /* Note that on some platforms 'timeout' may be modified by select().
               If you need access to the original value save a copy beforehand. */ 
            rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
        }

        switch(rc) {
            case -1:
                /* select error */ 
                break;
            case 0:
            default:
                /* timeout or readable/writable sockets */ 
                curl_multi_perform(multi_handle, &still_running);
                break;
        }

        /*
         * A little caution when doing server push is that libcurl itself has
         * created and added one or more easy handles but we need to clean them up
         * when we are done.
         */ 

        do {
            int msgq = 0;;
            m = curl_multi_info_read(multi_handle, &msgq);
            if(m && (m->msg == CURLMSG_DONE)) {
                CURL *e = m->easy_handle;

                //if is ping request
                if( e == ping_hnd )
                {
                    //start the ping timeout 
                    ping_hnd = NULL;
                }
                else if( e == directives_hnd )
                {
                    //we get the new directive
                    //add the directive to the list
                    alexa_directive_add(http2->as, directives_response);

                    //wake up the directive process

                    directives_hnd = NULL;
                    directives_response = NULL;
                }
                else
                {
                    //event has been send
                    http2->events_count --;
                }
                curl_multi_remove_handle(multi_handle, e);
                curl_easy_cleanup(e);
            }
        } while(m);

        //ping hnd request timeout, restart the connect
        if( TODO )
        {
        }
        //ping hnd timeout, set a ping request, and start the ping request timeout
        else if (TODO && ping_hnd_timeout)
        {
            //every 5 minutes, send the ping get request
            ping_hnd = curl_ping_construct(http2);
            if( ping_hnd == NULL )
            {
                sys_log_e( TAG, "construct the ping request error.\n" );
            }
            else
            {
                curl_multi_add_handle(multi_handle, ping_hnd);
            }
        }
        
        //has directive done, add the directive get request
        if( directives_hnd == NULL )
        {
            directives_hnd = curl_directives_construct(http2, &directives_response);
            if( directives_hnd == NULL )
            {
                sys_log_e( TAG, "construct the directives request error.\n" );
            }
            else
            {
                curl_multi_add_handle(multi_handle, directives_hnd);
            }
        }
        
        //has slot for events to send
        while(http2->events_count < http2->max_events_count)
        {
            //every 5 minutes, send the ping get request
            events_hnd = curl_events_construct(http2);
            if( events_hnd == NULL )
            {
                break;
            }
            else
            {
                curl_multi_add_handle(multi_handle, events_hnd);
            }
            http2->events_count ++;
        }
        
    } while(1); /* as long as we have transfers going */ 

    //loop ??
    do {
        int msgq = 0;;
        m = curl_multi_info_read(multi_handle, &msgq);
        if( m ) {
            CURL *e = m->easy_handle;
            curl_multi_remove_handle(multi_handle, e);
            curl_easy_cleanup(e);
        }
    } while(m);
    
    curl_multi_cleanup(multi_handle);
}

static struct alexa_http2* alexa_http2_construct(struct alexa_service* as, struct alexa_authmng* authmng)
{
    struct alexa_http2* http2 = alexa_new( struct alexa_http2 );
    if (http2)
    {
        http2->as = as;
        http2->authmng = authmng;
        http2->access_token = alexa_strdup(alexa_authmng_get_access_token(authmng));
        http2->base_url = ALEXA_BASE_URL;
        http2->api_version = ALEXA_API_VERSION;
        http2->boundary = ALEXA_BOUNDARY;
        http2->boundary_len = strlen(ALEXA_BOUNDARY);
        http2->dummy_len = 4;
        http2->audio_header_len = strlen(ALEXA_BINARY_AUDIO_HEADER);
        http2->event_header_len = strlen(ALEXA_EVENT_JSON_HEADER);

        http2->max_events_count = ALEXA_MAX_EVENTS_COUNT;

        INIT_LIST_HEAD(&http2->events_head);

    }
    return http2;
}

static void alexa_http2_destruct(struct alexa_http2* http2)
{
    ALEXA_SAFE_FREE(http2->access_token);
    alexa_delete(http2);
}

struct alexa_http2* alexa_http2_init(struct alexa_service* as, struct alexa_authmng* authmng)
{
    struct alexa_http2* http2 = alexa_http2_construct(as, authmng);

    alexa_begin_thread(alexa_http2_process, (void*)http2, NULL, 0);

    return http2;
}

void alexa_http2_done(struct alexa_http2* http2)
{
    alexa_http2_destruct(http2);
}


/*******************************************************************************
    END OF FILE
*******************************************************************************/
