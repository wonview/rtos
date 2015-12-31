#include <os_wrapper.h>
#include <dev.h>
#include "common/ieee802_11_defs.h"
#include "txrx_hdl.h"

struct qos_ctrl_st {
    u16                 tid:4;
    u16                 bit4:1;
    u16                 ack_policy:2;
    u16                 rsvd:1;
    u16                 bit8_15:8;
};

struct ht_ctrl_st {
	u32 				ht;
};


struct a4_ctrl_st {
	ETHER_ADDR			a4;
};


#define RXPKTFLT_NUM 5
OsMutex rxhdl_mtx;
u8  wifi_flt_num;
u8  eth_flt_num;
static struct wifi_flt rx_wifi_flt[RXPKTFLT_NUM];
static struct eth_flt rx_eth_flt[RXPKTFLT_NUM];

bool TxHdl_FrameProc(void *frame, bool apFrame, u32 priority, u32 flags);

u16 RxHdl_GetRawRxDataOffset(struct cfg_host_rxpkt *pPktInfo)
{
	u16 offset=0;

	do{
		switch(pPktInfo->c_type)
		{
			case M0_RXEVENT:
				offset = RX_M0_HDR_LEN;
				break;
			case M2_RXEVENT:
				ASSERT(FALSE);
				offset = M2_HDR_LEN;
				break;

			default:
				break;
		}

		//mac80211 no need to put extra header.
		if(pPktInfo->f80211)
			break;

		/*|(AL)|MAC|QOS|HT|*/
		if(pPktInfo->ht == 1)
			offset += IEEE80211_HT_CTL_LEN;

		if(pPktInfo->qos == 1)
			offset +=  IEEE80211_QOS_CTL_LEN;

		if(pPktInfo->use_4addr == 1)
			offset +=  ETHER_ADDR_LEN;

		if(pPktInfo->align2 == 1)
			offset +=  2;


	}while(0);



	return offset;
}


