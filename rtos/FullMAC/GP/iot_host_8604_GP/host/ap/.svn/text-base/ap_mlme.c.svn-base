#define __SFILE__ "ap.mlme.c"

#include <config.h>

#include <pbuf.h>
#include <common.h>

#include <os_wrapper.h>

#include "common/ieee802_11_defs.h"
#include "common/ieee80211.h"
#include "common/ieee802_11_common.h"
#include "ieee802_11_mgmt.h"
#include "ap_mlme.h"
#include "ap_config.h"
#include "ap_sta_info.h"
#include "ap_info.h"
#include "beacon.h"
#include "ap_def.h"
#include "wmm.h"
#include "ap_rx.h"
#include "ap_drv_cmd.h"
#include "ssv_timer.h"
#include "dev.h"

#include <host_apis.h>

#ifdef __AP_DEBUG__
	extern void APStaInfo_PrintStaInfo(void);
#endif//__AP_DEBUG__

extern void APStaInfo_free(ApInfo_st *pApInfo, APStaInfo_st *sta);

#undef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"

/*
* Compact form for string representation of MAC address
* To be used, e.g., for constructing dbus paths for P2P Devices
*/
//#define COMPACT_MACSTR "%02x%02x%02x%02x%02x%02x"








extern void _HCmdEng_TxHdlData(void *frame ,bool bAPFrame, u32 TxFlags);













#define S_SWAP(a,b) do { u8 t = S[a]; S[a] = S[b]; S[b] = t; } while(0)

int rc4_skip(const u8 *key, size_t keylen, size_t skip,
	     u8 *data, size_t data_len)
{
	u32 i, j, k;
	u8 S[256], *pos;
	size_t kpos;

	/* Setup RC4 state */
	for (i = 0; i < 256; i++)
		S[i] = i;
	j = 0;
	kpos = 0;
	for (i = 0; i < 256; i++) {
		j = (j + S[i] + key[kpos]) & 0xff;
		kpos++;
		if (kpos >= keylen)
			kpos = 0;
		S_SWAP(i, j);
	}

	/* Skip the start of the stream */
	i = j = 0;
	for (k = 0; k < skip; k++) {
		i = (i + 1) & 0xff;
		j = (j + S[i]) & 0xff;
		S_SWAP(i, j);
	}

	/* Apply RC4 to data */
	pos = data;
	for (k = 0; k < data_len; k++) {
		i = (i + 1) & 0xff;
		j = (j + S[i]) & 0xff;
		S_SWAP(i, j);
		*pos++ ^= S[(S[i] + S[j]) & 0xff];
	}

	return 0;
}


//------------------

static u16 auth_shared_key(struct ApInfo *pApInfo, APStaInfo_st *sta,
			   u16 auth_transaction, const u8 *challenge,
			   int iswep)
{
#ifdef __AP_DEBUG__
	LOG_TRACE("station authentication (shared key, transaction %d)\n", auth_transaction);
#endif

	if (auth_transaction == 1) {
		if (!sta->challenge) {
			/* Generate a pseudo-random challenge */
			u8 key[8];
			
			int r, tick;
			sta->challenge = MALLOC(WLAN_AUTH_CHALLENGE_LEN);
			if (sta->challenge == NULL)
				return WLAN_STATUS_UNSPECIFIED_FAILURE;
			
			tick = os_sys_jiffies();
			r = OS_Random();
			MEMCPY(key, &tick, 4);
			MEMCPY(key + 4, &r, 4);
			rc4_skip(key, sizeof(key), 0,
				 sta->challenge, WLAN_AUTH_CHALLENGE_LEN);
		}
		return 0;
	}

	if (auth_transaction != 3)
		return WLAN_STATUS_UNSPECIFIED_FAILURE;

	/* Transaction 3 */
	if (!iswep || !sta->challenge || !challenge ||
	    MEMCMP(sta->challenge, challenge, WLAN_AUTH_CHALLENGE_LEN)) {
		LOG_INFO("shared key authentication - invalid challenge-response. \n");
		return WLAN_STATUS_CHALLENGE_FAIL;
	}
#ifdef __AP_DEBUG__
	LOG_DEBUG("authentication OK (shared key). \n");
#endif
#ifdef IEEE80211_REQUIRE_AUTH_ACK
	/* Station will be marked authenticated if it ACKs the
	 * authentication reply. */
#else
	OS_MutexLock(sta->apsta_mutex);
	set_sta_flag(sta, WLAN_STA_AUTH);
    OS_MutexUnLock(sta->apsta_mutex);

	//wpa_auth_sm_event(sta->wpa_sm, WPA_AUTH);
#endif
	FREEIF(sta->challenge);
	sta->challenge = NULL;

	return 0;
}





//-------------------------------------------------------------------------------------------------------------------




static int wpa_driver_nl80211_send_mlme(const u8 *data,
					size_t data_len, bool bMgmtCopy)
{
	struct ieee80211_mgmt *mgmt;
    //lint -e550
	int encrypt = 1;
	u16 fc;

	mgmt = (struct ieee80211_mgmt *) data;
	fc = le_to_host16(mgmt->frame_control);
    
    #ifdef __AP_DEBUG__
	LOG_DEBUG("Send MLME Length %d\r\n", data_len);
    #endif

	if(data_len > AP_MGMT_PKT_LEN)
	{
		LOG_FATAL("Need to resize pack buffer size,%d\r\n",data_len);
	}
	else
	{
	    #ifdef __AP_DEBUG__
		LOG_TRACE("Send MLME Type->%02x SubType->%04x data len %d\r\n", 
			WLAN_FC_GET_TYPE(fc), WLAN_FC_GET_STYPE(fc), data_len);
        #endif
	}



	if (WLAN_FC_GET_TYPE(fc) == WLAN_FC_TYPE_MGMT &&
	    WLAN_FC_GET_STYPE(fc) == WLAN_FC_STYPE_AUTH) {
		/*
		 * Only one of the authentication frame types is encrypted.
		 * In order for static WEP encryption to work properly (i.e.,
		 * to not encrypt the frame), we need to tell mac80211 about
		 * the frames that must not be encrypted.
		 */
		u16 auth_alg = le_to_host16(mgmt->u.auth.auth_alg);
		u16 auth_trans = le_to_host16(mgmt->u.auth.auth_transaction);
		if (auth_alg != WLAN_AUTH_SHARED_KEY || auth_trans != 3)
			encrypt = 0;
	}

	//return wpa_driver_nl80211_send_frame(drv, data, data_len, encrypt);


	


	if(bMgmtCopy)
		MEMCPY(gDeviceInfo->APInfo->pMgmtPkt, data, data_len);

	return ap_soc_data_send(gDeviceInfo->APInfo->pOSMgmtframe, data_len, TRUE, 0);
	
	
}



