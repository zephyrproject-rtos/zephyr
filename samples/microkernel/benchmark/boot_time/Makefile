CONF_generic_pc_minuteia   = generic_pc/prj.conf
CONF_generic_pc_atom_n28xx = generic_pc/prj_expert_test.conf
CONF_quark                 = quark/prj.conf

MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
PLATFORM_CONFIG ?= generic_pc_atom_n28xx
CONF_FILE = ${CONF_${PLATFORM_CONFIG}}


$(if $(CONF_FILE),,$(error no prj.conf file available))
$(if $(wildcard $(CONF_FILE)),,$(error no $(CONF_FILE) file available))

include ${ZEPHYR_BASE}/Makefile.inc
