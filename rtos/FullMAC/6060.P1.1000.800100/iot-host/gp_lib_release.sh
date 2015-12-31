#/bin/bash

make clean

cp ./os/MicroC/gp3260xxa_15B_release_v0.0.2/application/task_wifi_lib/make/task_wifi_lib.a ./host/lib/

rm ./os/MicroC/gp3260xxa_15B_release_v0.0.2/application/task_wifi_lib/ -rf

rm ./host/version.c
rm ./host/tcpip/lwip-1.4.0/src/api/*.c
rm ./host/tcpip/lwip-1.4.0/src/core/*.c
rm ./host/tcpip/lwip-1.4.0/src/core/snmp/*.c
rm ./host/tcpip/lwip-1.4.0/src/core/ipv4/*.c
rm ./host/tcpip/lwip-1.4.0/src/netif/*.c
rm ./host/tcpip/lwip-1.4.0/ports/icomm/*.c
rm ./host/lib/*.c
rm ./host/lib/apps/*.c
rm ./host/core/*.c
rm ./host/ap/*.c
rm ./host/ap/common/*.c
rm ./host/app/netapp/iperf3.0/src/*.c
rm ./host/app/netmgr/*.c
rm ./host/app/netapp/udhcp/*.c

rm ./os/MicroC/gp3260xxa_15B_release_v0.0.2/project/GP3260xxa/gp3260xxa_platform_demo_release_captureRaw_lincorr.mcp

rm ./host/os_wrapper/LinuxSIM -rf

