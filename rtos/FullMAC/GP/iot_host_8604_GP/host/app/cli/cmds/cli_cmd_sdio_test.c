#include <config.h>
#include <cmd_def.h>
#include <log.h>
#include <host_global.h>
#include "lwip/netif.h"
#include <hdr80211.h>
#include "cli_cmd_wifi.h"
#include "cli_cmd_test.h"
#include "cli_cmd_sdio_test.h"
#include <host_apis.h>
#include <hwmac/drv_mac.h>
#include <rtos.h>
#include <ssv_ex_lib.h>
#include <config_ssid.h>


#include <os_wrapper.h>
#include <ap/common/ieee80211.h>
#include <time.h>

#if (defined _WIN32)
#include <drv/sdio/sdio.h>
#elif (defined __linux__)
#include <drv/sdio_linux/sdio.h>
#endif

#ifdef THROUGHPUT_TEST

#ifdef OLDCMDENG
extern s32 _ssv6xxx_send_msg_to_hcmd_eng(void *data);
extern s32 	ssv6xxx_drv_send(void *dat, size_t len);
extern bool ssv6xxx_prepare_wifi_txreq(void *frame, s32 len, bool f80211, enum ssv6xxx_data_priority priority);
#else
extern bool TxHdl_prepare_wifi_txreq(void *frame, u32 len, bool f80211, u32 priority, u8 tx_dscrp_flag);
#endif

u32 testTxFrameTimes = 0;
u32 testRxFrameTimes = 0;
u32 rx_count=0;
u8	throughput_opmode=0,randomLengthMode=0;
bool isQos =false;
u8 ack_policy = 0x0;
u16 qos=0x0;



static u32 hostTxFrameSize;
static clock_t startTime,finishTime;
static double duration =0 ;

ssv6xxx_data_result sdio_rx_test_dat_cb(void *dat, u32 len)
{
	u8 *rxData = (u8 *)dat; 
	double speed= 0;
	if(rx_count == 0)
		startTime=clock();
	
	rx_count++;
	if(rx_count == testRxFrameTimes)
	{
		if(testRxFrameTimes == 1)
		{
			LOG_DEBUG("only 1 frame\n");
			return SSV6XXX_DATA_ACPT;		
		}
		
		finishTime = clock();
		if(finishTime > startTime)
		{
			duration =(double)(finishTime-startTime)/CLOCKS_PER_SEC;
	        if(duration != 0)
			{
	             speed =  ((len*testRxFrameTimes)>>10)/duration;		
			     LOG_DEBUG("sdio rx data frame size=%d, speed=%f(KB/sec)\n",len,speed);
			     LOG_DEBUG("sdio rx data frame size =%d, frequence =%f\n (frame/sec)\n",len,testRxFrameTimes/duration);
		    }
			else
				LOG_DEBUG("FAIL!! duration = 0\n");
		    LOG_DEBUG("sdio rx data frame size =%d,duration=%f (sec)\n",len,duration);
		}
		else
			LOG_DEBUG("FAIL!! finishTime\n");
    }

    os_frame_free(dat);

	return SSV6XXX_DATA_ACPT;	
	
}

void sdio_rx_test_evt_cb(void *dat, u32 len)
{
	u8 *rxData = (u8 *)dat; 
	double speed= 0;
	if(rx_count == 0)
		startTime=clock();
	
	rx_count++;

	if(rx_count == testRxFrameTimes)
	{
		if(testRxFrameTimes == 1)
		{
			LOG_DEBUG("only 1 frame\n");
			return;
		
		}	
		finishTime = clock();
		if(finishTime > startTime)
		{
			duration =(double)(finishTime-startTime)/CLOCKS_PER_SEC;
	        if(duration != 0)
			{
	             speed =  ((hostTxFrameSize*testRxFrameTimes)>>10)/duration;		
			     LOG_DEBUG("sdio rx evt frame size=%d, speed=%f(KB/SEC)\n",len,speed);
			     LOG_DEBUG("sdio rx evt frame size =%d, frequence =%f\n (frame/sec)\n",len,testRxFrameTimes/duration);
		    }
			else
				LOG_DEBUG("FAIL!! duration = 0\n");
		    LOG_DEBUG("sdio rx evt frame size =%d,duration=%f (sec)\n",len,duration);
		}
		else
			LOG_DEBUG("FAIL!! finishTime\n");
    }
}

