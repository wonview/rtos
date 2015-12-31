
#include <config.h>
#include <msgevt.h>
#include <cli/cli.h>
#include <regs/ssv6200_reg.h>
#include <drv/ssv_drv_config.h>
#include <drv/ssv_drv.h>
#include "cli_cmd.h"
#include "cli_cmd_soc.h"
#include "cli_cmd_net.h"
#include "cli_cmd_wifi.h"
#include "cli_cmd_sim.h"
#include "cli_cmd_test.h"
#include "cli_cmd_sdio_test.h"
#include "cli_cmd_throughput.h"
#include "cli_cmd_sys.h"
#include <log.h>
#include <ssv_lib.h>
//#include <regs.h>
#include <cmd_def.h>
#include <host_apis.h>

#if (defined _WIN32)
#include <drv/sdio/sdio.h>
#elif (defined __linux__)
#include <drv/sdio_linux/sdio.h>
#endif
//#include <ssv6200_uart_bin.h>
#include <porting.h>
#include <os_wrapper.h>



/*---------------------------------- CMDs -----------------------------------*/

static void cmd_abort( s32 argc, char *argv[] )
{
    //char *prt = NULL;
	//printf("%d\n", *prt);
    abort();
}

#ifdef __SSV_UNIX_SIM__
extern char SysProfBuf[256];
extern s8 SysProfSt;
#endif

static void cmd_exit( s32 argc, char *argv[] )
{
	//ssv6xxx_drv_module_release();
#ifdef __SSV_UNIX_SIM__
    if(SysProfSt == 1)
    {
        s16 i;

        LOG_PRINTF("*************System profiling result*************\n");
        for(i=0; i<256; i++)
            LOG_PRINTF("%c",SysProfBuf[i]);

        LOG_PRINTF("*************End of profiling result*************\n");
    }
    LOG_PRINTF("cmd_exit");
    reset_term();
#endif
    ssv6xxx_wifi_deinit();
    OS_Terminate();
}

#if 0
static void cmd_sdrv(s32 argc, char *argv[])
{
    // Usage : sdrv {list} | {select [sim|uart|usb|sdio|spi]}
	if (argc == 2 && strcmp(argv[1], "list") == 0)
	{
		ssv6xxx_drv_list();
		return;
	}
	else if ((argc == 2) && ((strcmp(argv[1], DRV_NAME_SIM)  == 0) ||
							 (strcmp(argv[1], DRV_NAME_UART) == 0) ||
							 (strcmp(argv[1], DRV_NAME_USB)  == 0) ||
							 (strcmp(argv[1], DRV_NAME_SDIO) == 0) ||
							 (strcmp(argv[1], DRV_NAME_SPI)  == 0)))
	{
		ssv6xxx_drv_select(argv[1]);
		return;
	}
    log_printf("Usage : sdrv list\n"  \
			   "desc  : list all available drivers in ssv driver module.\n\n" \
			   "Usage : sdrv [sim|uart|usb|sdio|spi]\n" \
		       "desc  : select [sim|uart|usb|sdio|spi] driver to use.\n");
}
#endif


#if CONFIG_STATUS_CHECK
extern u32 g_dump_tx;
#endif

static void cmd_dump( s32 argc, char *argv[] )
{
//    u8 dump_all;

//    if (argc != 2) {
//        LOG_PRINTF("Invalid Parameter !\n");
//        return;
//    }
#if CONFIG_STATUS_CHECK
    if (strcmp(argv[1], "tx") == 0){
        if (strcmp(argv[2], "on") == 0){
            g_dump_tx = 1;
            LOG_PRINTF("tx on\r\n");
        }else{
            g_dump_tx = 0;
            LOG_PRINTF("tx off\r\n");
        }
    }
#endif//#if CONFIG_STATUS_CHECK



#if 0
    if (strcmp(argv[1], "all") == 0)
        dump_all = true;
    else dump_all = false;

    /* Dump PHY-Info: */
    if (strcmp(argv[1], "phyinfo")==0 || dump_all==true) {
        if (dump_all == false)
            ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_GET_PHY_INFO_TBL, NULL, 0);

    }

    /* Dump Decision Table: */
    if (strcmp(argv[1], "decision") == 0 || dump_all==true) {
        if (dump_all == false)
            ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_GET_DECI_TBL, NULL, 0);

    }

    /* Dump STA MAC Address: */
    if (strcmp(argv[1], "sta-mac") == 0 || dump_all==true) {
        if (dump_all == false)
            ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_GET_STA_MAC, NULL, 0);
    }


    /* Dump STA BSSID: */
    if (strcmp(argv[1], "bssid") == 0 || dump_all==true) {
        if (dump_all == false)
            ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_GET_BSSID, NULL, 0);

    }

    /* Dump Ether Trap Table: */
    if (strcmp(argv[1], "ether-trap") == 0 || dump_all==true) {
        if (dump_all == false)
            ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_GET_ETHER_TRAP, NULL, 0);

    }

    /* Dump Flow Command Table: */
    if (strcmp(argv[1], "fcmd") == 0 || dump_all==true) {
        if (dump_all == false)
            ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_GET_FCMDS, NULL, 0);

    }

    /* Dump WSID Table: */
    if (strcmp(argv[1], "wsid") == 0 || dump_all==true) {
        if (dump_all == false)
            ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_GET_WSID_TBL, NULL, 0);

    }
