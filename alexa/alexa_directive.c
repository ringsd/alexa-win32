/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.

    File: alexa_directive.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/13 16:30:40

*******************************************************************************/

#include "alexa_service.h"

#define TAG "alexa_directive"

struct alexa_directive{
    struct list_head        head;
    struct alexa_mutex*     mutex;
};

struct alexa_directive_process_item{
    struct list_head list;
    char name_space[40]; //actual is namespace, the c++ using this for key word
    int(*process)(struct alexa_service* as, struct alexa_directive_item* item);
};

LIST_HEAD(alexa_directive_process_head);

/*
 *@brief register the directive process
 *      register the directive process to the system, be careful this function not thread safe 
 *@param const char* namespace , the directive namespace
 *@param int(*process)( alexa_service* as, struct alexa_directive_item* item) the directive process function
 *@return 
 *   0 success
 *  -1 fail
 */
int alexa_directive_register(const char* name_space, int(*process)( struct alexa_service* as, struct alexa_directive_item* item) )
{
    struct alexa_directive_process_item* process_item = alexa_new(struct alexa_directive_process_item);
    if( process_item )
    {
        strcpy( process_item->name_space, name_space );
        process_item->process = process;
        list_add(&(process_item->list), &alexa_directive_process_head);
        return 0;
    }
    else
    {
        return -1;
    }
}

/*
 *@brief unregister the directive process
 *      unregister the directive process to the system, be careful this function not thread safe 
 *@param const char* namespace
 *@return 
 *   0 success
 *  -1 fail
 */
int alexa_directive_unregister(const char* name_space)
{
    struct alexa_directive_process_item* process_item;
    
    list_for_each_entry_type(process_item, &alexa_directive_process_head, struct alexa_directive_process_item, list)
    {
        if( !strcmp(process_item->name_space, name_space) )
        {
            list_del(&(process_item->list));
            return 0;
        }
    }
    
    return -1;
}

/*
 *@brief
 *
 *@param
 *@return
 */
int alexa_directive_add( struct alexa_service* as, const char* value )
{
    int ret = 0;
    struct alexa_directive* directive = as->directive;
    cJSON* cj_root = NULL;
    cJSON* cj_directive;
    cJSON* cj_header;
    cJSON* cj_payload;

    struct alexa_directive_item* item = (struct alexa_directive_item*)alexa_new( struct alexa_directive_item );
    if( !item )
    {
        sys_log_d( TAG, "memory not enough" );
        goto err;
    }

    //make the directive as a json
    cj_root= cJSON_Parse(value);
    if (!cj_root )
    {
        goto err1;
    }
    item->root = cj_root;

    cj_directive = cJSON_GetObjectItem(cj_root, "directive");
    if (cj_directive)
    {
        item->directive = cj_directive;
        cj_header = cJSON_GetObjectItem(cj_directive, "header");
        if (cj_header)
        {
            item->header = cj_header;
        }

        cj_payload = cJSON_GetObjectItem(cj_directive, "payload");
        if (cj_payload)
        {
            item->payload = cj_payload;
        }
    }
    else
    {
        cj_header = cJSON_GetObjectItem(cj_root, "header");
        if (cj_header)
        {
            item->header = cj_header;
        }

        cj_payload = cJSON_GetObjectItem(cj_root, "payload");
        if (cj_payload)
        {
            item->payload = cj_payload;
        }
    }

    //add the json directive to list
    alexa_mutex_lock(directive->mutex);
    list_add(&(item->list), &directive->head);
    alexa_mutex_unlock(directive->mutex);

    return 0;

err1:
    alexa_delete(item);
err:
    return ret;
}

void alexa_directive_free( struct alexa_directive_item* item )
{
    //free the directive resource
    cJSON_Delete(item->root);
    alexa_free(item);
}

struct alexa_directive_item* alexa_directive_get(struct alexa_directive* directive)
{
    struct alexa_directive_item* item = NULL;
    
    alexa_mutex_lock(directive->mutex);
    if( !list_empty(&directive->head) )
    {
        item = list_first_entry(&directive->head, struct alexa_directive_item, list);
        list_del(&(item->list));
    }
    alexa_mutex_unlock(directive->mutex);

    return item;
}

int alexa_directive_process(struct alexa_service* as, struct alexa_directive_item* item)
{
    int ret = 0;
    cJSON* cj_header = item->header;
    cJSON* cj_namespace;
    struct alexa_directive_process_item* process_item;
    
    cj_namespace = cJSON_GetObjectItem(cj_header, "namespace");
    if( !cj_namespace )
    {
        ret = -1;
        sys_log_e( TAG, "get json namespace error.\n" );
        goto err;
    }

    list_for_each_entry_type(process_item, &alexa_directive_process_head, struct alexa_directive_process_item, list)
    {
        if( !strcmp(process_item->name_space, cj_namespace->valuestring) )
        {
            return process_item->process( as, item );
        }
    }

    return 0;

err:
    return ret;
}

static struct alexa_directive* directive_construct( void )
{
    struct alexa_directive* directive = alexa_new( struct alexa_directive );
    if( directive )
    {
        INIT_LIST_HEAD(&directive->head);
        directive->mutex = alexa_mutex_create();
    }
    return directive;
}

static void directive_destruct( struct alexa_directive* directive )
{
    //clear directive list
    alexa_mutex_lock(directive->mutex);
    for (;;)
    {
        struct alexa_directive_item* item = NULL;
        item = alexa_directive_get(directive);
        if (item)
        {
            alexa_directive_free(item);
        }
        {
            break;
        }
    }
    alexa_mutex_unlock(directive->mutex);
    alexa_mutex_destroy(directive->mutex);
    alexa_delete(directive);
}

int alexa_directive_init( struct alexa_service* as )
{
    as->directive = directive_construct();
    if( as->directive == NULL )
    {
        sys_log_e( TAG, "construct alexa directive fail.\n" );
        return -1;
    }
    return 0;
}

void alexa_directive_done( struct alexa_service* as )
{
    directive_destruct( as->directive );
    return;
}

/*******************************************************************************
    END OF FILE
*******************************************************************************/
