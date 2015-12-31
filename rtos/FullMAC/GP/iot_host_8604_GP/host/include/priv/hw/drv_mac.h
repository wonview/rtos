#ifndef _DRV_MAC_H_
#define _DRV_MAC_H_

//#include <regs.h>
//#include <hw_regs_api.h>


/**
* enum switch_ops_en (sw_ops) - define enable/disable value.
*
* @ SW_DISABLE:



/**
* STA Infomation Definition:
* 
* @ STA_INFO_MODE()
* @ STA_INFO_HT()
* @ STA_INFO_QOS()
*/
#define STA_INFO_MODE(x)            ((x)&(3<<0))
#define STA_INFO_HT(x)              ((x)&(3<<2))
#define STA_INFO_QOS(x)             ((x)&(1<<4))



/**
* Define flags for WSID INFO field:
*
* @ WSID_INFO_VALID
*/
#define SET_WSID_INFO_VALID(s, v)       do {(s)->info = (((s)->info & ~(1<<0)) | ((v)<<0)); } while (0)
#define SET_WSID_INFO_QOS_EN(s, v)      do {(s)->info = (((s)->info & ~(1<<1)) | ((v)<<1)); } while (0)
#define SET_WSID_INFO_OP_MODE(s, v)     do {(s)->info = (((s)->info & ~(3<<2)) | ((v)<<2)); } while (0)
#define SET_WSID_INFO_HT_MODE(s, v)     do {(s)->info = (((s)->info & ~(3<<4)) | ((v)<<4)); } while (0)

#define GET_WSID_INFO_VALID(s)          ((s)->info&0x01)
#define GET_WSID_INFO_QOS_EN(s)         (((s)->info >>1) & 0x01)
#define GET_WSID_INFO_OP_MODE(s)        (((s)->info >>2) & 0x03)
#define GET_WSID_INFO_HT_MODE(s)        (((s)->info >>3) & 0x03)

/*
/**
* struct mac_wsid_tabl_st - define wsid (wiress session ID) entry.
*
* @ 
*/
typedef struct mac_wsid_entry_st {
    u32             info;
    ETHER_ADDR      sta_mac;
    u16             tx_ack_policy;
    u16             tx_seq_ctrl[8];
    u16             rx_seq_ctrl[8];
    
} mwsid_entry;

/**
* struct gmfilter_tbl_st - define group address filter entry
*
* @
*/
typedef struct gmfilter_st {
    u32             ctrl; /* 0/1: disable, 2: mask mode, 3: single mode */
    ETHER_ADDR      mac;
    ETHER_ADDR      msk;
    
} gmfilter;



#endif /* _DRV_MAC_H_ */


