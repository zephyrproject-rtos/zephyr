zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_IPM_MCUX}     ipm_mcux.c
            IFDEF:${CONFIG_IPM_IMX}      ipm_imx.c
            IFDEF:${CONFIG_IPM_QUARK_SE} ipm_quark_se.c
            IFDEF:${CONFIG_USERSPACE}    ipm_handlers.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
