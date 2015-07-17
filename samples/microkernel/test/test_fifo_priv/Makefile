MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
PLATFORM_CONFIG ?= basic_atom
CONF_FILE = prj_$(ARCH).conf

# Enable testing for private microkernel FIFOs
CFLAGS = -DTEST_PRIV_FIFO

include ${ZEPHYR_BASE}/Makefile.inc
