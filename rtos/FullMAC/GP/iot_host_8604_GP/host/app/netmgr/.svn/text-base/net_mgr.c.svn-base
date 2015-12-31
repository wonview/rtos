#include <config.h>
#include <regs/ssv6200_reg.h>
#include <lwip/sockets.h>
#include <lwip/netif.h>
#include <lwip/ip_addr.h>
#include <lwip/dhcp.h>
#include <lwip/sys.h>
#include <drv/ssv_drv.h>
#include <host_config.h>
#include "udhcp/udhcp_common.h"
#include "udhcp/dhcpc.h"
#include "udhcp/dhcpd.h"

#include "net_mgr.h"

#include <log.h>
#include <rtos.h>
#include "lwip/udp.h"
#include "lwip/api.h"
static struct netif wlan0;

static bool s_dhcpd_enable = true;
static bool s_dhcpc_enable = true;

static bool s_dhcpd_status = false;
static bool s_dhcpc_status = false;

static bool g_switch_join_cfg_b = false;
static wifi_sta_join_cfg g_join_cfg_data;

static bool g_netif_link_up = false;
static bool g_netif_status_up = false;

extern err_t tcpip_input(struct pbuf *p, struct netif *inp);
extern err_t ethernetif_init(struct netif *netif);
extern struct ssv6xxx_ieee80211_bss * ssv6xxx_wifi_find_ap_ssid(char *ssid);

static void netmgr_net_init(char hwmac[6]);
#if !NET_MGR_NO_SYS
static void netmgr_task(void *arg);
#endif
static void netmgr_wifi_reg_event(void);
static void netmgr_wifi_event_cb(u32 evt_id, void *data, s32 len);

#if !NET_MGR_NO_SYS
struct task_info_st st_netmgr_task[] =
{
    { "netmgr_task",  (OsMsgQ)0, 4, OS_NETMGR_TASK_PRIO, NETMGR_STACK_SIZE, NULL, netmgr_task   },
};
#endif

static bool g_wifi_is_joining_b = false; //
struct resp_evt_result *sconfig_done_cpy=NULL;

#ifdef  NET_MGR_AUTO_JOIN
typedef struct st_user_ap_info
{
    bool valid;
    struct cfg_80211_ssid ssid;
    char password[MAX_PASSWD_LEN+1];
    int join_times;
}user_ap_info;

static user_ap_info g_user_ap_info[NET_MGR_USER_AP_COUNT];

static struct ssv6xxx_ieee80211_bss *g_ap_list_p = NULL;

void netmgr_apinfo_clear();
void netmgr_apinfo_remove(char *ssid);
user_ap_info * netmgr_apinfo_find(char *ssid);
user_ap_info * netmgr_apinfo_find_best(struct ssv6xxx_ieee80211_bss * ap_list_p, int count);
void netmgr_apinfo_save();
void netmgr_apinfo_set(user_ap_info *ap_info, bool valid);
static int netmgr_apinfo_autojoin(user_ap_info *ap_info);
void netmgr_apinfo_show();
#endif

int netmgr_wifi_sconfig_broadcast(u8 *resp_data, u32 len, bool IsUDP,u32 port);

typedef struct st_wifi_sec_info
{
    char *sec_name;
    char dfl_password[MAX_PASSWD_LEN+1];
}wifi_sec_info;

const wifi_sec_info g_sec_info[SSV6XXX_SEC_MAX] =
{
    {"open",   ""},                              // WIFI_SEC_NONE
    {"wep40",  {0x31,0x32,0x33,0x34,0x35,0x00,}}, // WIFI_SEC_WEP_40
    {"wep104", "0123456789012"},                 // WIFI_SEC_WEP_104
    {"wpa",    "secret00"},                      // WIFI_SEC_WPA_PSK
    {"wpa2",   "secret00"},                      // WIFI_SEC_WPA2_PSK
    {"wps",    ""}, // WIFI_SEC_WPS

};

static void netif_link_change_cb(struct netif *netif)
{
    if (netif->flags & NETIF_FLAG_LINK_UP)
        g_netif_link_up = 1;
    else
        g_netif_link_up = 0;
    LOG_DEBUGF(LOG_L4_NETMGR, ("wlan0: link %s !\r\n", ((g_netif_link_up==1)? "up": "down")));
}

static void netif_status_change_cb(struct netif *netif)
{
    if (netif->flags & NETIF_FLAG_UP)
        g_netif_status_up = 1;
    else
        g_netif_status_up = 0;
    
    LOG_DEBUGF(LOG_L4_NETMGR, ("wlan0: status %s !\r\n", ((g_netif_status_up==1)? "up": "down")));
}

void netmgr_init()
{
    u32 STA_MAC_0;
    u32 STA_MAC_1;
    char mac[6];

#if !NET_MGR_NO_SYS
    OsMsgQ *msgq = NULL;
    s32 qsize = 0;
#endif
    STA_MAC_0 = ssv6xxx_drv_read_reg(ADR_STA_MAC_0);
    STA_MAC_1 = ssv6xxx_drv_read_reg(ADR_STA_MAC_1);

    MEMCPY((void *)mac,(u8 *)&STA_MAC_0,4);
    MEMCPY((void *)(mac + 4),(u8 *)&STA_MAC_1,2);

    netmgr_net_init(mac);

    #ifdef NET_MGR_AUTO_JOIN
    netmgr_apinfo_clear();
    #endif

    g_wifi_is_joining_b = false;

    #if !NET_MGR_NO_SYS
    msgq = &st_netmgr_task[0].qevt;
    qsize = (s32)st_netmgr_task[0].qlength;
    if (OS_MsgQCreate(msgq, qsize) != OS_SUCCESS)
    {
        LOG_PRINTF("OS_MsgQCreate faild\r\n");
        return;
    }

    if (OS_TaskCreate(st_netmgr_task[0].task_func,
                  st_netmgr_task[0].task_name,
                  st_netmgr_task[0].stack_size<<4,
                  NULL,
                  st_netmgr_task[0].prio,
                  NULL) != OS_SUCCESS)
    {
        LOG_PRINTF("OS_TaskCreate faild\r\n");
        return;
    }
    #endif

    netmgr_wifi_reg_event();
}

static void netmgr_net_init(char hwmac[6])
{
	struct ip_addr ipaddr, netmask, gw;
    struct netif * pwlan = NULL;

    /* net if init */
    pwlan = netif_find(WLAN_IFNAME);
    if (pwlan)
    {
        netif_remove(pwlan);
    }

    MEMCPY((void *)(wlan0.hwaddr), hwmac, 6);
    MEMCPY((void *)(wlan0.name),WLAN_IFNAME, 6);

    /* if sta mode and dhcpc enable, set ip is 0.0.0.0, otherwise is default ip */
    if (netmgr_wifi_check_sta() && s_dhcpc_enable)
    {
        ip_addr_set_zero(&ipaddr);
        ip_addr_set_zero(&netmask);
        ip_addr_set_zero(&gw);
        netif_add(&wlan0, &ipaddr, &netmask, &gw, NULL, ethernetif_init, tcpip_input);
        netif_set_default(&wlan0);
    }
    else
    {
        ipaddr.addr = ipaddr_addr(DEFAULT_IPADDR);
        netmask.addr = ipaddr_addr(DEFAULT_SUBNET);
        gw.addr = ipaddr_addr(DEFAULT_GATEWAY);

        netif_add(&wlan0, &ipaddr, &netmask, &gw, NULL, ethernetif_init, tcpip_input);
        netif_set_default(&wlan0);
    }

    netmgr_netif_link_set(false);

    /* if ap mode and dhcpd enable, set ip is default ip and set netif up */
    if (netmgr_wifi_check_ap() && s_dhcpd_enable)
    {
        netmgr_netif_link_set(true);
        netmgr_netif_status_set(true);
        netmgr_dhcpd_set(true);
    }

    /* Register link change callback function */
    netif_set_status_callback(&wlan0, netif_status_change_cb);
    netif_set_link_callback(&wlan0, netif_link_change_cb);

    LOG_PRINTF("MAC[%02x:%02x:%02x:%02x:%02x:%02x]\r\n",
        wlan0.hwaddr[0], wlan0.hwaddr[1], wlan0.hwaddr[2],
        wlan0.hwaddr[3], wlan0.hwaddr[4], wlan0.hwaddr[5]);
}

void netmgr_netif_status_set(bool on)
{
    LOG_DEBUGF(LOG_L4_NETMGR, ("L3 Link %s\r\n",(on==true?"ON":"OFF")));

    if (on)
    {
        netif_set_up(&wlan0);
    }
    else
    {
        netif_set_down(&wlan0);
    }
}

