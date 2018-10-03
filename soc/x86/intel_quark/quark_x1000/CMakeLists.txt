
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_cc_option(-march=lakemont -mtune=lakemont -msoft-float)

zephyr_sources(soc.c)
