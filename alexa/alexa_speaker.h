/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.
	
	File: alexa_speaker.h

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/1/18 21:47:44

*******************************************************************************/

#ifndef _alexa_speaker_h_
#define _alexa_speaker_h_

#ifdef __cplusplus
extern "C" {
#endif

cJSON* speaker_volume_state(alexa_service* as);


int alexa_speaker_init(alexa_service* as);

int alexa_speaker_done(alexa_service* as);

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
	END OF FILE
*******************************************************************************/