bool netmgr_netif_status_get()
{
    struct netif *netif = &wlan0;

    if (netif->flags & NETIF_FLAG_UP)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void netmgr_netif_link_set(bool on)
{
    LOG_DEBUGF(LOG_L4_NETMGR, ("L2 Link %s\r\n",(on==true?"ON":"OFF")));

    if (on)
    {
        netif_set_link_up(&wlan0);
    }
    else
    {
        netif_set_link_down(&wlan0);
    }
}

int netmgr_ipinfo_get(char *ifname, ipinfo *info)
{
    struct netif * pwlan = NULL;

    pwlan = netif_find(ifname);
    if (!pwlan)
    {
        LOG_PRINTF("%s is not exist\r\n", ifname);
        return -1;
    }

    info->ipv4 = pwlan->ip_addr.addr;
    info->netmask = pwlan->netmask.addr;
    info->gateway = pwlan->gw.addr;
    info->dns = 0;
    return 0;
}

int netmgr_ipinfo_set(char *ifname, ipinfo *info)
{
    struct netif * pwlan = NULL;
    ip_addr_t ipaddr, netmask, gw;

    pwlan = netif_find(ifname);
    if (!pwlan)
    {
        LOG_PRINTF("%s is not exist\r\n", ifname);
        return -1;
    }

    ipaddr.addr = info->ipv4;
    netmask.addr = info->netmask;
    gw.addr = info->gateway;

    netif_set_ipaddr(pwlan, &ipaddr);
    netif_set_netmask(pwlan, &netmask);
    netif_set_gw(pwlan, &gw);

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_ipinfo_set\r\n"));

    return 0;
}

int netmgr_hwmac_get(char *ifname, char mac[6])
{
    struct netif * pwlan = NULL;

    pwlan = netif_find(ifname);
    if (!pwlan)
    {
        LOG_PRINTF("%s is not exist\r\n", ifname);
        return -1;
    }

    MEMCPY((void *)mac, pwlan->hwaddr, 6);

    return 0;
}

int netmgr_dhcpd_default()
{
    dhcps_info if_dhcps;

    /* dhcps info init */
    if_dhcps.start_ip = ipaddr_addr(DEFAULT_DHCP_START_IP);
    if_dhcps.end_ip = ipaddr_addr(DEFAULT_DHCP_END_IP);
    if_dhcps.max_leases = DEFAULT_DHCP_MAX_LEASES;

    if_dhcps.subnet = ipaddr_addr(DEFAULT_SUBNET);
    if_dhcps.gw = ipaddr_addr(DEFAULT_GATEWAY);
    if_dhcps.dns = ipaddr_addr(DEFAULT_DNS);

    if_dhcps.auto_time = DEFAULT_DHCP_AUTO_TIME;
    if_dhcps.decline_time = DEFAULT_DHCP_DECLINE_TIME;
    if_dhcps.conflict_time = DEFAULT_DHCP_CONFLICT_TIME;
    if_dhcps.offer_time = DEFAULT_DHCP_OFFER_TIME;
    if_dhcps.min_lease_sec = DEFAULT_DHCP_MIN_LEASE_SEC;

    if (dhcps_set_info_api(&if_dhcps) != 0)
    {
        LOG_PRINTF("dhcps_set_info faild\r\n");
        return -1;
    }

    return 0;
}

int netmgr_dhcpd_set(bool enable)
{

    struct ip_addr ipaddr, netmask, gw;
    struct netif *pwlan = NULL;
    int ret = 0;

    pwlan = netif_find(WLAN_IFNAME);
    if (!pwlan)
    {
        LOG_PRINTF("%s error\r\n", WLAN_IFNAME);
        return -1;
    }
    
    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_dhcpd_set %d !!\r\n",enable));

    if (enable)
    {
        ipaddr.addr = ipaddr_addr(DEFAULT_IPADDR);
        netmask.addr = ipaddr_addr(DEFAULT_SUBNET);
        gw.addr = ipaddr_addr(DEFAULT_GATEWAY);
        netif_set_addr(pwlan, &ipaddr, &netmask, &gw);
        netmgr_netif_link_set(enable);
        netmgr_netif_status_set(enable);
        netmgr_dhcpd_default();
        ret = udhcpd_start();
        s_dhcpd_status = true;
    }
    else
    {
        ret = udhcpd_stop();
        s_dhcpd_status = false;
    }

    return ret;
}

int netmgr_dhcpc_set(bool enable)
{
    struct ip_addr ipaddr, netmask, gw;
    struct netif *pwlan = NULL;

    pwlan = netif_find(WLAN_IFNAME);
    if (!pwlan)
    {
        LOG_PRINTF("%s error\r\n", WLAN_IFNAME);
        return -1;
    }

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_dhcpc_set %d !!\r\n",enable));

    if (enable)
    {
        ip_addr_set_zero(&ipaddr);
        ip_addr_set_zero(&netmask);
        ip_addr_set_zero(&gw);
    	netif_set_addr(pwlan, &ipaddr, &netmask, &gw);
        dhcp_stop(pwlan);
    	dhcp_start(pwlan);
        s_dhcpc_status = true;
    }
    else
    {
        dhcp_stop(pwlan);
        ipaddr.addr = ipaddr_addr(DEFAULT_IPADDR);
        netmask.addr = ipaddr_addr(DEFAULT_SUBNET);
        gw.addr = ipaddr_addr(DEFAULT_GATEWAY);
        netif_set_addr(pwlan, &ipaddr, &netmask, &gw);
        s_dhcpc_status = false;
    }

    return 0;
}

int netmgr_dhcps_info_set(dhcps_info *if_dhcps)
{
    if (dhcps_set_info_api(if_dhcps) < 0)
    {
        return -1;
    }

    return 0;
}

int netmgr_dhcp_status_get(bool *dhcpd_status, bool *dhcpc_status)
{
    if (!dhcpd_status || !dhcpc_status)
    {
        return -1;
    }

    *dhcpc_status = s_dhcpc_status;
    *dhcpd_status = s_dhcpd_status;

    return 0;
}

int netmgr_dhcp_ipmac_get(dhcpdipmac *ipmac, int *size_count)
{
    int i = 0;
    struct dyn_lease *lease = NULL;
    int ret = 0;

    if (!ipmac || !size_count || (*size_count <= 0))
    {
        return -1;
    }

    lease = MALLOC(sizeof(struct dyn_lease) * (*size_count));
    if (!lease)
    {
        return -1;
    }

    ret = dhcpd_lease_get_api(lease, size_count);

    if (ret == 0)
    {
        for (i = 0; i < *size_count; i++)
        {
           ((dhcpdipmac *) (ipmac + i))->ip = lease[i].lease_nip;
            MEMCPY((void*)(((dhcpdipmac *) (ipmac + i))->mac), (void*)(lease[i].lease_mac), 6);
        }
    }

    FREE((void*)lease);

    return 0;
}

int netmgr_dhcp_getip_bymac(u8 *mac, u32 *ipaddr)
{
    dhcpdipmac *ipmac = NULL;
    int size_count = DEFAULT_DHCP_MAX_LEASES;
    int i;

    //PRINTF("netmgr_dhcd_getip_bymac MAC:[%02x:%02x:%02x:%02x:%02x:%02x] \r\n",
    //    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );

    ipmac = (dhcpdipmac *)MALLOC(DEFAULT_DHCP_MAX_LEASES * sizeof(dhcpdipmac));
    if (ipmac == NULL)
    {
        return -1;
    }
    MEMSET((void *)ipmac, 0, DEFAULT_DHCP_MAX_LEASES * sizeof(dhcpdipmac));

    if (netmgr_dhcp_ipmac_get(ipmac, &size_count)){
    //    PRINTF("netmgr_dhcd_ipmac_get return failure \r\n");
        FREE(ipmac);
        return -1;
    }

    for (i=0; i<size_count; i++){
        if (!MEMCMP(ipmac[i].mac, mac, 6)){
            *ipaddr = ipmac[i].ip;
   //         PRINTF("netmgr_dhcd_getip_bymac shot ipaddr:0x%X \r\n", ntohl(*ipaddr));
            FREE(ipmac);
            return 0;
        }
    }

   // PRINTF("netmgr_dhcd_getip_bymac get ipaddr failure\r\n");
    FREE(ipmac);
    return -1;
}

extern s8_t etharp_unicast (u8 *dst_mac, ip_addr_t *ipaddr);
int netmgr_send_arp_unicast (u8 *dst_mac)
{
    ip_addr_t ipaddr;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_send_arp_unicast %02X:%02X:%02X:%02X:%02X:%02X !!\r\n",dst_mac[0],
        dst_mac[1],dst_mac[2],dst_mac[3],dst_mac[4],dst_mac[5]));

    if (!netmgr_dhcp_getip_bymac(dst_mac, (u32 *)&ipaddr)){
        return etharp_unicast(dst_mac, &ipaddr);
    }
    return -1;
}

