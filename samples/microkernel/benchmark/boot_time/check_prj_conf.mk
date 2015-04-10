ifeq (${vGOAL_NEEDS_TARGET_SETUP},y)
  $(if ${PRJ_CONF},,$(error no prj.conf file available))
  $(if $(wildcard ${PRJ_CONF}),,$(error no ${PRJ_CONF} file available))

  SHOW_VARIANT = $(if ${vBSP_VARIANT},(${vBSP_VARIANT}),)
  NO_QUALIFIED_PRJ_CONF_WARNING = $(strip \
    no prj.conf for ${BOOTTIME_QUALIFIER} for ${vBSP}${SHOW_VARIANT}, using default prj.conf \
  )
  $(if $(filter ${vBSP}/prj.conf,${PRJ_CONF}),$(warning *** ${NO_QUALIFIED_PRJ_CONF_WARNING} ***),)

  $(info using PRJ_CONF = ${PRJ_CONF})
endif
