#define __SFILE__ "ap_sta_info.c"

#include <config.h>


#include "common/ether.h"

#include "ap_sta_info.h"
#include <os_wrapper.h>
#include "ap_info.h"
#include "ap_def.h"
#include "beacon.h"
#include "common/bitops.h"
#include <log.h>
#include "common/ap_common.h"
#include "ap_mlme.h"

#include <host_apis.h>
#include "ap_tx.h"

#include "common/ieee802_11_defs.h"
#include "common/ieee80211.h"
#include "ap_drv_cmd.h"
#include "ssv_timer.h"
#include "dev.h"
#include <txrx_hdl.h>




void APStaInfo_free(ApInfo_st *pApInfo, APStaInfo_st *sta);
#ifdef __AP_DEBUG__
	void APStaInfo_PrintStaInfo(void);
#endif//__AP_DEBUG__



#define AP_STA_CONNECTED "AP-STA-CONNECTED "
#define AP_STA_DISCONNECTED "AP-STA-DISCONNECTED "

//--------------------------------------------------------------
//struct list_q;
struct ap_tx_desp;
extern void ap_release_tx_desp_and_frame(struct ap_tx_desp *tx_desp);
extern struct ap_tx_desp *ap_tx_desp_peek(const struct list_q *list_);

extern u8* ssv6xxx_host_tx_req_get_qos_ptr(struct cfg_host_txreq0 *req0);
extern u8* ssv6xxx_host_tx_req_get_data_ptr(struct cfg_host_txreq0 *req0);
extern void purge_sta_ps_buffers(APStaInfo_st *pAPStaInfo);
//extern void purge_all_bc_buffers();


static inline bool __bss_tim_get(u16 aid)
{
	/*
	 * This format has been mandated by the IEEE specifications,
	 * so this line may not be changed to use the __set_bit() format.
	 */
	return !!(gDeviceInfo->APInfo->tim[aid / 8] & (1 << (aid % 8)));
}




static inline void __bss_tim_set(u16 aid)
{
	/*
	 * This format has been mandated by the IEEE specifications,
	 * so this line may not be changed to use the __set_bit() format.
	 */
	gDeviceInfo->APInfo->tim[aid / 8] |= (1 << (aid % 8));
}

static inline void __bss_tim_clear(u16 aid)
{
	/*
	 * This format has been mandated by the IEEE specifications,
	 * so this line may not be changed to use the __clear_bit() format.
	 */
	gDeviceInfo->APInfo->tim[aid / 8] &= ~(1 << (aid % 8));
}

#if 0
static unsigned long ieee80211_tids_for_ac(int ac)
{
	/* If we ever support TIDs > 7, this obviously needs to be adjusted */
	switch (ac) {
	case IEEE80211_AC_VO:
		return BIT(6) | BIT(7);
	case IEEE80211_AC_VI:
		return BIT(4) | BIT(5);
	case IEEE80211_AC_BE:
		return BIT(0) | BIT(3);
	case IEEE80211_AC_BK:
		return BIT(1) | BIT(2);
	default:
		//WARN_ON(1);
		return 0;
	}
}
#endif

//extern u32 ap_tx_desp_queue_len(const struct list_q *list_);




extern void GenBeacon();
void sta_info_recalc_tim(APStaInfo_st *sta)
{
	bool indicate_tim = false;

	//normal case. If there is at least one non-delivery AC, we just need to check those.

	u8 ignore_for_tim = sta->uapsd_queues;
	int ac;

	/*
	 * WMM spec 3.6.1.4
	 * If all ACs are delivery-enabled then we should build
	 * the TIM bit for all ACs anyway; if only some are then
	 * we ignore those and build the TIM bit using only the
	 * non-enabled ones.
	 */
	if (ignore_for_tim == BIT(IEEE80211_NUM_ACS) - 1)
		ignore_for_tim = 0;



	for (ac = 0; ac < IEEE80211_NUM_ACS; ac++) {
//		unsigned long tids;

		if (ignore_for_tim & BIT(ac))
			continue;

	    indicate_tim |= (list_q_len_safe(&sta->ps_tx_buf[ac], &sta->ps_tx_buf_mutex)!=0);

		if(indicate_tim)
			break;

		//feature for driver could buffer frame
		//tids = ieee80211_tids_for_ac(ac);
		//
		//indicate_tim |=
		//	sta->driver_buffered_tids & tids;
	}


	//Reduce to set beacon
	if(indicate_tim ==__bss_tim_get(sta->aid))
		return;


	if (indicate_tim)
		__bss_tim_set(sta->aid);
	else
		__bss_tim_clear(sta->aid);



	//GenBeacon();

}










bool sta_info_buffer_expired(APStaInfo_st *sta, struct ap_tx_desp *tx_desp)
{
	int timeout;

	//Null data. no data expired
	if (!tx_desp)
		return false;

	/* Timeout: (2 * listen_interval * beacon_int * 1024 / 1000000) sec */
	timeout = (sta->listen_interval *
		   AP_DEFAULT_BEACON_INT *
		   32 / 15625) * HZ;
	if (timeout < AP_STA_TX_BUFFER_EXPIRE)
		timeout = AP_STA_TX_BUFFER_EXPIRE;
	return time_after((unsigned long)os_sys_jiffies(), (unsigned long)tx_desp->jiffies + (unsigned long)timeout);
}


