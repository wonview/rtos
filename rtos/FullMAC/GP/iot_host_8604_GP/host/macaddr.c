#include "rtos.h"
#include "log.h"
#include "dev.h"
#include "ssv_dev.h"
#include <regs/ssv6200_reg.h>
#include "efuse.h"

extern struct efuse_map SSV_EFUSE_ITEM_TABLE[];

int ssv6xxx_get_cust_mac(u8 *mac)
{
    char *mac_method = "default";
#if(CONFIG_RANDOM_MAC ==1)
    u8 random_mac0[ETH_ALEN] = { 0x60, 0x11, 0x33, 0x33, 0x33, 0x33 };
    u32 rtemp;
#endif
#if(CONFIG_EFUSE_MAC ==1)
	struct ssv6xxx_efuse_cfg efuse_cfg;
    u8 efuse_mapping_table[EFUSE_HWSET_MAX_SIZE/8];
    u8 rom_mac0[ETH_ALEN];
	
    MEMSET(rom_mac0,0x00,ETH_ALEN);
    MEMSET(efuse_mapping_table,0x00,EFUSE_HWSET_MAX_SIZE/8);

    read_efuse(efuse_mapping_table);

    parser_efuse(efuse_mapping_table,rom_mac0);
	
    efuse_cfg.r_calbration_result = (u8)SSV_EFUSE_ITEM_TABLE[EFUSE_R_CALIBRATION_RESULT].value;
    efuse_cfg.sar_result = (u8)SSV_EFUSE_ITEM_TABLE[EFUSE_SAR_RESULT].value;
    efuse_cfg.crystal_frequency_offset = (u8)SSV_EFUSE_ITEM_TABLE[EFUSE_CRYSTAL_FREQUENCY_OFFSET].value;
    efuse_cfg.tx_power_index_1 = (u8)SSV_EFUSE_ITEM_TABLE[EFUSE_TX_POWER_INDEX_1].value;
    efuse_cfg.tx_power_index_2 = (u8)SSV_EFUSE_ITEM_TABLE[EFUSE_TX_POWER_INDEX_2].value;
    efuse_cfg.chip_identity    = (u8)SSV_EFUSE_ITEM_TABLE[EFUSE_CHIP_IDENTITY].value;
	
    if (is_valid_ether_addr(rom_mac0)) 
    {
        MEMCPY(mac, rom_mac0, ETH_ALEN);
        mac_method = "efuse";
        goto done;
    }
#endif//CONFIG_EFUSE_MAC ==1
#ifdef __SSV_UNIX_SIM__
    if(get_eth0_as_mac(mac)==0)
    {
        mac_method = "eth0";
        goto done;
    }
#endif//__SSV_UNIX_SIM__  
#if(CONFIG_RANDOM_MAC ==1)
    /*
    set random 4 byte mac address.
	Owing to HW design, first time to get random will be 0
	*/
	MAC_REG_WRITE(ADR_RAND_EN,1);
    MAC_REG_READ(ADR_RAND_NUM,rtemp);	
    MAC_REG_READ(ADR_RAND_NUM,rtemp);
    MEMCPY((random_mac0+2), &rtemp, 4);

    /*check the mac address is valid*/
    if (is_valid_ether_addr(random_mac0)) 
    {
        MEMCPY(mac, random_mac0, ETH_ALEN);
        mac_method = "random";
    }
    MAC_REG_WRITE(ADR_RAND_EN,0);
#endif//CONFIG_RANDOM_MAC ==1
done:

    LOG_PRINTF("[Info ] use \"%s\" to get mac address \r\n",mac_method);
    return 0;
}

