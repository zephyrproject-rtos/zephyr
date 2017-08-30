
zephyr_library_include_directories(${PROJECT_SOURCE_DIR}/drivers)

zephyr_cc_option(-march=lakemont -mtune=lakemont -msoft-float)

zephyr_sources(soc.c)
