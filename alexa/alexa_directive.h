/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_directive.h

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/13 17:03:29

*******************************************************************************/

#ifndef _alexa_directive_h_
#define _alexa_directive_h_

#ifdef __cplusplus
extern "C" {
#endif

struct alexa_directive_item{
    struct list_head    list;
    cJSON*              directive;
    cJSON*              payload;
    cJSON*              header;
    int                 data_len;
    const char*         data;
};


int alexa_directive_register(const char* name_space, int(*process)( struct alexa_service* as, cJSON* root) );
int alexa_directive_unregister(const char* name_space);
int alexa_directive_process(struct alexa_service* as, const char* value);


int alexa_directive_init( struct alexa_service* as );

void alexa_directive_done( struct alexa_service* as );

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
