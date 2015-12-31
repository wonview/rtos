//#include "host_global.h"
#include "os_wrapper.h"
#include "log.h"
#include <drv/ssv_drv.h>
#include "pbuf.h"
#include "ssv_dev.h"
#include "dev.h"
#include <cli/cmds/cli_cmd_wifi.h>

#include "httpserver_raw/httpd.h"


u32 g_log_module;
u32 g_log_min_level;

extern int udhcpd_init(void);

#if 0
struct ap_info_st sg_ap_info[20];
#endif // 0

void stop_and_halt (void)
{
    //while (1) {}
    /*lint -restore */
} // end of - stop_and_halt -

int ssv6xxx_add_interface(ssv6xxx_hw_mode hw_mode)
{
    //0:false 1:true
    return 1;

}
extern struct task_info_st g_txrx_task_info[];
extern struct task_info_st g_host_task_info[];
extern struct task_info_st g_timer_task_info[];
#if !NET_MGR_NO_SYS
extern struct task_info_st st_netmgr_task[];
#endif
extern struct task_info_st st_dhcpd_task[];
#if (MLME_TASK==1)
    struct task_info_st g_mlme_task_info[]
#endif

void ssv6xxx_init_task_para(void)
{
    g_txrx_task_info[0].prio = OS_TX_TASK_PRIO;
    g_txrx_task_info[1].prio = OS_RX_TASK_PRIO;
    g_host_task_info[0].prio = OS_CMD_ENG_PRIO;
    g_timer_task_info[0].prio = OS_TMR_TASK_PRIO;
#if !NET_MGR_NO_SYS
    st_netmgr_task[0].prio = OS_NETMGR_TASK_PRIO;
#endif
    st_dhcpd_task[0].prio = OS_DHCPD_TASK_PRIO;
#if (MLME_TASK==1)
    g_mlme_task_info[].prio = OS_MLME_TASK_PRIO;
#endif
}
int ssv6xxx_start()
{
    ssv6xxx_drv_start();

    /* Reset MAC & Re-Init */

    /* Initialize ssv6200 mac */
    if(cabrio_init_mac()!= 0)
    {
    	return 1;
    }

    //Set ap or station register
    if ((SSV6XXX_HWM_STA == gDeviceInfo->hw_mode)||(SSV6XXX_HWM_SCONFIG == gDeviceInfo->hw_mode))
    {
        cabrio_init_sta_mac();
    }
    else
    {
        cabrio_init_ap_mac();
    }

    return 0;
}



/**
 *  Entry point of the firmware code. After system booting, this is the
 *  first function to be called from boot code. This function need to
 *  initialize the chip register, software protoctol, RTOS and create
 *  tasks. Note that, all memory resources needed for each software
 *  modulle shall be pre-allocated in initialization time.
 */
/* return int to avoid from compiler warning */

extern void lwip_reg_rx_data(void);
extern void lwip_sys_init(void);

int ssv6xxx_dev_init(int argc, char *argv[])
{

	g_log_module = CONFIG_LOG_MODULE;
	g_log_min_level = CONFIG_LOG_LEVEL;

	/**
        * On simulation/emulation platform, initialize RTOS (simulation OS)
        * first. We use this simulation RTOS to create the whole simulation
        * and emulation platform.
        */
    ASSERT( OS_Init() == OS_SUCCESS );
    host_global_init();
    ssv6xxx_init_task_para();
    
	LOG_init(true, true, LOG_LEVEL_ON, LOG_MODULE_MASK(LOG_MODULE_EMPTY), false);
#ifdef __SSV_UNIX_SIM__
	LOG_out_dst_open(LOG_OUT_HOST_TERM, NULL);
	LOG_out_dst_turn_on(LOG_OUT_HOST_TERM);
#endif




    ASSERT( msg_evt_init() == OS_SUCCESS );

#if (CONFIG_USE_LWIP_PBUF==0)
    ASSERT( PBUF_Init() == OS_SUCCESS );
#endif//#if CONFIG_USE_LWIP_PBUF



    lwip_sys_init();

    /**
        * Initialize Host simulation platform. The Host initialization sequence
        * shall be the same as the sequence on the real host platform.
        *   @ Initialize host device drivers (SDIO/SIM/UART/SPI ...)
        */
    {

        ASSERT(ssv6xxx_drv_module_init() == true);
        LOG_PRINTF("Try to connecting CABRIO via %s...\n\r",INTERFACE);
        if (ssv6xxx_drv_select(INTERFACE) == false)
        {

            {
            LOG_PRINTF("==============================\n\r");
        	LOG_PRINTF("Please Insert %S wifi device\n",INTERFACE);
			LOG_PRINTF("==============================\n\r");
        	}
			return -1;
        }


        if(ssv6xxx_wifi_init()!=SSV6XXX_SUCCESS)
			return -1;
    }


    //------------------------------------------
    //Read E-fuse
    //efuse_read_all_map();
    //------------------------------------------
    if(ssv6xxx_chip_init() == 0)
    {
        LOG_PRINTF("ssv6xxx_chip_init fail\n\r");
		return 0;
    }
    #if AUTO_INIT_STATION
    sta_mode_on(SSV6XXX_HWM_STA);
    if(ssv6xxx_add_interface(gDeviceInfo->hw_mode) == 0)
	{
	    LOG_PRINTF("ssv6xxx_add_interface fail\n\r");
		return 0;
    }

    #endif





    /**
        * Initialize TCP/IP Protocol Stack. If tcpip is init, the net_app_ask()
        * also need to be init.
        */

    tcpip_init(NULL, NULL);
    lwip_reg_rx_data();

    if(udhcpd_init() != 0)
    {
		LOG_PRINTF("udhcpd_init fail\r\n");
		return 0;
    }

    httpd_init();
    if(net_app_init()== 1)
	{
		LOG_PRINTF("net_app_init fail\n\r");
		return 0;
    }
    netmgr_init();
    Cli_Init(argc, argv);


    /* Init simulation & emulation configuration */
#ifdef THROUGHPUT_TEST
//	ssv6xxx_wifi_reg_evt_cb(sdio_rx_test_evt_cb);
	_ssv6xxx_wifi_reg_rx_cb(sdio_rx_test_dat_cb, TRUE);
#endif

    ssv6xxx_wifi_cfg();
#if(MLME_TASK ==1)
    mlme_init();    //MLME task initial
#endif

    return 1;
}