int i802_sta_deauth(const u8 *own_addr, const u8 *addr,
	int reason)
{
	//struct i802_bss *bss = priv;
	struct ieee80211_mgmt mgmt;

	MEMSET((void*)&mgmt, 0, sizeof(mgmt));
	mgmt.frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT,
		WLAN_FC_STYPE_DEAUTH);
	MEMCPY((void*)mgmt.da, (void*)addr, ETH_ALEN);
	MEMCPY((void*)mgmt.sa, (void*)own_addr, ETH_ALEN);
	MEMCPY((void*)mgmt.bssid, (void*)own_addr, ETH_ALEN);
	mgmt.u.deauth.reason_code = host_to_le16(reason);
	return wpa_driver_nl80211_send_mlme((u8 *) &mgmt,
		IEEE80211_HDRLEN + sizeof(mgmt.u.deauth),
		TRUE);
}


int i802_sta_disassoc(const u8 *own_addr, const u8 *addr,
	int reason)
{
	//struct i802_bss *bss = priv;
	struct ieee80211_mgmt mgmt;

	MEMSET((void*)&mgmt, 0, sizeof(mgmt));
	mgmt.frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT,
		WLAN_FC_STYPE_DISASSOC);
	MEMCPY((void*)mgmt.da, (void*)addr, ETH_ALEN);
	MEMCPY((void*)mgmt.sa, (void*)own_addr, ETH_ALEN);
	MEMCPY((void*)mgmt.bssid, (void*)own_addr, ETH_ALEN);
	mgmt.u.disassoc.reason_code = host_to_le16(reason);
	return wpa_driver_nl80211_send_mlme((u8 *) &mgmt,
		IEEE80211_HDRLEN + sizeof(mgmt.u.disassoc),
		TRUE);
}




int  nl80211_poll_client(const u8 *own_addr, const u8 *addr,
	int qos)
{
	//struct i802_bss *bss = priv;
	struct {
		struct ieee80211_hdr hdr;
		u16 qos_ctl;
	} STRUCT_PACKED nulldata;
	size_t size;

	/* Send data frame to poll STA and check whether this frame is ACKed */

	MEMSET((void*)&nulldata, 0, sizeof(nulldata));

	if (qos) {
		nulldata.hdr.frame_control =
			IEEE80211_FC(WLAN_FC_TYPE_DATA,
			WLAN_FC_STYPE_QOS_NULL);
		size = sizeof(nulldata);
	} else {
		nulldata.hdr.frame_control =
			IEEE80211_FC(WLAN_FC_TYPE_DATA,
			WLAN_FC_STYPE_NULLFUNC);
		size = sizeof(struct ieee80211_hdr);
	}

	nulldata.hdr.frame_control |= host_to_le16(WLAN_FC_FROMDS);
	MEMCPY((void*)nulldata.hdr.IEEE80211_DA_FROMDS, (void*)addr, ETH_ALEN);
	MEMCPY((void*)nulldata.hdr.IEEE80211_BSSID_FROMDS, (void*)own_addr, ETH_ALEN);
	MEMCPY((void*)nulldata.hdr.IEEE80211_SA_FROMDS, (void*)own_addr, ETH_ALEN);

	return wpa_driver_nl80211_send_mlme((u8 *) &nulldata, size, TRUE);
	
}






//=--------------------------------------------------------------------------------

static void send_assoc_resp(struct ApInfo *pApInfo, APStaInfo_st *sta,
			    u16 status_code, int reassoc, const u8 *ies,
			    size_t ies_len)
{
	u32 send_len;
	//u8 *buf = //[sizeof(struct ieee80211_mgmt) + 1024];
	struct ieee80211_mgmt *reply = (struct ieee80211_mgmt *)pApInfo->pMgmtPkt;
	u8 *p;

	reply->frame_control =
		IEEE80211_FC(WLAN_FC_TYPE_MGMT,
			     (reassoc ? WLAN_FC_STYPE_REASSOC_RESP :
			      WLAN_FC_STYPE_ASSOC_RESP));
	MEMCPY((void*)reply->da, (void*)sta->addr, ETH_ALEN);
	MEMCPY((void*)reply->sa, (void*)pApInfo->own_addr, ETH_ALEN);
	MEMCPY((void*)reply->bssid, (void*)pApInfo->own_addr, ETH_ALEN);

	send_len = IEEE80211_HDRLEN;
	send_len += sizeof(reply->u.assoc_resp);
	reply->u.assoc_resp.capab_info =
		host_to_le16(hostapd_own_capab_info(pApInfo, sta, 0));
	reply->u.assoc_resp.status_code = host_to_le16(status_code);
	reply->u.assoc_resp.aid = host_to_le16((sta ? sta->aid : 0)
					       | BIT(14) | BIT(15));

//-----------------------
//Information element
	/* Supported rates */
	p = hostapd_eid_supp_rates(pApInfo, (u8*)reply->u.assoc_resp.variable);
	/* Extended supported rates */
	p = hostapd_eid_ext_supp_rates(pApInfo, p);
// 
// #ifdef CONFIG_IEEE80211R
// 	if (status_code == WLAN_STATUS_SUCCESS) {
// 		/* IEEE 802.11r: Mobility Domain Information, Fast BSS
// 		 * Transition Information, RSN, [RIC Response] */
// 		p = wpa_sm_write_assoc_resp_ies(sta->wpa_sm, p,
// 						buf + sizeof(buf) - p,
// 						sta->auth_alg, ies, ies_len);
// 	}
// #endif /* CONFIG_IEEE80211R */
// 
// #ifdef CONFIG_IEEE80211W
// 	if (status_code == WLAN_STATUS_ASSOC_REJECTED_TEMPORARILY)
// 		p = hostapd_eid_assoc_comeback_time(hapd, sta, p);
// #endif /* CONFIG_IEEE80211W */
// 
// #ifdef CONFIG_IEEE80211N
// 	p = hostapd_eid_ht_capabilities(hapd, p);
// 	p = hostapd_eid_ht_operation(hapd, p);
// #endif /* CONFIG_IEEE80211N */
// 
// 	p = hostapd_eid_ext_capab(hapd, p);

	if (test_sta_flag(sta, WLAN_STA_WMM))
		p = hostapd_eid_wmm(pApInfo, p);

// #ifdef CONFIG_WPS
// 	if ((sta->flags & WLAN_STA_WPS) ||
// 	    ((sta->flags & WLAN_STA_MAYBE_WPS) && hapd->conf->wpa)) {
// 		struct wpabuf *wps = wps_build_assoc_resp_ie();
// 		if (wps) {
// 			os_memcpy(p, wpabuf_head(wps), wpabuf_len(wps));
// 			p += wpabuf_len(wps);
// 			wpabuf_free(wps);
// 		}
// 	}
// #endif /* CONFIG_WPS */
// 
// #ifdef CONFIG_P2P
// 	if (sta->p2p_ie) {
// 		struct wpabuf *p2p_resp_ie;
// 		enum p2p_status_code status;
// 		switch (status_code) {
// 		case WLAN_STATUS_SUCCESS:
// 			status = P2P_SC_SUCCESS;
// 			break;
// 		case WLAN_STATUS_AP_UNABLE_TO_HANDLE_NEW_STA:
// 			status = P2P_SC_FAIL_LIMIT_REACHED;
// 			break;
// 		default:
// 			status = P2P_SC_FAIL_INVALID_PARAMS;
// 			break;
// 		}
// 		p2p_resp_ie = p2p_group_assoc_resp_ie(hapd->p2p_group, status);
// 		if (p2p_resp_ie) {
// 			os_memcpy(p, wpabuf_head(p2p_resp_ie),
// 				  wpabuf_len(p2p_resp_ie));
// 			p += wpabuf_len(p2p_resp_ie);
// 			wpabuf_free(p2p_resp_ie);
// 		}
// 	}
// #endif /* CONFIG_P2P */
// 
// #ifdef CONFIG_P2P_MANAGER
// 	if (hapd->conf->p2p & P2P_MANAGE)
// 		p = hostapd_eid_p2p_manage(hapd, p);
// #endif /* CONFIG_P2P_MANAGER */

	send_len += (u32)(p - reply->u.assoc_resp.variable);



	if (wpa_driver_nl80211_send_mlme( (u8 *) reply, send_len, FALSE) < 0)
		LOG_ERROR("Failed to send assoc resp:len=%d\r\n",send_len);
}







