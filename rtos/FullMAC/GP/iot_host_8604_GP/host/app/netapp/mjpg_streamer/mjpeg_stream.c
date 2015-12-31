/******************************************************
* mjpeg_stream.c
*
* Purpose: Motion JPEG streaming server application
*
* Author: Eugene Hsu
*
* Date: 2015/07/01
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 
* History :
*
*******************************************************/
//#include <lwip_netconf.h>
//#include <wifi/wifi_conf.h>
#include "api.h"	// for LWIP
#include "stdio.h"
#include "string.h"
#include "gplib.h"
#include "application.h"
#include "mjpeg_stream.h"

#if (APP_MJS)
#if 1
#define printf	DBG_PRINT
#else
#define printf(...)
#endif

#define C_MJPG_IMAGE_NUM	60

//extern xQueueHandle video_stream_q;
extern INT8U *RES_MJPG_DOG_100_START;
extern INT8U *RES_MJPG_DOG_101_START;
extern INT8U *RES_MJPG_DOG_102_START;
extern INT8U *RES_MJPG_DOG_103_START;
extern INT8U *RES_MJPG_DOG_104_START;
extern INT8U *RES_MJPG_DOG_105_START;
extern INT8U *RES_MJPG_DOG_106_START;
extern INT8U *RES_MJPG_DOG_107_START;
extern INT8U *RES_MJPG_DOG_108_START;
extern INT8U *RES_MJPG_DOG_109_START;
extern INT8U *RES_MJPG_DOG_110_START;
extern INT8U *RES_MJPG_DOG_111_START;
extern INT8U *RES_MJPG_DOG_112_START;
extern INT8U *RES_MJPG_DOG_113_START;
extern INT8U *RES_MJPG_DOG_114_START;
extern INT8U *RES_MJPG_DOG_115_START;
extern INT8U *RES_MJPG_DOG_116_START;
extern INT8U *RES_MJPG_DOG_117_START;
extern INT8U *RES_MJPG_DOG_118_START;
extern INT8U *RES_MJPG_DOG_119_START;
extern INT8U *RES_MJPG_DOG_120_START;
extern INT8U *RES_MJPG_DOG_121_START;
extern INT8U *RES_MJPG_DOG_122_START;
extern INT8U *RES_MJPG_DOG_123_START;
extern INT8U *RES_MJPG_DOG_124_START;
extern INT8U *RES_MJPG_DOG_125_START;
extern INT8U *RES_MJPG_DOG_126_START;
extern INT8U *RES_MJPG_DOG_127_START;
extern INT8U *RES_MJPG_DOG_128_START;
extern INT8U *RES_MJPG_DOG_129_START;
extern INT8U *RES_MJPG_DOG_130_START;
extern INT8U *RES_MJPG_DOG_131_START;
extern INT8U *RES_MJPG_DOG_132_START;
extern INT8U *RES_MJPG_DOG_133_START;
extern INT8U *RES_MJPG_DOG_134_START;
extern INT8U *RES_MJPG_DOG_135_START;
extern INT8U *RES_MJPG_DOG_136_START;
extern INT8U *RES_MJPG_DOG_137_START;
extern INT8U *RES_MJPG_DOG_138_START;
extern INT8U *RES_MJPG_DOG_139_START;
extern INT8U *RES_MJPG_DOG_140_START;
extern INT8U *RES_MJPG_DOG_141_START;
extern INT8U *RES_MJPG_DOG_142_START;
extern INT8U *RES_MJPG_DOG_143_START;
extern INT8U *RES_MJPG_DOG_144_START;
extern INT8U *RES_MJPG_DOG_145_START;
extern INT8U *RES_MJPG_DOG_146_START;
extern INT8U *RES_MJPG_DOG_147_START;
extern INT8U *RES_MJPG_DOG_148_START;
extern INT8U *RES_MJPG_DOG_149_START;
extern INT8U *RES_MJPG_DOG_150_START;
extern INT8U *RES_MJPG_DOG_151_START;
extern INT8U *RES_MJPG_DOG_152_START;
extern INT8U *RES_MJPG_DOG_153_START;
extern INT8U *RES_MJPG_DOG_154_START;
extern INT8U *RES_MJPG_DOG_155_START;
extern INT8U *RES_MJPG_DOG_156_START;
extern INT8U *RES_MJPG_DOG_157_START;
extern INT8U *RES_MJPG_DOG_158_START;
extern INT8U *RES_MJPG_DOG_159_START;
extern INT8U *RES_MJPG_END;

