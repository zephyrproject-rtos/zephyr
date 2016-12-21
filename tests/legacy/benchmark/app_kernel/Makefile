BOARD ?= qemu_x86
MDEF_FILE = prj.mdef

# Boards where we want to exercise this test case with floating point
# and SSE instructions
CONF_minnowboard = prj_fp.conf

CONF_FILE = ${CONF_${BOARD}}
ifeq ($(CONF_FILE),)
CONF_FILE = prj_no_fp.conf
endif

include ${ZEPHYR_BASE}/Makefile.test
