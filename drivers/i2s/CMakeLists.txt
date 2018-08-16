zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            i2s_common.c
            IFDEF:${CONFIG_I2S_SAM_SSC} i2s_sam_ssc.c
            IFDEF:${CONFIG_I2S_CAVS}    i2s_cavs.c
            IFDEF:${CONFIG_USERSPACE}   i2s_handlers.c
            IFDEF:${CONFIG_I2S_STM32}   i2s_ll_stm32.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
