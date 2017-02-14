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
    as->directive = alexa_directive_init();
    if (as->directive == NULL) goto err;

    as->event = alexa_event_init();
    if (as->event == NULL) goto err;

    as->sr = alexa_speechrecognizer_init();
    if (as->sr == NULL) goto err;

    as->ss = alexa_speechsynthesizer_init(as);
    if(as->ss == NULL) goto err;

    as->alerts = alexa_alerts_init(as);
    if (as->alerts == NULL) goto err;

    as->ap = alexa_audioplayer_init(as);
    if (as->ap == NULL) goto err;

    as->pc = alexa_pc_init(as);
    if(as->pc == NULL) goto err;

    as->speaker = alexa_speaker_init(as);
    if (as->speaker == NULL) goto err;

    as->system = alexa_system_init( as );
    if (as->system == NULL) goto err;

    return as;

err:
    alexa_service_done(as);
    return NULL;
}

void alexa_service_process(struct alexa_service* as, struct alexa_http2* http2)
{
    http2 = http2;
    alexa_speechrecognizer_process(as);
}

struct alexa_event* alexa_service_get_event(struct alexa_service* as)
{
    return as->event;
}


struct alexa_directive* alexa_service_get_direcitve(struct alexa_service* as)
{
    return as->directive;
}

void alexa_service_done( struct alexa_service* as )
{
    alexa_system_done(as->system);
    alexa_speaker_done(as->speaker);
    alexa_pc_done(as->pc);
    alexa_audioplayer_done(as->ap);
    alexa_alerts_done(as->alerts);
    alexa_speechsynthesizer_done(as->ss);
    alexa_speechrecognizer_done(as->sr);
    alexa_directive_done(as->directive);
    alexa_event_done(as->event);
    alexa_delete(as);

    return ;
}

/*******************************************************************************
    END OF FILE
*******************************************************************************/