static bool sta_info_cleanup_expire_buffered_ac(APStaInfo_st *sta, int ac)
{
	struct ap_tx_desp *tx_desp;

	/*
	 * Now also check the normal PS-buffered queue, this will
	 * only find something if the filtered queue was emptied
	 * since the filtered frames are all before the normal PS
	 * buffered frames.
	 */
	for (;;) {
        OS_MutexLock(sta->ps_tx_buf_mutex);
		tx_desp = ap_tx_desp_peek(&sta->ps_tx_buf[ac]);
		if (sta_info_buffer_expired(sta, tx_desp))
			tx_desp =(struct ap_tx_desp *) list_q_deq(&sta->ps_tx_buf[ac]);
		else
			tx_desp = NULL;
        OS_MutexUnLock(sta->ps_tx_buf_mutex);

		/*
		 * frames are queued in order, so if this one
		 * hasn't expired yet (or we reached the end of
		 * the queue) we can stop testing
		 */
		if (!tx_desp)
			break;
        OS_MutexLock(gDeviceInfo->APInfo->ap_info_ps_mutex);
		gDeviceInfo->APInfo->total_ps_buffered--;
        OS_MutexUnLock(gDeviceInfo->APInfo->ap_info_ps_mutex);

		ap_release_tx_desp_and_frame(tx_desp);
	}

	/*
	 * Finally, recalculate the TIM bit for this station -- it might
	 * now be clear because the station was too slow to retrieve its
	 * frames.
	 */
	sta_info_recalc_tim(sta);
    //Send to MLME task GenBeacon
#if(AP_MODE_BEACON_VIRT_BMAP_0XFF == 0)
#if(MLME_TASK ==1)
    ap_mlme_handler(NULL,MEVT_BCN_CMD);
#else
    GenBeacon();
#endif
#endif
	/*
	 * Return whether there are any frames still buffered, this is
	 * used to check whether the cleanup timer still needs to run,
	 * if there are no frames we don't need to rearm the timer.
	 */
	return (sta->ps_tx_buf[ac].qlen > 0);
}

static bool sta_info_cleanup_expire_buffered(APStaInfo_st *sta)
{
	bool have_buffered = false;
	int ac;

	for (ac = 0; ac < IEEE80211_NUM_ACS; ac++)
		have_buffered |=
			sta_info_cleanup_expire_buffered_ac(sta, ac);

	return have_buffered;
}





void sta_info_cleanup(void *data1, void *data2)
{
	u32 i;
	//APStaInfo_st *sta;
	bool timer_needed = false;

	for(i=0;i<gMaxAID;i++ )
	{

		APStaInfo_st *sta = &gDeviceInfo->StaConInfo[i];
		if(!test_sta_flag(sta, WLAN_STA_VALID))
			continue;

		if(sta_info_cleanup_expire_buffered(sta))
			timer_needed = true;
	}
    OS_MutexLock(gDeviceInfo->APInfo->ap_info_ps_mutex);
    gDeviceInfo->APInfo->sta_pkt_cln_timer = timer_needed;
    OS_MutexUnLock(gDeviceInfo->APInfo->ap_info_ps_mutex);
	if (timer_needed)
    {
		os_create_timer(AP_STA_INFO_CLEANUP_INTERVAL, sta_info_cleanup, NULL, NULL, (void*)TIMEOUT_TASK);
    }
}



static inline void APStaInfo_StaReset(APStaInfo_st *sta)
{

	int i;
    OsMutex ps_tx_buf_mutex = sta->ps_tx_buf_mutex;
    OsMutex apsta_mutex = sta->apsta_mutex;
	MEMSET(sta, 0, sizeof(APStaInfo_st));

    //OS_MutexInit(&sta->ps_tx_buf_mutex);
    sta->ps_tx_buf_mutex = ps_tx_buf_mutex;
    sta->apsta_mutex = apsta_mutex;
	for (i = 0; i < IEEE80211_NUM_ACS; i++) {
			//ap_tx_desp_head_init(&sta->ps_tx_buf[i]);
            list_q_init(&sta->ps_tx_buf[i]);
		}


}


void APStaInfo_Init()
{
	s32 size;
	u32 j;

	size = sizeof(APStaInfo_st) * gMaxAID;
	gDeviceInfo->StaConInfo = MALLOC((u32)size);
	ASSERT(gDeviceInfo->StaConInfo != NULL);


	//StaInfo = gStaInfo;

	//----------------------
	//init station ac list
	for (j=0;j<gMaxAID;j++)
    {
		APStaInfo_StaReset(&gDeviceInfo->StaConInfo[j]);
        OS_MutexInit(&gDeviceInfo->StaConInfo[j].ps_tx_buf_mutex);
        OS_MutexInit(&gDeviceInfo->StaConInfo[j].apsta_mutex);
    }

	//---------------------------

}









//**************************************************************************************************************************
//**************************************************************************************************************************
//**************************************************************************************************************************

void ap_sta_set_authorized(ApInfo_st *pApInfo, APStaInfo_st *sta,
	int authorized)
{
//	const u8 *dev_addr = NULL;
	if (!!authorized == !!(sta->_flags & WLAN_STA_AUTHORIZED))
		return;

#ifdef CONFIG_P2P
	dev_addr = p2p_group_get_dev_addr(hapd->p2p_group, sta->addr);
#endif /* CONFIG_P2P */

	if (authorized) {
// 		if (dev_addr)
// 			wpa_msg(hapd->msg_ctx, MSG_INFO, AP_STA_CONNECTED
// 			MACSTR " p2p_dev_addr=" MACSTR,
// 			MAC2STR(sta->addr), MAC2STR(dev_addr));
// 		else
			LOG_INFO(AP_STA_CONNECTED MACSTR "\r\n", MAC2STR(sta->addr));
// 		if (hapd->msg_ctx_parent &&
// 			hapd->msg_ctx_parent != hapd->msg_ctx && dev_addr)
// 			wpa_msg(hapd->msg_ctx_parent, MSG_INFO,
// 			AP_STA_CONNECTED MACSTR " p2p_dev_addr="
// 			MACSTR,
// 			MAC2STR(sta->addr), MAC2STR(dev_addr));
// 		else if (hapd->msg_ctx_parent &&
// 			hapd->msg_ctx_parent != hapd->msg_ctx)
// 			wpa_msg(hapd->msg_ctx_parent, MSG_INFO,
// 			AP_STA_CONNECTED MACSTR, MAC2STR(sta->addr));
		pApInfo->num_sta_authorized++;
        OS_MutexLock(sta->apsta_mutex);
		sta->_flags |= WLAN_STA_AUTHORIZED;
        OS_MutexUnLock(sta->apsta_mutex);
	} else {
// 		if (dev_addr)
// 			wpa_msg(hapd->msg_ctx, MSG_INFO, AP_STA_DISCONNECTED
// 			MACSTR " p2p_dev_addr=" MACSTR,
// 			MAC2STR(sta->addr), MAC2STR(dev_addr));
// 		else
			LOG_INFO( AP_STA_DISCONNECTED MACSTR "\r\n", MAC2STR(sta->addr));
// 		if (hapd->msg_ctx_parent &&
// 			hapd->msg_ctx_parent != hapd->msg_ctx && dev_addr)
// 			wpa_msg(hapd->msg_ctx_parent, MSG_INFO,
// 			AP_STA_DISCONNECTED MACSTR " p2p_dev_addr="
// 			MACSTR, MAC2STR(sta->addr), MAC2STR(dev_addr));
// 		else if (hapd->msg_ctx_parent &&
// 			hapd->msg_ctx_parent != hapd->msg_ctx)
// 			wpa_msg(hapd->msg_ctx_parent, MSG_INFO,
// 			AP_STA_DISCONNECTED MACSTR,
// 			MAC2STR(sta->addr));
		pApInfo->num_sta_authorized--;
        OS_MutexLock(sta->apsta_mutex);
		sta->_flags &= ~WLAN_STA_AUTHORIZED;
        OS_MutexUnLock(sta->apsta_mutex);
	}

// 	if (hapd->sta_authorized_cb)
// 		hapd->sta_authorized_cb(hapd->sta_authorized_cb_ctx,
// 		sta->addr, authorized, dev_addr);
}