int netmgr_wifi_mode_get(wifi_mode *mode, bool *status)
{
    Ap_sta_status *info = NULL;

    if (!mode || !status)
    {
        return -1;
    }

    info = (Ap_sta_status *)MALLOC(sizeof(Ap_sta_status));
    MEMSET((void *)info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);

    *mode = info->operate;
    *status = info->status ? true : false;

    #if NET_MGR_DEBUG
    if(info->status)
        LOG_PRINTF("status:ON\r\n");
    else
        LOG_PRINTF("status:OFF\r\n");
    if(SSV6XXX_HWM_STA==info->operate)
    {
        LOG_PRINTF("Mode:Station\r\n");
    }
    else
    {
        LOG_PRINTF("Mode:AP\r\n");
        LOG_PRINTF("SSID:%s\r\n",info->ap.ssid.ssid);
        LOG_PRINTF("Station number:%d\r\n",info->ap.stanum);
    }

    LOG_PRINTF("Mac addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
        info->ap.selfmac[0],
        info->ap.selfmac[1],
        info->ap.selfmac[2],
        info->ap.selfmac[3],
        info->ap.selfmac[4],
        info->ap.selfmac[5]);

    #endif

    FREE(info);

    return 0;
}

int netmgr_wifi_info_get(Ap_sta_status *info)
{
    if (info == NULL)
    {
        return -1;
    }

    MEMSET(info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);

#if NET_MGR_DEBUG
    if(info->status)
        LOG_PRINTF("status:ON\r\n");
    else
        LOG_PRINTF("status:OFF\r\n");
    if(SSV6XXX_HWM_STA==info->operate)
    {
        LOG_PRINTF("Mode:Station\r\n");
    }
    else
    {
        LOG_PRINTF("Mode:AP\r\n");
        LOG_PRINTF("SSID:%s\r\n",info->ap.ssid.ssid);
        LOG_PRINTF("Station number:%d\r\n",info->ap.stanum);
    }

    LOG_PRINTF("Mac addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
        info->ap.selfmac[0],
        info->ap.selfmac[1],
        info->ap.selfmac[2],
        info->ap.selfmac[3],
        info->ap.selfmac[4],
        info->ap.selfmac[5]);

#endif

    return 0;
}
bool netmgr_wifi_check_sconfig()
{
    Ap_sta_status *info = NULL;
    bool bRet = false;

    info = (Ap_sta_status *)OS_MemAlloc(sizeof(Ap_sta_status));
    if (info == NULL)
    {
        return false;
    }

    OS_MemSET((void *)info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);

    if ((info->operate == SSV6XXX_HWM_SCONFIG) && (info->status))
    {
         bRet = true;
    }

    OS_MemFree(info);

    return bRet;
}

bool netmgr_wifi_check_sta()
{
    Ap_sta_status *info = NULL;
    bool bRet = false;

    info = (Ap_sta_status *)MALLOC(sizeof(Ap_sta_status));
    if (info == NULL)
    {
        return false;
    }

    MEMSET((void *)info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);

    if ((info->operate == SSV6XXX_HWM_STA) && (info->status))
    {
         bRet = true;
    }

    FREE(info);

    return bRet;
}

bool netmgr_wifi_check_ap()
{
    Ap_sta_status *info = NULL;
    bool bRet = false;

    info = (Ap_sta_status *)MALLOC(sizeof(Ap_sta_status));
    if (info == NULL)
    {
        return false;
    }

    MEMSET((void *)info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);

    if ((info->operate == SSV6XXX_HWM_AP) && (info->status))
    {
         bRet = true;
    }

    FREE(info);

    return bRet;
}

bool netmgr_wifi_check_sta_connected()
{
    Ap_sta_status *info = NULL;
    bool bRet = false;

    info = (Ap_sta_status *)MALLOC(sizeof(Ap_sta_status));
    if (info == NULL)
    {
        return false;
    }

    MEMSET((void *)info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);

    if ((info->operate == SSV6XXX_HWM_STA) && (info->status) && (info->u.station.apinfo.status == CONNECT))
    {
        bRet = true;
    }

    FREE(info);

    return bRet;
}

int netmgr_wifi_sconfig_async(u16 channel_mask)
{
    #if !NET_MGR_NO_SYS
    MsgEvent *msg_evt = NULL;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_sconfig_async %x \r\n",channel_mask));

    if (!netmgr_wifi_check_sconfig())
    {
        LOG_PRINTF("netmgr_wifi_sonfig_async: mode error.\r\n");
        return -1;
    }

    msg_evt = msg_evt_alloc();
    ASSERT(msg_evt);
    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
    msg_evt->MsgData = MSG_SCONFIG_REQ;
    msg_evt->MsgData1 = (u32)(channel_mask);
    msg_evt->MsgData2 = 0;
    msg_evt->MsgData3 = 0;

    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
    #else
    netmgr_wifi_sconfig(channel_mask, 0, 0);
    #endif
    return 0;
}

int netmgr_wifi_scan_async(u16 channel_mask, char *ssids[], int ssids_count)
{
    #if !NET_MGR_NO_SYS
    MsgEvent *msg_evt = NULL;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_scan_async %x \r\n",channel_mask));

    if (!netmgr_wifi_check_sta())
    {
        LOG_PRINTF("netmgr_wifi_scan_async: mode error.\r\n");
        return -1;
    }

    msg_evt = msg_evt_alloc();
    ASSERT(msg_evt);
    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
    msg_evt->MsgData = MSG_SCAN_REQ;
    msg_evt->MsgData1 = (u32)(channel_mask);
    msg_evt->MsgData2 = (u32)(ssids);
    msg_evt->MsgData3 = (u32)(ssids_count);

    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
    #else
    netmgr_wifi_scan(channel_mask, 0, 0);
    #endif
    return 0;
}

int netmgr_wifi_join_async(wifi_sta_join_cfg *join_cfg)
{
    #if !NET_MGR_NO_SYS
    MsgEvent *msg_evt = NULL;
    wifi_sta_join_cfg *join_cfg_msg = NULL;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_join_async \r\n"));

    if (!netmgr_wifi_check_sta())
    {
        LOG_PRINTF("netmgr_wifi_join_async: mode error.\r\n");
        return -1;
    }

    msg_evt = msg_evt_alloc();
    ASSERT(msg_evt);
    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
    msg_evt->MsgData = MSG_JOIN_REQ;

    if (join_cfg)
    {
        join_cfg_msg = MALLOC(sizeof(wifi_sta_join_cfg));
        MEMCPY((void * )join_cfg_msg, (void * )join_cfg, sizeof(wifi_sta_join_cfg));
    }

    msg_evt->MsgData1 = (u32)(join_cfg_msg);

    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
    #else
    netmgr_wifi_join(join_cfg);
    #endif
    return 0;
}

int netmgr_wifi_leave_async(void)
{
    #if !NET_MGR_NO_SYS
    MsgEvent *msg_evt = NULL;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_leave_async \r\n"));

    if (!netmgr_wifi_check_sta())
    {
        LOG_PRINTF("mode error.\r\n");
        return -1;
    }

    msg_evt = msg_evt_alloc();
    ASSERT(msg_evt);
    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
    msg_evt->MsgData = MSG_LEAVE_REQ;
    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
    #else
    netmgr_wifi_leave();
    #endif
    return 0;
}

int netmgr_wifi_control_async(wifi_mode mode, wifi_ap_cfg *ap_cfg, wifi_sta_cfg *sta_cfg)
{
    #if !NET_MGR_NO_SYS
    MsgEvent *msg_evt = NULL;
    wifi_ap_cfg *ap_cfg_msg = NULL;
    wifi_sta_cfg *sta_cfg_msg = NULL;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_control_async \r\n"));

    msg_evt = msg_evt_alloc();
    ASSERT(msg_evt);
    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
    msg_evt->MsgData = MSG_CONTROL_REQ;

    if (ap_cfg)
    {
        ap_cfg_msg = MALLOC(sizeof(wifi_ap_cfg));
        MEMCPY((void * )ap_cfg_msg, (void * )ap_cfg, sizeof(wifi_ap_cfg));
    }

    if (sta_cfg)
    {
        sta_cfg_msg = MALLOC(sizeof(wifi_sta_cfg));
        MEMCPY((void * )sta_cfg_msg, (void * )sta_cfg, sizeof(wifi_sta_cfg));
    }

    msg_evt->MsgData1 = (u32)(mode);
    msg_evt->MsgData2 = (u32)(ap_cfg_msg);
    msg_evt->MsgData3 = (u32)(sta_cfg_msg);

    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
    #else
    if(ap_cfg)
    {
        if (netmgr_wifi_check_sta())
        {
            netmgr_wifi_station_off();
        }

        if (netmgr_wifi_check_sconfig())
        {
            netmgr_wifi_sconfig_off();
        }
    }

    if(sta_cfg)
    {
        if (netmgr_wifi_check_ap())
        {
            netmgr_wifi_ap_off();
        }

        if((mode==SSV6XXX_HWM_STA)&&netmgr_wifi_check_sconfig())
        {
           netmgr_wifi_sconfig_off();
        }

        if((mode == SSV6XXX_HWM_SCONFIG)&&netmgr_wifi_check_sta())
        {
            netmgr_wifi_station_off();
        }
    }
    
    return netmgr_wifi_control(mode, ap_cfg, sta_cfg);
    #endif
    return 0;
}


