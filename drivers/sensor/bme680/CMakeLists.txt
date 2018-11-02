zephyr_library()

zephyr_library_sources_ifdef(CONFIG_BME680 bme680.c)
