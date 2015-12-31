PERLPATH := $(shell perl ./script/modify.pl)
###########################################################
# Function to switch driver between relay device and SDIO 
# bridge device
# Enable ->Relay device	(CHAR)
# Disable->SDIO bridge device

DRV_OPT =  SDIO_LINUX_HCI_DRV

TOPDIR			:= .
export $(TOPDIR)

OS_DIR                  := $(TOPDIR)/os/LinuxSIM
SIM_OS_DIR              := $(TOPDIR)/os/LinuxSIM/rtos
HOST_DIR                := $(TOPDIR)/host


HOST_AP_DIR             := $(HOST_DIR)/ap
HOST_CORE_DIR             := $(HOST_DIR)/core
HOST_MAC_DIR             := $(HOST_DIR)/mac

LWIP_DIR		:= $(HOST_DIR)/tcpip/lwip-1.4.0




CC      := gcc
# Changed __linux__ to __SSV_UNIX_SIM__ for CYGWIN compatible
CFLAGS  := -D__SSV_UNIX_SIM__ -fno-common -Werror -Wextra -Wunused-variable -Wunused-function -MMD
#-pedantic -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wredundant-decls -Wshadow -Wstrict-overflow=5 -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option
#CFLAGS  += -DAUTO_INIT_CONFIG_FW
LDFLAGS := -lpthread

include $(TOPDIR)/script/lint.mk

##
# Note XXXXX:
# Please keep this order. The simulation include shall be used first.

INCLUDE += -I$(HOST_DIR) \
          -I$(HOST_DIR)/include \
	  -I$(HOST_DIR)/include/priv \
          -I$(HOST_DIR)/include/priv/hw

INCLUDE += -I$(HOST_DIR)/ap 

INCLUDE += -I$(HOST_DIR)/app \
           -I$(HOST_DIR)/app/netapp \
           -I$(HOST_DIR)/app/netapp/udhcp \
           -I$(HOST_DIR)/app/netapp/ \
           -I$(HOST_DIR)/drv \
	   -I$(HOST_DIR)/os_wrapper/LinuxSIM \

INCLUDE += -I$(OS_DIR) \
           -I$(SIM_OS_DIR)/include \
           -I$(SIM_OS_DIR)/portable \
           -I$(SIM_OS_DIR)/portable/GCC/Posix
           
INCLUDE += -I$(LWIP_DIR)/src/include/ \
           -I$(LWIP_DIR)/ports/icomm/include \
           -I$(LWIP_DIR)/src/include/ipv4 \
           -I$(LWIP_DIR)/src/include/lwip \
           -I$(LWIP_DIR)/src/include/netif \
           -I$(LWIP_DIR)/ports/icomm/include/arch


SIM_OS_SRC     := croutine.c \
                    list.c \
                    queue.c \
                    tasks.c \
                    timers.c \
                    portable/GCC/Posix/port.c \
                    portable/MemMang/heap_3.c


SIM_OS_SRC     := $(addprefix $(SIM_OS_DIR)/, $(SIM_OS_SRC))

ARCH_DIR     := $(HOST_DIR)/lib/apps
ARCH_SRC     := msgevt.c \
                pbuf.c \
                host_log.c \
                host_global.c
                	
ARCH_SRC     := $(addprefix $(ARCH_DIR)/, $(ARCH_SRC))

CLI_DIR      := $(HOST_DIR)/app/cli
CLI_SRC	     := cli.c \
                cli_key.c \
                cmds/cli_cmd.c \
                cmds/cli_cmd_net.c \
                cmds/cli_cmd_soc.c \
                cmds/cli_cmd_sys.c \
		cmds/cli_cmd_wifi.c 
CLI_SRC	     := $(addprefix $(CLI_DIR)/, $(CLI_SRC))



MAC_DIR     := $(HOST_DIR)/mac
MAC_SRC     := ap_cfg.c \
                sta_cfg.c
MAC_SRC     := $(addprefix $(UTIL_DIR)/, $(UTIL_SRC))

