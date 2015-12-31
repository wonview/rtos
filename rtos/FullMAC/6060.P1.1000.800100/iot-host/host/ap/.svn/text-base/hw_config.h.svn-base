
#ifndef _HW_CONFIG_H_
#define _HW_CONFIG_H_


#include <types.h>
#include "common/bitops.h"

/**
 * enum ieee80211_rate_flags - rate flags
 *
 * Hardware/specification flags for rates. These are structured
 * in a way that allows using the same bitrate structure for
 * different bands/PHY modes.
 *
 * @IEEE80211_RATE_SHORT_PREAMBLE: Hardware can send with short
 *	preamble on this bitrate; only relevant in 2.4GHz band and
 *	with CCK rates.
 * @IEEE80211_RATE_MANDATORY_A: This bitrate is a mandatory rate
 *	when used with 802.11a (on the 5 GHz band); filled by the
 *	core code when registering the wiphy.
 * @IEEE80211_RATE_MANDATORY_B: This bitrate is a mandatory rate
 *	when used with 802.11b (on the 2.4 GHz band); filled by the
 *	core code when registering the wiphy.
 * @IEEE80211_RATE_MANDATORY_G: This bitrate is a mandatory rate
 *	when used with 802.11g (on the 2.4 GHz band); filled by the
 *	core code when registering the wiphy.
 * @IEEE80211_RATE_ERP_G: This is an ERP rate in 802.11g mode.
 */
enum ieee80211_rate_flags {
	IEEE80211_RATE_FLAGS_NONE				= 0,
	IEEE80211_RATE_FLAGS_MANDATORY_A		= 1<<1, //----------------------------------------------------->AP_MODE_TO_HW_RATE_FLAGS_MANDATORY_SHIFT
	IEEE80211_RATE_FLAGS_MANDATORY_B		= 1<<2,
	IEEE80211_RATE_FLAGS_MANDATORY_G		= 1<<3,
	IEEE80211_RATE_FLAGS_SUPPORT_A		= 1<<4,		//------------------------------------------------------>AP_MODE_TO_HW_RATE_FLAGS_SUPPORT_SHIFT
	IEEE80211_RATE_FLAGS_SUPPORT_B		= 1<<5,
	IEEE80211_RATE_FLAGS_SUPPORT_G		= 1<<6,
	IEEE80211_RATE_FLAGS_SHORT_PREAMBLE	= 1<<7,
	IEEE80211_RATE_FLAGS_ERP_G			= 1<<8,
};


enum ieee80211_rate_num {
	IEEE80211_RATE_NUM_1M 	= 2,
	IEEE80211_RATE_NUM_2M 	= 4,
	IEEE80211_RATE_NUM_5_5M = 11,
	IEEE80211_RATE_NUM_6M 	= 12,
	IEEE80211_RATE_NUM_9M	= 18,
	IEEE80211_RATE_NUM_11M	= 22,
	IEEE80211_RATE_NUM_12M	= 24,
	IEEE80211_RATE_NUM_18M 	= 36,
	IEEE80211_RATE_NUM_22M	= 44,
	IEEE80211_RATE_NUM_24M	= 48,
	IEEE80211_RATE_NUM_33M	= 66,
	IEEE80211_RATE_NUM_36M	= 72,
	IEEE80211_RATE_NUM_48M	= 96,
	IEEE80211_RATE_NUM_54M	= 108,
};

#define AP_MODE_TO_HW_RATE_FLAGS_MANDATORY_SHIFT 0
#define AP_MODE_TO_HW_RATE_FLAGS_SUPPORT_SHIFT 4

//extern const struct ieee80211_rate hw_rates[];


#ifdef _MSC_VER
#pragma pack(push, 1)
#endif /* _MSC_VER */

STRUCT_PACKED struct ieee80211_rate {	
	u16 bitrate;
	u16 flags;
	//u16 hw_value, hw_value_short;
};











#ifdef _MSC_VER
#pragma pack(pop)
#endif /* _MSC_VER */

#endif /* _HW_CONFIG_H_ */
