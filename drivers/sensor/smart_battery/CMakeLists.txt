zephyr_library()

zephyr_library_sources_ifdef(CONFIG_SMART_BATTERY smart_battery.c)
