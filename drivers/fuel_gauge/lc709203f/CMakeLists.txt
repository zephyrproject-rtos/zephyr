zephyr_library_sources(lc709203f.c)

zephyr_include_directories_ifdef(CONFIG_EMUL_LC709203F .)
zephyr_library_sources_ifdef(CONFIG_EMUL_LC709203F ./emul_lc709203f.c)