LIBS_DIR     := $(HOST_DIR)/lib
LIBS_SRC     := ssv_lib.c \
                ssv_frame.c \
                ssv_msg.c \
                ssv_timer.c\
		efuse.c\
		
LIBS_SRC     := $(addprefix $(LIBS_DIR)/, $(LIBS_SRC))

LWIP_API_DIR := $(LWIP_DIR)/src/api
LWIP_API_SRC := api_lib.c \
                api_msg.c \
                err.c \
                netbuf.c \
                netdb.c \
                netifapi.c \
                sockets.c \
                tcpip.c
LWIP_API_SRC := $(addprefix $(LWIP_API_DIR)/, $(LWIP_API_SRC))

LWIP_CORE_DIR := $(LWIP_DIR)/src/core
LWIP_CORE_SRC := def.c \
                 dhcp.c \
                 dns.c \
                 inet_chksum.c \
                 lwip_init.c \
                 lwip_timers.c \
                 mem.c \
                 memp.c \
                 netif.c \
                 lwip_pbuf.c \
                 raw.c \
                 stats.c \
                 sys.c \
                 tcp.c \
                 tcp_in.c \
                 tcp_out.c \
                 udp.c
LWIP_CORE_SRC := $(addprefix $(LWIP_CORE_DIR)/, $(LWIP_CORE_SRC))

LWIP_CORE_IPV4_DIR := $(LWIP_DIR)/src/core/ipv4
LWIP_CORE_IPV4_SRC := autoip.c \
                      icmp.c \
                      igmp.c \
                      ip4.c \
                      ip4_addr.c \
                      ip_frag.c
LWIP_CORE_IPV4_SRC := $(addprefix $(LWIP_CORE_IPV4_DIR)/, $(LWIP_CORE_IPV4_SRC))

LWIP_CORE_SNMP_DIR := $(LWIP_DIR)/src/core/snmp
LWIP_CORE_SNMP_SRC := asn1_dec.c \
                      asn1_enc.c \
                      mib_structs.c \
                      mib2.c \
                      msg_in.c \
                      msg_out.c
LWIP_CORE_SNMP_SRC := $(addprefix $(LWIP_CORE_SNMP_DIR)/, $(LWIP_CORE_SNMP_SRC))

LWIP_NETIF_DIR := $(LWIP_DIR)/src/netif
LWIP_NETIF_SRC := etharp.c \
                  ethernetif.c
LWIP_NETIF_SRC := $(addprefix $(LWIP_NETIF_DIR)/, $(LWIP_NETIF_SRC))

LWIP_PORTS_DIR := $(LWIP_DIR)/ports/icomm
LWIP_PORTS_SRC := sys_arch.c
LWIP_PORTS_SRC := $(addprefix $(LWIP_PORTS_DIR)/, $(LWIP_PORTS_SRC))

OS_WRAPPER_DIR     := $(HOST_DIR)/os_wrapper/LinuxSIM
OS_WRAPPER_SRC     := rtos.c \
                   rtos_glue.c \
                   porting.c
OS_WRAPPER_SRC := $(addprefix $(OS_WRAPPER_DIR)/, $(OS_WRAPPER_SRC))

CORE_DIR     := $(HOST_DIR)/core
CORE_SRC     := host_apis.c \
                     host_cmd_engine.c \
                     host_cmd_engine_rx.c \
                     host_cmd_engine_tx.c \
					 txrx_hdl.c \
                     txrx_task.c \
					 mlme.c	
CORE_SRC     := $(addprefix $(CORE_DIR)/, $(CORE_SRC))

DRV_DIR     := $(HOST_DIR)/drv
DRV_SRC     := ssv_drv.c
                    
ifneq	($(DRV_OPT), SDIO_LINUX_HCI_DRV)
DRV_SRC     += sdio_linux/sdio_if_impl.c \
                    sdio_linux/sdio.c
else
DRV_SRC     += sdio_linux_hci/sdio_if_impl.c \
                    sdio_linux_hci/sdio.c
CFLAGS += -DSDIO_LINUX_HCI_DRV
GLOBAL_DEF += -DSDIO_LINUX_HCI_DRV
endif

