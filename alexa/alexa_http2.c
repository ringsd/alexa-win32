/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.
	
	File: alexa_http2.c

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/01/18 16:38:46

    https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/docs/managing-an-http-2-connection
    
*******************************************************************************/




/*
 * establishing HTTP/2 connection
 *   creating an HTTP/2 Connection 
 *     1. GET  /{{API version}}/directives
 *
 */

 
#define HTTP2_BASE_URL      "https://avs-alexa-na.amazon.com"
#define HTTP2_API_VERSION   "v20160207" 

struct alexa_http2{
    char*   accesss_token;
    CURL*   curl;
};

alexa_http2_event()
{
    
}

alexa_http2_downchannel()
{
    
}

alexa_http2_ping()
{
    
}

alexa_http2_conn_establishing(struct alexa_http2* https)
{
    https
}


alexa_http2_event_send( const char*  )
{
    
}



int alexa_http2_init( struct alexa_service* as )
{
    //
    
}


void alexa_http2_done( struct alexa_service* as )
{
    
}

/*******************************************************************************
	END OF FILE
*******************************************************************************/
