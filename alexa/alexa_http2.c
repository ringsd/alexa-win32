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

#define TODO    0

#define TAG "http2"

#define ALEXA_BASE_URL      "https://avs-alexa-na.amazon.com"
#define ALEXA_API_VERSION   "v20160207" 
#define ALEXA_BOUNDARY      "BOUNDARY1234"

#define ALEXA_HEADER_AUTH       "Authorization:Bearer %s"
#define ALEXA_HEADER_CONTENT    "Content-type = multipart/form-data"

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

#define ALEXA_PING_ACTIVE           (5*60)

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

struct alexa_response{
    char*           data;
    unsigned int    pos;
    unsigned int    size;
};

struct alexa_http2{
    struct alexa_authmng* authmng;
    struct alexa_service* as;
    char*   access_token;
    char*   boundary;
    char*   api_version;
    char*   base_url;
    char*   header_auth;
    char*   header_content;

    void*   curl_handle;
    CURL*   curl;

    int     events_count;
    int     max_events_count;

    struct list_head events_new;
    struct list_head events_remove;
    int boundary_len;
    int dummy_len;
    int audio_header_len;
    int event_header_len;

    struct alexa_response directive_header;
    struct alexa_response directive_body;
};

struct alexa_event_item{
    struct list_head    list;
    CURL*               curl;
    struct curl_slist*  curl_headers;
    char*               content;
    int                 content_len;
    char*               event;
    int                 event_len;
    char*               audio_data;
    int                 audio_data_len;

    struct alexa_response event_header;
    struct alexa_response event_body;
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

    item->event = (char*)event;
    item->event_len = event_len;

    item->audio_data = (char*)audio_data;
    item->audio_data_len = audio_data_len;

#if 0
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
#endif
    //add the event to the events list
    list_add(&(item->list), &http2->events_new);

    return 0;
#if 0
err1:
    alexa_delete( item );
#endif
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
        if (item->curl_headers)
        {
            curl_slist_free_all(item->curl_headers);
        }
        ALEXA_SAFE_FREE(item->event);
        ALEXA_SAFE_FREE(item->audio_data);
        ALEXA_SAFE_FREE(item->content);
        alexa_delete( item );
    }
    return 0;
}

static size_t response_function(void *buffer, size_t size, size_t nmemb, void *userp)
{
    struct alexa_response* response = (struct alexa_response*)userp;
    char *response_data = response->data;
    char *new_response_data;

    if (response->pos + size * nmemb >  response->size)
    {
        new_response_data = (char*)alexa_malloc(response->size + size * nmemb + 1);
        if (new_response_data == NULL) {
            alexa_free(response);
            *(char **)userp = NULL;
            return 0;
        }

        response->data = new_response_data;
        response->size += size * nmemb;

        if (response_data) {
            memcpy(new_response_data, response_data, response->pos);
            alexa_free(response_data);
        }
    }
    else
    {
        new_response_data = response_data;
    }

    memcpy(new_response_data + response->pos, buffer, size * nmemb);
    response->pos += size * nmemb;
    //for string
    new_response_data[response->pos] = 0;

    return size * nmemb;
}

#define DEL_HTTPHEAD_EXPECT  "Expect:"
#define DEL_HTTPHEAD_ACCEPT  "Accept:"

#define CURL_SSL_CART

static void curl_common_set(CURL* hnd)
{
    /* HTTP/2 please */
    curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);

    /* send it verbose for max debuggaility */
    curl_easy_setopt(hnd, CURLOPT_VERBOSE, 1L);
    //curl_easy_setopt(hnd, CURLOPT_DEBUGFUNCTION, my_trace);

    /* we use a self-signed test server, skip verification during debugging */
#ifdef CURL_SSL_CART
    curl_easy_setopt(hnd, CURLOPT_CAINFO, "cacert.pem");
    curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 2L);
#else
    curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 0L);
#endif
    curl_easy_setopt(hnd, CURLOPT_PIPEWAIT, 1);
}



