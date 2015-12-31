#include <config.h>
#include <msgevt.h>
#include <os_wrapper.h>
#include <cmd_def.h>
#include "ssv_drv.h"
#include "ssv_drv_if.h"
#include <time.h>
#if (defined _WIN32)
#include <drv/sdio/sdio.h>
#elif (defined __linux__)
#include <drv/sdio_linux/sdio.h>
#endif
#include <regs/ssv6200_reg.h>
#include "hctrl.h"
#include <txrx_hdl.h>

#define MAX_SSV6XXX_DRV     1
static s16                  s_drv_cnt;
static struct ssv6xxx_drv_ops*  s_drv_array[MAX_SSV6XXX_DRV];
static struct ssv6xxx_drv_ops   *s_drv_cur;

bool    ssv6xxx_drv_register(struct ssv6xxx_drv_ops *ssv_drv);
bool    ssv6xxx_drv_unregister(u16 i);

static bool _ssv6xxx_drv_started = false;
static OsSemaphore ssvdrv_rx_sphr;

OsMutex drvMutex;
#define OS_MUTEX_LOCK(x) { \
	SDRV_DEBUG("%s(): Mutex Lock\r\n",__FUNCTION__); \
    OS_MutexLock(drvMutex); \
}

#define OS_MUTEX_UNLOCK(x) { \
    OS_MutexUnLock(drvMutex); \
    SDRV_DEBUG("%s(): Mutex UnLock\r\n",__FUNCTION__); \
}
#ifndef __SSV_UNIX_SIM__
u32 free_tx_page=0;
u32 free_tx_id=0;

void ssvdrv_rx_isr(void)
{
    OS_SemSignal(ssvdrv_rx_sphr);
}

static bool _update_tx_resource(void)
{
    struct ssv6xxx_hci_txq_info *pInfo=NULL;
    u32 regVal;

    regVal=ssv6xxx_drv_read_reg(ADR_TX_ID_ALL_INFO);
    pInfo=(struct ssv6xxx_hci_txq_info *)&regVal;
    free_tx_page=SSV6200_PAGE_TX_THRESHOLD-pInfo->tx_use_page;
    free_tx_id=SSV6200_ID_TX_THRESHOLD-pInfo->tx_use_id;
    //LOG_PRINTF("%s():FREE_TX_PAGE=%d, FREE_TX_ID=%d\r\n",__FUNCTION__,free_tx_page,free_tx_id);
    return TRUE;
}

static bool _is_tx_resource_enough(u32 frame_len)
{
    u32 page_count=0;
    u32 rty_cnt=3;
    page_count= (frame_len + SSV6xxx_ALLOC_RSVD);

    if (page_count & HW_MMU_PAGE_MASK)
        page_count = (page_count >> HW_MMU_PAGE_SHIFT) + 1;
    else
        page_count = page_count >> HW_MMU_PAGE_SHIFT;

    if (free_tx_id <= 0) goto ERR;
    if (free_tx_page <= 0) goto ERR;

ReTRY:
    if (free_tx_page < page_count) goto ERR;


    free_tx_page -= page_count;
    free_tx_id --;
    //LOG_PRINTF("%s():FREE_TX_PAGE=%d, FREE_TX_ID=%d\r\n",__FUNCTION__,free_tx_page,free_tx_id);
    return TRUE;
ERR:
    //LOG_PRINTF("%s():tx resources is not enough\r\n",__FUNCTION__);
    if(rty_cnt){
        rty_cnt--;
        _update_tx_resource();
        goto ReTRY;
    }
    return FALSE;

}


bool ssv6xxx_drv_wait_tx_resource(u32 frame_len)
{
    u8 times=0;
    while(FALSE==_is_tx_resource_enough(frame_len))
    {
//        PRINTF("wait tx resource\n");
        if(times>=1){
            OS_MsDelay(10);
        }

        _update_tx_resource();
        times++;
    }
    return TRUE;
}
#endif//#ifndef __SSV_UNIX_SIM__

#ifdef THROUGHPUT_TEST
u32	sdio_rx_count=0;
u8	sdio_rx_verify_mode=0;
#ifdef WIN32
    clock_t sdio_rx_verify_start;
#else
    struct timeval sdio_rx_verify_start;
    double t1,t2;
