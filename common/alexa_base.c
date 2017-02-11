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

char* alexa_trim(char* str)
{
    char *str_org, *str_last, *str_cur;
    if (str == NULL)
        return NULL;
    str_org = str;
    str_cur = str;
    //found the front space and \t
    for (; *str_cur == ' ' || *str_cur == '\t'; ++str_cur);

    for (str_last = str_cur; *str_cur != '\0'; )
    {
        if (*str_cur != ' ' && *str_cur != '\t')
        {
            str_last = str;
        }
        *str++ = *str_cur++;
    }
    *++str_last = '\0';
    return str_org;
}


/*******************************************************************************
    END OF FILE
*******************************************************************************/
