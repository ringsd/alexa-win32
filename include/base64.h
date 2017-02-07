/*******************************************************************************
    Copyright Ringsd. 2017.
    All Rights Reserved.
    
    File: base64.h

    Description:

    TIME LIST:
    CREATE By Ringsd   2017/2/07 12:06:17

*******************************************************************************/

#ifndef _base64_h_
#define _base64_h_

#ifdef __cplusplus
extern "C" {
#endif

int base64_decode_len(const char *bufcoded);

int base64_decode(char *bufplain, const char *bufcoded);

int base64_encode_len(int len);

int base64_encode(char *encoded, const char *string, int len);

//https://tools.ietf.org/html/rfc7636#appendix-A
int base64_encode_urlsafe(char *encoded, const char *string, int len);

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