//Assoc
static void handle_assoc(struct ApInfo *pApInfo,
			 const struct ieee80211_mgmt *mgmt, size_t len,
			 int reassoc)
{
	u16 capab_info, listen_interval;
	u16 resp = WLAN_STATUS_SUCCESS;
	const u8 *pos;
	int left, i;
	APStaInfo_st *sta;

	if (len < IEEE80211_HDRLEN + (reassoc ? sizeof(mgmt->u.reassoc_req) :
		sizeof(mgmt->u.assoc_req))) {
			LOG_PRINTF("handle_assoc(reassoc=%d) - too short payload (len=%lu)"
				"\r\n", reassoc, (unsigned long) len);
			return;
	}

	if (reassoc) {
		capab_info = le_to_host16(mgmt->u.reassoc_req.capab_info);
		listen_interval = le_to_host16(
			mgmt->u.reassoc_req.listen_interval);
		LOG_DEBUG("reassociation request: STA=" MACSTR
			   " capab_info=0x%02x listen_interval=%d current_ap="
			   MACSTR "\r\n",
			   MAC2STR(mgmt->sa), capab_info, listen_interval,
			   MAC2STR(mgmt->u.reassoc_req.current_ap));
		left = len - (IEEE80211_HDRLEN + sizeof(mgmt->u.reassoc_req));
		pos = (u8 *)mgmt->u.reassoc_req.variable;
	} else {
		capab_info = le_to_host16(mgmt->u.assoc_req.capab_info);
		listen_interval = le_to_host16(
			mgmt->u.assoc_req.listen_interval);
        #ifdef __AP_DEBUG__
			LOG_DEBUG("association request: STA=" MACSTR
			   " capab_info=0x%02x listen_interval=%d\r\n",
			   MAC2STR(mgmt->sa), capab_info, listen_interval);
        #endif
            
		left = len - (IEEE80211_HDRLEN + sizeof(mgmt->u.assoc_req));
		pos = (u8 *)mgmt->u.assoc_req.variable;
	}

	sta = APStaInfo_FindStaByAddr((ETHER_ADDR *)mgmt->sa);
// #ifdef CONFIG_IEEE80211R
// 	if (sta && sta->auth_alg == WLAN_AUTH_FT &&
// 	    (sta->flags & WLAN_STA_AUTH) == 0) {
// 		wpa_printf(MSG_DEBUG, "FT: Allow STA " MACSTR " to associate "
// 			   "prior to authentication since it is using "
// 			   "over-the-DS FT", MAC2STR(mgmt->sa));
// 	} else
// #endif /* CONFIG_IEEE80211R */
	if (sta == NULL || test_sta_flag(sta, WLAN_STA_AUTH) == 0) {
		LOG_INFO("Station tried to "
			"associate before authentication "
			"(aid=%d flags=0x%x)\r\n",
			sta ? sta->aid : -1,
			sta ? sta->_flags : 0);

		i802_sta_deauth((u8 *)pApInfo->own_addr, (u8 *)mgmt->sa, WLAN_REASON_CLASS2_FRAME_FROM_NONAUTH_STA);
		return;
	}

	if (pApInfo->tkip_countermeasures) {
		resp = WLAN_REASON_MIC_FAILURE;
		goto fail;
	}

	if (listen_interval > AP_MAX_LISTEN_INTERVAL) {
		LOG_DEBUG("Too large Listen Interval (%d)\r\n",
			       listen_interval);
		resp = WLAN_STATUS_ASSOC_DENIED_LISTEN_INT_TOO_LARGE;
		goto fail;
	}

	/* followed by SSID and Supported rates; and HT capabilities if 802.11n
	 * is used */
	resp = check_assoc_ies(pApInfo, sta, pos,(size_t) left, reassoc);
	if (resp != WLAN_STATUS_SUCCESS)
		goto fail;
//-------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
//finish check. start to add station in this AP

 	if (hostapd_get_aid(pApInfo, sta) < 0) {									//---------->get aid------------------------------------------------------------------------>
		LOG_INFO("&s,No room for more AIDs\r\n",__func__);
		resp = WLAN_STATUS_AP_UNABLE_TO_HANDLE_NEW_STA;
		goto fail;
 	}

	sta->capability = capab_info;
	sta->listen_interval = listen_interval;

	if (pApInfo->eCurrentApMode == AP_MODE_IEEE80211G)
    {
        OS_MutexLock(sta->apsta_mutex);
		set_sta_flag(sta, WLAN_STA_NONERP);
        OS_MutexUnLock(sta->apsta_mutex);
    }
	for (i = 0; i < sta->supported_rates_len; i++) {
		if ((sta->supported_rates[i] & 0x7f) > 22) {
            OS_MutexLock(sta->apsta_mutex);
			clear_sta_flag(sta, WLAN_STA_NONERP);
            OS_MutexUnLock(sta->apsta_mutex);
			break;
		}
	}

//No WMM
//	if (!test_sta_flag(sta, WLAN_STA_WMM))
		


	


//Non ERP	
	if (test_sta_flag(sta, WLAN_STA_NONERP)&& !sta->nonerp_set) {
		sta->nonerp_set = 1;
		pApInfo->num_sta_non_erp++;
		if (pApInfo->num_sta_non_erp == 1)
			ieee802_11_set_beacon(pApInfo,TRUE);
	}

//Short Slot Time
	if (!(sta->capability & WLAN_CAPABILITY_SHORT_SLOT_TIME) &&
	    !sta->no_short_slot_time_set) {
		sta->no_short_slot_time_set = 1;
		pApInfo->num_sta_no_short_slot_time++;
		if (pApInfo->eCurrentApMode ==
		    AP_MODE_IEEE80211G &&
		    pApInfo->num_sta_no_short_slot_time == 1)
			ieee802_11_set_beacon(pApInfo,TRUE);
	}

	if (sta->capability & WLAN_CAPABILITY_SHORT_PREAMBLE)
    {   
        OS_MutexLock(sta->apsta_mutex);
		set_sta_flag(sta, WLAN_STA_SHORT_PREAMBLE);
        OS_MutexUnLock(sta->apsta_mutex);
    }
	else
    {  
        
        OS_MutexLock(sta->apsta_mutex);
		clear_sta_flag(sta, WLAN_STA_SHORT_PREAMBLE);
        OS_MutexUnLock(sta->apsta_mutex);
    }

	if (!(sta->capability & WLAN_CAPABILITY_SHORT_PREAMBLE) &&
		!sta->no_short_preamble_set) {
			sta->no_short_preamble_set = 1;
			pApInfo->num_sta_no_short_preamble++;
			if (pApInfo->eCurrentApMode == AP_MODE_IEEE80211G
				&& pApInfo->num_sta_no_short_preamble == 1)
				ieee802_11_set_beacon(pApInfo,TRUE);
	}
// 
// #ifdef CONFIG_IEEE80211N
// 	update_ht_state(hapd, sta);
// #endif /* CONFIG_IEEE80211N */

	LOG_DEBUG(MACSTR " association OK (aid %d)\r\n", MAC2STR(sta->addr), sta->aid);
//------------------------------------------------------------------------------------------------****************************************************************==>??	
	/* Station will be marked associated, after it acknowledges AssocResp
	 */
	OS_MutexLock(sta->apsta_mutex); 
	set_sta_flag(sta, WLAN_STA_ASSOC);
    OS_MutexUnLock(sta->apsta_mutex);


//	if ((!hapd->conf->ieee802_1x && !hapd->conf->wpa) ||
//   sta->auth_alg == WLAN_AUTH_FT) {
	/*
	 * Open, static WEP, or FT protocol; no separate authorization
	 * step.
	 */
	ap_sta_set_authorized(pApInfo, sta, 1);
	
//	wpa_msg(hapd->msg_ctx, MSG_INFO,
//		AP_STA_CONNECTED MACSTR, MAC2STR(sta->addr));
//}


// #ifdef CONFIG_IEEE80211W
// 	if ((sta->flags & WLAN_STA_MFP) && sta->sa_query_timed_out) {
// 		wpa_printf(MSG_DEBUG, "Allowing %sassociation after timed out "
// 			   "SA Query procedure", reassoc ? "re" : "");
// 		/* TODO: Send a protected Disassociate frame to the STA using
// 		 * the old key and Reason Code "Previous Authentication no
// 		 * longer valid". Make sure this is only sent protected since
// 		 * unprotected frame would be received by the STA that is now
// 		 * trying to associate.
// 		 */
// 	}
// #endif /* CONFIG_IEEE80211W */

	if (reassoc) {
		MEMCPY((void*)sta->previous_ap, (void*)mgmt->u.reassoc_req.current_ap,
			  ETH_ALEN);
	}
//Todo: check if no need
// 	if (sta->last_assoc_req)
// 		os_free(sta->last_assoc_req);
// 
// 	sta->last_assoc_req = os_malloc(len);
// 	if (sta->last_assoc_req)
// 		os_memcpy(sta->last_assoc_req, mgmt, len);

	/* Make sure that the previously registered inactivity timer will not
	 * remove the STA immediately. */
	sta->timeout_next = STA_NULLFUNC;

	//Set to driver
	
	ap_soc_cmd_sta_oper(CFG_STA_ADD, (struct ETHER_ADDR_st *)sta->addr, CFG_HT_NONE, test_sta_flag(sta, WLAN_STA_WMM));




fail:
	send_assoc_resp(pApInfo, sta, resp, reassoc, pos, (size_t)left);
}