#endif
}

#if 0
static void cmd_phy_config( s32 argc, char *argv[] )
{
	ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_PHY_ON, NULL, 0);
}

static void cmd_cal( s32 argc, char *argv[] )
{
	 u32 channel_index= 0;
	 channel_index=(u32)ssv6xxx_atoi(argv[1]);
	 ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_CAL, &channel_index, sizeof(u32));
}
#endif

#if CONFIG_STATUS_CHECK

extern void stats_display(void);
extern void stats_p(void);
extern void stats_m(void);
extern void SSV_PBUF_Status(void);

extern u32 g_l2_tx_packets;
extern u32 g_l2_tx_copy;
extern u32 g_l2_tx_late;
extern u32 g_notpool;

extern u32 g_heap_used;
extern u32 g_heap_max;
extern u32 g_tx_allocate_fail;


void dump_protocol(void){
    stats_p();
    LOG_PRINTF("\tl2 tx pakets[%d]\r\n\tl2_tx_copy[%d]\r\n\tl2_tx_late[%d]\r\n\tbuf_not_f_pool[%d]\r\n\ttx_allocate_fail[%d]\r\n\t",
                    g_l2_tx_packets, g_l2_tx_copy, g_l2_tx_late, g_notpool, g_tx_allocate_fail);
}



void dump_mem(void){

    stats_m();
#if (CONFIG_USE_LWIP_PBUF == 0)
    SSV_PBUF_Status();
#endif
    LOG_PRINTF("\tg_heap_used[%lu] g_heap_max[%lu]\r\n", g_heap_used, g_heap_max);

}


#if CONFIG_MEMP_DEBUG
#include "lwip/memp.h"
extern void dump_mem_pool(memp_t type);
extern void dump_mem_pool_pbuf();
#endif

void cmd_tcpip_status (s32 argc, char *argv[]){

    if(argc >=2){

        if (strcmp(argv[1], "p") == 0) {
            dump_protocol();
            return;
        }
        else if (strcmp(argv[1], "m") == 0) {
            dump_mem();
            return;
        }
#if CONFIG_MEMP_DEBUG
        else if(strcmp(argv[1], "mp") == 0){

            dump_mem_pool(ssv6xxx_atoi(argv[2]));
            return;
        }
        else if(strcmp(argv[1], "mpp") == 0){
            dump_mem_pool_pbuf();

             LOG_PRINTF("\tl2 tx pakets[%d]\r\n\tl2_tx_copy[%d]\r\n\tl2_tx_late[%d]\r\n\tbuf_not_f_pool[%d]\r\n\ttx_allocate_fail[%d]\r\n",
                    g_l2_tx_packets, g_l2_tx_copy, g_l2_tx_late, g_notpool, g_tx_allocate_fail);
            return;
        }

#endif//#if CONFIG_MEMP_DEBUG



    }

    dump_protocol();
    dump_mem();
}
#endif//CONFIG_STATUS_CHECK


u8 cmd_top_enable=0;
void cmd_top (s32 argc, char *argv[])
{
    u32 cpu_isr=0;
    u32 on=0;
    if(argc!=2){
        goto usage;
    }

    on=ssv6xxx_atoi(argv[1]);
    cpu_isr=OS_EnterCritical();
    cmd_top_enable=on;
    OS_ExitCritical(cpu_isr);
    return;

usage:
    LOG_PRINTF("usage\r\n");
    LOG_PRINTF("top 2  <--enable cpu usage log\r\n");
    LOG_PRINTF("top 1  <--enable task and cpu usage profiling\r\n");
    LOG_PRINTF("top 0  <--disable\r\n");
}


