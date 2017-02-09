#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
//#include <cjson/cJSON.h>
#include <cJSON/cJSON.h>
#include <stdlib.h>
//#include <pthread.h>


//	int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)   //这个函数是为了符合CURLOPT_WRITEFUNCTION, 而构造的
{
	printf("write data begin...\n");
    int written = fwrite(ptr, size, nmemb, (FILE *)stream);
    fflush((FILE *)stream);
    return written;
}
size_t readFileFunc(char *buffer, size_t size, size_t nitems, void *instream)
{
	printf("begin freadfunc ... \n");
	int readSize = fread( buffer, size, nitems, (FILE *)instream );

	return readSize;
}

#define UPLOAD_FILE_NAME     "./16k.raw"
#define HEAD_FILE_NAME       "./HeadFile"
#define BODY_FILE_NAME       "./BodyFile"
#define DEL_HTTPHEAD_EXPECT  "Expect:"
#define DEL_HTTPHEAD_ACCEPT  "Accept:"

char *atoken = "Atza|IwEBIOf2Wq_cyK1qW0dKdlU0KA4PRCq9CjTAD8PYKIWjN1o2zOneEMJtNIogP2XHBVARbPX_7PnSYhRPFKpQPumUMJa7_wXD3wGXFQ7-GwZcObU1_BXTzyY326gCC_LhVyWKIBhGN9Jogx_vjWaMS2SsGc-gy45dhRxwd0TsAUZ9vlSMFgl9U-_RLrVW_Awn_r8C6ucTFg4Q6P99iKN5-3fRz26Nb3kCLZJtVktYlHhBwND-8IlSfFy_It7aN4BWrS9r0Gwe8qMt8Fm69w-utfwDIV0eilh246Aosq6tATbUZjYPMbVC-IYImPc-719wrtd-RmvrKxeAKlQXv5UtAt6YPEt_DnHaj5PJLxIaMS0iKwj_X-K_TrB7tYmBwCU1LjAUPIiXWME1AT9bR_4cBPGnBhGcEytvbEgMqVPWWEafxOB25iqCC3K_JeEVYtxNPJsALFhzdW2bPqjTicSfoQakfDS2CmAXFV8by3IH22SG67XXSxdPn-NfLNGepLEy3trg6pUWtC53bSUAV2PYB_8oABOm5bumBnbZaxgBJKLFPDmEnQ";

