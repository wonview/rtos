#define __SFILE__ "ap.c"




#include <log.h>
#include <config.h>

#include <types.h>
#include <msgevt.h>
#include <hdr80211.h>
#include <pbuf.h>






#include "ap_config.h"

#include "ap.h"
#include <os_wrapper.h>
#include "common/ether.h"
#include "ap_sta_info.h"
#include "ap_info.h"

#include "common/bitops.h"
//#include "common/bitmap.h"

#include <host_apis.h>

#include <ethertype.h>
#include "common/ieee802_11_defs.h"
#include "common/ieee80211.h"

//mgmt function
#include "ap_mlme.h"
#include <common.h>
#include "ap_rx.h"

#include <cmd_def.h>
#include "ap_drv_cmd.h"
#include "dev.h"
#include "ssv_dev.h"

#include "regs/ssv6200_aux.h"
#include "regs/ssv6200_reg.h"



extern void AP_RxHandleAPMode(struct cfg_host_rxpkt *pPktInfo);
extern void StartBeacon(void);
extern void StopBeacon(void);
extern ssv6xxx_data_result AP_RxHandleData(void *frame);
extern void sta_info_cleanup(void *data1, void *data2);

#ifdef __TEST_DATA__
extern void TestCase_AddAPSta(void);
#endif



s32 AP_Start( void );
s32 AP_Stop( void );
void host_cmd_cb_handler(ssv6xxx_ap_cmd ap_cmd, ssv6xxx_result result);

//--------------------------------------------------
#define pb_offset 80
//=====>ADR_MTX_BCN_EN_MISC

#define MTX_BCN_PKTID_CH_LOCK_SHIFT MTX_BCN_PKTID_CH_LOCK_SFT 	
#define MTX_BCN_CFG_VLD_SHIFT MTX_BCN_CFG_VLD_SFT 
#define MTX_BCN_CFG_VLD_MASK  MTX_BCN_CFG_VLD_MSK 

#define MAX_FAIL_COUNT          100
#define MAX_RETRY_COUNT         20

#define PBUF_BASE_ADDR	            0x80000000
#define PBUF_ADDR_SHIFT	            16

#define PBUF_MapPkttoID(_PKT)		(((u32)_PKT&0x0FFF0000)>>PBUF_ADDR_SHIFT)	
#define PBUF_MapIDtoPkt(_ID)		(PBUF_BASE_ADDR|((_ID)<<PBUF_ADDR_SHIFT))

#define MTX_BCN_TIMER_EN_SHIFT				MTX_BCN_TIMER_EN_SFT															//	0
#define MTX_TSF_TIMER_EN_SHIFT				MTX_TSF_TIMER_EN_SFT															//	5
#define MTX_HALT_MNG_UNTIL_DTIM_SHIFT 		MTX_HALT_MNG_UNTIL_DTIM_SFT	//6
#define MTX_INT_DTIM_NUM_SHIFT 		        MTX_INT_DTIM_NUM_SFT	//8
#define MTX_EN_INT_Q4_Q_EMPTY_SHIFT 		MTX_EN_INT_Q4_Q_EMPTY_SFT	//24



#define MTX_BCN_ENABLE_MASK					(MTX_BCN_TIMER_EN_I_MSK)  	//0xffffff9e
#define MTX_TSF_ENABLE_MASK					(MTX_TSF_TIMER_EN_I_MSK)  	//0xffffff9e


//=====>ADR_MTX_BCN_CFG0/ADR_MTX_BCN_CFG1
#define MTX_DTIM_OFST0			MTX_DTIM_OFST0_SFT


//=====>ADR_MTX_BCN_PRD
#define MTX_BCN_PERIOD_SHIFT 	MTX_BCN_PERIOD_SFT		// 0			//bit0~7
#define MTX_DTIM_NUM_SHIFT 		MTX_DTIM_NUM_SFT		// 24			//bit 24~bit31


//=====>ADR_CH0_TRIG_1
#define ADDRESS_OFFSET            16                    //16
#define HW_ID_OFFSET              7

