ifeq (${PROFILER}, 1)
ccflags-y += -DPROFILER
endif

ccflags-y += -I${ZEPHYR_BASE}/subsys/net/ip
ccflags-y += -I${ZEPHYR_BASE}/samples/task_profiler/profiler/src

obj-y += zperf_shell.o
obj-y += shell_utils.o
obj-y += zperf_session.o
obj-$(CONFIG_NET_UDP) += zperf_udp_receiver.o zperf_udp_uploader.o
obj-${CONFIG_NET_TCP} += zperf_tcp_receiver.o zperf_tcp_uploader.o

ifeq (${PROFILER}, 1)
export PROFILER_NO_SHELL_REGISTER=1
obj-y += ../../../task_profiler/profiler/
endif

ifeq ($(CONFIG_NET_L2_BLUETOOTH), y)
ccflags-y +=-I${ZEPHYR_BASE}/samples/bluetooth/
obj-y += ../../../bluetooth/gatt/ipss.o
endif
