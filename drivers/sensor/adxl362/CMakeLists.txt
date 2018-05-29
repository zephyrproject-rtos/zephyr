zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ADXL362 adxl362.c)
