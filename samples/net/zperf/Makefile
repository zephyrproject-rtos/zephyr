BOARD ?= qemu_x86
#BOARD ?= frdm_k64f

ifeq (${PROFILER}, 1)
PROF="_prof"
endif

CONF_FILE ?= prj_${BOARD}${PROF}.conf

include ${ZEPHYR_BASE}/Makefile.inc
include $(ZEPHYR_BASE)/samples/net/common/Makefile.ipstack
