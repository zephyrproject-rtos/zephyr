zephyr_library()

zephyr_library_sources_ifdef(CONFIG_RTC_MCUX rtc_mcux.c)
zephyr_library_sources_ifdef(CONFIG_RTC_STM32 rtc_ll_stm32.c)
