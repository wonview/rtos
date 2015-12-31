/*
 * Copyright (c) 2014 South Silicon Valley Microelectronics Inc.
 * Copyright (c) 2015 iComm Semiconductor Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _SSV_EFUSE_H_
#define _SSV_EFUSE_H_

#define EFUSE_MAX_SECTION_MAP (EFUSE_HWSET_MAX_SIZE>>5)
#define SSV_EFUSE_POWER_SWITCH_BASE	0xc0000328
#if(CONFIG_EFUSE_DEFAULT_BASE==1)
#define EFUSE_HWSET_MAX_SIZE (256)
#define SSV_EFUSE_READ_SWITCH	0xc2000128
#define SSV_EFUSE_RAW_DATA_BASE	0xc200014c
#else	
#define EFUSE_HWSET_MAX_SIZE (256-32)	//224bit 
#define SSV_EFUSE_READ_SWITCH	0xc200012c
#define SSV_EFUSE_RAW_DATA_BASE	0xc2000150
#endif

enum efuse_data_item {
    EFUSE_R_CALIBRATION_RESULT = 1,
    EFUSE_SAR_RESULT,
    EFUSE_MAC,
    EFUSE_CRYSTAL_FREQUENCY_OFFSET,
    EFUSE_IQ_CALIBRATION_RESULT,	//orig is marked
    EFUSE_TX_POWER_INDEX_1,
    EFUSE_TX_POWER_INDEX_2,
    EFUSE_CHIP_IDENTITY
};

struct ssv6xxx_efuse_cfg {
    u32     r_calbration_result;
    u32     sar_result;
    u32     crystal_frequency_offset;
    //u16 iq_calbration_result;
    u32     tx_power_index_1;
    u32     tx_power_index_2;
    u32     chip_identity;
};

struct efuse_map {
    u8 offset;
    u8 byte_cnts;
    u16 value;
};

u16 parser_efuse(u8 *pbuf, u8 *mac_addr);
u8 read_efuse(u8 *pbuf);
#endif // _SSV_EFUSE_H_
