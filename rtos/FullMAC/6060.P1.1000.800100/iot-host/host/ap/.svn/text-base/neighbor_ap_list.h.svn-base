/*
 * hostapd / AP table
 * Copyright (c) 2002-2003, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2003-2004, Instant802 Networks, Inc.
 * Copyright (c) 2006, Devicescape Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#ifndef AP_LIST_H
#define AP_LIST_H

#include <types.h>
#include "ap_config.h"
#include <os_wrapper.h>
#include "ap_def.h"
struct neighbor_ap_info {
	
	u8 addr[6];
	u16 beacon_int;
	u16 capability;
	u8 supported_rates[AP_SUPP_RATES_MAX];
	u8 ssid[AP_MAX_SSID_LEN];
	size_t ssid_len;
	int wpa;
	int erp; /* ERP Info or -1 if ERP info element not present */

	int channel;
	//int datarate; /* in 100 kbps */
	//int ssi_signal;

	int ht_support;

	u32 num_beacons; /* number of beacon frames received */
	os_time_t last_beacon;

	//int already_seen; /* whether API call AP-NEW has already fetched
	//		   * information about this AP */
};

struct ieee802_11_elems;
//struct hostapd_frame_info;
struct ApInfo;
struct ieee80211_mgmt;


//struct neighbor_ap_info * ap_get_ap(ApInfo_st *pApInfo, const u8 *sta);


void neighbor_ap_list_process_beacon(struct ApInfo *pApInfo,
			    const struct ieee80211_mgmt *mgmt,
			    struct ieee802_11_elems *elems);

int neighbor_ap_list_init(struct ApInfo *pApInfo);
void neighbor_ap_list_deinit(struct ApInfo *pApInfo);






#endif /* AP_LIST_H */