/**
 * ap_handle_timer - Per STA timer handler
 * @eloop_ctx: struct hostapd_data *
 * @timeout_ctx: struct sta_info *
 *
 * This function is called to check station activity and to remove inactive
 * stations.
 */
void ap_handle_timer(void *data1, void *data2)
{
	ApInfo_st *pApInfo = (ApInfo_st *)data1;
	APStaInfo_st *sta = (APStaInfo_st *)data2;

	//struct hostapd_data *hapd = eloop_ctx;
	//struct sta_info *sta = timeout_ctx;
	unsigned long next_time = 0;

#ifdef __TEST__
	LOG_TRACE( "ap_handle_timer sta->timeout_next %d\r\n", sta->timeout_next);
#endif

	if (sta->timeout_next == STA_REMOVE) {
		LOG_INFO( "%s,deauthenticated due to local deauth request \r\n",__func__);
		APStaInfo_free(pApInfo, sta);
		return;
	}



//------------------------------------------------------------------------------

	if (test_sta_flag(sta, WLAN_STA_ASSOC) &&
	    (sta->timeout_next == STA_NULLFUNC ||
	     sta->timeout_next == STA_DISASSOC)) {
		int inactive_sec;
		inactive_sec = os_tick2ms(os_sys_jiffies()-sta->last_rx) /* MSEC_PER_SECI*/;

// 		if (inactive_sec == -1) {
// 			wpa_msg(hapd->msg_ctx, MSG_DEBUG,
// 				"Check inactivity: Could not "
// 				"get station info from kernel driver for "
// 				MACSTR, MAC2STR(sta->addr));
// 			/*
// 			 * The driver may not support this functionality.
// 			 * Anyway, try again after the next inactivity timeout,
// 			 * but do not disconnect the station now.
// 			 */
// 			next_time = hapd->conf->ap_max_inactivity;
// 		} else
		if (inactive_sec < AP_MAX_INACTIVITY &&
			   test_sta_flag(sta, WLAN_STA_ASSOC)) {
			/* station activity detected; reset timeout state */
        #ifdef __AP_DEBUG__
			LOG_DEBUG(
				"Station " MACSTR " has been active %i ms ago\r\n",
				MAC2STR(sta->addr), inactive_sec);
        #endif
			sta->timeout_next = STA_NULLFUNC;
			next_time = AP_MAX_INACTIVITY - (unsigned long)inactive_sec;
		} else {
			//LOG_DEBUG(
			//	"Station " MACSTR " has been "
			//	"inactive too long: %d msec, max allowed: %d\r\n",
			//	MAC2STR(sta->addr), inactive_sec,
			//	AP_MAX_INACTIVITY);
		}
 	}
//------------------------------------------------------------------------------

	if ( test_sta_flag(sta, WLAN_STA_ASSOC) &&
	    sta->timeout_next == STA_DISASSOC &&
	    !test_sta_flag(sta, WLAN_STA_PENDING_POLL)) {
		//LOG_DEBUG( "Station " MACSTR
		//	" has ACKed data poll\r\n", MAC2STR(sta->addr));
		/* data nullfunc frame poll did not produce TX errors; assume
		 * station ACKed it */
		sta->timeout_next = STA_NULLFUNC;
		next_time = AP_MAX_INACTIVITY;
	}

	if (next_time) {
        os_create_timer(next_time, ap_handle_timer, pApInfo, sta, (void*)TIMEOUT_TASK);
		return;																					//---------------------------------------->Return
	}

//------------------------------------------------------------------------------
//Send Frame-Poll, De-authentication, Dis-association.

	if (sta->timeout_next == STA_NULLFUNC &&
	     test_sta_flag(sta, WLAN_STA_ASSOC)) {
		//LOG_DEBUG(  "Polling STA,%s\r\n",__func__);
        OS_MutexLock(sta->apsta_mutex);
		set_sta_flag(sta, WLAN_STA_PENDING_POLL);
//Send NULL data to poll
 		//nl80211_poll_client(pApInfo->own_addr, sta->addr,										//----------------------------------->implement
 		//			test_sta_flag(sta, WLAN_STA_WMM));

        //poll station status arp retry count
        if (sta->arp_retry_count < STA_TIMEOUT_RETRY_COUNT )
        {

            sta->arp_retry_count ++;
            OS_MutexUnLock(sta->apsta_mutex);
            //send host event to net_mgr , for arp request.
            exec_host_evt_cb(SOC_EVT_POLL_STATION,sta->addr,ETH_ALEN);
            os_create_timer(STA_TIMEOUT_RETRY_TIMER, ap_handle_timer, pApInfo, sta, (void*)TIMEOUT_TASK);
            return;
        }
		OS_MutexUnLock(sta->apsta_mutex);

	} else if (sta->timeout_next != STA_REMOVE) {
		int deauth = (sta->timeout_next == STA_DEAUTH);

		LOG_DEBUG(  "Sending %s info to STA " MACSTR"\r\n",
			   deauth ? "deauthentication" : "disassociation",
			   MAC2STR(sta->addr));

		if (deauth) {
//Send De-authentication frame																									//----------------------------------->implement
			i802_sta_deauth(pApInfo->own_addr, sta->addr, WLAN_REASON_PREV_AUTH_NOT_VALID);
		} else {
//Send Dis-association frame
			i802_sta_disassoc(pApInfo->own_addr, sta->addr,	WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY);						//----------------------------------->implement
		}
	}


//------------------------------------------------------------------------------



	switch (sta->timeout_next)
	{
		case STA_NULLFUNC:
			sta->timeout_next = STA_DISASSOC;
            os_create_timer(AP_DISASSOC_DELAY, ap_handle_timer,
    					   pApInfo, sta, (void*)TIMEOUT_TASK);

			break;
		case STA_DISASSOC:
//Todo:security


			ap_sta_set_authorized(pApInfo, sta, 0);
            OS_MutexLock(sta->apsta_mutex);
			clear_sta_flag(sta, WLAN_STA_ASSOC);
            OS_MutexUnLock(sta->apsta_mutex);


// 			ieee802_1x_notify_port_enabled(sta->eapol_sm, 0);
// 			if (!sta->acct_terminate_cause)
// 				sta->acct_terminate_cause =
// 				RADIUS_ACCT_TERMINATE_CAUSE_IDLE_TIMEOUT;
// 			accounting_sta_stop(hapd, sta);//RADIUS
// 			ieee802_1x_free_station(sta);
			LOG_INFO("STA " MACSTR " disassociated due to inactivity \r\n", MAC2STR(sta->addr));
			sta->timeout_next = STA_DEAUTH;
            os_create_timer(AP_DEAUTH_DELAY, ap_handle_timer, pApInfo, sta, (void*)TIMEOUT_TASK);


//			mlme_disassociate_indication(
//				hapd, sta, WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY);
			break;
		case STA_DEAUTH:
		case STA_REMOVE:
			LOG_INFO("STA " MACSTR " deauthenticated due to inactivity \r\n", MAC2STR(sta->addr));
// 			if (!sta->acct_terminate_cause)
// 				sta->acct_terminate_cause =
// 					RADIUS_ACCT_TERMINATE_CAUSE_IDLE_TIMEOUT;
// 			mlme_deauthenticate_indication(
// 				hapd, sta,
// 				WLAN_REASON_PREV_AUTH_NOT_VALID);

 			APStaInfo_free(pApInfo, sta);
			break;

		default:
			break;
 	}
}












