/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.

	File: alexa_directive.c

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/01/13 16:30:40

*******************************************************************************/

LIST_HEAD(alexa_directive_head);

struct alexa_directive_process_item{
    struct list_head list;
    const char* name_space; //actual is namespace, the c++ using this for key word
    int (*process)( alexa_service* as, cJSON* root);
};

LIST_HEAD(alexa_directive_process_head);

int alexa_directive_register(const char* name_space, int(*process)( alexa_service* as, cJSON* root) )
{
    struct alexa_directive_process_item* item =  alexa_new( alexa_directive_process_item );
    if( item )
    {
        strcpy( item->name_space, name_space );
        item->process = process;
        list_add(&(item->list), &alexa_directive_process_head);
        return -1;
    }
    else
    {
        return 0;
    }
}

int alexa_directive_unregister(const char* name_space)
{
    struct alexa_directive_process_item* item;
    
    list_for_each(item, &alexa_directive_process_head)
    {
        if( !strcmp(item->name_space, name_space) )
        {
            list_del(item);
            return 0;
        }
    }
    
    return -1;
}

/*
 *@brief
 *
 *
 *
 *@param
 *@return
 */
int alexa_directive_add( struct alexa_service* as, const char* value )
{
    cJSON* root = NULL;
    struct alexa_directive_item* directive = (struct alexa_directive_item*)alexa_new( struct alexa_directive_item );
    if( !directive )
    {
        sys_log_d( TAG, "memory not enough" );
        goto err;
    }

    //make the directive as a json
    root = cJSON_Parse(value);
    if( !root )
    {
    }
    directive->cj_directive = root;
    
    //add the json directive to list
    //need lock
    list_add(&(directive->list), &alexa_directive_head);
    //need unlock

err2:
err1:
err:
    return ret;
}

int alexa_directive_free( struct alexa_service* as, struct alexa_directive_item* directive )
{
    //free the directive resource
    if(directive->data)
    {
        alexa_free(directive->data);
    }
    cJSON_Delete(directive->cj_directive);
    alexa_free(directive);
}

struct alexa_directive_item* alexa_directive_get( struct alexa_service* as )
{
    struct alexa_directive_item* directive = NULL;
    
    //lock
    if( !list_empty(&alexa_directive_head) )
    {
        directive = list_first_entry(&alexa_directive_head, struct alexa_directive_item, list);
        list_del(&(directive->list));
    }
    //unlock
    
    return directive;
}

int alexa_directive_process(alexa_service* as, const char* value)
{
    const char* name_space;
    cJSON* root = cJSON_Parse(value);

    if( !root )
    {
        //json parse error
        ret = -1;
        alexa_log_e( TAG, "parse json error.\n" );
        goto err;
    }

    directive = cJSON_GetObjectItem(root, "directive");
    if( !directive )
    {
        ret = -1;
        alexa_log_e( TAG, "get json directive error.\n" );
        goto err1;
    }

    header = cJSON_GetObjectItem(directive, "header");
    if( !header )
    {
        ret = -1;
        alexa_log_e( TAG, "get json header error.\n" );
        goto err1;
    }
    
    payload = cJSON_GetObjectItem(directive, "payload");
    if( !payload )
    {
        ret = -1;
        alexa_log_e( TAG, "get json payload error.\n" );
        goto err1;
    }
    
    as->directive = directive;
    as->header = header;
    as->payload = payload;
    
    name_space = cJSON_GetObjectItem(header, "namespace");
    if( !name_space )
    {
        ret = -1;
        alexa_log_e( TAG, "get json namespace error.\n" );
        goto err2;
    }
    
    list_for_each(item, &alexa_directive_head)
    {
        if( !strcmp(item->name_space, name_space->valuestring) )
        {
            if( item->process( as, root ) )
            {
                //
            }
            else
            {
                //
            }
            break;
        }
    }
    
err2:
    as->directive = NULL;
    as->header = NULL;
    as->payload = NULL;
err1:
    cJSON_Delete(root);
err:
    return ret;
}




/*******************************************************************************
	END OF FILE
*******************************************************************************/
