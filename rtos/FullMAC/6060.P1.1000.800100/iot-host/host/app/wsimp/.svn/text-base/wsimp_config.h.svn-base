#ifndef _WSIMP_CONFIG_H_
#define _WSIMP_CONFIG_H_

#define LEN_HOST_CMD                HOST_CMD_HDR_LEN
#define LEN_SOC_WSID                (27*sizeof(u32))
#define LEN_SOC_DECISION            (25*sizeof(u16))
#define LEN_SOC_CMDFLOW             (10*sizeof(u32))
#define LEN_SOC_ETHTRAP             (4*sizeof(u16))
#define LEN_SOC_BSSID               (ETHER_ADDR_LEN)
#define LEN_SOC_STA_MAC             (ETHER_ADDR_LEN)




#define INDEX_SOC_WSID              0
#define INDEX_SOC_DECISION          (INDEX_SOC_WSID+LEN_SOC_WSID+LEN_HOST_CMD)
#define INDEX_SOC_CMDFLOW           (INDEX_SOC_DECISION+LEN_SOC_DECISION+LEN_HOST_CMD)
#define INDEX_SOC_TX_ETHTRAP        (INDEX_SOC_CMDFLOW+LEN_SOC_CMDFLOW+LEN_HOST_CMD)
#define INDEX_SOC_RX_ETHTRAP        (INDEX_SOC_TX_ETHTRAP+LEN_SOC_ETHTRAP+LEN_HOST_CMD)
#define INDEX_SOC_BSSID             (INDEX_SOC_RX_ETHTRAP+LEN_SOC_BSSID+LEN_HOST_CMD)
#define INDEX_SOC_STA_MAC           (INDEX_SOC_BSSID+LEN_SOC_STA_MAC+LEN_HOST_CMD)


#define SOC_CMD_ADDR(x)             (((u8 *)&g_soc_cmd_buffer[x]) + LEN_HOST_CMD)


struct wsimp_cfg_cmd_tbl
{
    char              *cmd;
    u8              hcmd_id;
    s32             (*set_func)(void *args, s32 argc, char *argv[]);
    s32             (*show_func)(void *args, s32 argc, char *argv[]);
    void            *args;
};





s32 wsimp_cfg_regs(void *args, s32 argc, char *argv[]);


#endif /* _WSIMP_CONFIG_H_ */

