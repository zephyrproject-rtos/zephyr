zephyr_library()

zephyr_library_sources_ifdef(CONFIG_RTC_QMSI rtc_qmsi.c)
zephyr_library_sources_ifdef(CONFIG_RTC_MCUX rtc_mcux.c)
zephyr_library_sources_ifdef(CONFIG_RTC_STM32 rtc_ll_stm32.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE   rtc_handlers.c)
