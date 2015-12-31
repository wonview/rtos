#include <config.h>
#include <cmd_def.h>
#include <log.h>
#include <host_global.h>
#include "lwip/netif.h"
#include <hdr80211.h>
#include "cli_cmd_wifi.h"
#include "cli_cmd_net.h"
#include <drv_mac.h>
#include <os_wrapper.h>
#include "dev.h"
#include <core/mlme.h>
#include "netmgr/net_mgr.h"


#define WPA_AUTH_ALG_OPEN BIT(0)
#define WPA_AUTH_ALG_SHARED BIT(1)
#define WPA_AUTH_ALG_LEAP BIT(2)
#define WPA_AUTH_ALG_FT BIT(3)

//#define SEC_USE_NONE
//#define SEC_USE_WEP40_PSK
//#define SEC_USE_WEP40_OPEN
//#define SEC_USE_WEP104_PSK
//#define SEC_USE_WEP104_OPEN
//#define SEC_USE_WPA_TKIP
#define SEC_USE_WPA2_CCMP


// iw command executioner
void _cmd_wifi_scan (s32 argc, char *argv[]);
void _cmd_wifi_sconfig (s32 argc, char *argv[]);
static void _cmd_wifi_join (s32 argc, char *argv[]);
static void _cmd_wifi_leave (s32 argc, char *argv[]);
static void _cmd_wifi_list (s32 argc, char *argv[]);
void _cmd_wifi_ap (s32 argc, char *argv[]);

#ifdef USE_CMD_RESP
static void _handle_cmd_resp (u32 evt_id, u8 *data, s32 len);
static void _deauth_handler (void *data);
#endif

// CB for host event
static void _host_event_handler(u32 evt_id, void *data, s32 len);
// Individual event handler
static void _scan_down_handler (void *data);
static void _sconfig_down_handler (void *data);
static void _scan_result_handler(void * data);

static void _join_result_handler (void *data);
static void _leave_result_handler (void *data);
static void _get_soc_reg_response(u32 eid, void *data, s32 len);
void _soc_evt_handler_ssv6xxx_log(u32 eid, void *data, s32 len);

typedef struct ba_session_st {
	u16 tid;
	u16 buf_size;
	u16 policy;
	u16 timeout;
} BA_SESSION ;


//static BA_SESSION g_ba_session;


#ifdef THROUGHPUT_TEST
extern void cmd_sdio_get_result_cb(u32 evt_id, void *data, s32 len);
#endif


#define ON_OFF_LAG_INTERVAL 1000


/*===================== Start of Get Command Handlers ====================*/


void _soc_evt_get_soc_status(void *data)
{
    char *sta_status[]={ "STA_STATE_UNAUTH_UNASSOC",/*STA_STATE_UNAUTH_UNASSOC*/
                        "STA_STATE_AUTHED_UNASSOC",/*STA_STATE_AUTHED_UNASSOC*/
                        "STA_STATE_AUTHED_ASSOCED",/*STA_STATE_AUTHED_ASSOCED*/
                        "STA_STATE_ASSOCED_4way",/*STA_STATE_ASSOCED_4way*/
                        };
    char *sta_action[]={
                        "STA_ACTION_INIT",/*STA_ACTION_INIT*/
                        "STA_ACTION_IDLE",/*STA_ACTION_IDLE*/
                        "STA_ACTION_READY",/*STA_ACTION_READY*/
                        "STA_ACTION_RUNNING",/*STA_ACTION_RUNNING*/
                        "STA_ACTION_SCANING",/*STA_ACTION_SCANING*/
                        "STA_ACTION_JOING",/*STA_ACTION_JOING*/
                        "STA_ACTION_JOING_4WAY",/*STA_ACTION_JOING_4WAY*/
                        "STA_ACTION_LEAVING" /*STA_ACTION_LEAVING*/
                        };

    struct ST_SOC_STATUS{
        u8  u8SocState;
        u32 u32SocAction;
    }*ps1=NULL;

    ps1=(struct ST_SOC_STATUS *)data;
    //LOG_PRINTF("u8SocState=%d, u32SocAction=%d\r\n",ps1->u8SocState,ps1->u32SocAction);
    LOG_PRINTF("\n  >> soc status:%s\r\n",sta_status[ps1->u8SocState]);
    LOG_PRINTF("\n  >> soc action:%s\r\n",sta_action[ps1->u32SocAction]);
}




#if 0

