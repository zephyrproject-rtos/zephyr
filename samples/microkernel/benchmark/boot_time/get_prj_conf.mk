#####
# find correct prj.conf

# default, best, worst
BOOTTIME_QUALIFIER ?= default

# generic_pc BSP
PRJ_CONF_generic_pc_pentium4 = ${vBSP}/prj_${vBSP_VARIANT}_${BOOTTIME_QUALIFIER}.conf
PRJ_CONF_generic_pc_minuteia = ${vBSP}/prj.conf
PRJ_CONF_generic_pc_atom_n28xx = ${vBSP}/prj_expert_test.conf

# quark
PRJ_CONF_quark_ = ${vBSP}/prj.conf

PRJ_CONF = ${PRJ_CONF_${vBSP}_${vBSP_VARIANT}}

#####
