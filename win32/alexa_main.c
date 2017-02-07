/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_main.c

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/02/07 16:37:16

*******************************************************************************/

#include "alexa_service.h"
#include "alexa_auth.h"

#pragma comment ( lib, "ws2_32.lib" )
#pragma comment ( lib, "winmm.lib" )

int main( int argv, const char* argc[])
{
	alexa_authmng_init();
	alexa_http2_init(NULL);
	return 0;
}




/*******************************************************************************
    END OF FILE
*******************************************************************************/
