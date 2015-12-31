#define SSV_LIB_C

#include <config.h>
#include <stdarg.h>
#include <hdr80211.h>
#include <ssv_pktdef.h>
#include "ssv6200_uart_bin.h"
#include "regs/ssv6200_configuration.h"
#include "dev.h"

#include "regs/ssv6200_reg.h"
#include "regs/ssv6200_aux.h"
#include "hctrl.h"
#include "dev_tbl.h"
#include "ssv_dev.h"


#if (CONFIG_HOST_PLATFORM == 0)
#include <uart/drv_uart.h>
#endif

#include <log.h>
//#include "lib-impl.h"
#include "ssv_lib.h"


#define CONSOLE_UART   SSV6XXX_UART0

// return  -1 : fail
//        >=0 : ok

void list_q_init(struct list_q *qhd)
{
    qhd->prev = (struct list_q *)qhd;
    qhd->next = (struct list_q *)qhd;
    qhd->qlen = 0;
}

void list_q_qtail(struct list_q *qhd, struct list_q *newq)
{
    struct list_q *next = qhd;
    struct list_q* prev = qhd->prev;

    newq->next = next;
    newq->prev = prev;
    next->prev = newq;
    prev->next = newq;
    qhd->qlen++;
}

void list_q_insert(struct list_q *qhd, struct list_q *prev, struct list_q *newq)
{
    struct list_q *next = prev->next;

    newq->next = next;
    newq->prev = prev;
    next->prev = newq;
    prev->next = newq;
    qhd->qlen++;
}

void list_q_remove(struct list_q *qhd,struct list_q *curt)
{
    struct list_q *next = curt->next;
    struct list_q *prev = curt->prev;

    prev->next = next;
    next->prev = prev;
    qhd->qlen--;
}

struct list_q * list_q_deq(struct list_q *qhd)
{
    struct list_q *next, *prev;
    struct list_q *elm = qhd->next;

    if((qhd->qlen > 0) && (elm != NULL))
    {
        qhd->qlen--;
        next        = elm->next;
        prev        = elm->prev;
        elm->next   = NULL;
        elm->prev   = NULL;
        next->prev  = prev;
        prev->next  = next;

        return elm;
    }else{
        return NULL;
    }
}
unsigned int list_q_len(struct list_q *qhd)
{
    return qhd->qlen;
}

u32 list_q_len_safe(struct list_q *q, OsMutex *pmtx)
{
    u32 len = 0;
    OS_MutexLock(*pmtx);
    len = q->qlen;
    OS_MutexUnLock(*pmtx);
    return len;
}

void list_q_qtail_safe(struct list_q *qhd, struct list_q *newq, OsMutex *pmtx)
{
    OS_MutexLock(*pmtx);
    list_q_qtail(qhd, newq);
    OS_MutexUnLock(*pmtx);
}

struct list_q * list_q_deq_safe(struct list_q *qhd, OsMutex *pmtx)
{
    struct list_q *_list = NULL;
    OS_MutexLock(*pmtx);
    _list = list_q_deq(qhd);
    OS_MutexUnLock(*pmtx);
    return _list;
}

void list_q_insert_safe(struct list_q *qhd, struct list_q *prev, struct list_q *newq, OsMutex *pmtx)
{
    OS_MutexLock(*pmtx);
    list_q_insert(qhd, prev, newq);
    OS_MutexUnLock(*pmtx);
}

void list_q_remove_safe(struct list_q *qhd,struct list_q *curt, OsMutex *pmtx)
{
    OS_MutexLock(*pmtx);
    list_q_remove(qhd, curt);
    OS_MutexUnLock(*pmtx);
}

LIB_APIs s32 ssv6xxx_strrpos(const char *str, char delimiter)
{
	const char *p;

	for (p = (str + strlen(str)) - 1; (s32)p>=(s32)str; p--)
	{
		if (*p == delimiter)
			return ((s32)p - (s32)str);
	}

	return -1;	// find no matching delimiter

}

LIB_APIs s32	ssv6xxx_isalpha(s32 c)
{
	if (('A' <= c) && (c <= 'Z'))
		return 1;
	if (('a' <= c) && (c <= 'z'))
		return 1;
	return 0;
}

LIB_APIs s32 ssv6xxx_str_toupper(char *s)
{
	while (*s)
	{
		*s = ssv6xxx_toupper(*s);
		s++;
	}
	return 0;
}

LIB_APIs s32 ssv6xxx_str_tolower(char *s)
{
	while (*s)
	{
		*s = ssv6xxx_tolower(*s);
		s++;
	}
	return 0;
}

LIB_APIs u32 ssv6xxx_atoi_base( const char *s, u32 base )
{
    u32  idx, upbound=base-1;
    u32  value = 0, v;

    while( (v = (u8)*s) != 0 ) {
        idx = v - '0';
        if ( idx > 10 && base==16 ) {
            idx = (v >= 'a') ? (v - 'a') : (v - 'A');
            idx += 10;
        }
        if ( idx > upbound )
            break;
        value = value * base + idx;
        s++;
    }

    return value;
}

LIB_APIs s32 ssv6xxx_atoi( const char *s )
{
    u32 neg=0, value, base=10;

    if ( *s=='0' ) {
        switch (*++s) {
        case 'x':
        case 'X': base = 16; break;
        case 'b':
        case 'B': base = 2; break;
        default: return 0;
        }
        s++;
    }
    else if ( *s=='-' ) {
        neg = 1;
        s++;
    }

    value = ssv6xxx_atoi_base(s, base);

    if ( neg==1 )
        return -(s32)value;
    return (s32)value;

}


#if (CONFIG_HOST_PLATFORM == 1)
u64 ssv6xxx_64atoi( char *s )
{
    u8 bchar='A', idx, upbound=9;
    u32 neg=0, value=0, base=10;

    if ( *s=='0' ) {
        switch (*++s) {
                case 'x': bchar = 'a';
                case 'X': base = 16; upbound = 15; break;
                case 'b':
                case 'B': base = 2; upbound = 1; break;
                default: return 0;
        }
        s++;
    }
    else if ( *s=='-' ) {
        neg = 1;
        s++;
    }

    while( *s ) {
        idx = (u8)*s - '0';
        if ( base==16 && (*s>=bchar) && (*s<=(bchar+5)) )
        {
                idx = (u8)10 + (u8)*s - bchar;
        }
        if ( idx > upbound )
        {
                break;
        }
        value = value * base + idx;
        s++;
    }

    if ( neg==1 )
        return -(s32)value;
    return (u64)value;

}
#endif




LIB_APIs char ssv6xxx_toupper(char ch)
{
	if (('a' <= ch) && (ch <= 'z'))
		return ('A' + (ch - 'a'));

	// else, make the original ch unchanged
	return ch;
}

LIB_APIs char ssv6xxx_tolower(char ch)
{
	if (('A' <= ch) && (ch <= 'Z'))
		return ('a' + (ch - 'A'));

	// else, make the original ch unchanged
	return ch;
}

LIB_APIs s32 ssv6xxx_isupper(char ch)
{
    return (ch >= 'A' && ch <= 'Z');
}

LIB_APIs s32 ssv6xxx_strcmp( const char *s0, const char *s1 )
{
    s32 c1, c2;

    do {
        c1 = (u8) *s0++;
        c2 = (u8) *s1++;
        if (c1 == '\0')
            return c1 - c2;
    } while (c1 == c2);

    return c1 - c2;
}