void _soc_evt_get_wsid_tbl(u32 evt_id, void *data, s32 len)
{
    struct mac_wsid_entry_st    *wsid_entry;
    s32 i;

    ASSERT(len == sizeof(struct mac_wsid_entry_st)*4);
    wsid_entry = (struct mac_wsid_entry_st *)data;
    LOG_PRINTF("  >> WSID Table:\n      ");
    for(i=0; i<4; i++) {
        if (GET_WSID_INFO_VALID(wsid_entry) == 0) {
            LOG_PRINTF("[%d]: Invalid\n      ", i);
            continue;
        }
        LOG_PRINTF("[%d]: OP Mode: %d, QoS: %s, HT: %s\n      ",
            i, GET_WSID_INFO_OP_MODE(wsid_entry),
            ((GET_WSID_INFO_QOS_EN(wsid_entry)==0)? "disable": "enable"),
            ((GET_WSID_INFO_HT_MODE(wsid_entry)==0)? "disable": "enable")
        );
        LOG_PRINTF("      STA-MAC: %02x:%02x:%02x:%02x:%02x:%02x\n      ",
            wsid_entry->sta_mac.addr[0], wsid_entry->sta_mac.addr[1],
            wsid_entry->sta_mac.addr[2], wsid_entry->sta_mac.addr[3],
            wsid_entry->sta_mac.addr[4], wsid_entry->sta_mac.addr[5]
        );
    }
}


void _soc_evt_get_addba_req(u32 evt_id, void *data, s32 len)
{

	struct cfg_addba_resp *addba_resp;
    struct resp_evt_result *rx_addba_req = (struct resp_evt_result *)data;
	g_ba_session.policy=rx_addba_req->u.addba_req.policy;
	g_ba_session.tid=rx_addba_req->u.addba_req.tid;
	g_ba_session.buf_size=rx_addba_req->u.addba_req.agg_size;
	g_ba_session.timeout=rx_addba_req->u.addba_req.timeout;



    addba_resp = (void *)MALLOC (sizeof(struct cfg_addba_resp));
	addba_resp->dialog_token=1;//spec mention to set nonzero value
	addba_resp->policy=g_ba_session.policy;
	addba_resp->tid=g_ba_session.tid;
	addba_resp->buf_size=g_ba_session.buf_size;
	addba_resp->timeout=g_ba_session.timeout;
	addba_resp->status=0;
	addba_resp->start_seq_num=rx_addba_req->u.addba_req.start_seq_num;


    if (ssv6xxx_wifi_send_addba_resp(addba_resp) < 0)
       	LOG_PRINTF("Command failed !!\n");

    FREE(addba_resp);

}


void _soc_evt_get_delba(u32 evt_id, void *data, s32 len)
{
    struct resp_evt_result *rx_delba = (struct resp_evt_result *)data;
	LOG_PRINTF("RCV DELBA: reason:%d\n",rx_delba->u.delba_req.reason_code);
	ssv6xxx_memset((void *)&g_ba_session,0x00,sizeof(BA_SESSION));

}

#endif


/* ====================== End of Get Command Handlers ====================*/



//-------------------------------------------------------------------------------------

extern void cmd_loop_pattern(void);



void _host_event_handler(u32 evt_id, void *data, s32 len)
{
    switch (evt_id) {
    case SOC_EVT_LOG:
        //LOG_PRINTF("SOC_EVT_LOG\n");
        //_soc_evt_handler_ssv6xxx_log(evt_id, data, len);
        break;
    case SOC_EVT_SCAN_RESULT:
        _scan_result_handler(data);
        break;
    case SOC_EVT_SCAN_DONE:
        _scan_down_handler(data);
        break;
    case SOC_EVT_SCONFIG_DONE:
        _sconfig_down_handler(data);
        break;

    #ifdef USE_CMD_RESP
    case SOC_EVT_CMD_RESP:
        _handle_cmd_resp(evt_id, data, len);
        break;
    case SOC_EVT_DEAUTH:
        _deauth_handler(data);
        break;
    #else // USE_CMD_RESP
    case SOC_EVT_JOIN_RESULT:
        _join_result_handler(data);
        break;
    case SOC_EVT_LEAVE_RESULT:
        _leave_result_handler(data);
        break;
    case SOC_EVT_GET_REG_RESP:
        _get_soc_reg_response(evt_id, data, len);
        break;

    /*=================================================*/
    #endif // USE_CMD_RESP
	
	//sending arp request event
    case SOC_EVT_POLL_STATION:
        break;
    
    default:
        LOG_PRINTF("Unknown host event received. %d\r\n", evt_id);
        break;
    }
} // end of - _host_event_handler -


#ifdef USE_CMD_RESP
void _handle_cmd_resp (u32 evt_id, u8 *data, s32 len)
{
	struct resp_evt_result *resp = (struct resp_evt_result *)data;

	if (resp->result != CMD_OK)
	{
		LOG_PRINTF("Command %d is not OK with code %d.\n", resp->cmd, resp->result);
		return;
    }
    switch (resp->cmd)
        {
        case SSV6XXX_HOST_CMD_SCAN:
			LOG_PRINTF("Scan done.\n");
            break;
        case SSV6XXX_HOST_CMD_JOIN:
            _join_result_handler(data);
            break;
        case SSV6XXX_HOST_CMD_LEAVE:
            _leave_result_handler(data);
            break;
        }

} // end of - _handle_cmd_resp -

void _deauth_handler (void *data)
{
    struct resp_evt_result *leave_res = (struct resp_evt_result *)data;
	LOG_PRINTF("Deauth from AP (reason=%d) !!\n", leave_res->u.leave.reason_code);
    netmgr_netif_link_set(LINK_DOWN);
}

