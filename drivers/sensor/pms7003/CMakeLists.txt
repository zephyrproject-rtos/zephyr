zephyr_library()

zephyr_library_sources_ifdef(CONFIG_PMS7003 pms7003.c)
