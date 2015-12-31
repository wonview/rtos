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

static bool g_wifi_joining; //
struct resp_evt_result *sconfig_done_cpy=NULL;


#ifdef  NET_MGR_AUTO_JOIN
typedef struct st_user_ap_info
{
    bool valid;
    wifi_sec_type sec_type;
    struct cfg_80211_ssid ssid;
    char password[MAX_PASSWD_LEN+1];
    int join_times;
}user_ap_info;

static user_ap_info g_user_ap_info[NET_MGR_USER_AP_COUNT];


#if (MLME_TASK == 0)
#else
static struct ssv6xxx_ieee80211_bss *g_ap_list_p = NULL;
#endif

void netmgr_apinfo_clear();
void netmgr_apinfo_remove(char *ssid);
user_ap_info * netmgr_apinfo_find(char *ssid);
user_ap_info * netmgr_apinfo_find_best(struct ssv6xxx_ieee80211_bss * ap_list_p, int count);
void netmgr_apinfo_save();
void netmgr_apinfo_set(user_ap_info *ap_info, bool valid);
static int netmgr_apinfo_autojoin(user_ap_info *ap_info);
void netmgr_apinfo_show();
#endif

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

    OS_MemCPY((void *)mac,(u8 *)&STA_MAC_0,4);
    OS_MemCPY((void *)(mac + 4),(u8 *)&STA_MAC_1,2);

    netmgr_net_init(mac);

    #ifdef NET_MGR_AUTO_JOIN
    netmgr_apinfo_clear();
    #endif

    g_wifi_joining = false;

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

    OS_MemCPY((void *)(wlan0.hwaddr), hwmac, 6);
    OS_MemCPY((void *)(wlan0.name),WLAN_IFNAME, 6);

    /* if sta mode and dhcpc enable, set ip is 0.0.0.0, otherwise is default ip */
    if (netmgr_wifi_check_sta() && s_dhcpc_enable)
    {
        ip_addr_set_zero(&ipaddr);
        ip_addr_set_zero(&netmask);
        ip_addr_set_zero(&gw);
    }
    else
    {
        ipaddr.addr = ipaddr_addr(DEFAULT_IPADDR);
        netmask.addr = ipaddr_addr(DEFAULT_SUBNET);
        gw.addr = ipaddr_addr(DEFAULT_GATEWAY);
    }

    netif_add(&wlan0, &ipaddr, &netmask, &gw, NULL, ethernetif_init, tcpip_input);
    netif_set_default(&wlan0);
    netif_set_up(&wlan0);

    /* if ap mode and dhcpd enable, set ip is default ip and set netif up */
    if (netmgr_wifi_check_ap() && s_dhcpd_enable)
    {
        netmgr_set_netif_status(true);
        netmgr_dhcpd_default();
        netmgr_dhcpd_set(true);
    }

    LOG_PRINTF("MAC[%02x:%02x:%02x:%02x:%02x:%02x]\r\n",
        wlan0.hwaddr[0], wlan0.hwaddr[1], wlan0.hwaddr[2],
        wlan0.hwaddr[3], wlan0.hwaddr[4], wlan0.hwaddr[5]);
}

void netmgr_set_netif_status(bool on)
{
	//LOG_PRINTF("L2 Link %s\r\n",(on==true?"ON":"OFF"));

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

    OS_MemCPY((void *)mac, pwlan->hwaddr, 6);

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

    //LOG_PRINTF("netmgr_dhcpd_set %d !!\r\n",enable);
    if (enable)
    {
        ipaddr.addr = ipaddr_addr(DEFAULT_IPADDR);
        netmask.addr = ipaddr_addr(DEFAULT_SUBNET);
        gw.addr = ipaddr_addr(DEFAULT_GATEWAY);
        netif_set_addr(pwlan, &ipaddr, &netmask, &gw);
        netmgr_set_netif_status(enable);
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

    lease = OS_MemAlloc(sizeof(struct dyn_lease) * (*size_count));
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
            OS_MemCPY((void*)(((dhcpdipmac *) (ipmac + i))->mac), (void*)(lease[i].lease_mac), 6);
        }
    }

    OS_MemFree((void*)lease);

    return 0;
}