int netmgr_wifi_switch_async(wifi_mode mode, wifi_ap_cfg *ap_cfg, wifi_sta_join_cfg *join_cfg)
{
    #if !NET_MGR_NO_SYS
    MsgEvent *msg_evt = NULL;
    wifi_ap_cfg *ap_cfg_msg = NULL;
    wifi_sta_join_cfg *join_cfg_msg = NULL;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_switch_async \r\n"));

    msg_evt = msg_evt_alloc();
    ASSERT(msg_evt);
    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
    msg_evt->MsgData = MSG_SWITCH_REQ;

    if (ap_cfg)
    {
        ap_cfg_msg = MALLOC(sizeof(wifi_ap_cfg));
        MEMCPY((void * )ap_cfg_msg, (void * )ap_cfg, sizeof(wifi_ap_cfg));
    }

    if (join_cfg)
    {
        join_cfg_msg = MALLOC(sizeof(wifi_sta_join_cfg));
        MEMCPY((void * )join_cfg_msg, (void * )join_cfg, sizeof(wifi_sta_join_cfg));
    }

    msg_evt->MsgData1 = (u32)(mode);
    msg_evt->MsgData2 = (u32)(ap_cfg_msg);
    msg_evt->MsgData3 = (u32)(join_cfg_msg);

    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
    #else
    return netmgr_wifi_switch(mode, ap_cfg, join_cfg);
    #endif
    return 0;
}


void netmgr_wifi_station_off()
{
    wifi_mode mode;
    wifi_sta_cfg *sta_cfg = NULL;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_station_off \r\n"));

    sta_cfg = (wifi_sta_cfg *)MALLOC(sizeof(wifi_sta_cfg));
    if (sta_cfg == NULL)
    {
        return;
    }

    MEMSET((void *)sta_cfg, 0, sizeof(wifi_sta_cfg));

    mode = SSV6XXX_HWM_STA;
    sta_cfg->status = false;

    netmgr_wifi_control(mode, NULL, sta_cfg);

    FREE(sta_cfg);
}

void netmgr_wifi_sconfig_off()
{
    wifi_mode mode;
    wifi_sta_cfg *sta_cfg = NULL;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_sconfig_off \r\n"));

    sta_cfg = (wifi_sta_cfg *)MALLOC(sizeof(wifi_sta_cfg));
    if (sta_cfg == NULL)
    {
        return;
    }

    MEMSET((void *)sta_cfg, 0, sizeof(wifi_sta_cfg));

    mode = SSV6XXX_HWM_SCONFIG;
    sta_cfg->status = false;

    netmgr_wifi_control(mode, NULL, sta_cfg);

    FREE(sta_cfg);
}


void netmgr_wifi_ap_off()
{
    wifi_mode mode;
    wifi_ap_cfg *ap_cfg = NULL;
    
    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_ap_off \r\n"));

    ap_cfg = (wifi_ap_cfg *)MALLOC(sizeof(wifi_ap_cfg));
    if (ap_cfg == NULL)
    {
        return;
    }

    MEMSET((void *)ap_cfg, 0, sizeof(wifi_ap_cfg));

    mode = SSV6XXX_HWM_AP;
    ap_cfg->status = false;

    netmgr_wifi_control(mode, ap_cfg, NULL);

    FREE(ap_cfg);
}

int netmgr_wifi_sconfig_broadcast(u8 *resp_data, u32 len, bool IsUDP,u32 port)
{
    struct netconn *conn;
    struct netbuf *buf;
    char *data;
    int err=0;
    char send_count=50;
    struct netif *netif=netif_list;

    if(IsUDP==FALSE) return -1;

    //wait ip address is ready
    LOG_PRINTF("wait ip address ... \r\n");
    //Fix this behavior.
    do{
        if((ip4_addr1(&netif->ip_addr.addr)!=0)&&
            (ip4_addr4(&netif->ip_addr.addr)!=0))
            break;
    }while(1);
    LOG_PRINTF("ip address is ready \r\n");
    conn = netconn_new(NETCONN_UDP);
    if(conn==NULL) err++;

    if(err==0){
        if(ERR_OK!=netconn_bind(conn, IP_ADDR_ANY,port-1)) err++;
    }

    if(err==0){
        if(ERR_OK!=netconn_connect(conn,IP_ADDR_BROADCAST,port)) err++;
    }
    if(err==0){
        do{
            buf=netbuf_new();
            if(buf==NULL) {
                netbuf_delete(buf);
                err++;
                break;
            }

            data=netbuf_alloc(buf,len);
            if(data==NULL) {
                err++;
                break;
            }
            ssv6xxx_memcpy(data,(void *)resp_data,len);
            if(ERR_OK!=netconn_send(conn,buf)){
                netbuf_delete(buf);
                err++;
                break;
            }
            netbuf_delete(buf);
        }while(send_count--);
    }

    if(conn!=NULL) netconn_delete(conn);

    if(err!=0){
        LOG_PRINTF("smart config broadcast fail\r\n");
    }else{
        LOG_PRINTF("smart config broadcast ok\r\n");
    }

    return 0;
}
int netmgr_wifi_sconfig(u16 channel_mask)
{
    struct cfg_sconfig_request *SconfigReq = NULL;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_sconfig \r\n"));

    if (!netmgr_wifi_check_sconfig())
    {
        LOG_PRINTF("mode error.\r\n");
        return -1;
    }

    SconfigReq = (void *)OS_MemAlloc(sizeof(*SconfigReq));
    if (!SconfigReq)
    {
        return -1;
    }
    SconfigReq->channel_mask   = channel_mask;
    SconfigReq->dwell_time = 10;

    if (ssv6xxx_wifi_sconfig(SconfigReq) < 0)
    {
       	LOG_PRINTF("Command failed !!\r\n");
        OS_MemFree(SconfigReq);
        return -1;
    }

    OS_MemFree(SconfigReq);

    return 0;
}

int netmgr_wifi_scan(u16 channel_mask, char *ssids[], int ssids_count)
{
    struct cfg_80211_ssid   *ssid = NULL;
    struct cfg_scan_request *ScanReq = NULL;
    int                      i = 0;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_scan \r\n"));

    if (!netmgr_wifi_check_sta())
    {
        LOG_PRINTF("mode error.\r\n");
        return -1;
    }

    ScanReq = (void *)MALLOC(sizeof(*ScanReq) + ssids_count*sizeof(struct cfg_80211_ssid));
    if (!ScanReq)
    {
        return -1;
    }
    ScanReq->is_active      = true;
    ScanReq->n_ssids        = ssids_count;
    ScanReq->channel_mask   = channel_mask;
    ScanReq->dwell_time = 0;

    ssid = (struct cfg_80211_ssid *)ScanReq->ssids;
    for (i = 0; i < ssids_count; i++)
    {
        ScanReq->ssids[i].ssid_len = STRLEN(ssids[i]);
        MEMCPY((void*)(ScanReq->ssids[i].ssid), (void*)ssids[i], ScanReq->ssids[i].ssid_len);
    }

    if (ssv6xxx_wifi_scan(ScanReq) < 0)
    {
       	LOG_PRINTF("Command failed !!\r\n");
        FREE(ScanReq);
        return -1;
    }

    FREE(ScanReq);

    return 0;
}

