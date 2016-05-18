ccflags-y += -I${srctree}/net/ip/contiki
ccflags-y += -I${srctree}/net/ip/contiki/os/lib
ccflags-y += -I${srctree}/net/ip/contiki/os
ccflags-y += -I${srctree}/net/ip

obj-y += zperf_shell.o
obj-y += shell_utils.o
obj-y += zperf_udp_uploader.o
obj-y += zperf_udp_receiver.o
obj-y += zperf_session.o
obj-${CONFIG_NETWORKING_WITH_TCP} += zperf_tcp_receiver.o zperf_tcp_uploader.o