int netmgr_dhcp_getip_bymac(u8 *mac, u32 *ipaddr)
{
    dhcpdipmac *ipmac = NULL;
    int size_count = DEFAULT_DHCP_MAX_LEASES;
    int i;

    //PRINTF("netmgr_dhcd_getip_bymac MAC:[%02x:%02x:%02x:%02x:%02x:%02x] \r\n",
    //    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );

    ipmac = (dhcpdipmac *)OS_MemAlloc(DEFAULT_DHCP_MAX_LEASES * sizeof(dhcpdipmac));
    if (ipmac == NULL)
    {
        return -1;
    }
    OS_MemSET((void *)ipmac, 0, DEFAULT_DHCP_MAX_LEASES * sizeof(dhcpdipmac));

    if (netmgr_dhcp_ipmac_get(ipmac, &size_count)){
    //    PRINTF("netmgr_dhcd_ipmac_get return failure \r\n");
        OS_MemFree(ipmac);
        return -1;
    }

    for (i=0; i<size_count; i++){
        if (!MEMCMP(ipmac[i].mac, mac, 6)){
            *ipaddr = ipmac[i].ip;
   //         PRINTF("netmgr_dhcd_getip_bymac shot ipaddr:0x%X \r\n", ntohl(*ipaddr));
            OS_MemFree(ipmac);
            return 0;
        }
    }

   // PRINTF("netmgr_dhcd_getip_bymac get ipaddr failure\r\n");
    OS_MemFree(ipmac);
    return -1;
}

extern s8_t etharp_unicast (u8 *dst_mac, ip_addr_t *ipaddr);
int netmgr_send_arp_unicast (u8 *dst_mac)
{
    ip_addr_t ipaddr;

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

    info = (Ap_sta_status *)OS_MemAlloc(sizeof(Ap_sta_status));
    OS_MemSET((void *)info, 0, sizeof(Ap_sta_status));

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

    OS_MemFree(info);

    return 0;
}

int netmgr_wifi_info_get(Ap_sta_status *info)
{
    if (info == NULL)
    {
        return -1;
    }

    OS_MemSET(info, 0, sizeof(Ap_sta_status));

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

    info = (Ap_sta_status *)OS_MemAlloc(sizeof(Ap_sta_status));
    if (info == NULL)
    {
        return false;
    }

    OS_MemSET((void *)info, 0, sizeof(Ap_sta_status));

    ssv6xxx_wifi_status(info);

    if ((info->operate == SSV6XXX_HWM_STA) && (info->status))
    {
         bRet = true;
    }

    OS_MemFree(info);

    return bRet;
}

bool netmgr_wifi_check_ap()
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

    if ((info->operate == SSV6XXX_HWM_AP) && (info->status))
    {
         bRet = true;
    }

    OS_MemFree(info);

    return bRet;
}

bool netmgr_wifi_check_sta_connected()
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

    if ((info->operate == SSV6XXX_HWM_STA) && (info->status) && (info->u.station.apinfo.status == CONNECT))
    {
        bRet = true;
    }

    OS_MemFree(info);

    return bRet;
}

