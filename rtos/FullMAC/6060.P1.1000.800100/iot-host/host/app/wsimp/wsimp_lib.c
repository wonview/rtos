#include <config.h>
#include <ssv_lib.h>
#include <cmd_def.h>
#include <host_global.h>
#include "wsimp_config.h"
#include "wsimp_lib.h"

#include <drv_mac.h>
#include <log.h>



#define MAX_PKT_SIZE            8192*2
#define MAX_CMD_ARGS            100
#define MAX_PAT_INFO            32




struct wsimp_pat_info {
    void            *pkt_info;
    u32             pkt_id;
    u32             is_valid;
};


struct wsimp_pat_info reg_pat_info[MAX_PAT_INFO];
u8 g_pkt_process_flow[512] = { 0x00 };       



extern H_APIs s32 ssv6xxx_wifi_ioctl(u32 cmd_id, void *data, u32 len);

static s32 wsimp_snd_cmd(char *dat, s32 len)
{
    HDR_HostCmd *HostCmd;
    
    /**
     * argument - dat already includes HostCmd Header.
     * We need to remove HostCmd header here before calling
     * ssv6xxx_wifi_ioctl().
     */
    HostCmd = (HDR_HostCmd *)dat;
    _ssv6xxx_wifi_ioctl((u32)HostCmd->h_cmd, HostCmd->un.dat8, 
                   HostCmd->len-HOST_CMD_HDR_LEN,TRUE);
    return 0;
}



s32 wsimp_lib_init(void)
{
    ssv6xxx_memset((void *)reg_pat_info, 0, 
        sizeof(struct wsimp_pat_info));
    return 0;
}


s32 wsimp_mac_str2hex(char *str, ETHER_ADDR *mac)
{
    u32 m[ETHER_ADDR_LEN];
    if (sscanf(str, ETH_ADDR_FORMAT, m, m+1, m+2, m+3, m+4, m+5)!= 6)
        return 0;
    mac->addr[0] = (u8)m[0];
    mac->addr[1] = (u8)m[1];
    mac->addr[2] = (u8)m[2];
    mac->addr[3] = (u8)m[3];
    mac->addr[4] = (u8)m[4];
    mac->addr[5] = (u8)m[5];
    return 6;
}




FILE *wsimp_fopen(char *file, char *mode)
{
    FILE *fs;

    if (strcmp(file, "stdin")==0 || strcmp(file, "STDIN")==0)
    {
        fs = stdin;
        return fs;
    }
    if (strcmp(file, "stdout")==0 || strcmp(file, "STDOUT")==0)
    {
        fs = stdout;
        return fs;
    }
    if (strcmp(file, "stderr")==0 || strcmp(file, "stderr")==0)
    {
        fs = stderr;
        return fs;
    }
    fs = fopen(file, mode);
    return fs;

}



s32 wsimp_tokenlize(char *buff, char *argv[])
{
    s32 argc;
    char ch, *pch;

    for( argc=0,ch=0, pch=buff; (*pch!=0x00)&&(argc<MAX_CMD_ARGS); pch++ )
    {
        if (*pch==0x0a || *pch==0x0d || *pch=='\t')
            *pch = ' ';
        if ( (ch==0) && (*pch!=' ') )
        {
            ch = 1;
            argv[argc] = pch;
        }

        if ( (ch==1) && (*pch==' ') )
        {
            *pch = 0x00;
            ch = 0;
            argc ++;
        }
    }
    if ( ch == 1)
        argc ++;
    else if ( argc > 0 )
        *(pch-1) = 0x00;

    return argc;
}





//---------------------------------------------


// 				wf_info->frame_time = ssv6xxx_atoi(argv[1]);
// 				break;



//eid(u8)/length(u8)/content

s32 wsimp_generic_extrainfo(struct wsimp_file_info *wf_info, void *args, s32 argc, char *argv[])
{
	struct tx_extra_info_tbl *extra_info=(struct tx_extra_info_tbl *)args;
	u16 min = extra_info->data;
	u16 max = extra_info->data1;

	u8 *u8ptr;
	u16 *u16ptr;
//	u32 *u32ptr;

	//check if extra info space is enouggh	
	if(wf_info->extra_len>=WSIMP_EXTRA_BUF_SIZE)	
		ASSERT(FALSE)


	//Fill eid, length
	wf_info->extra_buf[wf_info->extra_len++] = extra_info->id;
	wf_info->extra_buf[wf_info->extra_len++] = extra_info->len;


	//Fill data
	if(extra_info->len == 1){

		u8ptr = &wf_info->extra_buf[wf_info->extra_len];
		*u8ptr = ssv6xxx_atoi(argv[1]);
		wf_info->extra_len++;

	}
	else if(extra_info->len == 2){

		u16ptr = (u16*)&wf_info->extra_buf[wf_info->extra_len];
		*u16ptr = ssv6xxx_atoi(argv[1]);

		wf_info->extra_len+=2;
	}
	else
	{ASSERT(FALSE);}


	return 0;
}












