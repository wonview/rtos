#include <config.h>
#include <regs/ssv6200_reg.h>
#include <lwip/sockets.h>
#include <lwip/netif.h>
#include <lwip/ip_addr.h>
#include <lwip/dhcp.h>
#include <lwip/sys.h>
#include <host_global.h>
#include <netapp/net_app.h>
#include "cli_cmd_net.h"
#include <drv/ssv_drv.h>
#include "dev.h"
#include "netmgr/net_mgr.h"

#include <log.h>
//#define __STATIC_MAC_ADDRESS_ASSIGN__






#if 0
static struct netif sg_netif_list[CONFIG_MAX_NETIF];
static struct dhcp sg_dhcp_list[CONFIG_MAX_NETIF];
#endif

extern err_t tcpip_input(struct pbuf *p, struct netif *inp);
extern err_t ethernetif_init(struct netif *netif);




static u8 *ipv4_to_str(u32 ipaddr, u8 *strbuf)
{
    sprintf((void*)strbuf, (void*)"%d.%d.%d.%d", IPV4_ADDR(&ipaddr));
    return strbuf;
}



static void netif_display(struct netif *netif)
{
    u32 bcast;
    assert(netif!=NULL);

    LOG_PRINTF("%s\tLink encap:Ethernet  HWaddr %02x:%02x:%02x:%02x:%02x:%02x\r\n", 
        netif->name,  netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2],
        netif->hwaddr[3], netif->hwaddr[4], netif->hwaddr[5]);
    bcast = (netif->ip_addr.addr & netif->netmask.addr) | ~netif->netmask.addr;
    LOG_PRINTF("\tinet addr:%d.%d.%d.%d  ", IPV4_ADDR(&netif->ip_addr.addr));  
    LOG_PRINTF("Bcast:%d.%d.%d.%d  ", IPV4_ADDR(&bcast));
    LOG_PRINTF("Mask:%d.%d.%d.%d\r\n", IPV4_ADDR(&netif->netmask.addr));

    LOG_PRINTF("\t%s ", ((netif->flags&NETIF_FLAG_UP)? "UP": "DOWN"));
    LOG_PRINTF("%s ", ((netif->flags&NETIF_FLAG_BROADCAST)? "BROADCAST": "POINT-TO-POINT"));
    LOG_PRINTF("%s ", ((netif->flags&NETIF_FLAG_LINK_UP)? "RUNNING": "LINK-DOWN"));
    LOG_PRINTF(" MTU:%d  GW:%d.%d.%d.%d\r\n", netif->mtu, IPV4_ADDR(&netif->gw.addr));
    LOG_PRINTF("\r\n");
    
}


#if 0
static struct netif *netif_malloc(char *name)
{
    s32 i;
    for(i=0; i<CONFIG_MAX_NETIF; i++) {
        if (sg_netif_mask & (1<<i))
            continue;
        sg_netif_mask |= (1<<i);
        memset((void *)&sg_netif_list[i], 0, sizeof(struct netif));
        strcpy(sg_netif_list[i].name, name);
        return &(sg_netif_list[i]);
    }
    return NULL;
}

static void netif_free(struct netif *netif)
{
    s32 i;
    for(i=0; i<CONFIG_MAX_NETIF; i++) {
        if (netif != &sg_netif_list[i])
            continue;
        sg_netif_mask &= ~(1<<i);
    }
}
#endif