//=====>ADR_MCU_STATUS
#define CH0_FULL_MASK             CH0_FULL_MSK              //0x00000001
//=====>

enum ssv6xxx_beacon_type{
	SSV6xxx_BEACON_0,
	SSV6xxx_BEACON_1,
};

void ssv6xxx_beacon_reg_lock(bool block)
{
	u32 val;
	
	val = block<<MTX_BCN_PKTID_CH_LOCK_SHIFT;
	MAC_REG_WRITE(ADR_MTX_BCN_MISC, val);
}





//-------------------------------------------------


//must be data/mgmt frame
ssv6xxx_data_result AP_RxHandleFrame(void *frame)
{
	struct cfg_host_rxpkt *pPktInfo = (struct cfg_host_rxpkt *)OS_FRAME_GET_DATA(frame);
	ssv6xxx_data_result ret = SSV6XXX_DATA_ACPT;
	

	if(pPktInfo->f80211)//MGMT, control	
    {   

        u8 *raw = ssv6xxx_host_rx_data_get_data_ptr(pPktInfo);
        u16 fc = (raw[1]<<8) | raw[0];

        //ps poll data send to ap handle ps function, execute on rx task
        if ( WLAN_FC_GET_TYPE(fc) == WLAN_FC_TYPE_CTRL )
        {
            if( !(fc & WLAN_FC_RETRY)&& 
                WLAN_FC_GET_STYPE(fc)== WLAN_FC_STYPE_PSPOLL)
                ap_handle_ps(frame);
            else
                os_frame_free(frame);
        }
        else
        {    
#if(MLME_TASK ==1)
            ap_mlme_handler(frame,MEVT_PKT_BUF);//send to mlme task
#else
            void *msg = os_msg_alloc();
            ASSERT(msg);
            os_msg_send(msg, frame);

        //AP_RxHandleAPMode(frame);	
#endif	
        }

    }
	else
    {   
		ret = AP_RxHandleData(frame);
    }
	return ret;
}


//ssv6xxx_data_result AP_RxDataCB (void *data, u32 len)
//{
//	return AP_RxHandleFrame((struct cfg_host_rxpkt *)data);
//}

extern void ap_tx_dtimexpiry_event();


//extern void BeaconGen(void *data1, void *data2);

//Could be null data event, or command response we call before
void AP_RxHandleEvent(void *frame)
{
    struct cfg_host_event *pPktInfo = (struct cfg_host_event *)OS_FRAME_GET_DATA(frame);
    struct ap_rx_desp rx_desp;
    MEMSET(&rx_desp, 0, sizeof(struct ap_rx_desp));

	//LOG_TRACE( "Enter AP_RxHandleEvent pHostEvt->c_type %d\n", pHostEvt->c_type);

    if(pPktInfo->c_type == HOST_EVENT)
    {
        if(pPktInfo->h_event == SOC_EVT_NULL_DATA)
        {
            struct cfg_null_data_info *null_data_info = (struct cfg_null_data_info *)(pPktInfo+1);

            
            //Frame type, PM/QoS bit/UP
            rx_desp.flags |= AP_RX_FLAGS_FRAME_TYPE_NULL_DATA;              
            rx_desp.flags |= (null_data_info->Flags& HOST_EVT_NULL_DATA_PM)?AP_RX_FLAGS_PM_BIT:0;
            
            
            rx_desp.sta = APStaInfo_FindStaByAddr(&null_data_info->SAAddr);
            if(rx_desp.sta)
            {            
                OS_MutexLock(rx_desp.sta->apsta_mutex);
                rx_desp.sta->arp_retry_count = 0;
                OS_MutexUnLock(rx_desp.sta->apsta_mutex);
                rx_desp.flags |= (rx_desp.sta->_flags&WLAN_STA_WMM)?AP_RX_FLAGS_QOS_BIT:0;
            }       
            //rx_desp.data = (void*)pHostEvt;
            rx_desp.UP = null_data_info->Priority;
            ssv6xxx_data_need_to_be_received(&rx_desp);          

        }    
    }
    else if (pPktInfo->c_type == M0_RXEVENT)
    {
        struct cfg_host_rxpkt *pPktInfo = (struct cfg_host_rxpkt *)OS_FRAME_GET_DATA(frame);
        u8 *raw = ssv6xxx_host_rx_data_get_data_ptr(pPktInfo);        
        struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)(raw);

                        
        //Frame type, PM/QoS bit/UP             
        rx_desp.flags |= AP_RX_FLAGS_FRAME_TYPE_PS_POLL;

        rx_desp.sta = APStaInfo_FindStaByAddr((ETHER_ADDR *)mgmt->sa);

      
        if(rx_desp.sta)
        {
            OS_MutexLock(rx_desp.sta->apsta_mutex);
            rx_desp.sta->arp_retry_count = 0;
            OS_MutexUnLock(rx_desp.sta->apsta_mutex);
            rx_desp.flags |= (rx_desp.sta->_flags&WLAN_STA_WMM)?AP_RX_FLAGS_QOS_BIT:0;
        }                   
        
        ssv6xxx_data_need_to_be_received(&rx_desp);     
 
    }        
    else
    {
        LOG_TRACE("not accept frame type [type:%x]\r\n",pPktInfo->c_type);        
    }

    
	

	

	//FREEIF(pHostEvt);
	
}





