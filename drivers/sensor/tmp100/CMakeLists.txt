zephyr_include_directories(.)
zephyr_sources_ifdef(CONFIG_TMP100 tmp100.c)