#endif
#endif
#if(CONFIG_CMD_BUS_TEST==1)
extern OsMsgQ spi_qevt;
#endif
void  CmdEng_RxHdlData(void *frame);
u32 ssv6xxx_drv_get_handle()
{
    u32 retVal=0;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(-1, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->handle == NULL)
    {
        SDRV_WARN("%s() : NO handle() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, s_drv_cur, s_drv_cur->name);
        return -1;
    }
    OS_MUTEX_LOCK(drvMutex);
    retVal=s_drv_cur->handle();
    OS_MUTEX_UNLOCK(drvMutex);
    return retVal;
}

bool ssv6xxx_drv_ack_int()
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->ack_int == NULL)
    {
        SDRV_WARN("%s() : NO ack_int() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->ack_int();
    OS_MUTEX_UNLOCK(drvMutex);
    return ret;
}

bool ssv6xxx_drv_register(struct ssv6xxx_drv_ops *ssv_drv)
{
    u16 i;

    if (s_drv_cnt == MAX_SSV6XXX_DRV)
    {
        SDRV_ERROR("%s s_drv_cnt = MAX_SSV6XXX_DRV!\r\n",__FUNCTION__);
        return false;
    }
    SDRV_TRACE("%s() <= : 0x%08x, %s\r\n", __FUNCTION__, ssv_drv, ssv_drv->name);

    // find empty slot in array
    for (i = 0; i < MAX_SSV6XXX_DRV; i++)
    {
        if (s_drv_array[i] == NULL)
        {
            s_drv_array[i] = ssv_drv;
            s_drv_cnt++;
            SDRV_TRACE("%s() => : ok! s_drv_cnt = %d, i = %d, ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, s_drv_cnt, i, ssv_drv, ssv_drv->name);
            return TRUE;
        }
    }

    /* never reach here! */
    SDRV_FATAL("%s should never reach here!\r\n",__FUNCTION__);
    return FALSE;
}

bool ssv6xxx_drv_unregister(u16 i)
{
    if (s_drv_cnt == 0)
    {
        SDRV_WARN("%s() : s_drv_cnt = 0, return true!\r\n", __FUNCTION__);
        return TRUE;
    }
    SDRV_TRACE("%s() <= : i = %d, 0x%08x, %s\r\n", __FUNCTION__, i, s_drv_array[i], s_drv_array[i]->name);

    // find matching slot in array
    s_drv_array[i] = 0;
    s_drv_cnt--;
    SDRV_TRACE("%s() => : s_drv_cnt = %d\r\n", __FUNCTION__, s_drv_cnt);
    return TRUE;
}

void ssv6xxx_drv_list(void)
{
    u16 i;

    LOG_PRINTF("available in ssv driver module :\r\n",__LINE__);
    for (i = 0; i < s_drv_cnt; i++)
    {
        if (s_drv_cur == s_drv_array[i])
        {
            LOG_PRINTF("%-10s : 0x%08x (selected)\r\n", s_drv_array[i]->name, s_drv_array[i]);
        }
        else
        {
            LOG_PRINTF("%-10s : 0x%08x\r\n", s_drv_array[i]->name, s_drv_array[i]);
        }
    }
    LOG_PRINTF("%-10s = %d\r\n", "TOTAL", s_drv_cnt);
}

bool ssv6xxx_drv_module_init(void)
{
    SDRV_TRACE("%s() <=\r\n", __FUNCTION__);

    s_drv_cnt = 0;
    MEMSET(s_drv_array, 0x00, MAX_SSV6XXX_DRV * sizeof(struct ssv6xxx_drv_ops *));
    s_drv_cur = 0;

    // register each driver
//#if (SDRV_INCLUDE_SIM)
//	ssv6xxx_drv_register(&g_drv_sim);
//#endif


#if (SDRV_INCLUDE_SDIO)
    ssv6xxx_drv_register(&g_drv_sdio);
#endif

#if (defined _WIN32)
#if (SDRV_INCLUDE_UART)
    ssv6xxx_drv_register(&g_drv_uart);
#endif

#if (SDRV_INCLUDE_USB)
    ssv6xxx_drv_register(&g_drv_usb);
#endif
#endif /* _WIN32 */

#if (SDRV_INCLUDE_SPI)
    ssv6xxx_drv_register(&g_drv_spi);
#endif

    OS_SemInit(&ssvdrv_rx_sphr, 256, 0);
    if(OS_SUCCESS == OS_MutexInit(&drvMutex))
        return TRUE;
    else
        return FALSE;
}

// ssv_drv module release
void ssv6xxx_drv_module_release(void)
{
    s16 i, tmp;

    SDRV_TRACE("%s() <= : s_drv_cnt = %d\r\n", __FUNCTION__, s_drv_cnt);
    // close each driver & unregister
    tmp = s_drv_cnt;
    for (i = 0; i < tmp; i++)
    {
        SDRV_TRACE("s_drv_array[%d] = 0x%08x\r\n", i, s_drv_array[i]);
        if (s_drv_array[i] != NULL)
        {
            if (s_drv_array[i]->close != NULL)
                s_drv_array[i]->close();

            if (!ssv6xxx_drv_unregister((u16)i))
                SDRV_WARN("ssv6xxx_drv_unregister(%d) fail!\r\n", i);
        }
    }

    s_drv_cur = 0;
    OS_MutexDelete(drvMutex);
    SDRV_TRACE("%s() =>\r\n", __FUNCTION__);
    return;
}

bool ssv6xxx_drv_select(char name[32])
{
    u16 i;
    bool bRet;
    struct ssv6xxx_drv_ops *drv_target;

    SDRV_TRACE("%s() <= : name = %s, s_drv_cnt = %d, s_drv_cur = (0x%08x)\r\n", __FUNCTION__, name, s_drv_cnt, s_drv_cur);

    if (s_drv_cnt == 0)
		SDRV_FAIL("%s s_drv_cnt = 0\r\n",__FUNCTION__);

    // find the matching ssv_drv
    drv_target = 0;
    for (i = 0; i < s_drv_cnt; i++)
    {
        if (strcmp(name, s_drv_array[i]->name) == 0)
        {
            drv_target = s_drv_array[i];
            break;
        }
    }

    if (drv_target == 0)
    {
	LOG_PRINTF("ssv driver '%s' is NOT available now!\r\n", name);
        // ssv6xxx_drv_list();
        return FALSE;
    }
    // if the target drv = current drv, just return
    if (drv_target == s_drv_cur)
    {
	LOG_PRINTF("ssv drv '%s' is already in selection.\r\n", drv_target->name);
        // ssv6xxx_drv_list();
        return TRUE;
    }

    // try to open the target ssv_drv
    bRet = FALSE;
    if (drv_target->open != NULL)
    {
        if ((bRet = drv_target->open()) == false)
		SDRV_FAIL("open() fail! in s_drv_cur (0x%08x, %s)\r\n", drv_target, drv_target->name);
    }
    else
    {
        bRet = true; // regard it as success
	SDRV_WARN("open() = NULL in s_drv_cur (0x%08x, %s)\r\n", drv_target, drv_target->name);
    }
    // if target drv open() fail, return
    if (bRet == FALSE)
    {
        // ssv6xxx_drv_list();
        return FALSE;
    }
    // init the target drv
    bRet = FALSE;
    if (drv_target->init != NULL)
    {
        if ((bRet = drv_target->init()) == FALSE)
		SDRV_FAIL("init() fail! in drv (0x%08x, %s)\r\n", drv_target, drv_target->name);
    }
    else
    {
        bRet = TRUE; // regard it as success
		SDRV_WARN("init() = NULL in drv (0x%08x, %s)\r\n", drv_target, drv_target->name);
    }
    // if target drv init() fail, return
    if (bRet == FALSE)
    {
        // ssv6xxx_drv_list();
        return FALSE;
    }
    // close the current drv
    if (s_drv_cur != 0)
    {
        if (s_drv_cur->close != NULL)
        {
            if (!s_drv_cur->close())
		SDRV_WARN("%s() : close() fail! in s_drv_cur (0x%08x, %s)\r\n", __FUNCTION__, s_drv_cur, s_drv_cur->name);
        }
        else
		SDRV_WARN("close() = NULL in s_drv_cur (0x%08x, %s)\r\n", s_drv_cur, s_drv_cur->name);
    }
    s_drv_cur = drv_target;
    LOG_PRINTF("select drv -> %-10s : 0x%08x\r\n", s_drv_cur->name, s_drv_cur);
    // ssv6xxx_drv_list();
    return TRUE;
}

bool ssv6xxx_drv_open(void)
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);

    assert(s_drv_cur->open != NULL);

    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->open();
    OS_MUTEX_UNLOCK(drvMutex);

    return ret;
}