#endif // USE_CMD_RESP

static void _scan_down_handler (void *data)
{
    struct resp_evt_result *scan_done = (struct resp_evt_result *)data;
    if(scan_done->u.scan_done.result_code==0){
        LOG_PRINTF("Scan Done\r\n");
    }else{
        LOG_PRINTF("Scan FAIL\r\n");
    }
    return;
}
static void _sconfig_down_handler (void *data)
{
    struct resp_evt_result *sconfig_done = (struct resp_evt_result *)data;

    if(sconfig_done->u.sconfig_done.result_code==0){
        LOG_PRINTF("SconfigDone. SSID:%s, PWD:%s , rand=%d\r\n",sconfig_done->u.sconfig_done.ssid,sconfig_done->u.sconfig_done.pwd,sconfig_done->u.sconfig_done.rand);
    }else{
        LOG_PRINTF("Sconfig FAIL\r\n");
    }
    return;
}

void _scan_result_handler(void *data)
{
	s32	pairwise_cipher_index=0,group_cipher_index=0;
	u8		sec_str[][7]={"OPEN","WEP40","WEP104","TKIP","CCMP"};

    ap_info_state *scan_res = (ap_info_state*)data;

    char *act = NULL;
    char act_str[][7]={"Remove","Add","Modify"};
    act = act_str[scan_res->act];
    LOG_DEBUGF(LOG_L2_STA, ("Action: %s AP Info:\r\n",act));
    LOG_DEBUGF(LOG_L2_STA, ("BSSID: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
               scan_res->apInfo[scan_res->index].bssid.addr[0], scan_res->apInfo[scan_res->index].bssid.addr[1],
			   scan_res->apInfo[scan_res->index].bssid.addr[2], scan_res->apInfo[scan_res->index].bssid.addr[3],
			   scan_res->apInfo[scan_res->index].bssid.addr[4], scan_res->apInfo[scan_res->index].bssid.addr[5]));
    LOG_DEBUGF(LOG_L2_STA, ("SSID: %s \t", scan_res->apInfo[scan_res->index].ssid.ssid));
    LOG_DEBUGF(LOG_L2_STA, ("@Channel Idx: %d\r\n", scan_res->apInfo[scan_res->index].channel_id));
    	
    if(scan_res->apInfo[scan_res->index].capab_info&BIT(4))
    {
        LOG_DEBUGF(LOG_L2_STA, ("Secure Type=[%s]\r\n",
                   scan_res->apInfo[scan_res->index].proto&WPA_PROTO_WPA?"WPA":
                   scan_res->apInfo[scan_res->index].proto&WPA_PROTO_RSN?"WPA2":"WEP"));


        if(scan_res->apInfo[scan_res->index].pairwise_cipher[0])
        {
            pairwise_cipher_index=0;
            LOG_DEBUGF(LOG_L2_STA, ("Pairwise cipher="));
            
            for(pairwise_cipher_index=0;pairwise_cipher_index<8;pairwise_cipher_index++)
            {
                if(scan_res->apInfo[scan_res->index].pairwise_cipher[0]&BIT(pairwise_cipher_index))
                {
                    LOG_DEBUGF(LOG_L2_STA, ("[%s] ",sec_str[pairwise_cipher_index]));
                }
            }
            LOG_DEBUGF(LOG_L2_STA, ("\r\n"));
        }
        if(scan_res->apInfo[scan_res->index].group_cipher)
        {
            group_cipher_index=0;
            LOG_DEBUGF(LOG_L2_STA, ("Group cipher="));
            for(group_cipher_index=0;group_cipher_index<8;group_cipher_index++)
            {
                if(scan_res->apInfo[scan_res->index].group_cipher&BIT(group_cipher_index))
                {
                    LOG_DEBUGF(LOG_L2_STA, ("[%s] ",sec_str[group_cipher_index]));
                }
            }
            LOG_DEBUGF(LOG_L2_STA, ("\r\n"));
            }
        }
    else
    {
        LOG_DEBUGF(LOG_L2_STA, ("Secure Type=[NONE]\r\n"));
    }
    LOG_DEBUGF(LOG_L2_STA, ("RCPI=%d\r\n",scan_res->apInfo[scan_res->index].rxphypad.rpci));
    LOG_DEBUGF(LOG_L2_STA, ("\r\n"));
}