#ifndef WIN32
unsigned int iseed;
struct timeval tv;
static void randomrandom(u8 *pointer,u16 repeat)
{
	unsigned int result=0,i;

	for(i=0;i<repeat;i++)
	{
		gettimeofday(&tv,NULL);
		iseed += tv.tv_usec;
		srand(iseed+result);
		result = rand();
		memcpy(&pointer[i*4],&result,4);
		usleep(10);
	}
}
#endif

extern u32	sdio_rx_count;
void _send(u32 len)
{
	u32 i=0;
#ifdef WIN32
	clock_t send_start,send_finish;
#else
	struct timeval tim;  
	double t1,t2; 
#endif
	double dur=0,tx_speed= 0,rx_speed= 0;

	sdio_rx_count = 0;
#ifdef WIN32
	send_start = clock();
#else
	gettimeofday(&tim, NULL);  
	t1=tim.tv_sec*1000+tim.tv_usec/1000; 
#endif
	{
		//LOG_DEBUG("_send frame count=%d\n",i);
		if(randomLengthMode)
		{
			for(i=0;i<testTxFrameTimes;i++)
			{
#ifdef WIN32
				len = rand();
				len = (len % 2240) + 64;
#else
				randomrandom((u8 *)&len,4);
				len = (len % 2240) + 64;
#endif
				cmd_test_send_data_frame(i,len);
			}
		}
		else
			cmd_test_send_data_frame(i,len);
	}
#ifdef WIN32
	send_finish=clock();
	dur = ((double)(send_finish-send_start)/CLOCKS_PER_SEC);
	tx_speed =  ((len*testTxFrameTimes)>>10)/dur;
	rx_speed =  ((len*sdio_rx_count)>>10)/dur;
	LOG_DEBUG("sdio tx data frame size=%d, tx_speed=%f(KB/sec)\n",len,tx_speed);
	LOG_DEBUG("sdio rx data frame size=%d, rx_speed=%f(KB/sec)\n",len,rx_speed);
	LOG_DEBUG("TX Frame rate=%f(sec)\n",testTxFrameTimes/dur);
	LOG_DEBUG("RX Frame rate=%f(sec)\n",sdio_rx_count/dur);
	LOG_DEBUG("Duration time %f (sec)\n",dur);
#else
	gettimeofday(&tim, NULL);  
	t2=tim.tv_sec*1000+tim.tv_usec/1000;
	dur = (t2-t1)/1000;
	tx_speed =  ((len*testTxFrameTimes)>>10)/dur;
	rx_speed =  ((len*sdio_rx_count)>>10)/dur;
	LOG_DEBUG("sdio tx data frame size=%d, tx_speed=%f(KB/sec)\n",len,tx_speed);
	LOG_DEBUG("sdio rx data frame size=%d, rx_speed=%f(KB/sec)\n",len,rx_speed);
	LOG_DEBUG("TX Frame rate=%f(sec)\n",testTxFrameTimes/dur);
	LOG_DEBUG("RX Frame rate=%f(sec)\n",sdio_rx_count/dur);
	LOG_DEBUG("Duration time %f (sec)\n",dur);
#endif
}


u8 SDIO_BSS_MAC_ADDR[ETH_ALEN];
u8 HOST_MAC_ADDR[ETH_ALEN];
u8 BROADCAST_ADDR[ETH_ALEN]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

