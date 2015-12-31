/*
 * hostapd / IEEE 802.11 Management: Beacon and Probe Request/Response
 * Copyright (c) 2002-2004, Instant802 Networks, Inc.
 * Copyright (c) 2005-2006, Devicescape Software, Inc.
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

#ifndef BEACON_H
#define BEACON_H

#include "ap_info.h"

struct ieee80211_mgmt;

void ieee802_11_set_beacon(ApInfo_st *pApInfo, bool post_to_mlme);

u8 * hostapd_eid_ds_params(ApInfo_st *pApInfo, u8 *eid);
u8 * hostapd_eid_country(ApInfo_st *pApInfo, u8 *eid,int max_len);
u8 * hostapd_eid_erp_info(ApInfo_st *pApInfo, u8 *eid);
u8 * hostapd_eid_wpa(ApInfo_st *pApInfo, u8 *eid, size_t len);
#endif /* BEACON_H */
