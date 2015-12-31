#include "config.h"
#include "rtos.h"
#include "os_cfg.h"
#include <log.h>


volatile u8 gOsFromISR;

#define STATIC_STK
#ifdef STATIC_STK
//(TOTAL_STACK_SIZE<<4) --> transfer unit to bytes
//((TOTAL_STACK_SIZE<<4)>>2) --> transfer unit to word
u32 task_stack[((TOTAL_STACK_SIZE<<4)>>2)];
u32 stk_used=0;
OsMutex StkMutex;
#endif
OS_APIs s32  OS_Init( void )
{
    #ifdef __WIN32__
    extern unsigned long ulCriticalNesting;

    /**
        *  Note!! ulCriticalNesting is a FreeRTOS gobal variable. Before
        *  using Mutex, we shall reset it to 0.
        */
    ulCriticalNesting = 0;
    #endif // __WIN32__
    gOsFromISR = 0;
    //enable uc-os timer
    timer_freq_setup(0, OS_TICKS_PER_SEC, 0, OSTimeTick);
#ifdef STATIC_STK
    OS_MutexInit(&StkMutex);
    //OS_MemSET((void *)wifi_task_stack,0,sizeof(wifi_task_stack));
#endif

#if OS_TASK_CREATE_EXT_EN > 0
    ssv6xxx_memset(taskUserData,0,sizeof(taskUserData));
#endif
#if(OS_TASK_STAT_EN==1)
    OSStatInit();
#endif
#ifdef STATIC_STK
    LOG_PRINTF("Total Stack size is 0x%x (bytes)\r\n",sizeof(task_stack));
#endif
    return OS_SUCCESS;
}

OS_APIs unsigned long OS_Random(void)
{
	return OSTimeGet()%65536+54*18;
}

OS_APIs void OS_Terminate( void )
{
    //vTaskEndScheduler();
}

OS_APIs u32 OS_EnterCritical(void)
{
    OS_CPU_SR cpu_sr;
    OS_ENTER_CRITICAL()
    return cpu_sr;
}

OS_APIs void OS_ExitCritical(u32 val)
{
    OS_CPU_SR cpu_sr = val;
    OS_EXIT_CRITICAL()
}

/* Task: */

