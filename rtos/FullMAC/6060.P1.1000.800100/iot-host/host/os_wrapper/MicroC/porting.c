#include "config.h"
#include "log.h"
#include "host_global.h"
#include "rtos.h"
#include "porting.h"
#include "os_cfg.h"
#if ((OS_CPU_HOOKS_EN>0)&&(OS_TASK_STAT_EN==1))
#include <os.h>
#include "drv_l1_timer.h"
#include "drv_l1_sfr.h"


extern u8 cmd_top_enable;
u32 lastTinyCount=0;
u32 currentTinyCount=0;
u32 lastTickCount=0;
u32 currentTickCount=0;

u32 lastTinyCountStatTask=0;
u32 currentTinyCountStatTask=0;
u32 lastTickCountStatTask=0;
u32 currentTickCountStatTask=0;

u32 acc_cpu_usage=0;
u32 acc_counts=0;

u32 OSCtxSwCtrPerSec;
#define TINY_COUNT(x) ((x*96)/144) // tiny_counter_get is the reference
#define TINY_COUNT_TO_US(x) (((x*266)/100) )
#endif

#if OS_TASK_CREATE_EXT_EN > 0
TASK_USER_DATA taskUserData[64];
#endif

#if (OS_CPU_HOOKS_EN > 0) && (OS_TASK_SW_HOOK_EN > 0) && (OS_TASK_CREATE_EXT_EN>0)
void output_message(char *s)
{
    char *p=s;
    while(*p!=NULL){
        hal_putchar(*p);
        p++;
    }

}
void TaskSwHook(void)
{
    OS_TCB      *ptcb;
    u32 *p=NULL;
    TASK_USER_DATA *puser=OSTCBCur->OSTCBExtPtr;
#if (OS_TASK_STAT_EN==1)
    //**************************************//
    //*********** Profile Task *********** //
    //**************************************//
    u32 time;
    currentTinyCount=R_TIMERD_UPCOUNT&0xFFFF;
    currentTickCount=OSTime;
    if((puser != (TASK_USER_DATA *)0)&&(cmd_top_enable==1)){

        puser->TaskCtr++;

        //Tiny counter as reference
        time = ((currentTinyCount|0x10000)-lastTinyCount)&0xFFFF;
        puser->TaskTotExecTimeFromTiny += time;

        //Tick counter as reference
        if(lastTickCount>currentTickCount)
            time = ((0xFFFFFFFF-lastTickCount+1)+currentTickCount);
        else
            time = (currentTickCount-lastTickCount);

        puser->TaskTotExecTimeFromTick += time;

    }
    lastTinyCount=currentTinyCount;
    lastTickCount=currentTickCount;
#endif

    //**************************************//
    //*********** check stack *********** //
    //**************************************//
    #if OS_STK_GROWTH == 1
    if(puser != (TASK_USER_DATA *)0){
        p = OSTCBCur->OSTCBStkBottom;
        if((*p!=0)||(*(p+1)!=0)||(OSTCBCur->OSTCBStkPtr<=p)){
            output_message("!!!STACK OVERFLOW!!! ");
            output_message(puser->TaskName);
            while(1){;}
        }
    }
    #else
     #To Do
    #endif

}
#endif

#if ((OS_CPU_HOOKS_EN>0)&&(OS_TASK_STAT_EN==1))
void TaskStatHook(void)
{
    u32 cpu_isr=0;
    cpu_isr=OS_EnterCritical();
    currentTinyCountStatTask=R_TIMERD_UPCOUNT&0xFFFF;
    currentTickCountStatTask=OSTime;

    if(cmd_top_enable==1){
        //LOG_PRINTF("current time=%d(us)\r\n",TINY_COUNT_TO_US(TINY_COUNT(R_TIMERD_UPCOUNT)));
        DispTaskStatus();
    }else if(cmd_top_enable==2){
        acc_cpu_usage+=OSCPUUsage;
        acc_counts++;
        if ((acc_cpu_usage > 4294967195) || (acc_counts > 4294967294))
        {
            u32 avg_cpu = (acc_cpu_usage/acc_counts);
            LOG_PRINTF("Update Average CPU usage = %d%% to avoid overflow\r\n", avg_cpu);
            acc_cpu_usage = 0;
            acc_counts = 0;
        }
    }
    else{
        ClearTaskStatus();
    }

    lastTinyCountStatTask=currentTinyCountStatTask;
    lastTickCountStatTask=currentTickCountStatTask;

    OS_ExitCritical(cpu_isr);
}