//-----------------------------------------------
#undef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


static const struct tx_extra_info_tbl extrainfo[]=
{
	{"frame_time:"			, 	SSV6XXX_TX_FRAME_TIME			,2, wsimp_generic_extrainfo,  0, 511},
	{"rts_cts_nav:"			,	SSV6XXX_TX_RTS_CTS_NAV			,2,	wsimp_generic_extrainfo,  0, 65535},
	{"do_rts_cts:"			, 	SSV6XXX_TX_DO_RTS_CTS			,1, wsimp_generic_extrainfo,  0, 3},
	{"ack_policy:"			, 	SSV6XXX_TX_ACK_POLICY			,1,	wsimp_generic_extrainfo,  0, 3},
	{"tx_brust:"  			, 	SSV6XXX_TX_TX_BRUST				,1,	wsimp_generic_extrainfo,  0, 1},
	{"tx_report:"  			, 	SSV6XXX_TX_TX_REPORT 			,1, wsimp_generic_extrainfo,  0, 1},
	{"crate_idx:"			, 	SSV6XXX_TX_CRATE_IDX			,1,	wsimp_generic_extrainfo,  0, 63},
	{"drate_idx:"			, 	SSV6XXX_TX_DRATE_IDX			,1,	wsimp_generic_extrainfo,  0, 63},
	{"dl_length:"			, 	SSV6XXX_TX_DL_LENGTH_IDX		,2,	wsimp_generic_extrainfo,  0, 4095},
	

};