bool TxHdl_prepare_wifi_txreq(void *frame, u32 len, bool f80211, u32 priority, u8 tx_dscrp_flag)
{
	//s32 ret = SSV6XXX_SUCCESS;
	//s32 ret = SSV6XXX_SUCCESS;
	struct cfg_host_txreq0 *host_txreq0;
	struct ht_ctrl_st *ht_st;
	u8 *pos;
	bool qos = false, ht = false, use_4addr = false;
	u32 extra_len=0;
    u32 padding_len=0;

	//get info from
	if(gDeviceInfo->hw_mode == SSV6XXX_HWM_STA)
	{
		//STA mode
		qos = !!(IS_TXREQ_WITH_QOS(gDeviceInfo));
		ht = !!(IS_TXREQ_WITH_HT(gDeviceInfo));
		use_4addr = !!(IS_TXREQ_WITH_ADDR4(gDeviceInfo));
	}
	else if(gDeviceInfo->hw_mode == SSV6XXX_HWM_AP)
	{
		//AP mode
		//get data from station info(ap mode)


#if (BEACON_DBG == 0)
		if(!f80211)
		{
			//802.3

			//get  DA
 			ETHER_ADDR *mac = (ETHER_ADDR *)OS_FRAME_GET_DATA(frame);//
			if(FALSE == ap_sta_info_capability(mac , &ht, &qos, &use_4addr))
				return false;

		}

#else//#if (BEACON_DBG == 1)

		qos=ht=use_4addr=0;

#endif//#if (BEACON_DBG == 1)


	}
	else
	{;}

	if(f80211 == 0)
	{
		extra_len += (qos)? IEEE80211_QOS_CTL_LEN: 0;
		extra_len += (ht)? IEEE80211_HT_CTL_LEN: 0;
		extra_len += (use_4addr)? ETHER_ADDR_LEN: 0;
	}



	switch(gDeviceInfo->tx_type)
	{
		case SSV6XXX_TX_REQ0:
            extra_len += sizeof(struct cfg_host_txreq0);
            os_frame_push(frame,extra_len);

            padding_len=(((u32)OS_FRAME_GET_DATA(frame))&(0x3));
            if(0!=padding_len){
                padding_len=4-padding_len;
                os_frame_push(frame,padding_len);
            }

            host_txreq0 = (struct cfg_host_txreq0 *)OS_FRAME_GET_DATA(frame);
            pos = (u8*)host_txreq0 + sizeof(struct cfg_host_txreq0);

            CFG_HOST_TXREQ0(host_txreq0,
                            OS_FRAME_GET_DATA_LEN(frame),
                            M0_TXREQ,
                            f80211,
                            qos,
                            ht,
                            use_4addr,
                            0,
                            padding_len,
                            (!!IS_BIT_SET(tx_dscrp_flag, TX_DSCRP_SET_BC_QUE)<<26),
                            0,
                            0,
                            0,
                            (!!IS_BIT_SET(tx_dscrp_flag, TX_DSCRP_SET_EXTRA_INFO)<<31));
            //LOG_PRINTF("host_txreq0=0x%x,extra_len = %d,txrq_s=%d,host_txreq0->len=%d,tot_len=%d,%d\r\n",(u32)host_txreq0,
            //    extra_len,sizeof(struct cfg_host_txreq0),host_txreq0->len,((struct pbuf *)frame)->tot_len,OS_FRAME_GET_DATA_LEN(frame));
            if(gDeviceInfo->hw_mode == SSV6XXX_HWM_AP)
                host_txreq0->security = (gDeviceInfo->APInfo->sec_type!= SSV6XXX_SEC_NONE);
            else
                host_txreq0->security = (gDeviceInfo->joincfg->sec_type != SSV6XXX_SEC_NONE);

            //host_txreq0->extra_info = !!IS_BIT_SET(tx_dscrp_flag, TX_DSCRP_SET_EXTRA_INFO);
            //host_txreq0->bc_queue = !!IS_BIT_SET(tx_dscrp_flag, TX_DSCRP_SET_BC_QUE);

            if (f80211 == 0) {

                if (use_4addr) {
                    MEMCPY(pos, (void*)&gDeviceInfo->addr4, ETHER_ADDR_LEN);
                    pos += ETHER_ADDR_LEN;
                }

                if (qos) {
                    u16* q_ctrl = (u16*)pos;
                    *q_ctrl = (gDeviceInfo->qos_ctrl&0xFFF8)|priority;
                    pos += IEEE80211_QOS_CTL_LEN;
                }

                if (ht) {
                    ht_st = (struct ht_ctrl_st *)pos;
                    ht_st->ht = gDeviceInfo->ht_ctrl;
                    pos += IEEE80211_HT_CTL_LEN;
                }

            }
            else
            {
                /* speicify "stype_b5b4" field of TxInfo */
                host_txreq0->sub_type = pos[0]>>4;
            }

            break;

        case SSV6XXX_TX_REQ1:
        case SSV6XXX_TX_REQ2:
            return FALSE;
            break;

        default:
            break;
	}

    return (TxHdl_FrameProc(frame, false, priority, 0));
}

extern tx_result ssv6xxx_data_could_be_send(void *frame, bool bAPFrame, u32 TxFlags);

