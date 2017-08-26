
zephyr_library_include_directories(PRIVATE ${PROJECT_SOURCE_DIR}/drivers)

target_cc_option(zephyr PUBLIC -march=lakemont -mtune=lakemont -msoft-float)

zephyr_sources(soc.c)