void cmd_ifconfig(s32 argc, char *argv[])
{
    extern struct netif *netif_list;
    struct netif *netif;
    struct ip_addr ipaddr, netmask, gw;
    s32 res, mac[6];
   
    
    /**
         * Usage: ifconfig <name> [up|down|ip-addr] [netmask xxx.xxx.xxx.xxx] [hw xx:xx:xx:xx:xx.xx]
         */
    if (argc == 1) {
        for(netif=netif_list; netif!=NULL; netif=netif->next)
            netif_display(netif);
        return;
    }
    else if (argc == 2 || argc == 3) {
        netif = netif_find(argv[1]);
        if (netif == NULL) {
            LOG_PRINTF("ifconfig: unknown device %s\r\n", argv[1]);
            return;
        }
        if (argc == 2) {
            /**
                       * Display information of the specified network interface 
                       */
            netif_display(netif);            
            return;              
        }
        else if (strcmp(argv[2], "up") == 0) {
            /* Interface up */
          netif_set_up(netif);
        }
        else if (strcmp(argv[2], "down") == 0) {
            /* Interface down */
            netif_set_down(netif);
        }
        else LOG_PRINTF("ifconfig: unknown parameter %s\r\n", argv[2]);
        return;
    }

    if (((argc==5)||(argc==7)) && strcmp(argv[3], "netmask")==0) {
        ipaddr.addr = inet_addr(argv[2]);
        netmask.addr = inet_addr(argv[4]);
        netif = netif_find(argv[1]);
        if (netif == NULL) {
            LOG_PRINTF("ifconfig: dev %s not found !\r\n", argv[1]);
            return;
        }

        if (argc==7 && strcmp(argv[5], "hw")==0) {
            res = sscanf(argv[6], "%02x:%02x:%02x:%02x:%02x:%02x",
                mac, mac+1, mac+2, mac+3, mac+4, mac+5); 
            if (res != 6) {
                LOG_PRINTF("ifconfig: invalid parameter %s\r\n", argv[5]);
                return;
            }
            netif->hwaddr[0] = mac[0];
            netif->hwaddr[1] = mac[1];
            netif->hwaddr[2] = mac[2];
            netif->hwaddr[3] = mac[3];
            netif->hwaddr[4] = mac[4];
            netif->hwaddr[5] = mac[5];
        }
        gw.addr = 0;
        netif_add(netif, &ipaddr, &netmask, &gw, NULL, ethernetif_init, tcpip_input);

        return;
    }

    LOG_PRINTF("\nifconfig: invalid parameter(s)!!\n\n");
}


void cmd_route(s32 argc, char *argv[])
{
    extern struct netif *netif_list;
    extern struct netif *netif_default;
    struct netif *netif;
    struct ip_addr gw, subnet;
    u8 bufstr[20];

    /**
         *  route add default  xxx.xxx.xxx.xxx
         */
    if (argc==4 && !strcmp(argv[1], "add") && !strcmp(argv[2], "default")) {
        gw.addr = inet_addr(argv[3]);
        for(netif = netif_list; netif != NULL; netif = netif->next) {
            if (!ip_addr_netcmp(&gw, &netif->ip_addr, &netif->netmask))
                continue;
            netif_set_gw(netif, &gw);
            netif_set_default(netif);
            return;
        }
        LOG_PRINTF("route: unknown gateway %s\n", argv[3]);
        return;        
    }


    LOG_PRINTF("%-20s %-20s %-20s %-10s\r\n", "Destination", 
    "Gateway", "Netmask", "Iface");

    for(netif = netif_list; netif != NULL; netif = netif->next) {
      if (! netif_is_up(netif))
          continue;

        subnet.addr = netif->ip_addr.addr & netif->netmask.addr;
        LOG_PRINTF("%-20s ", ipv4_to_str(subnet.addr, bufstr));
        LOG_PRINTF("%-20s ", "*");
        LOG_PRINTF("%-20s ", ipv4_to_str(netif->netmask.addr, bufstr));
        LOG_PRINTF("%-10s\r\n", netif->name);

    }

    if (netif_default != NULL) {
        LOG_PRINTF("%-20s ", "default");
        LOG_PRINTF("%-20s ", ipv4_to_str(netif_default->gw.addr, bufstr));
        LOG_PRINTF("%-20s ", "0.0.0.0");
        LOG_PRINTF("%-10s\r\n", netif_default->name);
    }

}



void cmd_ping(s32 argc, char *argv[])
{
    s32 res;
    res = net_app_run(argc, argv);
    if (res < 0)
        LOG_PRINTF("ping failure !!\n");
}



void cmd_ttcp(s32 argc, char *argv[])
{
    s32 res;
    res = net_app_run(argc, argv);
    if (res < 0)
        LOG_PRINTF("ttcp failure !!\n");
}




void cmd_dhcpd(s32 argc, char *argv[])
{
    int ret = -1;
    if (argc == 2) {
        if (strcmp(argv[1], "on") == 0) {
            ret = netmgr_dhcpd_set(true);
            if (ret == 0)
                LOG_PRINTF("dhcpd on ok !\n");
            else
                LOG_PRINTF("dhcpd on failure !\n");
            return;
        }
        if (strcmp(argv[1], "off") == 0) {
            ret = netmgr_dhcpd_set(false);
            if (ret == 0)
                LOG_PRINTF("dhcpd off ok !\n");
            else
                LOG_PRINTF("dhcpd off failure !\n");
            return;
        }
        if (strcmp(argv[1], "info") == 0) {
            dhcpd_lease_show();
            return;
        }        
    }
}

