/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.

    File: alexa_service.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/16 17:14:18

*******************************************************************************/

#include "alexa_service.h"
#include "alexa_auth.h"


#define TODO 1

void alexa_service_wakeup(void)
{
    //user wake up the alexa service
}

struct alexa_service* alexa_service_init(void)
{
    struct alexa_service* as = alexa_new(struct alexa_service);
    if (as == NULL)
    {
        goto err;
    }

    //init the directive center
    if( alexa_directive_init( as ) < 0 ) goto err1;
    if( alexa_speechrecognizer_init(as) < 0 ) goto err2;
    if( alexa_speechsynthesizer_init(as) < 0 ) goto err3;
    if( alexa_alerts_init(as) < 0 ) goto err4;
    if (alexa_audioplayer_init(as) < 0) goto err5;
    if( alexa_pc_init( as ) < 0 ) goto err6;
    if( alexa_speaker_init( as ) < 0 ) goto err7;
    if( alexa_system_init( as ) < 0 ) goto err8;

    return as;

err8:
    alexa_speaker_done( as );
err7:
    alexa_pc_done( as );
err6:
    alexa_audioplayer_done( as );
err5:
    alexa_alerts_done( as );
err4:
    alexa_speechsynthesizer_done( as );
err3:
    alexa_speechrecognizer_done( as );
err2:
    alexa_directive_done( as );
err1:
    alexa_delete( as );
err:
    return NULL;
}

void alexa_service_process(struct alexa_service* as, struct alexa_http2* http2)
{
    as->http2 = http2;
    alexa_speechrecognizer_process(as);
}

void alexa_service_done( struct alexa_service* as )
{
    alexa_speaker_done(as);
    alexa_pc_done(as);
    alexa_audioplayer_done(as);
    alexa_alerts_done(as);
    alexa_speechsynthesizer_done(as);
    alexa_speechrecognizer_done(as);
    alexa_directive_done(as);
    alexa_delete(as);

    return ;
}

/*******************************************************************************
    END OF FILE
*******************************************************************************/
