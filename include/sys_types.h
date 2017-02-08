/*******************************************************************************
    Copyright Ringsd. 2016.
    All Rights Reserved.
    
    File: sys_types.h

    Description:

    TIME LIST:
    CREATE By Ringsd   2016/06/08 11:35:59

*******************************************************************************/

#ifndef _sys_types_h_
#define _sys_types_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member)  (type *)( (char *)ptr - offsetof(type,member) )
#else
//#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({            \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})
    
//#define typeof __typeof_
#endif

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
