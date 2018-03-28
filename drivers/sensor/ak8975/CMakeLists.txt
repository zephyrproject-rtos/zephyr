zephyr_library()

zephyr_library_sources_ifdef(CONFIG_AK8975 ak8975.c)