//post events
static CURL* setup_events(struct alexa_http2* http2, struct alexa_event_item* item)
{
    CURL* hnd = NULL;
    struct curl_httppost *post_first = NULL, *post_last = NULL;

    //send the events post request
    struct curl_slist *headers = NULL;

    hnd = curl_easy_init();
    if (hnd == NULL)
    {
        sys_log_e(TAG, "");
        goto err;
    }

    //add to the remove list
    list_add(&(item->list), &http2->events_remove);

    headers = curl_slist_append(headers, DEL_HTTPHEAD_EXPECT);
    headers = curl_slist_append(headers, DEL_HTTPHEAD_ACCEPT);
    headers = curl_slist_append(headers, "Path: /v20160207/events");

    headers = curl_slist_append(headers, http2->header_auth);
    headers = curl_slist_append(headers, http2->header_content);

    headers = curl_slist_append(headers, "Transfer-Encoding: chunked");

    curl_easy_setopt(hnd, CURLOPT_HEADERFUNCTION, response_function);
    curl_easy_setopt(hnd, CURLOPT_HEADERDATA, &item->event_header);
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, response_function);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &item->event_body);

    item->curl = hnd;
    item->curl_headers = headers;

    /* set the events URL */
    curl_easy_setopt(hnd, CURLOPT_URL, ALEXA_BASE_URL"/"ALEXA_API_VERSION"/events" );
    sys_log_i(TAG, ALEXA_BASE_URL"/"ALEXA_API_VERSION"/events\n");

    curl_formadd(&post_first, &post_last,
        CURLFORM_COPYNAME, "metadata",
        CURLFORM_COPYCONTENTS, item->event,
        CURLFORM_CONTENTSLENGTH, item->event_len,
        CURLFORM_CONTENTTYPE, "application/json; charset=UTF-8",
        CURLFORM_END);

    //if (item->audio_data_len)
    {
        curl_formadd(&post_first, &post_last,
            CURLFORM_COPYNAME, "audio",
            CURLFORM_COPYCONTENTS, item->audio_data,
            CURLFORM_CONTENTSLENGTH, item->audio_data_len,
            CURLFORM_CONTENTTYPE, "application/octet-stream", //"audio/L16; rate=16000; channels=1",
            CURLFORM_END);
    }

    curl_common_set(hnd);

    /* set the http header */
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(hnd, CURLOPT_HTTPPOST, post_first);

err:
    return hnd;
}

static void alexa_http2_header_construct(struct alexa_http2* http2)
{
    int len;
    char* header_auth;
    char* header_content;

    //construct header auth
    len = strlen(ALEXA_HEADER_AUTH) + strlen(http2->access_token) + http2->dummy_len;
    header_auth = (char*)alexa_malloc( len );
    if (header_auth == NULL)
    {
        sys_log_e( TAG, "alloc memory for header auth error.\n" );
    }
    else
    {
        sprintf(header_auth, ALEXA_HEADER_AUTH, http2->access_token);
        http2->header_auth = header_auth;
    }

    //construct header auth
    len = strlen(ALEXA_HEADER_CONTENT) + strlen(http2->boundary) + http2->dummy_len;
    header_content = (char*)alexa_malloc(len);
    if (header_content == NULL)
    {
        sys_log_e(TAG, "alloc memory for header content error.\n");
    }
    else
    {
        sprintf(header_content, ALEXA_HEADER_CONTENT, http2->boundary);
        http2->header_content = header_content;
    }

    return;
}

//directive get request
static CURL* curl_directives_construct(struct alexa_http2* http2)
{
    //send the directives get request
    CURL* hnd = NULL;
    struct curl_slist *headers = NULL;

    hnd = curl_easy_init();
    if (hnd == NULL)
    {
        sys_log_e( TAG, "curl_easy_init directive fail.\n" );
        goto err;
    }
    
    headers = curl_slist_append(headers, DEL_HTTPHEAD_EXPECT);
    headers = curl_slist_append(headers, DEL_HTTPHEAD_ACCEPT);
    headers = curl_slist_append(headers, "Path: /"ALEXA_API_VERSION"/directives");
    headers = curl_slist_append(headers, http2->header_auth);

    /* set the http header */
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);

    /* set the directives URL */
    curl_easy_setopt(hnd, CURLOPT_URL, ALEXA_BASE_URL"/"ALEXA_API_VERSION"/directives");
    sys_log_i(TAG, ALEXA_BASE_URL"/"ALEXA_API_VERSION"/directives\n");

    /* write to this data */
    curl_easy_setopt(hnd, CURLOPT_HEADERFUNCTION, response_function);
    curl_easy_setopt(hnd, CURLOPT_HEADERDATA, &http2->directive_header);
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, response_function);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &http2->directive_body);

    curl_common_set(hnd);
    
    return hnd;
    
