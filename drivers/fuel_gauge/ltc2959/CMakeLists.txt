zephyr_library_sources(ltc2959.c)

zephyr_include_directories_ifdef(CONFIG_EMUL_LTC2959 .)
zephyr_library_sources_ifdef(CONFIG_EMUL_LTC2959  ./emul_ltc2959.c)