void cmd_dhcpc(s32 argc, char *argv[])
{
    int ret = -1;
    if (argc == 2) {
        if (strcmp(argv[1], "on") == 0) {
            ret = netmgr_dhcpc_set(true);
            if (ret == 0)
                LOG_PRINTF("dhcpc on ok !\n");
            else
                LOG_PRINTF("dhcpc on failure !\n");
            return;
        }
        if (strcmp(argv[1], "off") == 0) {
            ret = netmgr_dhcpc_set(false);
            if (ret == 0)
                LOG_PRINTF("dhcpc off ok !\n");
            else
                LOG_PRINTF("dhcpc off failure !\n");
            return;
        }
    }
}

void cmd_iperf3(s32 argc, char *argv[])
{
    s32 res;
    res = net_app_run(argc, argv);
    if (res < 0)
        LOG_PRINTF("iperf failure !!!\n");
}

void cmd_net_app(s32 argc, char *argv[])
{
    if (argc==2 && strcmp(argv[1], "show")==0)
        net_app_show();

}

void cmd_net_mgr(s32 argc, char *argv[])
{
    if (argc==2 && strcmp(argv[1], "show")==0)
        netmgr_show();
#ifdef  NET_MGR_AUTO_JOIN
    else if (argc==3 && strcmp(argv[1], "remove")==0)
        netmgr_apinfo_remove(argv[2]);
#endif

}






#if 0

#ifdef __STATIC_MAC_ADDRESS_ASSIGN__
static char IP_ADDR[4][4] = {"192","168","0","60"};
static char IP_MASK[4][4] = {"255","255","255","0"};
static char IP_GATEWAY[4][4] = {"192","168","0","250"};
//u8 HOST_MAC_ADDR[6] = {0xD0,0x11,0x22,0x33,0x44,0x55};
//#else//__STATIC_MAC_ADDRESS_ASSIGN__
//u8 HOST_MAC_ADDR[6];
#endif//__STATIC_MAC_ADDRESS_ASSIGN__

void cmd_setnetif(s32 argc, char *argv[])
{
	//test for netif_add
	static struct netif wlan0;
	struct ip_addr ipaddr, netmask, gw;
	int mscnt =0;
	
    {
	    //s32 i;
	   /* for(i=0;i<6;i++)
	    {
			wlan0.hwaddr[ i ] = HOST_MAC_ADDR[i];
	    }*/
       u32 STA_MAC_0;
       u32 STA_MAC_1;

       STA_MAC_0 = ssv6xxx_drv_read_reg(ADR_STA_MAC_0); 
       STA_MAC_1 = ssv6xxx_drv_read_reg(ADR_STA_MAC_1);
       
       MEMCPY(&(wlan0.hwaddr[0]),(u8 *)&STA_MAC_0,4);
       MEMCPY(&(wlan0.hwaddr[4]),(u8 *)&STA_MAC_1,2);
	}
    MEMCPY(&wlan0.name,"wlan0", sizeof("wlan0"));
    LOG_PRINTF("%02x:%02x:%02x:%02x:%02x:%02x\n", 
        wlan0.hwaddr[0], wlan0.hwaddr[1], wlan0.hwaddr[2],
        wlan0.hwaddr[3], wlan0.hwaddr[4], wlan0.hwaddr[5]);

#ifdef __STATIC_MAC_ADDRESS_ASSIGN__
		IP4_ADDR(&ipaddr,  192,168,25,1);	
		IP4_ADDR(&netmask,  255,255,255,0);
		IP4_ADDR(&gw, 192,168,25,1);
		netif_add(&wlan0, &ipaddr, &netmask, &gw, NULL, ethernetif_init, tcpip_input);
		netif_set_default(&wlan0);
		netif_set_up(&wlan0);
#else//__STATIC_MAC_ADDRESS_ASSIGN__
	{
 
        //AP mode use static MAC address
        if (gDeviceInfo->hw_mode == SSV6XXX_HWM_AP)
        {
                netif_set_down(&wlan0);
    		netif_remove(&wlan0);
        	IP4_ADDR(&ipaddr,  192,168,25,1);	
    		IP4_ADDR(&netmask,  255,255,255,0);
    		IP4_ADDR(&gw, 192,168,25,1);
    		netif_add(&wlan0, &ipaddr, &netmask, &gw, NULL, ethernetif_init, tcpip_input);
    		netif_set_default(&wlan0);
    		netif_set_up(&wlan0);
        }
        else
        {
            
            s32 retry_count = 15;
    		netif_set_down(&wlan0);
    		netif_remove(&wlan0);
    		IP4_ADDR(&ipaddr,0,0,0,0);	
    		IP4_ADDR(&netmask,0,0,0,0);
    		IP4_ADDR(&gw,0,0,0,0);

    		netif_add(&wlan0, &ipaddr, &netmask, &gw, NULL, ethernetif_init, tcpip_input);
    		netif_set_default(&wlan0);
    		netif_set_up(&wlan0);
    		// sys_msleep(DHCP_FINE_TIMER_MSECS);
    		dhcp_start(&wlan0);

    		while (wlan0.ip_addr.addr==0 && retry_count) {
                LOG_PRINTF("Waiting for IP ready. %d\n", retry_count--);
    			sys_msleep(DHCP_FINE_TIMER_MSECS);
    			dhcp_fine_tmr();
    			mscnt += DHCP_FINE_TIMER_MSECS;
    			if (mscnt >= DHCP_COARSE_TIMER_SECS*1000) {
    				dhcp_coarse_tmr();
    				mscnt = 0;
    			}
    		}
    		if (wlan0.ip_addr.addr==0)
    		{
    			netif_set_down(&wlan0);
    			netif_remove(&wlan0);
    		}
        }
	}
#endif//__STATIC_MAC_ADDRESS_ASSIGN__
}




