#ifndef _HOST_APIS_H_
#define _HOST_APIS_H_
#include <porting.h>

#undef PRINTF_FORMAT


#ifdef _MSC_VER
#pragma pack(push, 1)
#endif /* _MSC_VER */

#include <ssv_ex_lib.h>
#include <common.h>
#include <cmd_def.h>
#include "ap_sta_info.h"

//#include "os_wrapper.h"
#define CFG_HOST_TXREQ0(host_txreq0,len,c_type,f80211,qos,ht,use_4addr,RSVD0,padding,bc_queue,security,more_data,sub_type,extra_info) do{\
    u32 *temp;                                       \
    temp = (u32*)host_txreq0 ;                       \
    *temp = (len<<0) |                              \
            (c_type<<16)|                           \
            (f80211<<19)|                           \
            (qos<<20)|                              \
            (ht<<21)|                               \
            (use_4addr<<22)|                        \
            (RSVD0<<23)|                            \
            (padding<<24)|                          \
            (bc_queue<<26)|                         \
            (security<<27)|                         \
            (more_data<<28)|                        \
            (sub_type<<29)|                         \
            (extra_info<<31);                        \
            }while(0)


#define SSV6XXX_APLIST_PAGE_COUNT  10

enum TX_DSCRP_CONF_TYPE {
	TX_DSCRP_SET_EXTRA_INFO    = BIT(0),
	TX_DSCRP_SET_BC_QUE        = BIT(1),
};


//#define IS_TX_DSCRP_SET_EXTRA_INFO(x)        ((x) & (1<<TX_DSCRP_SET_EXTRA_INFO) )
//#define IS_TX_DSCRP_SET_BC_QUE(x)            ((x) & (1<<TX_DSCRP_SET_BC_QUE)   )


//#define SET_TX_DSCRP_SET_EXTRA_INFO(x, _tf)     (x) = (((x) & ~(1<<TX_DSCRP_SET_EXTRA_INFO))	| ((_tf)<<TX_DSCRP_SET_EXTRA_INFO) )
//#define SET_TX_DSCRP_SET_BC_QUE(x, _tf)      	(x) = (((x) & ~(1<<TX_DSCRP_SET_BC_QUE))	| ((_tf)<<TX_DSCRP_SET_BC_QUE)   )







/**
 * Define the error numbers of wifi host api:
 *
 * @ SSV6XXX_SUCCESS:
 * @ SSV6XXX_FAILED:
 */

typedef enum{
	SSV6XXX_SUCCESS                      =     0,
	SSV6XXX_FAILED                       =    -1,
	SSV6XXX_INVA_PARAM                   =    -2,
	SSV6XXX_NO_MEM                       =    -3,
	SSV6XXX_QUEUE_FULL					 =	  -4,
	SSV6XXX_WRONG_HW_MODE_CMD			 =	  -5,

}ssv6xxx_result;


#define AP_PS_FRAME 			BIT(0)		//This frame was buffered by AP.
#define SSV62XX_TX_MAX_RATES    3
STRUCT_PACKED struct fw_rc_retry_params {
    u32 count:4;
    u32 drate:6;
    u32 crate:6;
    u32 rts_cts_nav:16;
    u32 frame_consume_time:10;
    u32 dl_length:12;
    u32 RSVD:10;
};

/**
* struct ssv6200_tx_desc - ssv6200 tx frame descriptor.
* This descriptor is shared with ssv6200 hardware and driver.
*/
struct ssv6200_tx_desc
{
    /* The definition of WORD_1: */
    u32             len:16;
    u32             c_type:3;
    u32             f80211:1;
    u32             qos:1;          /* 0: without qos control field, 1: with qos control field */
    u32             ht:1;           /* 0: without ht control field, 1: with ht control field */
    u32             use_4addr:1;
    u32             RSVD_0:3;//used for rate control report event.
    u32             bc_que:1;
    u32             security:1;
    u32             more_data:1;
    u32             stype_b5b4:2;
    u32             extra_info:1;   /* 0: don't trap to cpu after parsing, 1: trap to cpu after parsing */

    /* The definition of WORD_2: */
    u32             fCmd;

    /* The definition of WORD_3: */
    u32             hdr_offset:8;
    u32             frag:1;
    u32             unicast:1;
    u32             hdr_len:6;
    u32             tx_report:1;
    u32             tx_burst:1;     /* 0: normal, 1: burst tx */
    u32             ack_policy:2;   /* See Table 8-6, IEEE 802.11 Spec. 2012 */
    u32             aggregation:1;
    u32             RSVD_1:3;//Used for AMPDU retry counter
    u32             do_rts_cts:2;   /* 0: no RTS/CTS, 1: need RTS/CTS */
                                    /* 2: CTS protection, 3: RSVD */
    u32             reason:6;

