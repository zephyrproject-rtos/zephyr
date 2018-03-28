zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LSM6DS0 lsm6ds0.c)
