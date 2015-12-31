#ifndef _HOST_GLOBAL_H_
#define _HOST_GLOBAL_H_

#define FW_STATUS_SIZE      (0xF)

#define FW_BUF_SIZE       (FW_BLOCK_SIZE)

#define FW_STATUS_REG   ADR_TX_SEG

#define FW_STATUS_MASK  (FW_STATUS_SIZE)<<28


#define BUF_SIZE                (FW_BLOCK_SIZE)

#define FW_BLOCK_CNT_SIZE       (0xFFF)                     //4096*FW_BLOCK_SIZE  support 4MB FW
#define FW_CHK_SUM_SIZE         (FW_BLOCK_CNT_SIZE)           //4096*FW_BLOCK_SIZE  support 4MB FW

#define FW_MASK             (0xFFFF<<16)
#define FW_BLOCK_CNT_MASK   (FW_BLOCK_CNT_SIZE)<<16
#define FW_CHK_SUM_MASK     (FW_CHK_SUM_SIZE)<<16
#define FW_STATUS_FW_CHKSUM_BIT      (1<<31)
#define FW_STATUS_HOST_CONFIRM_BIT   (1<<30)

//#define IS_WILDCARD_ADDR(m)             (!memcmp(&WILDCARD_ADDR, (m), ETHER_ADDR_LEN))
#define IS_EQUAL_MACADDR(m1, m2)        (!memcmp((m1), (m2), ETHER_ADDR_LEN))



extern struct task_info_st g_host_task_info[];
extern struct task_info_st g_mlme_task_info[];


extern u32 g_RunTaskCount;
//extern u32 g_MaxTaskCount;

//extern u8 g_soc_cmd_buffer[];

//extern u32 g_sim_net_up;
//extern u32 g_sim_link_up;


//extern ETHER_ADDR WILDCARD_ADDR;

extern u32 gTxFlowDataReason;
extern u32 gRxFlowDataReason;
extern u16 gTxFlowMgmtReason;
extern u16 gRxFlowMgmtReason;
extern u16 gRxFlowCtrlReason;

extern u32 g_free_msgevt_cnt;
extern u8 g_sta_mac[];


#if (CLI_HISTORY_ENABLE==1)

extern char gCmdHistoryBuffer[CLI_HISTORY_NUM][CLI_BUFFER_SIZE+1];
extern s8 gCmdHistoryIdx;
extern s8 gCmdHistoryCnt;

#endif//CLI_HISTORY_ENABLE
/* For cli_cmd_soc commands: */
//extern s32 g_soc_cmd_rx_ready;
//extern char g_soc_cmd_rx_buffer[];
extern char g_soc_cmd_prepend[];

void host_global_init(void);


#endif /* _HOST_GLOBAL_ */

