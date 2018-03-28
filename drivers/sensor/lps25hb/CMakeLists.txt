zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LPS25HB lps25hb.c)
