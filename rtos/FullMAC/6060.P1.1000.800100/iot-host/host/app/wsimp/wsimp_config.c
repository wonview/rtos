#include <time.h>
#include <config.h>
#include <log.h>
#include <hdr80211.h>
#include <ssv_lib.h>
#include <common.h>
#include <pbuf.h>
#include <cmd_def.h>
#include <regs/ssv6200_reg.h>
#include <host_global.h>
#include <drv_mac.h>
#include "wsimp_lib.h"
#include "wsimp_config.h"
#include <ssv_ex_lib.h>
#if (CONFIG_HOST_PLATFORM == 1 )	
#include <host_apis.h>
#endif

extern FILE *g_wsimp_outfp;
u32 g_wsimp_packet_count;


static u8 sg_cmd_buf[512];
static struct wsimp_cfg_cmd_tbl *sg_cur_cmdtbl=NULL;
static struct cfg_host_cmd *sg_host_cmd=NULL;

/* wsimp_config: */
u8 g_soc_cmd_buffer[1024];
char g_soc_cmd_prepend[50];




u32 gTxFlowDataReason;
u32 gRxFlowDataReason;
u16 gTxFlowMgmtReason;
u16 gRxFlowMgmtReason;
u16 gRxFlowCtrlReason;



static s32 hex2i(u8 ch)
{
    if (ch>='0' && ch<='9')
        return (ch-0x30);
    if (ch>='a' && ch <='f')
        return ((ch-'a')+10);
    if (ch>='A' && ch <='F')
        return ((ch-'A')+10);
    return -1;
}


static s32 send_cfg_cmd(HDR_HostCmd *host_cmd, s32 len, void *args)
{
    struct wsimp_cfg_cmd_tbl *cfg_entry=args;
    s32 (*snd_func)(char *, s32); 
    s32 retval = 0;

    /* We shall make sure all host command are 4-byte align */
    ASSERT((len&0x03) == 0);
    snd_func         = cfg_entry->args;
    host_cmd->len    = len+HOST_CMD_HDR_LEN;
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd  = cfg_entry->hcmd_id;    
    retval = snd_func((char *)host_cmd, host_cmd->len);
    return retval;
}


#ifdef THROUGHPUT_TEST
//u8 SDIO_BSS_MAC_ADDR[ETHER_ADDR_LEN] ;
extern u8 throughput_opmode;
#endif
static s32 set_sta_bssid(void *args, s32 argc, char *argv[])
{
    HDR_HostCmd *host_cmd;
    s32 len=ETHER_ADDR_LEN + 2; /* padding 2 bytes */
	
    if (argc != 2)
        return -1;
    
    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    if (wsimp_mac_str2hex(argv[1], (ETHER_ADDR *)host_cmd->un.dat8) != ETHER_ADDR_LEN)
        return -1;
//#ifdef THROUGHPUT_TEST
//	memcpy(SDIO_BSS_MAC_ADDR, host_cmd->un.dat8, ETHER_ADDR_LEN);
//#endif		
    return send_cfg_cmd(host_cmd, len, args);
    
}

//extern u8 HOST_MAC_ADDR[6];
static s32 set_sta_mac(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=ETHER_ADDR_LEN + 2; /* padding 2 bytes */
	//ETHER_ADDR sta_mac;


    if (argc != 2)
        return -1;

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    if (wsimp_mac_str2hex(argv[1], (ETHER_ADDR *)host_cmd->un.dat8) != ETHER_ADDR_LEN)
        return -1;
    
    //memcpy(HOST_MAC_ADDR, host_cmd->un.dat8, ETHER_ADDR_LEN);

    return send_cfg_cmd(host_cmd, len, args);

}

static s32 set_sta_opmode(void *args, s32 argc, char *argv[])
{
    u8 *str[] = { "STA", "AP", "IBSS", "WDS" };
    struct cfg_host_cmd *host_cmd;
    u32 i, len=sizeof(u32);
    
    if (argc != 2)
        return -1;
    for(i=0; i<sizeof(str)/sizeof(u8 *); i++) {
        if (strcmp(argv[1], str[i]) == 0) {
#ifdef THROUGHPUT_TEST
			throughput_opmode = i;
#endif
            host_cmd = (HDR_HostCmd *)sg_cmd_buf;
            host_cmd->un.dat32[0] = i;
            return send_cfg_cmd(host_cmd, len, args);
        }
    }
    return -1;
}

static s32 key32_str2hex(u8 *str,u8 *key)
{
    u32 key1[32],i;
    if (sscanf(str, KEY_32_FORMAT,key1,key1+1,key1+2,key1+3,key1+4,key1+5,key1+6,key1+7,key1+8,key1+9,key1+10,key1+11,key1+12,key1+13,key1+14,key1+15,
				key1+16,key1+17,key1+18,key1+19,key1+20,key1+21,key1+22,key1+23,key1+24,key1+25,key1+26,key1+27,key1+28,key1+29,key1+30,key1+31)!= 32)
        return 0;

	for(i=0;i<32;i++)
		key[i] = (u8)key1[i];
    return 32;
}



static s32 key16_str2hex(u8 *str,u8 *key)
{
    u32 key1[16],i;
    if (sscanf(str, KEY_16_FORMAT,key1,key1+1,key1+2,key1+3,key1+4,key1+5,key1+6,key1+7,key1+8,key1+9,key1+10,key1+11,key1+12,key1+13,key1+14,key1+15)!= 16)
        return 0;

	for(i=0;i<16;i++)
		key[i] = (u8)key1[i];
    return 16;
}

static s32 key8_str2hex(u8 *str,u8 *key)
{
	u32 key2[8],i;
    if (sscanf(str, KEY_8_FORMAT,key2,key2+1,key2+2,key2+3,key2+4,key2+5,key2+6,key2+7)!= 8)
        return 0;

	for(i=0;i<8;i++)
		key[i] = (u8)key2[i];

    return 8;
}


