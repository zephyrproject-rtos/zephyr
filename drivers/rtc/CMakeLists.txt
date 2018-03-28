zephyr_library()

zephyr_library_sources_ifdef(CONFIG_RTC_QMSI rtc_qmsi.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE   rtc_handlers.c)
