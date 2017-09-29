# default configuration

BOARD ?= qemu_x86
CONF_FILE ?= prj1.conf

KBUILD_KCONFIG = $(ZEPHYR_BASE)/tests/benchmarks/object_footprint/Kconfig
export KBUILD_KCONFIG

include $(ZEPHYR_BASE)/Makefile.test