LIB_APIs s32 ssv6xxx_strncmp ( const char * s1, const char * s2, size_t n)
{
  if ( !n )
      return(0);

  while (--n && *s1 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return( *s1 - *s2 );
}

LIB_APIs char *ssv6xxx_strcat(char *s, const char *append)
{
    char *save = s;

    while (*s) { s++; }
    while ((*s++ = *append++) != 0) { }
    return(save);
}

LIB_APIs char *ssv6xxx_strncat(char *s, const char *append, size_t n)
{
     char* save = s;
     while(*s){ s++; }
     while((n--)&&((*s++ = *append++) != 0)){}
     *s='\0';
     return(save);
}

/*Not considering the case of memory overlap*/
LIB_APIs char *ssv6xxx_strcpy(char *dst, const char *src)
{
    char *ret = dst;
    assert(dst != NULL);
    assert(src != NULL);


    while((* dst++ = * src++) != '\0')
        ;
    return ret;
}

LIB_APIs char *ssv6xxx_strncpy(char *dst, const char *src, size_t n)
{
    register char *d = dst;
    register const char *s = src;

    if (n != 0) {
        do {
            if ((*d++ = *s++) == 0) {
                /* NUL pad the remaining n-1 bytes */
                while (--n != 0)
                *d++ = 0;
                break;
            }
        } while (--n != 0);
    }
    return (dst);
}


LIB_APIs size_t ssv6xxx_strlen(const char *s)
{
    const char *ptr = s;
    while (*ptr) ptr++;
    return (size_t)ptr-(size_t)s;
}

LIB_APIs char * ssv6xxx_strchr(const char * s, char c)
{
    const char * p = s;
    while(*p != c && *p)
        p++;
    return (*p=='\0' ? NULL : (char *)p);
}

LIB_APIs void *ssv6xxx_memset(void *s, s32 c, size_t n)
{
    if ( NULL != s ) {
		u8 * ps= (u8 *)s;
		const u8 * pes= ps+n;
        while( ps != pes )
			*(ps++) = (u8) c;
    }
    return s;
}


LIB_APIs void *ssv6xxx_memcpy(void *dest, const void *src, size_t n)
{
    u8 *d = dest;
    const u8 *s = src;

    while (n-- > 0)
      *d++ = *s++;
    return dest;
}


LIB_APIs s32 ssv6xxx_memcmp(const void *s1, const void *s2, size_t n)
{
    const u8 *u1 = (const u8 *)s1, *u2 = (const u8 *)s2;

    while (n--) {
        s32 d = *u1++ - *u2++;
        if (d != 0)
            return d;
    }
    /*
    for ( ; n-- ; s1++, s2++) {
        u1 = *(u8 *)s1;
        u2 = *(u8 *)s2;
        if (u1 != u2)
            return (u1-u2);
    } */
    return 0;
}

#if 0


//extern s32 gOsInFatal;
LIB_APIs void fatal_printf(const char *format, ...)
{

#if 0
   va_list args;


//   gOsInFatal = 1;
   /*lint -save -e530 */
   va_start( args, format );
   /*lint -restore */
   ret = print( 0, 0, format, args );
   va_end(args);
#endif
//    printf(format, ...);


}
#endif

#if 0
LIB_APIs void ssv6xxx_printf(const char *format, ...)
{
   va_list args;

   /*lint -save -e530 */
   va_start( args, format );
   /*lint -restore */

    printf(format, args);
//   ret = print( 0, 0, format, args );
   va_end(args);
}


LIB_APIs void ssv6xxx_snprintf(char *out, size_t size, const char *format, ...)
{
#if 0
    va_list args;
    s32     ret;
    /*lint -save -e530 */
    va_start( args, format ); /*lint -restore */
    ret = print( out, (out + size - 1), format, args );
    va_end(args);
#endif
}

LIB_APIs void ssv6xxx_vsnprintf(char *out, size_t size, const char *format, va_list args)
{
#if 0
	return print( out, (out + size - 1), format, args );
#endif
}
#endif

//LIB_APIs s32 putstr (const char *s, size_t len)
//{
//    return  printstr(0, 0, s, len);
//}


//LIB_APIs s32 snputstr (char *out, size_t size, const char *s, size_t len)
//{
//    return  printstr( &out, (out + size - 1), s, len);
//}


//#endif


#if (CLI_ENABLE==1 && CONFIG_HOST_PLATFORM==0)
LIB_APIs s32 kbhit(void)
{
    return drv_uart_rx_ready(CONSOLE_UART);
}


LIB_APIs s32 getch(void)
{
    return drv_uart_rx(CONSOLE_UART);
}


LIB_APIs s32 putchar(s32 ch)
{
    return drv_uart_tx(CONSOLE_UART, ch);
}
#endif


#if 0
LIB_APIs void ssv6xxx_raw_dump(u8 *data, s32 len)
{
	ssv6xxx_raw_dump_ex(data, len, true, 10, 10, 16, LOG_LEVEL_ON, LOG_MODULE_EMPTY);
	return;
}


LIB_APIs bool ssv6xxx_raw_dump_ex(u8 *data, s32 len, bool with_addr, u8 addr_radix, s8 line_cols, u8 radix, u32 log_level, u32 log_module)
{
    s32 i;

	// check input parameters
	if ((addr_radix != 10) && (addr_radix != 16))
	{
		LOG_ERROR("%s(): invalid value 'addr_radix' = %d\n\r", __FUNCTION__, addr_radix);
		return false;
	}
	if ((line_cols != 8) && (line_cols != 10) && (line_cols != 16) && (line_cols != -1))
	{
		LOG_ERROR("%s(): invalid value 'line_cols' = %d\n\r", __FUNCTION__, line_cols);
		return false;
	}
	if ((radix != 10) && (radix != 16))
	{
		LOG_ERROR("%s(): invalid value 'radix' = %d\n\r", __FUNCTION__, radix);
		return false;
	}

	if (len == 0)	return true;

	// if ONLY have one line
	if (line_cols == -1)
	{
		LOG_TAG_SUPPRESS_ON();
		// only print addr heading at one time
		if ((with_addr == true))
		{
			if      (addr_radix == 10)	LOG_PRINTF_LM(log_level, log_module, "%08d: ", 0);
			else if (addr_radix == 16)	LOG_PRINTF_LM(log_level, log_module, "0x%08x: ", 0);
		}

		for (i=0; i<len; i++)
		{
			// print data
			if	    (radix == 10)	LOG_PRINTF_LM(log_level, log_module, "%4d ",  data[i]);
			else if (radix == 16)	LOG_PRINTF_LM(log_level, log_module, "%02x ", data[i]);
		}
		LOG_PRINTF_LM(log_level, log_module, "\n\r");
		LOG_TAG_SUPPRESS_OFF();
		return true;
	}

	// normal case
	LOG_TAG_SUPPRESS_ON();
    for (i=0; i<len; i++)
	{
		// print addr heading
		if ((with_addr == true) && (i % line_cols) == 0)
		{
			if      (addr_radix == 10)	LOG_PRINTF_LM(log_level, log_module, "%08d: ", i);
			else if (addr_radix == 16)	LOG_PRINTF_LM(log_level, log_module, "0x%08x: ", i);
		}
		// print data
		if	    (radix == 10)	LOG_PRINTF_LM(log_level, log_module, "%4d ",  data[i]);
		else if (radix == 16)	LOG_PRINTF_LM(log_level, log_module, "%02x ", data[i]);
		// print newline
        if (((i+1) % line_cols) == 0)
            LOG_PRINTF_LM(log_level, log_module, "\n\r");
    }
    LOG_PRINTF_LM(log_level, log_module, "\n\r");
	LOG_TAG_SUPPRESS_OFF();
	return true;
}
#endif

#define ONE_RAW 16
void _packetdump(const char *title, const u8 *buf,
                             size_t len)
{
    size_t i;
    LOG_PRINTF("%s - hexdump(len=%d):\r\n    ", title, len);
    if (buf == NULL) {
        LOG_PRINTF(" [NULL]");
    }else{


        for (i = 0; i < ONE_RAW; i++)
            LOG_PRINTF("%02X ", i);

        LOG_PRINTF("\r\n---\r\n00|");


        for (i = 0; i < len; i++){
            LOG_PRINTF(" %02x", buf[i]);
            if((i+1)%ONE_RAW ==0)
                LOG_PRINTF("\r\n%02x|", (i+1));
        }
    }
    LOG_PRINTF("\r\n-----------------------------\r\n");
}



void pkt_dump_txinfo(PKT_TxInfo *p)
{
	u32		payload_len;
	u8		*dat;
	u8		*a;

	LOG_PRINTF("========= TxInfo =========\n\r");
	LOG_PRINTF("%20s : %d\n\r", "len", 		p->len);
	LOG_PRINTF("%20s : %d\n\r", "c_type",		p->c_type);
	LOG_PRINTF("%20s : %d\n\r", "f80211",		p->f80211);
	LOG_PRINTF("%20s : %d\n\r", "qos",			p->qos);
	LOG_PRINTF("%20s : %d\n\r", "ht",			p->ht);
	LOG_PRINTF("%20s : %d\n\r", "use_4addr",	p->use_4addr);
	LOG_PRINTF("%20s : %d\n\r", "RSVD_0",		p->RSVD_0);
	LOG_PRINTF("%20s : %d\n\r", "bc_que",		p->bc_que);
	LOG_PRINTF("%20s : %d\n\r", "security",	p->security);
	LOG_PRINTF("%20s : %d\n\r", "more_data",	p->more_data);
	LOG_PRINTF("%20s : %d\n\r", "stype_b5b4",	p->stype_b5b4);
	LOG_PRINTF("%20s : %d\n\r", "extra_info",	p->extra_info);
	LOG_PRINTF("%20s : 0x%08x\n\r", "fCmd",	p->fCmd);
	LOG_PRINTF("%20s : %d\n\r", "hdr_offset",	p->hdr_offset);
	LOG_PRINTF("%20s : %d\n\r", "frag",		p->frag);
	LOG_PRINTF("%20s : %d\n\r", "unicast",		p->unicast);
	LOG_PRINTF("%20s : %d\n\r", "hdr_len",		p->hdr_len);
	LOG_PRINTF("%20s : %d\n\r", "tx_report",	p->tx_report);
	LOG_PRINTF("%20s : %d\n\r", "tx_burst",	p->tx_burst);
	LOG_PRINTF("%20s : %d\n\r", "ack_policy",  p->ack_policy);
	LOG_PRINTF("%20s : %d\n\r", "RSVD_1",		p->RSVD_1);
	LOG_PRINTF("%20s : %d\n\r", "do_rts_cts",  p->do_rts_cts);
	LOG_PRINTF("%20s : %d\n\r", "reason",		p->reason);
	LOG_PRINTF("%20s : %d\n\r", "payload_offset",	p->payload_offset);
	LOG_PRINTF("%20s : %d\n\r", "next_frag_pid",	p->next_frag_pid);
	LOG_PRINTF("%20s : %d\n\r", "RSVD_2",		p->RSVD_2);
	LOG_PRINTF("%20s : %d\n\r", "fCmdIdx",		p->fCmdIdx);
	LOG_PRINTF("%20s : %d\n\r", "wsid",		p->wsid);
	LOG_PRINTF("%20s : %d\n\r", "txq_idx",		p->txq_idx);
	LOG_PRINTF("%20s : %d\n\r", "TxF_ID",		p->TxF_ID);
	LOG_PRINTF("%20s : %d\n\r", "rts_cts_nav",	p->rts_cts_nav);
	LOG_PRINTF("%20s : %d\n\r", "frame_consume_time",	p->frame_consume_time);
	LOG_PRINTF("%20s : %d\n\r", "RSVD_3",		p->RSVD_3);
	// printf("%20s : %d\n\r", "RSVD_5",		p->RSVD_5);
	LOG_PRINTF("============================\n\r");
	payload_len = p->len - p->hdr_len;
	LOG_PRINTF("%20s : %d\n\r", "payload_len", payload_len);

	dat = (u8 *)p + p->hdr_offset;
	LOG_PRINTF("========== hdr     ==========\n\r");
	LOG_PRINTF("frame ctl     : 0x%04x\n\r", (((u16)dat[1] << 8)|dat[0]));
	LOG_PRINTF("  - more_frag : %d\n\r", GET_HDR80211_FC_MOREFRAG(p));

	a = (u8*)p + p->hdr_offset +  4;
	LOG_PRINTF("address 1     : %02x:%02x:%02x:%02x:%02x:%02x\n\r", a[0], a[1], a[2], a[3], a[4], a[5]);
	LOG_PRINTF("address 2     : %02x:%02x:%02x:%02x:%02x:%02x\n\r", a[6], a[7], a[8], a[9], a[10], a[11]);
	LOG_PRINTF("address 3     : %02x:%02x:%02x:%02x:%02x:%02x\n\r", a[12], a[13], a[14], a[15], a[16], a[17]);

	LOG_PRINTF("seq ctl       : 0x%04x\n\r", (((u16)dat[23] << 8)|dat[22]));
	LOG_PRINTF("  - seq num   : %d\n\r", GET_HDR80211_SC_SEQNUM(p));
	LOG_PRINTF("  - frag num  : %d\n\r", GET_HDR80211_SC_FRAGNUM(p));


	return;
}

void pkt_dump_rxinfo(PKT_RxInfo *p)
{
	u32		payload_len;
	u8		*dat;
	u8		*a;

	LOG_PRINTF("========= RxInfo =========\n\r");
	LOG_PRINTF("%20s : %d\n\r", "len",	        p->len);
	LOG_PRINTF("%20s : %d\n\r", "c_type",	    p->c_type);
	LOG_PRINTF("%20s : %d\n\r", "f80211",	    p->f80211);
	LOG_PRINTF("%20s : %d\n\r", "qos",	        p->qos);
	LOG_PRINTF("%20s : %d\n\r", "ht",		    p->ht);
	LOG_PRINTF("%20s : %d\n\r", "l3cs_err",	p->l3cs_err);
	LOG_PRINTF("%20s : %d\n\r", "l4cs_err",	p->l4cs_err);
	LOG_PRINTF("%20s : %d\n\r", "use_4addr",	p->use_4addr);
	LOG_PRINTF("%20s : %d\n\r", "RSVD_0",		p->RSVD_0);
	LOG_PRINTF("%20s : %d\n\r", "psm",	        p->psm);
	LOG_PRINTF("%20s : %d\n\r", "stype_b5b4",	p->stype_b5b4);
	LOG_PRINTF("%20s : %d\n\r", "extra_info",	p->extra_info);
	LOG_PRINTF("%20s : 0x%08x\n\r", "fCmd",	p->fCmd);
	LOG_PRINTF("%20s : %d\n\r", "hdr_offset",	p->hdr_offset);
	LOG_PRINTF("%20s : %d\n\r", "frag",	    p->frag);
	LOG_PRINTF("%20s : %d\n\r", "unicast",	    p->unicast);
	LOG_PRINTF("%20s : %d\n\r", "hdr_len",	    p->hdr_len);
	LOG_PRINTF("%20s : 0x%x\n\r", "RxResult",	p->RxResult);
	LOG_PRINTF("%20s : %d\n\r", "wildcard_bssid",	p->wildcard_bssid);
	LOG_PRINTF("%20s : %d\n\r", "RSVD_1",	    p->RSVD_1);
	LOG_PRINTF("%20s : %d\n\r", "reason",	    p->reason);
	LOG_PRINTF("%20s : %d\n\r", "payload_offset",	p->payload_offset);
	LOG_PRINTF("%20s : %d\n\r", "next_frag_pid",	p->next_frag_pid);
	LOG_PRINTF("%20s : %d\n\r", "RSVD_2",	    p->RSVD_2);
	LOG_PRINTF("%20s : %d\n\r", "fCmdIdx",	    p->fCmdIdx);
	LOG_PRINTF("%20s : %d\n\r", "wsid",	    p->wsid);
	LOG_PRINTF("%20s : %d\n\r", "RSVD_3",	    p->RSVD_3);
//	LOG_PRINTF("%20s : %d\n\r", "RxF_ID",	    p->RxF_ID);
	LOG_PRINTF("============================\n\r");

	payload_len = p->len - p->hdr_len;
	LOG_PRINTF("%20s : %d\n\r", "payload_len", payload_len);

	dat = (u8 *)p + p->hdr_offset;
	LOG_PRINTF("========== hdr     ==========\n\r");
	LOG_PRINTF("frame ctl     : 0x%04x\n\r", (((u16)dat[1] << 8)|dat[0]));
	LOG_PRINTF("  - more_frag : %d\n\r", GET_HDR80211_FC_MOREFRAG(p));

	a = (u8*)p + p->hdr_offset +  4;
	LOG_PRINTF("address 1     : %02x:%02x:%02x:%02x:%02x:%02x\n\r", a[0], a[1], a[2], a[3], a[4], a[5]);
	LOG_PRINTF("address 2     : %02x:%02x:%02x:%02x:%02x:%02x\n\r", a[6], a[7], a[8], a[9], a[10], a[11]);
	LOG_PRINTF("address 3     : %02x:%02x:%02x:%02x:%02x:%02x\n\r", a[12], a[13], a[14], a[15], a[16], a[17]);

	LOG_PRINTF("seq ctl       : 0x%04x\n\r", (((u16)dat[23] << 8)|dat[22]));
	LOG_PRINTF("  - seq num   : %d\n\r", GET_HDR80211_SC_SEQNUM(p));
	LOG_PRINTF("  - frag num  : %d\n\r", GET_HDR80211_SC_FRAGNUM(p));

	return;
}





LIB_APIs void hex_dump (const void *addr, u32 size)
{
    u32 i, j;
    const u32 *data = (const u32 *)addr;

#if (__SSV_UNIX_SIM__)
    LOG_TAG_SUPPRESS_ON();
#endif
    LOG_PRINTF("        ");
    for (i = 0; i < 8; i++)
        LOG_PRINTF("       %02X", i*sizeof(u32));

    LOG_PRINTF("\r\n--------");
    for (i = 0; i < 8; i++)
        LOG_PRINTF("+--------");

    for (i = 0; i < size; i+= 8)
    {
        LOG_PRINTF("\r\n%08X:%08X", (s32)data, data[0]);
        for (j = 1; j < 8; j++)
        {
            LOG_PRINTF(" %08X", data[j]);
        }
        data = &data[8];
    }
    LOG_PRINTF("\r\n");
#if (__SSV_UNIX_SIM__)
    LOG_TAG_SUPPRESS_OFF();
#endif
    return;
}

LIB_APIs void halt(void)
{
#if (__SSV_UNIX_SIM__)
    abort();
//	system("pause");
//	exit(EXIT_FAILURE);
#else
	/*lint -save -e716 */
    while (1) ;
	/*lint -restore */
#endif
}

//===================HW related=============================
extern const char *date;
extern const char *version;

#if (CONFIG_CHIP_ID != SSV6060P)
#define DO_IQ_CALIBRATION 1
#endif

#if DO_IQ_CALIBRATION
struct ssv6xxx_iqk_cfg init_iqk_cfg;
int ssv6xxx_do_iq_calib(struct ssv6xxx_iqk_cfg *p_cfg)
{
    void          *frame;
    struct cfg_host_cmd     *host_cmd;
    u32 fw_status=0;

    LOG_PRINTF("# Do init_cali (iq)\r\n");
    if((PHY_SETTING_SIZE > MAX_PHY_SETTING_TABLE_SIZE) ||
        (RF_SETTING_SIZE > MAX_RF_SETTING_TABLE_SIZE))
    {
        LOG_PRINTF("Please recheck RF or PHY table size!!!\n");
        return 0;
    }
    //return ;
    // make command packet
    frame = OS_MemAlloc(HOST_CMD_HDR_LEN + IQK_CFG_LEN + PHY_SETTING_SIZE + RF_SETTING_SIZE);

    if(frame == NULL)
    {
        LOG_PRINTF("init ssv6xxx_do_iq_calib fail!!!\n");
        return 0;
    }

    host_cmd = (struct cfg_host_cmd *)frame;

    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd  = (u8)SSV6XXX_HOST_CMD_INIT_CALI;
    host_cmd->len    = HOST_CMD_HDR_LEN + IQK_CFG_LEN + PHY_SETTING_SIZE + RF_SETTING_SIZE;

    p_cfg->cfg_xtal=SSV6XXX_IQK_CFG_XTAL_26M;
#ifdef CONFIG_SSV_DPD
    p_cfg->cfg_pa=SSV6XXX_IQK_CFG_PA_LI_MPB;
#else
    p_cfg->cfg_pa=SSV6XXX_IQK_CFG_PA_DEF;
#endif
    p_cfg->cfg_tssi_trgt=26;
    p_cfg->cfg_tssi_div=3;

#if (CONFIG_CHIP_ID == SSV6051Z)
    p_cfg->cfg_def_tx_scale_11b=(wifi_tx_gain[4]>>0) & 0xff;
    p_cfg->cfg_def_tx_scale_11b_p0d5=(wifi_tx_gain[4]>>8) & 0xff;
    p_cfg->cfg_def_tx_scale_11g=(wifi_tx_gain[4]>>16) & 0xff;
    p_cfg->cfg_def_tx_scale_11g_p0d5=(wifi_tx_gain[4]>>24) & 0xff;
    p_cfg->cfg_papd_tx_scale_11b=(wifi_tx_gain[4]>>0) & 0xff;
    p_cfg->cfg_papd_tx_scale_11b_p0d5=(wifi_tx_gain[4]>>8) & 0xff;
    p_cfg->cfg_papd_tx_scale_11g=((wifi_tx_gain[4]>>16) & 0xff)+0x30;
    p_cfg->cfg_papd_tx_scale_11g_p0d5=((wifi_tx_gain[4]>>24) & 0xff)+0x30;
#elif (CONFIG_CHIP_ID == SSV6060P)
	Fill tx gain
#else
    p_cfg->cfg_def_tx_scale_11b=(wifi_tx_gain[6]>>0) & 0xff;
    p_cfg->cfg_def_tx_scale_11b_p0d5=(wifi_tx_gain[6]>>8) & 0xff;
    p_cfg->cfg_def_tx_scale_11g=(wifi_tx_gain[4]>>16) & 0xff;
    p_cfg->cfg_def_tx_scale_11g_p0d5=(wifi_tx_gain[4]>>24) & 0xff;
    p_cfg->cfg_papd_tx_scale_11b=(wifi_tx_gain[6]>>0) & 0xff;
    p_cfg->cfg_papd_tx_scale_11b_p0d5=(wifi_tx_gain[6]>>8) & 0xff;
    p_cfg->cfg_papd_tx_scale_11g=((wifi_tx_gain[4]>>16) & 0xff)+0x30;
    p_cfg->cfg_papd_tx_scale_11g_p0d5=((wifi_tx_gain[4]>>24) & 0xff)+0x30;

#endif

    p_cfg->cmd_sel=SSV6XXX_IQK_CMD_INIT_CALI;
#ifdef CONFIG_SSV_DPD
    p_cfg->un.fx_sel=SSV6XXX_IQK_TEMPERATURE+SSV6XXX_IQK_RXDC+SSV6XXX_IQK_RXRC+ SSV6XXX_IQK_TXDC+ SSV6XXX_IQK_TXIQ+ SSV6XXX_IQK_RXIQ+SSV6XXX_IQK_PAPD;
#else
    p_cfg->un.fx_sel=SSV6XXX_IQK_TEMPERATURE+SSV6XXX_IQK_RXDC+SSV6XXX_IQK_RXRC+ SSV6XXX_IQK_TXDC+ SSV6XXX_IQK_TXIQ+ SSV6XXX_IQK_RXIQ;
#endif
    p_cfg->phy_tbl_size = PHY_SETTING_SIZE;
    p_cfg->rf_tbl_size = RF_SETTING_SIZE;

    OS_MemCPY(host_cmd->un.dat32, p_cfg, IQK_CFG_LEN);

    OS_MemCPY(host_cmd->un.dat8+IQK_CFG_LEN, phy_setting, PHY_SETTING_SIZE);
    OS_MemCPY(host_cmd->un.dat8+IQK_CFG_LEN+PHY_SETTING_SIZE, ssv6200_rf_tbl, RF_SETTING_SIZE);

    MAC_REG_WRITE(FW_STATUS_REG,fw_status);

    ssv6xxx_drv_send(host_cmd,host_cmd->len);

    OS_MemFree(frame);
    LOG_PRINTF("Wait iq ... \r\n");
    do{
        MAC_REG_READ(FW_STATUS_REG,fw_status);
    }while(fw_status!=0x5A5AA5A5);
    LOG_PRINTF("IQ Done ... \r\n");
    return 0;
}
#endif

bool SSV6XXX_SET_HW_TABLE(const ssv_cabrio_reg tbl_[], u32 size)
{
    bool ret = FALSE ;
    u32 i=0;
    for(; i<size; i++) {
        ret = MAC_REG_WRITE(tbl_[i].address, tbl_[i].data);
        if (ret==FALSE) break;
    }
    return ret;
}

int ssv6xxx_chip_init()
{
    bool ret;


#ifdef CONFIG_SSV_CABRIO_E
        u32 i,regval=0;
#endif

#ifdef CONFIG_SSV_CABRIO_E
        /*
                    Temp solution.Default 26M.
            */
        //priv->crystal_type = SSV6XXX_IQK_CFG_XTAL_26M;
        /*
            //Xctal setting
            Remodify RF setting For 24M 26M 40M or other xtals.
            */
        //if(priv->crystal_type == SSV6XXX_IQK_CFG_XTAL_26M)
        {
            //init_iqk_cfg.cfg_xtal = SSV6XXX_IQK_CFG_XTAL_26M;
            //printk("SSV6XXX_IQK_CFG_XTAL_26M\n");

            for(i=0; i<sizeof(ssv6200_rf_tbl)/sizeof(struct ssv6xxx_dev_table); i++)
            {
                //0xCE010038
                if(ssv6200_rf_tbl[i].address == ADR_SX_ENABLE_REGISTER)
                {
                    ssv6200_rf_tbl[i].data &= 0xFFFF7FFF;
                    ssv6200_rf_tbl[i].data |= 0x00008000;
                }
                //0xCE010060
                if(ssv6200_rf_tbl[i].address == ADR_DPLL_DIVIDER_REGISTER)
                {
                    ssv6200_rf_tbl[i].data &= 0xE0380FFF;
                    ssv6200_rf_tbl[i].data |= 0x00406000;
                }
                //0xCE01009C
                if(ssv6200_rf_tbl[i].address == ADR_DPLL_FB_DIVIDER_REGISTERS_I)
                {
                    ssv6200_rf_tbl[i].data &= 0xFFFFF800;
                    ssv6200_rf_tbl[i].data |= 0x00000024;
                }
                //0xCE0100A0
                if(ssv6200_rf_tbl[i].address == ADR_DPLL_FB_DIVIDER_REGISTERS_II)
                {
                    ssv6200_rf_tbl[i].data &= 0xFF000000;
                    ssv6200_rf_tbl[i].data |= 0x00EC4CC5;
                }
            }
        }
        #if 0
        else if(priv->crystal_type == SSV6XXX_IQK_CFG_XTAL_40M)
        {
            //init_iqk_cfg.cfg_xtal = SSV6XXX_IQK_CFG_XTAL_40M;
            printk("SSV6XXX_IQK_CFG_XTAL_40M\n");
            for(i=0; i<sizeof(ssv6200_rf_tbl)/sizeof(struct ssv6xxx_dev_table); i++)
            {
                //0xCE010038
                if(ssv6200_rf_tbl[i].address == ADR_SX_ENABLE_REGISTER)
                {
                    ssv6200_rf_tbl[i].data &= 0xFFFF7FFF;
                    ssv6200_rf_tbl[i].data |= 0x00000000;
                }
                //0xCE010060
                if(ssv6200_rf_tbl[i].address == ADR_DPLL_DIVIDER_REGISTER)
                {
                    ssv6200_rf_tbl[i].data &= 0xE0380FFF;
                    ssv6200_rf_tbl[i].data |= 0x00406000;
                }
                //0xCE01009C
                if(ssv6200_rf_tbl[i].address == ADR_DPLL_FB_DIVIDER_REGISTERS_I)
                {
                    ssv6200_rf_tbl[i].data &= 0xFFFFF800;
                    ssv6200_rf_tbl[i].data |= 0x00000030;
                }
                //0xCE0100A0
                if(ssv6200_rf_tbl[i].address == ADR_DPLL_FB_DIVIDER_REGISTERS_II)
                {
                    ssv6200_rf_tbl[i].data &= 0xFF000000;
                    ssv6200_rf_tbl[i].data |= 0x00EC4CC5;
                }
            }
        }
        else if(priv->crystal_type == SSV6XXX_IQK_CFG_XTAL_24M)
        {
            printk("SSV6XXX_IQK_CFG_XTAL_24M\n");
            //init_iqk_cfg.cfg_xtal = SSV6XXX_IQK_CFG_XTAL_24M;
            for(i=0; i<sizeof(ssv6200_rf_tbl)/sizeof(struct ssv6xxx_dev_table); i++)
            {
                //0xCE010038
                if(ssv6200_rf_tbl[i].address == ADR_SX_ENABLE_REGISTER)
                {
                    ssv6200_rf_tbl[i].data &= 0xFFFF7FFF;
                    ssv6200_rf_tbl[i].data |= 0x00008000;
                }
                //0xCE010060
                if(ssv6200_rf_tbl[i].address == ADR_DPLL_DIVIDER_REGISTER)
                {
                    ssv6200_rf_tbl[i].data &= 0xE0380FFF;
                    ssv6200_rf_tbl[i].data |= 0x00406000;
                }
                //0xCE01009C
                if(ssv6200_rf_tbl[i].address == ADR_DPLL_FB_DIVIDER_REGISTERS_I)
                {
                    ssv6200_rf_tbl[i].data &= 0xFFFFF800;
                    ssv6200_rf_tbl[i].data |= 0x00000028;
                }
                //0xCE0100A0
                if(ssv6200_rf_tbl[i].address == ADR_DPLL_FB_DIVIDER_REGISTERS_II)
                {
                    ssv6200_rf_tbl[i].data &= 0xFF000000;
                    ssv6200_rf_tbl[i].data |= 0x00000000;
                }
            }
        }
        else
        {
            printk("Illegal xtal setting \n");
            BUG_ON(1);
        }
        #endif
#endif

    /* reset ssv6200 mac */

    MAC_REG_WRITE(ADR_BRG_SW_RST, 1 << 1);  /* bug if reset ?? */


    //write rf table
    ret = SSV6XXX_SET_HW_TABLE(ssv6200_rf_tbl,sizeof(ssv6200_rf_tbl));

    if (ret == TRUE) ret = MAC_REG_WRITE( 0xce000004, 0x00000000); /* ???? */

    /* Turn off phy before configuration */

    //write phy table
    if (ret == TRUE) ret = SSV6XXX_SET_HW_TABLE(ssv6200_phy_tbl,sizeof(ssv6200_phy_tbl));

#ifdef CONFIG_SSV_CABRIO_E
    //Avoid SDIO issue.

    if (ret == TRUE) ret = MAC_REG_WRITE(ADR_TRX_DUMMY_REGISTER, 0xEAAAAAAA);
    if (ret == TRUE)  MAC_REG_READ(ADR_TRX_DUMMY_REGISTER,regval);

    if(regval != 0xEAAAAAAA)
    {
        LOG_PRINTF("@@@@@@@@@@@@\r\n");
        LOG_PRINTF(" SDIO issue -- please check 0xCE01008C %08x!!\r\n",regval);
        LOG_PRINTF(" It shouble be 0xEAAAAAAA!!\r\n");
        LOG_PRINTF("@@@@@@@@@@@@ \r\n");
    }
#endif

#ifdef CONFIG_SSV_CABRIO_E
    /* Cabrio E: GPIO setting */

    if (ret == TRUE) ret = MAC_REG_WRITE(ADR_PAD53, 0x21);        /* like debug-uart config ? */
    if (ret == TRUE) ret = MAC_REG_WRITE(ADR_PAD54, 0x3000);
    if (ret == TRUE) ret = MAC_REG_WRITE(ADR_PIN_SEL_0, 0x4000);

    MAC_REG_READ(ADR_PAD20,regval);
    if (ret == TRUE) ret = MAC_REG_WRITE(ADR_PAD20, regval|0x01);

    /* TR switch: */
    if (ret == TRUE) ret = MAC_REG_WRITE(0xc0000304, 0x01);
    if (ret == TRUE) ret = MAC_REG_WRITE(0xc0000308, 0x01);

#endif // CONFIG_SSV_CABRIO_E

    //Switch clock to PLL output of RF
    //MAC and MCU clock selection :   00 : OSC clock   01 : RTC clock   10 : synthesis 80MHz clock   11 : synthesis 40MHz clock


    if (ret == TRUE) ret = MAC_REG_WRITE(ADR_CLOCK_SELECTION, 0x3);


#ifdef CONFIG_SSV_CABRIO_E
        //Avoid consumprion of electricity
        //SDIO issue

    if (ret == TRUE) ret = MAC_REG_WRITE(ADR_TRX_DUMMY_REGISTER, 0xAAAAAAAA);
#endif

    MAC_REG_WRITE(ADR_SYS_INT_FOR_HOST, 0);
    /* reset ssv6200 mac */
    MAC_REG_WRITE(ADR_BRG_SW_RST, 1 << 1);  /* bug if reset ?? */

	if(ret == TRUE)
		return 1;
	else
		return 0;

}

int cabrio_init_mac()
{
    int i;
    bool ret;
    u32 regval,id_len,j;
    char    chip_id[24]="";
    u32     chip_tag1,chip_tag2;
    u32 *ptr;


    //CHIP TAG

    MAC_REG_READ(ADR_IC_TIME_TAG_1,regval);
    chip_tag1 = regval;
    MAC_REG_READ(ADR_IC_TIME_TAG_0,regval);
    chip_tag2= regval;
    //LOG_DEBUG("CHIP TAG: %llx \r\n",chip_tag);
    LOG_DEBUG("CHIP TAG: %08x%08x \r\n",chip_tag1,chip_tag2);
    //CHIP ID
    MAC_REG_READ(ADR_CHIP_ID_3,regval);
    *((u32 *)&chip_id[0]) = (u32)LONGSWAP(regval);
    MAC_REG_READ(ADR_CHIP_ID_2,regval);
    *((u32 *)&chip_id[4]) = (u32)LONGSWAP(regval);
    MAC_REG_READ(ADR_CHIP_ID_1,regval);
    *((u32 *)&chip_id[8]) = (u32)LONGSWAP(regval);
    MAC_REG_READ(ADR_CHIP_ID_0,regval);

    *((u32 *)&chip_id[12]) = (u32)LONGSWAP(regval);
    LOG_DEBUG("CHIP ID: %s \r\n", chip_id);
    LOG_DEBUG("RF TABLE: %s \r\n", chip_str[CONFIG_CHIP_ID]);
	LOG_DEBUG("SW VERSION: %s BUILD DATE: %s \r\n", version, date);

    //soc set HDR-STRIP-OFF       enable
    //soc set HCI-RX2HOST         enable
    //soc set AUTO-SEQNO          enable
    //soc set ERP-PROTECT          disable
    //soc set MGMT-TXQID            3
    //soc set NONQOS-TXQID      1
    regval = (RX_2_HOST_MSK|AUTO_SEQNO_MSK|HDR_STRIP_MSK|(3<<TXQ_ID0_SFT)|(1<<TXQ_ID1_SFT)|RX_ETHER_TRAP_EN_MSK);

    ret = MAC_REG_WRITE(ADR_CONTROL,regval);

    /* Enable hardware timestamp for TSF */
    // 28 => time stamp write location
    if(ret == TRUE) ret = MAC_REG_WRITE(ADR_RX_TIME_STAMP_CFG,((28<<MRX_STP_OFST_SFT)|0x01));

    //MMU[decide packet buffer no.]
    /* Setting MMU to 256 pages */
//    MAC_REG_READ(ADR_MMU_CTRL, regval);
//    regval |= (0xff<<MMU_SHARE_MCU_SFT);
//    MAC_REG_WRITE(ADR_MMU_CTRL, regval);

    /**
        * Tx/RX threshold setting for packet buffer resource.
        */

    MAC_REG_READ(ADR_TRX_ID_THRESHOLD,id_len);
    id_len = (id_len&0xffff0000 ) |
             (SSV6200_ID_TX_THRESHOLD<<TX_ID_THOLD_SFT)|
             (SSV6200_ID_RX_THRESHOLD<<RX_ID_THOLD_SFT);
    if(ret == TRUE) ret = MAC_REG_WRITE(ADR_TRX_ID_THRESHOLD, id_len);

    MAC_REG_READ(ADR_ID_LEN_THREADSHOLD1,id_len);
    id_len = (id_len&0x0f )|
             (SSV6200_PAGE_TX_THRESHOLD<<ID_TX_LEN_THOLD_SFT)|
             (SSV6200_PAGE_RX_THRESHOLD<<ID_RX_LEN_THOLD_SFT);
    if(ret == TRUE) ret = MAC_REG_WRITE(ADR_ID_LEN_THREADSHOLD1, id_len);

    if(ret == TRUE) ret = MAC_REG_WRITE(ADR_STA_MAC_0, *((u32 *)&(gDeviceInfo->self_mac[0])));
    if(ret == TRUE) ret = MAC_REG_WRITE(ADR_STA_MAC_1, *((u32 *)&(gDeviceInfo->self_mac[4])));
    /**
        * Reset all wsid table entry to invalid.
        */
    if(ret == TRUE) ret = MAC_REG_WRITE(ADR_WSID0, 0x00000000);
    if(ret == TRUE) ret = MAC_REG_WRITE(ADR_WSID1, 0x00000000);

    if(ret == TRUE) ret = MAC_REG_WRITE(ADR_SDIO_MASK, 0xfffe1fff);
#if 0
    //Enable EDCA low threshold
    MAC_REG_WRITE(ADR_MB_THRESHOLD6, 0x80000000);
    //Enable EDCA low threshold EDCA-1[8] EDCA-0[4]
    MAC_REG_WRITE(ADR_MB_THRESHOLD8, 0x08040000);
    //Enable EDCA low threshold EDCA-3[8] EDCA-2[8]
    MAC_REG_WRITE(ADR_MB_THRESHOLD9, 0x00000808);
#endif

    /**
        * Disable tx/rx ether trap table.
        */

    if(ret == TRUE) ret = MAC_REG_WRITE(ADR_TX_ETHER_TYPE_0, 0x00000000);
    if(ret == TRUE) ret = MAC_REG_WRITE(ADR_TX_ETHER_TYPE_1, 0x00000000);
    if(ret == TRUE) ret = MAC_REG_WRITE(ADR_RX_ETHER_TYPE_0, 0x00000000);
    if(ret == TRUE) ret = MAC_REG_WRITE(ADR_RX_ETHER_TYPE_1, 0x00000000);


//-----------------------------------------------------------------------------------------------------------------------------------------
//PHY and security table
    /**
        * Allocate a hardware packet buffer space. This buffer is for security
        * key caching and phy info space.
        */
    /*lint -save -e732  Loss of sign (assignment) (int to unsigned int)*/
    gDeviceInfo->hw_buf_ptr = ssv6xxx_pbuf_alloc((s32)sizeof(phy_info_tbl)+
                                            (s32)sizeof(struct ssv6xxx_hw_sec),(int)NOTYPE_BUF);
   /*lint -restore */
    if((gDeviceInfo->hw_buf_ptr>>28) != 8)
    {
    	//asic pbuf address start from 0x8xxxxxxxx
    	LOG_PRINTF("opps allocate pbuf error\n");
    	//WARN_ON(1);
    	ret = 1;
    	goto exit;
    }

    LOG_PRINTF("%s(): ssv6200 reserved space=0x%08x, size=%d\r\n",
        __FUNCTION__, gDeviceInfo->hw_buf_ptr, (u32)(sizeof(phy_info_tbl)+sizeof(struct ssv6xxx_hw_sec)));



/**
Part 1. SRAM
	**********************
	*				          *
	*	1. Security key table *
	* 				          *
	* *********************
	*				          *
	*    	2. PHY index table     *
	* 				          *
	* *********************
	* 				          *
	*	3. PHY ll-length table *
	*				          *
	* *********************
=============================================
Part 2. Register
	**********************
	*				          *
	*	PHY Infor Table         *
	* 				          *
	* *********************
*
*/

    /**
        * Init ssv6200 hardware security table: clean the table.
        * And set PKT_ID for hardware security.
        */
    gDeviceInfo->hw_sec_key = gDeviceInfo->hw_buf_ptr;

	//==>Section 1. Write Sec table to SRAM
    for(j=0; j<sizeof(struct ssv6xxx_hw_sec); j+=4) {
        MAC_REG_WRITE(gDeviceInfo->hw_sec_key+j, 0);
    }
    /*lint -save -e838*/
    regval = ((gDeviceInfo->hw_sec_key >> 16) << SCRT_PKT_ID_SFT);
    MAC_REG_READ(ADR_SCRT_SET, regval);
	regval &= SCRT_PKT_ID_I_MSK;
	regval |= ((gDeviceInfo->hw_sec_key >> 16) << SCRT_PKT_ID_SFT);
	MAC_REG_WRITE(ADR_SCRT_SET, regval);
    /*lint -restore*/

    /**
        * Set default ssv6200 phy infomation table.
        */
    gDeviceInfo->hw_pinfo = gDeviceInfo->hw_sec_key + sizeof(struct ssv6xxx_hw_sec);
    for(i=0, ptr=phy_info_tbl; i<PHY_INFO_TBL1_SIZE; i++, ptr++) {
        MAC_REG_WRITE(ADR_INFO0+(u32)i*4, *ptr);
    }


	//==>Section 2. Write PHY index table and PHY ll-length table to SRAM
	for(i=0; i<PHY_INFO_TBL2_SIZE; i++, ptr++) {
        MAC_REG_WRITE(gDeviceInfo->hw_pinfo+(u32)i*4, *ptr);
    }
    for(i=0; i<PHY_INFO_TBL3_SIZE; i++, ptr++) {
        MAC_REG_WRITE(gDeviceInfo->hw_pinfo+(PHY_INFO_TBL2_SIZE<<2)+(u32)i*4, *ptr);
    }


    MAC_REG_WRITE(ADR_INFO_RATE_OFFSET, 0x00040000);

	//Set SRAM address to register
	MAC_REG_READ(ADR_INFO_IDX_ADDR, regval);
    LOG_PRINTF("ADR_INFO_IDX_ADDR:%08x\r\n",regval);
	MAC_REG_WRITE(ADR_INFO_IDX_ADDR, gDeviceInfo->hw_pinfo);
    MAC_REG_WRITE(ADR_INFO_LEN_ADDR, gDeviceInfo->hw_pinfo+(PHY_INFO_TBL2_SIZE)*4); //4byte for one entry
    MAC_REG_READ(ADR_INFO_IDX_ADDR, regval);

	LOG_PRINTF("ADR_INFO_IDX_ADDR[%08x] ADR_INFO_LEN_ADDR[%08x]\r\n", regval, gDeviceInfo->hw_pinfo+(PHY_INFO_TBL2_SIZE)*4);

    //-----------------------------------------------------------------------------------------------------------------------------------------

    if(ret == TRUE) ret = MAC_REG_WRITE(0xca000800,0xffffffff);

    if(ret == TRUE) ret = MAC_REG_WRITE(0xCE000004,0x0000017F);//PHY b/g/n on

    //-----------------------------------------------------------------------------------------------------------------------------------------
    /* Set wmm parameter to EDCA Q4
        (use to send mgmt frame/null data frame in STA mode and broadcast frame in AP mode) */
        //C_REG_WRITE(ADR_TXQ4_MTX_Q_AIFSN, 0xffff2101);

    /* Setup q4 behavior STA mode-> act as normal queue
      *
      */
        MAC_REG_READ(ADR_MTX_BCN_EN_MISC,regval);
        regval&= ~(MTX_HALT_MNG_UNTIL_DTIM_MSK);
        regval |= (0);
    if(ret == TRUE) ret = MAC_REG_WRITE( ADR_MTX_BCN_EN_MISC, regval);

    //-----------------------------------------------------------------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------------------------------------------------------------
    //MMU[decide packet buffer no.]
        /* Setting MMU to 256 pages */

        //MAC_REG_READ(ADR_MMU_CTRL, regval);
        //regval |= (0xff<<MMU_SHARE_MCU_SFT);
        //MAC_REG_WRITE(ADR_MMU_CTRL, regval);

    //-----------------------------------------------------------------------------------------------------------------------------------------

    if(ret == TRUE) ret = MAC_REG_WRITE(ADR_INFO_RATE_OFFSET, 0x00040000);

    exit:
    //Load FW
//    LOG_PRINTF("bin size =%d\r\n",sizeof(ssv6200_uart_bin));
    MAC_LOAD_FW((u8 *)ssv6200_uart_bin,sizeof(ssv6200_uart_bin));//??? u8* bin

    ssv6xxx_drv_irq_enable();

    #if(SSV_IPD==1)
    {
        const unsigned char cmd_data=1;
        _ssv6xxx_wifi_ioctl_Ext(SSV6XXX_HOST_CMD_IPD, &cmd_data, sizeof(cmd_data), TRUE, FALSE);
        LOG_PRINTF("SSV_IPD\r\n");
    }
    #endif
    return ((ret == TRUE)?0:1);
}

int cabrio_init_sta_mac()
{
    int i, ret=0;
    u16 *mac_deci_tbl;

    MAC_REG_WRITE(ADR_GLBLE_SET,
        //(0 << OP_MODE_SFT)  |                           /* STA mode by default */
        //(0 << SNIFFER_MODE_SFT) |                           /* disable sniffer mode */
        (1 << DUP_FLT_SFT) |                           /* Enable duplicate detection */
        (3 << TX_PKT_RSVD_SFT) |                           /* PKT Reserve */
        (80 << PB_OFFSET_SFT)                          /* set rx packet buffer offset */
    );

    MAC_REG_WRITE(ADR_BSSID_0,   0x00000000);//*((u32 *)&priv->bssid[0]));
    MAC_REG_WRITE(ADR_BSSID_1,   0x00000000);//*((u32 *)&priv->bssid[4]));

     /**
        * Set reason trap to discard frames.
        */
    MAC_REG_WRITE(ADR_REASON_TRAP0, 0x7FBC7F8F);
    MAC_REG_WRITE(ADR_REASON_TRAP1, 0x00000000);

    //soc set TX-FLOW-MGMT        { M_ENG_HWHCI M_ENG_ENCRYPT M_ENG_TX_EDCA0 M_ENG_CPU }
    //soc set TX-FLOW-CTRL        { M_ENG_HWHCI M_ENG_TX_EDCA0 M_ENG_CPU M_ENG_CPU }


	MAC_REG_WRITE(ADR_TX_FLOW_0,  M_ENG_HWHCI|(M_ENG_TX_MNG<<4)|
	                              /*(M_ENG_CPU<<8)|
								  (M_ENG_CPU<<12)|*/
		 						(M_ENG_HWHCI<<16)|
								(M_ENG_ENCRYPT<<20)|
								(M_ENG_TX_MNG<<24)/*|
								(M_ENG_CPU<<28)*/);
    //Info :845 the right argument to operator '|' is certain to be 0


    // soc set TX-FLOW-DATA        { M_ENG_HWHCI M_ENG_MIC M_ENG_FRAG M_ENG_ENCRYPT M_CPU_EDCATX M_ENG_TX_EDCA0 M_ENG_CPU M_ENG_CPU }
    {
        const unsigned char cmd_data[] = {
            0x51, 0x03, 0x06, 0x00, 0x00, 0x40, 0x00, 0x00};
        _ssv6xxx_wifi_ioctl_Ext(SSV6XXX_HOST_CMD_SET_FCMD_TXDATA, (void *)cmd_data, 8, TRUE, FALSE);
    }
    // soc set RX-FLOW-DATA        { M_ENG_MACRX M_ENG_ENCRYPT_SEC M_CPU_DEFRAG M_ENG_MIC_SEC M_ENG_HWHCI M_ENG_CPU M_ENG_CPU M_ENG_CPU }
    //0xB4, 0xC0, 0x01, 0x00, 0x00, 0x03, 0x00, 0x00};
    // soc set RX-FLOW-DATA        { M_ENG_MACRX M_ENG_ENCRYPT_SEC M_ENG_CPU M_ENG_MIC_SEC M_ENG_HWHCI M_ENG_CPU M_ENG_CPU M_ENG_CPU }
    {
        const unsigned char cmd_data[] = {
            0xB4, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        _ssv6xxx_wifi_ioctl_Ext(SSV6XXX_HOST_CMD_SET_FCMD_RXDATA, (void *)cmd_data, 8, TRUE, FALSE);
    }
    // soc set RX-FLOW-MGMT        { M_ENG_MACRX M_CPU_RXMGMT M_ENG_CPU M_ENG_CPU }
    {
        const unsigned char cmd_data[] = {
            0x04, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00};
        _ssv6xxx_wifi_ioctl_Ext(SSV6XXX_HOST_CMD_SET_FCMD_RXMGMT, (void *)cmd_data, 8, TRUE, FALSE);
    }
    // soc set RX-FLOW-CTRL        { M_ENG_MACRX M_ENG_CPU M_ENG_CPU M_ENG_CPU }
    {
        const unsigned char cmd_data[] = {
            0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        _ssv6xxx_wifi_ioctl_Ext(SSV6XXX_HOST_CMD_SET_FCMD_RXCTRL, (void *)cmd_data, 8, TRUE, FALSE);
    }


    //#set EDCA parameter AP-a/g BK[0], BE[1], VI[2], VO[3]
    //soc set WMM-PARAM[0]      { aifsn=0 acm=0 cwmin=5 cwmax=10 txop=0 backoffvalue=6 }


    MAC_REG_WRITE(ADR_TXQ0_MTX_Q_AIFSN,

          (6 << TXQ0_MTX_Q_AIFSN_SFT)  |                           /* aifsn=7 */
          (4 << TXQ0_MTX_Q_ECWMIN_SFT) |                            /*cwmin=4 */
          (10 << TXQ0_MTX_Q_ECWMAX_SFT)                           /* cwmax=10 */

    );


    MAC_REG_WRITE(ADR_TXQ0_MTX_Q_BKF_CNT,0x00000006);

    //soc set WMM-PARAM[1]      { aifsn=0 acm=0 cwmin=4 cwmax=10 txop=0 backoffvalue=5 }

    MAC_REG_WRITE(ADR_TXQ1_MTX_Q_AIFSN,

              (2 << TXQ1_MTX_Q_AIFSN_SFT)  |                       /* aifsn=3 */
              (4 << TXQ1_MTX_Q_ECWMIN_SFT) |                        /*cwmin=4 */
              (10 << TXQ1_MTX_Q_ECWMAX_SFT)                      /* cwmax=10 */
    );


    //soc set WMM-PARAM[2]      { aifsn=0 acm=0 cwmin=3 cwmax=4 txop=94 backoffvalue=4 }
    {

        MAC_REG_WRITE( ADR_TXQ2_MTX_Q_AIFSN,

              (1 << TXQ2_MTX_Q_AIFSN_SFT)  |                       /* aifsn=2 */
              (3 << TXQ2_MTX_Q_ECWMIN_SFT) |                        /*cwmin=3 */
              (4 << TXQ2_MTX_Q_ECWMAX_SFT) |                       /* cwmax=4 */
              (94 << TXQ2_MTX_Q_TXOP_LIMIT_SFT)                        /*  txop=94 */
        );

    }




    //soc set WMM-PARAM[3]      { aifsn=0 acm=0 cwmin=2 cwmax=3 txop=47 backoffvalue=3 }
    {
        //info 845: The right argument to operator '|' is certain to be 0

        MAC_REG_WRITE( ADR_TXQ3_MTX_Q_AIFSN,

              (1 << TXQ3_MTX_Q_AIFSN_SFT)  |                   /* aifsn=2 */
              (2 << TXQ3_MTX_Q_ECWMIN_SFT) |                    /*cwmin=2 */
              (3 << TXQ3_MTX_Q_ECWMAX_SFT) |                  /* cwmax=3 */
              (47 << TXQ3_MTX_Q_TXOP_LIMIT_SFT)                     /*  txop=47 */
        );

    }
    /* Set wmm parameter to EDCA Q4
        (use to send mgmt frame/null data frame in STA mode and broadcast frame in AP mode) */
        //MAC_REG_WRITE(ADR_TXQ4_MTX_Q_AIFSN, 0xffff2101);
        /*lint -save -e648 Overflow in computing constant for operation:    'shift left'*/
        MAC_REG_WRITE( ADR_TXQ3_MTX_Q_AIFSN,

          (1 << TXQ4_MTX_Q_AIFSN_SFT)  |                   /* aifsn=2 */
          (1 << TXQ4_MTX_Q_ECWMIN_SFT) |                    /*cwmin=1 */
          (2 << TXQ4_MTX_Q_ECWMAX_SFT) |                  /* cwmax=2 */
          (65535 << TXQ4_MTX_Q_TXOP_LIMIT_SFT)                     /*  txop=65535 */
    );


    /* By default, we apply staion decion table. */
    mac_deci_tbl= sta_deci_tbl;

    for(i=0; i<MAC_DECITBL1_SIZE; i++) {

        MAC_REG_WRITE((ADR_MRX_FLT_TB0+(u32)i*4),(u32 )(mac_deci_tbl[i]));

    }
    for(i=0; i<MAC_DECITBL2_SIZE; i++) {

        MAC_REG_WRITE(ADR_MRX_FLT_EN0+(u32)i*4,

        mac_deci_tbl[i+MAC_DECITBL1_SIZE]);

    }
     #ifdef CONFIG_SSV_CABRIO_E
        /* Do RF-IQ cali. */
        #if(DO_IQ_CALIBRATION==1)
        ssv6xxx_do_iq_calib(&init_iqk_cfg);
        #endif
    #endif // CONFIG_SSV_CABRIO_E

    // cal 6
    {
        const unsigned char cmd_data[] = {0x06, 0x00, 0x00, 0x00};
        _ssv6xxx_wifi_ioctl_Ext(SSV6XXX_HOST_CMD_CAL, (void *)cmd_data, 4, TRUE, FALSE);
    }


    return ret;
}
int cabrio_init_ap_mac()
{
    int i, ret=0;
    u32 temp;
    u16 *mac_deci_tbl;

    MAC_REG_WRITE(ADR_GLBLE_SET,

          (1 << OP_MODE_SFT)  |                           /* AP mode by default */
          //(0 << SNIFFER_MODE_SFT) |                           /* disable sniffer mode */
          (1 << DUP_FLT_SFT) |                           /* Enable duplicate detection */
          (3 << TX_PKT_RSVD_SFT) |                           /* PKT Reserve */
          (80 << PB_OFFSET_SFT)                          /* set rx packet buffer offset */
    );

    MAC_REG_READ(ADR_SCRT_SET,temp);
    temp = temp & SCRT_RPLY_IGNORE_I_MSK;
    temp |= (1 << SCRT_RPLY_IGNORE_SFT);
    MAC_REG_WRITE(ADR_SCRT_SET, temp);

    MAC_REG_WRITE(ADR_BSSID_0,   *((u32 *)&gDeviceInfo->self_mac[0]));//*((u32 *)&priv->bssid[0]));
    MAC_REG_WRITE(ADR_BSSID_1,   *((u32 *)&gDeviceInfo->self_mac[4]));//*((u32 *)&priv->bssid[4]));
    /**
        * Set reason trap to discard frames.
        */

    //soc set TX-FLOW-DATA        { M_ENG_HWHCI M_ENG_MIC M_ENG_ENCRYPT M_CPU_EDCATX M_ENG_TX_EDCA0 M_ENG_CPU M_ENG_CPU M_ENG_CPU }
    //FMAC_REG_WRITE(priv, ADR_TX_FLOW_1,  M_ENG_HWHCI|(M_ENG_MIC<<4)|(M_ENG_ENCRYPT<<8)|(M_CPU_EDCATX<<12)|
    //                                    (M_ENG_TX_EDCA0<<16)|(M_ENG_CPU<<20)|(M_ENG_CPU<<24)|(M_ENG_CPU<<28));
    //soc set TX-FLOW-MGMT        { M_ENG_HWHCI M_ENG_ENCRYPT M_ENG_TX_EDCA0 M_ENG_CPU }

    {
    const unsigned char cmd_data[] = {
        0x01, 0x06, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00};
    _ssv6xxx_wifi_ioctl_Ext(SSV6XXX_HOST_CMD_SET_FCMD_TXMGMT, (void *)cmd_data, 8, TRUE, FALSE);


    }
    //soc set TX-FLOW-CTRL        { M_ENG_HWHCI M_ENG_TX_EDCA0 M_ENG_CPU M_ENG_CPU }
    {
    const unsigned char cmd_data[] = {
        0x01, 0x03, 0x06, 0x00, 0x40, 0x00, 0x00, 0x00};
    _ssv6xxx_wifi_ioctl_Ext(SSV6XXX_HOST_CMD_SET_FCMD_TXCTRL, (void *)cmd_data, 8, TRUE, FALSE);

    }
    //info 845: The right argument to operator '|' is certain to be 0

    //MAC_REG_WRITE(ADR_TX_FLOW_0,  M_ENG_HWHCI|(M_ENG_TX_EDCA0<<4)|(M_ENG_CPU<<8)|(M_ENG_CPU<<12)|
    //                                (M_ENG_HWHCI<<16)|(M_ENG_TX_EDCA0<<20)|(M_ENG_CPU<<24)|(M_ENG_CPU<<28));

    //soc set TX-FLOW-DATA        { M_ENG_HWHCI M_ENG_MIC M_ENG_FRAG M_ENG_ENCRYPT M_CPU_EDCATX M_ENG_TX_EDCA0 M_ENG_CPU M_ENG_CPU }
    {
    const unsigned char cmd_data[] = {
        0x51, 0x03, 0x06, 0x00, 0x00, 0x40, 0x00, 0x00};
    _ssv6xxx_wifi_ioctl_Ext(SSV6XXX_HOST_CMD_SET_FCMD_TXDATA, (void *)cmd_data, 8, TRUE, FALSE);


    }

    //soc set RX-FLOW-DATA        { M_ENG_MACRX M_ENG_ENCRYPT M_CPU_DEFRAG M_ENG_MIC M_ENG_HWHCI M_ENG_CPU M_ENG_CPU M_ENG_CPU }
    {
    const unsigned char cmd_data[] = {
        0xB4, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00};
    _ssv6xxx_wifi_ioctl_Ext(SSV6XXX_HOST_CMD_SET_FCMD_RXDATA, (void *)cmd_data, 8, TRUE, FALSE);

    }
     //soc set RX-FLOW-MGMT        { M_ENG_MACRX M_ENG_ENCRYPT M_ENG_HWHCI M_ENG_CPU}
    {
    const unsigned char cmd_data[] = {
        0x34, 0x01, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00};
    _ssv6xxx_wifi_ioctl_Ext(SSV6XXX_HOST_CMD_SET_FCMD_RXMGMT, (void *)cmd_data, 8, TRUE, FALSE);


    }
     //soc set RX-FLOW-CTRL        { M_ENG_MACRX M_CPU_RXCTRL M_ENG_CPU M_ENG_CPU }
    {
    const unsigned char cmd_data[] = {
        0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    _ssv6xxx_wifi_ioctl_Ext(SSV6XXX_HOST_CMD_SET_FCMD_RXCTRL, (void *)cmd_data, 8, TRUE, FALSE);

    }

     //soc set REASON-TRAP         { 0x7fbf7f8f 0xffffffff }
    {

    MAC_REG_WRITE(ADR_REASON_TRAP0,0x7FBF7F8F);
    MAC_REG_WRITE(ADR_REASON_TRAP1,0xFFFFFFFF);

    }

    //#set EDCA parameter AP-a/g BK[0], BE[1], VI[2], VO[3]
    //soc set WMM-PARAM[0]      { aifsn=0 acm=0 cwmin=5 cwmax=10 txop=0 backoffvalue=6 }


    MAC_REG_WRITE(ADR_TXQ0_MTX_Q_AIFSN,

          (6 << TXQ0_MTX_Q_AIFSN_SFT)  |                           /* aifsn=7 */
          (4 << TXQ0_MTX_Q_ECWMIN_SFT) |                            /*cwmin=4 */
          (10 << TXQ0_MTX_Q_ECWMAX_SFT)                           /* cwmax=10 */
         //| (0 << TXQ0_MTX_Q_TXOP_LIMIT_SFT )                                      /*  txop=0 */

    );


    MAC_REG_WRITE(ADR_TXQ0_MTX_Q_BKF_CNT,0x00000006);

    //soc set WMM-PARAM[1]      { aifsn=0 acm=0 cwmin=4 cwmax=10 txop=0 backoffvalue=5 }

    MAC_REG_WRITE(ADR_TXQ1_MTX_Q_AIFSN,

              (2 << TXQ1_MTX_Q_AIFSN_SFT)  |                       /* aifsn=3 */
              (4 << TXQ1_MTX_Q_ECWMIN_SFT) |                        /*cwmin=4 */
              (6 << TXQ1_MTX_Q_ECWMAX_SFT)                      /* cwmax=6 */
              //|(0 << TXQ1_MTX_Q_TXOP_LIMIT_SFT)                         /*  txop=0 */
    );

    //MAC_REG_WRITE(ADR_TXQ1_MTX_Q_BKF_CNT,0x00000005);




    //soc set WMM-PARAM[2]      { aifsn=0 acm=0 cwmin=3 cwmax=4 txop=94 backoffvalue=4 }
    {

        MAC_REG_WRITE( ADR_TXQ2_MTX_Q_AIFSN,

              (0 << TXQ2_MTX_Q_AIFSN_SFT)  |                       /* aifsn=1 */
              (3 << TXQ2_MTX_Q_ECWMIN_SFT) |                        /*cwmin=3 */
              (4 << TXQ2_MTX_Q_ECWMAX_SFT) |                       /* cwmax=4 */
              (94 << TXQ2_MTX_Q_TXOP_LIMIT_SFT)                        /*  txop=94 */
        );


        //MAC_REG_WRITE( ADR_TXQ2_MTX_Q_BKF_CNT,0x00000004);


    }




    //soc set WMM-PARAM[3]      { aifsn=0 acm=0 cwmin=2 cwmax=3 txop=47 backoffvalue=3 }
    {
        //info 845: The right argument to operator '|' is certain to be 0

        MAC_REG_WRITE( ADR_TXQ3_MTX_Q_AIFSN,

              (0 << TXQ3_MTX_Q_AIFSN_SFT)  |                   /* aifsn=1 */
              (2 << TXQ3_MTX_Q_ECWMIN_SFT) |                    /*cwmin=2 */
              (3 << TXQ3_MTX_Q_ECWMAX_SFT) |                  /* cwmax=3 */
              (47 << TXQ3_MTX_Q_TXOP_LIMIT_SFT)                     /*  txop=47 */
        );


        //MAC_REG_WRITE( ADR_TXQ3_MTX_Q_BKF_CNT,0x00000003);

    }
    /* Set wmm parameter to EDCA Q4
        (use to send mgmt frame/null data frame in STA mode and broadcast frame in AP mode) */
        //MAC_REG_WRITE(ADR_TXQ4_MTX_Q_AIFSN, 0xffff2101);
        /*lint -save -e648 Overflow in computing constant for operation:    'shift left'*/
        MAC_REG_WRITE( ADR_TXQ3_MTX_Q_AIFSN,

          (1 << TXQ4_MTX_Q_AIFSN_SFT)  |                   /* aifsn=2 */
          (1 << TXQ4_MTX_Q_ECWMIN_SFT) |                    /*cwmin=1 */
          (2 << TXQ4_MTX_Q_ECWMAX_SFT) |                  /* cwmax=2 */
          (65535 << TXQ4_MTX_Q_TXOP_LIMIT_SFT)                     /*  txop=65535 */
    );
    /*lint -restore*/





    /* By default, we apply ap decion table. */

    mac_deci_tbl = ap_deci_tbl;

    for(i=0; i<MAC_DECITBL1_SIZE; i++) {

        MAC_REG_WRITE( ADR_MRX_FLT_TB0+(u32)i*4,
        mac_deci_tbl[i]);
    }
    for(i=0; i<MAC_DECITBL2_SIZE; i++) {
        MAC_REG_WRITE( ADR_MRX_FLT_EN0+(u32)i*4,

        mac_deci_tbl[i+MAC_DECITBL1_SIZE]);

    }
    //trap null data
    //SET_RX_NULL_TRAP_EN(1)
    //MAC_REG_SET_BITS(ADR_CONTROL,1 << RX_NULL_TRAP_EN_SFT,RX_NULL_TRAP_EN_MSK);
    MAC_REG_READ(ADR_CONTROL,temp);
    temp = temp & RX_NULL_TRAP_EN_I_MSK;
    temp |= (1 << RX_NULL_TRAP_EN_SFT);
    MAC_REG_WRITE(ADR_CONTROL, temp);


    #ifdef CONFIG_SSV_CABRIO_E
        /* Do RF-IQ cali. */
        #if(DO_IQ_CALIBRATION==1)
        ssv6xxx_do_iq_calib(&init_iqk_cfg);
        #endif
    #endif // CONFIG_SSV_CABRIO_E

    // cal 6
    {
        unsigned char cmd_data[] = {
            0x00, 0x00, 0x00, 0x00};
        cmd_data[0]= gDeviceInfo->APInfo->nCurrentChannel;
        _ssv6xxx_wifi_ioctl_Ext(SSV6XXX_HOST_CMD_CAL, cmd_data, 4, TRUE, FALSE);
    }


    return ret;
}

void ssv6xxx_rf_enable()
{

    u32 _regval;
    MAC_REG_READ(0xce010000, _regval);
    _regval &= ~(0x03<<12);
    _regval |= (0x02<<12);
    MAC_REG_WRITE(0xce010000, _regval);
    return;

}

void ssv6xxx_rf_disable()
{

    u32 _regval;
    MAC_REG_READ(0xce010000, _regval);
    _regval &= ~(0x03<<12);
    _regval |= (0x01<<12);
    MAC_REG_WRITE(0xce010000, _regval);
    return;


}

void ssv6xxx_accept_none_wsid_frame(void)
{
    u32 _regval;
    MAC_REG_READ(ADR_MRX_FLT_EN3, _regval);
    _regval=_regval & 0x0000ffff;
    _regval|=0x0800;
    MAC_REG_WRITE(ADR_MRX_FLT_EN3,_regval);
    return;
}
void ssv6xxx_drop_none_wsid_frame(void)
{
    u32 _regval;
    MAC_REG_READ(ADR_MRX_FLT_EN3, _regval);
    _regval=_regval & 0x0000ffff;
    _regval&=~0x0800;
    MAC_REG_WRITE(ADR_MRX_FLT_EN3,_regval);
    return;
}
void ssv6xxx_HW_disable()
{

    ssv6xxx_rf_disable();

    //Disable MCU
    MAC_REG_WRITE(ADR_BRG_SW_RST, 0x0);
    MAC_REG_WRITE(ADR_BRG_SW_RST, 1 << 1);  /* bug if reset ?? */

    return;


}
void ssv6xxx_HW_enable(void)
{
    ssv6xxx_rf_enable();

    //Enable MCU, it's duplicate, load fw has already set this bit
    MAC_REG_WRITE(ADR_BRG_SW_RST, 0x01);

    return;


}