// void AP_HandleQueue(PKT_RxInfo *pPktInfo)
// {
// 
// 	//event/frames(data/ctrl/mgmt)/cmd
// 	switch (pPktInfo->c_type)
// 	{
// 		
// 	//-------------
// 	//RX
// 		case M0_RXEVENT:		
// 		//case M1_RXEVENT:
// 			AP_RxHandleFrame((struct cfg_host_rxpkt *)pPktInfo);
// 			break;
// 		
// 
// 		case HOST_EVENT:
// 			AP_RxHandleEvent((struct cfg_host_event *)pPktInfo);
// 			break;			
// 	//-------------
// 	//TX
// 		case HOST_CMD:
// 			AP_TxHandleCmd((struct cfg_host_cmd *)pPktInfo);
// 			break;
// 
// 
// 		default:
// 			LOG_WARN( "Unexpect c_type %d appeared", pPktInfo->c_type);
// 			ASSERT(0);
// 	}
// 
// }








void APInfoInit()
{

	struct cfg_host_txreq0 *req;
//	struct cfg_host_cmd *cmd;

    OS_MutexInit(&gDeviceInfo->APInfo->ap_info_ps_mutex);

//------------------------------
	gDeviceInfo->APInfo->pOSMgmtframe = os_frame_alloc(AP_MGMT_PKT_LEN);
	ASSERT(NULL != gDeviceInfo->APInfo->pOSMgmtframe);	
	gDeviceInfo->APInfo->pMgmtPkt = OS_FRAME_GET_DATA(gDeviceInfo->APInfo->pOSMgmtframe);
	req = os_frame_push(gDeviceInfo->APInfo->pOSMgmtframe, sizeof(struct cfg_host_txreq0));
	MEMSET(req, 0, sizeof(struct cfg_host_txreq0));	
	req->c_type = M0_TXREQ;
	req->f80211 = 1;

	
//------------------------------
//Beacon

//Send beacon by data path
    gDeviceInfo->APInfo->pBeaconHead = (u8 *)gDeviceInfo->APInfo->bcn + pb_offset;
    gDeviceInfo->enable_beacon=FALSE;


	gDeviceInfo->APInfo->pBeaconTail= MALLOC(AP_BEACON_TAIL_BUF_SIZE);
	ASSERT(NULL != gDeviceInfo->APInfo->pBeaconTail);



//----------------------
//timer
    
	gDeviceInfo->APInfo->sta_pkt_cln_timer =FALSE;
	


}

extern void os_timer_init(ApInfo_st *pApInfo);


void AP_Config_HW(void)
{

   

}
extern void APTxDespInit();