APStaInfo_st *APStaInfo_FindStaByAddr( ETHER_ADDR *mac )
{

    u32 idx;

    for(idx=0; idx<gMaxAID; idx++)
    {

		 APStaInfo_st *StaInfo= &gDeviceInfo->StaConInfo[idx];
        if (!test_sta_flag(StaInfo, WLAN_STA_VALID))
            continue;

        if (IS_EQUAL_MACADDR((char*)mac, (char*)StaInfo->addr))
            return StaInfo;
    }
    return NULL;
}





bool ap_sta_info_capability(ETHER_ADDR *mac , bool *ht, bool *qos, bool *wds)
{

	APStaInfo_st *sta = APStaInfo_FindStaByAddr(mac);

	//mc/bc frame use normal frame case
	if(is_multicast_ether_addr((const u8*)mac))
	{
		*ht  = *qos = *wds =0;
		return TRUE;
	}


	if(!sta)
	{
		LOG_TRACE( "Could not found STA \r\n"MACSTR, MAC2STR((u8*)mac));
		return FALSE;
	}



	*ht = test_sta_flag(sta, WLAN_STA_HT);
	*qos = test_sta_flag(sta, WLAN_STA_WMM);
	*wds = test_sta_flag(sta, WLAN_STA_WDS);



	return TRUE;
}











APStaInfo_st *APStaInfo_GetNewSta(void)
{

    u32 idx;

    for(idx=0; idx<gMaxAID; idx++)
    {

		 APStaInfo_st *StaInfo= &gDeviceInfo->StaConInfo[idx];
        if (!test_sta_flag(StaInfo, WLAN_STA_VALID))
        {
            OS_MutexLock(StaInfo->apsta_mutex);
			set_sta_flag(StaInfo, WLAN_STA_VALID);
            OS_MutexUnLock(StaInfo->apsta_mutex);
			return StaInfo;
		}
    }
    return NULL;
}


void APStaInfo_DrvRemove(ApInfo_st *pApInfo, APStaInfo_st *sta)
{

    OS_MutexLock(sta->apsta_mutex);

	if ( test_sta_flag(sta, WLAN_STA_PS_STA) )
	{
        clear_sta_flag(sta, WLAN_STA_PS_STA);
        OS_MutexUnLock(sta->apsta_mutex);
        OS_MutexLock(pApInfo->ap_info_ps_mutex);
		pApInfo->num_sta_ps--;
		OS_MutexUnLock(pApInfo->ap_info_ps_mutex);
		purge_sta_ps_buffers(sta);
		sta_info_recalc_tim(sta);

        //Send to MLME task GenBeacon
#if(AP_MODE_BEACON_VIRT_BMAP_0XFF == 0)
#if(MLME_TASK ==1)
        ap_mlme_handler(NULL,MEVT_BCN_CMD);
#else
        GenBeacon();
#endif
#endif
	}
    else
    {
        OS_MutexUnLock(sta->apsta_mutex);
    }

	ap_soc_cmd_sta_oper(CFG_STA_DEL, (struct ETHER_ADDR_st *)sta->addr, CFG_HT_NONE, test_sta_flag(sta, WLAN_STA_WMM));


}


