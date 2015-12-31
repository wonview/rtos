//#define __SFILE__ "ap_drv_cmd.c"
#include "ap_drv_cmd.h"
#include "ap_config.h"

#include "common/ether.h"
#include <cmd_def.h>
#include <host_apis.h>
#include <config.h>
#include <os_wrapper.h>
#include "dev.h"
#include <txrx_hdl.h>


#ifdef __AP_DEBUG__
#include "ap_info.h"
#endif



void ap_soc_set_bcn(enum ssv6xxx_tx_extra_type extra_type, void *frame, struct cfg_bcn_info *bcn_info, u8 dtim_cnt, u16 bcn_itv)
{
    struct ssv6200_tx_desc *req = (struct ssv6200_tx_desc *)frame;
	u16 len = sizeof(struct ssv6200_tx_desc)+ bcn_info->bcn_len;
	u8* raw = (u8*)req;
	u8 extra_len;



#ifdef __AP_DEBUG__
	if(extra_type != SSV6XXX_SET_INIT_BEACON &&  extra_type != SSV6XXX_SET_BEACON)
		ASSERT(FALSE);
#endif



	//Fill Extra info
	raw +=  len;

	//Fill EID
	*raw = extra_type;//-------------------------------------------------------------------->
	raw++;

	if(extra_type == SSV6XXX_SET_INIT_BEACON)
	{
		struct cfg_set_init_bcn init_bcn;
		init_bcn.bcn_info = *bcn_info;
		//init_bcn.param.bcn_enable = TRUE;
		init_bcn.param.bcn_itv = bcn_itv;
		init_bcn.param.dtim_cnt = dtim_cnt;

		extra_len = sizeof(struct cfg_set_init_bcn);

		//Fill E length
		*raw = extra_len;//------------------------------------------------------------------>
		raw++;

		//Fill data
		MEMCPY(raw, &init_bcn, extra_len);//---------------------------------------------->
	}
	else
	{
		//SSV6XXX_SET_BEACON

		extra_len = sizeof(struct cfg_bcn_info);

		//Fill E length
		*raw = extra_len;//-------------------------------------------------------------------->
		raw++;


		//Fill data
		MEMCPY(raw, bcn_info, extra_len);//------------------------------------------------>
	}

	raw+=extra_len;


	//Extra total length
	*(u8*)raw = extra_len+2;//data length plus EID and length
	raw+=2;


	req->len = (u32)(raw-(u8*)req);


#ifdef __AP_DEBUG__
	//check if write buffer too much
		if(frame == gDeviceInfo->APInfo->bcn &&  req->len> AP_MGMT_BEACON_LEN)
			ASSERT(FALSE);
#endif

// set tx_desc
    ssv6xxx_beacon_fill_tx_desc(gDeviceInfo->APInfo->bcn_info.bcn_len,gDeviceInfo->APInfo->bcn);
    ssv6xxx_beacon_set(gDeviceInfo->APInfo->bcn, gDeviceInfo->APInfo->bcn_info.tim_cnt_oft);


}




















#ifdef __AP_DEBUG__
extern void APStaInfo_PrintStaInfo();
#endif
void ap_soc_cmd_sta_oper(enum cfg_sta_oper sta_oper, struct ETHER_ADDR_st *addr, enum cfg_ht_type ht, bool qos)
{

	struct cfg_set_sta	cfg_sta;
	struct cfg_wsid_info *wsid_info = &cfg_sta.wsid_info;

    MEMSET(&cfg_sta, 0, sizeof(struct cfg_set_sta));
	cfg_sta.sta_oper = sta_oper;

	if(CFG_STA_ADD == sta_oper)
		SET_STA_INFO_VALID(wsid_info, TRUE);
	else
		SET_STA_INFO_VALID(wsid_info, FALSE);

	SET_STA_INFO_OP_MODE(wsid_info, CFG_OP_MODE_STA);
	SET_STA_INFO_QOS_EN(wsid_info, qos);
	SET_STA_INFO_HT_MODE(wsid_info, ht);

	if(addr)
		memcpy((void*)&wsid_info->addr, (void*)addr, ETH_ALEN);

	//Add sta to soc
	_ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_SET_STA, &cfg_sta, sizeof(struct cfg_set_sta),FALSE);

#ifdef __AP_DEBUG__
	APStaInfo_PrintStaInfo();
#endif//__AP_DEBUG__



}




//Send frame directly.
s32 ap_soc_data_send(void *frame, s32 len, bool bAPFrame, u32 TxFlags)
{
	struct cfg_host_txreq0 *req = (struct cfg_host_txreq0 *)OS_FRAME_GET_DATA(frame);
	len+=sizeof(struct cfg_host_txreq0);

#ifdef __AP_DEBUG__
//check if write buffer too much
	if(frame == gDeviceInfo->APInfo->pOSMgmtframe && len > AP_MGMT_PKT_LEN)
		ASSERT(FALSE);
#endif

	req->len = (u32)len;
	OS_FRAME_SET_DATA_LEN(frame, len);

    TxHdl_FrameProc(frame, bAPFrame, 0, TxFlags);
	return 0;
}



