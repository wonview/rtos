#ifndef _DEV_H_
#define _DEV_H_
#include "ap_info.h"
#include <core/mlme.h>

//#ifdef ARRAY_SIZE
#undef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
//#endif




typedef u32 tx_result;
#define TX_CONTINUE	((tx_result) 0u)
#define TX_DROP		((tx_result) 1u)
#define TX_QUEUED	((tx_result) 2u)


//Off->Init->Running->Pause->Off


enum TXHDR_ENCAP {
	TXREQ_ENCAP_ADDR4               =0,
	TXREQ_ENCAP_QOS                 ,
	TXREQ_ENCAP_HT                  ,
};


#define IS_TXREQ_WITH_ADDR4(x)          ((x)->txhdr_mask & (1<<TXREQ_ENCAP_ADDR4) )
#define IS_TXREQ_WITH_QOS(x)            ((x)->txhdr_mask & (1<<TXREQ_ENCAP_QOS)   )
#define IS_TXREQ_WITH_HT(x)             ((x)->txhdr_mask & (1<<TXREQ_ENCAP_HT)    )

#define SET_TXREQ_WITH_ADDR4(x, _tf)    (x)->txhdr_mask = (((x)->txhdr_mask & ~(1<<TXREQ_ENCAP_ADDR4))	| ((_tf)<<TXREQ_ENCAP_ADDR4) )
#define SET_TXREQ_WITH_QOS(x, _tf)      (x)->txhdr_mask = (((x)->txhdr_mask & ~(1<<TXREQ_ENCAP_QOS))	| ((_tf)<<TXREQ_ENCAP_QOS)   )
#define SET_TXREQ_WITH_HT(x, _tf)       (x)->txhdr_mask = (((x)->txhdr_mask & ~(1<<TXREQ_ENCAP_HT))		| ((_tf)<<TXREQ_ENCAP_HT)    )



typedef enum txreq_type_en {
	TX_TYPE_M0 = 0,
	TX_TYPE_M1,
	TX_TYPE_M2,
} txreq_type;




typedef enum{
	SSV6XXX_HWM_STA		,
        SSV6XXX_HWM_SCONFIG ,
	SSV6XXX_HWM_AP		,
	SSV6XXX_HWM_IBSS	,
	SSV6XXX_HWM_WDS	    ,
	SSV6XXX_HWM_INVALID	,
}ssv6xxx_hw_mode;


#define HOST_DATA_CB_NUM  2
#define HOST_EVT_CB_NUM  2


typedef enum {
	SSV6XXX_CB_ADD		,
	SSV6XXX_CB_REMOVE	,
	SSV6XXX_CB_MOD		,
} ssv6xxx_cb_action;


typedef struct DeviceInfo{

    OsMutex g_dev_info_mutex;
	ssv6xxx_hw_mode hw_mode;
	txreq_type tx_type;
	u32 txhdr_mask;
    u8 self_mac[ETH_ALEN];
	ETHER_ADDR addr4;
	u16 qos_ctrl;
	u32 ht_ctrl;

    //Set key base on VIF.
    u32 hw_buf_ptr;
    u32 hw_sec_key;
    u32 hw_pinfo;
    u8 beacon_usage;        //DRV_BCN_BCN0 bit 0,DRV_BCN_BCN1 bit 1
    bool enable_beacon;
    struct ApInfo *APInfo; // AP mode used
    struct APStaInfo *StaConInfo; // AP mode used
    struct cfg_join_request *joincfg; // Station mode used
    enum conn_status    status;                 //Auth,Assoc,Eapol
    struct ssv6xxx_ieee80211_bss ap_list[NUM_AP_INFO]; //station ap info list used

//-----------------------------
//Data path handler
//-----------------------------
	data_handler data_cb[HOST_DATA_CB_NUM];

//-----------------------------
//Event path handler
//-----------------------------
	evt_handler evt_cb[HOST_EVT_CB_NUM];

}DeviceInfo_st;

extern DeviceInfo_st *gDeviceInfo;


#endif /* _HOST_GLOBAL_ */