static s32 ap_mlme_rx_assoc_request(struct ApInfo*pApInfo, struct ieee80211_mgmt *mgmt, u16 len)
{
	handle_assoc(pApInfo, mgmt, (size_t)len, 0);		
	return AP_MLME_OK;
}


// static s32 ap_mlme_rx_assoc_response(struct ApInfo*pApInfo, struct ieee80211_mgmt *mgmt, u16 len)
// {
// 	//handle_assoc(pApInfo, mgmt, len, 1);
// 	return AP_MLME_OK;
// }


static s32 ap_mlme_rx_reassoc_request(struct ApInfo*pApInfo, struct ieee80211_mgmt *mgmt, u16 len)
{
	handle_assoc(pApInfo, mgmt, (size_t)len, 1);
	return AP_MLME_OK;
}


// static s32 ap_mlme_rx_reassoc_response(struct ApInfo*pApInfo, struct ieee80211_mgmt *mgmt, u16 len)
// {
// 
// 	return AP_MLME_OK;
// }


static s32 ap_mlme_rx_probe_request(struct ApInfo*pApInfo, struct ieee80211_mgmt *mgmt, u16 len)
{
	//PKT_TxInfo *pPktTxInfo;
	struct ieee80211_mgmt *resp_mgmt;
	struct ieee802_11_elems elems;
	char *ssid;
	u8 *pos, *epos;
	const u8 *ie;
	size_t ssid_len, ie_len;
	APStaInfo_st *sta = NULL;
//	size_t buflen;
//	size_t i;


	//size_t len = pPktInfo->len;

	ie = (u8 *)mgmt->u.probe_req.variable;
	ie_len = len - (IEEE80211_HDRLEN) ;//+ sizeof(mgmt->u.probe_req));


	

#if 0
	for (i = 0; hapd->probereq_cb && i < hapd->num_probereq_cb; i++)
		if (hapd->probereq_cb[i].cb(hapd->probereq_cb[i].ctx,
			mgmt->sa, ie, ie_len) > 0)
			return;
#endif

	if (!AP_DEFAULT_SNED_PROBE_RESPONSE)
		return AP_MLME_FAILED;

	if (ieee802_11_parse_elems(ie, ie_len, &elems, 0) == ParseFailed) {
		LOG_ERROR("Could not parse ProbeReq from " MACSTR "\r\n",
			MAC2STR(mgmt->sa));
		return AP_MLME_FAILED;
	}

	ssid = NULL;
	ssid_len = 0;

	if ((!elems.ssid || !elems.supp_rates)) {
		LOG_ERROR("STA " MACSTR " sent probe request "
			"without SSID or supported rates element\r\n",
			MAC2STR(mgmt->sa));
		return AP_MLME_FAILED;
	}

#ifdef CONFIG_P2P
	if (hapd->p2p && elems.wps_ie) {
		struct wpabuf *wps;
		wps = ieee802_11_vendor_ie_concat(ie, ie_len, WPS_DEV_OUI_WFA);
		if (wps && !p2p_group_match_dev_type(hapd->p2p_group, wps)) {
			wpa_printf(MSG_MSGDUMP, "P2P: Ignore Probe Request "
				"due to mismatch with Requested Device "
				"Type");
			wpabuf_free(wps);
			return;
		}
		wpabuf_free(wps);
	}

	if (hapd->p2p && elems.p2p) {
		struct wpabuf *p2p;
		p2p = ieee802_11_vendor_ie_concat(ie, ie_len, P2P_IE_VENDOR_TYPE);
		if (p2p && !p2p_group_match_dev_id(hapd->p2p_group, p2p)) {
			wpa_printf(MSG_MSGDUMP, "P2P: Ignore Probe Request "
				"due to mismatch with Device ID");
			wpabuf_free(p2p);
			return;
		}
		wpabuf_free(p2p);
	}
#endif /* CONFIG_P2P */

	if (AP_DEFAULT_IGNORE_BROADCAST_SSID && elems.ssid_len == 0) {
		LOG_ERROR("Probe Request from " MACSTR " for "
			"broadcast SSID ignored\r\n", MAC2STR(mgmt->sa));
		return AP_MLME_FAILED;
	}

	sta = APStaInfo_FindStaByAddr((ETHER_ADDR *)mgmt->sa);
#ifdef CONFIG_P2P
	if ((hapd->conf->p2p & P2P_GROUP_OWNER) &&
		elems.ssid_len == P2P_WILDCARD_SSID_LEN &&
		os_memcmp(elems.ssid, P2P_WILDCARD_SSID,
		P2P_WILDCARD_SSID_LEN) == 0) {
			/* Process P2P Wildcard SSID like Wildcard SSID */
			elems.ssid_len = 0;
	}
#endif /* CONFIG_P2P */
	

	if (elems.ssid_len == 0 ||
		(elems.ssid_len == gDeviceInfo->APInfo->config.ssid_len &&
		MEMCMP((void*)elems.ssid, (void*)gDeviceInfo->APInfo->config.ssid, elems.ssid_len) == 0)) {
			ssid =  gDeviceInfo->APInfo->config.ssid;
			ssid_len = gDeviceInfo->APInfo->config.ssid_len;
			
			//Todo: SEC
			//if (sta)
			//	sta->ssid_probe = &gAPInfo->ssid;
	}

	if (!ssid) {
		/*if (!(mgmt->da[0] & 0x01)) {*/
			char ssid_txt[33];
			ieee802_11_print_ssid(ssid_txt, elems.ssid,
				elems.ssid_len);
#ifdef __LOG_DEGUG__
			LOG_ERROR("Probe Request from " MACSTR	" for foreign SSID '%s'\n",
				MAC2STR(mgmt->sa), ssid_txt);
#endif			
		//}		

		return AP_MLME_FAILED;
	}

#ifdef CONFIG_INTERWORKING
	if (elems.interworking && elems.interworking_len >= 1) {
		u8 ant = elems.interworking[0] & 0x0f;
		if (ant != INTERWORKING_ANT_WILDCARD &&
			ant != hapd->conf->access_network_type) {
				wpa_printf(MSG_MSGDUMP, "Probe Request from " MACSTR
					" for mismatching ANT %u ignored",
					MAC2STR(mgmt->sa), ant);
				return;
		}
	}

	if (elems.interworking &&
		(elems.interworking_len == 7 || elems.interworking_len == 9)) {
			const u8 *hessid;
			if (elems.interworking_len == 7)
				hessid = elems.interworking + 1;
			else
				hessid = elems.interworking + 1 + 2;
			if (!is_broadcast_ether_addr(hessid) &&
				os_memcmp(hessid, hapd->conf->hessid, ETH_ALEN) != 0) {
					wpa_printf(MSG_MSGDUMP, "Probe Request from " MACSTR
						" for mismatching HESSID " MACSTR
						" ignored",
						MAC2STR(mgmt->sa), MAC2STR(hessid));
					return;
			}
	}
#endif /* CONFIG_INTERWORKING */
//***********************************************************************************************************************************	
//***********************************************************************************************************************************
//***********************************************************************************************************************************


	/* TODO: verify that supp_rates contains at least one matching rate
	* with AP configuration */
//#define MAX_PROBERESP_LEN 768
//	buflen = AP_MGMT_PKT_LEN;
#ifdef CONFIG_WPS
	if (hapd->wps_probe_resp_ie)
		buflen += wpabuf_len(hapd->wps_probe_resp_ie);
#endif /* CONFIG_WPS */
#ifdef CONFIG_P2P
	if (hapd->p2p_probe_resp_ie)
		buflen += wpabuf_len(hapd->p2p_probe_resp_ie);
#endif /* CONFIG_P2P */


	/* IEEE 802.11 MGMT frame header */

	resp_mgmt = (struct ieee80211_mgmt *)gDeviceInfo->APInfo->pMgmtPkt;
	epos = ((u8 *) resp_mgmt) + AP_MGMT_PKT_LEN;

	resp_mgmt->frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT,
		WLAN_FC_STYPE_PROBE_RESP);
	MEMCPY((void*)resp_mgmt->da, (void*)mgmt->sa, ETH_ALEN);
	MEMCPY((void*)resp_mgmt->sa, (void*)gDeviceInfo->APInfo->own_addr, ETH_ALEN);

	MEMCPY((void*)resp_mgmt->bssid, (void*)gDeviceInfo->APInfo->own_addr, ETH_ALEN);

	/* IEEE 802.11 MGMT frame body: information elements: */
	resp_mgmt->u.probe_resp.beacon_int =
		host_to_le16(AP_DEFAULT_BEACON_INT);

	/* hardware or low-level driver will setup seq_ctrl and timestamp */
	resp_mgmt->u.probe_resp.capab_info =
		host_to_le16(hostapd_own_capab_info(gDeviceInfo->APInfo, sta, 1));

	pos = (u8 *)resp_mgmt->u.probe_resp.variable;
	*pos++ = WLAN_EID_SSID;
	*pos++ = ssid_len;
	MEMCPY(pos, ssid, ssid_len);
	pos += ssid_len;

	/* Supported rates */
	pos = hostapd_eid_supp_rates(gDeviceInfo->APInfo,pos);

	/* DS Params */
	pos = hostapd_eid_ds_params(gDeviceInfo->APInfo, pos);

	pos = hostapd_eid_country(gDeviceInfo->APInfo, pos, epos - pos);

	/* ERP Information element */
	pos = hostapd_eid_erp_info(gDeviceInfo->APInfo, pos);

	/* Extended supported rates */
	pos = hostapd_eid_ext_supp_rates(gDeviceInfo->APInfo, pos);