err:
    return NULL;
}

//every 5 minutes, send the ping get request
static CURL* curl_ping_construct(struct alexa_http2* http2)
{
    CURL* hnd = NULL;
    struct curl_slist *headers = NULL;
    
    hnd = curl_easy_init();
    if (hnd == NULL)
    {
        sys_log_e( TAG, "" );
        goto err;
    }
    
    headers = curl_slist_append(headers, DEL_HTTPHEAD_EXPECT);
    headers = curl_slist_append(headers, DEL_HTTPHEAD_ACCEPT);
    headers = curl_slist_append(headers, "Path: /ping");
    headers = curl_slist_append(headers, http2->header_auth);

    /* set the http header */
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);

    /* set the ping URL */
    curl_easy_setopt(hnd, CURLOPT_URL, ALEXA_BASE_URL"/ping");
    sys_log_i(TAG, ALEXA_BASE_URL"/ping\n");

    curl_common_set(hnd);

    return hnd;
    
err:
    return NULL;
}

static CURL* curl_events_construct(struct alexa_http2* http2)
{
    CURL* events_hnd = NULL;
    
    if (!list_empty(&http2->events_new))
    {
        struct alexa_event_item* item;
        //find the first event, and remove from the new list
        item = list_first_entry(&http2->events_new, struct alexa_event_item, list);
        list_del(&(item->list));

        events_hnd = setup_events(http2, item);
    }
    
    return events_hnd;
}

static int get_header_content(struct alexa_response* header, const char* frontstr, char endchar, char* str, int str_len)
{
    char match_str[32];
    char* substr = strstr(header->data, frontstr);

    if (substr)
    {
        _snprintf(match_str, sizeof(match_str), "%s%%[^%c]", frontstr, endchar);
        _snscanf(substr, str_len - 1, match_str, str);
        alexa_trim(str);
        return 0;
    }
    else
    {
        return -1;
    }
}


enum{
    CONTENT_TYPE_UNKNOWN = 0,
    CONTENT_TYPE_JSON,
    CONTENT_TYPE_BINARY,
};

struct alexa_response_item{
    struct list_head list;
    char* json_data;
    int json_data_len;
    char* binary_data;
    int binary_data_len;
};

static void http2_parse_multipart(struct alexa_http2* http2, struct alexa_response* response, char* boundary)
{
    unsigned int boundary_len;
    char* buffer = response->data;
    unsigned int buffe_len = response->pos;
    char* org_buffer;
    unsigned int org_buffe_len;
    struct alexa_response_item* item = NULL;
    struct list_head response_list;

    INIT_LIST_HEAD(&response_list);

    org_buffer = buffer;
    org_buffe_len = buffe_len;

    boundary_len = strlen(boundary);
    buffer = alexa_strstr(buffer, buffe_len, boundary);
    while (buffer)
    {
        const char* content_type_json = "Content-Type: application/json";
        const char* content_type_binary = "Content-Type: application/octet-stream";
        int content_type_flag = CONTENT_TYPE_UNKNOWN;
        char* content_data;

        //skip the boundary
        buffer += boundary_len;

        buffe_len = org_buffe_len - (buffer - org_buffer);
        buffer = alexa_strstr(buffer, buffe_len, "Content-Type:");
        if (buffer == NULL)
        {
            break;
        }

        if (strncmp(buffer, content_type_json, strlen(content_type_json)) == 0)
        {
            content_type_flag = CONTENT_TYPE_JSON;
        }
        else if (strncmp(buffer, content_type_binary, strlen(content_type_binary)) == 0)
        {
            content_type_flag = CONTENT_TYPE_BINARY;
        }
        else
        {
            sys_log_e(TAG, "Unsupport this content type:%s\n", buffer);
        }

        buffe_len = org_buffe_len - (buffer - org_buffer);
        buffer = alexa_strstr(buffer, buffe_len, "\r\n\r\n");
        if (buffer == NULL || content_type_flag == 0)
        {
            sys_log_e(TAG, (buffer == NULL) ? "Not found new line\n" : "Not support content type\n");
            continue;
        }

        //skip the new line
        buffer += 4;

        content_data = buffer;

        buffe_len = org_buffe_len - (buffer - org_buffer);
        buffer = alexa_strstr(buffer, buffe_len, boundary);
        if (buffer == NULL)
        {
            sys_log_e(TAG, "The bad request\n");
            continue;
        }

        if (content_type_flag == CONTENT_TYPE_JSON)
        {
            item = alexa_new(struct alexa_response_item);
            if (item)
            {
                item->json_data_len = buffer - content_data - 2;
                item->json_data = alexa_malloc(item->json_data_len + 1);
                if (item->json_data)
                {
                    memcpy(item->json_data, content_data, item->json_data_len);
                    item->json_data[item->json_data_len] = 0;
                    list_add(&item->list, &response_list);
                }
                else
                {
                    alexa_delete(item);
                    item = NULL;
                }
            }
        }
        else if (content_type_flag == CONTENT_TYPE_BINARY)
        {
            if (item)
            {
                item->binary_data_len = buffer - content_data - 2;
                item->binary_data = alexa_malloc(item->binary_data_len + 1);
                if (item->binary_data)
                {
                    memcpy(item->binary_data, content_data, item->binary_data_len);
                }
                else
                {
                    item->binary_data_len = 0;
                }
            }
        }
        else
        {
            item = NULL;
        }
    }

    list_for_each_entry_type(item, &response_list, struct alexa_response_item, list)
    {
        alexa_directive_add(http2->as, item->json_data, item->binary_data, item->binary_data_len);
    }

    while (!list_empty(&response_list))
    {
        item = list_first_entry(&response_list, struct alexa_response_item, list);
        if (item)
        {
            list_del(&item->list);
            alexa_delete(item);
        }
        else
        {
            break;
        }
    }

}

