BOARD ?= qemu_x86

ifeq (${PROFILER}, 1)
PROF="_prof"
endif

CONF_FILE ?= prj_${BOARD}${PROF}.conf

include ${ZEPHYR_BASE}/Makefile.inc

ifeq ($(CONFIG_NET_L2_BLUETOOTH), y)
	QEMU_EXTRA_FLAGS = -serial unix:/tmp/bt-server-bredr
else
	include $(ZEPHYR_BASE)/samples/net/common/Makefile.ipstack
endif