void _join_result_handler (void *data)
{
    struct resp_evt_result *join_res = (struct resp_evt_result *)data;
    if (join_res->u.join.status_code != 0)
    {
        LOG_PRINTF("Join failure!!\r\n");
		return;
    }

    LOG_PRINTF("Join success!!\r\n");
    LOG_DEBUGF(LOG_L2_STA, ("Join AID=%d\r\n",join_res->u.join.aid));
	//ssv6xxx_wifi_apply_security();
    netmgr_netif_link_set(LINK_UP);

#if 0
			netif_add(&wlan, &ipaddr, &netmask, &gw, NULL, ethernetif_init, tcpip_input);
			netif_set_default(&wlan);
			netif_set_up(&wlan);
			dhcp_start(&wlan);
		   for(i=0;i<6;i++)
		   {
			 wlan.hwaddr[ i ] = MAC_ADDR[i];


		   }
			while (wlan.ip_addr.addr==0) {
				sys_msleep(DHCP_FINE_TIMER_MSECS);
				dhcp_fine_tmr();
				mscnt += DHCP_FINE_TIMER_MSECS;
			 if (mscnt >= DHCP_COARSE_TIMER_SECS*1000) {
				dhcp_coarse_tmr();
				mscnt = 0;
			 }
		  }
#endif
} // end of - _join_result_handler -


void _leave_result_handler (void *data)
{
    struct resp_evt_result *leave_res = (struct resp_evt_result *)data;
    LOG_PRINTF("Leave received deauth from AP!!\r\n");
    LOG_DEBUGF(LOG_L2_STA, ("Reason Code=%d\r\n",leave_res->u.leave.reason_code));
    //netmgr_netif_link_set(LINK_DOWN);
}


void _get_soc_reg_response(u32 eid, void *data, s32 len)
{
    LOG_PRINTF("%s(): HOST_EVENT=%d: len=%d\n", __FUNCTION__, eid, len);
//    memcpy((void *)g_soc_cmd_rx_buffer, (void *)data, len);
//    g_soc_cmd_rx_ready = 1;
}

void cmd_iw(s32 argc, char *argv[])
{
//    ssv6xxx_wifi_reg_evt_cb(_host_event_handler);
//gHCmdEngInfo

	if (argc<2)
		return;


    if (strcmp(argv[1], "scan")==0) {
        if (argc >= 3)
		    _cmd_wifi_scan(argc - 2, &argv[2]);
        else
            LOG_PRINTF("Invalid arguments.\n");
#if(ENABLE_SMART_CONFIG==1)
	}else if (strcmp(argv[1], "sconfig")==0) {
        if (argc >= 3)
		    _cmd_wifi_sconfig(argc - 2, &argv[2]);
        else
            LOG_PRINTF("Invalid arguments.\n");
#endif
	} else if (strcmp(argv[1], "join")==0) {
        if (argc >= 3)
            _cmd_wifi_join(argc - 2, &argv[2]);
        else
            LOG_PRINTF("Invalid arguments.\n");
    } else if (strcmp(argv[1], "leave")==0) {
        if (argc==2)
            _cmd_wifi_leave(argc - 2, &argv[2]);
        else
            LOG_PRINTF("Invalid arguments.\n");
    } else if (strcmp(argv[1], "list")==0) {
        if (argc == 2)
        {
            _cmd_wifi_list(argc - 2, &argv[2]);
        }
        else
            LOG_PRINTF("Invalid arguments.\n");
        /*
    } else if (strcmp(argv[1], "ap")==0) {
        if (argc >= 3)
            _cmd_wifi_ap(argc - 2, &argv[2]);
		else
            LOG_PRINTF("Invalid arguments.\n");
        */
    }
	else {
        LOG_PRINTF("Invalid iw command.\n");
    }
} // end of - cmd_iw -

void _cmd_wifi_sconfig (s32 argc, char *argv[])
{
    /**
     *  sconfig Command Usage:
     *  iw sconfig <chan_mask>
     */
    u16 channel = (u16)strtol(argv[0],NULL,16);

    netmgr_wifi_sconfig_async(channel);
} // end of - _cmd_wifi_sconfig -
void _cmd_wifi_scan (s32 argc, char *argv[])
{
    /**
     *  Scan Command Usage:
     *  iw scan <chan_mask> <ssid0> <ssid1> <ssid2> ...
     */
    int num_ssids = argc - 1;
    u16 channel = (u16)strtol(argv[0],NULL,16);

    if (num_ssids > 0)
    {
        netmgr_wifi_scan_async(channel, &(argv[1]), num_ssids);
    }
    else
    {
        netmgr_wifi_scan_async(channel, NULL, 0);
    }
} // end of - _cmd_wifi_scan -


//iw join ap_name [wep|wpa|wpa2] passwd
void _cmd_wifi_join (s32 argc, char *argv[])
{
    /**
	 * Join Command Usage:
	 *	iw join ssid
     */
	int ret = -1;
    wifi_sta_join_cfg *join_cfg = NULL;

    join_cfg = MALLOC(sizeof(wifi_sta_join_cfg));

    if (argc > 0)
    {
        STRCPY((const char *)join_cfg->ssid.ssid, argv[0]);

        if (argc == 2)
        {
            STRCPY((const char *)join_cfg->password, argv[1]);
        }
    }
    else
    {
        FREE(join_cfg);
        return;
    }

    ret = netmgr_wifi_join_async(join_cfg);
    if (ret != 0)
    {
	    LOG_PRINTF("netmgr_wifi_join_async failed !!\r\n");
    }

    FREE(join_cfg);
} // end of - _cmd_wifi_join -