DRV_SRC     := $(addprefix $(DRV_DIR)/, $(DRV_SRC))

AP_DIR := $(HOST_DIR)/ap
AP_SRC := ap.c \
               ap_config.c \
               ap_mlme.c \
               ap_rx.c \
               ap_sta_info.c \
               ap_tx.c \
               beacon.c \
               hw_config.c \
               ieee802_11_mgmt.c \
               neighbor_ap_list.c \
               wmm.c \
               wpa_auth.c \
               ap_drv_cmd.c
                             
AP_SRC := $(addprefix $(AP_DIR)/, $(AP_SRC))

AP_COMMON_DIR := $(AP_DIR)/common
AP_COMMON_SRC := bitmap.c \
                      ieee802_11_common.c
AP_COMMON_SRC := $(addprefix $(AP_COMMON_DIR)/, $(AP_COMMON_SRC))

NETAPP_DIR	:= $(HOST_DIR)/app/netapp
NETAPP_SRC	:= net_app.c \
                   getopt.c \
                   ping/ping.c \
                   ttcp/ttcp.c \
                   httpserver_raw/fs.c \
                   httpserver_raw/httpd.c \
                   httpserver_raw/httpd_cgi_ssi.c \
                   udhcp/arpping.c \
                   udhcp/common.c \
                   udhcp/dhcpd.c \
                   udhcp/files.c \
                   udhcp/leases.c \
                   udhcp/packet.c \
                   udhcp/read.c \
                   udhcp/socket.c \
                   udhcp/static_leases.c \
                   udhcp/time.c \
                   iperf3.0/src/cjson.c \
                   iperf3.0/src/iperf_api.c \
                   iperf3.0/src/iperf_client_api.c \
                   iperf3.0/src/iperf_error.c \
                   iperf3.0/src/iperf_locale.c \
                   iperf3.0/src/iperf_server_api.c \
                   iperf3.0/src/iperf_tcp.c \
                   iperf3.0/src/iperf_udp.c \
                   iperf3.0/src/iperf_util.c \
                   iperf3.0/src/iperf_main.c \
                   iperf3.0/src/net.c \
                   iperf3.0/src/tcp_info.c \
                   iperf3.0/src/tcp_window_size.c \
                   iperf3.0/src/units.c \
                   iperf3.0/src/timer.c
                   
NETAPP_SRC	:= $(addprefix $(NETAPP_DIR)/, $(NETAPP_SRC))

NETMGR_DIR	:= $(HOST_DIR)/app/netmgr
NETMGR_SRC	:= net_mgr.c
                  
                   
NETMGR_SRC	:= $(addprefix $(NETMGR_DIR)/, $(NETMGR_SRC))
HOST_SRC := main.c ethermac.c
HOST_SRC += init.c \
                    macaddr.c \
                    version.c
HOST_SRC := $(addprefix $(HOST_DIR)/, $(HOST_SRC))

WSIMP_DIR    := $(HOST_DIR)/app/wsimp
WSIMP_SRC    := wsimp_config.c \
                    wsimp_lib.c \
                    wsimp_sim.c 
WSIMP_SRC    := $(addprefix $(WSIMP_DIR)/, $(WSIMP_SRC))

TARGET := wifi-sim

SRCS := $(HOST_SRC)
SRCS += $(ARCH_SRC)
SRCS += $(CLI_SRC)
SRCS += $(MAC_SRC)
SRCS += $(LIBS_SRC)

SRCS += $(LWIP_API_SRC)	
SRCS += $(LWIP_CORE_SRC)
SRCS += $(LWIP_CORE_IPV4_SRC)
SRCS += $(LWIP_CORE_SNMP_SRC)
SRCS += $(LWIP_NETIF_SRC)
SRCS += $(LWIP_PORTS_SRC)
SRCS += $(LWIP_APPS_SRC)
SRCS += $(AP_SRC)
SRCS += $(AP_COMMON_SRC)
#SRCS += $(WSIMP_SRC)
SRCS += $(CORE_SRC)
SRCS += $(NETAPP_SRC)
SRCS += $(NETMGR_SRC)
SRCS += $(OS_WRAPPER_SRC)
SRCS += $(DRV_SRC)