static mjpeg_ctl_t mjpeg_ctl_blk =
{
	NULL,				/* Server connection */
	NULL,				/* Client connection */
	//NULL,				/* Task handle */
	NULL,				/* Message Q handler */
	MJPEG_IDLE_STATE,	/* State */
	0,					/* MJPEG buffer */
	0,					/* MJPEG size */
	0,					/* MJPEG index */
	0,					/* FPS lost count */
	0					/* FPS zero count */
};	

static INT8U mjpeg_tx_buf[MJPEG_BUF_SIZE] = {0};
static INT8U mjpeg_rx_buf[MJPEG_BUF_SIZE] = {0};
static INT32U mjpeg_stack[MJPEG_TASK_STACKSIZE/4];
static void *mjpeg_frame_q_stack[MJPEG_MSG_QUEUE_LEN];
static INT32U mjpeg_image_addr[C_MJPG_IMAGE_NUM+1];
static INT32U vdo_enc_stack[VDO_ENC_TASK_STACKSIZE/4];
static INT16U curr_jpg = 0;

/*
static void _video_stream_init(void)
{
	mjpeg_image_addr[0] = (INT32)&RES_MJPG_DOG_100_START;
	mjpeg_image_addr[1] = (INT32)&RES_MJPG_DOG_100_START;
	mjpeg_image_addr[2] = (INT32)&RES_MJPG_DOG_100_START;
	mjpeg_image_addr[3] = (INT32)&RES_MJPG_DOG_100_START;
	mjpeg_image_addr[4] = (INT32)&RES_MJPG_DOG_100_START;
	mjpeg_image_addr[5] = (INT32)&RES_MJPG_DOG_100_START;
	mjpeg_image_addr[6] = (INT32)&RES_MJPG_DOG_100_START;
	mjpeg_image_addr[7] = (INT32)&RES_MJPG_DOG_100_START;
	mjpeg_image_addr[8] = (INT32)&RES_MJPG_DOG_100_START;
	mjpeg_image_addr[9] = (INT32)&RES_MJPG_DOG_100_START;
	mjpeg_image_addr[10] = (INT32)&RES_MJPG_END;
	
	printf("mjpeg_image_addr[0] = %x\r\n", mjpeg_image_addr[0]);
	printf("mjpeg_image_addr[1] = %x\r\n", mjpeg_image_addr[1]);
	printf("mjpeg_image_addr[2] = %x\r\n", mjpeg_image_addr[2]);
}
*/

