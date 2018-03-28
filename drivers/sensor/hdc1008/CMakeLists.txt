zephyr_library()

zephyr_library_sources_ifdef(CONFIG_HDC1008 hdc1008.c)