int demo_main(int argc, char **argv)
{
	int ret;
	if( 2 != argc ){
		printf("没有输入 AccessToken ...\n");
		return -1;
	}
	char Authorization[1024] = "Authorization:Bearer ";
	//strcat( Authorization, argv[1] );
	strcat( Authorization, atoken );
	printf( "%s\nstarting ...\n", Authorization );




/*************************************************************************************
 *
 *--------------------------HTTP_HEADER----------------------------------------------
 *
 * **********************************************************************************/
	struct curl_slist		*httpHeaderSlist	= NULL;
	if( NULL == (httpHeaderSlist = curl_slist_append(httpHeaderSlist, DEL_HTTPHEAD_EXPECT)))
		{ printf( "DEL_HTTPHEAD_EXPECT_ERR" );goto CURL_SLIST_APPEND_ERR; }
	if( NULL == (httpHeaderSlist = curl_slist_append(httpHeaderSlist, DEL_HTTPHEAD_ACCEPT)))
		{ printf( "DEL_HTTPHEAD_ACCEPT_ERR" );goto CURL_SLIST_APPEND_ERR; }
	if( NULL == (httpHeaderSlist = curl_slist_append(httpHeaderSlist, "Path: /v20160207/events")))
		{ printf( "CURL_SLIST_APPEND_PATH_ERR" );goto CURL_SLIST_APPEND_ERR; }
	if( NULL == (httpHeaderSlist = curl_slist_append(httpHeaderSlist, Authorization)))
		{ printf( "CURL_SLIST_APPEND_AUTH_ERR" );goto CURL_SLIST_APPEND_ERR; }
	if( NULL == (httpHeaderSlist = curl_slist_append(httpHeaderSlist, "Content-type: multipart/form-data" )))
		{ printf( "CURL_SLIST_APPEND_TYPE_ERR" );goto CURL_SLIST_APPEND_ERR; }
	if( NULL == (httpHeaderSlist = curl_slist_append(httpHeaderSlist, "Transfer-Encoding: chunked")))
		{ printf( "CURL_SLIST_APPEND_CHUNK_ERR" );goto CURL_SLIST_APPEND_ERR; }

/**************************************************************************************
 *
 *--------------------------JSON_CREATE------------------------------------------------------
 *
 *************************************************************************************/
	cJSON *root, *context, *event, *header, *payload, *nonname, *tmpJson;char *strJSONout;
	root=cJSON_CreateObject();
	cJSON_AddItemToObject(root, "context", context = cJSON_CreateArray() );

				//===========Context=============//
				/*
	cJSON_AddItemToArray(context, nonname = cJSON_CreateObject() );
	cJSON_AddItemToObject(nonname, "header", header = cJSON_CreateObject() );
	cJSON_AddStringToObject( header, "namespace", "AudioPlayer" );
	cJSON_AddStringToObject( header, "name", "playbackState" );
	cJSON_AddItemToObject(nonname, "payload", payload = cJSON_CreateObject() );
	cJSON_AddStringToObject( payload, "token", "" );//"abcd1234"
	cJSON_AddNumberToObject( payload, "offsetInMilliseconds", 0 );
	cJSON_AddStringToObject( payload, "playerActivity", "FINISHED" );
				

	cJSON_AddItemToArray(context, nonname = cJSON_CreateObject() );
	cJSON_AddItemToObject(nonname, "header", header = cJSON_CreateObject() );
	cJSON_AddStringToObject( header, "namespace", "Alerts" );
	cJSON_AddStringToObject( header, "name", "AlertsState" );
	cJSON_AddItemToObject( nonname, "payload", payload = cJSON_CreateObject() );
	cJSON_AddItemToObject( payload, "allAlerts", tmpJson = cJSON_CreateArray() );
	cJSON_AddItemToArray( tmpJson, nonname = cJSON_CreateObject() );
	cJSON_AddStringToObject( nonname , "token", "" );
	cJSON_AddStringToObject( nonname, "type", "TIMER" );
	cJSON_AddStringToObject( nonname, "scheduledTime", "" );//2015-10-30T22:34:51+00:00
	cJSON_AddItemToObject( payload, "activeAlerts", tmpJson = cJSON_CreateArray() );



	cJSON_AddItemToArray(context, nonname = cJSON_CreateObject() );
	cJSON_AddItemToObject(nonname, "header", header = cJSON_CreateObject() );
	cJSON_AddStringToObject( header, "namespace", "Speaker" );
	cJSON_AddStringToObject( header, "name", "VolumeState" );
	cJSON_AddItemToObject(nonname, "payload", payload = cJSON_CreateObject() );
	cJSON_AddNumberToObject( payload, "volume",  );
	cJSON_AddFalseToObject( payload, "muted" );	

	cJSON_AddItemToArray(context, nonname = cJSON_CreateObject() );
	cJSON_AddItemToObject(nonname, "header", header = cJSON_CreateObject() );
	cJSON_AddStringToObject( header, "namespace", "SpeechSynthesizer" );
	cJSON_AddStringToObject( header, "name", "SpeechState" );
	cJSON_AddItemToObject(nonname, "payload", payload = cJSON_CreateObject() );
	cJSON_AddStringToObject( payload, "token", "" );//zxcv8523
	cJSON_AddNumberToObject( payload, "offsetInMilliseconds", 0 );
	cJSON_AddStringToObject( payload, "playerActivity", "FINISHED" );
					*/
				//========Events===========//
	cJSON_AddItemToObject(root, "event", event = cJSON_CreateObject());
	cJSON_AddItemToObject(event, "header", header = cJSON_CreateObject());
	cJSON_AddStringToObject(header, "namespace", "SpeechRecognizer");
	cJSON_AddStringToObject(header, "name", "Recognize");
	cJSON_AddStringToObject(header, "messageId", "messageId-seven" );//"messageId-123"
	cJSON_AddStringToObject(header, "dialogRequestId", "dialogRequestId-seven" );//"dialogRequestId-123"
	cJSON_AddItemToObject(event, "payload", payload = cJSON_CreateObject());
	cJSON_AddStringToObject(payload, "profile", "CLOSE_TALK");
	cJSON_AddStringToObject(payload, "format", "AUDIO_L16_RATE_16000_CHANNELS_1");

				//===========Tail===============//
	strJSONout = cJSON_Print(root);	
	cJSON_Delete(root);
	
	printf("%s\n%ld\n", strJSONout, strlen(strJSONout));




/****************************************************************************************
 *
 *--------------------------------- Formadd ---------------------------------------------
 *
 ***************************************************************************************/
	FILE *saveHeadFile  = fopen( HEAD_FILE_NAME, "w+" );
	FILE *saveBodyFile   = fopen( BODY_FILE_NAME,  "w+" );
	FILE *uploadFile = fopen( UPLOAD_FILE_NAME, "rb" );
	if ( ( NULL == saveHeadFile ) || ( NULL == saveBodyFile ) || ( NULL == uploadFile ) ) {
		printf("Fopen failed ...\n" );
		goto FOPEN_ERR;
	}
	fseek( uploadFile, 0, SEEK_END);
	long uploadFileSize = ftell( uploadFile );
	fseek( uploadFile, 0, SEEK_SET);
	printf("uploadFileSize = %ld\n", uploadFileSize);

	struct curl_slist		*contentAudioHeaderSlist	= NULL;

	
	contentAudioHeaderSlist = curl_slist_append(contentAudioHeaderSlist, "Content-Disposition: form-data; name=\"audio\"");
	contentAudioHeaderSlist = curl_slist_append(contentAudioHeaderSlist, "Content-Type: application/octet-stream");

 
	struct curl_httppost 	*postFirst 	= NULL,	*postLast	= NULL;

				//=============JSON==================//
	if(0 != curl_formadd(&postFirst, &postLast,
							CURLFORM_COPYNAME, "metadata", /* CURLFORM_PTRCONTENTS, pAlexaJSON,  */
							CURLFORM_COPYCONTENTS, strJSONout,
							CURLFORM_CONTENTTYPE, "application/json; charset=UTF-8",
							CURLFORM_END))
	{	printf( "CURL_FORMADD_JSON_ERR\n" ); goto CURL_FORMADD_ERR; }
				//=============Audio=================//
	if( 0 != curl_formadd(&postFirst, &postLast,
							CURLFORM_COPYNAME, "audio",
							//CURLFORM_CONTENTHEADER, contentAudioHeaderSlist,
							CURLFORM_STREAM, uploadFile,
							CURLFORM_CONTENTSLENGTH, uploadFileSize,//uploadFileSize -
							CURLFORM_CONTENTTYPE, "application/octet-stream", //"audio/L16; rate=16000; channels=1",
							CURLFORM_END) )
	{	printf( "CURL_FORMADD_AUDIO_ERR\n" ); goto CURL_FORMADD_ERR; }
	
/*************************************************************************************
 *
 *--------------------------- curl_easy_setopt ---------------------------------------
 *
 ************************************************************************************/
	curl_global_init(CURL_GLOBAL_ALL);
	CURL *curl = curl_easy_init();
	if( CURLE_OK != CURLE_OK ){ printf( "CURLE_EASY_ERR\n" ); goto CURLE_EASY_ERR;}
	if( CURLE_OK != curl_easy_setopt( curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0 ) )
		{printf( "CURLOPT_HTTP_VERSION_ERR\n" );goto CURL_EASY_SETOPT_ERR;}
		
	if(CURLE_OK != curl_easy_setopt( curl, CURLOPT_URL, "https://avs-alexa-na.amazon.com/v20160207/events" ))//v20160207/events
    //if(CURLE_OK != curl_easy_setopt( curl, CURLOPT_URL, "https://54.239.23.243/v20160207/events" ))//v20160207/events
		{printf( "CURLOPT_URL_ERR\n" ); goto CURL_EASY_SETOPT_ERR;}
		
	if(CURLE_OK != curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L))
		{printf( "CURLOPT_VERBOSE_ERR\n" ); goto CURL_EASY_SETOPT_ERR;}