/*
 * Be noticed that you have to process the frame if the return value of TxHdl_FrameProc is FALSE.
 * The FALSE return value means that the frame is not sent to txrx_task due to some reason.
*/
bool TxHdl_FrameProc(void *frame, bool apFrame, u32 priority, u32 flags)
{
    s32 retAP = TX_CONTINUE;
    void *dup_frame = frame;
    bool ret = true;
    struct cfg_host_txreq0 *host_txreq=(struct cfg_host_txreq0 *)OS_FRAME_GET_DATA(frame);
    u32 copy_len=0;
    u32 padding=host_txreq->padding;
    u8 *pS=NULL,*pD=NULL;

	do
	{

#if( BEACON_DBG == 0)
		if (SSV6XXX_HWM_AP == gDeviceInfo->hw_mode)
  		{
  			//Ap mode data could be drop or queue(power saving)
  			if (TX_CONTINUE != (retAP = ssv6xxx_data_could_be_send(frame, apFrame, flags)))
  				break;
  		}
#endif//#if( BEACON_DBG == 1)

        if ((padding!=0)&&(host_txreq->f80211==0)){
            copy_len=sizeof(struct cfg_host_txreq0);
            if(host_txreq->ht==1) copy_len+=IEEE80211_HT_CTL_LEN;
            if(host_txreq->qos==1) copy_len+=IEEE80211_QOS_CTL_LEN;
            if(host_txreq->use_4addr==1) copy_len+=ETHER_ADDR_LEN;
            host_txreq->len-=padding;
            pS=(u8 *)((u32)OS_FRAME_GET_DATA(frame)+copy_len-1);
            pD=pS+padding;
            do{ *pD--=*pS--;}while(--copy_len);
            os_frame_pull(frame,padding);
        }

		//Send to tx driver task
        if(apFrame)
            dup_frame = os_frame_dup(frame);

        if(dup_frame)
        {
            ret = TXRXTask_FrameEnqueue(dup_frame, 0);
        }
        else
        {
            ret = false;
            LOG_ERROR("%s can't duplicated frame\n",__func__);
        }
	} while (0);

	//reuse frame buffer in AP mgmt and beacon frame. no need to release
	if (/*!bAPFrame && TX_CONTINUE == retAP ||*/
		TX_DROP == retAP)
		os_frame_free(dup_frame);

    return ret;
}

s32 TxHdl_FlushFrame()
{
    //todo: execute pause of txrx task
    //flush the data frames inside txrx task
    //
    return true;
}


s32 TxRxHdl_Init()
{
    OS_MutexInit(&rxhdl_mtx);
    MEMSET(&rx_wifi_flt, 0, sizeof(rx_wifi_flt));
    MEMSET(&rx_eth_flt, 0, sizeof(rx_eth_flt));
    wifi_flt_num = 0;
    eth_flt_num = 0;
    TXRXTask_Init();

    return 0;
}

s32 RxHdl_FrameProc(void* frame)
{
    ssv6xxx_data_result data_ret = SSV6XXX_DATA_CONT;

	//Give it to AP handle firstly.
	//LOG_PRINTF("gHCmdEngInfo->hw_mode: %d \n",gHCmdEngInfo->hw_mode);
#ifdef RXFLT_ENABLE
    struct cfg_host_rxpkt *pPktInfo = (struct cfg_host_rxpkt *)OS_FRAME_GET_DATA(frame);
    u8 *raw = (u8 *)ssv6xxx_host_rx_data_get_data_ptr(pPktInfo);
    u8 b7b2 = raw[0]>>2, i = 0;

    if ((wifi_flt_num != 0) && (pPktInfo->f80211 == 1))
    {
        OS_MutexLock(rxhdl_mtx);
        for(;i < RXPKTFLT_NUM; i++)
        {
            if ((rx_wifi_flt[i].b7b2mask != 0) && ((b7b2&rx_wifi_flt[i].b7b2mask) == rx_wifi_flt[i].fc_b7b2))
                data_ret = rx_wifi_flt[i].cb_fn(frame, OS_FRAME_GET_DATA_LEN(frame));
        }
        OS_MutexUnLock(rxhdl_mtx);
    }

    if ((eth_flt_num) != 0 && (pPktInfo->f80211 != 1))
    {
        u16 eth_type = 0;
        u8* eth_type_addr = (u8*) ((u8*)pPktInfo + (RxHdl_GetRawRxDataOffset((struct cfg_host_rxpkt *)OS_FRAME_GET_DATA(frame)) + 12));
        eth_type = eth_type_addr[0]<<8|eth_type_addr[1];

        OS_MutexLock(rxhdl_mtx);
        for(i = 0;i < RXPKTFLT_NUM; i++)
        {
            if(eth_type == rx_eth_flt[i].ethtype)
                data_ret = rx_eth_flt[i].cb_fn(frame, OS_FRAME_GET_DATA_LEN(frame));
        }
        OS_MutexUnLock(rxhdl_mtx);
    }

    if (data_ret != SSV6XXX_DATA_CONT)
        return 0;
#endif

	if (SSV6XXX_HWM_AP == gDeviceInfo->hw_mode)
    	data_ret = AP_RxHandleFrame(frame);

	if (SSV6XXX_DATA_CONT == data_ret)
	{
		int i;
		//remove ssv descriptor(RxInfo), just leave raw data.
		os_frame_pull(frame, RxHdl_GetRawRxDataOffset((struct cfg_host_rxpkt *)OS_FRAME_GET_DATA(frame)));

		for (i=0;i<HOST_DATA_CB_NUM;i++)
		{
			data_handler handler = gDeviceInfo->data_cb[i];
			if (handler)
			{
				data_ret = handler(frame, OS_FRAME_GET_DATA_LEN(frame));
				if (SSV6XXX_DATA_ACPT==data_ret)
					break;
			}
		}//-----------------
	}

	if(SSV6XXX_DATA_ACPT != data_ret)
		os_frame_free(frame);
    return 0;
}