#pragma message("===================================================")
#pragma message("     hostapd_eid_wpa not implement yet")
#pragma message("===================================================")
	/* RSN, MDIE, WPA */
	pos = hostapd_eid_wpa(gDeviceInfo->APInfo, pos, (size_t)(epos - pos));


#if 0
#ifdef CONFIG_IEEE80211N
	pos = hostapd_eid_ht_capabilities(hapd, pos);
	pos = hostapd_eid_ht_operation(hapd, pos);
#endif /* CONFIG_IEEE80211N */

	pos = hostapd_eid_ext_capab(hapd, pos);

	pos = hostapd_eid_time_adv(hapd, pos);
	pos = hostapd_eid_time_zone(hapd, pos);

	pos = hostapd_eid_interworking(hapd, pos);
	pos = hostapd_eid_adv_proto(hapd, pos);
	pos = hostapd_eid_roaming_consortium(hapd, pos);
#endif
	/* Wi-Fi Alliance WMM */
	pos = hostapd_eid_wmm(pApInfo, pos);
#if 0
#ifdef CONFIG_WPS
	if (hapd->conf->wps_state && hapd->wps_probe_resp_ie) {
		os_memcpy(pos, wpabuf_head(hapd->wps_probe_resp_ie),
			wpabuf_len(hapd->wps_probe_resp_ie));
		pos += wpabuf_len(hapd->wps_probe_resp_ie);
	}