#if(CONFIG_IO_BUS_THROUGHPUT_TEST==1)
typedef enum {
    EN_TX_START=0x36,
    EN_RX_START,
    EN_GOING,
    EN_END,
}EN_BUS_THROUTHPUT_STATUS;

typedef struct{
    EN_BUS_THROUTHPUT_STATUS status;
    union{
        u32 TotalSize;
        u32 Size;
    }un;
}ST_BUS_THROUTHPUT;
//extern bool _wait_tx_resource(u32 frame_len);
u32 loop_times=0;
#define PER_SIZE MAX_RECV_BUF
#define TOTAL_SIZE (PER_SIZE*loop_times)
static void _cmd_bus_tput_tx(void)
{

    HDR_HostEvent *evt=NULL;
    struct cfg_host_cmd *host_cmd=NULL;
    u8* frame=NULL;
    ST_BUS_THROUTHPUT *pstTput=NULL;
    s32 send_size=0;
    u32 start_tick=0;
    u32 end_tick=0;
    u32 seq_no=0;
    seq_no=0;

    /* =======================================================
                     Send start cmd
    =======================================================*/
    LOG_PRINTF("Send Start cmd\r\n");
    frame = os_frame_alloc(sizeof(ST_BUS_THROUTHPUT)+HOST_CMD_HDR_LEN);
    host_cmd = (struct cfg_host_cmd *)OS_FRAME_GET_DATA(frame);
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd=SSV6XXX_HOST_CMD_BUS_THROUGHPUT_TEST ;
    host_cmd->len = HOST_CMD_HDR_LEN+sizeof(ST_BUS_THROUTHPUT);
    seq_no++;
    host_cmd->cmd_seq_no=seq_no;
    pstTput=(ST_BUS_THROUTHPUT *)host_cmd->un.dat8;
    pstTput->status=EN_TX_START;
    pstTput->un.TotalSize=TOTAL_SIZE;
#ifndef __SSV_UNIX_SIM__
    ssv6xxx_drv_wait_tx_resource(OS_FRAME_GET_DATA_LEN(frame));
#endif
    ssv6xxx_drv_send(OS_FRAME_GET_DATA(frame), OS_FRAME_GET_DATA_LEN(frame));
    os_frame_free(frame);


    /* =======================================================
                     Send Data
    =======================================================*/
    start_tick=OS_GetSysTick();
    send_size=TOTAL_SIZE;
    do{
        seq_no++;
        frame = os_frame_alloc(PER_SIZE);
        host_cmd = (struct cfg_host_cmd *)OS_FRAME_GET_DATA(frame);
        host_cmd->c_type = HOST_CMD;
        host_cmd->h_cmd=SSV6XXX_HOST_CMD_BUS_THROUGHPUT_TEST ;
        host_cmd->len = PER_SIZE ;

        host_cmd->cmd_seq_no=seq_no;
        pstTput=(ST_BUS_THROUTHPUT *)host_cmd->un.dat8;
        pstTput->status=EN_GOING;
        pstTput->un.Size=PER_SIZE;
#ifndef __SSV_UNIX_SIM__
        ssv6xxx_drv_wait_tx_resource(OS_FRAME_GET_DATA_LEN(frame));
#endif
        ssv6xxx_drv_send(OS_FRAME_GET_DATA(frame), OS_FRAME_GET_DATA_LEN(frame));
        send_size-=PER_SIZE;
        os_frame_free(frame);
    }while(send_size!=0);

    /* =======================================================
                     Get Status
    =======================================================*/
    //LOG_PRINTF("Get Status\r\n");
    os_frame_alloc(PER_SIZE);
    while(-1==ssv6xxx_drv_recv(OS_FRAME_GET_DATA(frame), OS_FRAME_GET_DATA_LEN(frame))){;}
    evt=(HDR_HostEvent *)OS_FRAME_GET_DATA(frame);
    pstTput=(ST_BUS_THROUTHPUT *)evt->dat;
    os_frame_free(frame);
    end_tick=OS_GetSysTick();
    LOG_PRINTF("Test Done \r\n");
    LOG_PRINTF("TOTAL_SIZE=%d,Total Time=%d\r\n",TOTAL_SIZE,(end_tick-start_tick)*OS_TICK_RATE_MS);
    LOG_PRINTF("Throughput=%d KBps (%d Mbps) \r\n",(TOTAL_SIZE/((end_tick-start_tick)*OS_TICK_RATE_MS)),(((TOTAL_SIZE/((end_tick-start_tick)*OS_TICK_RATE_MS))*8)/1024));

}

