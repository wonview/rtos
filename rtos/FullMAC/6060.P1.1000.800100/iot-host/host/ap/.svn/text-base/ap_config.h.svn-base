#ifndef _AP_CONFIG_H_
#define _AP_CONFIG_H_

#include <types.h>
#include "common/defs.h"
#include "common/ieee80211.h"
#include "hw_config.h"


#define AP_SUPPORT_80211N	1


typedef enum {
	LONG_PREAMBLE = 0,
	SHORT_PREAMBLE = 1
} ePreamble;




//typedef unsigned int     size_t;








	
#define AP_DEFAULT_SSID		"SSV"



//-----------------------------------
#define AP_DEFAULT_SNED_PROBE_RESPONSE  1
#define AP_DEFAULT_IGNORE_BROADCAST_SSID  0


//--------------------------------------------------------
//Item could be customized
#define AP_DEFAULT_PREAMBLE		SHORT_PREAMBLE


#define AP_DEFAULT_HW_MODE		AP_MODE_IEEE80211G
#define AP_DEFAULT_CHANNEL		6
//#define AP_DEFAULT_COUNTRY      "GB "   hard code

#if AP_SUPPORT_80211N
#define AP_DEFAULT_80211N		FALSE
#define AP_DEFAULT_REQUIRE_HT	TRUE
#endif

//-------------------------------------------------------



struct ssv6xxx_host_ap_config {


	char ssid[IEEE80211_MAX_SSID_LEN + 1];
	size_t ssid_len;

	u8 eApMode;
#if AP_SUPPORT_80211N
	bool b80211n;
	bool bRequire_ht;
#endif
	u8  nChannel;
	
	ePreamble preamble;

// 	char country[3]; /* first two octets: country code as described in
// 			  * ISO/IEC 3166-1. Third octet:
// 			  * ' ' (ascii 32): all environments			 
// 			  */
	u8  nNumrates;
	struct ieee80211_rate **pRates;
};

void ssv6xxx_config_init(struct ssv6xxx_host_ap_config *pConfig);















#endif /* _AP_CONFIG_H_ */