static void _video_stream_init(void)
{
	mjpeg_image_addr[0] = (INT32)&RES_MJPG_DOG_100_START;
	mjpeg_image_addr[1] = (INT32)&RES_MJPG_DOG_101_START;
	mjpeg_image_addr[2] = (INT32)&RES_MJPG_DOG_102_START;
	mjpeg_image_addr[3] = (INT32)&RES_MJPG_DOG_103_START;
	mjpeg_image_addr[4] = (INT32)&RES_MJPG_DOG_104_START;
	mjpeg_image_addr[5] = (INT32)&RES_MJPG_DOG_105_START;
	mjpeg_image_addr[6] = (INT32)&RES_MJPG_DOG_106_START;
	mjpeg_image_addr[7] = (INT32)&RES_MJPG_DOG_107_START;
	mjpeg_image_addr[8] = (INT32)&RES_MJPG_DOG_108_START;
	mjpeg_image_addr[9] = (INT32)&RES_MJPG_DOG_109_START;
	mjpeg_image_addr[10] = (INT32)&RES_MJPG_DOG_110_START;
	mjpeg_image_addr[11] = (INT32)&RES_MJPG_DOG_111_START;
	mjpeg_image_addr[12] = (INT32)&RES_MJPG_DOG_112_START;
	mjpeg_image_addr[13] = (INT32)&RES_MJPG_DOG_113_START;
	mjpeg_image_addr[14] = (INT32)&RES_MJPG_DOG_114_START;
	mjpeg_image_addr[15] = (INT32)&RES_MJPG_DOG_115_START;
	mjpeg_image_addr[16] = (INT32)&RES_MJPG_DOG_116_START;
	mjpeg_image_addr[17] = (INT32)&RES_MJPG_DOG_117_START;
	mjpeg_image_addr[18] = (INT32)&RES_MJPG_DOG_118_START;
	mjpeg_image_addr[19] = (INT32)&RES_MJPG_DOG_119_START;
	mjpeg_image_addr[20] = (INT32)&RES_MJPG_DOG_120_START;
	mjpeg_image_addr[21] = (INT32)&RES_MJPG_DOG_121_START;
	mjpeg_image_addr[22] = (INT32)&RES_MJPG_DOG_122_START;
	mjpeg_image_addr[23] = (INT32)&RES_MJPG_DOG_123_START;
	mjpeg_image_addr[24] = (INT32)&RES_MJPG_DOG_124_START;
	mjpeg_image_addr[25] = (INT32)&RES_MJPG_DOG_125_START;
	mjpeg_image_addr[26] = (INT32)&RES_MJPG_DOG_126_START;
	mjpeg_image_addr[27] = (INT32)&RES_MJPG_DOG_127_START;
	mjpeg_image_addr[28] = (INT32)&RES_MJPG_DOG_128_START;
	mjpeg_image_addr[29] = (INT32)&RES_MJPG_DOG_129_START;
	mjpeg_image_addr[30] = (INT32)&RES_MJPG_DOG_130_START;
	mjpeg_image_addr[31] = (INT32)&RES_MJPG_DOG_131_START;
	mjpeg_image_addr[32] = (INT32)&RES_MJPG_DOG_132_START;
	mjpeg_image_addr[33] = (INT32)&RES_MJPG_DOG_133_START;
	mjpeg_image_addr[34] = (INT32)&RES_MJPG_DOG_134_START;
	mjpeg_image_addr[35] = (INT32)&RES_MJPG_DOG_135_START;
	mjpeg_image_addr[36] = (INT32)&RES_MJPG_DOG_136_START;
	mjpeg_image_addr[37] = (INT32)&RES_MJPG_DOG_137_START;
	mjpeg_image_addr[38] = (INT32)&RES_MJPG_DOG_138_START;
	mjpeg_image_addr[39] = (INT32)&RES_MJPG_DOG_139_START;
	mjpeg_image_addr[40] = (INT32)&RES_MJPG_DOG_140_START;
	mjpeg_image_addr[41] = (INT32)&RES_MJPG_DOG_141_START;
	mjpeg_image_addr[42] = (INT32)&RES_MJPG_DOG_142_START;
	mjpeg_image_addr[43] = (INT32)&RES_MJPG_DOG_143_START;
	mjpeg_image_addr[44] = (INT32)&RES_MJPG_DOG_144_START;
	mjpeg_image_addr[45] = (INT32)&RES_MJPG_DOG_145_START;
	mjpeg_image_addr[46] = (INT32)&RES_MJPG_DOG_146_START;
	mjpeg_image_addr[47] = (INT32)&RES_MJPG_DOG_147_START;
	mjpeg_image_addr[48] = (INT32)&RES_MJPG_DOG_148_START;
	mjpeg_image_addr[49] = (INT32)&RES_MJPG_DOG_149_START;
	mjpeg_image_addr[50] = (INT32)&RES_MJPG_DOG_150_START;
	mjpeg_image_addr[51] = (INT32)&RES_MJPG_DOG_151_START;
	mjpeg_image_addr[52] = (INT32)&RES_MJPG_DOG_152_START;
	mjpeg_image_addr[53] = (INT32)&RES_MJPG_DOG_153_START;
	mjpeg_image_addr[54] = (INT32)&RES_MJPG_DOG_154_START;
	mjpeg_image_addr[55] = (INT32)&RES_MJPG_DOG_155_START;
	mjpeg_image_addr[56] = (INT32)&RES_MJPG_DOG_156_START;
	mjpeg_image_addr[57] = (INT32)&RES_MJPG_DOG_157_START;
	mjpeg_image_addr[58] = (INT32)&RES_MJPG_DOG_158_START;
	mjpeg_image_addr[59] = (INT32)&RES_MJPG_DOG_159_START;
	mjpeg_image_addr[60] = (INT32)&RES_MJPG_END;
}

