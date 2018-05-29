zephyr_library()

zephyr_library_sources_ifdef(CONFIG_BME280 bme280.c)
