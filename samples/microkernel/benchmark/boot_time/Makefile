CONF_basic_minuteia = generic_pc/prj.conf
CONF_basic_atom     = generic_pc/prj_expert_test.conf
CONF_galileo        = quark/prj.conf

MDEF_FILE = prj.mdef
KERNEL_TYPE = micro
PLATFORM_CONFIG ?= basic_atom
CONF_FILE = ${CONF_${PLATFORM_CONFIG}}

include ${ZEPHYR_BASE}/Makefile.inc