static void _cmd_bus_tput_rx(void)
{

    HDR_HostEvent *evt=NULL;
    struct cfg_host_cmd *host_cmd=NULL;
    u8* frame=NULL;
    ST_BUS_THROUTHPUT *pstTput=NULL;
    u32 receive_size=0;
    s32 rlen=0;
    u32 start_tick=0;
    u32 end_tick=0;

    /* =======================================================
                     Send start cmd
    =======================================================*/
    LOG_PRINTF("Send Start cmd\r\n");
    frame = os_frame_alloc(sizeof(ST_BUS_THROUTHPUT)+HOST_CMD_HDR_LEN);
    host_cmd = (struct cfg_host_cmd *)OS_FRAME_GET_DATA(frame);
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd=SSV6XXX_HOST_CMD_BUS_THROUGHPUT_TEST ;
    host_cmd->len = sizeof(ST_BUS_THROUTHPUT)+HOST_CMD_HDR_LEN;
    pstTput=(ST_BUS_THROUTHPUT *)host_cmd->un.dat8;
    pstTput->status=EN_RX_START;
    pstTput->un.TotalSize=TOTAL_SIZE;
#ifndef __SSV_UNIX_SIM__
    ssv6xxx_drv_wait_tx_resource(OS_FRAME_GET_DATA_LEN(frame));
#endif
    ssv6xxx_drv_send(OS_FRAME_GET_DATA(frame), OS_FRAME_GET_DATA_LEN(frame));
    os_frame_free(frame);


    /* =======================================================
                     Get Data
    =======================================================*/
    start_tick=OS_GetSysTick();
    do{
        os_frame_alloc(PER_SIZE);
        rlen=ssv6xxx_drv_recv(OS_FRAME_GET_DATA(frame), OS_FRAME_GET_DATA_LEN(frame));
        if(-1!=rlen){
            evt=(HDR_HostEvent *)OS_FRAME_GET_DATA(frame);
            pstTput=(ST_BUS_THROUTHPUT *)evt->dat;
            receive_size+=PER_SIZE;
        }
        os_frame_free(frame);
    }while(receive_size!=TOTAL_SIZE);


    /* =======================================================
                     Get Status
    =======================================================*/
    os_frame_alloc(PER_SIZE);
    while(-1==ssv6xxx_drv_recv(OS_FRAME_GET_DATA(frame), OS_FRAME_GET_DATA_LEN(frame))){;}
    evt=(HDR_HostEvent *)OS_FRAME_GET_DATA(frame);
    pstTput=(ST_BUS_THROUTHPUT *)evt->dat;
    os_frame_free(frame);
    end_tick=OS_GetSysTick();
    LOG_PRINTF("Test Done\r\n");
    LOG_PRINTF("TOTAL_SIZE=%d,Total Time=%d\r\n",TOTAL_SIZE,(end_tick-start_tick)*OS_TICK_RATE_MS);
    LOG_PRINTF("Throughput=%d KBps (%d Mbps) \r\n",(TOTAL_SIZE/((end_tick-start_tick)*OS_TICK_RATE_MS)),((TOTAL_SIZE/((end_tick-start_tick)*OS_TICK_RATE_MS))*8)/1024);
}

extern void ssv6xxx_rf_disable();
extern void ssv6xxx_rf_enable();


static void cmd_bus_tput(s32 argc, char *argv[])
{
    u32 cpu_sr;
    if(argc!=3){
        LOG_PRINTF("usage:\r\n");
        LOG_PRINTF("     bus -t [loop times]\r\n");
        LOG_PRINTF("     bus -r [loop times]\r\n");
        return;
    }
    ssv6xxx_drv_irq_disable();
    ssv6xxx_rf_disable();
    cpu_sr=OS_EnterCritical();
    loop_times = ssv6xxx_atoi_base(argv[2], 10);
    if(strcmp(argv[1], "-t") == 0)
    {
        _cmd_bus_tput_tx();
    }

    if(strcmp(argv[1], "-r") == 0)
    {
        _cmd_bus_tput_rx();
    }
    OS_ExitCritical(cpu_sr);
    ssv6xxx_rf_enable();
    ssv6xxx_drv_irq_enable();
}
#endif