bool ssv6xxx_drv_close(void)
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);

    assert(s_drv_cur->close != NULL);

    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->close();
    OS_MUTEX_UNLOCK(drvMutex);
    return ret;
}

bool ssv6xxx_drv_init(void)
{
    if (s_drv_cur == 0)
        SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);

    assert(s_drv_cur->init != NULL);

    return (s_drv_cur->init());
}

s32 ssv6xxx_drv_recv(u8 *dat, size_t len)
{
    s32 retVal=0;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(-1, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->recv == NULL)
    {
        SDRV_WARN("%s() : NO recv() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, s_drv_cur, s_drv_cur->name);
        return -1;
    }
    OS_MUTEX_LOCK(drvMutex);
    retVal=s_drv_cur->recv(dat, len);
    OS_MUTEX_UNLOCK(drvMutex);
    return retVal;
}

bool ssv6xxx_drv_get_name(char name[32])
{
    if (s_drv_cur == 0)
        SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);

    strcpy(name, s_drv_cur->name);
    return TRUE;
}

bool ssv6xxx_drv_ioctl(u32 ctl_code,
                            void *in_buf, size_t in_size,
                            void *out_buf, size_t out_size,
                            size_t *bytes_ret)
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL("%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->ioctl == NULL)
    {
        SDRV_WARN("%s() : NO ioctl() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->ioctl(ctl_code, in_buf, in_size, out_buf, out_size, bytes_ret);
    OS_MUTEX_UNLOCK(drvMutex);
    return ret;
}

#if CONFIG_STATUS_CHECK
extern u32 g_dump_tx;
extern void _packetdump(const char *title, const u8 *buf,
                             size_t len);

#endif

s32 ssv6xxx_drv_send(void *dat, size_t len)
{
    s32 retVal=0;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(-1, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->send == NULL)
    {
        SDRV_WARN("%s() : NO send() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, s_drv_cur, s_drv_cur->name);
        return -1;
    }


#if CONFIG_STATUS_CHECK
    if(g_dump_tx)
        _packetdump("ssv6xxx_drv_send", dat, len);
#endif

    OS_MUTEX_LOCK(drvMutex);
    retVal=s_drv_cur->send(dat, len);
    OS_MUTEX_UNLOCK(drvMutex);

    return retVal;
}

u32 ssv6xxx_drv_read_reg(u32 addr)
{
    u32 retVal=0;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(-1, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->read_reg == NULL)
    {
        SDRV_WARN("%s() : NO read_reg() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, s_drv_cur, s_drv_cur->name);
        return -1;
    }
    OS_MUTEX_LOCK(drvMutex);
    retVal=s_drv_cur->read_reg(addr);
    OS_MUTEX_UNLOCK(drvMutex);
    return retVal;
}


bool ssv6xxx_drv_write_reg(u32 addr, u32 data)
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->write_reg == NULL)
    {
        SDRV_WARN("%s() : NO write_reg() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->write_reg(addr, data);
    OS_MUTEX_UNLOCK(drvMutex);
    return ret;
}


bool ssv6xxx_drv_write_byte(u32 addr, u32 data)
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->write_byte == NULL)
    {
        SDRV_WARN("%s() : NO write_reg() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->write_byte(1,addr, data);
    OS_MUTEX_UNLOCK(drvMutex);
    return ret;
}

bool ssv6xxx_drv_read_byte(u32 addr)
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->read_byte == NULL)
    {
        SDRV_WARN("%s() : NO read_byte() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->read_byte(1,addr);
    OS_MUTEX_UNLOCK(drvMutex);
    return ret;
}


bool ssv6xxx_drv_write_sram (u32 addr, u8 *data, u32 size)
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->write_sram == NULL)
    {
        SDRV_WARN("%s() : NO write_sram() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->write_sram(addr, data, size);
    OS_MUTEX_UNLOCK(drvMutex);
    return ret;
}


bool ssv6xxx_drv_read_sram (u32 addr, u8 *data, u32 size)
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->read_sram == NULL)
    {
        SDRV_WARN("%s() : NO read_sram() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->read_sram(addr, data, size);
    OS_MUTEX_UNLOCK(drvMutex);
    return ret;
}

bool ssv6xxx_drv_load_fw(u8 *bin, u32 len)
{
    bool ret=TRUE;
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->load_fw== NULL)
    {
        SDRV_WARN("%s() : NO read_sram() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    OS_MUTEX_LOCK(drvMutex);
    ret=s_drv_cur->load_fw(bin, len);
    OS_MUTEX_UNLOCK(drvMutex);
    return ret;
}

bool ssv6xxx_drv_start (void)
{
#ifdef __linux__
    if ((_ssv6xxx_drv_started != TRUE) && (s_drv_cur->start != NULL))
    {
        // if sdio, use ioctl to start the HCI
        s_drv_cur->start();
    }
#endif
    _ssv6xxx_drv_started = TRUE;
    return TRUE;
}


bool ssv6xxx_drv_stop (void)
{
#ifdef __linux__
    if ((_ssv6xxx_drv_started != FALSE) && (s_drv_cur->stop != NULL))
    {
        s_drv_cur->stop();
    }
#endif
    _ssv6xxx_drv_started = FALSE;
    return TRUE;
}

bool ssv6xxx_drv_irq_enable(void)
{
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->irq_enable== NULL)
    {
        SDRV_WARN("%s() : NO read_sram() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    s_drv_cur->irq_enable();
    return TRUE;

}

bool ssv6xxx_drv_irq_disable(void)
{
    if (s_drv_cur == 0)
        SDRV_FAIL_RET(FALSE, "%s s_drv_cur = 0\r\n",__FUNCTION__);

    if (s_drv_cur->irq_disable== NULL)
    {
        SDRV_WARN("%s() : NO read_sram() in ssv_drv = (0x%08x, %s)\r\n", __FUNCTION__, s_drv_cur, s_drv_cur->name);
        return FALSE;
    }
    s_drv_cur->irq_disable();
    return TRUE;

}