void cmd_test_send_data_frame(u16 seq,u16 size)
{	
	struct ieee80211_qos_hdr*pHeader;		
	PKT_TxInfo *frame;
	u8 *raw;			
	u32 len,i;
	struct ieee80211_qos_hdr header={0};
	u32 fc;
    int mac_0,mac_1,bssid_0,bssid_1;

	//STA mode
	if(throughput_opmode == OP_MODE_STA)
	{
	    if(isQos)
        {
            fc = 0x0188;
        }
        else
        {
            
		    fc = 0x0108;
        }
	}
	//AP mode
	else if(throughput_opmode == OP_MODE_AP)
	{
		if(isQos)
        {
            fc = 0x0288;
        }
        else
        {
		    fc = 0x0208;
        }
	}
	else
	{
		LOG_DEBUG("throughput_opmode not valid!!\n");
		return;
	}

	frame = os_frame_alloc(size);
	raw = OS_FRAME_GET_DATA(frame);

	len = sizeof(header);

	pHeader = &header;	
	pHeader->frame_control = cpu_to_le16(fc);
	pHeader->duration_id = 0;

	mac_0 = ssv6xxx_drv_read_reg(ADR_STA_MAC_0);
    mac_1 = ssv6xxx_drv_read_reg(ADR_STA_MAC_1);

   
    memcpy(&HOST_MAC_ADDR[0],(u8 *)&mac_0,4);
    memcpy(&HOST_MAC_ADDR[4],(u8 *)&mac_1,2);

    bssid_0 = ssv6xxx_drv_read_reg(ADR_BSSID_0);
    bssid_1 = ssv6xxx_drv_read_reg(ADR_BSSID_1);

    memcpy(&SDIO_BSS_MAC_ADDR[0],(u8 *)&bssid_0,4);
    memcpy(&SDIO_BSS_MAC_ADDR[4],(u8 *)&bssid_1,2);
    
    
	if(IS_TODS_SET(fc))
	{
	    
		memcpy(pHeader->addr2, HOST_MAC_ADDR, ETH_ALEN);
       	if(isQos || ack_policy == 0)
		    memcpy(pHeader->addr1, SDIO_BSS_MAC_ADDR, ETH_ALEN);
        else
            memcpy(pHeader->addr1, BROADCAST_ADDR, ETH_ALEN);
        if(isQos || ack_policy == 0)
		    memcpy(pHeader->addr3, SDIO_BSS_MAC_ADDR, ETH_ALEN);
        else
            memcpy(pHeader->addr3, BROADCAST_ADDR, ETH_ALEN);
	}
	else
	{

       //need to modify by qos??
		memcpy(pHeader->addr1, HOST_MAC_ADDR, ETH_ALEN);
		memcpy(pHeader->addr2, SDIO_BSS_MAC_ADDR, ETH_ALEN);
		memcpy(pHeader->addr3, SDIO_BSS_MAC_ADDR, ETH_ALEN);
	}

	{
		//u16 seq_ctrl ;
		//seq_ctrl = seq;
		pHeader->seq_ctrl= cpu_to_le16(seq);	
	}
	//add the qos
	if (IS_QOS_DATA(fc) || IS_QOS_NULL_DATA(fc)) {
		pHeader->qos_ctrl = cpu_to_le16(qos);			
	}	
	else
	{
		len=len-2;
	}

	//add ht
	if(IS_ORDER_SET(fc) && IS_MGMT_FRAME(fc) )
	{
		
	}
	memcpy(raw, &header, len);

	//fill the LLC
    {
        u8 _802_1h[]={ 0xAA, 0xAA, 0x03, 0x00, 0x00, 0xF8 };
        memcpy(raw+len,_802_1h,6);
    }
   
    //fiil ether type
	raw[len+6] =0x00;
	raw[len+7]=0x08;

	//fill the packet	
	len+=8;
	{
		OS_FRAME_SET_DATA_LEN(frame, size);
		TxHdl_prepare_wifi_txreq(frame, size, TRUE, 0);

		if(randomLengthMode)
			ssv6xxx_drv_send(OS_FRAME_GET_DATA(frame), OS_FRAME_GET_DATA_LEN(frame));
		else
		{
			for(i=0;i<testTxFrameTimes;i++)
			{
				ssv6xxx_drv_send(OS_FRAME_GET_DATA(frame), OS_FRAME_GET_DATA_LEN(frame));
			}
		}
	}
	os_frame_free(frame);

	//while(ssv6xxx_wifi_send_80211(frame,size-frame->hdr_offset)==SSV6XXX_NO_MEM);
	
}

SDIO_THROUGHPUT_COMMAND	sdioSettingCommand;
void set_soc_trigger_rx_frame(u32 size,u32 testTimes,u32 h_cmd)
{
	sdioSettingCommand.transferLength = size;
	sdioSettingCommand.transferCount = testTimes;
	ssv6xxx_wifi_ioctl( h_cmd,
		&sdioSettingCommand, 
		sizeof(SDIO_THROUGHPUT_COMMAND)
	);
    //soc_cmd_send(buffer, len);
}
u8 wait_flag = 0;


extern u32	sdio_rx_count;
extern u8	sdio_rx_verify_mode;