static s32 set_wsid_entry(void *args, s32 argc, char *argv[])
{
    struct mac_wsid_entry_st *wsid_entry;
    struct cfg_host_cmd *host_cmd;
    s32 idx, i, res, flag=0;
    s32 seq_ctrl[9], len=sizeof(s32)+sizeof(mwsid_entry);
    u8 *ptr;

    idx = ssv6xxx_atoi(argv[1]);
    if (strcmp(argv[2], "{") != 0 || strcmp(argv[argc-1], "}") != 0)
        return -1;

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    wsid_entry = (struct mac_wsid_entry_st *)&host_cmd->un.dat32[1];
    ssv6xxx_memset((void *)wsid_entry, 0, sizeof(mwsid_entry));
    host_cmd->un.dat32[0] = (u32)idx;
    
    for(i=3; i<argc-1; i++) {
        if (strcmp(argv[i], "disable") == 0) {

            SET_WSID_INFO_VALID(wsid_entry, 0);
            return send_cfg_cmd(host_cmd, len, args);  
        }    
        if (memcmp(argv[i], "mac=", 4) == 0) {
            ptr = argv[i];
            ptr += 4;

            if (wsimp_mac_str2hex(ptr, &wsid_entry->sta_mac) != 6)
                return -1;
            continue;
        }
        if (memcmp(argv[i], "op-mode=", 8) == 0) {
            ptr = argv[i];
            ptr += 8;

            if (strcmp(ptr, "STA") == 0)
                SET_WSID_INFO_OP_MODE(wsid_entry, WLAN_STA);
            else if (strcmp(ptr, "AP") == 0)
                SET_WSID_INFO_OP_MODE(wsid_entry, WLAN_AP);
            else if (strcmp(ptr, "IBSS") == 0)
                SET_WSID_INFO_OP_MODE(wsid_entry, WLAN_IBSS);
            else if (strcmp(ptr, "WDS") == 0)
                SET_WSID_INFO_OP_MODE(wsid_entry, WLAN_WDS);
            else return -1;
            continue;
        }
        if (memcmp(argv[i], "tx_seq_ctrl=", 12) == 0) {
            res=sscanf(argv[i], "tx_seq_ctrl=(0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x)",
                &seq_ctrl[0], &seq_ctrl[1], &seq_ctrl[2], &seq_ctrl[3],
                &seq_ctrl[4], &seq_ctrl[5], &seq_ctrl[6], &seq_ctrl[7]);
            if (res != 8)
                return -1;

            wsid_entry->tx_seq_ctrl[0] = seq_ctrl[0];
            wsid_entry->tx_seq_ctrl[1] = seq_ctrl[1];
            wsid_entry->tx_seq_ctrl[2] = seq_ctrl[2];
            wsid_entry->tx_seq_ctrl[3] = seq_ctrl[3];
            wsid_entry->tx_seq_ctrl[4] = seq_ctrl[4];
            wsid_entry->tx_seq_ctrl[5] = seq_ctrl[5];
            wsid_entry->tx_seq_ctrl[6] = seq_ctrl[6];
            wsid_entry->tx_seq_ctrl[7] = seq_ctrl[7];
            continue;
        }
        if (memcmp(argv[i], "rx_seq_ctrl=", 12) ==0) {
            res=sscanf(argv[i], "rx_seq_ctrl=(0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x)",
                &seq_ctrl[0], &seq_ctrl[1], &seq_ctrl[2], &seq_ctrl[3],
                &seq_ctrl[4], &seq_ctrl[5], &seq_ctrl[6], &seq_ctrl[7]);
            if (res != 8)
                return -1; 

            wsid_entry->rx_seq_ctrl[0] = seq_ctrl[0];
            wsid_entry->rx_seq_ctrl[1] = seq_ctrl[1];
            wsid_entry->rx_seq_ctrl[2] = seq_ctrl[2];
            wsid_entry->rx_seq_ctrl[3] = seq_ctrl[3];
            wsid_entry->rx_seq_ctrl[4] = seq_ctrl[4];
            wsid_entry->rx_seq_ctrl[5] = seq_ctrl[5];
            wsid_entry->rx_seq_ctrl[6] = seq_ctrl[6];
            wsid_entry->rx_seq_ctrl[7] = seq_ctrl[7];
            continue;
        }
        
    }

    SET_WSID_INFO_VALID(wsid_entry, 1);
    return send_cfg_cmd(host_cmd, len, args);  
}

static s32 set_enable_disable(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 i, len=sizeof(u32);
    u8 *str[] = { "disable", "enable" };
   
    if (argc != 2)
        return -1;
    
    for(i=0; i<sizeof(str)/sizeof(u8 *); i++) {
        if (strcmp(argv[1], str[i]) == 0) {

            host_cmd = (HDR_HostCmd *)sg_cmd_buf;
            host_cmd->un.dat32[0] = (u32)i;
            return send_cfg_cmd(host_cmd, len, args);     
        }
    }
    return -1;
    
}

static s32 set_sta_qos(void *args, s32 argc, char *argv[])
{
    u8 *str[] = { "disable", "enable" };
    struct cfg_host_cmd *host_cmd;
    u32 i, len=sizeof(u32);

    if (argc != 2)
        return -1;
    for(i=0; i<sizeof(str)/sizeof(u8 *); i++) {
        if (strcmp(argv[1], str[i]) == 0) {
            host_cmd = (HDR_HostCmd *)sg_cmd_buf;
            host_cmd->un.dat32[0] = i;
            return send_cfg_cmd(host_cmd, len, args);   
        }
    }
    return -1;
}


static s32 set_pkt_offset(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32);

    if (argc != 2)
        return -1;
	
    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = (u32)ssv6xxx_atoi(argv[1]);
    return send_cfg_cmd(host_cmd, len, args);    
}


static s32 set_mcfilter(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    struct gmfilter_st *gmflt;
    s32 index, len=sizeof(struct gmfilter_st)+sizeof(s32);

    if (argc != 7)
        return -1;
    if (strcmp(argv[2], "{") != 0 || strcmp(argv[argc-1], "}") != 0)
        return -1;
    index = ssv6xxx_atoi(argv[1]);

    if (index<0 || index>3)
        return -1;

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = (u32)index;
    gmflt = (struct gmfilter_st *)&host_cmd->un.dat32[1];
    if (strcmp(argv[3], "mask") == 0)
        gmflt->ctrl = 2;
    else if (strcmp(argv[3], "single") == 0)
        gmflt->ctrl = 3;
    else if (strcmp(argv[3], "disable") == 0)
        gmflt->ctrl = 0;
    if (wsimp_mac_str2hex(argv[4], &gmflt->mac) != 6)
        return -1;
    if (wsimp_mac_str2hex(argv[5], &gmflt->msk) != 6)
        return -1;
    return send_cfg_cmd(host_cmd, len, args);
}


static s32 show_mcfilter(void *addr, s32 argc, char *argv[])
{
    /*s32 entry;
	gmfilter mf_addr;
    for(entry=0; entry<4; entry++)
    {
		drv_mac_get_gmfilter(entry,&mf_addr);
		//PRINTF("MCFILTER[%d] -> " ETH_ADDR_FORMAT "\n", entry,
        //ETH_ADDR(mf_addr.mac));
    }*/
    return 0;
}



static s32 set_tx_eth_trap(void *args, s32 argc, char *argv[])
{
    s32 index, len=sizeof(s32)+sizeof(u32);
    HDR_HostCmd *host_cmd;
    u32 eth_type;

    /**
        * Note that index from 0 to 3 is for Tx ether trapping while
        * index from 4 to 7 is for Rx ether trapping. 
        */
    if (argc != 3)
        return -1;
    if (strlen(argv[2]) != 6)
        return -1;
    index = ssv6xxx_atoi(argv[1]);
    if (index > 3) {
        PRINTF("Invalid index: %d\n", index);
        return -1;
    }
    if (argv[2][0]=='0' && (argv[2][1]=='X' || argv[2][1]=='x'))
    {
        eth_type = hex2i(argv[2][2])<<12;
        eth_type |= (hex2i(argv[2][3])<<8);
        eth_type |= (hex2i(argv[2][4])<<4);
        eth_type |= (hex2i(argv[2][5])<<0);

		host_cmd = (HDR_HostCmd *)sg_cmd_buf;
        host_cmd->un.dat32[0] = (u32)index;
        host_cmd->un.dat32[1] = eth_type;
        return send_cfg_cmd(host_cmd, len, args);
    }
    return -1;
}



static s32 set_rx_eth_trap(void *args, s32 argc, char *argv[])
{
    s32 index, len=sizeof(s32)+sizeof(u32);
    HDR_HostCmd *host_cmd;
    u32 eth_type;


    /**
        * Note that index from 0 to 3 is for Tx ether trapping while
        * index from 4 to 7 is for Rx ether trapping. 
        */
    if (argc != 3)
        return -1;
    if (strlen(argv[2]) != 6)
        return -1;
    index = ssv6xxx_atoi(argv[1]);
    if (index > 3) {
        PRINTF("Invalid index: %d\n", index);
        return -1;
    }
    if (argv[2][0]=='0' && (argv[2][1]=='X' || argv[2][1]=='x'))
    {
        eth_type = hex2i(argv[2][2])<<12;
        eth_type |= (hex2i(argv[2][3])<<8);
        eth_type |= (hex2i(argv[2][4])<<4);
        eth_type |= (hex2i(argv[2][5])<<0);

		host_cmd = (HDR_HostCmd *)sg_cmd_buf;
        host_cmd->un.dat32[0] = (u32)index;
        host_cmd->un.dat32[1] = eth_type;
        return send_cfg_cmd(host_cmd, len, args);
    }
    return -1;
}



