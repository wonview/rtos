#include <config.h>
#include <hdr80211.h>
#include <ssv_pktdef.h>
#include <log.h>
//#include "lib-impl.h"
#include "ssv_lib.h"
#include <ssv_frame.h>

//#define SSV_FRAME_HEADER_RESV 100

#if CONFIG_USE_LWIP_PBUF
#include "lwip/lwip_pbuf.h"
#include "lwip/mem.h"
#else
#include <pbuf.h>
#endif

void os_frame_free(void *frame)
{
#if CONFIG_USE_LWIP_PBUF

#if CONFIG_MEMP_DEBUG    
    PBUF_DBG_FLAGS(((struct pbuf *)frame), PBUF_DBG_FLAG_RESET);
#endif

	pbuf_free((struct pbuf *)frame);	

#else//#if USE_LWIP_PBUF 
	if (frame)
		PBUF_MFree(frame);
#endif//#if USE_LWIP_PBUF 
}


#if CONFIG_MEMP_DEBUG
void* os_frame_alloc_fn(u32 size, const char* file, const int line)
#else
void* os_frame_alloc(u32 size)
#endif
{

#if CONFIG_USE_LWIP_PBUF

#if CONFIG_MEMP_DEBUG
{
    struct pbuf * p;
    p = pbuf_alloc_fn(PBUF_RAW, size, PBUF_POOL, file, line);    
    
    if (p != NULL)
        PBUF_DBG_FLAGS(p, PBUF_DBG_FLAG_L2);
    
    return (void*)p;
}

#else
    return (void*)pbuf_alloc(PBUF_RAW, size, PBUF_POOL);
#endif

   
#else//CONFIG_USE_LWIP_PBUF
	PKT_Info * pPKtInfo = (PKT_Info *)PBUF_MAlloc(size ,NOTYPE_BUF);

	if(!pPKtInfo)
		return pPKtInfo;
		
	pPKtInfo->len = size;
	pPKtInfo->hdr_offset = 80;
	
	return pPKtInfo;
#endif//#if USE_LWIP_PBUF 
}


void* os_frame_dup(void *frame)
{
#if CONFIG_USE_LWIP_PBUF     
    struct pbuf *new_p, *old_p;    
	
	//u32 *p_end;
	u8 *p_end;
    u32 size;
    u32 shift_size;

    old_p = (struct pbuf *)frame;
    
	//p_end = (u32*)((u8*)old_p->payload + old_p->tot_len);
	p_end = (u8*)old_p->payload + old_p->tot_len;

    
    //original raw size
    size = (u32)p_end - (u32)((u8_t *)old_p+LWIP_MEM_ALIGN_SIZE(SIZEOF_STRUCT_PBUF+DRV_TRX_HDR_LEN));
	new_p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);
    
    if(new_p == NULL)
        return (void*)new_p;
    
    shift_size = old_p->tot_len - size;

    do{
        //+,-->increse header
        //-,-->decrease header
        if(pbuf_header(new_p, shift_size) != ERR_OK){
            pbuf_free(new_p);
            new_p = NULL;
            break;
        }
                   
        if (pbuf_copy(new_p, old_p) != ERR_OK) {
              pbuf_free(new_p);
              new_p = NULL;
              break;
        }
    }while(0);


    return (void*)new_p;
    
#else//#if USE_LWIP_PBUF 
	void * dframe = os_frame_alloc(OS_FRAME_GET_DATA_LEN(frame));
	
	if(dframe)	
		MEMCPY(OS_FRAME_GET_DATA(dframe), OS_FRAME_GET_DATA(frame), OS_FRAME_GET_DATA_LEN(frame));
	
	return (void*)dframe;


#endif//#if USE_LWIP_PBUF 
}

//increase space to set header
void* os_frame_push(void *frame, u32 len)
{

#if CONFIG_USE_LWIP_PBUF 
    struct pbuf *p = (struct pbuf *)frame;
    if(pbuf_header(p, len)!= ERR_OK){
       return NULL;        
    }

    return (void*)p->payload;


#else//#if USE_LWIP_PBUF 
	PKT_Info * pPKtInfo = (PKT_Info * )frame;

	pPKtInfo->hdr_offset -= len;
	pPKtInfo->len  += len;

	return (u8*)pPKtInfo+pPKtInfo->hdr_offset;
#endif//#if USE_LWIP_PBUF 
}


//decrease data space.
void* os_frame_pull(void *frame, u32 len)
{
#if CONFIG_USE_LWIP_PBUF 

    struct pbuf *p = (struct pbuf *)frame;
    if(pbuf_header(p, -len)!= ERR_OK){
       return NULL;        
    }

    return (void*)p->payload;


#else//#if USE_LWIP_PBUF 
	PKT_Info * pPKtInfo = (PKT_Info * )frame;

	pPKtInfo->hdr_offset += len;
	pPKtInfo->len  -= len;

	return (u8*)pPKtInfo+pPKtInfo->hdr_offset;
#endif//#if USE_LWIP_PBUF 
}


//----------------------------------------------