SRCS += $(SIM_OS_SRC)

OBJS := $(subst .c,.o,$(SRCS))
DEPS := $(subst .o,.d,$(OBJS))


MACS := $(MAC_SRC)
CORES := $(CORE_SRC)
APS := $(AP_SRC)
APS += $(AP_COMMON_SRC)

#LINT_SRC_LIST := $(SRCS) #for LINT

LINT_SRC_LIST := $(HOST_SRC)
LINT_SRC_LIST += $(ARCH_SRC)
LINT_SRC_LIST += $(MAC_SRC)
LINT_SRC_LIST += $(LIBS_SRC)


LINT_SRC_LIST += $(AP_SRC)
LINT_SRC_LIST += $(AP_COMMON_SRC)
#SRCS += $(WSIMP_SRC)
LINT_SRC_LIST += $(CORE_SRC)
LINT_SRC_LIST += $(OS_WRAPPER_SRC)
LINT_SRC_LIST += $(DRV_SRC)

#LINT_SRC_LIST += $(SIM_OS_SRC)

COREOBJS := $(subst .c,.o,$(CORES))
APOBJS := $(subst .c,.o,$(APS))

MACOBJS := $(subst .c,.o,$(MACS))
LIBOBJS := $(MAC)

.PHONY: all wifi-sim clean rlib ap core mac lint
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -m32 -Xlinker -Map=output.map -g -o $@ $(OBJS) $(LDFLAGS)

rlib:
	ar crsv ./ssv-core.a $(COREOBJS) $(MACOBJS) $(APOBJS)

ap: 
#	$(CC) -g -o $@ $(APOBJS) $(LDFLAGS)
	rm -rf ./ap.a
	ar crsv ./ap.a $(APOBJS)
core:
#	$(CC) -m32 -g -o $@ $(COREOBJS) $(LDFLAGS)
	rm -rf ./core.a
	ar crsv ./core.a $(COREOBJS)
mac:
#	$(CC) -m32 -g -o $@ $(MACOBJS) $(LDFLAGS)
	-rm -rf ./mac.a
	ar crsv ./mac.a $(MACOBJS)

lint:
	rm -rf $(LINT_OUT)
	rm -rf $(LINT_SRC_FILE)
	rm -rf $(LINT_DEF_FILE)
	@for def in $(subst ./,/,$(GLOBAL_DEF)); do \
		echo $$def >>  $(LINT_DEF_FILE); \
	done
	#@echo $(GLOBAL_DEF) >  $(LINT_DEF_FILE)
	@for src in $(subst ./,/,$(LINT_SRC_LIST)); do \
	$(CYGPATH) $(CWD)$$src >> $(LINT_SRC_FILE); \
	done
	@for inc in $(subst -I.,,$(INCLUDE)); do \
	    echo -n '-I' >> $(LINT_DEF_FILE); \
	    $(CYGPATH) $(CWD)$$inc >> $(LINT_DEF_FILE); \
	done
	$(LINT) $(LINT_SRC_FILE) -os"($(LINT_OUT))" > 1.txt

clean:
	rm -rf $(OBJS) $(TARGET) $(DEPS)
	rm -rf `find . -name "*.d"`
	rm -rf output.map

-include $(DEPS)

#%.o: %.c
#	@echo "Compiling $@"
#	$(CC) -m32 $(CFLAGS) $(INCLUDE) -c -g -o $@ $<
#
#ethermac:
#	@echo "Compiling ethermac.o"
#	$(CC) -m32 $(CFLAGS) -c -g -o host/ethermac.o host/ethermac.c


%.o: %.c
	@echo "Compiling: $(notdir $@)"
	@if [ "$(notdir $@)" = "ethermac.o" ]; then \
	$(CC) -m32 $(CFLAGS) -c -g -o $@ $<; \
	else \
	$(CC) -m32 $(CFLAGS) $(INCLUDE) -c -g -o $@ $<; \
	fi




  