#endif /* CONFIG_WPS */

#ifdef CONFIG_P2P
	if ((hapd->conf->p2p & P2P_ENABLED) && elems.p2p &&
		hapd->p2p_probe_resp_ie) {
			os_memcpy(pos, wpabuf_head(hapd->p2p_probe_resp_ie),
				wpabuf_len(hapd->p2p_probe_resp_ie));
			pos += wpabuf_len(hapd->p2p_probe_resp_ie);
	}
#endif /* CONFIG_P2P */
#ifdef CONFIG_P2P_MANAGER
	if ((hapd->conf->p2p & (P2P_MANAGE | P2P_ENABLED | P2P_GROUP_OWNER)) ==
		P2P_MANAGE)
		pos = hostapd_eid_p2p_manage(hapd, pos);
#endif /* CONFIG_P2P_MANAGER */

	/* IEEE 802.11 MGMT frame body: information elements: */
	//mgmt->u.auth.auth_algo   = _LE16(auth_algo);
	//mgmt->u.auth.trans_id    = _LE16(auth_transaction);
	//mgmt->u.auth.status_code = _LE16(0);
#endif







	ap_soc_data_send((void *)gDeviceInfo->APInfo->pOSMgmtframe, pos-gDeviceInfo->APInfo->pMgmtPkt, TRUE, 0);




	return AP_MLME_OK;
}


// static s32 ap_mlme_rx_probe_response(struct ApInfo*pApInfo, struct ieee80211_mgmt *mgmt, u16 len)
// {
// 
// 	return AP_MLME_OK;
// }


static s32 ap_mlme_rx_beacon(struct ApInfo*pApInfo, struct ieee80211_mgmt *mgmt, u16 len)
{

	struct ieee802_11_elems elems;
	const u8 *ie;
	size_t ie_len;
	

	ie = (u8 *)mgmt->u.beacon.variable;
	ie_len = len - (IEEE80211_HDRLEN + sizeof(mgmt->u.beacon));

	
	if (len < IEEE80211_HDRLEN + sizeof(mgmt->u.beacon)) {
		LOG_ERROR("handle_beacon - too short payload (len=%lu)\r\n",
		       (unsigned long) len);
		return AP_MLME_FAILED;
	}

	(void) ieee802_11_parse_elems(ie,
				      ie_len, &elems,
				      0);

	neighbor_ap_list_process_beacon(gDeviceInfo->APInfo, mgmt, &elems);
	



	return AP_MLME_OK;
}


static s32 ap_mlme_rx_atim(struct ApInfo*pApInfo, struct ieee80211_mgmt *mgmt, u16 len)
{

	return AP_MLME_OK;
}


static s32 ap_mlme_rx_disassoc(struct ApInfo*pApInfo, struct ieee80211_mgmt *mgmt, u16 len)
{


	APStaInfo_st *sta;

	if (len < IEEE80211_HDRLEN + sizeof(mgmt->u.disassoc)) {
		LOG_PRINTF("handle_disassoc - too short payload (len=%lu)\r\n",
		       (unsigned long) len);
		return AP_MLME_FAILED;
	}

	LOG_DEBUG("disassocation: STA=" MACSTR " reason_code=%d\r\n",
		   MAC2STR(mgmt->sa),
		   le_to_host16(mgmt->u.disassoc.reason_code));

	sta = APStaInfo_FindStaByAddr((ETHER_ADDR*)mgmt->sa);
	if (sta == NULL) {
		LOG_DEBUG("Station " MACSTR " trying to disassociate, but it "
		       "is not associated.\r\n", MAC2STR(mgmt->sa));
		return AP_MLME_FAILED;
	}

	//Todo: SEC
	ap_sta_set_authorized(pApInfo, sta, 0);
	
	sta->_flags &= ~(WLAN_STA_ASSOC | WLAN_STA_ASSOC_REQ_OK);
	//wpa_auth_sm_event(sta->wpa_sm, WPA_DISASSOC);
	LOG_INFO("Station " MACSTR " disassociated\r\n", MAC2STR(mgmt->sa));
	
	//sta->acct_terminate_cause = RADIUS_ACCT_TERMINATE_CAUSE_USER_REQUEST;
	//ieee802_1x_notify_port_enabled(sta->eapol_sm, 0);
	/* Stop Accounting and IEEE 802.1X sessions, but leave the STA
	 * authenticated. */
	//accounting_sta_stop(hapd, sta);
	//ieee802_1x_free_station(sta);
	//hapd->drv.sta_remove(hapd, sta->addr);
	APStaInfo_DrvRemove(pApInfo, sta);

	if (sta->timeout_next == STA_NULLFUNC ||
	    sta->timeout_next == STA_DISASSOC) {
		sta->timeout_next = STA_DEAUTH;
		os_cancel_timer(ap_handle_timer,  (u32)pApInfo, (u32)sta);
        os_create_timer(AP_DEAUTH_DELAY, ap_handle_timer,pApInfo, sta,(void*)TIMEOUT_TASK);
	}

// 	mlme_disassociate_indication(
// 		hapd, sta, le_to_host16(mgmt->u.disassoc.reason_code));


#ifdef __AP_DEBUG__
	APStaInfo_PrintStaInfo();	
#endif//__AP_DEBUG__

	return AP_MLME_OK;
}


//-----------------------------------------------------------------
//Authentication

static void send_auth_reply(struct ApInfo*pApInfo, const u8 *dst, const u8 *bssid,
	u16 auth_alg, u16 auth_transaction, u16 resp,
	const u8 *ies, size_t ies_len)
{
	struct ieee80211_mgmt *reply;
	u8 *buf;
	size_t rlen;

	rlen = IEEE80211_HDRLEN + sizeof(reply->u.auth) + ies_len;
	buf = pApInfo->pMgmtPkt;   //os_zalloc(rlen);
// 	if (buf == NULL)
// 		return;

	reply = (struct ieee80211_mgmt *) buf;
	reply->frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT,
		WLAN_FC_STYPE_AUTH);
	MEMCPY((void*)reply->da, (void*)dst, ETH_ALEN);
	MEMCPY((void*)reply->sa, (void*)gDeviceInfo->APInfo->own_addr, ETH_ALEN);
	MEMCPY((void*)reply->bssid, (void*)bssid, ETH_ALEN);

	reply->u.auth.auth_alg = host_to_le16(auth_alg);
	reply->u.auth.auth_transaction = host_to_le16(auth_transaction);
	reply->u.auth.status_code = host_to_le16(resp);

	if (ies && ies_len)
		MEMCPY((void*)reply->u.auth.variable, (void*)ies, ies_len);

    #ifdef __AP_DEBUG__
	LOG_DEBUG("authentication reply: STA=" MACSTR
		" auth_alg=%d auth_transaction=%d resp=%d (IE len=%lu)\n",
		MAC2STR(dst), auth_alg, auth_transaction,
		resp, (unsigned long) ies_len);
    #endif
    
	if (wpa_driver_nl80211_send_mlme((const u8*)reply, rlen, TRUE) < 0)
		LOG_DEBUG("send_auth_reply: send,%s",__func__);
	
	//os_free(buf);
}




