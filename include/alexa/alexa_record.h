/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: alexa_record.h

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/01/16 10:53:15

*******************************************************************************/

#ifndef _alexa_record_h_
#define _alexa_record_h_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct alexa_record alexa_record;

struct alexa_record* alexa_record_open(int channels, int sample_rate, int sample_bit);

int alexa_record_read(struct alexa_record* record, char* out, int out_len);


void alexa_record_close(struct alexa_record* record);

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
