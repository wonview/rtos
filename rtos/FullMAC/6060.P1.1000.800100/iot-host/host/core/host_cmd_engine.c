#define __SFILE__ "host_cmd_engine.c"

#include <log.h>
#include <config.h>
#include <types.h>
#include <common.h>


#include "os_wrapper.h"
#include "host_apis.h"
#include "host_cmd_engine.h"
#include "host_cmd_engine_priv.h"
#include "host_cmd_engine_sm.h"
#include "ssv_timer.h"

#include "ap/ap.h"

#include <pbuf.h>
#include <msgevt.h>
#include <cmd_def.h>

#include <drv/ssv_drv.h>		// for ssv6xxx_drv_send()
#include <txrx_hdl.h>

#if (defined _WIN32)
#include <wtypes.h>
#include <Dbt.h>
#endif

#define FREE_FRM_NUM 5

HostCmdEngInfo_st *gHCmdEngInfo;
#define STATE_MACHINE_DATA struct HostCmdEngInfo




extern void os_timer_init(os_timer_st *timer);






//-------------------------------------------------------------
extern void CmdEng_RxHdlEvent(struct cfg_host_event *pHostEvt);
extern void CmdEng_TxHdlCmd(struct cfg_host_cmd *pPktInfo);





s32 APCmdCallback(ssv6xxx_ap_cmd cmd, u32 nResult);


