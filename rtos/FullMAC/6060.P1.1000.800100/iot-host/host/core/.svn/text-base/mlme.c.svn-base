#include "rtos.h"
#include "log.h"
#include "mlme.h"
#include "ssv_timer.h"
#include "host_cmd_engine_priv.h"
#include "dev.h"
#include "common.h"
#include <cmd_def.h>


#define TIMER_COUNT_DOWN    (5*60*1000) //(ms)
#define TIME_STAMP_EXPIRE   (10*60*1000) //(ms)

#if (MLME_TASK==1)
struct task_info_st g_mlme_task_info[] =
{
    { "SSV_MLME",    (OsMsgQ)0, 8,   OS_MLME_TASK_PRIO,   MLME_TASK_STACK_SIZE, NULL, mlme_task},
};

s32 mlme_host_event_handler_(struct cfg_host_event *pPktInfo);
void sta_mode_handler(MsgEvent *pMsgEv); 
void ap_mode_handler(MsgEvent *pMsgEv);
#endif

extern void os_timer_init(void);
extern void _cmd_wifi_scan (s32 argc, char *argv[]);

void sta_mode_ap_list_handler (void *data,struct ssv6xxx_ieee80211_bss *ap_list);
void refresh_timer(void);
void sta_mode_scan_timer(void * data1,void * data2);
void update_ap_info_evt (u16 apIndex ,enum ap_act apAct);

void sta_mode_ap_list_handler (void *data,struct ssv6xxx_ieee80211_bss *ap_list)
{
    struct resp_evt_result *scan_res;
    struct ssv6xxx_ieee80211_bss *bss=NULL;
    u32    current_tick=OS_GetSysTick(); 
    u8     i;
	u8     oldest = 0;  
    u8     empty = 0xff;
    u8     duplicate = 0xff;

    if(data!=NULL)
    {
        scan_res = (struct resp_evt_result *)data;
        bss =&scan_res->u.scan.bss_info;
    }
	OS_MutexLock(gDeviceInfo->g_dev_info_mutex);
	/*get status*/
    for(i=0;i<NUM_AP_INFO;i++)
    {
        /*check empty by using channel id*/
        if(ap_list[i].channel_id==0)
        {
            empty=i;
        }
        /*check duplicate signal by using bssid*/
        else if((bss != NULL) && 
            MEMCMP((void*)(ap_list[i].bssid.addr), (void*)(bss->bssid.addr), ETHER_ADDR_LEN) == 0)
        {
            duplicate=i;
        }
        else
        {
            /*check whether time stamp is expire or not*/
            if(time_after((unsigned long)(current_tick),
                        (unsigned long)(ap_list[i].last_probe_resp+TIME_STAMP_EXPIRE))
                &&(STRCMP((char *)ap_list[i].ssid.ssid,(char *)gDeviceInfo->joincfg->bss.ssid.ssid)!=0))
            {
                update_ap_info_evt(i,AP_ACT_REMOVE_AP);
				MEMSET((void *)&(ap_list[i]),0,sizeof(struct ssv6xxx_ieee80211_bss));
				empty=i;
            }
            else
            {   
                /*check whether "i" is the oldest one*/
                if(time_after((unsigned long)(ap_list[oldest].last_probe_resp),
                    (unsigned long)(ap_list[i].last_probe_resp)))
                {
                  oldest=i;
                }
            }
        }
    }
	
    /*record status*/
    if(bss != NULL)
    { 
        /*priority : duplicate> empty> oldest*/
        if(duplicate!=0xff)
        {
            i=duplicate;
		}
        else if(empty!=0xff)
        {   
            i=empty;
        }
        else{
            i = oldest;
            update_ap_info_evt(i,AP_ACT_REMOVE_AP);
        }

		ap_list[i]=*bss;
        ap_list[i].last_probe_resp=current_tick;
		
		if(duplicate==0xff)
		{
			update_ap_info_evt(i,AP_ACT_ADD_AP);
		}
		else
		{
			if(ap_list[i].channel_id != bss->channel_id)
			{
				update_ap_info_evt(i,AP_ACT_MODIFY_AP);
			}
		}
    }

	OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex);
	
	refresh_timer();

    return;
} // end of - _scan_result_handler -
#if (CONFIG_AUTO_SCAN==0)
inline bool is_aplist_with_value(struct ssv6xxx_ieee80211_bss *ap_list)
{
    u8 i;
    for (i=0;i<NUM_AP_INFO;i++)
    {
        if (ap_list[i].channel_id)
        {
            return TRUE;
        }
    }
    return FALSE;
}
#endif

inline void refresh_timer(void)	// auto fluse should be stop when it is only one ap
{
    os_cancel_timer(sta_mode_scan_timer, (u32)NULL, (u32)NULL);
#if (CONFIG_AUTO_SCAN==0)
    if(is_aplist_with_value(gDeviceInfo->ap_list))
#endif
    {
        os_create_timer(TIMER_COUNT_DOWN, sta_mode_scan_timer, NULL, NULL, (void*)TIMEOUT_TASK);
    }
}

void sta_mode_scan_timer(void * data1,void * data2)
{
#if ( CONFIG_AUTO_SCAN == 1)
    /* do regular IW Scan*/
    char *fullCh = "0xfff";   // scan channel 1~11
    _cmd_wifi_scan((s32)1, &fullCh);
#else
    /*do regular refresh AP list table*/
    sta_mode_ap_list_handler(NULL,gDeviceInfo->ap_list);
#endif
}