INT32U mjpeg_get_send_status(void)
{
	static INT32U cnt = 0, max_zerocnt = 0;
	INT32U fps;
	
	if(mjpeg_ctl_blk.mjpeg_state != MJPEG_IDLE_STATE)
	{
		// One frame is lost because previous frame is still bent sent
		if(mjpeg_ctl_blk.mjpeg_state == MJPEG_CLIENT_SENDING_JPEG_STATE)
		{
			mjpeg_ctl_blk.lostcnt++;
		}

		// Check FPS in the last second
		if(++cnt == MJPEG_FPS)
		{
			fps = MJPEG_FPS - mjpeg_ctl_blk.lostcnt;
			mjpeg_ctl_blk.lostcnt = 0;
			cnt = 0;
			
			// If no frame is sent during the last second, increment zerocnt by 1.
			// Otherwise, reset zerocnt to 0.
			if(fps == 0)
			{
				// "zerocnt" means "the number of seconds during which no frame is sent".
				// If zerocnt is large enough, the connection may be lost.
				mjpeg_ctl_blk.zerocnt++;
				if (mjpeg_ctl_blk.zerocnt > max_zerocnt)
					max_zerocnt = mjpeg_ctl_blk.zerocnt;
			}
			else
			{
				mjpeg_ctl_blk.zerocnt = 0;
			}

			printf("%2d FPS (zero frame max %u seconds)\r\n", fps, max_zerocnt);

			/*
			// If no frame is sent for consecutive MJPEG_FPS_ZERO_CNT seconds,
			// the connection may be lost and we should disconnect.
			if(mjpeg_ctl_blk.zerocnt == MJPEG_FPS_ZERO_CNT)
			{
				INT32U msg = MJPEG_NETWORK_BUSY_EVENT;
				//xQueueSend(mjpeg_ctl_blk.mjpeg_frame_q, &msg, 0);
				OSQPost(mjpeg_ctl_blk.mjpeg_frame_q, (void*)msg);
				printf("No jpg sent in %u seconds. Connection closed.\r\n", MJPEG_FPS_ZERO_CNT);
			}
			*/
			
		}	
	}	
	
	return 	mjpeg_ctl_blk.mjpeg_state;
}	

void mjpeg_send_picture(INT32U addr, INT32U size, INT32U index)
{
	INT8U err;
	INT32U msg;
	
	if (mjpeg_get_send_status() == MJPEG_CLIENT_CONNECT_STATE)
	//if(mjpeg_ctl_blk.mjpeg_state == MJPEG_CLIENT_CONNECT_STATE)
	{
	    mjpeg_ctl_blk.mjpeg_size = size;
	    mjpeg_ctl_blk.mjpeg_addr = addr;
	    mjpeg_ctl_blk.mjpeg_index = index;
		mjpeg_ctl_blk.mjpeg_state = MJPEG_CLIENT_SENDING_JPEG_STATE;
		msg = MJPEG_SEND_EVENT;
		//xQueueSend(mjpeg_ctl_blk.mjpeg_frame_q, &msg, 0);
		//printf("mjpeg_send_picture: post msg %d\r\n", msg);
		
		err = OSQPost(mjpeg_ctl_blk.mjpeg_frame_q, (void*)msg);
		if (err != OS_NO_ERR)
			printf("mjpeg send pic[%u]:%ubytes error(%u)!!\r\n", mjpeg_ctl_blk.mjpeg_index, size, err);
		//else
		//	printf("mjpeg send pic[%u]:%ubytes ok\r\n", mjpeg_ctl_blk.mjpeg_index, size);
	}
	else
	{
		//printf("no client connected\r\n");
	}
}	