void pendingcmd_expired_handler(void* data1, void* data2)
{
    HostCmdEngInfo_st *info = (HostCmdEngInfo_st *)data1;
    OS_MutexLock(info->CmdEng_mtx);
    if(info->debug != false)
        LOG_DEBUG("[CmdEng]: pending cmd %d timeout!! \n", info->pending_cmd_seqno);

    info->pending_cmd_seqno = 0;
    info->blockcmd_in_q = false;
    OS_MutexUnLock(info->CmdEng_mtx);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool CmdEng_DbgSwitch()
{
    OS_MutexLock(gHCmdEngInfo->CmdEng_mtx);
    gHCmdEngInfo->debug = !gHCmdEngInfo->debug;
    OS_MutexUnLock(gHCmdEngInfo->CmdEng_mtx);
    return gHCmdEngInfo->debug;
}

void CmdEng_HandleQueue(void *frame)
{

	//event/frames(data/ctrl/mgmt)/cmd
	//pPktInfo;
	struct cfg_host_rxpkt * rxpkt = (struct cfg_host_rxpkt *)OS_FRAME_GET_DATA(frame);//(struct cfg_host_rxpkt *)pPktInfo;
	switch (rxpkt->c_type)
	{
		//-------------
		//RX data
#if(MLME_TASK==0)
	case M0_RXEVENT:
    {
        if (SSV6XXX_HWM_AP == gDeviceInfo->hw_mode)
        {
            AP_RxHandleAPMode(frame);

        }
		else
        {
            LOG_PRINTF("Unhandle M0_RXEVENT \r\n");
            os_frame_free(frame);
        }
        //    CmdEng_RxHdlData(frame);
		break;
    }
#endif
	case HOST_EVENT:
		CmdEng_RxHdlEvent(frame);
		break;
		//-------------
		//TX
	case HOST_CMD:
    {
        FrmQ *pcmd = NULL;
        struct cfg_host_cmd *hCmd = (struct cfg_host_cmd *)OS_FRAME_GET_DATA(frame);

        if (gHCmdEngInfo->blockcmd_in_q == true)
        {
            if( (pcmd = (FrmQ *)list_q_deq_safe(&gHCmdEngInfo->free_FrmQ, &gHCmdEngInfo->CmdEng_mtx)) == NULL)
                pcmd = (FrmQ *)MALLOC(sizeof(FrmQ));

            pcmd->frame = frame;
            list_q_qtail_safe(&gHCmdEngInfo->pending_cmds, (struct list_q *)pcmd, &gHCmdEngInfo->CmdEng_mtx);
            if (gHCmdEngInfo->debug != false)
                LOG_DEBUG("[CmdEng]: Pending cmd %d\n", hCmd->cmd_seq_no);
            return;
        }
        else
            CmdEng_TxHdlCmd(frame);
		break;
    }
	default:
	    hex_dump(rxpkt,128);
		LOG_FATAL("Unexpect c_type %d appeared\r\n", rxpkt->c_type);
		ASSERT(0);
	}

}

ssv6xxx_result CmdEng_GetStatus(struct CmdEng_st *st)
{
    if(st == NULL)
        return SSV6XXX_INVA_PARAM;

    OS_MutexLock(gHCmdEngInfo->CmdEng_mtx);
    st->mode = gHCmdEngInfo->curr_mode;
    st->BlkCmdIn = gHCmdEngInfo->blockcmd_in_q;
    st->BlkCmdNum = list_q_len(&gHCmdEngInfo->pending_cmds);
    OS_MutexUnLock(gHCmdEngInfo->CmdEng_mtx);

    return SSV6XXX_SUCCESS;
}








//---------------------------------------------------------------------------------------------
extern s32 AP_Start(void);
extern s32 AP_Stop( void );

void CmdEng_Task( void *args )
{

	PKT_RxInfo *pPktInfo;
	MsgEvent *MsgEv;
	s32 res;
	extern u32 g_RunTaskCount;

	LOG_TRACE("%s() Task started.\r\n", __FUNCTION__);
	g_RunTaskCount++;

#ifdef __TEST__
	//_Cmd_CreateSocketClient(0, NULL);

#endif

	//SSVHostCmdEng_Start();
	//	SM_ENTER(HCMDE, IDLE, NULL);

	while(1)
	{
        if ((gHCmdEngInfo->blockcmd_in_q == false) && (list_q_len_safe(&gHCmdEngInfo->pending_cmds, &(gHCmdEngInfo->CmdEng_mtx)) > 0))
        {
            //Proceeding pending cmds
            FrmQ *pcmd = NULL;
            struct cfg_host_cmd *hCmd = NULL;

            pcmd = (FrmQ *)list_q_deq_safe(&gHCmdEngInfo->pending_cmds, &gHCmdEngInfo->CmdEng_mtx);
            hCmd = (struct cfg_host_cmd *)OS_FRAME_GET_DATA(pcmd->frame);
            if (gHCmdEngInfo->debug != false)
                LOG_DEBUG("[CmdEng]: Pop pending cmd %d to execute\n", hCmd->cmd_seq_no);
            CmdEng_TxHdlCmd(pcmd->frame);
            OS_MutexLock(gHCmdEngInfo->CmdEng_mtx);
            if(list_q_len(&gHCmdEngInfo->free_FrmQ) < FREE_FRM_NUM)
                list_q_qtail(&gHCmdEngInfo->free_FrmQ, (struct list_q *)pcmd);
            else
                FREE(pcmd);
            OS_MutexUnLock(gHCmdEngInfo->CmdEng_mtx);
        }
        else
        {
            /* Wait Message: */
            res = msg_evt_fetch(MBOX_CMD_ENGINE, &MsgEv);
            ASSERT(res == OS_SUCCESS);

    		//LOG_TRACE("AP needs to handle msg:%d.\n", MsgEv->MsgType);
            switch(MsgEv->MsgType)
            {
            case MEVT_PKT_BUF:
                pPktInfo = (PKT_RxInfo *)MsgEv->MsgData;
                CmdEng_HandleQueue(pPktInfo);
                os_msg_free(MsgEv);
                break;

                /**
                *  Message from software timer timeout event.
                */
            case MEVT_HOST_TIMER:
                os_timer_expired((void *)MsgEv);
                os_msg_free(MsgEv);
                break;


            case MEVT_HOST_CMD:
                switch(MsgEv->MsgData)
                {

                case AP_CMD_AP_MODE_ON:
                    AP_Start();
                    break;

                case AP_CMD_AP_MODE_OFF:
                    AP_Stop();
                    break;
#ifdef __TEST_DATA__
                case AP_CMD_ADD_STA:
                    TestCase_AddAPSta();
                    break;

                case AP_CMD_PS_POLL_DATA:
                    TestCase_SendPSPoll();
                    break;

                case AP_CMD_PS_TRIGGER_FRAME:
                    TestCase_SendTriggerFrame();
                    break;
#endif//__TEST_DATA__
                default:
                    break;
                }

                msg_evt_free(MsgEv);
                break;

            default:
                //SoftMac_DropPkt(MsgEv);
                LOG_PRINTF("%s(): unknown message type(%02x) !!\n",
                    __FUNCTION__, MsgEv->MsgType);
                break;
            };
        }

	}
}

#if (_WIN32 == 1 && CONFIG_RX_POLL == 0)
#define INITGUID
#include <guiddef.h>
WNDPROC wpOrigProc;
DEFINE_GUID(GUID_DEVINTERFACE_ssvsdio_intevent,
			0x76c8ffb9, 0xf552, 0x4398, 0xa3, 0xd6, 0x52, 0x73, 0xfe, 0xc5, 0x5b, 0xe2);

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_DEVICECHANGE:
		{
			if ( wParam == DBT_CUSTOMEVENT )
			{
				DEV_BROADCAST_HANDLE* handle = (DEV_BROADCAST_HANDLE*)lParam;
				if ( handle->dbch_devicetype == DBT_DEVTYP_HANDLE && IsEqualGUID(&handle->dbch_eventguid, &GUID_DEVINTERFACE_ssvsdio_intevent ))
				{
					SSV6XXX_Drv_Rx_Task(NULL);
				}
			}
			break;
		}
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

HWND CreateMessageOnlyWindow()
{
	WNDCLASSEX wx;
	DEV_BROADCAST_HANDLE handle;
	HWND hwnd;
	ZeroMemory(&wx, sizeof(WNDCLASSEX));
	ZeroMemory(&handle, sizeof(DEV_BROADCAST_HANDLE));

	wx.cbSize = sizeof(WNDCLASSEX);
	wx.lpfnWndProc = WndProc;
	wx.lpszClassName = L"Message-Only Window";

	if ( RegisterClassEx(&wx) )
		hwnd = CreateWindow(L"Message-Only Window", NULL, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, NULL, NULL);
	else
		return NULL;

	handle.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
	handle.dbch_handle = (HANDLE)ssv6xxx_drv_get_handle();
	handle.dbch_devicetype = DBT_DEVTYP_HANDLE;
	handle.dbch_eventguid = GUID_DEVINTERFACE_ssvsdio_intevent;
	RegisterDeviceNotification(hwnd,&handle,DEVICE_NOTIFY_WINDOW_HANDLE);
}

void SSV6XXX_drv_msg_only(void *args)
{
	MSG msg;
	HWND m_hWnd;
	m_hWnd = CreateMessageOnlyWindow();



	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if(msg.message == WM_QUIT)
		{
			break;
		}
	}
}
#endif

