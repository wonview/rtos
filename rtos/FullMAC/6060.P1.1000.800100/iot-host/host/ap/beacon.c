#define __SFILE__ "beacon.c"

#include <config.h>
#include <log.h>
#include "common/ieee802_11_defs.h"
#include "common/ieee802_11_common.h"
#include "common/ieee80211.h"
#include <pbuf.h>
#include "common/bitmap.h"

#include "ssv_timer.h"
#include "ap_tx.h"
#include "beacon.h"
#include "ap_info.h"
#include "ap_drv_cmd.h"
#include <os_wrapper.h>
#include "ieee802_11_mgmt.h"
#include "wmm.h"
#include <host_apis.h>
#include "dev.h"

void GenBeacon();


//For 80211G
static u8 ieee802_11_erp_info(ApInfo_st *pApInfo)
{
	u8 erp = 0;

	if (pApInfo->eCurrentApMode != AP_MODE_IEEE80211G)
		return 0;

	if (pApInfo->olbc)
		erp |= ERP_INFO_USE_PROTECTION;

	if (pApInfo->num_sta_non_erp > 0) {
		erp |= ERP_INFO_NON_ERP_PRESENT |
			ERP_INFO_USE_PROTECTION;
	}
	
	if (pApInfo->num_sta_no_short_preamble > 0 ||
	    pApInfo->config.preamble == LONG_PREAMBLE)
		erp |= ERP_INFO_BARKER_PREAMBLE_MODE;

	return erp;
}


u8 * hostapd_eid_ds_params(ApInfo_st *pApInfo, u8 *eid)
{
	*eid++ = WLAN_EID_DS_PARAMS;
	*eid++ = 1;
	*eid++ = pApInfo->nCurrentChannel;// hapd->iconf->channel;
	return eid;
}


u8 * hostapd_eid_erp_info(ApInfo_st *pApInfo, u8 *eid)
{
	if (pApInfo->eCurrentApMode != AP_MODE_IEEE80211G)
		return eid;

	/* Set NonERP_present and use_protection bits if there
	 * are any associated NonERP stations. */

	/* TODO: 
	/*A. use_protection bit can be set to zero even if
	 * there are NonERP stations present. This optimization
	 * might be useful if NonERP stations are "quiet"	 
	 * See 802.11g/D6 E-1 for recommended practice.
	 
	 * B. In addition, Non ERP present might be set, if AP detects Non ERP
	 * operation on other APs. */

	/* Add ERP Information element */
	*eid++ = WLAN_EID_ERP_INFO;
	*eid++ = 1;
	*eid++ = ieee802_11_erp_info(pApInfo);

	return eid;
}

#if 0
static u8 * hostapd_eid_country_add(u8 *pos, u8 *end, int chan_spacing,
				    struct ieee80211_channel_data *start,
				    struct ieee80211_channel_data *prev)
{
	if (end - pos < 3)
		return pos;

	/* first channel number */
	*pos++ = start->chan;
	/* number of channels */
	*pos++ = (prev->chan - start->chan) / chan_spacing + 1;
	/* maximum transmit power level */
	*pos++ = start->max_tx_power;

	return pos;
}
#endif 

