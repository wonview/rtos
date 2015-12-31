#ifndef _AP_DRV_H_
#define _AP_DRV_H_

#include <cmd_def.h>
#include "common/ether.h"
#include <host_apis.h>


void ap_soc_set_bcn(enum ssv6xxx_tx_extra_type extra_type, void *frame, struct cfg_bcn_info *bcn_info, u8 dtim_cnt, u16 bcn_itv);

s32 ap_soc_data_send(void *frame, s32 len, bool bAPFrame, u32 TxFlags);


void ap_soc_cmd_sta_oper(enum cfg_sta_oper sta_oper, struct ETHER_ADDR_st *addr, enum cfg_ht_type ht, bool qos);


#endif//_AP_DRV_H_