static s32 show_eth_trap(void *addr, s32 argc, char *argv[])
{
    s32 entry;
	
	/*u16 eth_type = 0;
    for(entry=0; entry<4; entry++)
    {   
    	drv_mac_get_tx_ether_trap(entry,&eth_type);
    	PRINTF("Tx EtherType Trap: 0x%02x\n", eth_type);
    }
    for(entry=0; entry<4; entry++)
    {
    	drv_mac_get_rx_ether_trap(entry,&eth_type);
        PRINTF("Rx EtherType Trap: 0x%02x\n", eth_type);
    }*/
    return 0;
}



static s32 set_dup_filter(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 i, len=sizeof(u32);
    u8 *str[] = { "disable", "enable" };

    if (argc != 2)
        return -1;
    for(i=0; i<sizeof(str)/sizeof(u8 *); i++) {
        if (strcmp(argv[1], str[i]) == 0) {

            host_cmd = (HDR_HostCmd *)sg_cmd_buf;
            host_cmd->un.dat32[0] = (u32)i;
            return send_cfg_cmd(host_cmd, len, args);    
        }
    }
    return -1;
}

static s32 set_promis_mode(void *args, s32 argc, char *argv[])
{
    u8 *str[] = { "disable", "enable" };
    struct cfg_host_cmd *host_cmd;
    s32 i, len=sizeof(u32);

    if (argc != 2)
        return -1;
    for(i=0; i<sizeof(str)/sizeof(u8 *); i++) {
        if (strcmp(argv[1], str[i]) == 0) {

            host_cmd = (HDR_HostCmd *)sg_cmd_buf;
            host_cmd->un.dat32[0] = (u32)i;
            return send_cfg_cmd(host_cmd, len, args);    
        }
    }
    return -1;
}


static s32 set_pair_security(void *args, s32 argc, char *argv[])
{
    struct wsimp_cfg_cmd_tbl *cfg_entry=args;
    struct cfg_host_cmd *host_cmd;
    s32 (*snd_func)(char *, s32); 

    u8 *str[] = { "disable", "WEP40", "WEP104", "TKIP", "CCMP", "WAPI" };
    u8 i;
    
    if (argc != 2)
        return -1;
    for(i=0; i<sizeof(str)/sizeof(u8 *); i++) {
        if (strcmp(argv[1], str[i]) == 0) {

	        snd_func = cfg_entry->args;
			host_cmd = (HDR_HostCmd *)sg_cmd_buf;
	        host_cmd->len    = HOST_CMD_HDR_LEN+sizeof(u8);
	        host_cmd->c_type = HOST_CMD;
	        host_cmd->h_cmd  = cfg_entry->hcmd_id;
	        host_cmd->un.dat8[0] = i;
	        return snd_func((char *)host_cmd, host_cmd->len);
        }
    }
    return -1;
}


static s32 set_group_security(void *args, s32 argc, char *argv[])
{
    struct wsimp_cfg_cmd_tbl *cfg_entry=args;
    struct cfg_host_cmd *host_cmd;
    s32 (*snd_func)(char *, s32); 

    u8 *str[] = { "disable", "WEP40", "WEP104", "TKIP", "CCMP" };
    u8 i;
    
    if (argc != 2)
        return -1;
    for(i=0; i<sizeof(str)/sizeof(u8 *); i++) {
        if (strcmp(argv[1], str[i]) == 0) {

			snd_func = cfg_entry->args;
			host_cmd = (HDR_HostCmd *)sg_cmd_buf;
			host_cmd->len	 = HOST_CMD_HDR_LEN+sizeof(u8);
			host_cmd->c_type = HOST_CMD;
			host_cmd->h_cmd  = cfg_entry->hcmd_id;
			host_cmd->un.dat8[0] = i;
			return snd_func((char *)host_cmd, host_cmd->len);
        }
    }
    return -1;
}

static s32 set_pair_entry(void *args, s32 argc, char *argv[])
{
    struct wsimp_cfg_cmd_tbl *cfg_entry=args;
    struct cfg_host_cmd *host_cmd;
    s32 (*snd_func)(char *, s32);
	struct security_pair_entry security_entry;

    s32 idx, i, flag=0;
    u8 *ptr;

    idx = ssv6xxx_atoi(argv[1]);
    if (strcmp(argv[2], "{") != 0 || strcmp(argv[argc-1], "}") != 0)
        return -1;

	security_entry.index = idx;

    for(i=3; i<argc-1; i++) {
        if (strcmp(argv[i], "disable") == 0) {
            return 0;
        }

        if (memcmp(argv[i], "pair_key_idx=", 13) == 0) { //key index
            ptr = argv[i];
            ptr += 13;
			//pGSRAM_KEY->wsid[idx].pair_key_idx= ssv6xxx_atoi(ptr);

			security_entry.pair_key_idx= ssv6xxx_atoi(ptr);
            continue;
        }
        if (memcmp(argv[i], "group_key_idx=", 14) == 0) { //key index
            ptr = argv[i];
            ptr += 14;
			//pGSRAM_KEY->wsid[idx].group_key_idx= ssv6xxx_atoi(ptr);

			security_entry.group_key_idx= ssv6xxx_atoi(ptr);
            continue;
        }
        if (memcmp(argv[i], "key=", 4) == 0) { //key1
            ptr = argv[i];
            ptr += 4;
            //if (key32_str2hex(ptr, pGSRAM_KEY->wsid[idx].pair.key) != 32)
            //    return -1;

			if (key32_str2hex(ptr, security_entry.key) != 32)
							return -1;
            continue;
        }
        if (memcmp(argv[i], "tx_pn=", 6) == 0) { 
            ptr = argv[i];
            ptr += 6;
            //pGSRAM_KEY->wsid[idx].pair.tx_pn= (u64)ssv6xxx_64atoi(ptr);

            security_entry.tx_pn= (u64)ssv6xxx_64atoi(ptr);
            continue;
        }
        if (memcmp(argv[i], "rx_pn=", 6) == 0) { 
            ptr = argv[i];
            ptr += 6;
            //pGSRAM_KEY->wsid[idx].pair.rx_pn = (u64)ssv6xxx_64atoi(ptr);

			security_entry.rx_pn = (u64)ssv6xxx_64atoi(ptr);
            continue;
        }
    }
	
	snd_func = cfg_entry->args;
	host_cmd = (HDR_HostCmd *)sg_cmd_buf;
	host_cmd->len	 = HOST_CMD_HDR_LEN+sizeof(struct security_pair_entry);
	host_cmd->c_type = HOST_CMD;
	host_cmd->h_cmd  = cfg_entry->hcmd_id;
	*(struct security_pair_entry *)host_cmd->un.dat8 = security_entry;
	return snd_func((char *)host_cmd, host_cmd->len);

}

