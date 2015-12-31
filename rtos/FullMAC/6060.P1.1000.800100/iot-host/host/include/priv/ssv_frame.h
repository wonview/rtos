#ifndef _SSV_FRAME_H_
#define _SSV_FRAME_H_

//-----------------------------------------------------------------------------------------------------------------------------------------------
//																frame buffer related
//-----------------------------------------------------------------------------------------------------------------------------------------------






#if CONFIG_MEMP_DEBUG
void* os_frame_alloc_fn(u32 size, const char* file, const int line);
#define os_frame_alloc(_size) os_frame_alloc_fn(_size, __FILE__, __LINE__)
#else
void* os_frame_alloc(u32 size);
#endif

void os_frame_free(void *frame);
void* os_frame_dup(void *frame);


//increase space to set header
void* os_frame_push(void *frame, u32 len);

//decrease data space.
void* os_frame_pull(void *frame, u32 size);

#if CONFIG_USE_LWIP_PBUF 
#include "lwip/lwip_pbuf.h"
#include "lwip/mem.h"
#define OS_FRAME_GET_DATA(_frame)               ((u8*)(((struct pbuf *)_frame)->payload))
#define OS_FRAME_GET_DATA_LEN(_frame)           (((struct pbuf *)_frame)->tot_len)
#define OS_FRAME_SET_DATA_LEN(_frame, _len)     do{                                             \
                                                    if(_len == 0) {abort();}                    \
                                                    ((struct pbuf *)_frame)->tot_len = _len;    \
                                                    ((struct pbuf *)_frame)->len = _len;        \
                                                }while(0)


#else
#define OS_FRAME_GET_DATA(_frame)               ((u8*)(_frame)+(((PKT_Info *)_frame)->hdr_offset))
#define OS_FRAME_GET_DATA_LEN(_frame)           (((PKT_Info *)_frame)->len)
#define OS_FRAME_SET_DATA_LEN(_frame, _len)     (((PKT_Info *)_frame)->len = _len)
#endif


#endif /* _SSV_FRAME_H_ */

