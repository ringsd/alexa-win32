/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_base.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/02/07 18:06:12

*******************************************************************************/

#include <string.h>
#include "alexa_platform.h"

char* alexa_strdup(const char* str)
{
    int len = strlen(str);
    char* actual_str = (char*)alexa_malloc(len + 1);
    if (actual_str)
    {
        strcpy(actual_str, str);
    }
    return actual_str;
}


/*******************************************************************************
    END OF FILE
*******************************************************************************/
