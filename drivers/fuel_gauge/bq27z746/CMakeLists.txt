zephyr_library()

zephyr_library_sources(bq27z746.c)

zephyr_include_directories_ifdef(CONFIG_EMUL_BQ27Z746 .)
zephyr_library_sources_ifdef(CONFIG_EMUL_BQ27Z746 ./emul_bq27z746.c)
