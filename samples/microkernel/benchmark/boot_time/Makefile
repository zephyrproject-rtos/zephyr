TIMO_BASE = $(strip $(subst /samples/microkernel/benchmark/boot_time,,$(shell pwd) ))

BSP ?= generic_pc
# default, best, worst
BOOTTIME_QUALIFIER ?= default
CONF_generic_pc = generic_pc/prj_pentium4_${BOOTTIME_QUALIFIER}.conf
CONF_pentium4   = generic_pc/prj_pentium4_${BOOTTIME_QUALIFIER}.conf
CONF_minuteia   = generic_pc/prj.conf
CONF_atom_n28xx = generic_pc/prj_expert_test.conf
CONF_quark = quark/prj.conf

VPFILE = prj.vpf
KERNEL_TYPE = micro
CONF_OVERLAY = ${CONF_${BSP}}


$(if $(CONF_OVERLAY),,$(error no prj.conf file available))
$(if $(wildcard $(CONF_OVERLAY)),,$(error no $(CONF_OVERLAY) file available))

NO_QUALIFIED_PRJ_CONF_WARNING = $(strip \
  no prj.conf for $(BOOTTIME_QUALIFIER) and $(BSP), using default prj.conf \
)
$(if $(filter $(BSP)/prj.conf,$(CONF_OVERLAY)),$(warning *** $(NO_QUALIFIED_PRJ_CONF_WARNING) ***),)

$(info using CONF_OVERLAY = $(CONF_OVERLAY))

include ${TIMO_BASE}/Makefile.inc
