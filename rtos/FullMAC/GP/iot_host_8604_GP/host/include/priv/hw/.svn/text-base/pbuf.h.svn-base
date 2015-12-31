#ifndef _PBUF_H_
#define _PBUF_H_

#include <ssv_pktdef.h>
#include <config.h>
#include <host_config.h>

#if (CONFIG_USE_LWIP_PBUF==0)
void        *PBUF_MAlloc_Raw(u32 size, u32 need_header, PBuf_Type_E buf_type);
void		_PBUF_MFree(void *PKTMSG);

#define PBUF_MAlloc(size, type)  PBUF_MAlloc_Raw(size, 1, type);
#define PBUF_MFree(PKTMSG) _PBUF_MFree(PKTMSG)
#define PBUF_isPkt(addr)    (1)

s32		PBUF_Init(void);

#endif//#if CONFIG_USE_LWIP_PBUF 

#endif /* _PBUF_H_ */





