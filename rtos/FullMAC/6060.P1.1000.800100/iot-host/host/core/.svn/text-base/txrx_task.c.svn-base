#include <log.h>
#include <cmd_def.h>
#include <hctrl.h>
#include "host_cmd_engine_priv.h"
#include "txrx_task.h"
#include <ssv_drv.h>
#include <txrx_hdl.h>

#define PRI_Q_NUM 5


struct rcs_info
{
    u16 free_page;
    u16 free_id;
    u16 max_frame[PRI_Q_NUM];
};
//extern OsMutex  g_dev_info_mutex;
extern s32 ap_handle_ps(void *pPktInfo);

static OsMutex task_mtx;
static OsMutex tx_mtx;
static ModeType  curr_mode;
static u32 tx_data_frm_num;
static u32 tx_flush_data_frm_num;
static struct list_q tx_hwq[PRI_Q_NUM];
static struct list_q free_frmQ;
struct rcs_info tx_rcs;

static OsSemaphore tx_frm_sphr;
static OsSemaphore rx_frm_sphr;
#define TXFRM_MAX_NUM 26 //(total of 5q maximun) +1
#define RXFRM_MAX_NUM 26 //pbuf maximun

#if ONE_TR_THREAD
void TXRXTask_Task(void *args);
static bool Task_Done = false;
#else
void TXRXTask_TxTask(void *args);
void TXRXTask_RxTask(void *args);
static bool TxTask_Done = false;
static bool RxTask_Done = false;
#endif

struct task_info_st g_txrx_task_info[] =
{
#if ONE_TR_THREAD
	{ "txrxtask_task",   (OsMsgQ)0, 0, 	OS_TX_TASK_PRIO, WIFI_TX_STACK_SIZE, NULL, TXRXTask_Task },
#else
    { "txrxtask_txtask",   (OsMsgQ)0, 0, 	OS_TX_TASK_PRIO, WIFI_TX_STACK_SIZE, NULL, TXRXTask_TxTask },
    { "txrxtask_rxtask",   (OsMsgQ)0, 0, 	OS_RX_TASK_PRIO, WIFI_RX_STACK_SIZE, NULL, TXRXTask_RxTask },
#endif
};

