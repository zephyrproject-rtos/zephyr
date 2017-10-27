zephyr_library()
zephyr_library_include_directories(${PROJECT_SOURCE_DIR}/drivers)
zephyr_library_sources_if_kconfig(pinmux.c)
