/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.
	
	File: alexa_platform.c

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/01/17 09:46:03

*******************************************************************************/


void* alexa_malloc( int size )
{
    return malloc(size);
}

void alexa_free(void* p)
{
    free( p );
}

void alexa_free(void* p)
{
	free(p);
}

void alexa_generate_uuid( const char* uuid, int len )
{
    //linux read this file "/proc/sys/kernel/random/uuid"
}


/*******************************************************************************
	END OF FILE
*******************************************************************************/
