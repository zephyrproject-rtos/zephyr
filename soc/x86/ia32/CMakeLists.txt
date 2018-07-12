target_include_directories(soc_x86 PRIVATE ${ZEPHYR_BASE}/drivers)

zephyr_cc_option(-march=pentium)

if(CONFIG_X86_IAMCU)
  zephyr_cc_option(-msoft-float)
endif()

target_sources(soc_x86 PRIVATE ${CMAKE_CURRENT_LIST_DIR}/soc.c)
