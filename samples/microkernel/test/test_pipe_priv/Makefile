MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
PLATFORM_CONFIG ?= basic_atom
CONF_FILE = prj_$(ARCH).conf

# Enable testing for private microkernel pipe objects
CFLAGS = -DTEST_PRIV_PIPES

include ${ZEPHYR_BASE}/Makefile.inc
