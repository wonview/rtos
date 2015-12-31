#include "rtos.h"
#include "log.h"
#include "dev.h"
#include "common.h"
#include "ssv_dev.h"
#include "efuse.h"

struct efuse_map SSV_EFUSE_ITEM_TABLE[] = {
    {4, 0, 0},
    {4, 8, 0},
    {4, 8, 0},
    {4, 48, 0},//Mac address
    {4, 8, 0},
    //{4, 8, 0},
    {4, 12, 0},//EFUSE_IQ_CALIBRAION_RESULT
    {4, 8, 0},
    {4, 8, 0},
    {4, 8, 0},
};


static void efuse_power_switch(u32 on);

static void efuse_power_switch(u32 on)
{
    u32 temp_value;
    MAC_REG_READ(SSV_EFUSE_POWER_SWITCH_BASE,temp_value);

    temp_value&=(~(0x000000ff));

    if(on)
    {
        temp_value|=0x11;
    }
    else
    {	
        temp_value|=0x0a;
    }

    MAC_REG_WRITE(SSV_EFUSE_POWER_SWITCH_BASE,temp_value);
}

u8 read_efuse(u8 *pbuf)
{
    u32 *pointer32, i;
    pointer32 = (u32 *)pbuf;

    efuse_power_switch(1);

    MAC_REG_WRITE(SSV_EFUSE_READ_SWITCH,0x1);
    MAC_REG_READ(SSV_EFUSE_RAW_DATA_BASE,*pointer32);
    if (*pointer32 == 0x00) 
    {
        return 0;
    }

    /*get 8 section value*/ 
    for (i=0; i<EFUSE_MAX_SECTION_MAP; i++, pointer32++)
    {
        MAC_REG_WRITE(SSV_EFUSE_READ_SWITCH+i*4,0x1);
        MAC_REG_READ(SSV_EFUSE_RAW_DATA_BASE+i*4,*pointer32);
    }

    efuse_power_switch(0);
    return 1;
}

u16 parser_efuse(u8 *pbuf, u8 *mac_addr)
{
    u8 *rtemp8,idx=0;
    u16 shift=0,i;
    u16 efuse_real_content_len = 0;
    u8 temp1,temp2;

    rtemp8 = pbuf;

    if (*rtemp8 == 0x00) 
    {
        return efuse_real_content_len;
    }

    do
    {
        /*get efuse index, EX: 3=mac address*/
        idx = (*(rtemp8) >> shift)&0xf;
        switch(idx)
        {
            //1 byte type
            case EFUSE_R_CALIBRATION_RESULT:
            case EFUSE_CRYSTAL_FREQUENCY_OFFSET:
            case EFUSE_TX_POWER_INDEX_1:
            case EFUSE_TX_POWER_INDEX_2:
            case EFUSE_SAR_RESULT:
            case EFUSE_CHIP_IDENTITY:
            if(shift)
            {
                rtemp8 ++;
                SSV_EFUSE_ITEM_TABLE[idx].value = (u16)((u8)(*((u16*)rtemp8)) & ((1<< SSV_EFUSE_ITEM_TABLE[idx].byte_cnts) - 1));
            }
            else
            {
                SSV_EFUSE_ITEM_TABLE[idx].value = (u16)((u8)(*((u16*)rtemp8) >> 4) & ((1<< SSV_EFUSE_ITEM_TABLE[idx].byte_cnts) - 1));
            }
            efuse_real_content_len += (SSV_EFUSE_ITEM_TABLE[idx].offset + SSV_EFUSE_ITEM_TABLE[idx].byte_cnts);
            break;

            case EFUSE_MAC:
            if(shift)
            {
                rtemp8 ++;
                MEMCPY(mac_addr,rtemp8,6);
            }
            else
            {	
                for(i=0;i<6;i++)
                {
                    temp1=(*((u8*)rtemp8)>>4)&0x0F;
                    rtemp8 ++;
                    temp2 =(*((u8*)rtemp8)<<4)&0xF0; 

                    mac_addr[i]=temp2|temp1;
                }
            }
            efuse_real_content_len += (SSV_EFUSE_ITEM_TABLE[idx].offset + SSV_EFUSE_ITEM_TABLE[idx].byte_cnts);
            break;

            default:
            idx = 0;
            break;
        }

        shift = efuse_real_content_len % 8;
        rtemp8 = &pbuf[efuse_real_content_len / 8];	

    }while(idx != 0);

    return efuse_real_content_len;
}

