#ifndef _TXRX_TASK_H
#define _TXRX_TASK_H

#include <host_apis.h>
#include <common.h>

#define INT_RX              0x00000001  //1<<0
#define INT_TX              0x00000002  //1<<1
#define INT_SOC             0x00000004  //1<<2
#define INT_LOW_EDCA_0      0x00000008  //1<<3
#define INT_LOW_EDCA_1      0x00000010  //1<<4
#define INT_LOW_EDCA_2      0x00000020  //1<<5
#define INT_LOW_EDCA_3      0x00000040  //1<<6
#define INT_RESOURCE_LOW    0x00000080  //1<<7

s32 TXRXTask_Init();
s32 TXRXTask_FrameEnqueue(void* frame, u32 priority);
ssv6xxx_result TXRXTask_SetOpMode(ModeType mode);
void TXRXTask_Isr(u32 signo);

void TXRXTask_ShowSt();

#endif //_TXRX_TASK_H