    /* The definition of WORD_4: */
    u32             payload_offset:8;
    u32             RSVD_4:7;
    u32             RSVD_2:1;
    u32             fCmdIdx:3;
    u32             wsid:4;
    u32             txq_idx:3;
    u32             TxF_ID:6;

    /* The definition of WORD_5: */
    u32             rts_cts_nav:16;
    u32             frame_consume_time:10;  //32 units
    u32             crate_idx:6;

    /* The definition of WORD_6: */
    u32             drate_idx:6;
    u32             dl_length:12;
    u32             RSVD_3:14;
    /* The definition of WORD_7~15: */
    u32             RESERVED[8];
    /* The definition of WORD_16~20: */
    struct fw_rc_retry_params rc_params[SSV62XX_TX_MAX_RATES];
};


/**
 * struct cfg_host_txreq0 - Host frame transmission Request  Header
 *
 * TX-REQ0 uses 4-byte header to carry host message to wifi-controller.
 * The first two-byte is the length indicating the whole message length (
 * including 2-byte header length).
 */
 struct cfg_host_txreq0 {
    u32               len:16;
    u32               c_type:3;
    u32               f80211:1;
    u32               qos:1;
    u32               ht:1;
    u32               use_4addr:1;
    u32               RSVD0:1;			//AP mode use one bit to know if this packet is buffered frame(Power saving) AP_PS_FRAME
    u32               padding:2;
    u32				  bc_queue:1;
    u32               security:1;
    u32               more_data:1;
    u32               sub_type:2;
    u32               extra_info:1;
 }STRUCT_PACKED;

#if 0
/**
 *  struct cfg_host_txreq1 - Host frame transmission Request  Header
 */
struct cfg_host_txreq1 {
    u32               len:16;
    u32               c_type:3;
    u32               f80211:1;
    u32               qos:1;
    u32               ht:1;
    u32               use_4addr:1;
    u32               RSVD0:4;
	u32               security:1;
    u32               more_data:1;
    u32               sub_type:2;
    u32               extra_info:1;

    u32               f_cmd;
} STRUCT_PACKED;


/**
 *  struct cfg_host_txreq2 - Host frame transmission Request  Header
 */
struct cfg_host_txreq2 {
#pragma message("===================================================")
#pragma message("     cfg_host_txreq2 not implement yet")
#pragma message("===================================================")
    u32               len:16;
    u32               c_type:3;
    u32               f80211:1;
    u32               qos:1;
    u32               ht:1;
    u32               use_4addr:1;
    u32               RSVD0:4;
	u32               security:1;
    u32               more_data:1;
    u32               sub_type:2;
    u32               extra_info:1;

    u32               f_cmd;
    u32               AAA;
}; //__attribute__((packed))

 /**
 *  struct cfg_host_txreq - Host frame transmission Request Parameters
 */
struct cfg_host_txreq {
	struct cfg_host_txreq0 txreq0;
        u16           qos;
        u32           ht;
        u8            addr4[ETHER_ADDR_LEN];
	u8            priority;
}; //__attribute__((packed));


#endif






//---------------------------------------------------------
//private







/************************************************************************************************/
/*                                 SSV  WIFI Command function							 		*/
/************************************************************************************************/



H_APIs s32 ssv6xxx_wifi_scan(struct cfg_scan_request *csreq);
H_APIs s32 ssv6xxx_wifi_join(struct cfg_join_request *cjreq);
H_APIs s32 ssv6xxx_wifi_leave(struct cfg_leave_request *clreq);
H_APIs s32 ssv6xxx_wifi_sconfig(struct cfg_sconfig_request *csreq);

//----------------------------I/O Ctrl-----------------------------

//id = ssv6xxx_host_cmd_id
H_APIs s32 ssv6xxx_wifi_ioctl(u32 cmd_id, void *data, u32 len);


//H_APIs s32 ssv6xxx_wifi_ioctl(struct cfg_ioctl_request *cireq);





/************************************************************************************************/
/*                                  Data related function																				*/
/************************************************************************************************/





enum ssv6xxx_data_priority {
	ssv6xxx_data_priority_0,
	ssv6xxx_data_priority_1,
	ssv6xxx_data_priority_2,
	ssv6xxx_data_priority_3,
	ssv6xxx_data_priority_4,
	ssv6xxx_data_priority_5,
	ssv6xxx_data_priority_6,
	ssv6xxx_data_priority_7,
};




//------------------------------------Send-------------------------------------


H_APIs s32 ssv6xxx_wifi_send_ethernet(void *frame, s32 len, enum ssv6xxx_data_priority priority);
//H_APIs s32 ssv6xxx_wifi_send_80211(void *frame, s32 len);