static INT32S mjpeg_process_stream(struct netconn *pxClient)
{
	struct netbuf *pxRxBuffer = NULL;
	err_t err;
	INT8U* rxstring;
	INT8U os_err;
	INT32S ret = 0;
	INT16U len = 0;
	BOOLEAN is_print = TRUE;
	static INT32U last_sec = 0;
	static INT32U first_frame_tick = 0xFFFFFFFF;
	
	/* Get net buffer */
    //pxRxBuffer = netconn_recv(pxClient);
    err = netconn_recv(pxClient, &pxRxBuffer);
    
    //if(pxRxBuffer)
    if(err == ERR_OK)
    {
    	/* Where is the data? */
        netbuf_data(pxRxBuffer, (void*)&rxstring, &len);
        if(len)
        {
        	memset(mjpeg_tx_buf, 0 , sizeof(mjpeg_tx_buf));
        	memset(mjpeg_rx_buf, 0 , sizeof(mjpeg_rx_buf));
        	
        	if(len > MJPEG_BUF_SIZE)
        		len = MJPEG_BUF_SIZE;
        		
        	strncpy((char*)mjpeg_rx_buf, (char*)rxstring, len);
        	printf("rxstring[%d]: \r\n%s\r\n", len, mjpeg_rx_buf);
        	
	        if(strstr((char*)rxstring, "GET /?action=stream") != NULL)
			{
				printf("do mjpeg streaming\r\n");
				
				memset(mjpeg_tx_buf, 0 , sizeof(mjpeg_tx_buf));
				sprintf((char*)mjpeg_tx_buf, "HTTP/1.0 200 OK\r\n" \
	            		STD_HEADER \
	           			"Content-Type: multipart/x-mixed-replace;boundary=" BOUNDARY "\r\n" \
	            		"\r\n" \
	            		"--" BOUNDARY "\r\n");
	
	            /* Write header back */
	            err = netconn_write(pxClient, mjpeg_tx_buf, (INT16U) strlen((char*)mjpeg_tx_buf), NETCONN_COPY);
				if(err != ERR_OK)
					printf("\r\nSend MIME header error %d\r\n", err);
				//else
				//	printf("\r\nSend MIME header ok\r\n");

	          	/* Let upper AP could send mjpeg via streamer */
				mjpeg_ctl_blk.mjpeg_state = MJPEG_CLIENT_CONNECT_STATE;
	          	mjpeg_ctl_blk.lostcnt = 0;
	          	mjpeg_ctl_blk.zerocnt = 0;
	          	printf("One client connected\r\n");

	          	/* Start send JPEG frame */
				while(1)
				{
                    INT32U tick, orig_tick, frame_start_tick, frame_end_tick, start_tick, end_tick;
                    
					/* Receive message from queue */
                    INT32U msg;
                    //INT32S err;
                    
		            //err = xQueueReceive(mjpeg_ctl_blk.mjpeg_frame_q, &msg, 0);
					//if(err != pdPASS)				
					//	continue;

					msg = (INT32U)OSQAccept(mjpeg_ctl_blk.mjpeg_frame_q, &os_err);
					//if ((os_err != OS_NO_ERR) || (msg == 0))
					if (os_err != OS_NO_ERR)
					{
						//if (is_print)
						//{
						//	printf("Waiting for image. queue=%x, err=%d, msg=%d\r\n", mjpeg_ctl_blk.mjpeg_frame_q, os_err, msg);
							//is_print = FALSE;
						//}
						OSTimeDly(3);
						continue;
					}
					else
					{
						//is_print = TRUE;
						//printf("mjpeg_frame_q incoming msg. err=%d, msg=%d\r\n", os_err, msg);
					}
					
					switch(msg)				
					{
						case MJPEG_SEND_EVENT:
                            //tick = xTaskGetTickCount();
                            tick = OSTimeGet();
                            if (first_frame_tick == 0xFFFFFFFF)
                            	first_frame_tick = tick;
                            
                            start_tick = tick;
                            tick -= first_frame_tick;
                            if (((tick+50)/100) != last_sec)
                            {
                            	last_sec = (tick+50)/100;
                            	printf("Time:%u sec\r\n", last_sec);
                            }

							/* Send frame header */
							sprintf((char*)mjpeg_tx_buf, "Content-Type: image/jpeg\r\n" \
									"Content-Length: %d\r\n" \
									"X-Timestamp: %d.%02d\r\n" \
									//"\r\n", mjpeg_ctl_blk.mjpeg_size, (int)(tick/1000), (int)(tick%1000));
									"\r\n", mjpeg_ctl_blk.mjpeg_size, (int)(tick/100), (int)(tick%100));
							                		
							err = netconn_write(pxClient, mjpeg_tx_buf, (INT16U) strlen((char*)mjpeg_tx_buf), NETCONN_COPY);
							if(err != ERR_OK)
								printf("\r\nSend JPEG header error %d\r\n", err);
							//else
							//	printf("\r\nSend JPEG header ok\r\n");

							/* Send frame */
                            frame_start_tick = OSTimeGet();
							//err = netconn_write(pxClient, (char*)mjpeg_ctl_blk.mjpeg_addr, (INT16U)mjpeg_ctl_blk.mjpeg_size, NETCONN_COPY);
							err = netconn_write(pxClient, (char*)mjpeg_ctl_blk.mjpeg_addr, (INT16U)mjpeg_ctl_blk.mjpeg_size, NETCONN_NOFLAG);
							frame_end_tick = OSTimeGet();

							//[TODO] Implement a fake AVI task later
							//avi_encode_post_empty(video_stream_q, mjpeg_ctl_blk.mjpeg_addr);
							if(err != ERR_OK)
							{
								printf("\r\nSend JPEG frame error %d\r\n", err);
								goto exit;
							}
							//else
								//printf("Send JPEG frame %ubytes ok (use %ums)\r\n", (INT16U)mjpeg_ctl_blk.mjpeg_size, (frame_end_tick-frame_start_tick)*10);
							
							/* End of frame */
							sprintf((char*)mjpeg_tx_buf, "\r\n--" BOUNDARY "\r\n");				
							
							//err = netconn_write(pxClient, mjpeg_tx_buf, (INT16U) strlen((char*)mjpeg_tx_buf), NETCONN_COPY);
							err = netconn_write(pxClient, mjpeg_tx_buf, (INT16U) strlen((char*)mjpeg_tx_buf), NETCONN_NOFLAG);
							end_tick = OSTimeGet();
							if(err != ERR_OK)
								printf("*ERROR* Send %u end of JPEG error %d!!\r\n", curr_jpg, err);
							//else
								//printf("Send JPEG %u (use %ums). X-Timestamp: %d.%02d\r\n", mjpeg_ctl_blk.mjpeg_index, (end_tick-start_tick)*10, (int)(tick/100), (int)(tick%100));
							
                            mjpeg_ctl_blk.mjpeg_size = 0;
							mjpeg_ctl_blk.mjpeg_state = MJPEG_CLIENT_CONNECT_STATE;
							break;
						
						case MJPEG_NETWORK_BUSY_EVENT:
							ret = -1;
							printf("MJPEG TX busy, abort this connection\r\n");
							goto exit;
						
						default:
							break;		
					}	
				}	            
			}
			else
			{
				printf("Canot find 'GET /?action=stream' in HTML header!\r\n");
			}
        }	
	}
	else
	{
		printf("pxRxBuffer = NULL\r\n");
	}

exit:
	netbuf_delete(pxRxBuffer);
	netconn_close(mjpeg_ctl_blk.mjpeg_client);	
	mjpeg_ctl_blk.mjpeg_state = MJPEG_IDLE_STATE;
	/* Exit this socket to accept a new connection */
	printf("Exit %s\r\n", __func__);
	
	return ret;
}	