static s32 set_group_entry(void *args, s32 argc, char *argv[])
{
    struct wsimp_cfg_cmd_tbl *cfg_entry=args;
    struct cfg_host_cmd *host_cmd;
    s32 (*snd_func)(char *, s32);
	struct security_group_entry security_entry;


    s32 idx, i, flag=0;
    u8 *ptr;

    idx = ssv6xxx_atoi(argv[1]);
    if (strcmp(argv[2], "{") != 0 || strcmp(argv[argc-1], "}") != 0)
        return -1;

	security_entry.index = idx;

    for(i=3; i<argc-1; i++) {
        if (strcmp(argv[i], "disable") == 0) {
            return 0;
        }

        if (memcmp(argv[i], "key=", 4) == 0) { //key1
            ptr = argv[i];
            ptr += 4;
            //if (key32_str2hex(ptr, pGSRAM_KEY->group[idx].key) != 32)
            //    return -1;

			if (key32_str2hex(ptr, security_entry.key) != 32)
                return -1;
            continue;
        }
        if (memcmp(argv[i], "tx_pn=", 6) == 0) { 
            ptr = argv[i];
            ptr += 6;
           // pGSRAM_KEY->group[idx].tx_pn= (u64)ssv6xxx_64atoi(ptr);

			security_entry.tx_pn= (u64)ssv6xxx_64atoi(ptr);
            continue;
        }
        if (memcmp(argv[i], "rx_pn=", 6) == 0) { 
            ptr = argv[i];
            ptr += 6;
            //pGSRAM_KEY->group[idx].rx_pn = (u64)ssv6xxx_64atoi(ptr);

			security_entry.rx_pn = (u64)ssv6xxx_64atoi(ptr);
            continue;
        }
    }

	snd_func = cfg_entry->args;
	host_cmd = (HDR_HostCmd *)sg_cmd_buf;
	host_cmd->len	 = HOST_CMD_HDR_LEN+sizeof(struct security_group_entry);
	host_cmd->c_type = HOST_CMD;
	host_cmd->h_cmd  = cfg_entry->hcmd_id;
	*(struct security_group_entry *)host_cmd->un.dat8 = security_entry;
	return snd_func((char *)host_cmd, host_cmd->len);

}

static s32 set_tx_info_security(void *addr, s32 argc, char *argv[])
{
#if (CONFIG_HOST_PLATFORM == 1 )	
	u8 *str[] = { "disable", "enable" };
	s32 i;
	
	addr = addr;
	if (argc != 2)
		return -1;
	for(i=0; i<sizeof(str)/sizeof(u8 *); i++) {
		if (strcmp(argv[1], str[i]) == 0) {
			_ssv6xxx_wifi_apply_security_SIM(i,TRUE);
			return 0;
		}
	}	
#endif	
	return -1;
}


static s32 set_ht_mode(void *args, s32 argc, char *argv[])
{
    u8 *str[] = { "Non-HT", "HT-MF", "HT-GF", "RSVD" };
    struct cfg_host_cmd *host_cmd;
    s32 i, len=sizeof(u32);
    
    if (argc != 2)
        return -1;
    for(i=0; i<sizeof(str)/sizeof(u8 *); i++) {
        if (strcmp(argv[1], str[i]) == 0) {

            host_cmd = (HDR_HostCmd *)sg_cmd_buf;
            host_cmd->un.dat32[0] = (u32)i;
            return send_cfg_cmd(host_cmd, len, args);
        }
    }
    return -1;
}


static s32 set_sta_nav(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32);

    if (argc != 2)
        return -1;

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = (u32)ssv6xxx_atoi(argv[1]);
    return send_cfg_cmd(host_cmd, len, args);        
}

static s32 set_hdr_strip_off(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 i, len=sizeof(u32);
    u8 *str[] = { "disable", "enable" };
    
    if (argc != 2)
        return -1;
    for(i=0; i<sizeof(str)/sizeof(u8 *); i++) {
        if (strcmp(argv[1], str[i]) == 0) {

            host_cmd = (HDR_HostCmd *)sg_cmd_buf;
            host_cmd->un.dat32[0] = (u32)i;
            return send_cfg_cmd(host_cmd, len, args);             
        }
    }
    return -1;
}

static s32 set_hci_rx2host(void *args, s32 argc, char *argv[])
{
    u8 *str[] = { "disable", "enable" };
    struct cfg_host_cmd *host_cmd;
    s32 i, len=sizeof(u32);
    
    if (argc != 2)
        return -1;
    for(i=0; i<sizeof(str)/sizeof(u8 *); i++) {
        if (strcmp(argv[1], str[i]) == 0) {

            host_cmd = (HDR_HostCmd *)sg_cmd_buf;
            host_cmd->un.dat32[0] = (u32)i;
            return send_cfg_cmd(host_cmd, len, args);     
        }
    }
    return -1;
}

static s32 set_reason_trap(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32)+sizeof(u32);
    
    if (argc != 5)
        return -1;
    if (strcmp(argv[1], "{") && strcmp(argv[4], "}"))
        return -1;

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = (u32)ssv6xxx_atoi(argv[2]);
    host_cmd->un.dat32[1] = (u32)ssv6xxx_atoi(argv[3]);
    return send_cfg_cmd(host_cmd, len, args);     
}


static s32 set_seqno_ctrl(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32);
    
    if (argc != 2)
        return -1;

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = (u32)ssv6xxx_atoi(argv[1]);
    return send_cfg_cmd(host_cmd, len, args);     
}


static u32 _gen_flow_cmd_reason(s32 argc, char *argv[]) 
{
    u8 *str[] = { "M_CPU_TXL34CS", "M_CPU_RXL34CS", "M_CPU_DEFRAG", "M_CPU_EDCATX",
                  "M_CPU_RXDATA", "M_CPU_RXMGMT", "M_CPU_RXCTRL", "M_CPU_FRAG"};
    s32 i, j;
    u32 value;

    for(i=2, value=0; i<argc-1; i++) {
        for(j=0; j<sizeof(str)/sizeof(u8 *); j++) {
            if (strcmp(argv[i], str[j]) == 0) {
                value |= ((j+1)<<((i-2)<<2));
                break;
            }
        }
    }
    return value; 
}


static u32 _gen_flow_cmd(s32 argc, char *argv[])
{
  
	u8 *str[] = { "M_ENG_CPU", "M_ENG_HWHCI", "M_ENG_EMPTY", "M_ENG_ENCRYPT", "M_ENG_MACRX",
                          "M_ENG_MIC", "M_ENG_TX_EDCA0", "M_ENG_TX_EDCA1", "M_ENG_TX_EDCA2", "M_ENG_TX_EDCA3", 
                          "M_ENG_TX_MNG", "M_ENG_ENCRYPT_SEC", "M_ENG_MIC_SEC", "M_ENG_RESERVED_1", "M_ENG_RESERVED_2",
                          "M_ENG_TRASH_CAN"};
    u8 *str1[] = { "M_CPU_TXL34CS", "M_CPU_RXL34CS", "M_CPU_DEFRAG", "M_CPU_EDCATX",
                  "M_CPU_RXDATA", "M_CPU_RXMGMT", "M_CPU_RXCTRL", "M_CPU_FRAG"};    
    u32 fcmd=0;
	s32 i, j;
    
    for(i=2; i<argc-1; i++) {
        for(j=0; j<sizeof(str)/sizeof(u8 *); j++) {
            if (strcmp(argv[i], str[j]) == 0) {
                fcmd |= (j<<((i-2)<<2));
                goto next;
            }
        }
        /* We treat ME_CPU_XXX as ME_ENG_CPU */
        for(j=0; j<sizeof(str1)/sizeof(u8 *); j++) {
            if (strcmp(argv[i], str1[j]) == 0) {
                fcmd |= (0<<((i-2)<<2));
                goto next;
            }
        }
        ASSERT(0);
next:;
    }
    return fcmd;
}


static s32 set_tx_flow_data(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32)+sizeof(u32);
    
    if (argc != 11)
        return -1;
    if (strcmp(argv[1], "{") || strcmp(argv[argc-1], "}"))
        return -1;

    gTxFlowDataReason = _gen_flow_cmd_reason(argc, argv);

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = _gen_flow_cmd(argc, argv);
    host_cmd->un.dat32[1] = gTxFlowDataReason;
    return send_cfg_cmd(host_cmd, len, args);
}


