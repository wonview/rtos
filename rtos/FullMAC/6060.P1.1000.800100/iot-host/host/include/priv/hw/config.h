#ifndef _CONFIG_H_
#define _CONFIG_H_

// Common configuration should be applied to for both host simuator and device
// FW calculate checksum to check correctness of downloaded image.
#define ENABLE_FW_SELF_CHECK
#define FW_BLOCK_SIZE                   (1024)
#define FW_CHECKSUM_INIT                (0x12345678)
#define _INTERNAL_MCU_SUPPLICANT_SUPPORT_
// Use signle command response event instead of specific response event for each command.
// #define USE_CMD_RESP                    1

// Get TX queue statistics in EDCA handler.
#define ENABLE_TX_QUEUE_STATS

//#define THROUGHPUT_TEST
//HOST_SDIO_THROUGHPUT_TEST base on THROUGHPUT_TEST
//#define HOST_SDIO_THROUGHPUT_TEST

// Send log messages to host.
//#define ENABLE_LOG_TO_HOST
#define CONFIG_IO_BUS_THROUGHPUT_TEST 0

// -------------------------XTAL config -------------------------------
#define CONFIG_HOST_PLATFORM         1
#include <host_config.h>


//#define CONFIG_SSV_CABRIO_A			1
#define CONFIG_SSV_CABRIO_E			1
#define SSV6XXX_IQK_CFG_XTAL		SSV6XXX_IQK_CFG_XTAL_26M

/*-------------------------HW RF setting-------------------------------*/
//For IC with IPD: SSV6051Z/6060P, Otherwise set to 0

#if (CONFIG_CHIP_ID==SSV6051Z || CONFIG_CHIP_ID==SSV6060P)
    #define SSV_IPD 1
    //Internal LDO setting([MP4-4.2V]=0 or [ON BOARD IC-3.3V]=1)
    //If IPD=1, INTERNAL_LDO MUST to 1
    #define SSV_INTERNAL_LDO    1

    #define SSV_VOLT_REGULATOR  VOLT_LDO_REGULATOR
#else//CONFIG_SSV6051Z
    #define SSV_IPD 0
    #define SSV_INTERNAL_LDO    0

    #define SSV_VOLT_REGULATOR  VOLT_DCDC_CONVERT
#endif//CONFIG_SSV6051Z



//Voltage setting: LDO or DCDC, SSV6051Z is LDO mode.
#define VOLT_LDO_REGULATOR  0
#define VOLT_DCDC_CONVERT   1



#endif	/* _CONFIG_H_ */
