#include <config.h>
#include <msgevt.h>
#include <pbuf.h>
#include <host_apis.h>
//#include <mbox/drv_mbox.h>
#include <drv_mac.h>
#include <ssv_lib.h>
#include <log.h>
#include <cmd_def.h>
#include <os_wrapper.h>

#include "wsimp_sim.h"
#include "wsimp_lib.h"
#include <drv_mac.h>

#include <rtos.h>


extern inline H_APIs s32 _ssv6xxx_wifi_send_80211(void *frame, s32 len, u8 tx_dscrp_flag);
extern inline H_APIs s32 _ssv6xxx_wifi_send_ehternet(void *frame, s32 len, enum ssv6xxx_data_priority priority, u8 tx_dscrp_flag);

FILE *g_wsimp_outfp;
u32 g_wsimp_flags;




#define NET_APP_MAX_ARGS            10
#define NET_APP_ARGS_SIZE           (sizeof(char *)*NET_APP_MAX_ARGS)
#define NET_APP_BUF_SIZE            128


u8 g_wsimp_cmd_buf[NET_APP_ARGS_SIZE+NET_APP_BUF_SIZE];
u8 g_wsimp_cmd_argc;
//char *g_wsimp_cmd_argv[100];



extern net_app_run(s32 argc, char *argv[]);

void cmd_loop_pattern(void)
{
	s32 res = 0;

	OS_MutexLock(g_wsimp_mutex);
	if(g_wsimp_cmd_argc)
		res = net_app_run(g_wsimp_cmd_argc, (char**)g_wsimp_cmd_buf);
	OS_MutexUnLock(g_wsimp_mutex);
	
	if (res < 0)
		LOG_PRINTF("run pattern failure !!\n");

}
	
static struct wsimp_file_info sg_sim_info;