u8 * hostapd_eid_country(ApInfo_st *pApInfo, u8 *eid,
				int max_len)
{
	u8 *pos = eid;
//	u8 *end = eid + max_len;
//	int i;
	//struct hostapd_hw_modes *mode;
//	struct ieee80211_channel_data *start, *prev;
//	int chan_spacing = 1;

	if (max_len < 6)
		return eid;

	*pos++ = WLAN_EID_COUNTRY;
	pos++; /* length will be set later */



//-------------------------------------------------
//Hardcode

	MEMCPY(pos, "GB ", 3); /* e.g., 'US ' */
	pos += 3;
	/* first channel number */
	*pos++ = 1;
	/* number of channels */
	*pos++ = 11;
	/* maximum transmit power level */
	*pos++ = 16;
	
//-------------------------------------------------	

#if 0
	MEMCPY(pos, pApInfo->config.country, 3); /* e.g., 'US ' */




	pos += 3;


// 	mode = hapd->iface->current_mode;
// 	if (mode->mode == HOSTAPD_MODE_IEEE80211A)
// 		chan_spacing = 4;



	start = prev = NULL;
	for (i = 0; i < AP_MAX_CHANNEL; i++) {
		struct ieee80211_channel_data *chan = &pApInfo->stChannelData[i];
		if (chan->flag & IEEE80211_CHAN_FLAGS_DISABLED)
			continue;
		if (start && prev &&
		    prev->chan + chan_spacing == chan->chan &&
		    start->max_tx_power == chan->max_tx_power) {
			prev = chan;
			continue; /* can use same entry */
		}

		if (start) {
			pos = hostapd_eid_country_add(pos, end, chan_spacing,
						      start, prev);
			start = NULL;
		}

		/* Start new group */
		start = prev = chan;
	}

	if (start) {
		pos = hostapd_eid_country_add(pos, end, chan_spacing,
					      start, prev);
	}

	if ((pos - eid) & 1) {
		if (end - pos < 1)
			return eid;
		*pos++ = 0; /* pad for 16-bit alignment */
	}
#endif	

	eid[1] = (pos - eid) - 2;

	return pos;
}

#pragma message("===================================================")
#pragma message("       wpa_auth_get_wpa_ie not implement yet")
#pragma message("===================================================")
u8 * hostapd_eid_wpa(ApInfo_st *pApInfo, u8 *eid, size_t len)
{
	const u8 *ie;
	size_t ielen;

//*********************
	//not include the field firstly
	return  eid;

//	ie = wpa_auth_get_wpa_ie(hapd->wpa_auth, &ielen);
	if (ie == NULL || ielen > len)
		return eid;

	MEMCPY(eid, ie, ielen);
	return eid + ielen;
}


void ieee802_11_set_beacon(ApInfo_st *pApInfo, bool post_to_mlme)
{
//--------------------------------
//Head-TIM-Tail
//--------------------------------

	struct ieee80211_mgmt *head;
	u8 *pos, *tailpos;
	u16 capab_info;
	

#ifdef CONFIG_WPS
	if (hapd->conf->wps_state && hapd->wps_beacon_ie)
		tail_len += wpabuf_len(hapd->wps_beacon_ie);
#endif /* CONFIG_WPS */
#ifdef CONFIG_P2P
	if (hapd->p2p_beacon_ie)
		tail_len += wpabuf_len(hapd->p2p_beacon_ie);
#endif /* CONFIG_P2P */



    head = (struct ieee80211_mgmt *)pApInfo->pBeaconHead;


	head->frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT,
					   WLAN_FC_STYPE_BEACON);
	head->duration = host_to_le16(0);

	MEMSET((void*)head->da, 0xff, ETH_ALEN);



	

	MEMCPY((void*)head->sa, (void*)pApInfo->own_addr, ETH_ALEN);
	MEMCPY((void*)head->bssid, (void*)pApInfo->own_addr, ETH_ALEN);

	head->u.beacon.beacon_int =
		host_to_le16(AP_DEFAULT_BEACON_INT);

	/* hardware or low-level driver will setup seq_ctrl and timestamp */

	capab_info = hostapd_own_capab_info(pApInfo, NULL, 0);
	head->u.beacon.capab_info = host_to_le16(capab_info);
	pos = (u8 *)&head->u.beacon.variable[0];

	/* SSID */
	*pos++ = WLAN_EID_SSID;
// 	if (hapd->conf->ignore_broadcast_ssid == 2) {
// 		/* clear the data, but keep the correct length of the SSID */
// 		*pos++ = hapd->conf->ssid.ssid_len;
// 		os_memset(pos, 0, hapd->conf->ssid.ssid_len);
// 		pos += hapd->conf->ssid.ssid_len;
// 	} else if (hapd->conf->ignore_broadcast_ssid) {
// 		*pos++ = 0; /* empty SSID */
// 	} else {
	
		*pos++ = pApInfo->config.ssid_len;
		MEMCPY(pos, pApInfo->config.ssid,
			  pApInfo->config.ssid_len);
		pos += pApInfo->config.ssid_len;