//H_APIs s32 ssv_wifi_send(void *data, s32 len, struct cfg_host_txreq *txreq);
//time to remove
//H_APIs s32 ssv_wifi_send(void *data, s32 len, struct cfg_host_txreq *txreq);


//------------------------------------Receive-------------------------------------

// struct cfg_host_rxpkt {
//
// 	 /* The definition of WORD_1: */
// 	u32             len:16;
// 	u32             c_type:3;
//     u32             f80211:1;
// 	u32             qos:1;          /* 0: without qos control field, 1: with qos control field */
//     u32             ht:1;           /* 0: without ht control field, 1: with ht control field */
//     u32             use_4addr:1;
// 	u32             l3cs_err:1;
//     u32             l4cs_err:1;
//     u32             align2:1;
//     u32             RSVD_0:2;
// 	u32             psm:1;
//     u32             stype_b5b4:2;
//     u32             extra_info:1;
//
//     /* The definition of WORD_2: */
//     u32             fCmd;
//
//     /* The definition of WORD_3: */
//     u32             hdr_offset:8;
//     u32             frag:1;
//     u32             unicast:1;
//     u32             hdr_len:6;
//     u32             RxResult:8;
//     u32             wildcard_bssid:1;
//     u32             RSVD_1:1;
//     u32             reason:6;
//
//     /* The definition of WORD_4: */
//     u32             payload_offset:8;
//     u32             next_frag_pid:7;
//     u32             RSVD_2:1;
//     u32             fCmdIdx:3;
//     u32             wsid:4;
// 	u32				RSVD_3:3;
// 	u32				drate_idx:6;
//
// 	/* The definition of WORD_5: */
// 	u32 			seq_num:16;
// 	u32				tid:16;
//
// 	/* The definition of WORD_6: */
// 	u32				pkt_type:8;
// 	u32				RCPI:8;
// 	u32				SNR:8;
// 	u32				RSVD:8;
//
//
//
// };
//


typedef enum{
	SSV6XXX_DATA_ACPT		= 0,	//Accept
	SSV6XXX_DATA_CONT		= 1,	//Pass data
	SSV6XXX_DATA_QUEUED		= 2,	//Data Queued
}ssv6xxx_data_result;




typedef ssv6xxx_data_result (*data_handler)(void *data, u32 len);
typedef void (*evt_handler)(u32 nEvtId, void *data, s32 len);


//Register handler to get RX data
H_APIs ssv6xxx_result ssv6xxx_wifi_reg_rx_cb(data_handler handler);
//Register handler to get event
H_APIs ssv6xxx_result ssv6xxx_wifi_reg_evt_cb(evt_handler evtcb);


//UnRegister Rx data handler
H_APIs ssv6xxx_result ssv6xxx_wifi_unreg_rx_cb(data_handler handler);

//UnRegister Rx event handler
H_APIs ssv6xxx_result ssv6xxx_wifi_unreg_evt_cb(evt_handler evtcb);


H_APIs void ssv6xxx_wifi_clear_security (void);
H_APIs void ssv6xxx_wifi_apply_security (void);

H_APIs s32 ssv6xxx_wifi_send_addba_resp(struct cfg_addba_resp *addba_resp);
H_APIs s32 ssv6xxx_wifi_send_delba(struct cfg_delba *delba);


#if 0
//Data to soc
#if 1
H_APIs void* ssv_wifi_allocate(u32 size);
#else
H_APIs void* ssv_wifi_allocate(enum ssv_ts_type type,u32 size);
H_APIs struct cfg_host_txreq0* ssv_wifi_get_tx_req(void *data);
#endif
//H_APIs s32 ssv_wifi_free(void *data);
#endif

#if (CONFIG_HOST_PLATFORM == 1 )
H_APIs void ssv6xxx_wifi_apply_security_SIM (u8 bValue);
#endif


H_APIs s32 ssv6xxx_wifi_ioctl(u32 cmd_id, void *data, u32 len);
H_APIs s32 ssv6xxx_wifi_ioctl_Ext(u32 cmd_id, void *data, u32 len,bool blocking);
H_APIs ssv6xxx_result ssv6xxx_wifi_ap(struct ap_setting *ap_setting);
H_APIs ssv6xxx_result ssv6xxx_wifi_station(u8 hw_mode,struct sta_setting *sta_station);
H_APIs ssv6xxx_result ssv6xxx_wifi_status(struct ap_sta_status *status_info);
H_APIs ssv6xxx_result ssv6xxx_wifi_ap(Ap_setting *ap_setting);
H_APIs ssv6xxx_result ssv6xxx_wifi_station(u8 hw_mode,Sta_setting *sta_station);
H_APIs ssv6xxx_result ssv6xxx_wifi_status(Ap_sta_status *status_info);


#ifdef _MSC_VER
#pragma pack(pop)
#endif /* _MSC_VER */

#endif /* _HOST_APIS_H_ */



