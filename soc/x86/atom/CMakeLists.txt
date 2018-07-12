target_include_directories(soc_x86 PRIVATE ${ZEPHYR_BASE}/drivers)

zephyr_cc_option(-march=atom)
zephyr_cc_option_fallback(-mtune=atom -mtune=generic)

target_sources(soc_x86 PRIVATE ${CMAKE_CURRENT_LIST_DIR}/soc.c)
