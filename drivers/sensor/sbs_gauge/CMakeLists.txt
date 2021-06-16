zephyr_library()

zephyr_library_sources_ifdef(CONFIG_SBS_GAUGE sbs_gauge.c)
