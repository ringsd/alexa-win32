/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.
	
	File: alexa_auth.h

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/1/19 11:28:04

*******************************************************************************/

#ifndef _alexa_auth_h_
#define _alexa_auth_h_

#ifdef __cplusplus
extern "C" {
#endif

struct alexa_authmng* alexa_authmng_init(void);

void alexa_authmng_done(struct alexa_authmng* authmng);

#ifdef ALEXA_UNIT_TEST
void alexa_authmng_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
	END OF FILE
*******************************************************************************/
