/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_platform.h

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/1/17 09:41:30

*******************************************************************************/

#ifndef _alexa_platform_h_
#define _alexa_platform_h_

#ifdef __cplusplus
extern "C" {
#endif

#define alexa_new(x)        (x*)alexa_malloc(sizeof(x))
#define alexa_delete(x)     alexa_free((void*)x)

typedef struct alexa_mutex	alexa_mutex;

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
