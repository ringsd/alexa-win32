/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.

    File: sys_log.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/04 11:51:25

*******************************************************************************/

#include    <stdarg.h>
#include    <string.h>
#include    <stdio.h>

#include    "sys_log.h"

#define DEBUG_LEVEL     4

static int sys_log_level = 1;

struct log_level_item{
    const char* name;
    int         level;
};

static struct log_level_item log_level_map[] = {
    {"debug",   4},
    {"info",    3},
    {"warning", 2},
    {"error",   1},
    {NULL,      0},
};

void sys_log_set_levelname(const char* name)
{
    int i = 0;
    while(log_level_map[i].name)
    {
        if( !strcmp(name, log_level_map[i].name) )
        {
            sys_log_level = log_level_map[i].level;
            break;
        }
        i++;
    }
}

void sys_log_set_level(int level)
{
    sys_log_level = level;
}

int sys_log_get_level(void)
{
    return sys_log_level;
}

int sys_log(int level, const char* tag, const char* fmt, ...)
{
    char    buf[1024];
    va_list    arg;
    int        len;

    level = level;

    va_start(arg, fmt);
    vsnprintf(buf, sizeof(buf), fmt, arg);
    va_end(arg);

    len = strlen(buf);

    printf("("SYS_LOG_PROFIX"): %s", tag, buf);

    return len;
}

int sys_log_d(const char* tag, const char* fmt, ...)
{
#if DEBUG_LEVEL > 3
    if( sys_log_level > 3 )
    {
        char    buf[1024];
        va_list    arg;
        int        len;

        va_start(arg, fmt);
        vsnprintf(buf, sizeof(buf), fmt, arg);
        va_end(arg);

        len = strlen(buf);

        printf("("SYS_LOG_PROFIX":D)%s: %s", tag, buf);
        return len;
    }
    else
    {
        return 0;
    }
#else
    return 0;
#endif
}

int sys_log_i(const char* tag, const char* fmt, ...)
{
#if DEBUG_LEVEL > 2
    if( sys_log_level > 2 )
    {
        char    buf[1024];
        va_list    arg;
        int        len;

        va_start(arg, fmt);
        vsnprintf(buf, sizeof(buf), fmt, arg);
        va_end(arg);

        len = strlen(buf);

        printf("("SYS_LOG_PROFIX":I)%s: %s", tag, buf);

        return len;
    }
    else
    {
        return 0;
    }
#else
    return 0;
#endif
}

int sys_log_w(const char* tag, const char* fmt, ...)
{
#if DEBUG_LEVEL > 1
    if( sys_log_level > 1 )
    {
        char    buf[1024];
        va_list    arg;
        int        len;

        va_start(arg, fmt);
        vsnprintf(buf, sizeof(buf), fmt, arg);
        va_end(arg);

        len = strlen(buf);

        printf("("SYS_LOG_PROFIX":W)%s: %s", tag, buf);

        return len;
    }
    else
    {
        return 0;
    }
#else
    return 0;
#endif
}

int sys_log_e(const char* tag, const char* fmt, ...)
{
#if DEBUG_LEVEL > 0
    if( sys_log_level > 0 )
    {
        char    buf[1024];
        va_list    arg;
        int        len;

        va_start(arg, fmt);
        vsnprintf(buf, sizeof(buf), fmt, arg);
        va_end(arg);

        len = strlen(buf);

        printf("("SYS_LOG_PROFIX":E)%s: %s", tag, buf);

        return len;
    }
    else
    {
        return 0;
    }
#else
    return 0;
#endif
}






/*******************************************************************************
    END OF FILE
*******************************************************************************/
