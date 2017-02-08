/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_context.h

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/1/17 14:56:40

*******************************************************************************/

#ifndef _alexa_context_h_
#define _alexa_context_h_

#ifdef __cplusplus
extern "C" {
#endif


/*
 *@brief construct the alexa context
 *@param struct alexa_service* as, the alexa_service object
 *@return alexa_context json object
 */
cJSON* alexa_context_get_state(struct alexa_service* as);

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