struct task_info_st g_host_task_info[] =
{
	{ "host_cmd",   (OsMsgQ)0, 16, 	OS_CMD_ENG_PRIO, CMD_ENG_STACK_SIZE, NULL, CmdEng_Task },

};

#ifdef RXFLT_ENABLE
ssv6xxx_data_result test_cb(void * data, u32 len)
{
    LOG_DEBUG("[CmdEng]: RXFLT test, data = %x, len = %d\n", data, len);
    return SSV6XXX_DATA_CONT;
}
#endif

s32 CmdEng_Init(void)
{
	u32 i, size, res=OS_SUCCESS;

	size = sizeof(HostCmdEngInfo_st);
	gHCmdEngInfo = MALLOC(size);
	MEMSET(gHCmdEngInfo, 0, size);
    OS_MutexInit(&(gHCmdEngInfo->CmdEng_mtx));
    list_q_init(&gHCmdEngInfo->pending_cmds);
    list_q_init(&gHCmdEngInfo->free_FrmQ);
    gHCmdEngInfo->curr_mode = MT_STOP;
    gHCmdEngInfo->blockcmd_in_q = false;
    gHCmdEngInfo->pending_cmd_seqno = 0;
    gHCmdEngInfo->debug = false;


    TxRxHdl_Init();

	size = sizeof(g_host_task_info)/sizeof(struct task_info_st);
	for(i = 0; i < size; i++)
    {
		if (g_host_task_info[i].qlength> 0)
        {
			ASSERT(OS_MsgQCreate(&g_host_task_info[i].qevt,
				(u32)g_host_task_info[i].qlength)==OS_SUCCESS);
		}

		/* Create Registered Task: */
		OS_TaskCreate(g_host_task_info[i].task_func,
			g_host_task_info[i].task_name,
			g_host_task_info[i].stack_size<<4,
			g_host_task_info[i].args,
			g_host_task_info[i].prio,
			NULL);
	}
#if 0 //RXFLT_ENABLE demo
    struct wifi_flt test_flt_w;
    test_flt_w.b7b2mask = 0x03;
    test_flt_w.fc_b7b2 = 0x00;
    test_flt_w.cb_fn = test_cb;
    RxHdl_SetWifiRxFlt(&test_flt_w, SSV6XXX_CB_ADD);
    struct eth_flt test_flt_e;
    test_flt_e.ethtype = 0x886E;
    test_flt_e.cb_fn = test_cb;
    RxHdl_SetEthRxFlt(&test_flt_e, SSV6XXX_CB_ADD);
#endif
	return res;
}

