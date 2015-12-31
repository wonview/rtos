#ifndef NET_MGR_H_
#define NET_MGR_H_

//#include "host_cmd_engine_priv.h"
#include <dev.h>
#include <host_apis.h>

#define WLAN_IFNAME "wlan0"

#define NET_MGR_NO_SYS 0


//#define NET_MGR_AUTO_JOIN

#ifdef  NET_MGR_AUTO_JOIN
#define NET_MGR_USER_AP_COUNT     10
#endif

typedef enum link_status_en {
    LINK_DOWN           = 0,
	LINK_UP             = 1,
} link_status;

typedef enum e_netmgr_msg
{
    MSG_SCAN_REQ, // when station mode, save ap list
    MSG_SCONFIG_REQ,
    MSG_JOIN_REQ, // when station mode and connect ok, if dhcpc flag is 1, netmgr will start dhcpc enable, or do nothing.
    MSG_LEAVE_REQ,
    MSG_CONTROL_REQ,
    MSG_SWITCH_REQ,

    MSG_SCAN_DONE,
    MSG_SCAN_RESULT, // when station mode, save ap list
    MSG_JOIN_RESULT, // when station mode and connect ok, if dhcpc flag is 1, netmgr will start dhcpc enable, or do nothing.
    MSG_LEAVE_RESULT, // when station mode
    MSG_SCONFIG_DONE, // when station mode, save ap list
    //MSG_DHCPD_CHANGE, // when ap mode, enable or disable dhcpd.
    //MSG_DHCPC_CHANGE, // when station mode, set dhcpc flag 1 or 0.
    MSG_AP_IP_CHANGE, // when ap mode, user change ip info, netmgr need to restart dhcpd
}netmgr_msg;

typedef enum{
	WIFI_SEC_NONE,
	WIFI_SEC_WEP,
	WIFI_SEC_WPA_PSK,		//8~63	ASCII
	WIFI_SEC_WPA2_PSK,		//8~63	ASCII
	WIFI_SEC_WPS,
	WIFI_SEC_MAX,
}wifi_sec_type;

typedef struct st_ipinfo
{
    u32 ipv4;
    u32 netmask;
    u32 gateway;
    u32 dns;
}ipinfo;

typedef struct st_dhcps_info{
	/* start,end are in host order: we need to compare start <= ip <= end */
	u32 start_ip;              /* start address of leases, in host order */
	u32 end_ip;                /* end of leases, in host order */
	u32 max_leases;            /* maximum number of leases (including reserved addresses) */

    u32 subnet;
    u32 gw;
    u32 dns;

	u32 auto_time;             /* how long should udhcpd wait before writing a config file.
			                         * if this is zero, it will only write one on SIGUSR1 */
	u32 decline_time;          /* how long an address is reserved if a client returns a
			                         * decline message */
	u32 conflict_time;         /* how long an arp conflict offender is leased for */
	u32 offer_time;            /* how long an offered address is reserved */
	u32 max_lease_sec;         /* maximum lease time (host order) */
	u32 min_lease_sec;         /* minimum lease time a client can request */
}dhcps_info;

typedef struct st_dhcpdipmac
{
    u32 ip;
    u8 mac[6];
    u8 reserved[2];
}dhcpdipmac;

typedef ssv6xxx_hw_mode wifi_mode;
typedef Ap_setting wifi_ap_cfg;
typedef Sta_setting wifi_sta_cfg;
typedef Ap_sta_status wifi_info;

typedef struct st_wifi_sta_join_cfg
{
    //wifi_sec_type    sec_type;
    struct cfg_80211_ssid ssid;
    u8                  password[MAX_PASSWD_LEN+1];
}wifi_sta_join_cfg;



extern void netmgr_init();
extern int netmgr_ipinfo_get(char *ifname, ipinfo *info);
extern int netmgr_ipinfo_set(char *ifname, ipinfo *info);
extern int netmgr_hwmac_get(char *ifname, char mac[6]);
extern void netmgr_netif_status_set(bool on);
extern bool netmgr_netif_status_get();
extern int netmgr_dhcpd_default();
extern int netmgr_dhcpd_set(bool enable);
extern int netmgr_dhcpc_set(bool enable);
extern int netmgr_dhcps_info_set(dhcps_info *if_dhcps);
extern int netmgr_dhcp_status_get(bool *dhcpd_status, bool *dhcpc_status);
extern int netmgr_dhcd_ipmac_get(dhcpdipmac *ipmac, int *size_count);
extern bool netmgr_wifi_check_sconfig();
extern bool netmgr_wifi_check_sta();
extern bool netmgr_wifi_check_ap();
extern bool netmgr_wifi_check_sta_connected();
extern void netmgr_netif_status_set(bool on);
extern void netmgr_netif_link_set(bool on);

extern int netmgr_wifi_scan_async(u16 channel_mask, char *ssids[], int ssids_count);
extern int netmgr_wifi_sconfig_async(u16 channel_mask);
extern int netmgr_wifi_join_async(wifi_sta_join_cfg *join_cfg);
extern int netmgr_wifi_leave_async(void);
extern int netmgr_wifi_control_async(wifi_mode mode, wifi_ap_cfg *ap_cfg, wifi_sta_cfg *sta_cfg);

extern int netmgr_wifi_scan(u16 channel_mask, char *ssids[], int ssids_count);
extern int netmgr_wifi_sconfig(u16 channel_mask);
extern int netmgr_wifi_join(wifi_sta_join_cfg *join_cfg);
extern int netmgr_wifi_leave(void);
extern int netmgr_wifi_control(wifi_mode mode, wifi_ap_cfg *ap_cfg, wifi_sta_cfg *sta_cfg);
extern void netmgr_wifi_station_off();
extern void netmgr_wifi_ap_off();

#ifdef  NET_MGR_AUTO_JOIN
extern void netmgr_apinfo_clear();
extern void netmgr_apinfo_remove(char *ssid);
extern void netmgr_apinfo_show();

#endif
#endif //NET_MGR_H_