static s32 set_tx_flow_mgmt(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32)+sizeof(u32);
    
    if (argc != 7)
        return -1;
    if (strcmp(argv[1], "{") || strcmp(argv[argc-1], "}"))
        return -1;    

    gTxFlowMgmtReason = _gen_flow_cmd_reason(argc, argv);

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = _gen_flow_cmd(argc, argv);
    host_cmd->un.dat32[1] = (u32)gTxFlowMgmtReason;
    return send_cfg_cmd(host_cmd, len, args);
}


static s32 set_tx_flow_ctrl(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32);

    if (argc != 7)
        return -1;
    if (strcmp(argv[1], "{") || strcmp(argv[argc-1], "}"))
        return -1;

#if (CONFIG_HOST_PLATFORM == 0 )	
	if (0 != _gen_flow_cmd_reason(argc, argv))
	{
		//we don't support extra dispatch of TX CTRL in softmain. 
		ASSERT(FALSE);
	}
#endif

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = _gen_flow_cmd(argc, argv);
    return send_cfg_cmd(host_cmd, len, args);
}


static s32 set_rx_flow_data(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32)+sizeof(u32);

    if (argc != 11)
        return -1;
    if (strcmp(argv[1], "{") || strcmp(argv[argc-1], "}"))
        return -1;    

    gRxFlowDataReason = _gen_flow_cmd_reason(argc, argv);

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = _gen_flow_cmd(argc, argv);
    host_cmd->un.dat32[1] = gRxFlowDataReason;
    return send_cfg_cmd(host_cmd, len, args);
}


static s32 set_rx_flow_mgmt(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32)+sizeof(u32);
    
    if (argc != 7)
        return -1;
    if (strcmp(argv[1], "{") || strcmp(argv[argc-1], "}"))
        return -1;   

    gRxFlowMgmtReason = _gen_flow_cmd_reason(argc, argv);

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = _gen_flow_cmd(argc, argv);
    host_cmd->un.dat32[1] = (u32)gRxFlowMgmtReason;
    return send_cfg_cmd(host_cmd, len, args);
}

static s32 set_rx_flow_ctrl(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32)+sizeof(u32);

    if (argc != 7)
        return -1;
    if (strcmp(argv[1], "{") || strcmp(argv[argc-1], "}"))
        return -1;    

	gRxFlowCtrlReason = _gen_flow_cmd_reason(argc, argv);

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = _gen_flow_cmd(argc, argv);
	host_cmd->un.dat32[1] = (u32)gRxFlowCtrlReason;
    return send_cfg_cmd(host_cmd, len, args);    
}




static s32 set_wmm_param(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32);
	
    s32 idx, i;
    u8 *ptr;
	struct cfg_set_wmm_param wmm_param_array={0};	

	len+=sizeof(struct cfg_set_wmm_param);
    idx = ssv6xxx_atoi(argv[1]);
    if (strcmp(argv[2], "{") != 0 || strcmp(argv[argc-1], "}") != 0)
        return -1;

		
	for(i=3; i<argc-1; i++) {
			if (strcmp(argv[i], "disable") == 0) {

				return 0;
			}	 
			if (memcmp(argv[i], "aifsn=", 6) == 0) {
				ptr = argv[i];
				ptr += 6;
				wmm_param_array.aifsn = ssv6xxx_atoi(ptr);
				continue;
			}
	
			if (memcmp(argv[i], "acm=", 4) == 0) {
				ptr = argv[i];
				ptr += 4;
				wmm_param_array.acm = ssv6xxx_atoi(ptr);
				continue;
			}
	
			if (memcmp(argv[i], "cwmin=", 6) == 0) {
				ptr = argv[i];
				ptr += 6;
				wmm_param_array.cwmin = ssv6xxx_atoi(ptr);
				continue;
			}
			
			if (memcmp(argv[i], "cwmax=", 6) == 0) {
				ptr = argv[i];
				ptr += 6;
				wmm_param_array.cwmax = ssv6xxx_atoi(ptr);
				continue;
			}
	
			if (memcmp(argv[i], "txop=", 5) == 0) {
				ptr = argv[i];
				ptr += 5;
				wmm_param_array.txop = ssv6xxx_atoi(ptr);
				continue;
			}
	
			if (memcmp(argv[i], "backoffvalue=", 13) == 0) {
				ptr = argv[i];
				ptr += 13;
				wmm_param_array.backoffvalue= ssv6xxx_atoi(ptr);
				wmm_param_array.enable_backoffvalue = 1;			
				continue;
			}		
        }




	host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = idx;
	*(struct cfg_set_wmm_param *)&host_cmd->un.dat32[1] = wmm_param_array;
	
	//memcpy(host_cmd->un.dat32[1], &wmm_param_array, sizeof(struct cfg_set_wmm_param));


    return send_cfg_cmd(host_cmd, len, args); 
	
}




static s32 set_txop_dc_by_frame(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32);

	int i;
	u16 val=0x00;
    if (argc != 2)
        return -1;
	
	 
	 for(i=1; i<argc; i++) {
	 	
        if (strcmp(argv[i], "disable") == 0) {
			val = 0x00;			
            //return 0;
        }

		 if (strcmp(argv[i], "enable") == 0) {
			 val = 0x01;	 
			 //return 0;
		 }

	 }


	host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = val;
    return send_cfg_cmd(host_cmd, len, args); 
	
}



static s32 set_tx_queue_halt(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32);

	int i;
	u16 halt_q_mb=0x00;
    if (argc != 2)
        return -1;
	
	 
	 for(i=1; i<argc; i++) {
	 	
        if (strcmp(argv[i], "disable") == 0) {
			halt_q_mb = 0x00;			
            //return 0;
        }

		 if (strcmp(argv[i], "enable") == 0) {
			 halt_q_mb = 0x3f;	 
			 //return 0;
		 }

	 }


	host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = halt_q_mb;
    return send_cfg_cmd(host_cmd, len, args); 
	
}


static s32 set_wmm_random(void *args, s32 argc, char *argv[])
{
#if 0
	s32 i, cnt=0;
	struct cfg_host_cmd *host_cmd;
    s32 len=4;//cnt number;


	host_cmd = (HDR_HostCmd *)sg_cmd_buf;


    if (strcmp(argv[1], "{") != 0 || strcmp(argv[argc-1], "}") != 0)
        return -1;

	//if(argc-1-2> (512/4-2))
	//	ASSERT(FALSE);
	
	
    for(i=2; i<argc-1; i++) {
        if (strcmp(argv[i], "disable") == 0) {		
            return 0;
        }

		host_cmd->un.dat32[i-1] = ssv6xxx_atoi(argv[i]);   
		cnt++;
		len+=4;

		
    }


	 host_cmd->un.dat32[0] = cnt;

    return send_cfg_cmd(host_cmd, len, args); 
   

#endif  
    return 0;
}


static s32 set_tx_packet_cnt(void *args, s32 argc, char *argv[])
{
	
    if (argc != 2)
        return -1;
    //g_wsimp_packet_count = ssv6xxx_atoi(argv[1]);
	//g_mactx_packet_count = 0;



	//set command to soc
	{		
		struct wsimp_cfg_cmd_tbl *cfg_entry=args;
		struct cfg_host_cmd *host_cmd;

		s32 len=HOST_CMD_HDR_LEN+sizeof(u32);
		s32 (*snd_func)(char *, s32); 
		
	    snd_func = cfg_entry->args;
	    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
	    host_cmd->len    = len;
	    host_cmd->c_type = HOST_CMD;
	    host_cmd->h_cmd  = cfg_entry->hcmd_id;
		host_cmd->RSVD0 = 0;
		
		host_cmd->un.dat32[0] = ssv6xxx_atoi(argv[1]);
								
	    return snd_func((char *)host_cmd, len);    
	}
}



