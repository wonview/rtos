#include <config.h>
#include <ssv_regs.h>
#include <log.h>
#include <pbuf.h>

#include <os_wrapper.h>



#if (CONFIG_USE_LWIP_PBUF==0) 


/**
 * Define Data structure and static global variables for simulation
 * and emulation use only.
 */
typedef struct HW_PBUF_Info_st 
{
    u16         valid;
    u16         pbuf_id;
    u16         size;
    u32         pa;     /* physical address */
    u32         va;     /* virtual address */

    /* logging infomation: */
    u32         pktid;  /* Packet ID */
    u32         fragid;
    u8          parsed; /* 0: not parsed frame, 1: parsed frame */
    u8          drop;   /* 0: not drop, 1: drop */
    u32         rx2host; /* 0: not to host, 1: to host format */
    u8          flow_buffer[1024];
    
} HW_PBUF_Info, *PHW_PBUF_Info;

#define PBUF_MAX_NO                     128
#define PBUF_VA_BASE                    0x8000





static HW_PBUF_Info HW_PBUF_Pool[PBUF_MAX_NO];
static OsMutex PBUF_LogMutex;
static OsMutex sg_pbuf_mutex;





/**
 *  Packet buffer driver for SSV6200 Hardware Packet Buffer Manipulation.
 *  Three APIs are implemented for this driver:
 *
 *      @ PBUF_Init()           -Initialize hardware packet buffer setting 
 *                                      and a mutex.
 *      @ PBUF_MAlloc()       -Request a packet buffer from hardware.      
 *      @ PBUF_MFree()        -Release a packet buffer to hardware.
 *
 */

#if CONFIG_STATUS_CHECK
u16 g_pbuf_max;
u16 g_pbuf_used;
#endif

s32 PBUF_Init(void)
{
#if (CONFIG_HOST_PLATFORM == 1)
    /* For simulation/emulation only */
    ssv6xxx_memset((void *)&HW_PBUF_Pool, 0, sizeof(HW_PBUF_Info)*PBUF_MAX_NO);
    OS_MutexInit(&PBUF_LogMutex);
#endif    
    OS_MutexInit(&sg_pbuf_mutex);


#if  CONFIG_STATUS_CHECK
    g_pbuf_max = 0;
    g_pbuf_used = 0;
#endif //CONFIG_STATUS_CHECK

    return OS_SUCCESS;
}




#if CONFIG_STATUS_CHECK
void SSV_PBUF_Status(void){
    LOG_PRINTF("MEM SSV PBUF\n\t");    
    LOG_PRINTF("avail: %d\r\n", PBUF_MAX_NO-g_pbuf_used); 
    LOG_PRINTF("used: %d\r\n", g_pbuf_used); 
    LOG_PRINTF("max: %d\r\n", g_pbuf_max);     
}
#endif//CONFIG_STATUS_CHECK

void *PBUF_MAlloc_Raw(u32 size, u32 need_header, PBuf_Type_E buf_type)
{
    PKT_Info *pkt_info;
	u32 extra_header=0;
    OS_MutexLock(sg_pbuf_mutex);
    if (need_header)
    {
        extra_header = (PBU_OFFSET+ TXPB_RVSD*TX_PKT_RES_BASE);
	    size += extra_header;
    }
    //printf("PBUF_MAlloc_Raw len[%d]\n");
    {
        s32 i;
        for(i=0; i<PBUF_MAX_NO; i++) {
            if (HW_PBUF_Pool[i].valid == 1) 
                continue;
            pkt_info = (PKT_Info *)MALLOC(size);
            assert(pkt_info != NULL);
            ssv6xxx_memset((void *)pkt_info, 0, size);
    		ssv6xxx_memset((void *)&HW_PBUF_Pool[i], 0, sizeof(HW_PBUF_Info));
            HW_PBUF_Pool[i].valid   = 1;
            HW_PBUF_Pool[i].pbuf_id = i;
            HW_PBUF_Pool[i].size    = size;
            HW_PBUF_Pool[i].pa      = (u32)pkt_info;
            HW_PBUF_Pool[i].va      = (PBUF_VA_BASE+i)<<16;
            HW_PBUF_Pool[i].pktid   = 0xFFFFFFFF;
            HW_PBUF_Pool[i].fragid  = 0xFFFFFFFF;
            break;        
        }
        if (i >= PBUF_MAX_NO)
            pkt_info = NULL;
        else{
#if  CONFIG_STATUS_CHECK
            g_pbuf_used++;
            if(g_pbuf_max < g_pbuf_used)
               g_pbuf_max = g_pbuf_used; 
#endif
        }
                
    }

    OS_MutexUnLock(sg_pbuf_mutex);    

    return (void *)pkt_info;
}
static void __PBUF_MFree_0(void *PKTMSG)
{   
    /**
        * The following code is for simulation/emulation platform only.
        * In real chip, this code shall be replaced by manipulation of 
        * hardware packet engine.
        */ 
    {
        s32 i;
        for(i=0; i<PBUF_MAX_NO; i++) {
            if (HW_PBUF_Pool[i].valid== 0)
                continue;
            if ((u32)HW_PBUF_Pool[i].pa != (u32)PKTMSG)
                continue;
            FREE(PKTMSG);
            HW_PBUF_Pool[i].valid = 0;

#if  CONFIG_STATUS_CHECK
            g_pbuf_used--;
#endif                                    
            break;
        }
        if(i >= PBUF_MAX_NO)
            ASSERT(i < PBUF_MAX_NO);
    }

}

static inline void __PBUF_MFree_1(void *PKTMSG)
{   
    OS_MutexLock(sg_pbuf_mutex);

    __PBUF_MFree_0(PKTMSG);    

    OS_MutexUnLock(sg_pbuf_mutex);
}

void _PBUF_MFree (void *PKTMSG)
{
	if (gOsFromISR)
		__PBUF_MFree_0(PKTMSG);
	else
		__PBUF_MFree_1(PKTMSG);
}


#endif//#if CONFIG_USE_LWIP_PBUF 