inline void update_ap_info_evt (u16 apIndex ,enum ap_act apAct)
{   
    u8 i;

    ap_info_state apInfoState ;
    apInfoState.apInfo=gDeviceInfo->ap_list;
    apInfoState.index=apIndex;
    apInfoState.act=apAct;
	
    for (i=0;i<HOST_EVT_CB_NUM;i++)
    {
        evt_handler handler = gDeviceInfo->evt_cb[i];
        if (handler) {
            handler(SOC_EVT_SCAN_RESULT, &apInfoState,(s32)sizeof(ap_info_state));
        }
    }	
}

void mlme_sta_mode_deinit(void)
{

    /*stop scan timer*/
    os_cancel_timer(sta_mode_scan_timer, (u32)NULL, (u32)NULL);

    OS_MutexLock(gDeviceInfo->g_dev_info_mutex);

    /*clean gDeviceInfo ap_list record*/
    MEMSET((void *)&(gDeviceInfo->ap_list),0,sizeof(gDeviceInfo->ap_list));

    OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex);
}

#if(MLME_TASK==1)

void mlme_sta_mode_init(void)
{
    OS_MutexLock(gDeviceInfo->g_dev_info_mutex); 

    /*init gDeviceInfo ap_list*/
    MEMSET((void *)&(gDeviceInfo->ap_list),0,sizeof(gDeviceInfo->ap_list));

    OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex); 
}

s32 mlme_init(void)
{
    if (g_mlme_task_info[0].qlength> 0) {
        ASSERT(OS_MsgQCreate(&g_mlme_task_info[0].qevt,
        (u32)g_mlme_task_info[0].qlength)==OS_SUCCESS);
    }

    /* Create Registered Task: */
    OS_TaskCreate(g_mlme_task_info[0].task_func,
    g_mlme_task_info[0].task_name,
    g_mlme_task_info[0].stack_size<<4,
    g_mlme_task_info[0].args,
    g_mlme_task_info[0].prio,
    NULL);

	mlme_sta_mode_init();
	
	return OS_SUCCESS;
}

void mlme_task( void *args )
{
    s32 res,hw_mode;
    MsgEvent *pMsgEv = NULL;

    while(1)
    {
        res = msg_evt_fetch(MBOX_MLME_TASK, &pMsgEv);
        ASSERT(res == OS_SUCCESS);

		OS_MutexLock(gDeviceInfo->g_dev_info_mutex); 
		hw_mode=gDeviceInfo->hw_mode;
		OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex); 

		if(pMsgEv)
        {   
            if(hw_mode == SSV6XXX_HWM_STA)    
            {   
                sta_mode_handler(pMsgEv);
            }
            else    
            {
                ap_mode_handler(pMsgEv);
            }
            os_msg_free(pMsgEv);
        }
    }
}

inline void sta_mode_handler(MsgEvent *pMsgEv)
{
    void* pMsgData = NULL;

    switch(pMsgEv->MsgType)
    {    
        case MEVT_PKT_BUF:  
            switch(pMsgEv->MsgData1)
            {
                case SOC_EVT_SCAN_RESULT:
					pMsgData = (void*)pMsgEv->MsgData;
                    sta_mode_ap_list_handler(pMsgData,gDeviceInfo->ap_list);
                    FREE(pMsgData);
                    break;
                default:
                    break;
            }
            break;
        case MEVT_HOST_TIMER:
            {
                os_timer_expired((void *)pMsgEv);
            }
            break;
        default:
            break;
    }
}

inline void ap_mode_handler(MsgEvent *pMsgEv)
{

    switch(pMsgEv->MsgType)
    {    
        case MEVT_PKT_BUF:  
            AP_RxHandleAPMode(pMsgEv->MsgData);
            break;
        case MEVT_BCN_CMD:
            GenBeacon();
            break;
        case MEVT_HOST_TIMER:
        {
            os_timer_expired((void *)pMsgEv);
        }
            break;
        default:
            break;
    }


}

s32 mlme_host_event_handler_(struct cfg_host_event *pPktInfo)
{

    MsgEvent *pMsgEv= NULL;
    void     *pRecvData = NULL;
    u32 len = pPktInfo->len-sizeof(HDR_HostEvent);
 
    pRecvData =  MALLOC(len);
    MEMCPY(pRecvData,pPktInfo->dat,len);
        
    pMsgEv = msg_evt_alloc();
    pMsgEv->MsgType  = MEVT_PKT_BUF;    
    pMsgEv->MsgData  = (u32)pRecvData;
    pMsgEv->MsgData1 = SOC_EVT_SCAN_RESULT;
    pMsgEv->MsgData2 = 0;
    pMsgEv->MsgData3 = 0;
    msg_evt_post(MBOX_MLME_TASK, pMsgEv);

    return 0;
} // end of - _host_event_handler -

s32 ap_mlme_handler( void *frame, msgevt_type MsgType)
{
    MsgEvent *pMsgEv= NULL;
    pMsgEv = msg_evt_alloc();
    ASSERT(pMsgEv);
    pMsgEv->MsgType  = MsgType;
    pMsgEv->MsgData  = (u32)frame;
    pMsgEv->MsgData1 = 0;
    pMsgEv->MsgData2 = 0;
    pMsgEv->MsgData3 = 0;
    ASSERT(OS_SUCCESS == msg_evt_post(MBOX_MLME_TASK, pMsgEv));


    return SSV6XXX_SUCCESS;

} // end of - _host_event_handler -

#endif



