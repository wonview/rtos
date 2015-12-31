#define __SFILE__ "host_cmd_engine_rx.c"

#include <log.h>
#include <config.h>
#include <types.h>
#include <common.h>
#include <hdr80211.h>

#include "os_wrapper.h"
#include "host_apis.h"
#include "host_cmd_engine.h"
#include "host_cmd_engine_priv.h"


#include "ap/ap.h"


#include <pbuf.h>
#include <msgevt.h>
#include <cmd_def.h>
#include "dev.h"
#include "mlme.h"
#define EVT_PREPROCESS


extern void AP_RxHandleEvent(struct cfg_host_event *pHostEvt);
//extern void pendingcmd_expired_handler(void* data1, void* data2);
s32 join_host_event_handler_(struct cfg_host_event *pPktInfo);
s32 scan_host_event_handler_(struct cfg_host_event *pPktInfo);
void exec_host_evt_cb(u32 nEvtId, void *data, s32 len);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//																						Rx Event
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


typedef s32 (*CmdEng_RxEvtHandler)(struct cfg_host_event *pPktInfo);


s32 CmdEng_evt_drop(struct cfg_host_event *pPktInfo)
{
	ssv6xxx_result ret = SSV6XXX_SUCCESS;

	LOG_TRACE("Drop Host Evt\n");
	//FREEIF(pPktInfo);

	return ret;
}

// only handle null data event, ps poll control frame
s32 ap_handle_ps(void *frame)
{
	//ssv6xxx_result ret = SSV6XXX_SUCCESS;
	if (SSV6XXX_HWM_AP==gDeviceInfo->hw_mode)
		AP_RxHandleEvent(frame); // only handle null data event, ps poll control frame

    os_frame_free(frame);
	return SSV6XXX_SUCCESS;
}



s32 CmdEng_evt_get_reg_handle(struct cfg_host_event *pEvent)
{

    LOG_PRINTF("%s(): len=%d, val=0x%08x\r\n",
        __FUNCTION__, pEvent->len, *(u32 *)pEvent->dat);

    return SSV6XXX_SUCCESS;
}

#ifdef EVT_PREPROCESS
s32 CmdEng_evt_test_fn(struct cfg_host_event *pEvent)
{

    LOG_PRINTF("[CmdEng]: evtid= %d , len=%d, val=0x%08x\r\n" , pEvent->h_event ,pEvent->len, *(u32 *)pEvent->dat);

    return SSV6XXX_FAILED;
}
#endif
//
// s32 CmdEng_evt_pspoll(struct cfg_host_event *pPktInfo)
// {
// 	//ssv6xxx_result ret = SSV6XXX_SUCCESS;
// 	if (SSV6XXX_HWM_AP==gHCmdEngInfo->hw_mode)
// 		AP_RxHandleEvent(pPktInfo);
// 	else
// 		FREEIF(pPktInfo);
//
// 	return SSV6XXX_SUCCESS;
// }
//
//
// s32 CmdEng_evt_nulldata(struct cfg_host_event *pPktInfo)
// {
// 	if (SSV6XXX_HWM_AP==gHCmdEngInfo->hw_mode)
// 		AP_RxHandleEvent(pPktInfo);
// 	else
// 		FREEIF(pPktInfo);
//
// 	return SSV6XXX_SUCCESS;
// }


/*HOST_EVT_SCAN_RESULT*/





const CmdEng_RxEvtHandler PrivateRxEvtHandler[] =
{
	CmdEng_evt_drop,		//SOC_EVT_CONFIG_HW_RESP
	NULL,					//SOC_EVT_SET_BSS_PARAM_RESP
	NULL,	                //SOC_EVT_PS_POLL      ,handle on rx task
	NULL,	                //SOC_EVT_NULL_DATA ,handle on rx task
	NULL,					//SOC_EVT_REG_RESULT
};

struct evt_prefn
{
    ssv6xxx_soc_event evt;
    CmdEng_RxEvtHandler pre_fn;
};

const struct evt_prefn PreFnTbl[] =
{
#if (MLME_TASK==1)
    { SOC_EVT_SCAN_RESULT, mlme_host_event_handler_},
#else
    { SOC_EVT_SCAN_RESULT, scan_host_event_handler_},
#endif
	{ SOC_EVT_JOIN_RESULT, join_host_event_handler_},
};

s32 scan_host_event_handler_(struct cfg_host_event *pPktInfo)
{
	void* pPktData = NULL;
    if(pPktInfo==NULL)
    {
         LOG_PRINTF("Error %s @line %d free null pointer\r\n",__FUNCTION__,__LINE__);
         return 0;
    }
	pPktData = (void*)((u32)(pPktInfo->dat));
	sta_mode_ap_list_handler(pPktData,gDeviceInfo->ap_list);
	return 0;
}

s32 join_host_event_handler_(struct cfg_host_event *pPktInfo)
{
	if(gDeviceInfo->joincfg->bss.wmm_used)
		SET_TXREQ_WITH_QOS(gDeviceInfo, 1);
 	else
		SET_TXREQ_WITH_QOS(gDeviceInfo, 0);

	exec_host_evt_cb(pPktInfo->h_event,pPktInfo->dat,pPktInfo->len-sizeof(HDR_HostEvent));
	return 0;
}