int netmgr_wifi_sconfig_async(u16 channel_mask)
{
    #if !NET_MGR_NO_SYS
    MsgEvent *msg_evt = NULL;

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

int netmgr_wifi_join_async(char *ssid, wifi_sec_type encrypt_mode, char *password)
{
    #if !NET_MGR_NO_SYS
    MsgEvent *msg_evt = NULL;
    char *ssid_msg = NULL;
    char *password_msg = NULL;

    if (!netmgr_wifi_check_sta())
    {
        LOG_PRINTF("netmgr_wifi_join_async: mode error.\r\n");
        return -1;
    }

    msg_evt = msg_evt_alloc();
    ASSERT(msg_evt);
    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
    msg_evt->MsgData = MSG_JOIN_REQ;

    if (ssid && (strlen(ssid) > 0))
    {
        ssid_msg = OS_MemAlloc(strlen(ssid) + 1);
        strcpy(ssid_msg, ssid);
    }
    else
    {
        LOG_PRINTF("ssid error.\r\n");
        return -1;
    }

    if (password)
    {
        password_msg = OS_MemAlloc(strlen(password) + 1);
        strcpy(password_msg, password);
    }

    msg_evt->MsgData1 = (u32)(ssid_msg);
    msg_evt->MsgData2 = (u32)(encrypt_mode);
    msg_evt->MsgData3 = (u32)(password_msg);

    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
    #else
    netmgr_wifi_join(ssid, encrypt_mode, password);
    #endif
    return 0;
}

int netmgr_wifi_leave_async(void)
{
    #if !NET_MGR_NO_SYS
    MsgEvent *msg_evt = NULL;

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

    msg_evt = msg_evt_alloc();
    ASSERT(msg_evt);
    msg_evt->MsgType = MEVT_NET_MGR_EVENT;
    msg_evt->MsgData = MSG_CONTROL_REQ;

    if (ap_cfg)
    {
        ap_cfg_msg = OS_MemAlloc(sizeof(wifi_ap_cfg));
        OS_MemCPY((void * )ap_cfg_msg, (void * )ap_cfg, sizeof(wifi_ap_cfg));

        {
            if (netmgr_wifi_check_sta()||netmgr_wifi_check_sconfig())
            {
                OS_MemFree(ap_cfg_msg);
                LOG_PRINTF("netmgr_wifi_control_async: mode error.\r\n");
                return -1;
            }
        }
    }

    if (sta_cfg)
    {
        sta_cfg_msg = OS_MemAlloc(sizeof(wifi_sta_cfg));
        OS_MemCPY((void * )sta_cfg_msg, (void * )sta_cfg, sizeof(wifi_sta_cfg));

        if (netmgr_wifi_check_ap())
        {
            OS_MemFree(sta_cfg_msg);
            LOG_PRINTF("netmgr_wifi_control_async: mode error.\r\n");
            return -1;
        }

        if((mode==SSV6XXX_HWM_STA)&&netmgr_wifi_check_sconfig()){
            OS_MemFree(sta_cfg_msg);
            LOG_PRINTF("mode error.\r\n");
            return -1;
        }

        if((mode==SSV6XXX_HWM_SCONFIG)&&netmgr_wifi_check_sta()){
            OS_MemFree(sta_cfg_msg);
            LOG_PRINTF("mode error.\r\n");
            return -1;
        }

    }

    msg_evt->MsgData1 = (u32)(mode);
    msg_evt->MsgData2 = (u32)(ap_cfg_msg);
    msg_evt->MsgData3 = (u32)(sta_cfg_msg);

    msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
    #else

    if (ap_cfg)
    {
        if (netmgr_wifi_check_sta()||netmgr_wifi_check_sconfig())
        {
            LOG_PRINTF("mode error.\r\n");
            return -1;
        }
    }

    if(sta_cfg){

        if (netmgr_wifi_check_ap()){
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


    return netmgr_wifi_control(mode, ap_cfg, sta_cfg);
    #endif
    return 0;
}

void netmgr_wifi_station_off()
{
    wifi_mode mode;
    wifi_sta_cfg *sta_cfg = NULL;

    sta_cfg = (wifi_sta_cfg *)OS_MemAlloc(sizeof(wifi_sta_cfg));
    if (sta_cfg == NULL)
    {
        return;
    }

    OS_MemSET((void *)sta_cfg, 0, sizeof(wifi_sta_cfg));

    mode = SSV6XXX_HWM_STA;
    sta_cfg->status = false;

    netmgr_wifi_control_async(mode, NULL, sta_cfg);

    OS_MemFree(sta_cfg);
}

void netmgr_wifi_ap_off()
{
    wifi_mode mode;
    wifi_ap_cfg *ap_cfg = NULL;

    ap_cfg = (wifi_ap_cfg *)OS_MemAlloc(sizeof(wifi_ap_cfg));
    if (ap_cfg == NULL)
    {
        return;
    }

    OS_MemSET((void *)ap_cfg, 0, sizeof(wifi_ap_cfg));

    mode = SSV6XXX_HWM_AP;
    ap_cfg->status = false;

    netmgr_wifi_control_async(mode, ap_cfg, NULL);

    OS_MemFree(ap_cfg);
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

    if (!netmgr_wifi_check_sta())
    {
        LOG_PRINTF("mode error.\r\n");
        return -1;
    }

    ScanReq = (void *)OS_MemAlloc(sizeof(*ScanReq) + ssids_count*sizeof(struct cfg_80211_ssid));
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
        ScanReq->ssids[i].ssid_len = strlen(ssids[i]);
        OS_MemCPY((void*)(ScanReq->ssids[i].ssid), (void*)ssids[i], ScanReq->ssids[i].ssid_len);
    }

    if (ssv6xxx_wifi_scan(ScanReq) < 0)
    {
       	LOG_PRINTF("Command failed !!\r\n");
        OS_MemFree(ScanReq);
        return -1;
    }

    OS_MemFree(ScanReq);

    return 0;
}

int netmgr_wifi_join(char *ssid, wifi_sec_type encrypt_mode, char *password)
{
    s32    size = 0;
    struct ssv6xxx_ieee80211_bss       *ap_info_bss = NULL;
    struct cfg_join_request *JoinReq = NULL;
    u32 channel = 0;

    if (g_wifi_joining)
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

    if ((encrypt_mode >= WIFI_SEC_MAX) || (ssid == NULL) ||
        (strlen(ssid) == 0) || (password && (strlen(password) > MAX_PASSWD_LEN)))
    {
        LOG_PRINTF("netmgr_wifi_join parameter error.\r\n");
        return -1;
    }

    ap_info_bss = ssv6xxx_wifi_find_ap_ssid(ssid);

    if (ap_info_bss == NULL)
    {
        LOG_PRINTF("No AP \"%s\" was found.\r\n", ssid);
        return -1;
    }

    channel = (ap_info_bss->channel_id);

    ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_CAL, &channel, sizeof(u32));

    size = sizeof(struct cfg_join_request) + sizeof(struct ssv6xxx_ieee80211_bss);
    JoinReq = (struct cfg_join_request *)OS_MemAlloc(size);
    ssv6xxx_memset((void *)JoinReq, 0, size);

    if (encrypt_mode == WIFI_SEC_NONE)
    {
        JoinReq->auth_alg = WPA_AUTH_ALG_OPEN;
        JoinReq->sec_type = SSV6XXX_SEC_NONE;
    }
    else if (encrypt_mode == WIFI_SEC_WEP)
    {
        #if defined(SEC_USE_WEP40_OPEN) || defined(SEC_USE_WEP104_OPEN)
        JoinReq->auth_alg = WPA_AUTH_ALG_OPEN;
        #else
        JoinReq->auth_alg = WPA_AUTH_ALG_SHARED;
        #endif
        JoinReq->wep_keyidx = 0;
        if(ssv6xxx_strlen(password)==5)
            JoinReq->sec_type = SSV6XXX_SEC_WEP_40;
        else if(ssv6xxx_strlen(password)==13)
            JoinReq->sec_type = SSV6XXX_SEC_WEP_104;
        else{
            LOG_PRINTF("wrong password failed !!\r\n");
            OS_MemFree(JoinReq);
            return -1;
        }
    }
    else if (encrypt_mode == WIFI_SEC_WPA_PSK)
    {
        JoinReq->auth_alg = WPA_AUTH_ALG_OPEN;
        JoinReq->sec_type = SSV6XXX_SEC_WPA_PSK;
    }
    else if (encrypt_mode == WIFI_SEC_WPA2_PSK)
    {
        JoinReq->auth_alg = WPA_AUTH_ALG_OPEN;
        JoinReq->sec_type = SSV6XXX_SEC_WPA2_PSK;
    }
    else
    {
        JoinReq->auth_alg = WPA_AUTH_ALG_OPEN;
        JoinReq->sec_type = SSV6XXX_SEC_NONE;
        LOG_PRINTF("ERROR: unkown security type: %d\r\n", encrypt_mode);
    }

    if (password == NULL)
    {
        OS_MemCPY((void *)(JoinReq->password), g_sec_info[JoinReq->sec_type].dfl_password, strlen(g_sec_info[JoinReq->sec_type].dfl_password) + 1);
    }
    else
    {
        OS_MemCPY((void *)(JoinReq->password), password, strlen(password) + 1);
    }

    OS_MemCPY((void*)&JoinReq->bss, (void*)ap_info_bss, sizeof(struct ssv6xxx_ieee80211_bss));

    LOG_PRINTF("dtim_period = %d\r\n",JoinReq->bss.dtim_period);
    LOG_PRINTF("wmm_used    = %d\r\n",JoinReq->bss.wmm_used);
    LOG_PRINTF("Joining \"%s\" using security type \"%s\".\r\n", JoinReq->bss.ssid.ssid, g_sec_info[encrypt_mode].sec_name);

    if (ssv6xxx_wifi_join(JoinReq) < 0)
    {
        LOG_PRINTF("ssv6xxx_wifi_join failed !!\r\n");
        OS_MemFree(JoinReq);
        return -1;
    }

    g_wifi_joining = true;

#ifdef NET_MGR_AUTO_JOIN
    {
        user_ap_info ap_item;
        OS_MemSET(&ap_item, 0, sizeof(ap_item));
        ap_item.valid = false;
        ap_item.join_times = 0;
        strcpy((char *)(ap_item.ssid.ssid), ssid);
        ap_item.sec_type = encrypt_mode;
        strcpy(ap_item.password, (char *)JoinReq->password);
        netmgr_apinfo_save(&ap_item);
    }
#endif

    OS_MemFree(JoinReq);

    return 0;
}

int netmgr_wifi_leave(void)
{
    struct cfg_leave_request *LeaveReq = NULL;

    if (!netmgr_wifi_check_sta())
    {
        LOG_PRINTF("mode error.\r\n");
        return -1;
    }

	LeaveReq = (void *)OS_MemAlloc(sizeof(struct cfg_leave_request));
	LeaveReq->reason = 1;

    if (ssv6xxx_wifi_leave(LeaveReq) < 0)
    {
        OS_MemFree(LeaveReq);
        return -1;
    }
    else
    {
        netmgr_set_netif_status(LINK_DOWN);
    }

    OS_MemFree(LeaveReq);
    return 0;
}

int netmgr_wifi_control(wifi_mode mode, wifi_ap_cfg *ap_cfg, wifi_sta_cfg *sta_cfg)
{
    int ret = 0;

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
                if (sta_cfg->status)
                {
                    //netmgr_dhcpc_set(true);
                }
                else
                {
                    netmgr_dhcpc_set(false);
                }
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
                if (ap_cfg->status)
                {
                    netmgr_dhcpd_set(true);
                }
                else
                {
                    netmgr_dhcpd_set(false);
                }
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
    switch (evt_id)
    {
        case SOC_EVT_SCAN_RESULT: // join result
        {
#if !NET_MGR_NO_SYS
            #if (MLME_TASK ==0)
            struct resp_evt_result *scan_res = (struct resp_evt_result *)data;
            struct resp_evt_result *scan_res_cpy = NULL;
            if (scan_res)
            {
                msg_evt = msg_evt_alloc();
                ASSERT(msg_evt);
                msg_evt->MsgType = MEVT_NET_MGR_EVENT;
                msg_evt->MsgData = MSG_SCAN_RESULT;
                scan_res_cpy = OS_MemAlloc(sizeof(struct resp_evt_result));
                OS_MemCPY((void *)scan_res_cpy, (char *)scan_res, sizeof(struct resp_evt_result));
                msg_evt->MsgData1 = (u32)scan_res_cpy;
                msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
            }
            #else
            ap_info_state *scan_res = (ap_info_state*)data;
            ap_info_state *scan_res_cpy = NULL;
            if (scan_res)
            {
                msg_evt = msg_evt_alloc();
                ASSERT(msg_evt);
                msg_evt->MsgType = MEVT_NET_MGR_EVENT;
                msg_evt->MsgData = MSG_SCAN_RESULT;
                scan_res_cpy = OS_MemAlloc(sizeof(ap_info_state));
                OS_MemCPY((void *)scan_res_cpy, (char *)scan_res, sizeof(ap_info_state));
                msg_evt->MsgData1 = (u32)scan_res_cpy;
                msg_evt_post(st_netmgr_task[0].qevt, msg_evt);
            }

            #endif
#else
            #if (MLME_TASK ==0)
             // according to user ap list and ap rssi, net mgr will join the best ap automatic.
            #ifdef NET_MGR_AUTO_JOIN
            struct resp_evt_result *scan_res = (struct resp_evt_result *)data;
            if (!netmgr_wifi_check_sta_connected())
            {
                user_ap_info *ap_info;
                ap_info = netmgr_apinfo_find((char *)scan_res->u.scan.bss_info.ssid.ssid);
                if (ap_info && ap_info->valid)
                {
                    netmgr_apinfo_autojoin(ap_info);
                }
            }
            #endif
            #else
            #ifdef  NET_MGR_AUTO_JOIN

            ap_info_state *scan_res = (ap_info_state *)data;
            if (scan_res)
            {
                g_ap_list_p = scan_res->apInfo;
                netmgr_autojoin_process();
            }
            #endif
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
            g_wifi_joining = false;

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
            g_wifi_joining = false;

            /* join success */
            if (join_status == CONNECT)
            {
                netmgr_set_netif_status(true);

                if (s_dhcpc_enable)
                {
                    netmgr_dhcpc_set(true);
                }

                #ifdef NET_MGR_AUTO_JOIN
                {
                    user_ap_info *ap_info;
                    Ap_sta_status *info = NULL;

                    info = (Ap_sta_status *)OS_MemAlloc(sizeof(Ap_sta_status));
                    if (netmgr_wifi_info_get(info) == 0)
                    {
                        LOG_PRINTF("#### SSID[%s] connected \r\n", info->u.station.ssid.ssid);
                        ap_info = netmgr_apinfo_find((char *)info->u.station.ssid.ssid);
                        if (ap_info)
                        {
                           netmgr_apinfo_set(ap_info, true);
                        }
                    }

                    OS_MemFree(info);
                }
                #endif
            }
            /* join failure */
            else if (join_status == DISCONNECT)
            {
                netmgr_set_netif_status(false);

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
            g_wifi_joining = false;

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

            g_wifi_joining = false;

            /* leave success */
            if (netmgr_wifi_check_sta())
            {
                netmgr_set_netif_status(false);

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
            //LOG_PRINTF("EVENT [%d]\r\n", msg_evt->MsgData);
            switch (msg_evt->MsgData)
            {
                case MSG_SCAN_REQ:
                {
                    u16 channel_mask = msg_evt->MsgData1;
                    netmgr_wifi_scan(channel_mask, 0, 0);
                    break;
                }
                case MSG_SCONFIG_REQ:
                {
                    u16 channel_mask = msg_evt->MsgData1;
                    netmgr_wifi_sconfig(channel_mask);
                    break;
                }
                case MSG_JOIN_REQ:
                {
                    char *ssid_msg = (char *)msg_evt->MsgData1;
                    wifi_sec_type encrypt_mode = msg_evt->MsgData2;
                    char *password_msg = (char *)msg_evt->MsgData3;

                    netmgr_wifi_join(ssid_msg, encrypt_mode, password_msg);
                    if (ssid_msg)
                    {
                        OS_MemFree(ssid_msg);
                    }

                    if (password_msg)
                    {
                        OS_MemFree(password_msg);
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

                    netmgr_wifi_control(mode, ap_cfg_msg, sta_cfg_msg);

                    if (ap_cfg_msg)
                    {
                        OS_MemFree(ap_cfg_msg);
                    }
                    if (sta_cfg_msg)
                    {
                        OS_MemFree(sta_cfg_msg);
                    }
                    break;
                }
                case MSG_SCAN_RESULT:
                {
                    #if (MLME_TASK ==0)
                    struct resp_evt_result *scan_res_cpy = (struct resp_evt_result *)msg_evt->MsgData1;
                     // according to user ap list and ap rssi, net mgr will join the best ap automatic.
                    #ifdef NET_MGR_AUTO_JOIN
                    if (!netmgr_wifi_check_sta_connected())
                    {
                        user_ap_info *ap_info;
                        ap_info = netmgr_apinfo_find((char *)scan_res_cpy->u.scan.bss_info.ssid.ssid);
                        if (ap_info && ap_info->valid)
                        {
                            netmgr_apinfo_autojoin(ap_info);
                        }
                    }
                    #endif

                    OS_MemFree(scan_res_cpy);
                    #else

                    ap_info_state *scan_res_cpy = (ap_info_state *)msg_evt->MsgData1;

                    #ifdef  NET_MGR_AUTO_JOIN
                    if (scan_res_cpy)
                    {
                        g_ap_list_p = scan_res_cpy->apInfo;
                        netmgr_autojoin_process();
                    }
                    #endif
                    OS_MemFree(scan_res_cpy);
                    #endif
                    break;
                }
                case MSG_SCONFIG_DONE:
                {

                    wifi_sta_cfg sta_cfg;
                    wifi_sec_type encrypt_mode=WIFI_SEC_NONE;
                    struct ssv6xxx_ieee80211_bss * _bss=NULL;
                    sconfig_done_cpy = (struct resp_evt_result *)msg_evt->MsgData1;

                    sta_cfg.status=FALSE;
                    netmgr_wifi_control(SSV6XXX_HWM_SCONFIG,NULL,&sta_cfg);

                    sta_cfg.status=TRUE;
                    netmgr_wifi_control(SSV6XXX_HWM_STA,NULL,&sta_cfg);

                    _bss = (struct ssv6xxx_ieee80211_bss * )ssv6xxx_wifi_find_ap_ssid((char *)sconfig_done_cpy->u.sconfig_done.ssid);
                    if(_bss==NULL){
                        LOG_PRINTF("Can''t find this AP's information\r\n");
                        netmgr_wifi_scan(0x3FFF,NULL,0);
                        Sleep(3000);
                        _bss = (struct ssv6xxx_ieee80211_bss * )ssv6xxx_wifi_find_ap_ssid((char *)sconfig_done_cpy->u.sconfig_done.ssid);

                    }

                    encrypt_mode=(_bss->proto&WPA_PROTO_WPA)?WIFI_SEC_WPA_PSK:((_bss->proto&WPA_PROTO_RSN)?WIFI_SEC_WPA2_PSK:WIFI_SEC_NONE);
                    netmgr_wifi_join((char *)sconfig_done_cpy->u.sconfig_done.ssid,encrypt_mode,(char *)sconfig_done_cpy->u.sconfig_done.pwd);

                    break;
                }
                case MSG_JOIN_RESULT:
                {
                    int join_status;
                    join_status = msg_evt->MsgData1;

                    /* join success */
                    if (join_status == CONNECT)
                    {
                        netmgr_set_netif_status(true);

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

                            info = (Ap_sta_status *)OS_MemAlloc(sizeof(Ap_sta_status));
                            if (netmgr_wifi_info_get(info) == 0)
                            {
                                LOG_PRINTF("#### SSID[%s] connected \r\n", info->u.station.ssid.ssid);
                                ap_info = netmgr_apinfo_find((char *)info->u.station.ssid.ssid);
                                if (ap_info)
                                {
                                   netmgr_apinfo_set(ap_info, true);
                                }
                            }

                            OS_MemFree(info);
                        }
                        #endif
                    }
                    /* join failure */
                    else if (join_status == DISCONNECT)
                    {
                        netmgr_set_netif_status(false);

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
                        netmgr_set_netif_status(false);

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
    #if (MLME_TASK !=0)
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
    #endif
}

void netmgr_apinfo_clear()
{
    int i = 0;

    for (i = 0; i < NET_MGR_USER_AP_COUNT; i++)
    {
        OS_MemSET(&g_user_ap_info[i], 0, sizeof(g_user_ap_info[i]));
    }
}

void netmgr_apinfo_remove(char *ssid)
{
    int i = 0;

    if (!ssid || (strlen(ssid) == 0))
    {
        return;
    }

    for (i = 0; i < NET_MGR_USER_AP_COUNT; i++)
    {
        if (strcmp((char *)ssid, (char *)g_user_ap_info[i].ssid.ssid) == 0)
        {
            OS_MemSET(&g_user_ap_info[i], 0, sizeof(g_user_ap_info[i]));
            return;
        }
    }

    return;
}


user_ap_info * netmgr_apinfo_find(char *ssid)
{
    int i = 0;

    if (!ssid || (strlen(ssid) == 0))
    {
        return NULL;
    }

    for (i = 0; i < NET_MGR_USER_AP_COUNT; i++)
    {
        if (strcmp((char *)ssid, (char *)g_user_ap_info[i].ssid.ssid) == 0)
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
        if (strcmp((char *)ap_info->ssid.ssid, (char *)g_user_ap_info[i].ssid.ssid) == 0)
        {
            // replace old item, but valid/join time not change
            OS_MemCPY((void *)&(g_user_ap_info[i].ssid), (char *)&(ap_info->ssid), sizeof(struct cfg_80211_ssid));
            OS_MemCPY((void *)g_user_ap_info[i].password, (char *)ap_info->password, sizeof(char)*(MAX_PASSWD_LEN+1));
            g_user_ap_info[i].sec_type = ap_info->sec_type;
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
                OS_MemCPY((void *)&(g_user_ap_info[i]), (char *)ap_info, sizeof(user_ap_info));
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
        OS_MemSET(ap_info, 0, sizeof(user_ap_info));
    }
}

static int netmgr_apinfo_autojoin(user_ap_info *ap_info)
{
    int ret = 0;

    if (ap_info == NULL)
    {
        return -1;
    }


    ret = netmgr_wifi_join((char *)ap_info->ssid.ssid, ap_info->sec_type, ap_info->password);
    if (ret != 0)
    {
        // do nothing
    }
    else
    {
        LOG_PRINTF("\r\nAuto join [%s]\r\n", ap_info->ssid.ssid);
        ap_info->join_times++;
    }

    return ret;
}

void netmgr_apinfo_show()
{
    int i = 0;

    //LOG_PRINTF("  netmgr_apinfo_show  \r\n");
    LOG_PRINTF("\r\n");
    LOG_PRINTF("---------------------------------------------------------------\r\n");
    LOG_PRINTF("|%20s|    |%s|    |%s|   |%8s|  V\r\n", "ssid        ", "security", "password", "autoJoin");
    LOG_PRINTF("---------------------------------------------------------------\r\n");
    for (i = 0; i < NET_MGR_USER_AP_COUNT; i++)
    {
        //if(g_user_ap_info[i].valid)
        {
            LOG_PRINTF("|%20.20s      %6s         %s            %3d |  %d\r\n", g_user_ap_info[i].ssid.ssid, g_sec_info[g_user_ap_info[i].sec_type].sec_name, "****", \
            g_user_ap_info[i].join_times, g_user_ap_info[i].valid);
        }
    }

    LOG_PRINTF("---------------------------------------------------------------\r\n");
}



#endif