static s32 set_mac_mode(void *args, s32 argc, char *argv[])
{
    char *mstr[] = { "normal", "mac-loopback", "mac-to-mac", "phy-loopback" };
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32);

    if (argc != 2)
        return -1;
    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    
    if (strcmp(argv[1], "normal") == 0) {

        host_cmd->un.dat32[0] = 0;
    }
    else if (strcmp(argv[1], "mac-loopback") == 0) {

        host_cmd->un.dat32[0] = 1;
    }
    else if (strcmp(argv[1], "mac-to-mac") == 0) {

        host_cmd->un.dat32[0] = 2;
    }    
    else if (strcmp(argv[1], "phy-loopback") ==0) {
        host_cmd->un.dat32[0] = 4;
    }
    else if (strcmp(argv[1], "rx-check-loopback") == 0) {

        host_cmd->un.dat32[0] = 8;
    }
    return send_cfg_cmd(host_cmd, len, args); 

}


static s32 set_sifs(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32);

    if (argc != 2)
        return -1;

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = (u32)ssv6xxx_atoi(argv[1]);
    return send_cfg_cmd(host_cmd, len, args);    
}


static s32 set_difs(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32);

    if (argc != 2)
        return -1;

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = (u32)ssv6xxx_atoi(argv[1]);
    return send_cfg_cmd(host_cmd, len, args);    
}


static s32 set_eifs(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32);
    
    if (argc != 2)
        return -1;

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = (u32)ssv6xxx_atoi(argv[1]);
    return send_cfg_cmd(host_cmd, len, args);    
}


static s32 set_mgmt_txqid(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32);

    if (argc != 2)
        return -1;

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = (u32)ssv6xxx_atoi(argv[1]);
    return send_cfg_cmd(host_cmd, len, args);    
}


static s32 set_nonqos_txqid(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32);

    if (argc != 2)
        return -1;

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = (u32)ssv6xxx_atoi(argv[1]);
    return send_cfg_cmd(host_cmd, len, args);    
}


static s32 set_auto_seqno(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 i, len=sizeof(u32);
    u8 *str[] = { "disable", "enable" };
   
    if (argc != 2)
        return -1;
    
    for(i=0; i<sizeof(str)/sizeof(u8 *); i++) {
        if (strcmp(argv[1], str[i]) == 0) {

            host_cmd = (HDR_HostCmd *)sg_cmd_buf;
            host_cmd->un.dat32[0] = (u32)i;
            return send_cfg_cmd(host_cmd, len, args);     
        }
    }
    return -1;
    
}



static s32 set_rx2host_fmt(void *args, s32 argc, char *argv[])
{	 

	struct cfg_host_cmd *host_cmd;
    s32 i, len=sizeof(u32);
    char *str[] = { "disable", "enable" };
    
    if (argc != 2)
        return -1;
	
    for(i=0; i<sizeof(str)/sizeof(u8 *); i++) {
        if (strcmp(argv[1], str[i]) == 0) {

			host_cmd = (HDR_HostCmd *)sg_cmd_buf;
			host_cmd->un.dat32[0] = (u32)i;
            return send_cfg_cmd(host_cmd, len, args);
        }
    }
    return -1;
}


static s32 set_tx_rx_info_size(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32);

    if (argc != 2)
        return -1;

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat8[0] = (u32)ssv6xxx_atoi(argv[1]);
    return send_cfg_cmd(host_cmd, len, args);    
}


static s32 set_erp_protect(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 i, len=sizeof(u32);
    char *str[] = { "disable", "rts/cts", "cts-to-self" };
    
    if (argc != 2)
        return -1;

    for(i=0; i<sizeof(str)/sizeof(u8 *); i++) {
        if (strcmp(argv[1], str[i]) == 0) {

            host_cmd = (HDR_HostCmd *)sg_cmd_buf;
            host_cmd->un.dat32[0] = (u32)i;
            return send_cfg_cmd(host_cmd, len, args);     
        }
    }    
    return -1;
}



static s32 set_decision_table(void *args, s32 argc, char *argv[])
{
#if 0
    struct cfg_host_cmd *host_cmd;

    s32 index, val, len=sizeof(u16)*25 + 2; /* padding 2 bytes */

    if (argc != 28 || strcmp(argv[1], "{") || strcmp(argv[27], "}"))
        return -1;
    
    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    for(index=0; index<16; index++) {
        val = ssv6xxx_atoi(argv[index+2]);

        host_cmd->un.dat16[index] = val;
    }
    
    for(index=0; index<9; index++) {
        val = ssv6xxx_atoi(argv[index+2+16]);

        host_cmd->un.dat16[index+16] = val;        
    }
    return send_cfg_cmd(host_cmd, len, args);     
#else
    s32 i, res;
    char *pos;

    ssv6xxx_memset((void *)g_soc_cmd_prepend, 0, sizeof(g_soc_cmd_prepend));
    if (argc == 2 && strcmp(argv[1], "--BEGIN--")==0) {
        if (sg_cur_cmdtbl != NULL)
            return -1;
        sg_cur_cmdtbl = args;
        strcpy(g_soc_cmd_prepend, "soc set ");
        sg_host_cmd = (HDR_HostCmd *)sg_cmd_buf;
        sg_host_cmd->len = HOST_CMD_HDR_LEN;
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--END--")==0) {
        if (sg_cur_cmdtbl == NULL)
            return -1;
        if (sg_host_cmd->len & 0x03)
            sg_host_cmd->len += 2; /* padding 2 bytes */
        //ssv6xxx_raw_dump(sg_host_cmd, sg_host_cmd->len);
        res = send_cfg_cmd(sg_host_cmd, sg_host_cmd->len-HOST_CMD_HDR_LEN, args);
        sg_cur_cmdtbl = NULL;
        g_soc_cmd_prepend[0] = 0x00;
        sg_host_cmd = NULL;
        return res;
    }

    pos = (char *)sg_host_cmd + sg_host_cmd->len;
    for(i=0; i<argc; i++) {
        //LOG_PRINTF("  [%d]: ==> %s\n", i, argv[i]);
        *(u16 *)pos = (u16)ssv6xxx_atoi(argv[i]);
        sg_host_cmd->len += sizeof(u16);
        pos += sizeof(u16);
    }
    return 0;


#endif
}




static s32 set_frag_threshold(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32);

    if (argc != 2)
        return -1;

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = (u32)ssv6xxx_atoi(argv[1]);
    return send_cfg_cmd(host_cmd, len, args);    
}





#if 0
static s32 init_phy_table(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat32[0] = NULL;
    return send_cfg_cmd(host_cmd, 0, args);
}
#endif

/*static s32 set_mib(void *args, s32 argc, char *argv[])
{
	struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(struct cfg_set_mib);
	struct cfg_set_mib mib_info;
	
    if (argc < 2)
        return -1;

	host_cmd = (HDR_HostCmd *)sg_cmd_buf;
	mib_info.mib_value = (u32)ssv6xxx_atoi(argv[2]);
    if (!strcmp(argv[1], "dot11RtsThres") && argc==3)
    {    		
		mib_info.mib_type = CFG_MIB_RTS;
        //dot11RTSThreshold = mib_info.mib_value;	
    }    
	else
    {
		return -1;
	}
	 
     *(struct cfg_set_mib *)host_cmd->un.dat8 = mib_info;
     return send_cfg_cmd(host_cmd, len, args);
}
*/

