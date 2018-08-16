
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_RTC_QMSI}  rtc_qmsi.c
            IFDEF:${CONFIG_RTC_MCUX}  rtc_mcux.c
            IFDEF:${CONFIG_RTC_STM32} rtc_ll_stm32.c
            IFDEF:${CONFIG_USERSPACE} rtc_handlers.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