/**
* void sim_link_status_change() - function of indiciating of L3/4 status.
*
* This function shall be registered to netif structure by calling 
* netif_set_status_callback() API so that L3/4 status change (got IP or
* not) will be notified.
*/
static void sim_link_status_change(struct netif *netif)
{
    if (netif->flags & NETIF_FLAG_UP)
        g_sim_net_up = 1;
    else g_sim_net_up = 0;

    LOG_PRINTF("%s(): l34 %s !\n", 
    ((g_sim_net_up==1)? "up": "down"));
}



/**
* void sim_link_change() - function of indicating link-layer status.
*
* This function shall be registered to netif structure by calling 
* netif_set_link_callback() API so that any link-layer status
* change (link up/down) will be notified.
*/
static void sim_link_change(struct netif *netif)
{
    if (netif->flags & NETIF_FLAG_LINK_UP)
        g_sim_link_up = 1;
    else g_sim_link_up = 0;

    LOG_PRINTF("%s(): link %s !\n", 
    ((g_sim_link_up==1)? "up": "down"));

    /* Static IP configuration: */
//    if (netif->dhcp == NULL)
//        netif_set_up(netif);
}


void sim_set_link_status(link_status lstatus)
{
	LOG_PRINTF("lstatus=%d\n",lstatus);
    if (lstatus == LINK_UP)
        netif_set_link_up(&sg_netif_list[0]);
    else netif_set_link_down(&sg_netif_list[0]);
}


/**
* void sim_net_cfg() - TCP/IP & network interface default configuration.
*
*/
void sim_net_cfg(void)
{
    struct ip_addr ipaddr, netmask, gw;
    
    ssv6xxx_memset((void *)sg_netif_list, 0, 
        sizeof(struct netif)*CONFIG_MAX_NETIF);
    g_sim_net_up = 0;
    g_sim_link_up = 0;



    /* Create a wlan0 network interface */
    strcpy(sg_netif_list[0].name, "wlan0");
    ipaddr.addr=netmask.addr=gw.addr = 0;
#if 0
	/*for(i=0;i<6;i++)
	{		

		sg_netif_list[0].hwaddr[ i ] = HOST_MAC_ADDR[i];
        
        
	}*/
    {
        u32 STA_MAC_0= ssv6xxx_drv_read_reg(ADR_STA_MAC_0);
        u32 STA_MAC_1= ssv6xxx_drv_read_reg(ADR_STA_MAC_1)	  
        memcpy(&(wlan0.hwaddr[0]),(u8 *)&STA_MAC_0,4);
        memcpy(&(wlan0.hwaddr[4]),(u8 *)&STA_MAC_1,2); 
    }
	
#endif
    netif_add(&sg_netif_list[0], &ipaddr, &netmask, &gw, 
        NULL, ethernetif_init, tcpip_input);


    /* By default, we enable DHCP unless disable it by issue dhcp command */
    dhcp_set_struct(&sg_netif_list[0], &sg_dhcp_list[0]);
    sg_dhcp_list[0].state = DHCP_INIT;
    
    /* Register link change callback function */
    netif_set_status_callback(&sg_netif_list[0], sim_link_status_change);
    netif_set_link_callback(&sg_netif_list[0], sim_link_change);

	
}

#endif