static void http2_parse_response(struct alexa_http2* http2, struct alexa_response* header, struct alexa_response* body)
{
    if (header->pos > 0)
    {
        sys_log_i(TAG, "%s\n", header->data);
        if (body->pos > 0)
        {
            char content_type[32];
            //parse the header
            if (get_header_content(header, "content-type:", ';', content_type, sizeof(content_type)) >= 0)
            {
                if (strcmp(content_type, "application/json") == 0)
                {
                    alexa_directive_add(http2->as, body->data, NULL, 0);
                }
                else if (strcmp(content_type, "multipart/related") == 0)
                {
                    char boundary[64];
                    get_header_content(header, "boundary=", ';', boundary, sizeof(boundary));
                    http2_parse_multipart(http2, body, boundary);
                }
            }
        }
        header->pos = 0;
        body->pos = 0;
    }
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
    int still_running; /* keep number of running handles */ 
    struct CURLMsg *m;
    struct alexa_http2* http2 = (struct alexa_http2*)data;
    int ping_hnd_timeout = 0;
    time_t ping_last_time;
    time_t current_time;

    int wakeup_test_timeout = 0;

#if 0
    {
        //test the header content type get
        struct alexa_response header;
        char content_type[32];
        header.data = "HTTP/2 200 \naccess-control-allow-origin: *\nx-amzn-requestid: 12ece9fffe4c89f7-000075ea-00051b4c-3c2d6dcbbace2d4a-bc7fdf1b-1\ncontent-type: multipart/related; boundary=------abcde123; type=application/json";

        get_header_content(&header, "content-type:", ';', content_type, sizeof(content_type));
        get_header_content(&header, "boundary=", ';', content_type, sizeof(content_type));
    }
#endif

    /* init a multi stack */ 
    multi_handle = curl_multi_init();

    directives_hnd = curl_directives_construct(http2);
    if( directives_hnd == NULL )
    {
        sys_log_e( TAG, "construct the directives request error.\n" );
    }
    else
    {
        curl_multi_add_handle(multi_handle, directives_hnd);
    }

    ping_hnd = curl_ping_construct(http2);
    if (ping_hnd == NULL)
    {
        sys_log_e(TAG, "construct the ping request error.\n");
    }
    else
    {
        curl_multi_add_handle(multi_handle, ping_hnd);
        time(&ping_last_time);
    }

#if 1
    {
    const char* event;
    char* audio_data = NULL;
    int audio_read_len = 0;
    int audio_data_len = 16 / 8 * 16000 * 5;
    //record data
    //send Recognize Event to avs
    //change state to BUSY

#if 0
    audio_data = alexa_malloc(audio_data_len);
    if (audio_data)
    {
        struct alexa_record* record = alexa_record_open(1, 16000, 16);
        char* audio_data_tmp = audio_data;
        while (audio_data_len)
        {
            int read_len = 0;
            read_len = alexa_record_read(record, audio_data_tmp, audio_data_len - audio_read_len);
            if (read_len >= 0)
            {
                audio_data_tmp += read_len;
                audio_read_len += read_len;
            }
            else
            {
                break;
            }
        }

        //alexa_record_save_file(record, "record.wav", audio_data, audio_read_len);
        alexa_record_close(record);
    }
    //how to implement the NEAR_FIELD FAR_FIELD profile
#elif 0
#define ALEXA_RECORD_TEST_FILE  "16k.raw"

    FILE* fp = fopen(ALEXA_RECORD_TEST_FILE, "rb");
    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        audio_data_len = ftell(fp);
        audio_data = alexa_malloc(audio_data_len);
        fseek(fp, 0, SEEK_SET);
        audio_read_len = fread(audio_data, 1, audio_data_len, fp);
        fclose(fp);
    }
    sr_generate_request_id(http2->as->sr);
    event = sr_recognizer_event(http2->as);
#else
    event = alexa_system_synchronizestate_construct(http2->as);
#endif

    //event + binary audio stream
    alexa_http2_event_audio_add(http2, event, strlen(event), audio_data, audio_read_len);

    events_hnd = curl_events_construct(http2);
    if (events_hnd == NULL)
    {
    }
    else
    {
        curl_multi_add_handle(multi_handle, events_hnd);
        http2->events_count++;
    }
}
#endif

    curl_multi_setopt(multi_handle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);

    /* We do HTTP/2 so let's stick to one connection per host */ 
    curl_multi_setopt(multi_handle, CURLMOPT_MAX_HOST_CONNECTIONS, 1L);    
    
    /* we start some action by calling perform right away */ 
    curl_multi_perform(multi_handle, &still_running);

    for (;;)
    {
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
                    time(&ping_last_time);
                    ping_hnd = NULL;
                }
                else if( e == directives_hnd )
                {
                    //we get the new directive
                    //add the directive to the list
                    http2_parse_response(http2, &http2->directive_header, &http2->directive_body);
                    directives_hnd = NULL;
                }
                else
                {
                    struct alexa_event_item* item;

                    list_for_each_entry_type(item, &http2->events_remove, struct alexa_event_item, list)
                    {
                        if (!strcmp(item->curl, e))
                        {
                            list_del(&(item->list));

                            http2_parse_response(http2, &item->event_header, &item->event_body);

                            alexa_http2_event_audio_remove(item);
                            break;
                        }
                    }

                    //event has been send
                    http2->events_count --;
                }
                curl_multi_remove_handle(multi_handle, e);
                curl_easy_cleanup(e);
            }
        } while(m);


        time(&current_time);
        //ping hnd request timeout, restart the connect
        if( TODO )
        {
        }
        //ping hnd timeout, set a ping request, and start the ping request timeout
        else if (ping_hnd == NULL && difftime(current_time, ping_last_time) > ALEXA_PING_ACTIVE )
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
            directives_hnd = curl_directives_construct(http2);
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
        
        if ( wakeup_test_timeout < 1 )
        {
            //alexa_speechrecognizer_user_wake_up(http2->as);
        }
        wakeup_test_timeout += 100;

    }

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

        INIT_LIST_HEAD(&http2->events_new);
        INIT_LIST_HEAD(&http2->events_remove);

        alexa_http2_header_construct(http2);
    }
    return http2;
}

static void alexa_http2_destruct(struct alexa_http2* http2)
{
    ALEXA_SAFE_FREE(http2->header_auth);
    ALEXA_SAFE_FREE(http2->header_content);
    ALEXA_SAFE_FREE(http2->access_token);
    alexa_delete(http2);
}

struct alexa_http2* alexa_http2_init(struct alexa_service* as, struct alexa_authmng* authmng)
{
    struct alexa_http2* http2 = alexa_http2_construct(as, authmng);

    as->http2 = http2;
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