//	}


	/* Supported rates */
	pos = hostapd_eid_supp_rates(pApInfo, pos);

	/* DS Params */
	pos = hostapd_eid_ds_params(pApInfo, pos);

	pApInfo->nBHeadLen = pos - (u8 *) head;

//-----------------------------------------------------
//-----------------------------------------------------

//TIM
	pApInfo->pBeaconTim = pos;

//***************************************************************************
//***************************************************************************
	// Tail
	tailpos = pApInfo->pBeaconTail;


	//Plus 2 for 
	//pApInfo->pBeaconTail = tailpos = pos;

	tailpos = hostapd_eid_country(pApInfo, tailpos,
		AP_MGMT_BEACON_LEN - pApInfo->nBHeadLen);

	/* ERP Information element */
	tailpos = hostapd_eid_erp_info(pApInfo, tailpos);

 	/* Extended supported rates */
 	tailpos = hostapd_eid_ext_supp_rates(pApInfo, tailpos);
 
 	/* RSN, MDIE, WPA */
 	tailpos = hostapd_eid_wpa(pApInfo, tailpos,(size_t) AP_BEACON_TAIL_BUF_SIZE - (tailpos-(u8*)pApInfo->pBeaconTail) );
 
#ifdef CONFIG_IEEE80211N
	tailpos = hostapd_eid_ht_capabilities(hapd, tailpos);
	tailpos = hostapd_eid_ht_operation(hapd, tailpos);
#endif /* CONFIG_IEEE80211N */

#if 0
	tailpos = hostapd_eid_ext_capab(hapd, tailpos);

	/*
	 * TODO: Time Advertisement element should only be included in some
	 * DTIM Beacon frames.
	 */
	tailpos = hostapd_eid_time_adv(hapd, tailpos);

	tailpos = hostapd_eid_interworking(hapd, tailpos);
	tailpos = hostapd_eid_adv_proto(hapd, tailpos);
	tailpos = hostapd_eid_roaming_consortium(hapd, tailpos);
#endif
	/* Wi-Fi Alliance WMM */
	tailpos = hostapd_eid_wmm(pApInfo, tailpos);

#ifdef CONFIG_WPS
	if (hapd->conf->wps_state && hapd->wps_beacon_ie) {
		os_memcpy(tailpos, wpabuf_head(hapd->wps_beacon_ie),
			  wpabuf_len(hapd->wps_beacon_ie));
		tailpos += wpabuf_len(hapd->wps_beacon_ie);
	}
#endif /* CONFIG_WPS */

#ifdef CONFIG_P2P
	if ((hapd->conf->p2p & P2P_ENABLED) && hapd->p2p_beacon_ie) {
		os_memcpy(tailpos, wpabuf_head(hapd->p2p_beacon_ie),
			  wpabuf_len(hapd->p2p_beacon_ie));
		tailpos += wpabuf_len(hapd->p2p_beacon_ie);
	}
#endif /* CONFIG_P2P */
#ifdef CONFIG_P2P_MANAGER
	if ((hapd->conf->p2p & (P2P_MANAGE | P2P_ENABLED | P2P_GROUP_OWNER)) ==
	    P2P_MANAGE)
		tailpos = hostapd_eid_p2p_manage(hapd, tailpos);
#endif /* CONFIG_P2P_MANAGER */



#if (BEACON_DBG == 1)
//Add this element info for beacon release test	
#define WPA_PUT_BE24(a, val)					\
	do {							\
	(a)[0] = (u8) ((((u32) (val)) >> 16) & 0xff);	\
	(a)[1] = (u8) ((((u32) (val)) >> 8) & 0xff);	\
	(a)[2] = (u8) (((u32) (val)) & 0xff);		\
	} while (0)

#define OUI_WFA 0x506f9a

	*tailpos++ = WLAN_EID_VENDOR_SPECIFIC;
	*tailpos++ = 7;
	WPA_PUT_BE24(tailpos, OUI_WFA);
	tailpos += 3;

	/* Hotspot Configuration: DGAF Enabled */
	//u32 length
	*tailpos++ = 16;	
	*tailpos++ = 16;
	*tailpos++ = 16;
	*tailpos++ = 16;