#ifndef __SSV_UNIX_SIM__
#include <regs/ssv6200_reg.h>
static bool _update_tx_resource(void)
{
    struct ssv6xxx_hci_txq_info *pInfo=NULL;
    u32 regVal;

    regVal=ssv6xxx_drv_read_reg(ADR_TX_ID_ALL_INFO);
    pInfo=(struct ssv6xxx_hci_txq_info *)&regVal;
    tx_rcs.free_page=SSV6200_PAGE_TX_THRESHOLD-pInfo->tx_use_page;
    tx_rcs.free_id=SSV6200_ID_TX_THRESHOLD-pInfo->tx_use_id;
    //LOG_PRINTF("%s():FREE_TX_PAGE=%d, FREE_TX_ID=%d\r\n",__FUNCTION__,tx_rcs.free_page,tx_rcs.free_id);
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

    if (tx_rcs.free_id <= 0) goto ERR;
    if (tx_rcs.free_page <= 0) goto ERR;

ReTRY:
    if (tx_rcs.free_page < page_count) goto ERR;

    tx_rcs.free_page -= page_count;
    tx_rcs.free_id --;
    //LOG_PRINTF("%s():FREE_TX_PAGE=%d, FREE_TX_ID=%d\r\n",__FUNCTION__,tx_rcs.free_page,tx_rcs.free_id);
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
/*
static bool _wait_tx_resource(u32 frame_len)
{
    u8 times=0;
    while(FALSE==_is_tx_resource_enough(frame_len))
    {
        if(times>=1){
            OS_MsDelay(10);
        }

        _update_tx_resource();
        times++;
    }
    return TRUE;
}*/
#endif //#ifdef RCS_CONTROL

//TXRX_RxFrameProc Process rx frame
s32 TXRXTask_RxFrameProc(void* frame)
{
    //send to CmdEngine if it is host_event
    //pass to RxHdl if it is data frame
    s32 ret = SSV6XXX_SUCCESS;
    struct cfg_host_rxpkt * rxpkt = (struct cfg_host_rxpkt *)OS_FRAME_GET_DATA(frame);
    if(rxpkt->c_type == M0_RXEVENT)
    {
        if(curr_mode == MT_RUNNING)
        {

            ret = RxHdl_FrameProc(frame);
            return ret;
        }
        else
        {
            os_frame_free(frame);
            LOG_DEBUG("[TXRXTask]: free rx pkt due to mode = stop\r\n");
            return SSV6XXX_FAILED;
        }
    }
    else if(rxpkt->c_type == HOST_EVENT)
    {
        struct cfg_host_event *pPktInfo = (struct cfg_host_event *)OS_FRAME_GET_DATA(frame);

    	if ((pPktInfo->h_event ==SOC_EVT_PS_POLL) || (pPktInfo->h_event == SOC_EVT_NULL_DATA))
        {
            ap_handle_ps(frame); //handle null data and ps_poll

        }
        else
        {
            void *msg = os_msg_alloc();
            if (msg)
                os_msg_send(msg, frame);
            else
                ret = SSV6XXX_NO_MEM;
        }
        return ret;
    }
    else
    {
        LOG_DEBUG("[TXRXTask]: unavailable type of rx pkt\r\n");
        return SSV6XXX_INVA_PARAM;
    }

    return ret;
}

//TXRX_Task TXRX main thread
#if ONE_TR_THREAD
void TXRXTask_Task(void *args)
{
    void *rFrame = NULL, *tFrame = NULL;
    FrmQ *outFrm = NULL;
    u8  *msg_data = NULL;
    s32 recv_len = 0;
    s8 i = 0;

    while(curr_mode != MT_EXIT)
    {
        if(rFrame == NULL)
        {
            if((rFrame = (u8 *)os_frame_alloc(MAX_RECV_BUF)) != NULL)
            {
                os_frame_push(rFrame, DRV_TRX_HDR_LEN);
                msg_data = OS_FRAME_GET_DATA(rFrame);
            }
            else
                LOG_DEBUG("[TxRxTask]: malloc = 0!\r\n");
        }

        if(rFrame != NULL)
        {
            OS_SemWait(rx_frm_sphr, 1);
            recv_len = ssv6xxx_drv_recv(msg_data, OS_FRAME_GET_DATA_LEN(rFrame));
            if(recv_len != -1)
            {
                OS_FRAME_SET_DATA_LEN(rFrame, recv_len);
                TXRXTask_RxFrameProc(rFrame);
                rFrame = NULL;
                msg_data = NULL;
            }
            else
            {
                LOG_DEBUG("[TxRxTask]: Rx semaphore comes in, but not frame receive\r\n");
            }

        }

        for (i = PRI_Q_NUM; i >= 0; i--)
        {
            if((outFrm = (FrmQ *)list_q_deq_safe(&tx_hwq[i], &tx_mtx)) != NULL)
            {
                tFrame = outFrm->frame;
#ifndef __SSV_UNIX_SIM__
                if(_is_tx_resource_enough(OS_FRAME_GET_DATA_LEN(tFrame)) == TRUE)
#endif
                {
                    if (ssv6xxx_drv_send(OS_FRAME_GET_DATA(tFrame), OS_FRAME_GET_DATA_LEN(tFrame)) <0)
                    {
                        LOG_PRINTF("%(): ssv6xxx_drv_send() data failed !!\r\n", __FUNCTION__);
                    }
                    os_frame_free(tFrame);
                    OS_MutexLock(tx_mtx);
                    if(list_q_len(&free_frmQ) > TXFRM_MAX_NUM)
                        FREE(outFrm);
                    else
                        list_q_qtail(&free_frmQ,(struct list_q *)outFrm);
                    OS_MutexUnLock(tx_mtx);
                    OS_SemSignal(tx_frm_sphr);
                    tFrame = NULL;
                    outFrm = NULL;
                }
#ifndef __SSV_UNIX_SIM__
                else
                {
                    list_q_insert_safe(&tx_hwq[i],&tx_hwq[i], (struct list_q *)outFrm, &tx_mtx);
                    outFrm = NULL;
                    break;
                }
#endif
            }
        }
    }
    Task_Done = true;
}
#else
void TXRXTask_TxTask(void *args)
{
    void *tFrame = NULL;
    FrmQ *outFrm = NULL;
    s8 i = 0, prc_count = 0;
    bool flush_frm = false;
    //u32 start_ticks = 0, account_ticks = 0, wakeup_ticks = 0, wakeup_total = 0, wait_ticks = 0, acc_wait = 0;
    //u32 periodic_pktnum = 0;

    while(curr_mode != MT_EXIT)
    {
        prc_count = 0;
        /*
        account_ticks += (OS_GetSysTick() - start_ticks);
        wakeup_total += (OS_GetSysTick() - wakeup_ticks);
        if(account_ticks > 1000)
        {
            LOG_PRINTF("%s: %d pkt is sent in %d ms, wakeup in %d ms, acc_wait %d ms !!\r\n",
                       __FUNCTION__, periodic_pktnum, account_ticks, wakeup_total, acc_wait);
            periodic_pktnum = 0;
            account_ticks = 0;
            wakeup_total = 0;
            acc_wait = 0;
        }
        start_ticks = OS_GetSysTick();*/
        OS_SemWait(tx_frm_sphr, 0);
        /*wakeup_ticks = OS_GetSysTick();
        wait_ticks = (wakeup_ticks-start_ticks);
        acc_wait += wait_ticks;*/
        for (i = 1; i <= PRI_Q_NUM; i++)
        {
            while((outFrm = (FrmQ *)list_q_deq_safe(&tx_hwq[(PRI_Q_NUM-i)], &tx_mtx)) != NULL)
            {
                tFrame = outFrm->frame;
                if(tFrame){
#ifndef __SSV_UNIX_SIM__
                    if(_is_tx_resource_enough(OS_FRAME_GET_DATA_LEN(tFrame)) == TRUE)
#endif
                    {
                        //LOG_PRINTF("%s:  Send tx frame %08x with FrmQ %08X!!\r\n", __FUNCTION__, tFrame, outFrm);
                        OS_MutexLock(tx_mtx);
                        flush_frm = (tx_flush_data_frm_num == 0)?false:true;
                        tx_data_frm_num --;
                        if(flush_frm == true)
                            tx_flush_data_frm_num--;
                        OS_MutexUnLock(tx_mtx);

                        if (flush_frm == false)
                        {
                            if (ssv6xxx_drv_send(OS_FRAME_GET_DATA(tFrame), OS_FRAME_GET_DATA_LEN(tFrame)) <0)
                            {
                                LOG_DEBUG("%s: ssv6xxx_drv_send() data failed !!\r\n", __FUNCTION__);
                            }
                        }

                        os_frame_free(tFrame);
                        outFrm->frame = NULL;
                        //periodic_pktnum++;
                        prc_count++;
                        OS_MutexLock(tx_mtx);
                        if(list_q_len(&free_frmQ) > TXFRM_MAX_NUM)
                        {
                            FREE(outFrm);
                        }
                        else
                        {
                            list_q_qtail(&free_frmQ,(struct list_q *)outFrm);
                        }

                        OS_MutexUnLock(tx_mtx);

                        tFrame = NULL;
                        outFrm = NULL;

                        if(prc_count > 1)
                        {
                            OS_SemWait(tx_frm_sphr, 0); //More frames are sent and resource should be pulled out.
                            //LOG_PRINTF("******************%s: push back due to multi-send:%d !!\r\n", __FUNCTION__, tx_count);
                        }
                    }
#ifndef __SSV_UNIX_SIM__
                    else
                    {
                        list_q_insert_safe(&tx_hwq[(PRI_Q_NUM-i)],&tx_hwq[(PRI_Q_NUM-i)], (struct list_q *)outFrm, &tx_mtx);
                        outFrm = NULL;
                        tFrame = NULL;

                        if(prc_count == 0)
                            OS_SemSignal(tx_frm_sphr); // Frame is not sent and resource should be added back.

                        break;
                    }
#endif
                }
                else
                {
                    LOG_ERROR("outFrm without pbuf?? \r\n");
                }
            }
        }
    }
    TxTask_Done = true;
}
//u32 RxTaskRetryCount[32]={0};
void TXRXTask_RxTask(void *args)
{
    void * rFrame = NULL;
    u8  *msg_data = NULL;
    s32 recv_len = 0;
    s32 retry = 0;
    s8 ma_retry = 0;

    while(curr_mode != MT_EXIT)
    {
        OS_SemWait(rx_frm_sphr, 0);
        ssv6xxx_drv_irq_disable();
        for(retry=0;retry<32;retry++)
        {
            if(rFrame == NULL)
            {
                while ((rFrame = (u8 *)os_frame_alloc(MAX_RECV_BUF)) == NULL)
                {
                    ma_retry++;
                    if (ma_retry == 3)
                    {
                    	ma_retry = 0;
                    	OS_MsDelay(10);
                    	LOG_DEBUG("[TxRxTask]: wakeup from sleep!\r\n");
                    }
                    //continue;
                }
                os_frame_push(rFrame, DRV_TRX_HDR_LEN);
                msg_data = OS_FRAME_GET_DATA(rFrame);
            }

            recv_len = ssv6xxx_drv_recv(msg_data, OS_FRAME_GET_DATA_LEN(rFrame));
            if(recv_len > 0)
            {
                OS_FRAME_SET_DATA_LEN(rFrame, recv_len);
                TXRXTask_RxFrameProc(rFrame);
                rFrame = NULL;
                msg_data = NULL;
            }
            else if(recv_len == 0)
            {
                LOG_DEBUG("[TxRxTask]: recv_len == 0 at frame 0x08%x, first 8 bytes content is dumpped as below:\r\n",rFrame);
                hex_dump(msg_data, 8);
            }
            else
            {
                //LOG_DEBUG("[TxRxTask]: Rx semaphore comes in, but not frame receive\r\n");
                break;
            }
        }
        //RxTaskRetryCount[retry]++;
        ssv6xxx_drv_irq_enable();
    }
    RxTask_Done = true;

    if(rFrame != NULL)
    {
        os_frame_free(rFrame);
        rFrame = NULL;
    }
}
#endif


//TXRX_Init Initialize
s32 TXRXTask_Init()
{
    u32 i, size, res=OS_SUCCESS;

	curr_mode = MT_STOP;
    tx_flush_data_frm_num = 0;
    tx_data_frm_num = 0;

    MEMSET(&tx_rcs, 0, sizeof(tx_rcs));
    OS_MutexInit(&task_mtx);
    OS_MutexInit(&tx_mtx);

    OS_SemInit(&tx_frm_sphr, TXFRM_MAX_NUM, 0);
    OS_SemInit(&rx_frm_sphr, RXFRM_MAX_NUM, 0);

    for(i = 0; i < PRI_Q_NUM; i++)
        list_q_init(&tx_hwq[i]);

    list_q_init(&free_frmQ);

	size = sizeof(g_txrx_task_info)/sizeof(struct task_info_st);
	for (i = 0; i < size; i++)
    {
		/* Create Registered Task: */
		OS_TaskCreate(g_txrx_task_info[i].task_func,
			g_txrx_task_info[i].task_name,
			g_txrx_task_info[i].stack_size<<4,
			g_txrx_task_info[i].args,
			g_txrx_task_info[i].prio,
			NULL);
	}

    LOG_PRINTF("%s(): end !!\r\n", __FUNCTION__);
    return res;
}

s32 TXRXTask_DeInit()
{
    FrmQ *freeFrm = NULL;

    while((freeFrm = (FrmQ *)list_q_deq(&free_frmQ)) != NULL)
    {
        FREE(freeFrm);
        freeFrm = NULL;
    }

    OS_MutexDelete(task_mtx);
    OS_MutexDelete(tx_mtx);

    OS_SemDelete(tx_frm_sphr);
    OS_SemDelete(rx_frm_sphr);
}
//TXRX_FrameEnqueue Enqueue tx frames
s32 TXRXTask_FrameEnqueue(void* frame, u32 priority)
{
    FrmQ *dataFrm = NULL;
    struct cfg_host_txreq0 *req = (struct cfg_host_txreq0 *)OS_FRAME_GET_DATA(frame);;

    if ((curr_mode == MT_STOP) && (req->c_type == M0_TXREQ))
    {
        LOG_DEBUG("[TxRx_task]: No enqueue frame due to mode = stop and frame type = data\r\n");
        return false;
    }

    OS_MutexLock(tx_mtx);
    if(list_q_len(&free_frmQ) > 0)
    {
        dataFrm = (FrmQ *)list_q_deq(&free_frmQ);
        //LOG_DEBUG("[TxRx_task]: tx frame fill w/free_frmQ, dataFrm = %08x\r\n", dataFrm);
    }
    else
    {
        dataFrm = (FrmQ *)MALLOC(sizeof(FrmQ));
        //LOG_DEBUG("[TxRx_task]: tx frame fill w/newly frmQ, dataFrm = %08x\r\n", dataFrm);
        if(dataFrm == NULL)
        {
            LOG_DEBUG("[TxRx_task]: No memory for incoming frame\r\n");
            OS_MutexUnLock(tx_mtx);
            return false;
        }
    }

#if CONFIG_USE_LWIP_PBUF
#if CONFIG_MEMP_DEBUG
    PBUF_DBG_FLAGS(((struct pbuf *)frame), PBUF_DBG_FLAG_L2_TX_DRIVER);
#endif
#endif

    dataFrm->frame = frame;
    //LOG_DEBUG("[TxRx_task]: Enqueue %d for incoming frame %08x with FrmQ %08x\r\n", priority, frame, dataFrm);

    tx_data_frm_num++;

    list_q_qtail(&tx_hwq[priority], &dataFrm->_list);
    //LOG_DEBUG("****************[TxRx_task]: size of tx_hwq[priority] = %d\r\n", tx_hwq[priority].qlen);
    OS_MutexUnLock(tx_mtx);

    OS_SemSignal(tx_frm_sphr);

    return true;
}

//TXRX_SetOpMode Set the operation mode
ssv6xxx_result TXRXTask_SetOpMode(ModeType mode)
{
    ssv6xxx_result ret = SSV6XXX_SUCCESS;

    if(mode > MT_EXIT)
        return SSV6XXX_INVA_PARAM;

    OS_MutexLock(tx_mtx);

    switch (curr_mode)
    {
        case MT_STOP:
        {
            if(mode == MT_EXIT)
            {
                tx_flush_data_frm_num = tx_data_frm_num;
            }
        }
            break;
        case MT_RUNNING:
            if(mode == MT_STOP)
            {
                //To stop
                tx_flush_data_frm_num = tx_data_frm_num;
            }
            else
            {
                //error handling
                ret = SSV6XXX_INVA_PARAM;
            }
            break;
        case MT_EXIT:
        default:
            //error handling
            ret = SSV6XXX_FAILED;
            break;
    }

    if(ret == SSV6XXX_SUCCESS)
        curr_mode = mode;

    OS_MutexUnLock(tx_mtx);
    LOG_DEBUG("[TxRx_task]: curr_mode = %d\r\n", curr_mode);

    if(curr_mode == MT_EXIT)
    {
#if ONE_TR_THREAD
        while(TxTask_Done != true)
#else
        while ((TxTask_Done != true) || (RxTask_Done != true))
#endif
        {
            OS_MsDelay(10);
        }

    }
    return SSV6XXX_SUCCESS;
}

void TXRXTask_Isr(u32 signo)
{
    if(signo &INT_RX)
    {
        OS_SemSignal(rx_frm_sphr);
    }
}

void TXRXTask_ShowSt(void)
{
    LOG_DEBUG("[TxRx_task]: curr_mode = %d\r\n %d tx frames in q\r\n %d tx frames need to be flushed",
              curr_mode, tx_data_frm_num, tx_flush_data_frm_num);
}