s32 AP_Init( void )
{
	
// 		struct cfg_host_event _pHostEvt={0};
// 		struct cfg_host_event *pHostEvt =&_pHostEvt;
// 		struct cfg_ps_poll_info *ps_poll_info = (struct cfg_ps_poll_info *)(pHostEvt+1);

//	LOG_TRACE( "Enter AP_Init \n");

	//Belong to cmd engine
	{
		s32 size;

		//Create cmd table
		/*  Allocate Memory for STA data structure. */
		size = sizeof(ApInfo_st);
		gDeviceInfo->APInfo = MALLOC((u32)size);
		ASSERT_RET(gDeviceInfo->APInfo != NULL, OS_FAILED);
		MEMSET(gDeviceInfo->APInfo, 0, (u32)size);
		
		//os_timer_init(gAPInfo);
	}
	


	APInfoInit();

	
	ssv6xxx_config_init(&gDeviceInfo->APInfo->config);
	//Resource allocate
	APTxDespInit();

	

	
	
	APStaInfo_Init();


//-----------------------------------------------

	gDeviceInfo->APInfo->eCurrentApMode	= gDeviceInfo->APInfo->config.eApMode;
	gDeviceInfo->APInfo->b80211n			= gDeviceInfo->APInfo->config.b80211n;
	gDeviceInfo->APInfo->nCurrentChannel	= gDeviceInfo->APInfo->config.nChannel;

//-----------------------------------------------
//Security
	gDeviceInfo->APInfo->auth_algs = WPA_AUTH_ALG_OPEN|WPA_AUTH_ALG_SHARED;

	
//----------------------------------------
//Fill some fake data
    MEMCPY(gDeviceInfo->APInfo->own_addr,gDeviceInfo->self_mac,6);	

//----------------------------------------

	//Get related data from HW

#if 0
	if (AP_MODE_IEEE80211B == gAPInfo->eCurrentApMode)
	{
		
	}
	else
	{
		//Set rate
	}
#endif

	//Set beacon
   	AP_Config_HW();

	


//	LOG_TRACE( "Leave AP_Init\n");
    return OS_SUCCESS;
}




void AP_InfoReset()
{

	
	
	MEMSET(gDeviceInfo->APInfo->password, 0, MAX_PASSWD_LEN+1);
	//SSID
	MEMSET(gDeviceInfo->APInfo->config.ssid, 0, IEEE80211_MAX_SSID_LEN+1);	
	gDeviceInfo->APInfo->config.ssid_len = 0;

	gDeviceInfo->APInfo->auth_algs = 0;
	gDeviceInfo->APInfo->tkip_countermeasures = 0;
	gDeviceInfo->APInfo->sec_type=0;
	MEMSET(gDeviceInfo->APInfo->password, 0, MAX_PASSWD_LEN+1);

}





s32 AP_Config(struct cfg_set_ap_cfg *ap_cfg)
{

	//SEC
	gDeviceInfo->APInfo->sec_type = ap_cfg->sec_type;
	
	//password
	MEMCPY((void*)gDeviceInfo->APInfo->password, (void*)ap_cfg->password, strlen((void*)ap_cfg->password));
	

	//SSID
	MEMCPY((void*)gDeviceInfo->APInfo->config.ssid, (void*)ap_cfg->ssid.ssid, ap_cfg->ssid.ssid_len);
	gDeviceInfo->APInfo->config.ssid[ap_cfg->ssid.ssid_len]=0;
	gDeviceInfo->APInfo->config.ssid_len = ap_cfg->ssid.ssid_len;



		
	return 0;
}

