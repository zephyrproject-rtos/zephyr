zephyr_library()

zephyr_library_sources(bq40z50.c)

zephyr_include_directories_ifdef(CONFIG_EMUL_BQ40Z50 .)
zephyr_library_sources_ifdef(CONFIG_EMUL_BQ40Z50 ./emul_bq40z50.c)
