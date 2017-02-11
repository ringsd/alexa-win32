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

static void getnext(char str[], unsigned int _next[], int size)
{
    _next[0] = 0;
    _next[1] = 0;
    int i = 1, j = 0;
    for (; i<size - 1; i++)
    {
        j = _next[i];
        if (str[i] == str[j])
        {
            _next[i + 1] = j + 1;
        }
        else
        {
            j = _next[j];
            while ((str[i] != str[j]) && (j != 0))
            {
                j = _next[j];
            }
            _next[i + 1] = j;
        }
    }
}

char *alexa_strstr(char *s, unsigned int len, char *p)
{
    unsigned int* next = NULL;
    unsigned int sn = len;
    unsigned int pn = strlen(p);
    unsigned int i = 0, j = 0;
    if (pn == 0)
        return s;

    next = alexa_malloc(pn * sizeof(unsigned int) );
    if (next == NULL)
    {
        return NULL;
    }

    getnext(p, next, pn);
    while (j < sn)
    {
        if (p[i] == s[j])
        {
            i++;
            j++;
        }
        else if (i == 0)
            j++;
        else
            i = next[i];

        if (i == pn)
        {
            alexa_free(next);
            return s + j - i;
        }
    }
    alexa_free(next);
    return NULL;
}


/*******************************************************************************
    END OF FILE
*******************************************************************************/
