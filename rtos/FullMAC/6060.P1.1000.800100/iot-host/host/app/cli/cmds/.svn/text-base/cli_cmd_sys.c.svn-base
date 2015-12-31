#include <host_cmd_engine.h>
#include "../../../core/txrx_task.h"
#include <host_global.h>
#include <os_wrapper.h>
#include <log.h>
#include "cli_cmd_sys.h"

char CmdEngModeString[3][8] = {"STOP", "RUNNING", "EXIT"};
#ifdef __SSV_UNIX_SIM__
char SysProfBuf[256];
s8 SysProfSt = -1;
char g_SysTaskStack[1024];
#endif
void usage(void)
{
    LOG_PRINTF("Usage:\r\n");
    LOG_PRINTF("      sys cmdeng st\r\n");
    LOG_PRINTF("      sys soc st\r\n");
    LOG_PRINTF("      sys txq\r\n");
	LOG_PRINTF("      sys log\r\n");
#ifdef __SSV_UNIX_SIM__
    LOG_PRINTF("      sys profile\r\n");
#endif
}


void log_usage(void)
{
    LOG_PRINTF("Usage:\r\n");
    LOG_PRINTF("      sys log show\r\n");
    LOG_PRINTF("      sys log reset\r\n");
    LOG_PRINTF("      sys log lev [0|1|2|3]\r\n");
    LOG_PRINTF("      sys log mod [add|rem] [module name]\r\n");
	LOG_PRINTF("      module name = [all|mem|socket|api|tcp|udp|ip|other]\r\n");
}


static void GetSocStatus(void)
{
	 ssv6xxx_wifi_ioctl(SSV6XXX_HOST_CMD_GET_SOC_STATUS, NULL, 0);
}



extern u32 g_log_module;
extern u32 g_log_min_level;

static void ShowLog(void)
{
	 LOG_PRINTF("g_log_module=%08x \r\n", g_log_module);
     LOG_PRINTF("g_log_min_level=%08x \r\n", g_log_min_level);
}


void cmd_sys(s32 argc, char *argv[])
{
    if ( (argc == 3) && (strcmp(argv[1], "cmdeng") == 0))
    {
        if(strcmp(argv[2], "st") == 0)
        {
            struct CmdEng_st st;
            MEMSET(&st, 0, sizeof(struct CmdEng_st));
            
            CmdEng_GetStatus(&st);
            LOG_PRINTF("Mode:%s , BlockCmd:%s, %d Cmds In pendingQ\r\n", CmdEngModeString[st.mode], (st.BlkCmdIn == true)?"YES":"NO", st.BlkCmdNum);            
        }
        else if(strcmp(argv[2], "dbg") == 0)
        {
            bool dbgst = CmdEng_DbgSwitch();
            LOG_PRINTF("CmdEng debug %s\r\n", (dbgst != false)?"on":"off");
        }
        else
            LOG_PRINTF("Invalid Command\r\n");
    }
    else if ( (argc == 2) &&(strcmp(argv[1], "txq") == 0))
        TXRXTask_ShowSt();
    else if ( (argc == 3) && (strcmp(argv[1], "soc") == 0) )
    {
        if(strcmp(argv[2], "st") == 0)
        {
            GetSocStatus();
        }
    }
#ifdef __SSV_UNIX_SIM__
    else if ((argc == 2) && (strcmp(argv[1],"profile") == 0) )
    {
	s32 ret = OS_SysProfiling(&SysProfBuf);
	if(ret == OS_SUCCESS)
	{
            SysProfSt = 1;
            LOG_PRINTF("Enable system profiling\n");
	}
        else
            LOG_PRINTF("Fail to enable system profiling\n");
    }
    else if((argc == 2)&&(strcmp(argv[1],"stack") == 0)){
        vTaskList( g_SysTaskStack );
        LOG_PRINTF("%s \r\n", g_SysTaskStack);
    }    
#endif
	else if ((argc >= 2) && (strcmp(argv[1],"log") == 0)) {
        bool add=true;
        u32 module_bit = 0;


        if ((argc == 3) && (strcmp(argv[2],"show") == 0 )){
            ShowLog();
            return;
        }else if ((argc == 3) && (strcmp(argv[2],"reset") == 0 )){
            g_log_module = CONFIG_LOG_MODULE;
		    g_log_min_level = CONFIG_LOG_LEVEL;
            ShowLog();

            return;
        }else if ((argc == 4)&&(strcmp(argv[2],"lev") == 0 )){
             g_log_min_level = ssv6xxx_atoi(argv[3]);
             ShowLog();

            return;
        }else if ((argc == 5)&&(strcmp(argv[2],"mod") == 0)){

            //operation
            if ((strcmp(argv[3],"add") == 0 ))
                add =true;

            if ((strcmp(argv[3],"rem") == 0 ))
                add =false;


            //module
            if ((strcmp(argv[4],"all") == 0 ))
                module_bit = LOG_ALL_MODULES;
            else if((strcmp(argv[4],"mem") == 0 ))
                module_bit = LOG_MEM;
            else if((strcmp(argv[4],"socket") == 0 ))
                module_bit = LOG_L3_SOCKET;
            else if((strcmp(argv[4],"api") == 0 ))
                module_bit = LOG_L3_API;
            else if((strcmp(argv[4],"tcp") == 0 ))
                module_bit = LOG_L3_TCP;
            else if((strcmp(argv[4],"udp") == 0 ))
                module_bit = LOG_L3_UDP;
            else if((strcmp(argv[4],"ip") == 0 ))
                module_bit = LOG_L3_IP;
            else if((strcmp(argv[4],"other") == 0 ))
                module_bit = LOG_L3_OTHER_PROTO;
            else
            {;} 


            if( true == add){
                //ADD module
                g_log_module |= module_bit;
            }else{

                //Remove module
                g_log_module &= (~module_bit);
            }
            ShowLog();
            return;
        }else
        {;}

        
        log_usage();		



	}
    else
    {
        LOG_PRINTF("Invalid Command\r\n");
        usage();
    }   
}