#if (SDRV_INCLUDE_SPI)
extern u32 gSpiZeroLenCount;
extern u32 gSpiZeroLenRetryCount;
extern u32 gSpiReadCount;
//extern u32 RxTaskRetryCount[32];
static void cmd_spi_status(s32 argc, char *argv[])
{
    //int i=0;

    LOG_PRINTF("SPI total reading count is %1d\r\n",gSpiReadCount);
    LOG_PRINTF("The count of zero length is %d\r\n",gSpiZeroLenCount);
    LOG_PRINTF("The count of spi status double checking is %d\r\n",gSpiZeroLenRetryCount);
    #if 0
    for(i=0;i<32;i++){
        if(RxTaskRetryCount[i]!=0){
            LOG_PRINTF("RxTaskRetryCount[%d]: %d\r\n",i,RxTaskRetryCount[i]);
        }
    }
    #endif
    return;

}
#endif

CLICmds gCliCmdTable[] =
{
    #ifdef __SSV_UNIX_SIM__
    { "exit",       cmd_exit,           "Terminate uWifi system."           },
    { "abort",       cmd_abort,           "force abort"           },
    #endif
//	{ "sdrv",       cmd_sdrv,           "SSV driver module commands."       },
    { "ifconfig",   cmd_ifconfig,       "Network interface configuration"   },
//    { "route",      cmd_route,          "Routing table manipulation"        },
    { "ping",       cmd_ping,           "ping"                              },
//    { "ttcp",       cmd_ttcp,           "ttcp"                              },
    //{ "dhcpd",       cmd_dhcpd,           "dhcpd"                           },
    //{ "dhcpc",       cmd_dhcpc,           "dhcpc"                          },
    { "iperf3",      cmd_iperf3,         "throughput testing via tcp or udp"},
    { "netapp",     cmd_net_app,        "Network application utilities"     },
    { "netmgr",     cmd_net_mgr,        "Network Management utilities"     },

    { "iw",         cmd_iw,             "Wireless utility"                  },
    { "ctl",       cmd_ctl,           "wi-fi interface control (AP/station on or off)"       },
    { "r",          cmd_read,           "Read SoC"                          },
    { "w",          cmd_write,          "Write SoC"                         },

   // { "pattern",    cmd_pattern,        "Pattern mode"                      },

//	{ "setnetif",   cmd_setnetif,		"SetNetIf Ip Mask Gateway Mac"		},
	//{ "socket-c",   cmd_createsocketclient, "Create Socket client and set server addreass" },
//	{ "52w",        cmd_write_52,          "Write 52 dommand"               },

#ifdef __AP_DEBUG__
   // { "ap"		,   cmd_ap				, "list ap mode info." 				},
   // { "test"		,cmd_test			, "send test frame"					},
#endif

    { "dump",       cmd_dump,           "Do dump."                          },
//	{ "phy-on",		cmd_phy_config,		"Load PHY configuration"			},
//    { "cal",        cmd_cal,			"Calibration Process"				},
#ifdef THROUGHPUT_TEST
    { "sdio",		cmd_sdio_test,		"SDIO throughput test(from HOST)"	},
    { "dut",  		cmd_throughput,		"DUT throughput test(from DUT)"		},
#endif

//    { "phy",        Cmd_Phy,            "phy" },
//    { "delba",  	cmd_wifi_delba,		"DELBA"		                        },

#if CONFIG_STATUS_CHECK
    { "s",     cmd_tcpip_status,    "dump tcp/ip status"                },
#endif
    { "sys",       cmd_sys,           "Components info"       },
#if(OS_TASK_STAT_EN==1)
    {"top",cmd_top,"task profile"},
#endif
#if(CONFIG_IO_BUS_THROUGHPUT_TEST==1)
    {"bus",         cmd_bus_tput, "Bus throughput Test" },
#endif
#if (SDRV_INCLUDE_SPI)
    {"spi", cmd_spi_status,"check spi status"},
#endif
#if ((!defined(__SSV_UNIX_SIM__))&&(APP_MJS))
    {"mjs", cmd_mjpegsd,"mjpeg streaming server"},
#endif
    { NULL, NULL, NULL },
};