static s32 set_phy_info(void *args, s32 argc, char *argv[])
{
    char *pos;
    s32 i, res;

    if (argc == 2 && strcmp(argv[1], "--BEGIN--")==0) {
        if (sg_cur_cmdtbl != NULL)
            return -1;
        sg_cur_cmdtbl = args;
        strcpy(g_soc_cmd_prepend, "soc set ");
        sg_host_cmd = (HDR_HostCmd *)sg_cmd_buf;
        sg_host_cmd->len = HOST_CMD_HDR_LEN;
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--END--")==0) {
        if (sg_cur_cmdtbl == NULL)
            return -1;
        //ssv6xxx_raw_dump(sg_host_cmd, sg_host_cmd->len);
        res = send_cfg_cmd(sg_host_cmd, sg_host_cmd->len-HOST_CMD_HDR_LEN, args);
        sg_cur_cmdtbl = NULL;
        g_soc_cmd_prepend[0] = 0x00;
        sg_host_cmd = NULL;
        return res;
    }
    
    pos = (char *)sg_host_cmd + sg_host_cmd->len;
    for(i=0; i<argc; i++) {
        //LOG_PRINTF("  [%d]: ==> %s\n", i, argv[i]);
        *(u32 *)pos = (u32)ssv6xxx_atoi(argv[i]);
        sg_host_cmd->len += sizeof(u32);
        pos += sizeof(u32);
    }
    return 0;

}
static s32 set_tx_rx_check(void *args, s32 argc, char *argv[])
{	 

	struct cfg_host_cmd *host_cmd;
    s32 i, len=sizeof(u32);
    char *str[] = { "disable", "enable" };
    
    if (argc != 2)
        return -1;
	
    for(i=0; i<sizeof(str)/sizeof(u8 *); i++) {
        if (strcmp(argv[1], str[i]) == 0) {

			host_cmd = (HDR_HostCmd *)sg_cmd_buf;
			host_cmd->un.dat32[0] = (u32)i;
            return send_cfg_cmd(host_cmd, len, args);
        }
    }
	return -1;
}

static s32 show_all(void *args, s32 argc, char *argv[])
{
	struct cfg_host_cmd *host_cmd;
    s32	len=sizeof(u32);
    
    //if (argc != 2)
    //    return -1;
    
	host_cmd = (HDR_HostCmd *)sg_cmd_buf;
	host_cmd->un.dat32[0] = 0;
    return send_cfg_cmd(host_cmd, len, args);
        
    
	return -1;
}



#if 0
static s32 show_all(void *args, s32 argc, char *argv[])
{

    time_t curtime=time(NULL);
    struct tm *l_time=localtime(&curtime);
	int i=0;

#ifdef __SSV_UNIX_SIM__
	sleep(1000);
#else
	Sleep(60);
#endif
    PRINTF("\n");
    PRINTF("Date: %s\n", asctime(l_time));
    PRINTF("===================================================================\n");
    PRINTF("                 <<Global Configuration>>\n");
    PRINTF("===================================================================\n");
    PRINTF("Register        Value\n");
    PRINTF("--------------  ---------------------------------------------------\n");

    PRINTF("\n\n");
    PRINTF("SW VARs: \n");
    PRINTF("   gTxFlowDataReason=0x%08x\n", gTxFlowDataReason);
    PRINTF("   gTxFlowMgmtReason=0x%08x\n", gTxFlowMgmtReason);
    PRINTF("   gRxFlowDataReason=0x%08x\n", gRxFlowDataReason);
    PRINTF("   gRxFlowMgmtReason=0x%08x\n", gRxFlowMgmtReason);
    PRINTF("-------------------------------------------------------------------\n\n\n");
    PRINTF("------------------------ HW Register Dump -------------------------\n");

    PRINTF("--------------------------- HW Register ---------------------------\n");
    for(i=0; i<BANK_COUNT; i++)
        sim_reg_dump_bank(i);
    PRINTF("----------------------------- end ---------------------------------\n");
	sim_key_dump();	
    return 0;
	
}

#endif

static s32 set_multi_mac_mode(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32);
    u32 mode = 0;

    if (argc != 2)
        return -1;
	
    mode = (u32)ssv6xxx_atoi(argv[1]);
    if (mode > 7)
        return -1;

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat8[0] = mode;
    return send_cfg_cmd(host_cmd, len, args); 
}

static s32 set_rx_tods_mask(void *args, s32 argc, char *argv[])
{
    struct cfg_host_cmd *host_cmd;
    s32 len=sizeof(u32);
    u32 mask = 0;

    if (argc != 2)
        return -1;

    mask = (u32)ssv6xxx_atoi(argv[1]);
    if (mask > 1)
        return -1;

    host_cmd = (HDR_HostCmd *)sg_cmd_buf;
    host_cmd->un.dat8[0] = mask;
    return send_cfg_cmd(host_cmd, len, args);
}
/*---------------------------- Command Table --------------------------------*/