void ssv6xxx_beacon_fill_tx_desc(u8 bcn_len, void *frame)
{
    struct ssv6200_tx_desc *tx_desc = (struct ssv6200_tx_desc *)frame;

	//length
    tx_desc->len            = bcn_len;
	tx_desc->c_type         = M2_TXREQ;
    tx_desc->f80211         = 1;
	tx_desc->ack_policy     = 1;//no ack;	
    tx_desc->hdr_offset 	= pb_offset;					
    tx_desc->hdr_len 		= 24;									
    tx_desc->payload_offset = tx_desc->hdr_offset + tx_desc->hdr_len;


}
// 00: bcn cfg0 / cfg1 not be config 01: cfg0 is valid 10: cfg1 is valid 11: error (valid cfg be write) 
inline enum ssv6xxx_beacon_type ssv6xxx_beacon_get_valid_reg()
{
	u32 regval =0;
	MAC_REG_READ(ADR_MTX_BCN_MISC, regval);

	
	regval &= MTX_BCN_CFG_VLD_MASK;
	regval = regval >>MTX_BCN_CFG_VLD_SHIFT;



	//get MTX_BCN_CFG_VLD
	
	if(regval==0x2 || regval == 0x0)//bcn 0 is availabke to use.
		return SSV6xxx_BEACON_0;
	else if(regval==0x1)//bcn 1 is availabke to use.
		return SSV6xxx_BEACON_1;
	else
		LOG_PRINTF("=============>ERROR!!drv_bcn_reg_available\n");//ASSERT(FALSE);// 11 error happened need check with ASIC.

		
	return SSV6xxx_BEACON_0;
}

u32 ssv6xxx_pbuf_alloc(int size, int type)
{
    u32 regval, pad;
    int cnt = MAX_RETRY_COUNT;
    //int page_cnt = (size + ((1 << HW_MMU_PAGE_SHIFT) - 1)) >> HW_MMU_PAGE_SHIFT;

    regval = 0;

    //mutex_lock(&sc->mem_mutex);
    
    //brust could be dividen by 4
    pad = (u32)size%4;
    size += (int)pad;

    do{
        //printk("[A] ssv6xxx_pbuf_alloc\n");

        MAC_REG_WRITE(ADR_WR_ALC, (size | (type << 16)));
        MAC_REG_READ(ADR_WR_ALC, regval);
        
        if (regval == 0) {
            cnt--;
            //msleep(1);
            OS_MsDelay(50);
        }
        else
            break;
                
    } while (cnt);

    // If TX buffer is allocated, AMPDU maximum size m
    /*
        if (type == TX_BUF)
        {
            sc->sh->tx_page_available -= page_cnt;
            sc->sh->page_count[PACKET_ADDR_2_ID(regval)] = page_cnt;
        }
        */
    //mutex_unlock(&sc->mem_mutex);

   
    return regval;


}
int ssv6xxx_beacon_fill_content(u32 regaddr, u8 *beacon, int size)
{
	int i;
    u32 val;
	u32 *ptr = (u32*)beacon;
	size = size/4;
    //size = size*4;

	for(i=0; i<size; i++)
	{
		val = (u32)(*(ptr+i));
		
		MAC_REG_WRITE(regaddr+(u32)i*4, val);
	}

    return 0;
}

//0-->success others-->fail
bool ssv6xxx_beacon_enable( bool bEnable)
{

    u32 regval=0;
    int ret = 0;

    //If there is no beacon set to register, beacon could not be turn on.
    if(bEnable && !gDeviceInfo->beacon_usage)
    {
        LOG_PRINTF("[A] Reject to set beacon!!!.        ssv6xxx_beacon_enable bEnable[%d] sc->beacon_usage[%d]\n",bEnable ,gDeviceInfo->beacon_usage);
        gDeviceInfo->enable_beacon = FALSE;
        return 0;
    }

    if((bEnable && (gDeviceInfo->enable_beacon))||
        (!bEnable && !gDeviceInfo->enable_beacon))
    {
        LOG_PRINTF("[A] ssv6xxx_beacon_enable bEnable[%d] and sc->enable_beacon[%d] are the same. no need to execute.\n",bEnable ,gDeviceInfo->enable_beacon);
        //return -1;
        if(bEnable){
            LOG_PRINTF("        Ignore enable beacon cmd!!!!\n");
            return 0;
        }
    }
    
    MAC_REG_READ(ADR_MTX_BCN_EN_MISC, regval);
    regval&= MTX_BCN_ENABLE_MASK;
	regval|=(bEnable<<MTX_BCN_TIMER_EN_SHIFT)|
	(bEnable<<MTX_TSF_TIMER_EN_SHIFT) |
	(bEnable<<MTX_HALT_MNG_UNTIL_DTIM_SHIFT)|
	(bEnable<<MTX_INT_DTIM_NUM_SHIFT);
	MAC_REG_WRITE(ADR_MTX_BCN_EN_MISC, regval);

    MAC_REG_READ(ADR_MTX_INT_EN, regval);
    regval&= MTX_EN_INT_Q4_Q_EMPTY_I_MSK;
	regval|=(bEnable<<MTX_EN_INT_Q4_Q_EMPTY_SHIFT);
	MAC_REG_WRITE(ADR_MTX_INT_EN, regval);

    gDeviceInfo->enable_beacon = bEnable;

    return ret;
		
}
void ssv6xxx_beacon_set_info(u8 beacon_interval, u8 dtim_cnt)
{
	u32 val;

	//if default is 0 set to our default
	if(beacon_interval==0)
		beacon_interval = 100;

	val = (beacon_interval<<MTX_BCN_PERIOD_SHIFT)| (dtim_cnt<<MTX_DTIM_NUM_SHIFT);
	MAC_REG_WRITE( ADR_MTX_BCN_PRD, val);
}
inline bool ssv6xxx_mcu_input_full()
{
    u32 regval=0;
    MAC_REG_READ(ADR_MCU_STATUS, regval);
    return CH0_FULL_MASK&regval;
}

