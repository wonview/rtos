#ifndef _HOST_CMD_ENGINE_H_
#define _HOST_CMD_ENGINE_H_

#include <host_apis.h>
#include <common.h>
#include <cmd_def.h>

struct CmdEng_st
{
    ModeType mode;
    u32 BlkCmdNum;
    bool BlkCmdIn;
};

void CmdEng_Task( void *args );
s32 CmdEng_Init( void );
ssv6xxx_result SSVHostCmdEng_Start( void );
ssv6xxx_result SSVHostCmdEng_Stop( void );
ssv6xxx_result CmdEng_SetOpMode(ModeType mode);
ssv6xxx_result CmdEng_GetStatus(struct CmdEng_st *st);
bool CmdEng_DbgSwitch();



// #define HOST_CMD_SCAN
// #define HOST_CMD_JOIN
// #define HOST_CMD_LEAVE
// #define HOST_CMD_IOCTL

#if 0
s32 HCmdEng_cmd_send(ssv6xxx_host_cmd_id CmdId, void *pCmdData, u32 nCmdLen);
#endif



#define AP_CMD_AP_MODE_ON			0
#define AP_CMD_AP_MODE_OFF			1
#define AP_CMD_ADD_STA				2
#define AP_CMD_PS_POLL_DATA     	3
#define AP_CMD_PS_TRIGGER_FRAME     4

#define PENDING_CMD_TIMEOUT  (1*1000) //msec
#define SCAN_CMD_TIMEOUT  (5*1000) //msec

#endif//_AP_H_