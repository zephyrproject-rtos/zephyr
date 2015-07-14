CONF_nano_generic_pc_minuteia_defconfig   = generic_pc/prj.conf
CONF_nano_generic_pc_atom_n28xx_defconfig = generic_pc/prj_expert_test.conf
CONF_nano_quark_defconfig                 = quark/prj.conf

KERNEL_TYPE = nano
KBUILD_DEFCONFIG ?= $(KERNEL_TYPE)_generic_pc_atom_n28xx_defconfig
CONF_FILE = $(CONF_$(KBUILD_DEFCONFIG))
SOURCE_DIR = $(ZEPHYR_BASE)/samples/microkernel/benchmark/boot_time/src/

include $(ZEPHYR_BASE)/Makefile.inc

$(if $(CONF_FILE),,$(error no prj.conf file available))
$(if $(wildcard $(CONF_FILE)),,$(error no $(CONF_FILE) file available))