#endif//#if (BEACON_DBG == 1)


 
	pApInfo->nBTailLen = tailpos > pApInfo->pBeaconTail ? tailpos - pApInfo->pBeaconTail : 0;

//----------------------------------------------------------------------



//short_slot_time, preamble, cts_protect, ht_opmode, 

#pragma message("===================================================")
#pragma message("Todo Set preamble, protection to HW")
#pragma message("===================================================")
	
	//GenBeacon();
#if(MLME_TASK ==1)
    if(post_to_mlme)
        ap_mlme_handler(NULL,MEVT_BCN_CMD);        
    else
        GenBeacon();
        
#else
    GenBeacon();
#endif


}

u8 *ieee80211_beacon_add_tim(ApInfo_st *pApInfo,
				    u8 *eid)
{
	u8 *pos, *tim;
	int aid0 = 0;
	int i, have_bits = 0, n1, n2;
 	u32 j;
	/* Generate bitmap for TIM only if there are any STAs in power save
	 * mode. */
	OS_MutexLock(pApInfo->ap_info_ps_mutex);
	if (pApInfo->num_sta_ps > 0)
		/* in the hope that this is faster than
		 * checking byte-for-byte */
		have_bits = !bitmap_empty((unsigned long*)pApInfo->tim,
					  IEEE80211_MAX_AID+1);
    OS_MutexUnLock(pApInfo->ap_info_ps_mutex);

	//if (pApInfo->dtim_count == 0)
	//	pApInfo->dtim_count = AP_DEFAULT_DTIM_PERIOD - 1;
	//else
	//	pApInfo->dtim_count--;

	tim = pos = eid;//= (u8 *) skb_put(skb, 6);

	*pos++ = WLAN_EID_TIM;
	*pos++ = 4;

	gDeviceInfo->APInfo->bcn_info.tim_cnt_oft = pos - pApInfo->pBeaconHead;

	*pos++ = pApInfo->dtim_count;
	*pos++ = AP_DEFAULT_DTIM_PERIOD;

	//if (pApInfo->dtim_count == 0 && !ap_tx_desp_queue_empty(&pApInfo->ps_bc_buf))
	//	aid0 = 1;

	//pApInfo->dtim_bc_mc = aid0 == 1;

	if (have_bits) {
		/* Find largest even number N1 so that bits numbered 1 through
		 * (N1 x 8) - 1 in the bitmap are 0 and number N2 so that bits
		 * (N2 + 1) x 8 through 2007 are 0. */
		n1 = 0;
		for (j = 0; j < IEEE80211_MAX_TIM_LEN; j++) {
			if (pApInfo->tim[j]) {
				n1 = j & 0xfe;
				break;
			}
		}
		n2 = n1;
		for (i = IEEE80211_MAX_TIM_LEN - 1; i >= n1; i--) {
			if (pApInfo->tim[i]) {
				n2 = i;
				break;
			}
		}

		/* Bitmap control */
		*pos++ = n1 | aid0;
		/* Part Virt Bitmap */
		//skb_put(skb, n2 - n1);
		MEMCPY(pos, pApInfo->tim + n1, (u32)n2 - n1 + 1);

		tim[1] = n2 - n1 + 4;							//length
		pos+=(n2-n1+1);
	} else {
		*pos++ = aid0; /* Bitmap control */
		#if(AP_MODE_BEACON_VIRT_BMAP_0XFF == 0)
		*pos++ = 0; /* Part Virt Bitmap */
		#else
		*pos++ = 0xff; /* Part Virt Bitmap */
		#endif
	}

	return pos;
}



//extern H_APIs s32 _ssv_wifi_send(void *data, s32 len, struct cfg_host_txreq *txreq, bool bAPFrame, u32 TxFlags);


extern s32 ssv6xxx_drv_send(void *dat, size_t len);


extern struct ap_tx_desp *
ap_get_buffered_bc(ApInfo_st *pAPInfo);




//u32 g_bcount = 0;


