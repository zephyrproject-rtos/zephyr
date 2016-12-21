BOARD ?= qemu_x86
# In QEMU x86, we have to force CONFIG_PCI being defined, as the defconfig does not have it.
ifeq ($(BOARD), qemu_x86)
CONF_FILE = prj_$(BOARD).conf
else
CONF_FILE = prj.conf
endif

include ${ZEPHYR_BASE}/Makefile.test