void cmd_sdio_test(s32 argc, char *argv[])
{
	u32 loopTimes;
	rx_count = 0;
	randomLengthMode = 0;
    if(argc ==3)
    {
        if(strcmp(argv[1], "qos") == 0)
        {
            qos = ssv6xxx_atoi_base(argv[2],16);
            isQos = true;
        }
        else if(strcmp(argv[1], "nqos") == 0)
        {
            isQos= false;
            qos = 0;
            ack_policy = ssv6xxx_atoi(argv[2]);
        }
         
        _ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_SET_QOS, (void *)&qos, sizeof(u16)*2,,TRUE);
        return;
        
    }
	if (argc == 4)
	{
		hostTxFrameSize =  atoi(argv[2]);
		if(hostTxFrameSize == 9999)
		{
#ifndef WIN32
			//Random init
			iseed = (unsigned int)time(NULL);
			gettimeofday(&tv,NULL);
			iseed += tv.tv_usec;
			srand(iseed);
#endif
			randomLengthMode = 1;
			log_printf("Random mode!!\n");
		}
		else if((hostTxFrameSize < 63) && (hostTxFrameSize > 2304))
		{
			log_printf("Error Size!!\n");
			return;
		}
		
		loopTimes = atoi(argv[3]);

		if(strcmp(argv[1], "loop") == 0)		
			return;

		if(strcmp(argv[1], "tx") == 0 )
		{
			testTxFrameTimes = loopTimes;
			_send(hostTxFrameSize);
		}	
		else if(strcmp(argv[1], "rx-hci") == 0)
		{
			testRxFrameTimes = loopTimes;
			set_soc_trigger_rx_frame(hostTxFrameSize,testRxFrameTimes,SSV6XXX_HOST_CMD_HCI2SDIO);
		}
		else if(strcmp(argv[1], "rx-mic") == 0)
		{
			testRxFrameTimes = loopTimes;
			set_soc_trigger_rx_frame(hostTxFrameSize,testRxFrameTimes,SSV6XXX_HOST_CMD_MIC2SDIO);
		}
		else if(strcmp(argv[1], "rx-sec") == 0)
		{
			testRxFrameTimes = loopTimes;
			set_soc_trigger_rx_frame(hostTxFrameSize,testRxFrameTimes,SSV6XXX_HOST_CMD_SEC2SDIO);
		}
		else if(strcmp(argv[1], "duplex") == 0)
		{
            testRxFrameTimes = loopTimes;
            testTxFrameTimes = loopTimes;
			set_soc_trigger_rx_frame(hostTxFrameSize,testRxFrameTimes*6,SSV6XXX_HOST_CMD_HCI2SDIO);
			_send(hostTxFrameSize);           			
		}
		else
			log_printf("Error parameter!!\n");

		return;
		
	}		
	else if (argc == 5 )
	{
		hostTxFrameSize =  atoi(argv[2]);
		if(hostTxFrameSize == 9999)
		{
#ifndef WIN32
			//Random init
			iseed = (unsigned int)time(NULL);
			gettimeofday(&tv,NULL);
			iseed += tv.tv_usec;
			srand(iseed);
#endif
			randomLengthMode = 1;
			log_printf("Random mode!!\n");
		}
		else if((hostTxFrameSize < 63) && (hostTxFrameSize > 2304))
		{
			log_printf("Error Size!!\n");
			return;
		}

		loopTimes = atoi(argv[4]);
        
		 if(strcmp(argv[1], "loop") == 0)
		 {
			if(strcmp(argv[3], "mac") == 0)
			{
				testRxFrameTimes = loopTimes;
                testTxFrameTimes = loopTimes;
			    _send(hostTxFrameSize);
				
			}
			else if(strcmp(argv[3], "phy") == 0)
			{
				//sdio_write_reg(ADR_TX_FLOW_1,0x00000061); //set hci data flow
				//sdio_write_reg(ADR_RX_FLOW_DATA,0x00000014); //set mar-rx data flow
				//sdio_write_reg(ADR_MAC_MODE,0x00000004);//set loopback by phy
				_send(hostTxFrameSize);
			}
			else
				log_printf("Error parameter!!\n");
		}
		else
			log_printf("Error parameter!!\n");
		 return;
	}
	else if (argc == 3 )
	{
		if(strcmp(argv[1], "rx") == 0)
		{
			if(atoi(argv[2]))
			{
				log_printf("SDIO rx verify on!\n");
				sdio_rx_count=0;
				sdio_rx_verify_mode=1;
			}
			else
			{
				log_printf("SDIO rx verify off!\n");
				sdio_rx_verify_mode=0;
			}
			return;
		}
	}
	
	log_printf("\nUsage: sdio [tx/rx-hci/rx-sec/rx-mic/duplex] [64-2304] [send frames]\n");
	log_printf("              Size [9999] random length mode\n");
	log_printf("Usage: sdio [loop] [64-2304] [mac/phy] [times(1-n)]\n\n");
	log_printf("Verify rx throughput: sdio rx [0-1]\n\n");
	return;
	
}
#endif	

