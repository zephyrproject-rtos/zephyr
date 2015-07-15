CONF_basic_minuteia = generic_pc/prj.conf
CONF_basic_atom     = generic_pc/prj_expert_test.conf
CONF_galileo        = quark/prj.conf

KERNEL_TYPE = nano
PLATFORM_CONFIG ?= basic_atom
CONF_FILE = $(CONF_$(PLATFORM_CONFIG))
SOURCE_DIR = $(ZEPHYR_BASE)/samples/microkernel/benchmark/boot_time/src/

include $(ZEPHYR_BASE)/Makefile.inc
