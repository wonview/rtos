#ifndef _WSIMP_LIB_H_
#define _WSIMP_LIB_H_

#include <cmd_def.h>

#define SIM_EOF                                   2

#define WIFISIM_READ_LINE(_buf_, _info_)                \
{                                                       \
    if (NULL == fgets((_buf_), 1024, (_info_)->fp))    \
        return SIM_EOF;                                 \
    (_info_)->line ++;                                  \
}



/**
 *  u32 g_wsimp_flags - The description of run-sim information.
 *
 *  @ WSIMP_ENABLE:
 *  @ WSIMP_RUN_TX:
 *  @ WSIMP_RUN_RAW:
 *  @ WSIMP_RUN_DETAIL:
 *
 */
#define WSIMP_ENABLE                    0x1
#define WSIMP_RUN_TX                    0x2
#define WSIMP_RUN_RAW                   0x4
#define WSIMP_RUN_DETAIL                0x8
#define WSIMP_RUN_AMPDU                	0x10

#define WSIMP_RUN_LOOP                	0x20



#define WSIMP_EXTRA_BUF_SIZE			1024
#define WSIMP_EXTRA_TOTAL_LEN_SIZE		SSV_EXTRA_TOTAL_LEN_SIZE

struct tx_extra_info
{
	u8              id;
	u8 				len;	//ssv6xxx_tx_extra_type	
    union { /*lint -save -e157 */
    u32             dummy; // Put a u32 dummy to make MSVC and GCC treat tx_extra_info as the same size.
    u8              dat8[0];
    u16             dat16[0];
    u32             dat32[0];
    }; /*lint -restore */
};







struct wsimp_file_info {
    FILE            *fp;
    void            *mem_ptr;
    void            *pkt_info;
    u32             line;
    u32             pkt_id;
    u32             frag_no;    
    s32             length;

    u32             is_valid:1;
    u32             is_80211:1;
	u32				is_ht:1;
	u32			    is_wds:1;
    u32             extra_info;

    char            EngName[64];
    char            Flows[1024];

    u8              priority;
    u8              addr4[6];
    u16             qos;
    u32             ht;
    u32             with_qos;
    u32             with_ht;
    u32             with_wds;


	u16 extra_len;
	u8 extra_buf[WSIMP_EXTRA_BUF_SIZE];


	bool bc_queue;

	
};


struct tx_extra_info_tbl
{
	const u8              *extra_info_str;
	u8 				id;	//ssv6xxx_tx_extra_type
	u8			len;//bytes
	s32             (*func)(struct wsimp_file_info *wf_info, void *args, s32 argc, char *argv[]);	
	u16				data;
	u16				data1;
};





extern u32 g_wsimp_flags;




FILE *wsimp_fopen(char *file, char *mode);
u32 wsimp_crc32(char *message, s32 len);
s32 wsimp_tokenlize(char *buff, char *argv[]);
s32 wsimp_read_pattern(struct wsimp_file_info *wf_info);
s32 wsimp_read_pattern_f1(struct wsimp_file_info *wf_info);
s32 wsimp_register_pattern(void *pkt_info, s32 pkt_id);
s32 wsimp_unregister_pattern(void *pkt_info);
s32 wsimp_get_pkt_id(void *pkt_info);
s32 wsimp_mac_str2hex(char *str, ETHER_ADDR *mac);







#endif /* _WSIMP_LIB_H_ */