void APStaInfo_free(ApInfo_st *pApInfo, APStaInfo_st *sta)
{
	int set_beacon = 0;


	if(!test_sta_flag(sta, WLAN_STA_VALID))
		return;



	LOG_TRACE("%s\r\n",__func__);


	//Todo: SEC
	//accounting_sta_stop(hapd, sta);

	/* just in case */
	ap_sta_set_authorized(pApInfo, sta, 0);

	//if (sta->flags & WLAN_STA_WDS)
	//	hostapd_set_wds_sta(hapd, sta->addr, sta->aid, 0);

	//if (!(sta->flags & WLAN_STA_PREAUTH))
	//	hostapd_drv_sta_remove(hapd, sta->addr);


 	APStaInfo_DrvRemove(pApInfo, sta);



	//Remove AID
	if (sta->aid > 0)
		pApInfo->sta_aid[(sta->aid - 1) / 32] &=
			~ BIT((sta->aid - 1) % 32);

	pApInfo->num_sta--;

	//all station are remove.....no need to clean buffer frames
	if(!pApInfo->num_sta)
	{
		os_cancel_timer(sta_info_cleanup, 0, 0);
        OS_MutexLock(gDeviceInfo->APInfo->ap_info_ps_mutex);
        gDeviceInfo->APInfo->sta_pkt_cln_timer = FALSE;
        OS_MutexUnLock(gDeviceInfo->APInfo->ap_info_ps_mutex);
		//purge_all_bc_buffers();
	}

	if (sta->nonerp_set) {
		sta->nonerp_set = 0;
		pApInfo->num_sta_non_erp--;
		if (pApInfo->num_sta_non_erp == 0)
			set_beacon++;
	}

	if (sta->no_short_slot_time_set) {
		sta->no_short_slot_time_set = 0;
		pApInfo->num_sta_no_short_slot_time--;
		if (pApInfo->eCurrentApMode == AP_MODE_IEEE80211G
		    && pApInfo->num_sta_no_short_slot_time == 0)
			set_beacon++;
	}

	if (sta->no_short_preamble_set) {
		sta->no_short_preamble_set = 0;
		pApInfo->num_sta_no_short_preamble--;
		if (pApInfo->eCurrentApMode == AP_MODE_IEEE80211G
		    && pApInfo->num_sta_no_short_preamble == 0)
			set_beacon++;
	}

	if (sta->no_ht_gf_set) {
		sta->no_ht_gf_set = 0;
		pApInfo->num_sta_ht_no_gf--;
	}

	if (sta->no_ht_set) {
		sta->no_ht_set = 0;
		pApInfo->num_sta_no_ht--;
	}

	if (sta->ht_20mhz_set) {
		sta->ht_20mhz_set = 0;
		pApInfo->num_sta_ht_20mhz--;
	}

//#ifdef CONFIG_P2P
//	if (sta->no_p2p_set) {
//		sta->no_p2p_set = 0;
//		hapd->num_sta_no_p2p--;
//		if (hapd->num_sta_no_p2p == 0)
//			hostapd_p2p_non_p2p_sta_disconnected(hapd);
//	}
//#endif /* CONFIG_P2P */

//Todo: HT
#if defined(NEED_AP_MLME) && defined(CONFIG_IEEE80211N)
	if (hostapd_ht_operation_update(hapd->iface) > 0)
		set_beacon++;
#endif /* NEED_AP_MLME && CONFIG_IEEE80211N */

	if (set_beacon)
		ieee802_11_set_beacon(pApInfo,TRUE);

	os_cancel_timer(ap_handle_timer,  (u32)pApInfo, (u32)sta);












//Todo:
//	os_cancel_timer(ap_handle_session_timer);
//	os_cancel_timer(ap_sta_deauth_cb_timeout);
//	os_cancel_timer(ap_sta_disassoc_cb_timeout);

//Todo: SEC
	//ieee802_1x_free_station(sta);
	//wpa_auth_sta_deinit(sta->wpa_sm);
	//rsn_preauth_free_station(hapd, sta);

//#ifndef CONFIG_NO_RADIUS
//	radius_client_flush_auth(hapd->radius, sta->addr);
//#endif /* CONFIG_NO_RADIUS */

//Todo: SEC
//	os_free(sta->last_assoc_req);
//	os_free(sta->challenge);

//#ifdef CONFIG_IEEE80211W
//	os_free(sta->sa_query_trans_id);
//	eloop_cancel_timeout(ap_sa_query_timer, hapd, sta);
//#endif /* CONFIG_IEEE80211W */

//#ifdef CONFIG_P2P
//	p2p_group_notif_disassoc(hapd->p2p_group, sta->addr);
//#endif /* CONFIG_P2P */


//Todo: SEC
	//wpabuf_free(sta->wps_ie);
	//wpabuf_free(sta->p2p_ie);

//Todo: HT
	//os_free(sta->ht_capabilities);

	OS_MutexLock(sta->ps_tx_buf_mutex);
	APStaInfo_StaReset(sta);
	OS_MutexUnLock(sta->ps_tx_buf_mutex);

#ifdef __AP_DEBUG__
	APStaInfo_PrintStaInfo();
#endif//__AP_DEBUG__

}





APStaInfo_st *APStaInfo_add(ApInfo_st *pApInfo, const u8 *addr)
{
	APStaInfo_st *sta = NULL;

	sta = APStaInfo_FindStaByAddr((ETHER_ADDR*)addr);
	if (sta)
		return sta;

	LOG_TRACE( "%s\r\n",__func__);
	if (pApInfo->num_sta >= gMaxAID) {
		/* FIX: might try to remove some old STAs first? */
		LOG_DEBUG( "no more room for new STAs (%d/%d)\r\n",
			   pApInfo->num_sta, gMaxAID);
		return NULL;
	}

	sta = APStaInfo_GetNewSta();
	if (sta == NULL) {
		LOG_ERROR( "%s can't get new sta\r\n",__func__);
		return NULL;
	}

	//sta->acct_interim_interval = hapd->conf->acct_interim_interval;

	/* initialize STA info data */
	os_create_timer(AP_MAX_INACTIVITY, ap_handle_timer, pApInfo, sta, (void*)TIMEOUT_TASK);


	MEMCPY(sta->addr, addr, ETH_ALEN);

	gDeviceInfo->APInfo->num_sta++;


	//Add new sta in soc after finishing association.


	return sta;
}



