int netmgr_wifi_join(wifi_sta_join_cfg *join_cfg)
{
    s32    size = 0;
    struct ssv6xxx_ieee80211_bss       *ap_info_bss = NULL;
    struct cfg_join_request *JoinReq = NULL;
    u32 channel = 0;
    wifi_sec_type    sec_type = WIFI_SEC_NONE;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_join \r\n"));

    if (g_wifi_is_joining_b)
    {
        //LOG_PRINTF("wifi joining, don't join repeate.\r\n");
        return -1;
    }

    if (!netmgr_wifi_check_sta())
    {
        LOG_PRINTF("mode error.\r\n");
        return -1;
    }

    if (netmgr_wifi_check_sta_connected())
    {
        LOG_PRINTF("leave old ap\r\n");
        netmgr_wifi_leave();
        Sleep(1000);
    }

    if ((STRLEN((char *)join_cfg->ssid.ssid) == 0) || (STRLEN((char *)join_cfg->password) > MAX_PASSWD_LEN))
    {
        LOG_PRINTF("netmgr_wifi_join parameter error.\r\n");
        return -1;
    }

    ap_info_bss = ssv6xxx_wifi_find_ap_ssid((char *)join_cfg->ssid.ssid);

    if (ap_info_bss == NULL)
    {
        LOG_PRINTF("No AP \"%s\" was found.\r\n", join_cfg->ssid.ssid);
        return -1;
    }

    channel = (ap_info_bss->channel_id);

    ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_CAL, &channel, sizeof(u32));

    size = sizeof(struct cfg_join_request) + sizeof(struct ssv6xxx_ieee80211_bss);
    JoinReq = (struct cfg_join_request *)MALLOC(size);
    MEMSET((void *)JoinReq, 0, size);

    if (ap_info_bss->capab_info&BIT(4))
    {
        if (ap_info_bss->proto&WPA_PROTO_WPA)
        {
            sec_type = WIFI_SEC_WPA_PSK;

        }
        else if (ap_info_bss->proto&WPA_PROTO_RSN)
        {
            sec_type = WIFI_SEC_WPA2_PSK;
        }
        else
        {
            sec_type = WIFI_SEC_WEP;
        }
    }
    else
    {
        sec_type = WIFI_SEC_NONE;
    }

    if (sec_type == WIFI_SEC_NONE)
    {
        JoinReq->auth_alg = WPA_AUTH_ALG_OPEN;
        JoinReq->sec_type = SSV6XXX_SEC_NONE;
    }
    else if (sec_type == WIFI_SEC_WEP)
    {
        #if defined(SEC_USE_WEP40_OPEN) || defined(SEC_USE_WEP104_OPEN)
        JoinReq->auth_alg = WPA_AUTH_ALG_OPEN;
        #else
        JoinReq->auth_alg = WPA_AUTH_ALG_SHARED;
        #endif
        JoinReq->wep_keyidx = 0;
        if (STRLEN((char *)join_cfg->password) == 5)
        {
            JoinReq->sec_type = SSV6XXX_SEC_WEP_40;
        }
        else if (STRLEN((char *)join_cfg->password) == 13)
        {
            JoinReq->sec_type = SSV6XXX_SEC_WEP_104;
        }
        else
        {
            LOG_PRINTF("wrong password failed !!\r\n");
            OS_MemFree(JoinReq);
            return -1;
        }
    }
    else if (sec_type == WIFI_SEC_WPA_PSK)
    {
        JoinReq->auth_alg = WPA_AUTH_ALG_OPEN;
        JoinReq->sec_type = SSV6XXX_SEC_WPA_PSK;
    }
    else if (sec_type == WIFI_SEC_WPA2_PSK)
    {
        JoinReq->auth_alg = WPA_AUTH_ALG_OPEN;
        JoinReq->sec_type = SSV6XXX_SEC_WPA2_PSK;
    }
    else
    {
        JoinReq->auth_alg = WPA_AUTH_ALG_OPEN;
        JoinReq->sec_type = SSV6XXX_SEC_NONE;
        LOG_PRINTF("ERROR: unkown security type: %d\r\n", sec_type);
    }

    if (STRLEN((char *)join_cfg->password) == 0)
    {
        MEMCPY((void *)(JoinReq->password), g_sec_info[JoinReq->sec_type].dfl_password, STRLEN(g_sec_info[JoinReq->sec_type].dfl_password) + 1);
    }
    else
    {
        MEMCPY((void *)(JoinReq->password), (char *)join_cfg->password, STRLEN((char *)join_cfg->password) + 1);
    }

    MEMCPY((void*)&JoinReq->bss, (void*)ap_info_bss, sizeof(struct ssv6xxx_ieee80211_bss));

    LOG_PRINTF("dtim_period = %d\r\n",JoinReq->bss.dtim_period);
    LOG_PRINTF("wmm_used    = %d\r\n",JoinReq->bss.wmm_used);
    LOG_PRINTF("Joining \"%s\" using security type \"%s\".\r\n", JoinReq->bss.ssid.ssid, g_sec_info[JoinReq->sec_type].sec_name);

    if (ssv6xxx_wifi_join(JoinReq) < 0)
    {
        LOG_PRINTF("ssv6xxx_wifi_join failed !!\r\n");
        FREE(JoinReq);
        return -1;
    }

    g_wifi_is_joining_b = true;

#ifdef NET_MGR_AUTO_JOIN
    {
        user_ap_info ap_item;
        MEMSET(&ap_item, 0, sizeof(ap_item));
        ap_item.valid = false;
        ap_item.join_times = 0;
        STRCPY((char *)(ap_item.ssid.ssid), join_cfg->ssid.ssid);
        STRCPY(ap_item.password, (char *)JoinReq->password);
        netmgr_apinfo_save(&ap_item);
    }
#endif

    FREE(JoinReq);

    return 0;
}

int netmgr_wifi_leave(void)
{
    struct cfg_leave_request *LeaveReq = NULL;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_leave \r\n"));

    if (!netmgr_wifi_check_sta())
    {
        LOG_PRINTF("mode error.\r\n");
        return -1;
    }

	LeaveReq = (void *)MALLOC(sizeof(struct cfg_leave_request));
	LeaveReq->reason = 1;

    if (ssv6xxx_wifi_leave(LeaveReq) < 0)
    {
        FREE(LeaveReq);
        return -1;
    }
    else
    {
        netmgr_netif_link_set(LINK_DOWN);
    }

    FREE(LeaveReq);
    return 0;
}

int netmgr_wifi_control(wifi_mode mode, wifi_ap_cfg *ap_cfg, wifi_sta_cfg *sta_cfg)
{
    int ret = 0;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_control \r\n"));

    if (mode >= SSV6XXX_HWM_INVALID)
    {
        return -1;
    }

    if(ap_cfg)
    {
        if (netmgr_wifi_check_sta()||netmgr_wifi_check_sconfig())
        {
            LOG_PRINTF("mode error.\r\n");
            return -1;
        }
    }

    if(sta_cfg){
        if (netmgr_wifi_check_ap())
        {
            LOG_PRINTF("mode error.\r\n");
            return -1;
        }

        if((mode==SSV6XXX_HWM_STA)&&netmgr_wifi_check_sconfig()){
            LOG_PRINTF("mode error.\r\n");
            return -1;
        }

        if((mode==SSV6XXX_HWM_SCONFIG)&&netmgr_wifi_check_sta()){
            LOG_PRINTF("mode error.\r\n");
            return -1;
        }
    }

    if ((mode == SSV6XXX_HWM_STA)||(mode == SSV6XXX_HWM_SCONFIG))
    {
        if (sta_cfg)
        {
            if (s_dhcpd_enable && sta_cfg->status)
            {
                netmgr_dhcpd_set(false);
            }
            ret = ssv6xxx_wifi_station(mode,sta_cfg);
            if ((ret == (int)SSV6XXX_SUCCESS) && s_dhcpc_enable)
            {
                netmgr_netif_link_set(LINK_DOWN);
                netmgr_netif_status_set(false);
                if (sta_cfg->status)
                {
                    //netmgr_dhcpc_set(true);
                }
                else
                {
                    netmgr_dhcpc_set(false);
                }
            }
            else if(!s_dhcpc_enable)
            {
                netmgr_netif_link_set(LINK_UP);
                netmgr_netif_status_set(true);
            }
        }
    }
    else if(mode == SSV6XXX_HWM_AP)
    {
        if (ap_cfg)
        {
            if (s_dhcpc_enable && ap_cfg->status)
            {
                netmgr_dhcpc_set(false);
            }

            ret = ssv6xxx_wifi_ap(ap_cfg);

            if ((ret == (int)SSV6XXX_SUCCESS) && s_dhcpd_enable)
            {
                netmgr_netif_link_set(LINK_UP);
                netmgr_netif_status_set(true);
                if (ap_cfg->status)
                {
                    netmgr_dhcpd_set(true);
                }
                else
                {
                    netmgr_dhcpd_set(false);
                }
            }
            else if (!s_dhcpd_enable)
            {
                netmgr_netif_link_set(LINK_UP);
                netmgr_netif_status_set(true);
            }
        }
    }
    else
    {
        // not support
    }

    return ret;
}

