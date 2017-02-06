/*******************************************************************************
	Copyright Ringsd. 2017.
	All Rights Reserved.

	File: sys_log.h

	Description:

	TIME LIST:
	CREATE By Ringsd   2017/01/04 11:52:56

*******************************************************************************/

#ifndef _sys_log_h_
#define _sys_log_h_

#ifdef __cplusplus
extern "C" {
#endif

void sys_log_set_level(int level);
int sys_log_get_level(void);
void sys_log_set_levelname(const char* name);

int sys_log(int level, const char* tag, const char* fmt, ...);
int sys_log_d(const char* tag, const char* fmt, ...);
int sys_log_i(const char* tag, const char* fmt, ...);
int sys_log_w(const char* tag, const char* fmt, ...);
int sys_log_e(const char* tag, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
	END OF FILE
*******************************************************************************/
