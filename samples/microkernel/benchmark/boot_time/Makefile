# default, best, worst
BOOTTIME_QUALIFIER ?= default
CONF_micro_generic_pc_defconfig            = generic_pc/prj_pentium4_${BOOTTIME_QUALIFIER}.conf
CONF_micro_generic_pc_pentium4_defconfig   = generic_pc/prj_pentium4_${BOOTTIME_QUALIFIER}.conf
CONF_micro_generic_pc_minuteia_defconfig   = generic_pc/prj.conf
CONF_micro_generic_pc_atom_n28xx_defconfig = generic_pc/prj_expert_test.conf
CONF_micro_quark_defconfig                 = quark/prj.conf

MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
KBUILD_DEFCONFIG ?= $(KERNEL_TYPE)_generic_pc_atom_n28xx_defconfig
CONF_FILE = ${CONF_${KBUILD_DEFCONFIG}}


$(if $(CONF_FILE),,$(error no prj.conf file available))
$(if $(wildcard $(CONF_FILE)),,$(error no $(CONF_FILE) file available))

NO_QUALIFIED_PRJ_CONF_WARNING = $(strip \
  no prj.conf for $(BOOTTIME_QUALIFIER) and $(KBUILD_DEFCONFIG), using default prj.conf \
)
$(if $(filter $(KBUILD_DEFCONFIG)/prj.conf,$(CONF_FILE)),$(warning *** $(NO_QUALIFIED_PRJ_CONF_WARNING) ***),)

$(info using CONF_FILE = $(CONF_FILE))

include ${ZEPHYR_BASE}/Makefile.inc
