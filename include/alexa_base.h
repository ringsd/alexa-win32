/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_base.h

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/2/07 18:06:44

*******************************************************************************/

#ifndef _alexa_base_h_
#define _alexa_base_h_

#ifdef __cplusplus
extern "C" {
#endif

char* alexa_strdup(const char* str);

char* alexa_trim(char* str);

char *alexa_strstr(char *s, unsigned int len, char *p);


#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