void CmdEng_FlushPendingCmds()
{
    os_cancel_timer(pendingcmd_expired_handler, (u32)gHCmdEngInfo, (u32)NULL);
    gHCmdEngInfo->blockcmd_in_q = false;
    gHCmdEngInfo->pending_cmd_seqno = 0;
    while(list_q_len(&gHCmdEngInfo->pending_cmds) > 0)
    {
        FrmQ *pcmd = (FrmQ *)list_q_deq(&gHCmdEngInfo->pending_cmds);
        os_frame_free(pcmd->frame);
        pcmd->frame = NULL;
        if(list_q_len(&gHCmdEngInfo->free_FrmQ) > FREE_FRM_NUM)
        {
            FREE(pcmd);
        }
        else
        {
            list_q_qtail(&gHCmdEngInfo->free_FrmQ, (struct list_q *)pcmd);
        }
    }
    //LOG_DEBUG("[CmdEng]: CmdEng_FlushPendingCmds\n");
}

ssv6xxx_result CmdEng_SetOpMode(ModeType mode)
{
    ssv6xxx_result ret = SSV6XXX_SUCCESS;

    if(mode > MT_EXIT)
        return SSV6XXX_INVA_PARAM;

    OS_MutexLock(gHCmdEngInfo->CmdEng_mtx);

    switch (gHCmdEngInfo->curr_mode)
    {
        case MT_STOP:
        {
            switch (mode)
            {
                case MT_RUNNING:
                    // To run
                    break;
                case MT_EXIT:
                    // To end
                    break;
                default:
                    ret = SSV6XXX_INVA_PARAM;
                    break;
            }
        }
            break;
        case MT_RUNNING:
            if(mode == MT_STOP)
            {
                //To stop
                CmdEng_FlushPendingCmds();
            }
            else
            {
                //error handling
                ret = SSV6XXX_INVA_PARAM;
            }
            break;
        case MT_EXIT:
        default:
            //error handling
            ret = SSV6XXX_FAILED;
            break;
    }

    if(ret == SSV6XXX_SUCCESS)
        gHCmdEngInfo->curr_mode = mode;

    OS_MutexUnLock(gHCmdEngInfo->CmdEng_mtx);

    return ret;
}




s32 APCmdCallback(ssv6xxx_ap_cmd cmd, u32 nResult)
{
	// 	{
	// 		//int size = sizeof(struct cfg_host_event)+sizeof(struct cfg_hw_mode_info_resp);
	// 		//struct cfg_host_event* evt = MALLOC(size);
	// 		//struct cfg_hw_mode_info_resp *resp = (struct cfg_hw_mode_info_resp *)&evt->dat;
	// 		//evt->len =size;
	//
	//
	// 		// 1. notify host user hw mode resp
	// 		//evt->c_type = M0_RXEVENT;
	// 		//evt->h_event = SOC_EVT_HW_MODE_RESP;
	// 		//resp->mode = gHCmdEngInfo->hw_mode;
	// 		//resp->ret = nResult;
	//
	// #if 0
	// 		// 2. change to running state
	// 		if (cmd == SSV6XXX_AP_ON)
	// 		{
	// 			SM_ENTER(HCMDE, RUNNING, NULL);
	// 		}
	// 		else
	// 		{
	// 			SM_ENTER(HCMDE, IDLE, NULL);
	// 		}
	// #endif
	//
	//
	// 		//HCmdEng_RxHdlEvent(evt);
	// 	}
	return 0;
}


#if 0
void RX_public_host_event(u32 nEvtId, void *data)
{

	HDR_HostEvent *HostEvent;
	HostEvent= (HDR_HostEvent *)data;

	switch(nEvtId){
	case HOST_EVT_HW_MODE_RESP:
		SM_ENTER(HCMDE, RUNNING, NULL);

		//LOG_TRACE(SSV_STA,"set hw mode sucessful\n");
		break;
	case HOST_EVT_SCAN_RESULT:
		//LOG_TRACE(SSV_STA, "scan sucessful\n");
		break;
	case HOST_EVT_JOIN_RESULT:
		{
			struct resp_evt_result *join_resp ;

			join_resp=(struct resp_evt_result *)HostEvent->dat;
			//struct netif *netif;
			//char *name="wlan0";
			if(join_resp->u.status_code==0)
			{
				/*netif = netif_find(name);
				if(netif!=NULL)
				{
				netifapi_dhcp_start(netif);
				}*/
				//LOG_TRACE(SSV_STA, "join sucessful\n");

			}

			break;
		}

	case HOST_EVT_LEAVE_RESULT:
		break;

	}

}

#endif

// ssv6xxx_result SSVHostCmdEng_Start()
// {
//
//
//
// 	//gHCmdEngInfo->state = SSV_STATE_IDLE;
// 	return SSV6XXX_SUCCESS;
// }
// ssv6xxx_result SSVHostCmdEng_Stop()
// {
//
//
//
//
// 	return SSV6XXX_SUCCESS;
// }






