zephyr_library()
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_cc_option(-march=pentium)

if(CONFIG_X86_IAMCU)
  zephyr_cc_option(-msoft-float)
endif()

zephyr_library_sources(soc.c)