//****************************************************************************************************************************************
//PS-poll, USPAD
//****************************************************************************************************************************************

void ap_send_buffered_frame(struct ap_tx_desp *tx_desp)
{
	tx_desp->host_txreq0->RSVD0 |= AP_PS_FRAME;
        if ( (TxHdl_FrameProc(tx_desp->frame, false, 0, 0)) == FALSE)
            os_frame_free(tx_desp->frame);

	ap_release_tx_desp(tx_desp);
}



//**************************************************************
//							RX
//**************************************************************

//Should send through normal path->host apis tx->hcmd queue->hcmd engine->drv.
static void ieee80211_send_null_response(APStaInfo_st *sta, int tid,
					 enum frame_release_type reason)
{

	struct ieee80211_qos_hdr *nullfunc;

	int size = sizeof(*nullfunc);
	__le16 fc;
	bool qos = test_sta_flag(sta, WLAN_STA_WMM);


	if (qos) {
		fc = cpu_to_le16(IEEE80211_FTYPE_DATA |
				 IEEE80211_STYPE_QOS_NULLFUNC |
				 IEEE80211_FCTL_FROMDS);
	} else {
		size -= 2;
		fc = cpu_to_le16(IEEE80211_FTYPE_DATA |
				 IEEE80211_STYPE_NULLFUNC |
				 IEEE80211_FCTL_FROMDS);
	}

	nullfunc = (struct ieee80211_qos_hdr *)gDeviceInfo->APInfo->pMgmtPkt;


	nullfunc->frame_control = fc;
	nullfunc->duration_id = 0;
	MEMCPY((void*)nullfunc->addr1, (void*)sta->addr, ETH_ALEN);
	MEMCPY((void*)nullfunc->addr2, (void*)gDeviceInfo->APInfo->own_addr, ETH_ALEN);
	MEMCPY((void*)nullfunc->addr3, (void*)gDeviceInfo->APInfo->own_addr, ETH_ALEN);


	if (qos) {
		nullfunc->qos_ctrl = cpu_to_le16(tid);

		if (reason == FRAME_RELEASE_UAPSD)
			nullfunc->qos_ctrl |=
			cpu_to_le16(IEEE80211_QOS_CTL_EOSP);
	}


	/*
	 * Tell TX path to send this frame even though the
	 * STA may still remain is PS mode after this frame
	 * exchange. Also set EOSP to indicate this packet
	 * ends the poll/service period.
	 */

// 	info->flags |= IEEE80211_TX_CTL_POLL_RESPONSE |
// 		       IEEE80211_TX_STATUS_EOSP |
// 		       IEEE80211_TX_CTL_REQ_TX_STATUS;





	ap_soc_data_send(gDeviceInfo->APInfo->pOSMgmtframe, size, TRUE, AP_TX_POLL_RESPONSE);


}




static void
ieee80211_sta_ps_deliver_response(APStaInfo_st *sta,			//PS poll UAPSD
				  int n_frames, u8 ignored_acs,
				  enum frame_release_type reason)
{
	bool found = false;
	bool more_data = false;
	int ac;
	unsigned long driver_release_tids = 0;
	struct list_q frames;

