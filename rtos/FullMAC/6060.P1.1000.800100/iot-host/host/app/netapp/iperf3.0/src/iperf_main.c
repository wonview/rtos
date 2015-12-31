/*
 * iperf, Copyright (c) 2014, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE.  This software is owned by the U.S. Department of Energy.
 * As such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */
#include "iperf_config.h"

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <getopt.h>
//#include <errno.h>
//#include <signal.h>
//#include <unistd.h>
//#ifdef HAVE_STDINT_H
//#include <stdint.h>
//#endif
//#include <sys/types.h>

#include <os_wrapper.h>
#include <rtos.h>
#include <log.h>
#include "common.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/tcp.h"

#include "iperf.h"
#include "iperf_api.h"
#include "units.h"
#include "iperf_locale.h"
#include "net.h"


static int run(struct iperf_test *test);
static bool iperf_running_flag = false;
static struct iperf_test *g_iperf_test1 = NULL, *g_iperf_test2 = NULL;
static OsMutex iperf_api_mutex = NULL;

void iperf_api_mutex_lock (bool lock)
{
    if(lock)
        OS_MutexLock(iperf_api_mutex);
    else
        OS_MutexUnLock(iperf_api_mutex);
}

bool iperf_get_runflag()
{
    return iperf_running_flag;
}

void iperf_clear_runflag()
{
    iperf_api_mutex_lock(1);

    iperf_running_flag = false;

    iperf_api_mutex_lock(0);
}

void iperf_list()
{
    int i=0;
    iperf_api_mutex_lock(1);

    if (g_iperf_test1){
        PRINTF("iperf task%d: %s, server port: %d \r\n", ++i,
            g_iperf_test1->role == 'c' ? "client" : "server",
            g_iperf_test1->server_port);
    }
    if (g_iperf_test2){
        PRINTF("iperf task%d: %s, server port: %d \r\n", ++i,
            g_iperf_test2->role == 'c' ? "client" : "server",
            g_iperf_test2->server_port);
    }

    if (!g_iperf_test1 && !g_iperf_test2){
        PRINTF("No iperf thread running. \r\n");
    }
    iperf_api_mutex_lock(0);
}

/*check iperf3 thread running via server_port
   only one instance permitted on a #server_port
   if you need two running instances, please add option -p <server_port> when run iperf3
   Totally, maximum instance is 2.
*/
bool iperf_add_test (struct iperf_test *test)
{
    bool result = true;

    iperf_api_mutex_lock(1);
    
    if (g_iperf_test1 && g_iperf_test2){
        PRINTF("ERROR -- there have already been two iperf threads running! \r\n");
        result = false;
    }
    else{    
        if (g_iperf_test1 && g_iperf_test1->server_port == test->server_port)
            result = false;
        else if (g_iperf_test2 && g_iperf_test2->server_port == test->server_port)
            result = false;

        if (result){
            if (!g_iperf_test1)
                g_iperf_test1 = test;
            else
                g_iperf_test2 = test;

            iperf_running_flag = true;
        }else{
            PRINTF("ERROR -- there has already been an iperf thread with server port %d! \r\n", test->server_port);
            PRINTF("Please use option '-p <port>' to designate another server port.\r\n");
        }
    }
    
    iperf_api_mutex_lock(0);
    
    return result;
}

void iperf_del_test (struct iperf_test *test)
{
    iperf_api_mutex_lock(1);
    
    if (g_iperf_test1 == test)
        g_iperf_test1 = NULL;
    else if (g_iperf_test2 == test)
        g_iperf_test2 = NULL;

    if (!g_iperf_test1 && !g_iperf_test2)
        iperf_running_flag = false;

    iperf_api_mutex_lock(0);
}

int iperf_init()
{
    iperf_running_flag = false;
    g_iperf_test1 = NULL;
    g_iperf_test2 = NULL;

    return OS_MutexInit(&iperf_api_mutex);
}


#ifdef IPERF_DEBUG_MEM
extern int iperf_mallocs;
extern int iperf_frees;
extern int iperf_msize;
extern int iperf_max_mallocs;
extern int iperf_max_msize;
#endif

#ifdef __SSV_UNIX_SIM__
extern int netmgr_send_arp_unicast (u8 *dst_mac);
#endif

