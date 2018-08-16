zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_WDT_QMSI}       wdt_qmsi.c
            IFDEF:${CONFIG_IWDG_STM32}     iwdg_stm32.c
            IFDEF:${CONFIG_WDOG_CMSDK_APB} wdog_cmsdk_apb.c
            IFDEF:${CONFIG_WDT_SAM}        wdt_sam.c
            IFDEF:${CONFIG_WDT_ESP32}      wdt_esp32.c
            IFDEF:${CONFIG_WDT_SAM0}       wdt_sam0.c
            IFDEF:${CONFIG_WDT_NRFX}       wdt_nrfx.c
            IFDEF:${CONFIG_WDT_MCUX_WDOG} wdt_mcux_wdog.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
