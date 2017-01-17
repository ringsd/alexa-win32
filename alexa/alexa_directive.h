/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.
	
	File: alexa_directive.h

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/01/13 17:03:29

*******************************************************************************/

#ifndef _alexa_directive_h_
#define _alexa_directive_h_

#ifdef __cplusplus
extern "C" {
#endif

int alexa_directive_register(const char* name_space, int(*process)( alexa_service* as, cJSON* root) );
int alexa_directive_unregister(const char* name_space);
int alexa_directive_process(alexa_service* as, const char* value);

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
	END OF FILE
*******************************************************************************/