#if 0
/* ca by seven */
    if(CURLE_OK != curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L)){
        printf("CURLOPT_SSL_VERIFYPEER!!!\n");
    }
//    curl_easy_setopt(curl,CURLOPT_CAPATH,"/etc/ssl/cert/");
    //curl_easy_setopt(curl,CURLOPT_CAINFO,"F:\\JZ\\X1000\\alexa\\alexa-avs-sample-app\\samples\\javaclient\\certs\\client\\client.crt");
    curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,1L);
#else
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif

/* ca */		
	if(CURLE_OK != curl_easy_setopt(curl, CURLOPT_HTTPHEADER, httpHeaderSlist))
		{printf( "CURLOPT_HTTPHEADER_ERR\n" ); goto CURL_EASY_SETOPT_ERR;}

	curl_easy_setopt(curl, CURLOPT_READFUNCTION, readFileFunc);////////////////////////////////设置读文件函数接口
	curl_easy_setopt( curl, CURLOPT_HEADERFUNCTION, write_data );  //CURLOPT_WRITEFUNCTION 将后继的动作交给write_data函数处理
    curl_easy_setopt( curl, CURLOPT_HEADERDATA, saveHeadFile );
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_data );  //CURLOPT_WRITEFUNCTION 将后继的动作交给write_data函数处理
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, saveBodyFile );

 
	if(CURLE_OK != curl_easy_setopt(curl, CURLOPT_HTTPPOST, postFirst))
		{printf( "CURLOPT_HTTPPOST_ERR\n" ); goto CURL_EASY_SETOPT_ERR;}

	printf("Next Step is Pertorm ... \n");
	ret = curl_easy_perform(curl);
	if(ret != CURLE_OK )
		{printf( "CURL_EASY_PERFORM_ERR:%d\n", ret ); goto CURL_EASY_PERFORM_ERR;}
	printf( "Next step is the end ... \n" );
	
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	curl_formfree( postFirst );
	free(strJSONout);
	curl_slist_free_all(httpHeaderSlist);
	fclose(uploadFile);
	fclose(saveBodyFile);
	fclose(saveHeadFile);
	printf( "This is the end ... \n" );
	return 0;

CURL_EASY_PERFORM_ERR:
CURL_EASY_SETOPT_ERR:
	curl_easy_cleanup(curl);
CURLE_EASY_ERR:
	curl_global_cleanup();
	curl_formfree( postFirst );
CURL_FORMADD_ERR:
	free(strJSONout);
CURL_SLIST_APPEND_ERR:
	curl_slist_free_all(httpHeaderSlist);
	fclose(uploadFile);
	fclose(saveBodyFile);
	fclose(saveHeadFile);
FOPEN_ERR:	
	return -1;
}