void _cmd_wifi_leave(s32 argc, char *argv[])
{
    /**
	 *	Leave Command Usage:
	 *	host leave ... ...
	 */
	int ret = -1;

    ret = netmgr_wifi_leave_async();
    if (ret != 0)
    {
	    LOG_PRINTF("netmgr_wifi_leave failed !!\r\n");
    }
} // end of - _cmd_wifi_leave -

void _cmd_wifi_list(s32 argc, char *argv[])
{
    u32 i=0;
    s32     pairwise_cipher_index=0,group_cipher_index=0;
	u8      sec_str[][7]={"OPEN","WEP40","WEP104","TKIP","CCMP"};
    DeviceInfo_st *gdeviceInfo = gDeviceInfo ;

	LOG_PRINTF("\r\n");
	OS_MutexLock(gDeviceInfo->g_dev_info_mutex);

	for (i=0; i<NUM_AP_INFO; i++)
    {
        if(gdeviceInfo->ap_list[i].channel_id!= 0)
		{
		    LOG_PRINTF("BSSID: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
            gdeviceInfo->ap_list[i].bssid.addr[0],  gdeviceInfo->ap_list[i].bssid.addr[1], gdeviceInfo->ap_list[i].bssid.addr[2],  gdeviceInfo->ap_list[i].bssid.addr[3],  gdeviceInfo->ap_list[i].bssid.addr[4],  gdeviceInfo->ap_list[i].bssid.addr[5]);
            LOG_PRINTF("SSID: %s\t", gdeviceInfo->ap_list[i].ssid.ssid);
			LOG_PRINTF("@Channel Idx: %d\r\n", gdeviceInfo->ap_list[i].channel_id);
            if(gdeviceInfo->ap_list[i].capab_info&BIT(4)){
                LOG_PRINTF("Secure Type=[%s]\r\n",
                gdeviceInfo->ap_list[i].proto&WPA_PROTO_WPA?"WPA":
                gdeviceInfo->ap_list[i].proto&WPA_PROTO_RSN?"WPA2":"WEP");

                if(gdeviceInfo->ap_list[i].pairwise_cipher[0]){
                    pairwise_cipher_index=0;
                    LOG_PRINTF("Pairwise cipher=");
                    for(pairwise_cipher_index=0;pairwise_cipher_index<8;pairwise_cipher_index++){
                        if(gdeviceInfo->ap_list[i].pairwise_cipher[0]&BIT(pairwise_cipher_index)){
                            LOG_PRINTF("[%s] ",sec_str[pairwise_cipher_index]);
                        }
                    }
                    LOG_PRINTF("\r\n");
                }
                if(gdeviceInfo->ap_list[i].group_cipher){
                    group_cipher_index=0;
                    LOG_PRINTF("Group cipher=");
                    for(group_cipher_index=0;group_cipher_index<8;group_cipher_index++){
                        if(gdeviceInfo->ap_list[i].group_cipher&BIT(group_cipher_index)){
                            LOG_PRINTF("[%s] ",sec_str[group_cipher_index]);
                        }
                    }
                    LOG_PRINTF("\r\n");
                }
            }else{
                LOG_PRINTF("Secure Type=[NONE]\r\n");
            }
            LOG_PRINTF("RCPI=%d\r\n",gdeviceInfo->ap_list[i].rxphypad.rpci);
            LOG_PRINTF("\r\n");
		}
	}
	OS_MutexUnLock(gDeviceInfo->g_dev_info_mutex);
} // end of - _cmd_wifi_list -




