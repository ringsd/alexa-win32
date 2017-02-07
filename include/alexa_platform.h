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

void* alexa_malloc(int size);
void alexa_free(void* p);

#define alexa_new(x)        (x*)alexa_malloc(sizeof(x))
#define alexa_delete(x)     alexa_free((void*)x)

#ifdef _WIN32

#define ETIMEDOUT			-1

typedef struct alexa_mutex alexa_mutex;
typedef struct alexa_cond alexa_cond;

#else

#definf alexa_cond	pthread_cond_t
#definf alexa_mutex	pthread_mutex


#endif

typedef void (*alexa_thread_proc)(void* p_data);

int alexa_begin_thread(alexa_thread_proc proc, void* p_data, void* stack, int prio);

int alexa_begin_thread2(alexa_thread_proc proc, void* p_data, void* stack, int prio, int size);

int alexa_end_thread(int id);

struct alexa_mutex* alexa_mutex_create(void);
int alexa_mutex_lock(struct alexa_mutex* mutex);
int alexa_mutex_unlock(struct alexa_mutex* mutex);
void alexa_mutex_destroy(struct alexa_mutex* mutex);

struct alexa_cond* alexa_cond_create(void);
int alexa_cond_signal(struct alexa_cond* cond);
int alexa_cond_broadcast(struct alexa_cond* cond);
int alexa_cond_wait(struct alexa_cond* cond, struct alexa_mutex* mutex);
int alexa_cond_timedwait(struct alexa_cond* cond, struct alexa_mutex* mutex, struct timespec * abstime);
void alexa_cond_destroy(struct alexa_cond* cond);

void alexa_delay(long ms);


#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