#if 0
s32 wsimp_read_pattern(struct wsimp_file_info *wf_info)
{
    char *argv[128], temp[36], *pos;
    char buffer[1024], buffer1[4096];
    s32 argc, i, state=0;
	s32 rd_count=0;
	u32 size = sizeof(struct wsimp_file_info)-OFFSETOF(struct wsimp_file_info,mem_ptr);

    if (wf_info->mem_ptr != NULL)
        return -1;
    memset((void *)&wf_info->mem_ptr, 0, size);
    
    wf_info->is_valid = 0;
    wf_info->extra_info = 0xF0000000; /* init value to indicate no assign value */
    wf_info->priority = 99;//5; /* by default set to 5 */
    wf_info->with_qos = 0;
    wf_info->with_ht = 0;
    wf_info->with_wds = 0;
    
    while(1)
    {
read_next_line:	
        buffer1[0] = 0x00;
        r_again:
        WIFISIM_READ_LINE(buffer, wf_info);
        i = strlen(buffer)-1;
        if (buffer[i] == '\\') {
            buffer[i] = ' ';
            buffer[i+1] = 0x00;
            strcat(buffer1, buffer);
            goto r_again;
        } 
        else if (buffer[i]==0x0a && buffer[i-1]=='\\') {
            buffer[i-1] = ' ';
            buffer[i] = 0x00;
            strcat(buffer1, buffer);
            goto r_again;
        }
        strcat(buffer1, buffer);        
        argc = wsimp_tokenlize(buffer1, argv);
        if (argc>0 && (argv[0][0]=='#' || argv[0][0]==';'))
            continue;
        if (argc <= 0)
            continue;

again:
        switch(state)
        {
        case 0:
            if (argc==2 && strcmp(argv[0], "PKT_ID:")==0 ) {

                wf_info->pkt_id = ssv6xxx_atoi((char *)argv[1]);
                state = 1;
            }
            else
            {
				if (wsimp_cfg_regs(wsimp_snd_cmd, argc, argv) < 0)//-------------------------------------->Config command
                	goto out;
				else
				{
					//Use while loop to make sure command has already set to reg.
					
	                /**
			                 * Note that we shall wait here till all host commands
			                 * have been processed.
			                 *
			                 * @ we check all message events are all freed !
			                 */
	                while(g_free_msgevt_cnt < MBOX_MAX_MSG_EVENT)
	                    { }
				}
            }
            break;

        case 1:
            /**
                       * Extra keywords can be parsed here.
                       */
            if (argc==2 && strcmp(argv[0], "encap:")==0) {
                if (strcmp(argv[1], "ether") == 0)
                    wf_info->is_80211 = 0;
                else if (strcmp(argv[1], "802.11") == 0)
                    wf_info->is_80211 = 1;
                else goto out;
                break;
            }
            else if (argc==2 && strcmp(argv[0], "priority:")==0) {
                wf_info->priority = ssv6xxx_atoi((char *)argv[1]);
                wf_info->with_qos = 1;
                wf_info->qos = wf_info->priority&0x0F;
                break;
            }
            else if (argc==2 && strcmp(argv[0], "extra-info:") == 0) {
                wf_info->extra_info = ssv6xxx_atoi(argv[1]);
                break;
            }
            else if (argc==2 && strcmp(argv[0], "qos:") == 0) {
                wf_info->with_qos = 1;
                wf_info->qos = ssv6xxx_atoi((char *)argv[1]);
                break;
            }
            else if (argc==2 && strcmp(argv[0], "ht:") == 0) {
                wf_info->with_ht = 1;
                wf_info->ht = ssv6xxx_atoi(argv[1]);
                break;
            }
            else if (argc==2 && strcmp(argv[0], "wds:") == 0) {
                wf_info->with_wds = 1;
                wsimp_mac_str2hex(argv[1], (ETHER_ADDR *)wf_info->addr4);
                break;
            }
			else if (argc==2 && strcmp(argv[0], "bc_queue:") == 0) {
                wf_info->bc_queue = ssv6xxx_atoi(argv[1]);
                break;
            }
			else
			{
				const struct tx_extra_info_tbl *infotbl;
    			for(i=0, infotbl=extrainfo; i<ARRAY_SIZE(extrainfo); i++, infotbl++)  {
					if (strcmp(argv[0], infotbl->extra_info_str) == 0) {							
				        if(infotbl->func(wf_info, (void*)infotbl, argc, argv)>=0)
							goto read_next_line;
						else
							goto out;				    
					}					
				}
			}



            state = 2;
            goto again;
                        
        case 2:
            if (argc==2 && strcmp(argv[0], "length:")==0) {
                
				wf_info->length = ssv6xxx_atoi(argv[1]);
                
				//Add extra info data size+sizeof u16(total length)
				wf_info->mem_ptr= MALLOC(wf_info->length + 4 + 
                drv_mac_get_pbuf_offset()+ wf_info->extra_len+WSIMP_EXTRA_TOTAL_LEN_SIZE);
                
				assert(wf_info != NULL);
                memset(wf_info->mem_ptr, 0, drv_mac_get_pbuf_offset());
                pos = (char *)wf_info->mem_ptr;
                pos += drv_mac_get_pbuf_offset(); /* for PktInfo space */
                wf_info->pkt_info = pos;


                state = 3;
                break;
            }
            goto out;

        case 3:
            if (argc <= 0)
                continue;
            if (rd_count < wf_info->length) {
                for(i=0; i<argc; i++)
                {
                    strcpy(temp, "0x");
                    strcat(temp, argv[i]);
                    *pos++ = ssv6xxx_atoi(temp)&0xFF;
                }
                rd_count += argc;
                if (rd_count >= wf_info->length) { 
                    
					wf_info->is_valid = 1;
                    					
					if (!wf_info->extra_len)
						return 0;

					//cause parent function will remove 4 bytes for fcs.							
					//we should shift firstly.
					pos-=4;
					memcpy(pos, wf_info->extra_buf, wf_info->extra_len);					
					pos+=wf_info->extra_len;

					*(u16*)pos=wf_info->extra_len;

					//Fill extra data total length in the tail
					wf_info->length+=wf_info->extra_len+WSIMP_EXTRA_TOTAL_LEN_SIZE;					
					return 0;


                }
                break;
            }



			
            goto out;
            
        default: assert(0);

        }
    }

out:
    if (state >= 3)
        FREE(wf_info->mem_ptr);
    wf_info->mem_ptr  = NULL;
    wf_info->pkt_info = NULL;
    return -1;
}



/**
 * s32 wsimp_read_pattern_f1() - read wifi-sim output to a packet buffer
 *
 */
