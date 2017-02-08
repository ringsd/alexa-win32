/*******************************************************************************
    Copyright Ringsd. 2017.
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

#ifdef ALEXA_UNIT_TEST
    {
        //alexa_authmng_test();
    }
#endif

    struct alexa_authmng* authmng = alexa_authmng_init();
    if( authmng )
    {
        //start alexa_service
        struct alexa_service* service = alexa_service_init();
        if (service)
        {
            struct alexa_http2* http2 = alexa_http2_init(service, authmng);
            alexa_service_process(service, http2);
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
