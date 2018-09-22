zephyr_library()
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_cc_option(-march=silvermont)
zephyr_cc_option_fallback(-march=atom -mtune=silvermont)

zephyr_library_sources(soc.c)
