/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_setting.h

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/2/17 10:04:34

*******************************************************************************/

#ifndef _alexa_setting_h_
#define _alexa_setting_h_

#ifdef __cplusplus
extern "C" {
#endif


typedef struct alexa_setting alexa_setting;

/*
*@brief init the alexa setting
*@param
*@return struct alexa_setting* setting
*        NULL is fail, otherwise success
*/
struct alexa_setting* alexa_setting_init(struct alexa_service* as);

/*
*@brief done the alexa setting
*@param struct alexa_setting* setting, the alexa_setting object
*@return 0 success, otherwise fail
*/
int alexa_setting_done(struct alexa_setting* setting);

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