void mjpeg_stream_task(void *pvParameters)
{
	INT8U os_err;
	INT32S err;
	err_t lwip_err;

	printf("Enter %s port %d\r\n", __func__, MJPEG_SERVER_PORT);

	//mjpeg_ctl_blk.mjpeg_frame_q = xQueueCreate(MJPEG_MSG_QUEUE_LEN, sizeof(INT32U));
	//if(!mjpeg_ctl_blk.mjpeg_frame_q)
	mjpeg_ctl_blk.mjpeg_frame_q = OSQCreate(mjpeg_frame_q_stack, MJPEG_MSG_QUEUE_LEN);
	if(!mjpeg_ctl_blk.mjpeg_frame_q)
	{
		printf("Create mjpeg_frame_q failed\r\n");
	}
	else
	{
		printf("Create mjpeg_frame_q ok. mjpeg_ctl_blk.mjpeg_frame_q=%x\r\n", mjpeg_ctl_blk.mjpeg_frame_q);
	}

	_video_stream_init();

	mjpeg_ctl_blk.mjpeg_size = 0;
	mjpeg_ctl_blk.mjpeg_state = MJPEG_IDLE_STATE;	
	/* Create a new tcp connection handle */
	mjpeg_ctl_blk.mjpeg_server = netconn_new(NETCONN_TCP);
	if (mjpeg_ctl_blk.mjpeg_server != NULL)
		printf("mjpeg_stream_task: mjpeg server connection created(%x)\r\n", mjpeg_ctl_blk.mjpeg_server);
	else
		printf("mjpeg_stream_task: mjpeg server connection creation fail\r\n");
	
	
	lwip_err = netconn_bind(mjpeg_ctl_blk.mjpeg_server, NULL, MJPEG_SERVER_PORT);
	if (lwip_err == ERR_OK)
		printf("mjpeg_stream_task: mjpeg server bind to port %d\r\n", MJPEG_SERVER_PORT);
	else
		printf("mjpeg_stream_task: mjpeg server bind fail (err=%d)\r\n", lwip_err);
	
	lwip_err = netconn_listen(mjpeg_ctl_blk.mjpeg_server);
	if (lwip_err == ERR_OK)
		printf("mjpeg_stream_task: mjpeg server listen ok\r\n");
	else
		printf("mjpeg_stream_task: mjpeg server listen fail (err=%d)\r\n", lwip_err);
	
    for( ;; )
    {	
    	mjpeg_ctl_blk.mjpeg_client = NULL;

		/* Wait for connection */
		//mjpeg_ctl_blk.mjpeg_client = netconn_accept(mjpeg_ctl_blk.mjpeg_server);
		lwip_err = netconn_accept(mjpeg_ctl_blk.mjpeg_server, &mjpeg_ctl_blk.mjpeg_client);

        //if(mjpeg_ctl_blk.mjpeg_client != NULL)
        if (lwip_err == ERR_OK)
        {
        	printf("mjpeg_stream_task: A new client connected\r\n");

            /* A client connected */
            err = mjpeg_process_stream(mjpeg_ctl_blk.mjpeg_client);
            while(netconn_delete(mjpeg_ctl_blk.mjpeg_client) != ERR_OK)
            {
     			printf("Unable to delete mjpeg_client connection\r\n");
     			/* Delay 10 ms */
                //vTaskDelay(MJPEG_SHORT_DELAY);
                OSTimeDly(MJPEG_SHORT_DELAY/10);
            }
            printf("222 err = %d\r\n", err);
            if(err == -1)
            {
            	while(netconn_delete(mjpeg_ctl_blk.mjpeg_server) != ERR_OK)
	            {
	     			printf("Unable to delete mjpeg_server connection\r\n");
	     			/* Delay 10 ms */
	                //vTaskDelay(MJPEG_SHORT_DELAY);
	                OSTimeDly(MJPEG_SHORT_DELAY/10);
	            }
	            mjpeg_ctl_blk.mjpeg_server = NULL;
	            /* delete message Q */
	            //xQueueReset(mjpeg_ctl_blk.mjpeg_frame_q);
	            OSQFlush(mjpeg_ctl_blk.mjpeg_frame_q);
   				//vQueueDelete(mjpeg_ctl_blk.mjpeg_frame_q);
   				OSQDel(mjpeg_ctl_blk.mjpeg_frame_q, OS_DEL_ALWAYS, &os_err);
				//mjpeg_ctl_blk.mjpeg_frame_q = NULL;
				mjpeg_ctl_blk.mjpeg_frame_q = NULL;
				printf("Exit mjpeg_stream_task...\r\n");
				//vTaskDelete(mjpeg_ctl_blk.mjpeg_task_handle);
				OSTaskDel(MJPG_STREAMER_PRIO);
				
	            break;
        	}	
        }
        else
        {
        	printf("mjpeg_stream_task: mjpeg netconn_accept error(%d)\r\n", lwip_err);
        }
    }
}	

