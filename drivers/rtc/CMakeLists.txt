zephyr_sources_ifdef(CONFIG_RTC_QMSI rtc_qmsi.c)
zephyr_sources_if_kconfig(rtc_mcux.c)
zephyr_sources_ifdef(CONFIG_USERSPACE   rtc_handlers.c)