// iw ap ssid sectype password
// iw ap ssv  password
void _cmd_wifi_ap (s32 argc, char *argv[])
{
    /**
	 * Join Command Usage:
	 *	iw ap ssid
     */
    const char *sec_name;
	struct cfg_set_ap_cfg ApCfg;
    MEMSET(&ApCfg, 0, sizeof(struct cfg_set_ap_cfg));

//Fill SSID
    ApCfg.ssid.ssid_len = strlen(argv[0]);
    memcpy( (void*)&ApCfg.ssid.ssid, (void*)argv[0], strlen(argv[0]));

//Fill PASSWD
	if (argc == 3)
		memcpy(&ApCfg.password, argv[2], strlen(argv[2]));


    if (argc == 1){
        sec_name = "open";
        ApCfg.sec_type = SSV6XXX_SEC_NONE;
    }
	else if (strcmp(argv[1], "wep40") == 0){
        sec_name = "wep40";
    	ApCfg.sec_type = SSV6XXX_SEC_WEP_40;
		if (argc != 3)
		{
	        ApCfg.password[0]= 0x31;
			ApCfg.password[1]= 0x32;
    		ApCfg.password[2]= 0x33;
	    	ApCfg.password[3]= 0x34;
			ApCfg.password[4]= 0x35;
    		ApCfg.password[5]= '\0';
		}


	}
	else if (strcmp(argv[1], "wep104") == 0){
			sec_name = "wep104";

			ApCfg.sec_type = SSV6XXX_SEC_WEP_104;
			if (argc != 3)
			{
				ApCfg.password[0]= '0';
				ApCfg.password[1]= '1';
				ApCfg.password[2]= '2';
				ApCfg.password[3]= '3';
				ApCfg.password[4]= '4';
				ApCfg.password[5]= '5';
				ApCfg.password[6]= '6';
				ApCfg.password[7]= '7';
				ApCfg.password[8]= '8';
				ApCfg.password[9]= '9';
				ApCfg.password[10]= '0';
				ApCfg.password[11]= '1';
				ApCfg.password[12]= '2';
				ApCfg.password[13]= '\0';
			}
    }
	else if (strcmp(argv[1], "wpa2") == 0){
        sec_name = "wpa2";
	  	ApCfg.sec_type = SSV6XXX_SEC_WPA2_PSK;

		if (argc != 3)
		{
	        ApCfg.password[0]= 's';
		    ApCfg.password[1]= 'e';
	       	ApCfg.password[2]= 'c';
		    ApCfg.password[3]= 'r';
			ApCfg.password[4]= 'e';
			ApCfg.password[5]= 't';
	        ApCfg.password[6]= '0';
		    ApCfg.password[7]= '0';
			ApCfg.password[8]= '\0';
		}

    }
	else if (strcmp(argv[1], "wpa") == 0){

	    sec_name = "wpa";
	    ApCfg.sec_type = SSV6XXX_SEC_WPA_PSK;

		if (argc != 3)
		{
			ApCfg.password[0]= 's';
			ApCfg.password[1]= 'e';
			ApCfg.password[2]= 'c';
			ApCfg.password[3]= 'r';
			ApCfg.password[4]= 'e';
			ApCfg.password[5]= 't';
			ApCfg.password[6]= '0';
			ApCfg.password[7]= '0';
		    ApCfg.password[8]= '\0';
		}

    }
	else{
        LOG_PRINTF("ERROR: unkown security type: %s\n", argv[1]);
        sec_name = "open";
        //ApCfg.auth_alg = WPA_AUTH_ALG_OPEN;
        ApCfg.sec_type = SSV6XXX_SEC_NONE;
    }


    LOG_PRINTF("AP configuration==>\nSSID:\"%s\" \nSEC Type:\"%s\" \nPASSWD:\"%s\"\n",
		ApCfg.ssid.ssid, sec_name, ApCfg.password);



    if (ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_SET_AP_CFG, &ApCfg, sizeof(ApCfg)) < 0)
	    LOG_PRINTF("Command failed !!\n");


} // end of - _cmd_wifi_join -




void ssv6xxx_wifi_cfg(void)
{
    ssv6xxx_wifi_reg_evt_cb(_host_event_handler);
}

