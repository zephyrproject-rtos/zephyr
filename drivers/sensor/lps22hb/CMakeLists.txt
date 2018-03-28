zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LPS22HB lps22hb.c)