void mjpeg_start_service(void)
{
	INT8U err;

	//if(xTaskCreate(mjpeg_stream_task, "mjpeg_stream_task", MJPEG_TASK_STACKSIZE, NULL, tskIDLE_PRIORITY + 1, &mjpeg_ctl_blk.mjpeg_task_handle) != pdPASS)
	err = OSTaskCreate(mjpeg_stream_task, NULL, (void *)&mjpeg_stack[MJPEG_TASK_STACKSIZE/4-1], MJPG_STREAMER_PRIO);
	if(err != OS_NO_ERR)
	{
		printf("mjpeg_stream_task create failed\r\n");
	}
	else
	{
		printf("mjpeg_stream_task created ok. pri=%d\r\n", MJPG_STREAMER_PRIO);
	}
}

void mjpeg_stop_service(void)
{
	INT8U err;

	OSQFlush(mjpeg_ctl_blk.mjpeg_frame_q);
	OSQDel(mjpeg_ctl_blk.mjpeg_frame_q, OS_DEL_ALWAYS, &err);
	OSTaskDel(MJPG_STREAMER_PRIO);
}

//=========================================================================================================
// Video encode
//=========================================================================================================
// Get next jpg in mjpeg_image_addr[]
static void get_next_jpg(INT32U *addr, INT32U *size, INT32U *index)
{
	*addr = mjpeg_image_addr[curr_jpg];
	*size = mjpeg_image_addr[curr_jpg+1] - mjpeg_image_addr[curr_jpg];
	*index = curr_jpg;
	
	curr_jpg = (curr_jpg >= (C_MJPG_IMAGE_NUM-1)) ? 0 : (curr_jpg+1);
}

static void mjpeg_encode_task(void *pvParameters)
{
	INT32U msg = MJPEG_SEND_EVENT;
	INT32U addr, size, index;

	while (1)
	{
		// Get next image and sened to mjpeg stream task
		//if (mjpeg_get_send_status() == MJPEG_CLIENT_CONNECT_STATE)
		//{
			get_next_jpg(&addr, &size, &index);
			mjpeg_send_picture(addr, size, index);
		//}

		OSTimeDly(1000/MJPEG_FPS/10);
		//printf("mjpeg_encode_task wake up\r\n");
	}
}

void mjpeg_encode_start_service(void)
{
	INT8U err;

	err = OSTaskCreate(mjpeg_encode_task, NULL, (void *)&vdo_enc_stack[VDO_ENC_TASK_STACKSIZE/4-1], VDO_ENC_PRIO);
	if(err != OS_NO_ERR)
	{
		printf("mjpeg_encode_task create failed\r\n");
	}
	else
	{
		printf("mjpeg_encode_task created ok. pri=%d\r\n", VDO_ENC_PRIO);
	}
}

void mjpeg_encode_stop_service(void)
{
}

#endif //#if (APP_MJS)