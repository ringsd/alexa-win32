/*******************************************************************************
    Copyright SmartAction Tech. 2017.
    All Rights Reserved.

    File: alexa_main.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/02/08 15:47:48

*******************************************************************************/

#include "alexa_service.h"
#include "alexa_auth.h"
#include "alexa_http2.h"

int alexa_main(void* data)
{
    //authmng init
    struct alexa_authmng* authmng = alexa_authmng_init();
    if( authmng )
    {
        //start alexa_service
        struct alexa_http2* http2 = alexa_http2_init(authmng);
        if( http2 )
        {
            struct alexa_service* service = alexa_service_init(http2);
            if( service ) alexa_service_done(service);
            alexa_http2_done(http2);
        }
        alexa_authmng_done(authmng);
    }
    
    return 0;
}

/*******************************************************************************
    END OF FILE
*******************************************************************************/
