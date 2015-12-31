#ifndef _AP_H_
#define _AP_H_

#include <host_apis.h>



//void AP_Task( void *args );

s32 AP_Init(u32 max_sta_num);

ssv6xxx_data_result AP_RxHandleFrame(void *frame);


typedef enum{
	SSV6XXX_AP_ON		,	//On
	SSV6XXX_AP_OFF		,	//Off
}ssv6xxx_ap_cmd;



typedef s32 (*APCmdCb)(ssv6xxx_ap_cmd cmd, u32 nResult);
s32 AP_Command(ssv6xxx_ap_cmd cmd, APCmdCb cb, void *data);






#endif//_AP_H_