bool ssv6xxx_pbuf_free(u32 pbuf_addr)
{
    u32  regval=0;
    u16  failCount=0;
   
    while (ssv6xxx_mcu_input_full())
    {
        if (failCount++ < 1000) 
            continue;
        LOG_PRINTF("=============>ERROR!!MAILBOX Block[%d]\n", failCount);
        return false;
    } //Wait until input queue of cho is not full.

    // {HWID[3:0], PKTID[6:0]}
    regval = ((M_ENG_TRASH_CAN << HW_ID_OFFSET) |(pbuf_addr >> ADDRESS_OFFSET));

    MAC_REG_WRITE(ADR_CH0_TRIG_1, regval);

    
    return true;
}


//Assume Beacon come from one sk_buf
//Beacon
bool ssv6xxx_beacon_set(void* beacon_skb, int dtim_offset)
{
	u32 reg_tx_beacon_adr = ADR_MTX_BCN_CFG0;
    struct ssv6200_tx_desc *tx_desc = (struct ssv6200_tx_desc *)beacon_skb;
    int size = tx_desc->len+ pb_offset;
	enum ssv6xxx_beacon_type avl_bcn_type = SSV6xxx_BEACON_0;
	bool ret = true;
	int val;
    //u32 pubf_addr;
	ssv6xxx_beacon_reg_lock(1);

	//1.Decide which register can be used to set
	avl_bcn_type = ssv6xxx_beacon_get_valid_reg();
	if(avl_bcn_type == SSV6xxx_BEACON_1)
		reg_tx_beacon_adr = ADR_MTX_BCN_CFG1;


	//2.Get Pbuf from ASIC
	do{		
		if((gDeviceInfo->beacon_usage) & (0x01<<(avl_bcn_type) ))
		{
			if (gDeviceInfo->APInfo->hw_bcn_info[avl_bcn_type].len >= size)
			{
				break;
			}
			else
			{
				//old beacon too small, need to free
				if(false == ssv6xxx_pbuf_free( gDeviceInfo->APInfo->hw_bcn_info[avl_bcn_type].pubf_addr))
				{
					ret = false;
					goto out;
				}			                
                (gDeviceInfo->beacon_usage) &= ~(0x01<<avl_bcn_type);				
			}			
		}


		//Allocate new one
		gDeviceInfo->APInfo->hw_bcn_info[avl_bcn_type].pubf_addr = ssv6xxx_pbuf_alloc(size, RX_BUF);
		gDeviceInfo->APInfo->hw_bcn_info[avl_bcn_type].len = size;

		//if can't allocate beacon, just leave.
		if(gDeviceInfo->APInfo->hw_bcn_info[avl_bcn_type].pubf_addr == 0)
		{
			ret = false;
			goto out;
		}
						
		//Indicate reg is stored packet buf.
		(gDeviceInfo->beacon_usage) |= (0x01<<avl_bcn_type);

	}while(0);


	//3. Write Beacon content.
	ssv6xxx_beacon_fill_content( gDeviceInfo->APInfo->hw_bcn_info[avl_bcn_type].pubf_addr, beacon_skb, size);

	//4. Assign to register let tx know. Beacon is updated.	
	val = (PBUF_MapPkttoID(gDeviceInfo->APInfo->hw_bcn_info[avl_bcn_type].pubf_addr))|(dtim_offset<<MTX_DTIM_OFST0);	
	MAC_REG_WRITE( reg_tx_beacon_adr,  (u32)val);

out:
	ssv6xxx_beacon_reg_lock(0);     

    if(ret)
        ssv6xxx_beacon_set_info(AP_DEFAULT_BEACON_INT,AP_DEFAULT_DTIM_PERIOD-1);

    if(ret&&gDeviceInfo->beacon_usage && (!gDeviceInfo->enable_beacon)){
        LOG_PRINTF("[A] enable beacon for BEACON_WAITING_ENABLED flags\n");
        ssv6xxx_beacon_enable(true);        
    }
    

	return ret;
}

