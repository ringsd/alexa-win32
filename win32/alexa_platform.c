/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.
	
	File: alexa_platform.c

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/01/17 09:46:03

*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include <process.h>

#include "alexa_platform.h"

#define TAG			"win32"

void* alexa_malloc( int size )
{
    return malloc(size);
}

void alexa_free(void* p)
{
    free( p );
}

void alexa_generate_uuid( const char* uuid, int len )
{
    //linux read this file "/proc/sys/kernel/random/uuid"
}

/*******************************************************************************
    thread
*******************************************************************************/

int alexa_begin_thread(alexa_thread_proc proc, void* p_data, void* stack, int prio)
{
	prio = prio;
	stack = stack;
	return _beginthread(proc, 0, p_data);
}

int alexa_begin_thread2(alexa_thread_proc proc, void* p_data, void* stack, int prio, int size)
{
	prio = prio;
	stack = stack;
	size = size;
	return _beginthread(proc, 0, p_data);
}

int alexa_end_thread(int id)
{
	id = id;
//	_endthreadex(id);
	return 0;
}

/*******************************************************************************
    mutex
*******************************************************************************/

struct alexa_mutex{
	HANDLE  h;
};

struct alexa_cond{
	HANDLE  h;
};

struct alexa_mutex* alexa_mutex_create(void)
{
    struct alexa_mutex* mutex = alexa_new( struct alexa_mutex );
    if( mutex )
    {
        mutex->h = CreateEvent(0, 0, 1, 0);
        if( mutex->h == NULL )
        {
            alexa_delete(mutex);
            mutex = NULL;
        }

        return mutex;
    }
    return NULL;
}

int alexa_mutex_lock(struct alexa_mutex* mutex)
{
	int ret;

	ret = WaitForSingleObject(mutex->h, 0);

	if (WAIT_OBJECT_0 == ret)
		return 0;

	if (WAIT_TIMEOUT == ret)
		return 1;

	return -1;
}

int alexa_mutex_unlock(struct alexa_mutex* mutex)
{
	SetEvent(mutex->h);
	return 0;
}

void alexa_mutex_destory(struct alexa_mutex* mutex)
{
	CloseHandle(mutex->h);
    alexa_delete(mutex);
}

struct alexa_cond* alexa_cond_create( void )
{
    struct alexa_cond* cond = alexa_new( struct alexa_cond );
    if( cond )
    {
        return cond;
    }
    return NULL;
}

int alexa_cond_signal(struct alexa_cond* cond)
{
    return 0;
}

int alexa_cond_broadcast(struct alexa_cond* cond)
{
    return 0;
}

int alexa_cond_wait(struct alexa_cond* cond, struct alexa_mutex* mutex)
{
    return 0;
}

int alexa_cond_timedwait(struct alexa_cond* cond, struct alexa_mutex* mutex, struct timespec * abstime)
{
	return 0;
}

void alexa_cond_destory( struct alexa_cond* cond )
{
    alexa_delete(cond);
}


void alexa_delay(long ms)
{
	Sleep(ms);
}

/*****************************************************************************/



/*******************************************************************************
	END OF FILE
*******************************************************************************/