s32 RxHdl_SetWifiRxFlt(struct wifi_flt *flt, ssv6xxx_cb_action act)
{
    bool ret = false;
    s8 i = 0, empty = -1, exist = -1;

    OS_MutexLock(rxhdl_mtx);
    for (;i < RXPKTFLT_NUM; i++)
    {
        if(rx_wifi_flt[i].cb_fn == NULL)
        {
            empty = i;
            continue;
        }
        if(flt->cb_fn == rx_wifi_flt[i].cb_fn)
            exist = i;
    }

    if(act == SSV6XXX_CB_ADD)
    {
        if(empty >= 0)
        {
            MEMCPY((void *)&rx_wifi_flt[empty], (void *)flt, sizeof(struct wifi_flt));
            wifi_flt_num++;
            ret = true;
        }
    }
    else
    {
        if(exist >= 0)
        {
            if(act == SSV6XXX_CB_REMOVE)
            {
                MEMSET((void *)&rx_wifi_flt[exist], 0, sizeof(struct wifi_flt));
                wifi_flt_num--;
            }
            else
                MEMCPY((void *)&rx_wifi_flt[exist], (void *)flt, sizeof(struct wifi_flt));
            ret = true;
        }
    }
    OS_MutexUnLock(rxhdl_mtx);

    return ret;
}

s32 RxHdl_SetEthRxFlt(struct eth_flt *flt, ssv6xxx_cb_action act)
{
    bool ret = false;
    s8 i = 0, empty = -1, exist = -1;

    OS_MutexLock(rxhdl_mtx);
    for (;i < RXPKTFLT_NUM; i++)
    {
        if(rx_eth_flt[i].cb_fn == NULL)
        {
            empty = i;
            continue;
        }
        if(flt->cb_fn == rx_eth_flt[i].cb_fn)
            exist = i;
    }

    if(act == SSV6XXX_CB_ADD)
    {
        if(empty >= 0)
        {
            MEMCPY((void *)&rx_eth_flt[empty], (void *)flt, sizeof(struct eth_flt));
            eth_flt_num++;
            ret = true;
        }
    }
    else
    {
        if(exist >= 0)
        {
            if(act == SSV6XXX_CB_REMOVE)
            {
                MEMSET((void *)&rx_eth_flt[exist], 0, sizeof(struct eth_flt));
                eth_flt_num--;
            }
            else
                MEMCPY((void *)&rx_eth_flt[exist], (void *)flt, sizeof(struct eth_flt));
            ret = true;
        }
    }
    OS_MutexUnLock(rxhdl_mtx);

    return ret;
}