int netmgr_wifi_switch(wifi_mode mode, wifi_ap_cfg *ap_cfg, wifi_sta_join_cfg *join_cfg)
{
    int ret = 0;
    wifi_sta_cfg sta_cfg;

    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_switch \r\n"));

    if (mode >= SSV6XXX_HWM_INVALID)
    {
        return -1;
    }

    if (mode == SSV6XXX_HWM_STA)
    {
        if (netmgr_wifi_check_ap())
        {
            netmgr_wifi_ap_off();
        }

        if (netmgr_wifi_check_sconfig())
        {
            netmgr_wifi_sconfig_off();
        }

        if (s_dhcpd_enable)
        {
            netmgr_dhcpd_set(false);
        }

        netmgr_netif_link_set(LINK_DOWN);
        netmgr_netif_status_set(false);

        sta_cfg.status = TRUE;

        ret = ssv6xxx_wifi_station(SSV6XXX_HWM_STA, &sta_cfg);

        if (join_cfg)
        {
            // do scan for join.
            g_switch_join_cfg_b = true;
            MEMCPY((void * )&g_join_cfg_data, (void * )join_cfg, sizeof(wifi_sta_join_cfg));
            netmgr_wifi_scan(0x3FFF, 0, 0);
        }

    }
    else if(mode == SSV6XXX_HWM_AP)
    {
        if (netmgr_wifi_check_sta())
        {
            netmgr_wifi_station_off();
        }

        if (netmgr_wifi_check_sconfig())
        {
            netmgr_wifi_sconfig_off();
        }

        if (netmgr_wifi_check_ap())
        {
            netmgr_wifi_ap_off();
        }

        if (ap_cfg)
        {
            if (s_dhcpc_enable)
            {
                netmgr_dhcpc_set(false);
            }

            ret = ssv6xxx_wifi_ap(ap_cfg);

            if ((ret == (int)SSV6XXX_SUCCESS) && s_dhcpd_enable)
            {
                netmgr_dhcpd_set(true);
            }
        }
    }
    else
    {
        // not support
    }

    return ret;
}

void netmgr_wifi_reg_event(void)
{
    ssv6xxx_wifi_reg_evt_cb(netmgr_wifi_event_cb);
}

