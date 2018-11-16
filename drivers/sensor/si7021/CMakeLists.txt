zephyr_library()

zephyr_library_sources_ifdef(CONFIG_SI7021 si7021.c)