	list_q_init(&frames);
	do{


		/* Service or PS-Poll period starts */
		if(FRAME_RELEASE_UAPSD == reason)
        {
            OS_MutexLock(sta->apsta_mutex);
			set_sta_flag(sta, WLAN_STA_SP);		//clear bit in ieee80211_tx_status
			OS_MutexUnLock(sta->apsta_mutex);
        }
		//ap_tx_desp_head_init(&frames);
        list_q_init(&frames);

		/*
		 * Get response frame(s) and more data bit for it.
		 */
		for (ac = 0; ac < IEEE80211_NUM_ACS; ac++)
		{


			if (ignored_acs & BIT(ac))
				continue;

	//		tids = ieee80211_tids_for_ac(ac);
	//
	//		if (!found) {
	//			driver_release_tids = sta->driver_buffered_tids & tids;
	//			if (driver_release_tids) {
	//				found = true;
	//			}
	//			else
				{
					//struct sk_buff *skb;
					struct ap_tx_desp *tx_desp;

					while (n_frames > 0) {										//------------------------------------->
						//skb = skb_dequeue(&sta->tx_filtered[ac]);
						//if (!skb)
						{
							//tx_desp = ap_tx_desp_dequeue(
							//	&sta->ps_tx_buf[ac]);
                            tx_desp = (struct ap_tx_desp *)list_q_deq_safe(&sta->ps_tx_buf[ac],&sta->ps_tx_buf_mutex);


							if (tx_desp)
                            {
                                OS_MutexLock(gDeviceInfo->APInfo->ap_info_ps_mutex);
								gDeviceInfo->APInfo->total_ps_buffered--;
                                OS_MutexUnLock(gDeviceInfo->APInfo->ap_info_ps_mutex);
                            }
						}
						if (!tx_desp)
							break;
						n_frames--;
						found = true;
                        list_q_qtail_safe(&frames, (struct list_q *)tx_desp, &sta->ps_tx_buf_mutex);
					}
				}
	//
	// 			/*
	// 			 * If the driver has data on more than one TID then
	// 			 * certainly there's more data if we release just a
	// 			 * single frame now (from a single TID).
	// 			 */
	// 			if (reason == FRAME_RELEASE_PSPOLL &&
	// 			    hweight16(driver_release_tids) > 1) {
	// 				more_data = true;
	// 				driver_release_tids =
	// 					BIT(ffs(driver_release_tids) - 1);
	// 				break;
	// 			}
	// 		}
	//
			if (!ap_tx_desp_queue_empty(&sta->ps_tx_buf[ac],&sta->ps_tx_buf_mutex)) {
				more_data = true;
				break;
			}
	 	}

		if (!found) {
			int tid;

			/*
			 * For PS-Poll, this can only happen due to a race condition
			 * when we set the TIM bit and the station notices it, but
			 * before it can poll for the frame we expire it.
			 *
			 * For uAPSD, this is said in the standard (11.2.1.5 h):
			 *	At each unscheduled SP for a non-AP STA, the AP shall
			 *	attempt to transmit at least one MSDU or MMPDU, but no
			 *	more than the value specified in the Max SP Length field
			 *	in the QoS Capability element from delivery-enabled ACs,
			 *	that are destined for the non-AP STA.
			 *
			 * Since we have no other MSDU/MMPDU, transmit a QoS null frame.
			 */

			/* This will evaluate to 1, 3, 5 or 7. */
			tid = 7 - ((ffs(~ignored_acs) - 1) << 1);

			ieee80211_send_null_response(sta, tid, reason);

			break;
		}

		if (!driver_release_tids) {

			struct ap_tx_desp *tx_desp;


			while ((tx_desp = (struct ap_tx_desp *)list_q_deq_safe(&frames,&sta->ps_tx_buf_mutex))) {


				u8 *qoshdr = NULL;


				/*
				 * Tell TX path to send this frame even though the
				 * STA may still remain is PS mode after this frame
				 * exchange.
				 */

				/*
				 * Use MoreData flag to indicate whether there are
				 * more buffered frames for this STA
				 */

				if (more_data || !ap_tx_desp_queue_empty(&frames,&sta->ps_tx_buf_mutex))
					tx_desp->host_txreq0->more_data = 1;
				else
					tx_desp->host_txreq0->more_data = 0;


				//it's 80211 packet, need to add bit by hand
				if(tx_desp->host_txreq0->f80211)
				{
					struct ieee80211_hdr *hdr = (void*) tx_desp->data;

					if (tx_desp->host_txreq0->more_data)
						hdr->frame_control |=
							cpu_to_le16(IEEE80211_FCTL_MOREDATA);
					else
						hdr->frame_control &=
							cpu_to_le16(~IEEE80211_FCTL_MOREDATA);
				}



				//Set QoS Info only in data
				/* set EOSP for the frame */
				if (reason == FRAME_RELEASE_UAPSD &&
				     ap_tx_desp_queue_empty(&frames,&sta->ps_tx_buf_mutex))//Only set in Data frame ??? non data frame is last in queue
				{

					if(1 == tx_desp->host_txreq0->qos){

						//802.11 frame
						if(1 == tx_desp->host_txreq0->f80211)
							qoshdr = ieee80211_get_qos_ctl((struct ieee80211_hdr_4addr *)tx_desp->data);
						else//802.3 frame
							qoshdr = ssv6xxx_host_tx_req_get_qos_ptr(tx_desp->host_txreq0);

						*qoshdr |= IEEE80211_QOS_CTL_EOSP;

					}
					else{

						LOG_WARN("buffered fame is not qos frame sub_type:%d \r\n", tx_desp->host_txreq0->sub_type);
					}

				}





#if 1
				ap_send_buffered_frame(tx_desp);
#else
				//should not call host_apis interface.
				ssv6xxx_drv_send(OS_FRAME_GET_DATA(tx_desp->frame), OS_FRAME_GET_DATA_LEN(tx_desp->frame));
				//Tricky and waste
		    	ap_release_tx_desp_and_frame(tx_desp);
#endif



			}




			sta_info_recalc_tim(sta);
            //Send to MLME task GenBeacon
#if(AP_MODE_BEACON_VIRT_BMAP_0XFF == 0)
#if(MLME_TASK ==1)
            ap_mlme_handler(NULL,MEVT_BCN_CMD);
#else
            GenBeacon();
#endif
#endif
		}









	//	else {
	// 		/*
	// 		 * We need to release a frame that is buffered somewhere in the
	// 		 * driver ... it'll have to handle that.
	// 		 * Note that, as per the comment above, it'll also have to see
	// 		 * if there is more than just one frame on the specific TID that
	// 		 * we're releasing from, and it needs to set the more-data bit
	// 		 * accordingly if we tell it that there's no more data. If we do
	// 		 * tell it there's more data, then of course the more-data bit
	// 		 * needs to be set anyway.
	// 		 */
	// 		drv_release_buffered_frames(local, sta, driver_release_tids,
	// 					    n_frames, reason, more_data);
	//
	// 		/*
	// 		 * Note that we don't recalculate the TIM bit here as it would
	// 		 * most likely have no effect at all unless the driver told us
	// 		 * that the TID became empty before returning here from the
	// 		 * release function.
	// 		 * Either way, however, when the driver tells us that the TID
	// 		 * became empty we'll do the TIM recalculation.
	// 		 */
	// 	}
	}while(0);



	//Actually it should be clear when EOSP frame has already sent.
	OS_MutexLock(sta->apsta_mutex);
	clear_sta_flag(sta, WLAN_STA_SP);
    OS_MutexUnLock(sta->apsta_mutex);



}

void sta_ps_deliver_poll_response(APStaInfo_st *sta)
{
	u8 ignore_for_response = sta->uapsd_queues;

	/*
	 * If all ACs are delivery-enabled then we should reply
	 * from any of them, if only some are enabled we reply
	 * only from the non-enabled ones.
	 */
	if (ignore_for_response == BIT(IEEE80211_NUM_ACS) - 1)
		ignore_for_response = 0;

	ieee80211_sta_ps_deliver_response(sta, 1, ignore_for_response,
					  FRAME_RELEASE_PSPOLL);
}

void sta_ps_deliver_uapsd(APStaInfo_st *sta)
{
	int n_frames = sta->max_sp;
	u8 delivery_enabled = sta->uapsd_queues;

	/*
	 * If we ever grow support for TSPEC this might happen if
	 * the TSPEC update from hostapd comes in between a trigger
	 * frame setting WLAN_STA_UAPSD in the RX path and this
	 * actually getting called.
	 */
	if (!delivery_enabled)
		return;

	switch (sta->max_sp) {
	case 1:
		n_frames = 2;
		break;
	case 2:
		n_frames = 4;
		break;
	case 3:
		n_frames = 6;
		break;
	case 0:
		/* XXX: what is a good value? */
		n_frames = 8;
		break;
	}

	ieee80211_sta_ps_deliver_response(sta, n_frames, ~delivery_enabled,
					  FRAME_RELEASE_UAPSD);
}





