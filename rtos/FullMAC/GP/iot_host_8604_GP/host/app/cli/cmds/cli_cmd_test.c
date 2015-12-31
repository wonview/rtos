#include <config.h>
#include <cmd_def.h>
#include <log.h>
#include <host_global.h>
#include "lwip/netif.h"
#include <hdr80211.h>
#include "cli_cmd_wifi.h"
#include "cli_cmd_test.h"
#include <host_apis.h>
#include <hwmac/drv_mac.h>
#include <rtos.h>
//#include <apps/mac80211/mlme.h>
//#include <apps/mac80211/sta_info.h>


#include <os_wrapper.h>
#include <ap/common/ieee80211.h>

#define ETH_ALEN	6


enum cmd_frame_type {
	CMD_FRAME_PS_ON,		
	CMD_FRAME_PS_OFF,
	CMD_FRAME_PS_POLL,
	CMD_FRAME_TRIGER_FRAME,	
};





#ifdef __AP_DEBUG__

u8 BSS_MAC_ADDR[6] = {0};
static u8 MAC_ADDR[6] ={0};

//extern bool g_sta_sleep;


u32 gen_data_frame(u8 *raw,  bool qos, bool pm)
{
	struct ieee80211_qos_hdr *nullfunc;
	__le16 fc;
	u32 len;
	struct ieee80211_qos_hdr null={0};	
			
	len = sizeof(null);

   

	if (qos) {
		fc = cpu_to_le16(IEEE80211_FTYPE_DATA |
				 IEEE80211_STYPE_QOS_NULLFUNC |
				 IEEE80211_FCTL_TODS);
	} else {
		len -= 2;
		fc = cpu_to_le16(IEEE80211_FTYPE_DATA |
				 IEEE80211_STYPE_NULLFUNC |
				 IEEE80211_FCTL_TODS);
	}

	if(pm)
		fc |= cpu_to_le16(IEEE80211_FCTL_PM);

	
	nullfunc = &null;	
	nullfunc->frame_control = fc;
	nullfunc->duration_id = 0;
	memcpy(nullfunc->addr1, BSS_MAC_ADDR, ETH_ALEN);
	memcpy(nullfunc->addr2, MAC_ADDR, ETH_ALEN);
	memcpy(nullfunc->addr3, BSS_MAC_ADDR, ETH_ALEN);


	if (qos) {
		nullfunc->qos_ctrl = cpu_to_le16(0);			
	}	
	


	memcpy(raw, &null, len);

	return len;
}

extern u16 get_sta_aid();
u32 gen_ps_poll_frame(u8 *raw)
{
	struct ieee80211_pspoll *pspoll = (struct ieee80211_pspoll *)raw;		
	u32 len = sizeof(struct ieee80211_pspoll);
	


	
	pspoll->frame_control = cpu_to_le16(IEEE80211_FTYPE_CTL | 
										IEEE80211_STYPE_PSPOLL|
										IEEE80211_FCTL_TODS);

	
	pspoll->aid = cpu_to_le16(get_sta_aid());

	/* aid in PS-Poll has its two MSBs each set to 1 */
	pspoll->aid |= cpu_to_le16(1 << 15 | 1 << 14);




	memcpy(pspoll->bssid, &BSS_MAC_ADDR, ETH_ALEN);
	memcpy(pspoll->ta, &MAC_ADDR, ETH_ALEN);






	//memcpy(raw, &null, len);

	return len;
}



void cmd_test_gen_frame(enum cmd_frame_type f_type)
{
	s32 len;
	void *frame, *raw;	
	bool qos = 0;


	drv_mac_get_bssid(&BSS_MAC_ADDR[0]);
	drv_mac_get_sta_mac(&MAC_ADDR[0]);
	

	frame = os_frame_alloc(1024);
	raw = OS_FRAME_GET_DATA(frame);


	switch(f_type)
	{
		case CMD_FRAME_PS_ON:
			len = gen_data_frame(raw, 0, 1);
			
			break;

		case CMD_FRAME_PS_OFF:
			len = gen_data_frame(raw, 0, 0);
//			g_sta_sleep = FALSE;
			break;			

		case CMD_FRAME_TRIGER_FRAME:
			len = gen_data_frame(raw, 1, 1);		
			break;

		case CMD_FRAME_PS_POLL:
			len = gen_ps_poll_frame(raw);		
			break;


		default:
			break;
	}




	
	



	
	OS_FRAME_SET_DATA_LEN(frame, len);	
	ssv6xxx_wifi_send_80211(frame, len);


	

}






#include <edca/drv_edca.h>
extern void drv_edca_get_wmm_param(edca_tx_queue_type que_type, wmm_param *pWmmParam);

void print_edca()
{
	int i;
	wmm_param pWmmParam={0};

	for(i=0;i<6;i++)
	{
		drv_edca_get_wmm_param(i, &pWmmParam);

		LOG_PRINTF("pWmmParam->aifsn:%d \npWmmParam->acm:%d \npWmmParam->resv:%d \npWmmParam->cwmin:%d \npWmmParam->cwmax:%d \npWmmParam->txop:%d \npWmmParam->backoffvalue:%d \n"
			"pWmmParam->enable_backoffvalue:%d \npWmmParam->RESV:%d\n"
			,pWmmParam.a.aifsn, 
			pWmmParam.a.acm, 
			pWmmParam.a.resv, 
			pWmmParam.a.cwmin,
			pWmmParam.a.cwmax,
			pWmmParam.a.txop,
			pWmmParam.backoffvalue,
			pWmmParam.enable_backoffvalue,
			pWmmParam.RESV);
	
		
	}
	
}


#if (BEACON_DBG == 1) 
void cmd_test_b_release_test_cmd()
{
	u32 dummy=0;
	ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_BEACON_RELEASE_TEST, &dummy, 4);	
}
#endif





void cmd_test(s32 argc, char *argv[])
{
	
	if ((argc == 2) && (strcmp(argv[1], "ps-on") == 0))	
		cmd_test_gen_frame(CMD_FRAME_PS_ON);	
	else if ((argc == 2) && (strcmp(argv[1], "ps-off") == 0))
		cmd_test_gen_frame(CMD_FRAME_PS_OFF);
	else if ((argc == 2) && (strcmp(argv[1], "ps-poll") == 0))
		cmd_test_gen_frame(CMD_FRAME_PS_POLL);
	else if ((argc == 2) && (strcmp(argv[1], "t-frame") == 0))
		cmd_test_gen_frame(CMD_FRAME_TRIGER_FRAME);
	else if ((argc == 2) && (strcmp(argv[1], "edca") == 0))
		print_edca();
	
#if (BEACON_DBG == 1)
	else if((argc == 2) && (strcmp(argv[1], "b-release") == 0))
		cmd_test_b_release_test_cmd();
#endif
	
	else
	{;}


	return;
	
}





extern void APStaInfo_PrintStaInfo(void);
void cmd_ap(s32 argc, char *argv[])
{


	if ((argc == 2) && (strcmp(argv[1], "sta_list") == 0))
	{
		APStaInfo_PrintStaInfo();
		return;
	}

	


}







#endif







