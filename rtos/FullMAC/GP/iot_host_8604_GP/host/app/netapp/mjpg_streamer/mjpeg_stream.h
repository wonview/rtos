/******************************************************
* mjpeg_stream.h
*
* Purpose: Motion JPEG streaming server application header file
*
* Author: Eugene Hsu
*
* Date: 2015/07/08
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 
* History :
*
*******************************************************/
#ifndef __MJPEG_STREAM_H__
#define __MJPEG_STREAM_H__
/*
 * Standard header to be send along with other header information like mimetype.
 *
 * The parameters should ensure the browser does not cache our answer.
 * A browser should connect for each file and not serve files from his cache.
 * Using cached pictures would lead to showing old/outdated pictures
 * Many browser seem to ignore, or at least not always obey those headers
 * since i observed caching of files from time to time.
 */
#define STD_HEADER "Connection: close\r\n" \
    "Server: GPMJPEG-Streamer/0.2\r\n" \
    "Cache-Control: no-store, no-cache, must-revalidate, pre-check=0, post-check=0, max-age=0\r\n" \
    "Pragma: no-cache\r\n" \
    "Expires: Wen, 1 Jul 2015 12:34:56 GMT\r\n"
    

/* the boundary is used for the M-JPEG stream, it separates the multipart stream of pictures */
#define BOUNDARY "boundarydonotcross"    

#define MJPEG_SERVER_PORT		8080
#define MJPEG_BUF_SIZE			10*1024
#define MJPEG_TASK_STACKSIZE	2048
#define MJPEG_SHORT_DELAY       10
#define MJPEG_MSG_QUEUE_LEN		4
#define MJPEG_FPS				30
#define MJPEG_Q					10
#define MJPEG_FPS_ZERO_CNT		10

#define VDO_ENC_TASK_STACKSIZE	2048

typedef struct mjpeg_msg_q_s
{
	OS_EVENT *hdl;
	INT32U tbl[MJPEG_MSG_QUEUE_LEN];
} mjpeg_msg_q_t;

typedef struct mjpeg_ctl_s
{
	struct netconn *mjpeg_server;
	struct netconn *mjpeg_client;
	//xTaskHandle mjpeg_task_handle;
	//xQueueHandle mjpeg_frame_q;
	OS_EVENT *mjpeg_frame_q;
	INT32U mjpeg_state;
	INT32U mjpeg_addr;
	INT32U mjpeg_size;
	INT32U mjpeg_index;
	INT32U lostcnt;
	INT32U zerocnt;
} mjpeg_ctl_t;	

typedef enum MJPEG_EVENT_E
{
	MJPEG_SEND_EVENT = 0,
	MJPEG_NETWORK_BUSY_EVENT
} MJPEG_EVENT_T;

typedef enum MJPEG_STATE_E
{
	MJPEG_IDLE_STATE = 0,
	MJPEG_CLIENT_CONNECT_STATE,
	MJPEG_CLIENT_SENDING_JPEG_STATE
} MJPEG_STATE_T;

//extern xQueueHandle video_stream_q;
extern OS_EVENT *video_stream_q;

extern void mjpeg_start_service(void);
extern void mjpeg_stop_service(void);

#endif	//__MJPEG_STREAM_H__