void GenBeacon()
{	
	
	u8 nBlen=0;		
	u8 *pos = ieee80211_beacon_add_tim(gDeviceInfo->APInfo, gDeviceInfo->APInfo->pBeaconTim);
	
	//Add beacon rail
	MEMCPY(pos, gDeviceInfo->APInfo->pBeaconTail, gDeviceInfo->APInfo->nBTailLen);
	nBlen = (pos+gDeviceInfo->APInfo->nBTailLen) - gDeviceInfo->APInfo->pBeaconHead;


	gDeviceInfo->APInfo->bcn_info.bcn_len = nBlen;
	ap_soc_set_bcn(SSV6XXX_SET_BEACON, gDeviceInfo->APInfo->bcn, &gDeviceInfo->APInfo->bcn_info, AP_DEFAULT_DTIM_PERIOD-1, AP_DEFAULT_BEACON_INT);


}








void StartBeacon(void)
{
	LOG_TRACE("%s Set Beacon and relative params to soc.\r\n",__func__);
	ieee802_11_set_beacon(gDeviceInfo->APInfo,FALSE);
}

void release_beacon_info(void)
{
    MEMSET(gDeviceInfo->APInfo->hw_bcn_info,0,sizeof(struct ssv6xxx_beacon_info)*bcn_cnt);
    gDeviceInfo->beacon_usage =0;
    gDeviceInfo->enable_beacon = FALSE;
	LOG_TRACE("%s Stop to send Beacon\r\n",__func__);
}


//------------------------------------------------------------------------------------------------


#if 0



{
	os_create_timer(AP_DEFAULT_BEACON_INT*10, timertest1, NULL, NULL);
	os_create_timer(AP_DEFAULT_BEACON_INT*20, timertest2, NULL, NULL);
	os_create_timer(AP_DEFAULT_BEACON_INT*30, timertest3, NULL, NULL);
	os_create_timer(AP_DEFAULT_BEACON_INT*40, timertest4, NULL, NULL);
	os_create_timer(AP_DEFAULT_BEACON_INT*50, timertest5, NULL, NULL);



	os_create_timer(AP_DEFAULT_BEACON_INT*60, timertest6, (void*)6, NULL);
	os_create_timer(AP_DEFAULT_BEACON_INT*70, timertest6, (void*)7, NULL);
	os_create_timer(AP_DEFAULT_BEACON_INT*80, timertest6, (void*)8, NULL);
	os_create_timer(AP_DEFAULT_BEACON_INT*90, timertest6, (void*)9, NULL);
}

void timertest1(void *data1, void *data2)
{	
	LOG_TRACE( "timertest1 %d MS \n", os_tick2ms(os_sys_jiffies()));
	os_create_timer(AP_DEFAULT_BEACON_INT*10, timertest1, NULL, NULL);	
}


void timertest2(void *data1, void *data2)
{	
	LOG_TRACE( "timertest2 %d MS \n", os_tick2ms(os_sys_jiffies()));
	os_create_timer(AP_DEFAULT_BEACON_INT*20, timertest2, NULL, NULL);	
}

void timertest3(void *data1, void *data2)
{	
	LOG_TRACE( "timertest3 %d MS \n", os_tick2ms(os_sys_jiffies()));
	os_create_timer(AP_DEFAULT_BEACON_INT*30, timertest3, NULL, NULL);	
}


void timertest4(void *data1, void *data2)
{	
	LOG_TRACE( "timertest4 %d MS \n", os_tick2ms(os_sys_jiffies()));
	os_create_timer(AP_DEFAULT_BEACON_INT*40, timertest4, NULL, NULL);	
}




void timertest5(void *data1, void *data2)
{	
	LOG_TRACE( "timertest5 %d MS \n", os_tick2ms(os_sys_jiffies()));
	os_create_timer(AP_DEFAULT_BEACON_INT*50, timertest5, NULL, NULL);	
}

void timertest6(void *data1, void *data2)
{	
	u32 itval = AP_DEFAULT_BEACON_INT* (u32)data1 * 10;
	LOG_TRACE( "timertest6 (u32)data1:%d--> %d MS \n",(u32)data1, os_tick2ms(os_sys_jiffies()));
	os_create_timer(itval, timertest6, data1, NULL);	
}

#endif
//#endif /* CONFIG_NATIVE_WINDOWS */
