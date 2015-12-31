#define __SFILE__ "ap_config.c"

#include <os_wrapper.h>
#include "ap_config.h"
#include "hw_config.h"


//11a		6¡B9¡B12¡B18¡B24¡B36¡B48 ,54 						Basic 6,12,24
//11b only	1, 2, 5.5, 11										Basic 1 (default), 2
//11bg		1, 2, 5.5, 11, 6, 9, 12, 18, 24, 36, 48, 54			Basic 1, 2 (default), 5.5, 11

struct ieee80211_rate hw_rates[] = {	
	{	IEEE80211_RATE_NUM_1M,
	IEEE80211_RATE_FLAGS_MANDATORY_B|IEEE80211_RATE_FLAGS_MANDATORY_G|IEEE80211_RATE_FLAGS_SUPPORT_B|IEEE80211_RATE_FLAGS_SUPPORT_G
	},
//#ifdef __SSV_GN__	
	{	IEEE80211_RATE_NUM_2M,
	IEEE80211_RATE_FLAGS_SHORT_PREAMBLE|IEEE80211_RATE_FLAGS_MANDATORY_B|IEEE80211_RATE_FLAGS_MANDATORY_G|IEEE80211_RATE_FLAGS_SUPPORT_B|IEEE80211_RATE_FLAGS_SUPPORT_G
	},
	
	{	IEEE80211_RATE_NUM_5_5M,
	IEEE80211_RATE_FLAGS_SHORT_PREAMBLE|IEEE80211_RATE_FLAGS_MANDATORY_G|IEEE80211_RATE_FLAGS_SUPPORT_B|IEEE80211_RATE_FLAGS_SUPPORT_G
	},	
	{	IEEE80211_RATE_NUM_11M,
	IEEE80211_RATE_FLAGS_SHORT_PREAMBLE|IEEE80211_RATE_FLAGS_MANDATORY_G|IEEE80211_RATE_FLAGS_SUPPORT_B|IEEE80211_RATE_FLAGS_SUPPORT_G
	},	
	{	IEEE80211_RATE_NUM_6M,
	IEEE80211_RATE_FLAGS_MANDATORY_A|IEEE80211_RATE_FLAGS_SUPPORT_A|IEEE80211_RATE_FLAGS_SUPPORT_G
	},	
	{	IEEE80211_RATE_NUM_9M,
	IEEE80211_RATE_FLAGS_NONE|IEEE80211_RATE_FLAGS_SUPPORT_A|IEEE80211_RATE_FLAGS_SUPPORT_G
	},
	{	IEEE80211_RATE_NUM_12M,
	IEEE80211_RATE_FLAGS_MANDATORY_A|IEEE80211_RATE_FLAGS_SUPPORT_A|IEEE80211_RATE_FLAGS_SUPPORT_G
	},
	{	IEEE80211_RATE_NUM_18M,
	IEEE80211_RATE_FLAGS_NONE|IEEE80211_RATE_FLAGS_SUPPORT_A|IEEE80211_RATE_FLAGS_SUPPORT_G
	},
	{	IEEE80211_RATE_NUM_24M,
	IEEE80211_RATE_FLAGS_MANDATORY_A|IEEE80211_RATE_FLAGS_SUPPORT_A|IEEE80211_RATE_FLAGS_SUPPORT_G
	},
	{	IEEE80211_RATE_NUM_36M,
	IEEE80211_RATE_FLAGS_NONE|IEEE80211_RATE_FLAGS_SUPPORT_A|IEEE80211_RATE_FLAGS_SUPPORT_G
	},
	{	IEEE80211_RATE_NUM_48M,
	IEEE80211_RATE_FLAGS_NONE|IEEE80211_RATE_FLAGS_SUPPORT_A|IEEE80211_RATE_FLAGS_SUPPORT_G
	},
	{	IEEE80211_RATE_NUM_54M,
	IEEE80211_RATE_FLAGS_NONE|IEEE80211_RATE_FLAGS_SUPPORT_A|IEEE80211_RATE_FLAGS_SUPPORT_G
	},
//#endif	
};









void ssv6xxx_config_init_rates(struct ssv6xxx_host_ap_config *pConfig)
{
	u32 i, num=0,count=0;

	for (i=0;i<(sizeof(hw_rates)/sizeof(hw_rates[0]));i++)
	{
		if ((hw_rates[i].flags & ( 1 << (pConfig->eApMode+AP_MODE_TO_HW_RATE_FLAGS_SUPPORT_SHIFT) )) !=0  )
		{
			num++;
		}
	}

	pConfig->nNumrates = (u8)num;
	pConfig->pRates = MALLOC(sizeof(u32)*pConfig->nNumrates);

	for (i=0;i<(sizeof(hw_rates)/sizeof(hw_rates[0]));i++)
	{
		if ((hw_rates[i].flags & ( 1 << (pConfig->eApMode+AP_MODE_TO_HW_RATE_FLAGS_SUPPORT_SHIFT) )) !=0  )
		{
			pConfig->pRates[count++] = &hw_rates[i];
		}
	}
}











void ssv6xxx_config_init(struct ssv6xxx_host_ap_config *pConfig)
{

	pConfig->ssid_len = strlen(AP_DEFAULT_SSID);
	MEMCPY(pConfig->ssid, AP_DEFAULT_SSID, pConfig->ssid_len);

	//ap_config.country =
	//MEMCPY(&pConfig->country, AP_DEFAULT_COUNTRY, 3);
	
	pConfig->preamble = AP_DEFAULT_PREAMBLE;

	pConfig->eApMode =			AP_DEFAULT_HW_MODE;
	
	pConfig->nChannel =			AP_DEFAULT_CHANNEL; 
#if AP_SUPPORT_80211N
	pConfig->b80211n =					AP_DEFAULT_80211N;
	pConfig->bRequire_ht =				AP_DEFAULT_REQUIRE_HT;
#endif

	ssv6xxx_config_init_rates(pConfig);
}




struct ap_wmm_ac_params {
	u32 aifsn:4;
	u32 acm:1;
	u32 aci:2;	
	u32 reserved:1;

	u32 ecwin:4;
	u32 ecwax:4;

	u32 txop_limit:16;
};

#if AP_DEFAULT_HW_MODE > AP_MODE_IEEE80211B

//g/n/a
const struct ap_wmm_ac_params ac_params[4]=
{
	//# Normal priority / AC_BE = best effort	
	{3, 0, 0, 0, 4, 10, 0},
	//# Low priority / AC_BK = background
	{7, 0, 1, 0, 4, 10, 0},
	//# High priority / AC_VI = video	
	{2, 0, 2, 0, 3, 4, 94},	
	//# Highest priority / AC_VO = voice	
	{2, 0, 3, 0, 2, 3, 47},
};

#else

//b
const struct ap_wmm_ac_params ac_params[4]=
{	
	//# Normal priority / AC_BE = best effort
	{3, 0, 0, 0, 5, 10, 0},	
	//# Low priority / AC_BK = background
	{7, 0, 1, 0, 5, 10, 0},
	//# High priority / AC_VI = video	
	{2, 0, 2, 0, 4, 5, 188},
	//# Highest priority / AC_VO = voice	
	{2, 0, 3, 0, 3, 4, 102},
};

#endif