static s32 ap_mlme_rx_auth(struct ApInfo*pApInfo, struct ieee80211_mgmt *mgmt, u16 len)
{
	
	u16 auth_alg, auth_transaction, status_code;
	u16 resp = WLAN_STATUS_SUCCESS;
	APStaInfo_st *sta = NULL;

	u16 fc;
	const u8 *challenge = NULL;

//	int vlan_id = 0;
	u8 resp_ies[2 + WLAN_AUTH_CHALLENGE_LEN];
	size_t resp_ies_len = 0;
	



	if (len < IEEE80211_HDRLEN + sizeof(mgmt->u.auth)) {
		LOG_ERROR("handle_auth - too short payload (len=%lu)\r\n",
		       (unsigned long) len);
		return AP_MLME_FAILED;
	}

	auth_alg = le_to_host16(mgmt->u.auth.auth_alg);
	auth_transaction = le_to_host16(mgmt->u.auth.auth_transaction);
	status_code = le_to_host16(mgmt->u.auth.status_code);
	fc = le_to_host16(mgmt->frame_control);

	if (len >= IEEE80211_HDRLEN + sizeof(mgmt->u.auth) +
	    2 + WLAN_AUTH_CHALLENGE_LEN &&
	    mgmt->u.auth.variable[0] == WLAN_EID_CHALLENGE &&
	    mgmt->u.auth.variable[1] == WLAN_AUTH_CHALLENGE_LEN)
		challenge = (u8 *)&mgmt->u.auth.variable[2];
#ifdef __AP_DEBUG__

	LOG_TRACE("authentication: STA=" MACSTR " auth_alg=%d "
		   "auth_transaction=%d status_code=%d wep=%d%s\r\n",
		   MAC2STR(mgmt->sa), auth_alg, auth_transaction,
		   status_code, !!(fc & WLAN_FC_ISWEP),
		   challenge ? " challenge" : "");
#endif
	if (pApInfo->tkip_countermeasures) {
		resp = WLAN_REASON_MIC_FAILURE;
		goto fail;
	}

	if (!(
		(((u32)pApInfo->auth_algs & WPA_AUTH_ALG_OPEN) &&
	       (u32)auth_alg == WLAN_AUTH_OPEN) ||
//#ifdef CONFIG_IEEE80211R
//	      (hapd->conf->wpa &&
//	       (hapd->conf->wpa_key_mgmt &
//		(WPA_KEY_MGMT_FT_IEEE8021X | WPA_KEY_MGMT_FT_PSK)) &&
//	       auth_alg == WLAN_AUTH_FT) ||
//#endif /* CONFIG_IEEE80211R */
	      (((u32)pApInfo->auth_algs & WPA_AUTH_ALG_SHARED) &&
	       (u32)auth_alg == WLAN_AUTH_SHARED_KEY))) {
		LOG_DEBUG("Unsupported authentication algorithm (%d)\r\n",
		       auth_alg);
		resp = WLAN_STATUS_NOT_SUPPORTED_AUTH_ALG;
		goto fail;
	}

	if (!(auth_transaction == 1 ||
	      (auth_alg == WLAN_AUTH_SHARED_KEY && auth_transaction == 3))) {
		LOG_DEBUG("Unknown authentication transaction number (%d)\r\n",
		       auth_transaction);
		resp = WLAN_STATUS_UNKNOWN_AUTH_TRANSACTION;
		goto fail;
	}

	if (MEMCMP((void*)mgmt->sa, (void*)pApInfo->own_addr, ETH_ALEN) == 0) {
		LOG_DEBUG("Station " MACSTR " not allowed to authenticate.\r\n",
		       MAC2STR(mgmt->sa));
		resp = WLAN_STATUS_UNSPECIFIED_FAILURE;
		goto fail;
	}

// 	res = hostapd_allowed_address(hapd, mgmt->sa, (u8 *) mgmt, len,
// 				      &session_timeout,
// 				      &acct_interim_interval, &vlan_id);
// 	if (res == HOSTAPD_ACL_REJECT) {
// 		LOG_TRACE("Station " MACSTR " not allowed to authenticate.\n",
// 		       MAC2STR(mgmt->sa));
// 		resp = WLAN_STATUS_UNSPECIFIED_FAILURE;
// 		goto fail;
// 	}
// 	if (res == HOSTAPD_ACL_PENDING) {
// 		wpa_printf(MSG_DEBUG, "Authentication frame from " MACSTR
// 			   " waiting for an external authentication",
// 			   MAC2STR(mgmt->sa));
// 		/* Authentication code will re-send the authentication frame
// 		 * after it has received (and cached) information from the
// 		 * external source. */
// 		return;
// 	}

	sta = APStaInfo_add(pApInfo, (u8 *)mgmt->sa);
	if (!sta) {
		resp = WLAN_STATUS_AP_UNABLE_TO_HANDLE_NEW_STA;//WLAN_STATUS_UNSPECIFIED_FAILURE;
		goto fail;
	}


// 	if (vlan_id > 0) {
// 		if (hostapd_get_vlan_id_ifname(hapd->conf->vlan,
// 					       vlan_id) == NULL) {
// 			hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_RADIUS,
// 				       HOSTAPD_LEVEL_INFO, "Invalid VLAN ID "
// 				       "%d received from RADIUS server",
// 				       vlan_id);
// 			resp = WLAN_STATUS_UNSPECIFIED_FAILURE;
// 			goto fail;
// 		}
// 		sta->vlan_id = vlan_id;
// 		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_RADIUS,
// 			       HOSTAPD_LEVEL_INFO, "VLAN ID %d", sta->vlan_id);
// 	}
// 
// 	sta->flags &= ~WLAN_STA_PREAUTH;
// 	ieee802_1x_notify_pre_auth(sta->eapol_sm, 0);
// 
// 	if (hapd->conf->acct_interim_interval == 0 && acct_interim_interval)
// 		sta->acct_interim_interval = acct_interim_interval;
// 	if (res == HOSTAPD_ACL_ACCEPT_TIMEOUT)
// 		ap_sta_session_timeout(hapd, sta, session_timeout);
// 	else
// 		ap_sta_no_session_timeout(hapd, sta);
// 
	switch (auth_alg) {
	case WLAN_AUTH_OPEN:
#ifdef __AP_DEBUG__
		LOG_DEBUG("authentication OK (open system),%s\n",__func__);
#endif
#ifdef IEEE80211_REQUIRE_AUTH_ACK
		/* Station will be marked authenticated if it ACKs the
		 * authentication reply. */
#else
        OS_MutexLock(sta->apsta_mutex);
		set_sta_flag(sta, WLAN_STA_AUTH);
        OS_MutexUnLock(sta->apsta_mutex);
		//wpa_auth_sm_event(sta->wpa_sm, WPA_AUTH);
		sta->auth_alg = WLAN_AUTH_OPEN;
		//mlme_authenticate_indication(hapd, sta);
#endif
		break;
	case WLAN_AUTH_SHARED_KEY:
 		resp = auth_shared_key(pApInfo, sta, auth_transaction, challenge,
 			fc & WLAN_FC_ISWEP);
 		sta->auth_alg = WLAN_AUTH_SHARED_KEY;
 		//mlme_authenticate_indication(hapd, sta);
 		if (sta->challenge && auth_transaction == 1) {
 			resp_ies[0] = WLAN_EID_CHALLENGE;
 			resp_ies[1] = WLAN_AUTH_CHALLENGE_LEN;
 			MEMCPY(resp_ies + 2, sta->challenge,
 				WLAN_AUTH_CHALLENGE_LEN);
 			resp_ies_len = 2 + WLAN_AUTH_CHALLENGE_LEN;
 		}
		break;
//#ifdef CONFIG_IEEE80211R
//	case WLAN_AUTH_FT:
//		sta->auth_alg = WLAN_AUTH_FT;
//		if (sta->wpa_sm == NULL)
//			sta->wpa_sm = wpa_auth_sta_init(hapd->wpa_auth,
//							sta->addr);
//		if (sta->wpa_sm == NULL) {
//			LOG_DEBUG("FT: Failed to initialize WPA "
//				   "state machine");
//			resp = WLAN_STATUS_UNSPECIFIED_FAILURE;
//			goto fail;
//		}
//		wpa_ft_process_auth(sta->wpa_sm, mgmt->bssid,
//				    auth_transaction, mgmt->u.auth.variable,
//				    len - IEEE80211_HDRLEN -
//				    sizeof(mgmt->u.auth),
//				    handle_auth_ft_finish, hapd);
//		/* handle_auth_ft_finish() callback will complete auth. */
//		return;
//#endif /* CONFIG_IEEE80211R */
	}

	fail:
	send_auth_reply(pApInfo, (u8 *)mgmt->sa, (u8 *)mgmt->bssid, auth_alg,
			auth_transaction + 1, resp, resp_ies, resp_ies_len);
	
	return AP_MLME_OK;
}