static struct wsimp_cfg_cmd_tbl cfg_cmd_table[] =
{
    { "STA-OPMODE",         SSV6XXX_HOST_CMD_SET_OPMODE,        set_sta_opmode,         NULL,               NULL },
    { "STA-BSSID",          SSV6XXX_HOST_CMD_SET_BSSID,         set_sta_bssid,          NULL,               NULL },
    { "STA-MAC",            SSV6XXX_HOST_CMD_SET_STA_MAC,       set_sta_mac,            NULL,               NULL },
    { "STA-WSID",           SSV6XXX_HOST_CMD_SET_WSIDTBL,       set_wsid_entry,         NULL,               NULL },
    { "STA-QOS-CAP",        SSV6XXX_HOST_CMD_SET_QOS_CAP,       set_enable_disable,              NULL,               NULL },
    { "PKTRX-OFFSET",       SSV6XXX_HOST_CMD_SET_PBUF_OFFSET,   set_pkt_offset,           NULL,               NULL },
    { "STA-MCFILTER",       SSV6XXX_HOST_CMD_SET_GMFLT,         set_mcfilter,             show_mcfilter,      NULL },
    { "TX-ETHER-TRAP",      SSV6XXX_HOST_CMD_SET_TX_ETHTRAP,    set_tx_eth_trap,        show_eth_trap,      NULL },
    { "RX-ETHER-TRAP",      SSV6XXX_HOST_CMD_SET_RX_ETHTRAP,    set_rx_eth_trap,        show_eth_trap,      NULL },
    { "HW-DUP-FILTER",      SSV6XXX_HOST_CMD_SET_DUP_FLT,       set_enable_disable,         NULL,               NULL },
    //{ "NULL-TRAP",          0x00, set_null_trap,          NULL,               NULL },
    { "PROMIS-MODE",        SSV6XXX_HOST_CMD_SET_PROMIS_MODE,   set_enable_disable,        NULL,               NULL },
    { "STA-PAIR-SECURITY",  SSV6XXX_HOST_CMD_SET_PAIR_SECURITY, set_pair_security,      NULL,               NULL },
    { "STA-GROUP-SECURITY", SSV6XXX_HOST_CMD_SET_GROUP_SECURITY,set_group_security,     NULL,               NULL },
    { "STA-PAIR",           SSV6XXX_HOST_CMD_SET_PAIR_ENTRY, 	set_pair_entry,     	NULL,               NULL },
    { "STA-GROUP",          SSV6XXX_HOST_CMD_SET_GROUP_ENTRY, 	set_group_entry,     	NULL,               NULL },
    { "TX-INFO-SECURITY",   SSV6XXX_HOST_CMD_SET_TX_INFO_SECURITY, set_tx_info_security,   NULL,               NULL },
    //{ "PVER-FILTER",        0x00, set_protov_filter,      NULL,               NULL },
    { "HT-MODE",            SSV6XXX_HOST_CMD_SET_HT_MODE,      set_ht_mode,            NULL,               NULL },
    { "STA-NAV",            SSV6XXX_HOST_CMD_SET_NAV,          set_sta_nav,            NULL,               NULL },
    { "HDR-STRIP-OFF",      SSV6XXX_HOST_CMD_STRIP_OFF,        set_enable_disable,      NULL,               NULL },
    { "HCI-RX2HOST",        SSV6XXX_HOST_CMD_SET_RX2HOST,      set_enable_disable,        NULL,               NULL },
    { "RX-SNIFFER",         SSV6XXX_HOST_CMD_SET_RXSNIFFER,    set_enable_disable,        NULL,               NULL },
    { "REASON-TRAP",        SSV6XXX_HOST_CMD_SET_TRAP_MASK,    set_reason_trap,        NULL,               NULL },
    { "SEQNO-CTRL",         SSV6XXX_HOST_CMD_SET_GLOBAL_SEQCTRL, set_seqno_ctrl,         NULL,               NULL },
    { "TX-FLOW-DATA",       SSV6XXX_HOST_CMD_SET_FCMD_TXDATA,  set_tx_flow_data,       NULL,               NULL },
    { "TX-FLOW-MGMT",       SSV6XXX_HOST_CMD_SET_FCMD_TXMGMT,  set_tx_flow_mgmt,       NULL,               NULL },
    { "TX-FLOW-CTRL",       SSV6XXX_HOST_CMD_SET_FCMD_TXCTRL,  set_tx_flow_ctrl,       NULL,               NULL },
    { "RX-FLOW-DATA",       SSV6XXX_HOST_CMD_SET_FCMD_RXDATA,  set_rx_flow_data,       NULL,               NULL },
    { "RX-FLOW-MGMT",       SSV6XXX_HOST_CMD_SET_FCMD_RXMGMT,  set_rx_flow_mgmt,       NULL,               NULL },
    { "RX-FLOW-CTRL",       SSV6XXX_HOST_CMD_SET_FCMD_RXCTRL,  set_rx_flow_ctrl,       NULL,               NULL },
    { "STA-SIFS",           SSV6XXX_HOST_CMD_SET_SIFS,         set_sifs,               NULL,               NULL },
    { "STA-DIFS",           SSV6XXX_HOST_CMD_SET_DIFS,         set_difs,               NULL,               NULL },
    { "STA-EIFS",           SSV6XXX_HOST_CMD_SET_EIFS,         set_eifs,               NULL,               NULL },
    { "MGMT-TXQID",         SSV6XXX_HOST_CMD_SET_MGMT_TXQID,    set_mgmt_txqid,         NULL,               NULL },
    { "NONQOS-TXQID",       SSV6XXX_HOST_CMD_SET_NONQOS_TXQID,  set_nonqos_txqid,       NULL,               NULL },
    { "AUTO-SEQNO",         SSV6XXX_HOST_CMD_SET_AUTO_SEQNO,    set_enable_disable,         NULL,               NULL },
    { "RX-NULL-DATA-TRAP",  SSV6XXX_HOST_CMD_SET_RX_NULL_DATA_TRAP,    set_enable_disable,         NULL,               NULL },    
    { "RX-INFO-SIZE",       SSV6XXX_HOST_CMD_SET_RX_INFO_SIZE, set_tx_rx_info_size,        NULL,               NULL },
    { "ERP-PROTECT",        SSV6XXX_HOST_CMD_SET_ERP_PROTECT,   set_erp_protect,        NULL,               NULL },
    { "MRX-DECITBL",        SSV6XXX_HOST_CMD_SET_DECITBL,       set_decision_table, NULL,               NULL },
//    { "MRX-DECIMSK",        0x00, set_mrx_decision_mask,  NULL,               NULL },
	{ "WMM-PARAM",			SSV6XXX_HOST_CMD_SET_WMM_PARAM		, set_wmm_param,		  NULL,				  NULL },
	{ "WMM-RAMDOM",			SSV6XXX_HOST_CMD_SET_WMM_RANDOM		, set_wmm_random,		  NULL,				  NULL },
	{ "WMM-TX-PACKET-CNT", 	SSV6XXX_HOST_CMD_SET_TX_PACKET_CNT	, set_tx_packet_cnt,	  NULL,				  NULL },
	{ "WMM-QUEUE_HALT", 	SSV6XXX_HOST_CMD_SET_TX_QUEUE_HALT	, set_tx_queue_halt,	  NULL,				  NULL },
	{ "WMM-TXOP-BY-FRAME", 	SSV6XXX_HOST_CMD_SET_TXOP_SUB_FRM_TIME, set_enable_disable,	  NULL,				  NULL },
    { "BCN-TIMER-EN", 	SSV6XXX_HOST_CMD_SET_BCN_TIMER_EN, set_enable_disable,	  NULL,				  NULL },

//	{ "MAC-TO-MAC",			0x00, set_mac_to_mac,		  NULL,				  NULL },
//    { "FPGA-LOOPBACK",		0x00, set_fpga_loopbak,		  NULL,				  NULL },
    { "MAC-MODE",           SSV6XXX_HOST_CMD_SET_MAC_MODE,        set_mac_mode,           NULL,             NULL   },
    { "PHY-TBL",            SSV6XXX_HOST_CMD_SET_PHY_INFO_TBL,    set_phy_info,           NULL,             NULL   },    
//    { "WDS-SUPPORT",        CFG_WSD_SUPPORT,    set_wds_support,        NULL                    },
     


//    { "INIT-PHY-TABLE",     SSV6XXX_HOST_CMD_INIT_PHY_TABLE,        init_phy_table,           NULL,             NULL   },
    /* IEEE 802.11 MIB attributes: */
    //{ "MIB",                SSV6XXX_HOST_CMD_SET_MIB,    set_mib,                show_mib, NULL                },    
    { "TxRx",              SSV6XXX_HOST_CMD_SET_RX_CHECK,    set_enable_disable,     NULL , NULL                }, 
    //{ "all",                0x00,             NULL  , show_all    , NULL                },
    { "all",               SSV6XXX_HOST_CMD_SHOW_ALL,     NULL, show_all  , NULL                },
	{ "MULTI-MAC",               SSV6XXX_HOST_CMD_SET_MULTI_MAC_MODE,     set_multi_mac_mode, NULL  , NULL                },
	{ "RX-TODS-MASK",               SSV6XXX_HOST_CMD_SET_RX_TODS_MASK,     set_rx_tods_mask, NULL  , NULL                },
    { NULL, 0x00, NULL, NULL, NULL },
};

/*---------------------------------------------------------------------------*/


s32 wsimp_cfg_regs(void *args, s32 argc, char *argv[])
{    
    struct wsimp_cfg_cmd_tbl *cmdtbl;
    s32 i, sargc, res;
    char *sargv[100], buffer[1024];

    for(buffer[0]=0x00, i=1; i<argc; i++) {
        strcat(buffer, argv[i]);
        strcat(buffer, " ");
    }
    for(i=0; i<(s32)strlen(buffer); i++) {
        if (buffer[i]=='[' || buffer[i]==']')
            buffer[i] = ' ';
    }
    sargc = wsimp_tokenlize(buffer, sargv);

    if (sargc <= 0)
        return -1;

    if (sg_cur_cmdtbl != NULL) {
        return sg_cur_cmdtbl->set_func(
            (void *)sg_cur_cmdtbl,
            sargc, sargv
        );
    }
    ssv6xxx_memset((void *)sg_cmd_buf, 0, sizeof(sg_cmd_buf));
    
    for(i=0, cmdtbl=cfg_cmd_table; cmdtbl->cmd!=NULL; i++, cmdtbl++)  {
        if (strcmp(sargv[0], cmdtbl->cmd) == 0) {
            if (!strcmp(argv[0], "set") && cmdtbl->set_func != NULL) {
                cmdtbl->args = args;
                res = cmdtbl->set_func((void *)cmdtbl, sargc, sargv);
                return res;
            }
            if (!strcmp(argv[0], "show") && cmdtbl->show_func != NULL) {
				 cmdtbl->args = args;
                res = cmdtbl->show_func((void *)cmdtbl, sargc, sargv);
                return res;
            }
            return -1;
        }
    }  
	
    return -1;
}