s32 wsimp_read_pattern_f1(struct wsimp_file_info *wf_info)
{
    u8 *argv[100], temp[20], *pos;
    u8 buffer[1024];
    s32 argc, rd_count=0, i, state=0;


    if (wf_info->mem_ptr != NULL)
        return -1;
    
    wf_info->is_valid = 0;
    
    while(1)
    {
        WIFISIM_READ_LINE(buffer, wf_info);
        argc = wsimp_tokenlize(buffer, argv);
     
        if (argc<=0 || (state!=3 && strcmp(argv[0], ">>"))) {
            if (rd_count>0 && wf_info->is_valid==0) {
                return -1;
            }
            continue;
        }
    
        switch (state)
        {
        case 0:
            if ((argc==3||argc==4) && !strcmp(argv[0], ">>") && !strcmp(argv[1], "Packet:")) {
                wf_info->pkt_id = ssv6xxx_atoi(argv[2]);
                wf_info->frag_no = 0;
                if (argc == 4) {  /* fragment case */
                    i = sscanf(argv[3], "(%d)", &(wf_info->frag_no));
                    if (i != 1)
                        return -1;
                } 
                state = 1;
            }
            else goto out;
            break;

        case 1:
            if (argc>=3 && !strcmp(argv[0], ">>") && !strcmp(argv[1], "Flow:")) {
                wf_info->Flows[0] = 0x00;
                for(i=2; i<argc; i++) {
                    strcat(wf_info->Flows, argv[i]);
                    strcat(wf_info->Flows, " ");
                }                    
                state = 2;
            }
            else goto out;
            break;
            
        case 2: 
            if (argc==3 && !strcmp(argv[0], ">>") && !strcmp(argv[1], "Length:")) {
                wf_info->length = ssv6xxx_atoi(argv[2]);
                wf_info->mem_ptr= MALLOC(wf_info->length + 128);
                assert(wf_info != NULL);
                memset(wf_info->mem_ptr, 0, wf_info->length + 128);
                pos = wf_info->mem_ptr;
                wf_info->pkt_info = pos;
                rd_count = 0;
                state = 3;
                break;
            }
            goto out;            

        case 3:
            if (argc <= 0)
                continue;
            if (rd_count < wf_info->length) {
                for(i=1; i<argc; i++)
                {
                    strcpy(temp, "0x");
                    strcat(temp, argv[i]);
                    *pos++ = ssv6xxx_atoi(temp)&0xFF;
                }
                rd_count += (argc-1);
                if (rd_count >= wf_info->length) { 
                    wf_info->is_valid = 1;
                    return 0;
                }
                break;
            }
            goto out;            
            
        default: assert(0);
        
        }
    }


out:
    if (state >= 3)
        FREE(wf_info->mem_ptr);
    wf_info->mem_ptr  = NULL;
    wf_info->pkt_info = NULL;
    return -1;

}

#endif


s32 wsimp_register_pattern(void *pkt_info, s32 pkt_id)
{
    s32 i;
    for(i=0; i<MAX_PAT_INFO; i++)
    {
        if (reg_pat_info[i].is_valid != 0)
            continue;
        reg_pat_info[i].is_valid = 1;
        reg_pat_info[i].pkt_id = pkt_id;
        reg_pat_info[i].pkt_info = pkt_info;
        return 0;
    }
    return -1;
}


s32 wsimp_unregister_pattern(void *pkt_info)
{
    s32 i;
    for(i=0; i<MAX_PAT_INFO; i++)
    {
        if (reg_pat_info[i].is_valid == 0)
            continue;
        if (reg_pat_info[i].pkt_info!=pkt_info)
            continue;
        reg_pat_info[i].is_valid = 0;
        return 0;
    }
    return -1;
}


s32 wsimp_get_pkt_id(void *pkt_info)
{
    s32 i;
    for(i=0; i<MAX_PAT_INFO; i++)
    {
        if (reg_pat_info[i].is_valid == 0)
            continue;
        if (reg_pat_info[i].pkt_info!=pkt_info)
            continue;
        return reg_pat_info[i].pkt_id;
    }
    return -1;
}





static u32 reverse(register u32 x)
{
    x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
    x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
    x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
    x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
    return((x >> 16) | (x << 16));
}

u32 wsimp_crc32(char *message, s32 len) 
{  
    s32 i, j;  
    u32 byte, crc;
    i = 0;  
    crc = 0xFFFFFFFF;  
    while (len) {  
        byte = message[i];            // Get next byte.  
        byte = reverse(byte);         // 32-bit reversal.  
        for (j = 0; j <= 7; j++) {    // Do eight times.  
            if ((int)(crc ^ byte) < 0)  
                crc = (crc << 1) ^ 0x04C11DB7;  
            else crc = crc << 1;  
                byte = byte << 1;          // Ready next msg bit.  
        }
        i = i + 1;
        len--;
    }  
    return reverse(~crc);
}




