void netmgr_wifi_event_cb(u32 evt_id, void *data, s32 len)
{
#if !NET_MGR_NO_SYS
    MsgEvent *msg_evt = NULL;
#endif

    //LOG_PRINTF("evt_id = %d\r\n", evt_id);
    
    LOG_DEBUGF(LOG_L4_NETMGR, ("netmgr_wifi_event_cb evt_id = %d\r\n", evt_id));

    switch (evt_id)
    {
        case SOC_EVT_SCAN_DONE:
        {
            #if !NET_MGR_NO_SYS

            struct resp_evt_result *scan_done = (struct resp_evt_result *)data;
            struct resp_evt_result *scan_done_cpy = NULL;

            if (scan_done)
            {
                msg_evt = msg_evt_alloc();
                ASSERT(msg_evt);
                msg_evt->MsgType = MEVT_NET_MGR_EVENT;
                msg_evt->MsgData = MSG_SCAN_DONE;

                scan_done_cpy = MALLOC(sizeof(struct resp_evt_result));
                MEMCPY((void *)scan_done_cpy, (char *)scan_done, sizeof(struct resp_evt_result));
                msg_evt->MsgData1 = (u32)(scan_done_cpy);
                msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
            }

            #else

            struct resp_evt_result *scan_done = (struct resp_evt_result *)data;
            if (scan_done->u.scan_done.result_code == 0)
            {
                if (g_switch_join_cfg_b)
                {
                    // do join
                    netmgr_wifi_join(&g_join_cfg_data);
                }
            }
            else
            {
                if (g_switch_join_cfg_b)
                {
                    // can't join
                    LOG_PRINTF("Scan FAIL, can't join [%s]\r\n", g_join_cfg_data.ssid.ssid);
                }
            }

            g_switch_join_cfg_b = false;

            #ifdef  NET_MGR_AUTO_JOIN
            netmgr_autojoin_process();
            #endif

            #endif
            break;
        }

        case SOC_EVT_SCAN_RESULT: // join result
        {
#if !NET_MGR_NO_SYS

            ap_info_state *scan_res = (ap_info_state *) data;
            ap_info_state *scan_res_cpy = NULL;
            if (scan_res)
            {
                msg_evt = msg_evt_alloc();
                ASSERT(msg_evt);
                msg_evt->MsgType = MEVT_NET_MGR_EVENT;
                msg_evt->MsgData = MSG_SCAN_RESULT;
                scan_res_cpy = MALLOC(sizeof(ap_info_state));
                MEMCPY((void *)scan_res_cpy, (char *)scan_res, sizeof(ap_info_state));
                msg_evt->MsgData1 = (u32)scan_res_cpy;
                msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
            }
#else
            #ifdef  NET_MGR_AUTO_JOIN

            ap_info_state *scan_res = (ap_info_state *)data;
            if (scan_res)
            {
                g_ap_list_p = scan_res->apInfo;
            }
            #endif
#endif
            break;
        }
        case SOC_EVT_SCONFIG_DONE: // join result
        {
#if !NET_MGR_NO_SYS
            struct resp_evt_result *sconfig_done = (struct resp_evt_result *)data;
            struct resp_evt_result *sconfig_done_cpy=NULL;
            if (sconfig_done)
            {
                msg_evt = msg_evt_alloc();
                ASSERT(msg_evt);
                msg_evt->MsgType = MEVT_NET_MGR_EVENT;
                msg_evt->MsgData = MSG_SCONFIG_DONE;
                sconfig_done_cpy = OS_MemAlloc(sizeof(struct resp_evt_result));
                OS_MemCPY((void *)sconfig_done_cpy, (char *)sconfig_done, sizeof(struct resp_evt_result));
                msg_evt->MsgData1 = (u32)sconfig_done_cpy;
                msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
            }
#else

#endif
            break;
        }
        case SOC_EVT_JOIN_RESULT: // join result
        {

#if !NET_MGR_NO_SYS
            struct resp_evt_result *join_res = (struct resp_evt_result *)data;
            g_wifi_is_joining_b = false;

            if (join_res)
            {
                msg_evt = msg_evt_alloc();
                ASSERT(msg_evt);
                msg_evt->MsgType = MEVT_NET_MGR_EVENT;
                msg_evt->MsgData = MSG_JOIN_RESULT;
                msg_evt->MsgData1 = (join_res->u.join.status_code != 0) ? DISCONNECT : CONNECT;
                msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
            }
#else
            struct ip_addr ipaddr, netmask, gw;
            int join_status;
            join_status = (((struct resp_evt_result *)data)->u.join.status_code != 0) ? DISCONNECT : CONNECT;
            g_wifi_is_joining_b = false;

            /* join success */
            if (join_status == CONNECT)
            {
                netmgr_netif_link_set(true);

                if (s_dhcpc_enable)
                {
                    netmgr_dhcpc_set(true);
                }

                #ifdef NET_MGR_AUTO_JOIN
                {
                    user_ap_info *ap_info;
                    Ap_sta_status *info = NULL;

                    info = (Ap_sta_status *)MALLOC(sizeof(Ap_sta_status));
                    if (netmgr_wifi_info_get(info) == 0)
                    {
                        LOG_PRINTF("#### SSID[%s] connected \r\n", info->u.station.ssid.ssid);
                        ap_info = netmgr_apinfo_find((char *)info->u.station.ssid.ssid);
                        if (ap_info)
                        {
                           netmgr_apinfo_set(ap_info, true);
                        }
                    }

                    FREE(info);
                }
                #endif
            }
            /* join failure */
            else if (join_status == DISCONNECT)
            {
                netmgr_netif_link_set(false);
                netmgr_netif_status_set(false);

                ip_addr_set_zero(&ipaddr);
                ip_addr_set_zero(&netmask);
                ip_addr_set_zero(&gw);

                netif_set_addr(&wlan0, &ipaddr, &netmask, &gw);

                if (s_dhcpc_enable)
                {
                    dhcp_stop(&wlan0);
                    s_dhcpc_status = false;
                }

                #ifdef NET_MGR_AUTO_JOIN
                //netmgr_autojoin_process();
                #endif
            }

#endif
            break;
        }
        case SOC_EVT_LEAVE_RESULT: // leave result include disconnnet
        {
#if !NET_MGR_NO_SYS
            struct resp_evt_result *leave_res = (struct resp_evt_result *)data;
            g_wifi_is_joining_b = false;

            if (leave_res)
            {
                msg_evt = msg_evt_alloc();
                ASSERT(msg_evt);
                msg_evt->MsgType = MEVT_NET_MGR_EVENT;
                msg_evt->MsgData = MSG_LEAVE_RESULT;
                msg_evt->MsgData1 = (u32)(leave_res->u.leave.reason_code);
                msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
            }
#else
            int leave_reason;
            struct ip_addr ipaddr, netmask, gw;
            leave_reason = ((struct resp_evt_result *)data)->u.leave.reason_code;

            g_wifi_is_joining_b = false;

            /* leave success */
            if (netmgr_wifi_check_sta())
            {
                netmgr_netif_link_set(false);
                netmgr_netif_status_set(false);

                ip_addr_set_zero(&ipaddr);
                ip_addr_set_zero(&netmask);
                ip_addr_set_zero(&gw);

                netif_set_addr(&wlan0, &ipaddr, &netmask, &gw);
                if (s_dhcpc_enable)
                {
                    dhcp_stop(&wlan0);
                    s_dhcpc_status = false;
                }

                #ifdef NET_MGR_AUTO_JOIN
                if (leave_reason != 0)
                {
                    netmgr_autojoin_process();
                }
                #endif
            }
            else
            {
                // do nothing
            }

#endif
            break;
        }
        case SOC_EVT_POLL_STATION: // ARP request
        {
            if(netmgr_send_arp_unicast(data)== -1)
            {
                u8 * mac = data;
                 LOG_PRINTF("Poll stattion fail, MAC:[%02x:%02x:%02x:%02x:%02x:%02x] \r\n",
                   mac[0], mac[1], mac[2],mac[3],mac[4],mac[5] );
            }
            break;
        }
        default:
            // do nothing
            break;
    }

    return;
}
#if !NET_MGR_NO_SYS
void netmgr_task(void *arg)
{
    MsgEvent *msg_evt = NULL;
    OsMsgQ mbox = st_netmgr_task[0].qevt;
    s32 res = 0;
    struct ip_addr ipaddr, netmask, gw;

    while(1)
    {
        res = msg_evt_fetch_timeout(mbox, &msg_evt, 10);
        if (res != OS_SUCCESS)
        {
            continue;
        }
        else if (msg_evt && (msg_evt->MsgType == MEVT_NET_MGR_EVENT))
        {
            LOG_DEBUGF(LOG_L4_NETMGR, ("EVENT [%d]\r\n", msg_evt->MsgData));
            switch (msg_evt->MsgData)
            {
                case MSG_SCAN_DONE:
                {
                    struct resp_evt_result *scan_done_msg = (struct resp_evt_result *)msg_evt->MsgData1;
                    if (scan_done_msg->u.scan_done.result_code == 0)
                    {
                        if (g_switch_join_cfg_b)
                        {
                            // do join
                            netmgr_wifi_join(&g_join_cfg_data);
                        }
                    }
                    else
                    {
                        if (g_switch_join_cfg_b)
                        {
                            // can't join
                            LOG_PRINTF("Scan FAIL, can't join [%s]\r\n", g_join_cfg_data.ssid.ssid);
                        }
                    }

                    g_switch_join_cfg_b = false;

                    #ifdef  NET_MGR_AUTO_JOIN
                    netmgr_autojoin_process();
                    #endif

                    if (scan_done_msg)
                    {
                        FREE(scan_done_msg);
                    }
                    break;
                }
                case MSG_SCONFIG_REQ:
                {
                    u16 channel_mask = msg_evt->MsgData1;
                    netmgr_wifi_sconfig(channel_mask);
                    break;
                }
                case MSG_SCAN_REQ:
                {
                    u16 channel_mask = msg_evt->MsgData1;
                    netmgr_wifi_scan(channel_mask, 0, 0);
                    break;
                }
                case MSG_JOIN_REQ:
                {
                    wifi_sta_join_cfg *join_cfg_msg = (wifi_sta_join_cfg *)msg_evt->MsgData1;

                    netmgr_wifi_join(join_cfg_msg);

                    if (join_cfg_msg)
                    {
                        FREE(join_cfg_msg);
                    }
                    break;
                }
                case MSG_LEAVE_REQ:
                {
                    netmgr_wifi_leave();
                    break;
                }
                case MSG_CONTROL_REQ:
                {
                    wifi_mode mode = (wifi_mode)msg_evt->MsgData1;
                    wifi_ap_cfg *ap_cfg_msg = (wifi_ap_cfg *)msg_evt->MsgData2;
                    wifi_sta_cfg *sta_cfg_msg = (wifi_sta_cfg *)msg_evt->MsgData3;

                    if(ap_cfg_msg)
                    {
                        if (netmgr_wifi_check_sta())
                        {
                            netmgr_wifi_station_off();
                        }
                    
                        if (netmgr_wifi_check_sconfig())
                        {
                            netmgr_wifi_sconfig_off();
                        }
                    }
                    
                    if(sta_cfg_msg)
                    {
                        if (netmgr_wifi_check_ap())
                        {
                            netmgr_wifi_ap_off();
                        }
                    
                        if((mode==SSV6XXX_HWM_STA)&&netmgr_wifi_check_sconfig())
                        {
                           netmgr_wifi_sconfig_off();
                        }
                    
                        if((mode == SSV6XXX_HWM_SCONFIG)&&netmgr_wifi_check_sta())
                        {
                            netmgr_wifi_station_off();
                        }
                    }
                    
                    netmgr_wifi_control(mode, ap_cfg_msg, sta_cfg_msg);

                    if (ap_cfg_msg)
                    {
                        FREE(ap_cfg_msg);
                    }
                    if (sta_cfg_msg)
                    {
                        FREE(sta_cfg_msg);
                    }
                    break;
                }
                case MSG_SWITCH_REQ:
                {
                    wifi_mode mode = (wifi_mode)msg_evt->MsgData1;
                    wifi_ap_cfg *ap_cfg_msg = (wifi_ap_cfg *)msg_evt->MsgData2;
                    wifi_sta_join_cfg *join_cfg_msg = (wifi_sta_join_cfg *)msg_evt->MsgData3;

                    netmgr_wifi_switch(mode, ap_cfg_msg, join_cfg_msg);

                    if (ap_cfg_msg)
                    {
                        FREE(ap_cfg_msg);
                    }
                    if (join_cfg_msg)
                    {
                        FREE(join_cfg_msg);
                    }
                    break;
                }

                case MSG_SCAN_RESULT:
                {
                    ap_info_state *scan_res_cpy = (ap_info_state *)msg_evt->MsgData1;
                    #ifdef  NET_MGR_AUTO_JOIN
                    if (scan_res_cpy)
                    {
                        g_ap_list_p = scan_res_cpy->apInfo;
                    }
                    #endif
                    FREE(scan_res_cpy);
                    break;
                }
                case MSG_SCONFIG_DONE:
                {
                    //wifi_sta_cfg sta_cfg;
                    //struct ssv6xxx_ieee80211_bss * _bss=NULL;
                    wifi_sta_join_cfg *join_cfg = NULL;

                    join_cfg = MALLOC(sizeof(wifi_sta_join_cfg));

                    sconfig_done_cpy = (struct resp_evt_result *)msg_evt->MsgData1;

                    //sta_cfg.status = FALSE;
                    //netmgr_wifi_control(SSV6XXX_HWM_SCONFIG,NULL,&sta_cfg);


                    #if 0
                    sta_cfg.status=TRUE;
                    netmgr_wifi_control(SSV6XXX_HWM_STA,NULL,&sta_cfg);

                    _bss = (struct ssv6xxx_ieee80211_bss * )ssv6xxx_wifi_find_ap_ssid((char *)sconfig_done_cpy->u.sconfig_done.ssid);
                    if(_bss==NULL){
                        LOG_PRINTF("Can''t find this AP's information\r\n");
                        netmgr_wifi_scan(0x3FFF,NULL,0);
                        Sleep(3000);
                        _bss = (struct ssv6xxx_ieee80211_bss * )ssv6xxx_wifi_find_ap_ssid((char *)sconfig_done_cpy->u.sconfig_done.ssid);

                    }

                    STRCPY((const char *)join_cfg->ssid.ssid, sconfig_done_cpy->u.sconfig_done.ssid);
                    STRCPY((const char *)join_cfg->password, sconfig_done_cpy->u.sconfig_done.pwd);
                    netmgr_wifi_join(join_cfg);
                    #endif

                    STRCPY((const char *)join_cfg->ssid.ssid, sconfig_done_cpy->u.sconfig_done.ssid);
                    STRCPY((const char *)join_cfg->password, sconfig_done_cpy->u.sconfig_done.pwd);
                    netmgr_wifi_switch(SSV6XXX_HWM_STA, NULL, join_cfg);
                    FREE(join_cfg);

                    break;
                }
                case MSG_JOIN_RESULT:
                {
                    int join_status;
                    join_status = msg_evt->MsgData1;

                    /* join success */
                    if (join_status == CONNECT)
                    {
                        netmgr_netif_link_set(true);

                        if (s_dhcpc_enable)
                        {
                            netmgr_dhcpc_set(true);
                        }
                        if(sconfig_done_cpy!=NULL){
                            netmgr_wifi_sconfig_broadcast((u8 *)&sconfig_done_cpy->u.sconfig_done.rand,1,TRUE,10000);
                            OS_MemFree(sconfig_done_cpy);
                            sconfig_done_cpy=NULL;
                        }
                        #ifdef NET_MGR_AUTO_JOIN
                        {
                            user_ap_info *ap_info;
                            Ap_sta_status *info = NULL;

                            info = (Ap_sta_status *)MALLOC(sizeof(Ap_sta_status));
                            if (netmgr_wifi_info_get(info) == 0)
                            {
                                LOG_PRINTF("#### SSID[%s] connected \r\n", info->u.station.ssid.ssid);
                                ap_info = netmgr_apinfo_find((char *)info->u.station.ssid.ssid);
                                if (ap_info)
                                {
                                   netmgr_apinfo_set(ap_info, true);
                                }
                            }

                            FREE(info);
                        }
                        #endif
                    }
                    /* join failure */
                    else if (join_status == DISCONNECT)
                    {
                        netmgr_netif_link_set(false);
                        netmgr_netif_status_set(false);

                        ip_addr_set_zero(&ipaddr);
                        ip_addr_set_zero(&netmask);
                        ip_addr_set_zero(&gw);

                        netif_set_addr(&wlan0, &ipaddr, &netmask, &gw);

                        if (s_dhcpc_enable)
                        {
                            dhcp_stop(&wlan0);
                            s_dhcpc_status = false;
                        }

                        #ifdef NET_MGR_AUTO_JOIN
                        //netmgr_autojoin_process();
                        #endif
                    }
                    break;
                }
                case MSG_LEAVE_RESULT:
                {
                    int leave_reason;
                    leave_reason = msg_evt->MsgData1;

                    /* leave success */
                    if (netmgr_wifi_check_sta())
                    {
                        netmgr_netif_link_set(false);
                        netmgr_netif_status_set(false);

                        ip_addr_set_zero(&ipaddr);
                        ip_addr_set_zero(&netmask);
                        ip_addr_set_zero(&gw);

                        netif_set_addr(&wlan0, &ipaddr, &netmask, &gw);
                        if (s_dhcpc_enable)
                        {
                            dhcp_stop(&wlan0);
                            s_dhcpc_status = false;
                        }

                        #ifdef NET_MGR_AUTO_JOIN
                        if (leave_reason != 0)
                        {
                            netmgr_autojoin_process();
                        }
                        #endif
                    }
                    else
                    {
                        // do nothing
                    }

                    break;
                }
            }

            msg_evt_free(msg_evt);
        }
    }
}
#endif

