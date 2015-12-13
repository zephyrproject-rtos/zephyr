CONF_basic_minuteia = ia32/prj.conf
CONF_quark_d2000_ref_board   = quark_d2000/quark_d2000_prj.conf
CONF_quark_se_test_board     = quark_se/quark_se_prj.conf
CONF_qemu_x86       = ia32/prj_expert_test.conf
CONF_galileo        = ia32_pci/prj.conf
CONF_basic_atom     = ia32/prj_expert_test.conf

KERNEL_TYPE = nano
PLATFORM_CONFIG ?= qemu_x86
CONF_FILE = $(CONF_$(PLATFORM_CONFIG))
SOURCE_DIR = $(ZEPHYR_BASE)/samples/microkernel/benchmark/boot_time/src/

include $(ZEPHYR_BASE)/Makefile.inc
