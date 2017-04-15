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

#ifdef _WIN32
#include <windows.h>
#include <time.h>

#else
    
#include <pthread.h>

#endif

void* alexa_malloc(int size);
void* alexa_zmalloc(int size);
void alexa_free(void* p);

#define alexa_new(x)        (x*)alexa_zmalloc(sizeof(x))
#define alexa_delete(x)     alexa_free((void*)x)

#define ALEXA_SAFE_FREE(x) if(x){alexa_free(x);x=NULL;}

#ifdef _WIN32

#define ETIMEDOUT            -1

#endif

#ifdef WIN32
typedef void (*alexa_thread_proc)(void* p_data);
#else
typedef void* (*alexa_thread_proc)(void* p_data);

#define _snprintf   snprintf

#endif

int alexa_begin_thread(alexa_thread_proc proc, void* p_data, void* stack, int prio);

int alexa_begin_thread2(alexa_thread_proc proc, void* p_data, void* stack, int prio, int size);

int alexa_end_thread(int id);

typedef struct alexa_mutex alexa_mutex;
typedef struct alexa_cond alexa_cond;

struct alexa_mutex* alexa_mutex_create(void);
int alexa_mutex_lock(struct alexa_mutex* mutex);
int alexa_mutex_unlock(struct alexa_mutex* mutex);
void alexa_mutex_destroy(struct alexa_mutex* mutex);

struct alexa_cond* alexa_cond_create(void);
int alexa_cond_signal(struct alexa_cond* cond);
int alexa_cond_broadcast(struct alexa_cond* cond);
int alexa_cond_wait(struct alexa_cond* cond, struct alexa_mutex* mutex);
//int alexa_cond_timedwait(struct alexa_cond* cond, struct alexa_mutex* mutex, struct timespec* abstime);
void alexa_cond_destroy(struct alexa_cond* cond);

void alexa_delay(long ms);

#define ALEXA_UUID_LENGTH      36
void alexa_generate_uuid(char* uuid, int len);

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