s32 netmgr_show(void)
{
    bool dhcpd_status, dhcpc_status;

#ifdef  NET_MGR_AUTO_JOIN
    netmgr_apinfo_show();
#endif

    netmgr_dhcp_status_get(&dhcpd_status, &dhcpc_status);

    LOG_PRINTF("\r\n");

    LOG_PRINTF("Dhcpd: %s\r\n", dhcpd_status ? "on" : "off");
    LOG_PRINTF("Dhcpc: %s\r\n", dhcpc_status ? "on" : "off");
    if (dhcpd_status)
    {
        dhcpdipmac ipmac[10];
        int i, count = 10;
        int ret = 0;

        ret = netmgr_dhcp_ipmac_get(ipmac, &count);

        if ((ret == 0) && (count > 0))
        {
            LOG_PRINTF("----------------------------------------\r\n");
            LOG_PRINTF("|        MAC         |          IP     |\r\n");
            LOG_PRINTF("----------------------------------------\r\n");
            for (i = 0; i < count; i++)
            {
                LOG_PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x] --- [%d.%d.%d.%d]\r\n", ipmac[i].mac[0], ipmac[i].mac[1], ipmac[i].mac[2],
                                    ipmac[i].mac[3], ipmac[i].mac[4], ipmac[i].mac[5],
                                     ip4_addr1_16(&ipmac[i].ip),
                          ip4_addr2_16(&ipmac[i].ip),
                          ip4_addr3_16(&ipmac[i].ip),
                          ip4_addr4_16(&ipmac[i].ip));
            }
        }
    }

    return 0;
}

#ifdef  NET_MGR_AUTO_JOIN

int netmgr_autojoin_process()
{
    if (g_ap_list_p)
    {
        if (!netmgr_wifi_check_sta_connected())
        {
            user_ap_info *ap_info;
            ap_info = netmgr_apinfo_find_best(g_ap_list_p, NUM_AP_INFO);
            if ((ap_info != NULL) && ap_info->valid)
            {
                netmgr_apinfo_autojoin(ap_info);
            }
       }
    }
}

void netmgr_apinfo_clear()
{
    int i = 0;

    for (i = 0; i < NET_MGR_USER_AP_COUNT; i++)
    {
        MEMSET(&g_user_ap_info[i], 0, sizeof(g_user_ap_info[i]));
    }
}

void netmgr_apinfo_remove(char *ssid)
{
    int i = 0;

    if (!ssid || (STRLEN(ssid) == 0))
    {
        return;
    }

    for (i = 0; i < NET_MGR_USER_AP_COUNT; i++)
    {
        if (STRCMP((char *)ssid, (char *)g_user_ap_info[i].ssid.ssid) == 0)
        {
            MEMSET(&g_user_ap_info[i], 0, sizeof(g_user_ap_info[i]));
            return;
        }
    }

    return;
}


user_ap_info * netmgr_apinfo_find(char *ssid)
{
    int i = 0;

    if (!ssid || (STRLEN(ssid) == 0))
    {
        return NULL;
    }

    for (i = 0; i < NET_MGR_USER_AP_COUNT; i++)
    {
        if (STRCMP((char *)ssid, (char *)g_user_ap_info[i].ssid.ssid) == 0)
        {
            return &(g_user_ap_info[i]);
        }
    }

    return NULL;
}

user_ap_info * netmgr_apinfo_find_best(struct ssv6xxx_ieee80211_bss * ap_list_p, int count)
{
    int i = 0;
    struct ssv6xxx_ieee80211_bss *item = NULL;
    user_ap_info * ap_info_p = NULL;

    for (i = 0; i < count; i++)
    {
        item = (ap_list_p + i);

        if (item->channel_id != 0)
        {
            ap_info_p = netmgr_apinfo_find((char *)(item->ssid.ssid));
            if (ap_info_p != NULL)
            {
                return ap_info_p;
            }
        }
    }

    return NULL;
}


void netmgr_apinfo_save(user_ap_info *ap_info)
{
    int i = 0;

    if (ap_info == NULL)
    {
        return;
    }

    for (i = 0; i < NET_MGR_USER_AP_COUNT; i++)
    {
        if (STRCMP((char *)ap_info->ssid.ssid, (char *)g_user_ap_info[i].ssid.ssid) == 0)
        {
            // replace old item, but valid/join time not change
            MEMCPY((void *)&(g_user_ap_info[i].ssid), (char *)&(ap_info->ssid), sizeof(struct cfg_80211_ssid));
            MEMCPY((void *)g_user_ap_info[i].password, (char *)ap_info->password, sizeof(char)*(MAX_PASSWD_LEN+1));
            break;
        }
    }

    if (i >= NET_MGR_USER_AP_COUNT)
    {
        // new item
        for (i = 0; i < NET_MGR_USER_AP_COUNT; i++)
        {
            if (g_user_ap_info[i].valid == false)
            {
                MEMCPY((void *)&(g_user_ap_info[i]), (char *)ap_info, sizeof(user_ap_info));
                break;
            }
        }
    }
}

void netmgr_apinfo_set(user_ap_info *ap_info, bool valid)
{
    if (ap_info == NULL)
    {
        return;
    }

    ap_info->valid = valid;

    if (!valid)
    {
        MEMSET(ap_info, 0, sizeof(user_ap_info));
    }
}

static int netmgr_apinfo_autojoin(user_ap_info *ap_info)
{
    int ret = 0;
    wifi_sta_join_cfg *join_cfg = NULL;

    if (ap_info == NULL)
    {
        return -1;
    }

    join_cfg = MALLOC(sizeof(wifi_sta_join_cfg));

    STRCPY((const char *)join_cfg->ssid.ssid, (char *)ap_info->ssid.ssid);
    STRCPY((const char *)join_cfg->password, ap_info->password);

    ret = netmgr_wifi_join(join_cfg);
    if (ret != 0)
    {
        // do nothing
    }
    else
    {
        LOG_PRINTF("\r\nAuto join [%s]\r\n", ap_info->ssid.ssid);
        ap_info->join_times++;
    }

    FREE(join_cfg);

    return ret;
}

void netmgr_apinfo_show()
{
    int i = 0;

    //LOG_PRINTF("  netmgr_apinfo_show  \r\n");
    LOG_PRINTF("\r\n");
    LOG_PRINTF("-------------------------------------------------\r\n");
    LOG_PRINTF("|%20s|    |%s|   |%8s|  V\r\n", "ssid        ",  "password", "autoJoin");
    LOG_PRINTF("-------------------------------------------------\r\n");
    for (i = 0; i < NET_MGR_USER_AP_COUNT; i++)
    {
        //if(g_user_ap_info[i].valid)
        {
            LOG_PRINTF("|%20.20s      %s            %3d  |  %d\r\n", g_user_ap_info[i].ssid.ssid, "****", \
            g_user_ap_info[i].join_times, g_user_ap_info[i].valid);
        }
    }

    LOG_PRINTF("-------------------------------------------------\r\n");
}

#endif