void cmd_ctl(s32 argc, char *argv[])
{

    bool errormsg = FALSE;
    char *err_str = "";
    if (argc <= 1)
    {
        errormsg = TRUE;
    }
	else if (strcmp(argv[1], "status")==0)
    {
        Ap_sta_status info;
        MEMSET(&info , 0 , sizeof(Ap_sta_status));
        errormsg = FALSE;
        ssv6xxx_wifi_status(&info);
        if(info.status)
            LOG_PRINTF("status:ON\r\n");
        else
            LOG_PRINTF("status:OFF\r\n");
        if((SSV6XXX_HWM_STA==info.operate)||(SSV6XXX_HWM_SCONFIG==info.operate))
        {
            LOG_PRINTF("Mode:%s, %s\r\n",(SSV6XXX_HWM_STA==info.operate)?"Station":"Sconfig",(info.u.station.apinfo.status == CONNECT) ? "connected" :"disconnected");
            LOG_PRINTF("self Mac addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                info.u.station.selfmac[0],
                info.u.station.selfmac[1],
                info.u.station.selfmac[2],
                info.u.station.selfmac[3],
                info.u.station.selfmac[4],
                info.u.station.selfmac[5]);
            LOG_PRINTF("SSID:%s\r\n",info.u.station.ssid.ssid);
            LOG_PRINTF("channel:%d\r\n",info.u.station.channel);
            LOG_PRINTF("AP Mac addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                info.u.station.apinfo.Mac[0],
                info.u.station.apinfo.Mac[1],
                info.u.station.apinfo.Mac[2],
                info.u.station.apinfo.Mac[3],
                info.u.station.apinfo.Mac[4],
                info.u.station.apinfo.Mac[5]);


        }
        else if(SSV6XXX_HWM_AP==info.operate)
        {
            u32 statemp;
            LOG_PRINTF("Mode:AP\r\n");
            LOG_PRINTF("self Mac addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                info.u.ap.selfmac[0],
                info.u.ap.selfmac[1],
                info.u.ap.selfmac[2],
                info.u.ap.selfmac[3],
                info.u.ap.selfmac[4],
                info.u.ap.selfmac[5]);
            LOG_PRINTF("SSID:%s\r\n",info.u.ap.ssid.ssid);
            LOG_PRINTF("channel:%d\r\n",info.u.ap.channel);
            LOG_PRINTF("Station number:%d\r\n",info.u.ap.stanum);
            for(statemp=0; statemp < info.u.ap.stanum ;statemp ++ )
            {
                LOG_PRINTF("station Mac addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                    info.u.ap.stainfo[statemp].Mac[0],
                    info.u.ap.stainfo[statemp].Mac[1],
                    info.u.ap.stainfo[statemp].Mac[2],
                    info.u.ap.stainfo[statemp].Mac[3],
                    info.u.ap.stainfo[statemp].Mac[4],
                    info.u.ap.stainfo[statemp].Mac[5]);
            }
            APStaInfo_PrintStaInfo();
        }
    }
    else if (strcmp(argv[1], "ap")==0&&argc >= 3)
    {
        Ap_setting ap;
        MEMSET(&ap , 0 , sizeof(Ap_setting));
        ap.channel =AP_DEFAULT_CHANNEL;
        //instruction dispatch
        // ctl ap on [ap_name] [channel] [security] [password] 
        switch(argc)
        {
            #if 0
            case 3: // wifi ap off
                if(strcmp(argv[2], "off")==0)
                {
                    errormsg =FALSE;
                    ap.status = FALSE;
                }

                break;
            #endif     
            case 4: // only ssid , security open , //ctl ap on [ap_name] 
                if((strcmp(argv[2], "on" )== 0)&&(strlen(argv[3])<32))
                {
                    errormsg =FALSE;
                    ap.status = TRUE;
                    ap.security = SSV6XXX_SEC_NONE;
                    ap.ssid.ssid_len = strlen(argv[3]);
                    MEMCPY( (void*)ap.ssid.ssid, (void*)argv[3], strlen(argv[3]));
                }
                else
                {
                    errormsg = TRUE;
                    err_str = "SSID is too long";    
                }
                break;

            case 5: // only ssid , security open, set channel, // ctl ap on [ap_name] [channel]
                    if((strcmp(argv[2], "on" )== 0)&&(0<strtol(argv[4],NULL,10))&&
                        (strtol(argv[4],NULL,10)<=14)&&(strlen(argv[3])<32)) 
                    {
                        errormsg =FALSE;
                        ap.status = TRUE;
                        ap.security = SSV6XXX_SEC_NONE;
                        ap.ssid.ssid_len = strlen(argv[3]);
                        ap.channel = (u8)strtol(argv[4],NULL,10);
                        MEMCPY( (void*)ap.ssid.ssid, (void*)argv[3], strlen(argv[3]));
                    }
                    else
                    {
                        errormsg = TRUE;
                        err_str = "Channel invalid/ SSID is too long";
                    }
                    break;


            case 6: //have security type // ctl ap on [ap_name] [security] [password] 
                if(strcmp(argv[2], "on") == 0&&(strlen(argv[3])<32))
                {
                    if(strcmp(argv[4], "wep") == 0)
                    {
                        errormsg =FALSE;
                        ap.status = TRUE;
                        if(strlen(argv[5]) == 5)
                            ap.security = 	SSV6XXX_SEC_WEP_40;
                        else if (strlen(argv[5]) == 13)
                            ap.security = 	SSV6XXX_SEC_WEP_104;
                        else
                        {
                            LOG_PRINTF("WEP key length must be 5 or 13 character. \r\n");
                            errormsg =TRUE;
                            break;
                        }
                    }
                    else
                    {
                        LOG_PRINTF("SSID:%s, Security type:%s, Password:%s. \r\n",argv[3],argv[4],argv[5] );
                        errormsg =TRUE;
                        break;                    
                        
                    }
                    STRCPY((const char *)ap.password, argv[5]);
                    ap.ssid.ssid_len = strlen(argv[3]);
                    MEMCPY( (void*)ap.ssid.ssid, (void*)argv[3], strlen(argv[3]));

                }
                break;
            case 7: //have security type and channel setting // ctl ap on [ap_name] [channel] [security] [password] 
                if(strcmp(argv[2], "on") == 0&&(0<strtol(argv[4],NULL,10))&&
                        (strtol(argv[4],NULL,10)<=14)&&(strlen(argv[3])<32))
                {
                    if(strcmp(argv[5], "wep") == 0)
                    {
                        errormsg =FALSE;
                        ap.status = TRUE;
                        if(strlen(argv[6]) == 5)
                            ap.security = 	SSV6XXX_SEC_WEP_40;
                        else if (strlen(argv[6]) == 13)
                            ap.security = 	SSV6XXX_SEC_WEP_104;
                        else
                        {
                            LOG_PRINTF("WEP key length must be 5 or 13 character. \r\n");
                            errormsg =TRUE;
                        }
                    }
                    else
                    {
                        LOG_PRINTF("SSID:%s, channel:%d, Security type:%s, Password:%s. \r\n",argv[3],strtol(argv[4],NULL,10),argv[5],argv[6] );
                        errormsg =TRUE;
                        break;                   
                        
                    }
                    STRCPY((const char *)ap.password, argv[6]);
                    ap.ssid.ssid_len = strlen(argv[3]);
                    MEMCPY( (void*)ap.ssid.ssid, (void*)argv[3], strlen(argv[3]));
                    ap.channel = (u8)strtol(argv[4],NULL,10);

                }
                break;
            default:
                errormsg = TRUE;
                break;


        }
        if(!errormsg)
        {
	        if (netmgr_wifi_control_async(SSV6XXX_HWM_AP, &ap, NULL) == SSV6XXX_FAILED)
                errormsg =  TRUE;
            else
                errormsg = FALSE;
        }
        else
        {
            LOG_PRINTF("Invalid wifictl command. %s\r\n",err_str);
        }


	}
    else if (strcmp(argv[1], "sta")==0&&argc >= 3)
	{
        Sta_setting sta;
        MEMSET(&sta, 0 , sizeof(Sta_setting));

        if(strcmp(argv[2], "on") == 0)
        {
            sta.status = TRUE;
        }
        else
        {
            #if 0
            sta.status = FALSE;
            #endif
            errormsg = TRUE;
        }

        if (!errormsg)
        {
            if (netmgr_wifi_control_async(SSV6XXX_HWM_STA, NULL, &sta) == SSV6XXX_FAILED)
                errormsg =  TRUE;
            else
                errormsg = FALSE;
        }


    }
#if(ENABLE_SMART_CONFIG==1)
    else if (strcmp(argv[1], "sconfig")==0&&argc >= 3)
	{
        Sta_setting sta;
        MEMSET(&sta, 0 , sizeof(Sta_setting));

        if(strcmp(argv[2], "on") == 0)
        {
            sta.status = TRUE;
        }
        else
        {
            #if 0
            sta.status = FALSE;
            #endif
            errormsg = TRUE;
        }

        if (!errormsg)
        {
            if (netmgr_wifi_control_async(SSV6XXX_HWM_SCONFIG, NULL, &sta) == SSV6XXX_FAILED)
                errormsg =  TRUE;
            else
                errormsg = FALSE;
        }

    }
#endif
    else if (strcmp(argv[1], "off")==0)
	{
        Sta_setting sta;
        MEMSET(&sta, 0 , sizeof(Sta_setting));
        sta.status = FALSE;
        
        if (!errormsg)
        {
            if (netmgr_wifi_control_async(SSV6XXX_HWM_STA, NULL, &sta) == SSV6XXX_FAILED)
                errormsg =  TRUE;
            else
                errormsg = FALSE;
        }
    }
    else if (strcmp(argv[1], "test")==0 && argc >= 4)
    {
        u32 r;
        u32 maxcouunt = (u32)ssv6xxx_atoi_base(argv[3], 16);
        u32 testcount=1;

        Sta_setting sta;
        Ap_setting ap;
        MEMSET(&sta, 0 , sizeof(Sta_setting));
        MEMSET(&ap, 0 , sizeof(Ap_setting));


        ap.security = SSV6XXX_SEC_NONE;
        ap.ssid.ssid_len = strlen(argv[2]);
        ap.channel =AP_DEFAULT_CHANNEL;
        MEMCPY( (void*)ap.ssid.ssid, (void*)argv[2], strlen(argv[2]));

        errormsg =FALSE;

        sta.status = FALSE;
        ssv6xxx_wifi_station(SSV6XXX_HWM_STA,&sta);
        ap.status = FALSE;
        ssv6xxx_wifi_ap(&ap);


        for(testcount=1 ;testcount <= maxcouunt;testcount++)
        {
            r = OS_Random();
            if(r&0x0001 == 1)//ap mode
            {

                ap.status = TRUE;
                if(ssv6xxx_wifi_ap(&ap) == SSV6XXX_SUCCESS)
                    LOG_PRINTF("No.%d:\tAP off.\r\n",testcount);
                Sleep(ON_OFF_LAG_INTERVAL);
                ap.status = FALSE;
                if(ssv6xxx_wifi_ap(&ap) == SSV6XXX_SUCCESS)
                    LOG_PRINTF("No.:%d\tAP on.\r\n",testcount);

            }
            else //station mode
            {
                sta.status = TRUE;
                if(ssv6xxx_wifi_station(SSV6XXX_HWM_STA,&sta) == SSV6XXX_SUCCESS)
                    LOG_PRINTF("No.%d:\tSta on.\r\n",testcount);

                Sleep(ON_OFF_LAG_INTERVAL);

                sta.status = FALSE;
                if(ssv6xxx_wifi_station(SSV6XXX_HWM_STA,&sta) == SSV6XXX_SUCCESS)
                    LOG_PRINTF("No.%d:\tSta off.\r\n",testcount);
            }
        }

    }else{
        LOG_PRINTF("Invalid wifictl command.\r\n");
    }

	if(FALSE==errormsg)
    {
        LOG_PRINTF("OK.\r\n");
    }
} // end of - cmd_iw -