static s32 ap_mlme_rx_deauth(struct ApInfo*pApInfo, struct ieee80211_mgmt *mgmt, u16 len)
{
	APStaInfo_st *sta;

	if (len < IEEE80211_HDRLEN + sizeof(mgmt->u.deauth)) {

		LOG_DEBUG("handle_deauth - too short "
			"payload (len=%lu)\r\n", (unsigned long) len);
		return AP_MLME_FAILED;
	}

	LOG_DEBUG("deauthentication: STA=" MACSTR
		" reason_code=%d\r\n",
		MAC2STR(mgmt->sa), le_to_host16(mgmt->u.deauth.reason_code));

	sta = APStaInfo_FindStaByAddr((ETHER_ADDR*)mgmt->sa);
	if (sta == NULL) {
		LOG_DEBUG("Station " MACSTR " trying "
			"to deauthenticate, but it is not authenticated\r\n",
			MAC2STR(mgmt->sa));
		return AP_MLME_FAILED;
	}

	ap_sta_set_authorized(pApInfo, sta, 0);

	
	sta->_flags &= ~(WLAN_STA_AUTH | WLAN_STA_ASSOC |
		WLAN_STA_ASSOC_REQ_OK);
	//wpa_auth_sm_event(sta->wpa_sm, WPA_DEAUTH);
	
// 	mlme_deauthenticate_indication(
// 		hapd, sta, le_to_host16(mgmt->u.deauth.reason_code));
// 	sta->acct_terminate_cause = RADIUS_ACCT_TERMINATE_CAUSE_USER_REQUEST;
// 	ieee802_1x_notify_port_enabled(sta->eapol_sm, 0);
	
	APStaInfo_free(pApInfo, sta);

	return AP_MLME_OK;
}


static s32 ap_mlme_rx_action(struct ApInfo*pApInfo, struct ieee80211_mgmt *mgmt, u16 len)
{
	return AP_MLME_OK;
}

/**
*  IEEE 802.11 Management Frame Handler:
*/

AP_MGMT80211_RxHandler AP_RxHandler[] = 
{
	ap_mlme_rx_assoc_request, //Association Request Handler (subtype=00)
	NULL, //Association Response Handler (subtype=01)
	ap_mlme_rx_reassoc_request,// Re-association Request Handler (subtype=02)
	NULL,//Re-association Response Handler (subtype=03)
	ap_mlme_rx_probe_request,//Probe Request Handler (subtype=04)
	NULL,//Probe Response Handler (subtype=05)
	NULL,//Reserved (subtype=06)
	NULL,//Reserved (subtype=07)
	ap_mlme_rx_beacon,//Beacon Handler (subtype=08)
	ap_mlme_rx_atim,//ATIM Handler (subtype=09)
	
	ap_mlme_rx_disassoc,//Disassociation Handler (subtype=10)
	ap_mlme_rx_auth,//Authentication Handler (subtype=11)
	ap_mlme_rx_deauth,//De-authentication Handler (subtype=12)


	ap_mlme_rx_action,//Action Handler (subtype=13)
};



void AP_RxHandleAPMode(void *frame)
{
	struct cfg_host_rxpkt *pPktInfo = (struct cfg_host_rxpkt *)OS_FRAME_GET_DATA(frame);
	u8 bFreePacket = TRUE;
	u8 *raw = ssv6xxx_host_rx_data_get_data_ptr(pPktInfo);
	u16 fc = (raw[1]<<8) | raw[0];
	u32 nDataLen = pPktInfo->len - (u32)(raw-(u8*)pPktInfo);
	
	if ( WLAN_FC_GET_TYPE(fc) == WLAN_FC_TYPE_MGMT )
	{
		u8 stype;
		stype = WLAN_FC_GET_STYPE(fc);


		

		if ( stype < ARRAY_SIZE(AP_RxHandler) )
		{

			struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)(raw);
			APStaInfo_st * sta = APStaInfo_FindStaByAddr((ETHER_ADDR *)mgmt->sa);
			if(sta)
				sta->last_rx = os_sys_jiffies();
			
			MEMSET((void*)gDeviceInfo->APInfo->pMgmtPkt, 0, AP_MGMT_PKT_LEN);

			//LOG_TRACE("AP get frame. Frame type %d\n",stype);

			if ( AP_RxHandler[stype] !=NULL )
			{
				AP_RxHandler[stype](gDeviceInfo->APInfo, mgmt, nDataLen);
			}
			else
			{
				LOG_TRACE("not implement manager func [sub type:%x]\r\n",stype);				
			}
		}
		else
		{
			LOG_TRACE("error! out of func bound [sub type:%x]\r\n",stype);			
		}

	}
	else
	{
		LOG_TRACE("not accept type [type:%x]\r\n",WLAN_FC_GET_TYPE(fc));		
	}


	if (bFreePacket)
		os_frame_free(frame);

}