/**************************************************************************/
void
net_app_iperf3(int argc, char **argv)
{
    struct iperf_test *test;
    int retval;

#ifdef __SSV_UNIX_SIM__
    if (argc == 3 && STRCMP(argv[1], "arp") == 0)
    {
        u8 mac[6]={0}; //={0xd8,0xfc,0x93,0x2c,0xc9,0xff};
        int dst_mac[6], i;
        retval = sscanf(argv[2], "%02x:%02x:%02x:%02x:%02x:%02x", //sscanf doesn't work on GP15B platform
                &dst_mac[0],&dst_mac[1],&dst_mac[2],
                &dst_mac[3],&dst_mac[4],&dst_mac[5]);
        
        if (retval != 6){
            PRINTF("sscanf get MAC address: %02x:%02x:%02x:%02x:%02x:%02x \r\n",
                dst_mac[0],dst_mac[1],dst_mac[2],
                dst_mac[3],dst_mac[4],dst_mac[5]);
            PRINTF("Invalid MAC address [%s], retval:%d \r\n", argv[2], retval);
            return;
        }
        for(i=0;i<6;i++)
            mac[i]=(u8)dst_mac[i];
        retval = netmgr_send_arp_unicast(mac);
        PRINTF("arp unicast request return %s \r\n", retval ? "FAILURE" : "SUCCESS!");
        return ;
    }
#endif

    if (!iperf_api_mutex && iperf_init()){
        PRINTF("iperf init failed! \r\n");
        return;
    }

    PRINTF("\r\n");
    if (argc == 2 && STRCMP(argv[1], "stop") == 0)
    {
        if (iperf_get_runflag()){
            iperf_clear_runflag();
            PRINTF("The iperf threads stopped! \r\n");
            sys_msleep(2000);

            #ifdef IPERF_DEBUG_MEM
            PRINTF("iperf mallocs:%d, frees:%d, delta:%d, delta size:%d\r\n", iperf_mallocs, iperf_frees, iperf_mallocs - iperf_frees, iperf_msize);
            PRINTF("iperf max mallocs:%d, max size:%d \r\n", iperf_max_mallocs, iperf_max_msize);
            iperf_msize = iperf_mallocs = iperf_frees = iperf_max_mallocs = iperf_max_msize = 0;
            #endif
        }
        else{
            PRINTF("No iperf thread running. \r\n");
        }
        return;
    }else if (argc == 2 && STRCMP(argv[1], "list") == 0){
        iperf_list();
        return;
    }else if (argc == 2 && STRCMP(argv[1], "diag") == 0){
        float float_var = 1.4;
        double double_var = 1.4;
        unsigned long long u64_var = 0x500000004;
        char str[30];

        PRINTF("float_var = %f \r\n \r\n", float_var);
        PRINTF("double_var = %g \r\n \r\n", double_var);
        PRINTF("u64_var = %llu \r\n", u64_var);

        sprintf( str, "%lld", u64_var);
        PRINTF("u64_var=%s after sprintf \r\n", str);
        return;
    }
    test = iperf_new_test();
    if (!test)
    {
        PRINTF("create new test error - %s \r\n", iperf_strerror(IENEWTEST));
        return;
    }
    
    iperf_defaults(test);	/* sets defaults */
    retval = iperf_parse_arguments(test, argc, argv);
    if (retval < 0) {
        PRINTF("parameter error - %s \r\n", iperf_strerror(test->i_errno));
        PRINTF("\r\n");

        iperf_free_test(test);
        usage_long();
        return;
    }
    else if (retval == 100)
    {        
        iperf_free_test(test);
        return;
    }
    
    if (iperf_add_test(test)){
        if (run(test) < 0)
            PRINTF("error - %s \r\n", iperf_strerror(test->i_errno));
    }else{
        PRINTF("Please run command \"iperf3 list\" to show the running iperf threads. \r\n");	
        PRINTF("Please run command \"iperf3 stop\" to stop the running threads. \r\n");	
    }
	
    iperf_del_test(test);
    iperf_free_test(test);
}

/**************************************************************************/
static int
run(struct iperf_test *test)
{
    int consecutive_errors;

    switch (test->role) {
        case 's':
    	    consecutive_errors = 0;
            for (; iperf_get_runflag(); ) {
    		    if (iperf_run_server(test) < 0) {
    		        PRINTF("error - %s \r\n", iperf_strerror(test->i_errno));
                    PRINTF("\r\n");
    		        ++consecutive_errors;
    		        if (consecutive_errors >= 5) {
    		            PRINTF("too many errors, exiting\r\n");
    			        break;
    		        }
                } else
    		        consecutive_errors = 0;
                
                iperf_reset_test(test);
            }
            break;
    	case 'c':
    	    if (iperf_run_client(test) < 0)
    		    PRINTF("error - %s \r\n", iperf_strerror(test->i_errno));
            break;
        default:
            iperf_usage();
            break;
    }

    return 0;
}
