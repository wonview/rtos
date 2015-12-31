#ifndef _COMMON_H_
#define _COMMON_H_


#include <types.h>
#include "porting.h"

#ifndef EAPOL_ETHER_TYPPE
#define EAPOL_ETHER_TYPPE	0x888E
#endif

typedef enum{
	SSV6XXX_SEC_NONE,
	SSV6XXX_SEC_WEP_40,			//5		ASCII
	SSV6XXX_SEC_WEP_104,		//13	ASCII
	SSV6XXX_SEC_WPA_PSK,		//8~63	ASCII
	SSV6XXX_SEC_WPA2_PSK,		//8~63	ASCII
	SSV6XXX_SEC_WPS,
	SSV6XXX_SEC_MAX,
}ssv6xxx_sec_type;

#define MAX_SSID_LEN 32
#define MAX_PASSWD_LEN 63

#define MAX_WEP_PASSWD_LEN (13+1)

#define SECURITY_KEY_LEN              (32)

STRUCT_PACKED struct ssv6xxx_hw_key {
    u8          key[SECURITY_KEY_LEN];
	u32			tx_pn_l;
    u32         tx_pn_h;
	u32        	rx_pn_l;
    u32         rx_pn_h;
};

STRUCT_PACKED struct ssv6xxx_hw_sta_key {
	u8         	pair_key_idx:4;		/* 0: pairwise key, 1-3: group key */
	u8         	group_key_idx:4;	/* 0: pairwise key, 1-3: group key */

    u8          valid;              /* 0: invalid entry, 1: valid entry asic hw don't check this field*/
	u8			reserve[2];

	struct ssv6xxx_hw_key	pair;
};

STRUCT_PACKED struct ssv6xxx_hw_sec {
    struct ssv6xxx_hw_key group_key[3];	//mapping to protocol group key 1-3
    struct ssv6xxx_hw_sta_key sta_key[8];
};

#define PHY_INFO_TBL1_SIZE          39
#define PHY_INFO_TBL2_SIZE          39
#define PHY_INFO_TBL3_SIZE          8



//------------------------------------------------

/**
 *  struct cfg_sta_info - STA structure description
 *
*/
#if 0
struct cfg_sta_info {
    ETHER_ADDR      addr;
#if 1
		u32 			bit_rates; /* The first eight rates are the basic rate set */

		u8				listen_interval;

		u8				key_id;
		u8				key[16];
		u8				mic_key[8];

		/* TKIP IV */
		u16 			iv16;
		u32 			iv32;
#endif

} ;//__attribute__ ((packed));

#endif



/**
 *  struct cfg_bss_info - BSS/IBSS structure description
 *
 */
#if 0
struct cfg_bss_info {
    ETHER_ADDR          bssid;

};// __attribute__ ((packed));

#endif
PACK(struct ETHER_ADDR_st
{
    u8      addr[ETHER_ADDR_LEN];
})
typedef struct ETHER_ADDR_st        ETHER_ADDR;

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

#define DIV_ROUND_UP(n, d)	(((n) + (d) - 1) / (d))

typedef enum
{
    MT_STOP,
    MT_RUNNING,
    MT_EXIT,
}  ModeType; //CmdEngine/TXRX_Task mode type


#endif /* _COMMON_H_ */