void exec_host_evt_cb(u32 nEvtId, void *data, s32 len)
{
	u32 i;
	for (i=0;i<HOST_EVT_CB_NUM;i++)
	{
		evt_handler handler = gDeviceInfo->evt_cb[i];
		if (handler)
			handler(nEvtId, data, len);
    }
}

void CmdEng_RxHdlEvent(void *frame)
{
	struct cfg_host_event *pPktInfo = (struct cfg_host_event *)OS_FRAME_GET_DATA(frame);
    struct resp_evt_result *join_res = NULL;
#ifdef EVT_PREPROCESS
	int i;
    s32 PreFnTblSize = sizeof(PreFnTbl)/sizeof(struct evt_prefn);
#endif

	if (pPktInfo->h_event >= SOC_EVT_PRIVE_CMD_START)
		PrivateRxEvtHandler[pPktInfo->h_event-SOC_EVT_PRIVE_CMD_START](pPktInfo);
	else
	{
		//Customer event need to free packet info
		/*if(pPktInfo->h_event==HOST_EVT_HW_MODE_RESP)
		{

		}*/
        OS_MutexLock(gHCmdEngInfo->CmdEng_mtx);
        if ((gHCmdEngInfo->blockcmd_in_q == true) && (pPktInfo->h_event == SOC_EVT_MLME_CMD_DONE) && (pPktInfo->evt_seq_no == gHCmdEngInfo->pending_cmd_seqno))
        {            
            LOG_DEBUGF(LOG_CMDENG, ("[CmdEng]: Got SOC_EVT_MLME_CMD_DONE %d for block cmd\r\n", gHCmdEngInfo->pending_cmd_seqno));
            gHCmdEngInfo->blockcmd_in_q = false;
            gHCmdEngInfo->pending_cmd_seqno = 0;
            //os_cancel_timer(pendingcmd_expired_handler, gHCmdEngInfo, NULL);
        }
        OS_MutexUnLock(gHCmdEngInfo->CmdEng_mtx);

        if (pPktInfo->h_event == SOC_EVT_MLME_CMD_DONE)
        {
            os_frame_free(frame);
            return;
        }
        //handle station mode fw event

        switch (pPktInfo->h_event) {
#ifdef USE_CMD_RESP
        case SOC_EVT_DEAUTH:
            //clean sta mode global variable
            OS_MutexLock(gDeviceInfo->g_dev_info_mutex);
            gDeviceInfo->status= DISCONNECT;
            MEMSET(gDeviceInfo->joincfg, 0, sizeof(struct cfg_join_request));
            OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex);
            break;
#else
        case SOC_EVT_JOIN_RESULT:
            join_res = (struct resp_evt_result *)pPktInfo->dat;
            if (join_res->u.join.status_code != 0)
            {
                OS_MutexLock(gDeviceInfo->g_dev_info_mutex);
                gDeviceInfo->status = DISCONNECT;
                OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex);
            }
            else
            {
                OS_MutexLock(gDeviceInfo->g_dev_info_mutex);
                gDeviceInfo->status = CONNECT;
                OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex);
            }
            break;
        case SOC_EVT_LEAVE_RESULT:
            //clean sta mode global variable
            OS_MutexLock(gDeviceInfo->g_dev_info_mutex);
            gDeviceInfo->status= DISCONNECT;
            MEMSET(gDeviceInfo->joincfg, 0, sizeof(struct cfg_join_request));
            OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex);
            break;
 #endif
         default:
            break;
        }

#ifdef EVT_PREPROCESS

        for(i=0; i< PreFnTblSize;i++)
        {
            if(pPktInfo->h_event == PreFnTbl[i].evt)
            {
                if(PreFnTbl[i].pre_fn(pPktInfo) == SSV6XXX_SUCCESS)
                {
                    os_frame_free(frame);
                    return;
                }
            }
        }
#endif
        exec_host_evt_cb(pPktInfo->h_event,pPktInfo->dat,pPktInfo->len-sizeof(HDR_HostEvent));
	}

	os_frame_free(frame);
}


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//																						Rx Data
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

u16 CmdEng_GetRawRxDataOffset(struct cfg_host_rxpkt *pPktInfo)
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



void  CmdEng_RxHdlData(void *frame)
{
	//struct cfg_host_rxpkt *pPktInfo = ;
	ssv6xxx_data_result data_ret = SSV6XXX_DATA_CONT;

	//Give it to AP handle firstly.
	//LOG_PRINTF("gHCmdEngInfo->hw_mode: %d \n",gHCmdEngInfo->hw_mode);
	if (SSV6XXX_HWM_AP == gDeviceInfo->hw_mode)
    	data_ret = AP_RxHandleFrame(frame);




	if (SSV6XXX_DATA_CONT == data_ret)
	{
		int i;
		//remove ssv descriptor(RxInfo), just leave raw data.
		os_frame_pull(frame, CmdEng_GetRawRxDataOffset((struct cfg_host_rxpkt *)OS_FRAME_GET_DATA(frame)));

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

}






