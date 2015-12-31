#include <config.h>
#include <cmd_def.h>
#include <log.h>
#include <host_global.h>
#include "lwip/netif.h"
#include <hdr80211.h>
#include "cli_cmd_wifi.h"
#include "cli_cmd_test.h"
#include "cli_cmd_throughput.h"
#include <host_apis.h>
#include <hwmac/drv_mac.h>
#include <rtos.h>
#include <os_wrapper.h>
#include <ap/common/ieee80211.h>
#include <time.h>

#ifdef THROUGHPUT_TEST

THROUGHPUT_COMMAND	settingCommand;

void cmd_throughput(s32 argc, char *argv[])
{
	if(argc == 6)
	{
		settingCommand.transferLength = atoi(argv[2]);
		settingCommand.transferCount = atoi(argv[3]);

		if(strcmp(argv[1], "tx") == 0)
			settingCommand.mode = 1;
		else
			goto carsh;

		if(strcmp(argv[4], "r") == 0)
	    {
		    settingCommand.rateIndex= atoi(argv[5]);
	    }
		else
			goto carsh;

		ssv6xxx_wifi_ioctl( SSV6XXX_HOST_CMD_THROUGHTPUT,
	        &settingCommand, 
	        sizeof(THROUGHPUT_COMMAND)  
	    );
		return;
	}
	else if(argc == 4)
	{
		if(strcmp(argv[1], "fix") == 0)
			settingCommand.mode = 0;
		else
			goto carsh;

		if(strcmp(argv[2], "r") == 0)
	    {
		    settingCommand.rateIndex= atoi(argv[3]);
	    }
		else
			goto carsh;

		ssv6xxx_wifi_ioctl( SSV6XXX_HOST_CMD_THROUGHTPUT,
	        &settingCommand, 
	        sizeof(THROUGHPUT_COMMAND)
	    );
		return;
	}
	else if(argc == 3)
	{
		if(strcmp(argv[1], "backoff") == 0)
        {      
			settingCommand.mode = 2;
            settingCommand.rateIndex= atoi(argv[2]);
        }
		else if(strcmp(argv[1], "rx") == 0)
		{
		    settingCommand.mode = 3;
            settingCommand.rateIndex= atoi(argv[2]);
       }
        else if(strcmp(argv[1], "qos")==0)
        {
            settingCommand.mode = 4;
            settingCommand.qos = atoi(argv[2]);
            settingCommand.noack =0x0;
            
        }
        else if(strcmp(argv[1], "nqos")==0)
        {
            settingCommand.mode = 4;
            settingCommand.qos = 0;
            settingCommand.noack = atoi(argv[2]);
            
        }
		else
			goto carsh;

		
        
		ssv6xxx_wifi_ioctl( SSV6XXX_HOST_CMD_THROUGHTPUT,
	        &settingCommand, 
	        sizeof(THROUGHPUT_COMMAND)
	    );
		return;
	}
    
carsh:
	log_printf("\nSend tx frame from DUT: dut tx [50-65535] [send frames] r [rateidx]");
	log_printf("\n    rateidx[39] special mode");
	log_printf("\n    rateidx[40] Report mode [64/1536/2304] [0-38]");
	log_printf("\n    rateidx[41] Report mode [64/1536/2304] [0-6]");
	log_printf("\n    rateidx[42] Report mode [64/1536/2304] [0-14]");
	log_printf("\n    rateidx[43] Report mode [64/1536/2304] [0-30]");
	log_printf("\n    rateidx[44] Report mode [64/1536/2304] [7-14]");
	log_printf("\n    rateidx[45] Report mode [64/1536/2304] [31-38]");
	log_printf("\n    rateidx[46] Report mode [length] [0-6]");
	log_printf("\n    rateidx[47] Report mode [length] [7-14]");
	log_printf("\n    rateidx[48] Report mode [length] [15-38]\n\n");

	log_printf("Fix tx rate(SDIO)    : dut fix r [rateidx]\n");
	log_printf("Backoff switch       : dut backoff [0-1] \n");
	log_printf("Verify rx throughput : dut rx [0-1]\n");
    log_printf("qos                  : dut qos [0x00-0xff]\n");
    log_printf("ack                 : dut nqos [0-1]\n");
	return;
	
}
#endif	