void sta_ps_deliver_wakeup(APStaInfo_st *sta)//Station wake up, so deliver so buffer and recal TIM
{
	//struct ieee80211_sub_if_data *sdata = sta->sdata;
	//struct ieee80211_local *local = sdata->local;
	//struct sk_buff_head pending;
	int buffered = 0, ac;
	struct ap_tx_desp *tx_desp;

    OS_MutexLock(sta->apsta_mutex);
	clear_sta_flag(sta, WLAN_STA_SP);
	clear_sta_flag(sta, WLAN_STA_PS_STA);
    OS_MutexUnLock(sta->apsta_mutex);

	//BUILD_BUG_ON(BITS_TO_LONGS(STA_TID_NUM) > 1);
	//sta->driver_buffered_tids = 0;

//	if (!(local->hw.flags & IEEE80211_HW_AP_LINK_PS))
//		drv_sta_notify(local, sdata, STA_NOTIFY_AWAKE, &sta->sta);
//	skb_queue_head_init(&pending);

	/* Send all buffered frames to the station */
	for (ac = 0; ac < IEEE80211_NUM_ACS; ac++) {
		//int count = skb_queue_len(&pending), tmp;
			//while (tx_desp = ap_tx_desp_dequeue(&sta->ps_tx_buf[ac])) {
            while (tx_desp = (struct ap_tx_desp *)list_q_deq_safe(&sta->ps_tx_buf[ac],&sta->ps_tx_buf_mutex)) {

#if 1
				ap_send_buffered_frame(tx_desp);
#else
				ssv6xxx_drv_send(OS_FRAME_GET_DATA(tx_desp->frame), OS_FRAME_GET_DATA_LEN(tx_desp->frame));
		    	ap_release_tx_desp(tx_desp);
#endif

				buffered++;
			}
//		skb_queue_splice_tail_init(&sta->ps_tx_buf[ac], &pending);
//		tmp = skb_queue_len(&pending);
//		buffered += tmp - count;
	}


	OS_MutexLock(gDeviceInfo->APInfo->ap_info_ps_mutex);
	gDeviceInfo->APInfo->total_ps_buffered -= buffered;
    OS_MutexUnLock(gDeviceInfo->APInfo->ap_info_ps_mutex);

	sta_info_recalc_tim(sta);
    //Send to MLME task GenBeacon
#if(AP_MODE_BEACON_VIRT_BMAP_0XFF == 0)
#if(MLME_TASK ==1)
    ap_mlme_handler(NULL,MEVT_BCN_CMD);
#else
    GenBeacon();
#endif
#endif

//#ifdef CONFIG_MAC80211_VERBOSE_PS_DEBUG
//	printk(KERN_DEBUG "%s: STA %pM aid %d sending %d filtered/%d PS frames "
//	       "since STA not sleeping anymore\n", sdata->name,
//	       sta->sta.addr, sta->sta.aid, filtered, buffered);
//#endif /* CONFIG_MAC80211_VERBOSE_PS_DEBUG */
}






void APStaInfo_Release(void)
{
	u32 idx=0;

	do
	{
		APStaInfo_free(gDeviceInfo->APInfo, &gDeviceInfo->StaConInfo[idx]);
		idx++;
	}while(idx<gMaxAID);

}

struct ap_sta_info_tbl
{
	const char *string;
};

static const struct ap_sta_info_tbl ap_station_state[32]=
{
	{"VALID"},
	{"AUTH"},
	{"ASSOC"},
	{""},
	{""},					//.4

	{""},
	{"AUTHORIZED"},
	{"PENDING_POLL"},
	{"SHORT_PREAMBLE"},
	{""},					//9

	{"WMM"},
	{""},
	{"HT"},
	{""},
	{""},					//14

	{"WDS"},
	{"NONERP"},
	{"PS_STA"},
	{""},
	{""},					//19

	{""},
	{""},
	{"PSPOLL"},
	{""},
	{"SP"},					//24

	{"ASSOC_REQ_OK"},
	{""},
	{""},
	{""},
	{""},					//29


	{""},
	{""},					//31

};



extern void ap_print_queue_info(struct list_q *head);





void APStaInfo_PrintStaInfo(void)
{
	u32 idx=0;
	int i;


	LOG_TRACE("===================%s==================\r\n",__func__);
	//LOG_TRACE("PS STA:%d BUF BC:%d Jiffies:%d\r\n", gDeviceInfo->APInfo->num_sta_ps, list_q_len_safe(&(gDeviceInfo->APInfo->ps_bc_buf), &gDeviceInfo->APInfo->ps_bc_buf_mutex), os_sys_jiffies());

	for(idx=0; idx<gMaxAID; idx++)
    {
		int timeout;

		APStaInfo_st *StaInfo= &gDeviceInfo->StaConInfo[idx];

		LOG_TRACE("STA:%d ",idx);

		if (!test_sta_flag(StaInfo, WLAN_STA_VALID))
        {
        	LOG_PRINTF("=>Invalid\r\n");
        	continue;
		}
		LOG_PRINTF("=>addr:"MACSTR" aid:%d _flag:%08x", MAC2STR(StaInfo->addr), StaInfo->aid, StaInfo->_flags);

		for(i=0;i<32;i++)
		{
			if(!!( BIT(i)& StaInfo->_flags ) == TRUE)
				LOG_PRINTF(" %s", ap_station_state[i]);
		}
		LOG_PRINTF("\r\n");

		timeout = (StaInfo->listen_interval *
		   AP_DEFAULT_BEACON_INT *
		   32 / 15625) * HZ;

		LOG_TRACE("idle:%d buf timeout:%d\r\n", os_tick2ms(os_sys_jiffies()-StaInfo->last_rx), timeout);

		for(i=0;i<IEEE80211_NUM_ACS;i++)
		{

			LOG_TRACE("BUF AC[%d]:%d \r\n", i, list_q_len_safe(&StaInfo->ps_tx_buf[i], &StaInfo->ps_tx_buf_mutex));
			ap_print_queue_info(&StaInfo->ps_tx_buf[i]);
		}



		LOG_PRINTF("\r\n");



	}





	LOG_TRACE("========================================\r\n",__LINE__);

}



