ifeq (${PROFILER}, 1)
ccflags-y += -DPROFILER
endif


ccflags-y += -I${ZEPHYR_BASE}/net/ip/contiki
ccflags-y += -I${ZEPHYR_BASE}/net/ip/contiki/os/lib
ccflags-y += -I${ZEPHYR_BASE}/net/ip/contiki/os
ccflags-y += -I${ZEPHYR_BASE}/net/ip
ccflags-y += -I${ZEPHYR_BASE}/samples/task_profiler/profiler/src

obj-y += zperf_shell.o
obj-y += shell_utils.o
obj-y += zperf_udp_uploader.o
obj-y += zperf_udp_receiver.o
obj-y += zperf_session.o
obj-${CONFIG_NETWORKING_WITH_TCP} += zperf_tcp_receiver.o zperf_tcp_uploader.o

ifeq (${PROFILER}, 1)
export PROFILER_NO_SHELL_REGISTER=1
obj-y += ../../../task_profiler/profiler/
endif
