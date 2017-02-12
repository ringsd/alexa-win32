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

typedef struct alexa_directive alexa_directive;

struct alexa_directive_item{
    struct list_head    list;
    cJSON*              root;
    cJSON*              directive;
    cJSON*              payload;
    cJSON*              header;
    int                 data_len;
    const char*         data;
};

int alexa_directive_register(const char* name_space, int(*process)(struct alexa_service* as, struct alexa_directive_item* item));
int alexa_directive_unregister(const char* name_space);
int alexa_directive_process(struct alexa_service* as, struct alexa_directive_item* item);

int alexa_directive_add(struct alexa_service* as, const char* value, const char* data, int data_len);

struct alexa_directive_item* alexa_directive_get(struct alexa_directive* directive);

void alexa_directive_free(struct alexa_directive_item* item);

int alexa_directive_init( struct alexa_service* as );

void alexa_directive_done( struct alexa_service* as );

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
