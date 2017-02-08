/*******************************************************************************
    Copyright Ringsd. 2016.
    All Rights Reserved.
    
    File: amazon_alexa_if.h

    Description:
    1.aa means the amazon alexa

    TIME LIST:
    CREATE By Ringsd   2016/1/20 13:50:04

*******************************************************************************/

#ifndef _amazon_alexa_if_h_
#define _amazon_alexa_if_h_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief voice recognition state
 */
enum aa_state_type {
    /**
     * @brief shutdown state
     */
    AA_STATE_SHUTDOWN = 0,
    /**
     * @brief idle state
     */
    AA_STATE_IDLE,
    /**
     * @brief recognize state
     */
    AA_STATE_RECOG,
    /**
     * @brief quit state
     */
    AA_STATE_QUIT,
    /**
     * @brief error state
     */
    AA_STATE_ERROR,
};

typedef struct aa_service aa_service;

typedef int (*mozart_amazon_callback)(void *);

/**
 * @brief start voice recognition of amazon
 */
struct aa_service* aa_startup_service(mozart_amazon_callback callback);

/**
 * @brief quit voice recognition of amazon
 */
int aa_shutdown_service(struct aa_service* aa);

/**
 * @brief wakeup voice recognition of amazon
 */
int aa_wakeup_service(struct aa_service* aa);

/**
 * @brief wakeup voice recognition of amazon
 */
void aa_cancel_service(struct aa_service* aa);

enum aa_state_type aa_get_status(struct aa_service* aa);

/**
 @brief those param recive from the companion app
 */
void aa_set_authorization(char *authorization_code, char *redirect_uri, char *client_id, char *code_verifier);

#ifdef __cplusplus
}
#endif

#endif

/*******************************************************************************
    END OF FILE
*******************************************************************************/