void DispTaskStatus(void)
{
    u8 i=0;
    u32 time2=0;
    u32 time1=0;
    OSCtxSwCtrPerSec=OSCtxSwCtr;
    OSCtxSwCtr=0;
    LOG_PRINTF("\r\n");
#if (OS_CPU_HOOKS_EN > 0) && (OS_TASK_SW_HOOK_EN > 0) && (OS_TASK_CREATE_EXT_EN>0)
    LOG_PRINTF("\33[32m%-30s %-10s %-10s %-10s %-10s\33[0m\r\n","Task Name","Priority","Counter","Tiny Time(us)","Tick Time(ms)");
    LOG_PRINTF("\r\n");
    for(i=0;i<sizeof(taskUserData)/sizeof(TASK_USER_DATA);i++){
        if(1==taskUserData[i].valid){
        LOG_PRINTF("%-30s %2d      %5d       %5u            %5u                 \r\n",
                taskUserData[i].TaskName,i,taskUserData[i].TaskCtr, TINY_COUNT_TO_US(TINY_COUNT(taskUserData[i].TaskTotExecTimeFromTiny)) ,taskUserData[i].TaskTotExecTimeFromTick*10 );
        taskUserData[i].TaskCtr=0;
        taskUserData[i].TaskTotExecTimeFromTiny=0;
        taskUserData[i].TaskTotExecTimeFromTick=0;
        }
    }
#endif
    LOG_PRINTF("\r\n");
    LOG_PRINTF("CPU usage = %d%% \r\n",OSCPUUsage);

    time1=((currentTinyCountStatTask|0x10000)-lastTinyCountStatTask)&0xFFFF;
    if(lastTickCountStatTask>currentTickCountStatTask)
        time2 = ((0xFFFFFFFF-lastTickCountStatTask+1)+currentTickCountStatTask);
    else
        time2 = (currentTickCountStatTask-lastTickCountStatTask);

    LOG_PRINTF("Calculate interval = %u(us,Tiny) %u(ms,Tick) \r\n",
            TINY_COUNT_TO_US(TINY_COUNT(time1)),time2*10);
}

void ClearTaskStatus(void)
{
    
#if (OS_CPU_HOOKS_EN > 0) && (OS_TASK_SW_HOOK_EN > 0) && (OS_TASK_CREATE_EXT_EN>0)
    u8 i=0;
    u32 avg_cpu = 0;
    if(acc_counts != 0)
    {
        avg_cpu = (acc_cpu_usage/acc_counts);
        LOG_PRINTF("Average CPU usage = %d%% \r\n", avg_cpu);
        acc_cpu_usage = 0;
        acc_counts = 0;
    }
    for(i=0;i<sizeof(taskUserData)/sizeof(TASK_USER_DATA);i++){
        if(1==taskUserData[i].valid){
        taskUserData[i].TaskCtr=0;
        taskUserData[i].TaskTotExecTimeFromTiny=0;
        taskUserData[i].TaskTotExecTimeFromTick=0;
        }
    }
#endif
    OSCtxSwCtr=0;    
}
#else
void TaskStatHook(void)
{
}
#endif
u8 hal_getchar(void)
{
    u8 data=0;
#if (GP_PLATFORM_SEL == GP_19B)
    if(STATUS_OK == uart0_data_get(&data, 0))
#elif (GP_PLATFORM_SEL == GP_15B)
    if(STATUS_OK == drv_l1_uart1_data_get(&data, 0))
#endif
        return data;
    else
        return 0;
}