OS_APIs s32 OS_TaskCreate( OsTask task, const char *name, u32 stackSize, void *param, u32 pri, OsTaskHandle *taskHandle )
{
    u32* stk_ptr = NULL;
    s32 ret;
#ifdef STATIC_STK
    u32 stk_size=(stackSize>>2); //transfer unit to word
#endif

#ifdef STATIC_STK
    OS_MutexLock(StkMutex);
    if((stk_used+stk_size)>(sizeof(task_stack)>>2)){
        LOG_ERROR("Stack is not enough for new task %s\r\n",name);
        ASSERT(FALSE);
    }
    stk_ptr=&task_stack[stk_used];
    stk_used+=stk_size;
    LOG_PRINTF("Free Stack size is 0x%x (bytes)\r\n",(sizeof(task_stack)-(stk_used<<2)));
    OS_MutexUnLock(StkMutex);
#else
    stk_ptr = (void*)OS_MemAlloc(stackSize);
    if(! stk_ptr){
        LOG_ERROR("alloc %s stack fail\n",name);
        return OS_FAILED;
    }
#endif
    LOG_PRINTF("OS_TaskCreate =%s,pri=%d,stackSize=%d, stack top=0x%x, stack botton=0x%x\r\n",name,pri,stackSize,&stk_ptr[stk_size-1],&(stk_ptr[0]));

#if OS_TASK_CREATE_EXT_EN > 0
    ssv6xxx_strcpy(taskUserData[pri].TaskName,name);
    LOG_PRINTF("task name:%s,%d\r\n",taskUserData[pri].TaskName,pri);
    taskUserData[pri].TaskCtr=0;
    taskUserData[pri].TaskTotExecTimeFromTiny=0;
    taskUserData[pri].TaskTotExecTimeFromTick=0;
    taskUserData[pri].valid=1;
#endif
#if (OS_STK_GROWTH==0)
    ret = (s32)OSTaskCreate(task, param, stk_ptr, (INT8U)pri);
#else
#ifdef STATIC_STK
#if OS_TASK_CREATE_EXT_EN > 0
        ret = (s32)OSTaskCreateExt(task, param, &(stk_ptr[stk_size - 1]), (INT8U)pri,
                                (INT8U)pri,&(stk_ptr[0]),stk_size,&taskUserData[pri],OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
#else
        ret = (s32)OSTaskCreate(task, param, &(stk_ptr[stk_size - 1]), (INT8U)pri);
#endif
#else
    ret = (s32)OSTaskCreate(task, param, (((u32 *)stk_ptr)+(stackSize/4) - 1), (INT8U)pri);
#endif
#endif
    if(ret == OS_NO_ERR)
        return OS_SUCCESS;
    else
        LOG_ERROR("OSTaskCreate ret=%d \r\n",ret);

    return OS_FAILED;
}


OS_APIs void OS_TaskDelete(OsTaskHandle taskHandle)
{
    //vTaskDelete(taskHandle);
    OSTaskDel(OS_PRIO_SELF);
}



OS_APIs void OS_StartScheduler( void )
{
    //vTaskStartScheduler();
}

OS_APIs u32 OS_GetSysTick(void)
{
    return OSTimeGet();
}


/* Mutex APIs: */
OS_APIs s32 OS_MutexInit( OsMutex *mutex )
{
    u8 error;

    //*mutex = xSemaphoreCreateMutex();
    //*mutex = OSMutexCreate(0, &error);
    *mutex = OSSemCreate(1);

    if ( NULL == *mutex ){
        return OS_FAILED;
    }
    else{
        return OS_SUCCESS;
    }
}


OS_APIs void OS_MutexLock( OsMutex mutex )
{
    u8 err;
    //xSemaphoreTake( mutex, portMAX_DELAY);
    //OSMutexPend(mutex,0,&err);
    //LOG_PRINTF("+\r\n");
    OSSemPend(mutex, 0, &err);
    if(err!=OS_NO_ERR) assert(FALSE);
}



OS_APIs void OS_MutexUnLock( OsMutex mutex )
{
    //xSemaphoreGive( mutex );
    //OSMutexPost(mutex);
    //LOG_PRINTF("-\r\n");
    if(OS_NO_ERR!=OSSemPost(mutex)) assert(FALSE);
}

OS_APIs void OS_MutexDelete( OsMutex mutex )
{
    u8 err;

    //OSMutexDel(mutex, OS_DEL_ALWAYS, &err);
    OSSemDel(mutex, OS_DEL_ALWAYS, &err);
}

OS_APIs void OS_MsDelay(u32 ms)
{

    u8 sec = 0;
    if(ms >= 1000){
        sec = ms/1000;
        ms = ms%1000;
    }

    OSTimeDlyHMSM(0,0,sec,ms);
}

OS_APIs s32 OS_SemInit( OsSemaphore* Sem, u16 maxcnt, u16 cnt)
{
    u8 error;

    *Sem = OSSemCreate(cnt);
    if ( NULL == *Sem ){
        return OS_FAILED;
    }
    else{
        return OS_SUCCESS;
    }
}

OS_APIs bool OS_SemWait( OsSemaphore Sem , u16 timeout)
{
    u8 err;

    OSSemPend(Sem, timeout, &err);
    if (err != OS_NO_ERR) {
        return OS_FAILED;
    } else {
        return OS_SUCCESS;
    }
}

OS_APIs u8 OS_SemSignal( OsSemaphore Sem)
{
    u8 ret;
    ret = OSSemPost(Sem);
    if(ret != OS_NO_ERR)
        LOG_PRINTF("SemPost err=%d\r\n",ret);

	return ret;
}

OS_APIs void OS_SemDelete(OsSemaphore Sem)
{
    u8 err;
	OSSemDel(Sem, OS_DEL_ALWAYS, &err);
}
extern void * gp_malloc(u32 size);                          // SDRAM allocation
OS_APIs void *OS_MemAlloc( u32 size )
{
    /**
        *  Platform dependent code. Please rewrite
        *  this piece of code for different system.
        */
    void * ptr = NULL;
    ptr = gp_malloc(size);
    if (ptr) gp_memset(ptr,0,size);
    return ptr;
}

extern void gp_free(void *ptr);                                 // Both SDRAM and IRAM can be freed by this function
/**
 *  We do not recommend using OS_MemFree() API
 *  because we do not want to support memory
 *  management mechanism in embedded system.
 */
OS_APIs void __OS_MemFree( void *m )
{
    /**
        *  Platform depedent code. Please rewrite
        *  this piece of code for different system.
        */
    gp_free(m);
}

void OS_MemCPY(void *pdest, const void *psrc, u32 size)
{
    OS_MemCopy((void*)pdest,(void*)psrc,size);
}

void OS_MemSET(void *pdest, u8 byte, u32 size)
{
    gp_memset(pdest,byte,size);
}

#define MSGQ_LOCK()		OSSchedLock()
#define MSGQ_UNLOCK()		OSSchedUnlock()

/* Message Queue: */
OS_APIs s32 OS_MsgQCreate( OsMsgQ *MsgQ, u32 QLen )
{
    u16 size = sizeof( OsMsgQEntry )*QLen;
    void* qpool = OS_MemAlloc(size+sizeof(OsMessgQ));
    OsMsgQ tmpq;

    if(qpool)
    {
        gp_memset((void *)qpool, 0, size);	/* clear out msg q structure */
        *MsgQ = tmpq = (OsMsgQ)qpool;
        tmpq->qpool = qpool;

        //get double size msg q length
        tmpq->msssageQ = OSQCreate((void*)(((u8*)qpool)+sizeof(OsMessgQ)), (size/4));
    }
    else
    {
        LOG_ERROR("%s,size=%d\r\n",__func__,size);
    }

    if ( NULL == *MsgQ )
        return OS_FAILED;
    return OS_SUCCESS;
}

OS_APIs s32 OS_MsgQDelete( OsMsgQ MsgQ)
{
    u8 err=0;
    MSGQ_LOCK();
    OSQDel(MsgQ->msssageQ, OS_DEL_ALWAYS, &err);
    OS_MemFree(MsgQ->qpool);
    MSGQ_UNLOCK();
    return ( 0!=err )? OS_FAILED: OS_SUCCESS;
}

OS_APIs s32 OS_MsgQEnqueue( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, bool fromISR )
{
    u8 err=0;
    MSGQ_LOCK();
    err = OSQPost(MsgQ->msssageQ, MsgItem->MsgData);
    MSGQ_UNLOCK();
    return ( 0!=err )? OS_FAILED: OS_SUCCESS;
}


OS_APIs s32 OS_MsgQDequeue( OsMsgQ MsgQ, OsMsgQEntry *MsgItem, u32 timeOut, bool fromISR )

{

    u8 err=0;

    MsgItem->MsgData = OSQPend(MsgQ->msssageQ, timeOut, &err);
    MsgItem->MsgCmd = 0;
    return ( 0!=err )? OS_FAILED: OS_SUCCESS;

}


OS_APIs s32 OS_MsgQWaitingSize( OsMsgQ MsgQ )
{
    //return ( uxQueueMessagesWaiting( MsgQ ) );
    return 0;
}



/* Timer: */
OS_APIs s32 OS_TimerCreate( OsTimer *timer, u32 ms, u8 autoReload, void *args, OsTimerHandler timHandler )
{
#if 0
    ms = ( 0 == ms )? 1: ms;
    *timer = xTimerCreate( NULL, OS_MS2TICK(ms), autoReload, args, timHandler);
    if ( NULL == *timer )
        return OS_FAILED;
    return OS_SUCCESS;
#endif
}

OS_APIs s32 OS_TimerSet( OsTimer timer, u32 ms, u8 autoReload, void *args )
{
#if 0
    if ( pdFAIL == xTimerChangeSetting( timer, OS_MS2TICK(ms), autoReload, args) )
        return OS_FAILED;
    return OS_SUCCESS;
#endif
}

OS_APIs s32 OS_TimerStart( OsTimer timer )
{
    //return xTimerStart( timer, 0 );
}

OS_APIs s32 OS_TimerStop( OsTimer timer )
{
    //return xTimerStop( timer, 0 );
}

OS_APIs void *OS_TimerGetData( OsTimer timer )
{
   // return pvTimerGetTimerID(timer);
}



#if 0
OS_APIs void OS_TimerGetSetting( OsTimer timer, u8 *autoReload, void **args )
{
    xTimerGetSetting(timer, autoReload, args);
}

OS_APIs bool OS_TimerIsRunning( OsTimer timer )
{
    return xTimerIsTimerActive(timer);
}

#endif