s32 AP_Start( void )
{		
    neighbor_ap_list_init(gDeviceInfo->APInfo);
    //GenBeacon
	StartBeacon();
	ap_soc_set_bcn(SSV6XXX_SET_INIT_BEACON, gDeviceInfo->APInfo->bcn, &gDeviceInfo->APInfo->bcn_info, AP_DEFAULT_DTIM_PERIOD-1, AP_DEFAULT_BEACON_INT);
    

    return 1;
}


extern void APStaInfo_Release(void);
s32 AP_Stop( void )
{
	
	neighbor_ap_list_deinit(gDeviceInfo->APInfo);
	release_beacon_info();
	
	APStaInfo_Release();
	os_cancel_timer(sta_info_cleanup, 0, 0);
    OS_MutexLock(gDeviceInfo->APInfo->ap_info_ps_mutex);
    gDeviceInfo->APInfo->sta_pkt_cln_timer = FALSE;
    OS_MutexUnLock(gDeviceInfo->APInfo->ap_info_ps_mutex);

    ssv6xxx_beacon_enable(false); 

	
	return 1;
}



// void host_cmd_cb_handler(ssv6xxx_ap_cmd ap_cmd, ssv6xxx_result result)
// {
// 	gAPInfo->cb(ap_cmd, result);
// }



	
			

s32 AP_Command(ssv6xxx_ap_cmd cmd, APCmdCb cb, void *data)
{

// 	if (cmd == SSV6XXX_AP_ON)
// 	{
// 		struct cfg_host_cmd *pPktInfo = (struct hw_mode_st *)data;
// 		struct cfg_ioctl_request* ioctlreq = (struct cfg_ioctl_request*) pPktInfo->dat;
// 		struct hw_mode_st *hw_mode = &ioctlreq->u.hw_mode;
// 		u8 len = strlen(hw_mode->ssid);
// 
// 		//Set SSID
// 		memcpy(gAPInfo->config.ssid, hw_mode->ssid, len);			
// 		gAPInfo->config.ssid[len]='\0';
// 		gAPInfo->config.ssid_len = len;
// 
// 		//Set Password
// 		memcpy((void*)&gAPInfo->password, (void*)&hw_mode->password, strlen((const char*)&hw_mode->password));
// 		
// 
// 		//Set Security Type
// 		gAPInfo->sec_type = hw_mode->sec_type;//SSV6XXX_SEC_NONE, SSV6XXX_SEC_WEP_40, SSV6XXX_SEC_WEP_104...
// 
// 		AP_Start();
// 		gAPInfo->cb = cb;
// 	} 
// 	else
// 	{
// 		//AP off
// 		AP_Stop();
// 		gAPInfo->cb = cb;
// 	}

	return SSV6XXX_SUCCESS;
}
