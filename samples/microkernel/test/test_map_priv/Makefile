MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
PLATFORM_CONFIG ?= basic_atom
CONF_FILE = prj_$(ARCH).conf
SOURCE_DIR := $(ZEPHYR_BASE)/samples/microkernel/test/test_map/src

# Enable testing for private microkernel memory map objects
CFLAGS = -DTEST_PRIV_MEM_MAPS

include ${ZEPHYR_BASE}/Makefile.inc
