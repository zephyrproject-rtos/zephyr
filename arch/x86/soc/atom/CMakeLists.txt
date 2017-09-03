zephyr_library()
zephyr_library_include_directories(${PROJECT_SOURCE_DIR}/drivers)

zephyr_cc_option(-march=atom)
zephyr_cc_option_fallback(-mtune=atom -mtune=generic)

zephyr_library_sources(soc.c)
