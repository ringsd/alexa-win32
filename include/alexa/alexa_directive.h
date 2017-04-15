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
    char*               data;
    int                 data_len;
};

typedef int(*directive_process_func)(void* as, struct alexa_directive_item* item);

int alexa_directive_register(const char* name_space, int(*process)(void* as, struct alexa_directive_item* item), void* data);
int alexa_directive_unregister(const char* name_space);
int alexa_directive_process(struct alexa_directive* directive, struct alexa_directive_item* item);

int alexa_directive_add(struct alexa_directive* directive, const char* value, char* data, int data_len);

struct alexa_directive_item* alexa_directive_get(struct alexa_directive* directive);

void alexa_directive_free(struct alexa_directive_item* item);

struct alexa_directive* alexa_directive_init( void );

void alexa_directive_done(struct alexa_directive* directive);

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
