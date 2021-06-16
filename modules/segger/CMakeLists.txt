if(CONFIG_USE_SEGGER_RTT)

  zephyr_library()
  set(SEGGER_DIR ${ZEPHYR_CURRENT_MODULE_DIR})
  zephyr_include_directories_ifdef(CONFIG_USE_SEGGER_RTT
    ${SEGGER_DIR}/SEGGER
    ${SEGGER_DIR}/Config
    .
    )
  zephyr_library_sources(
    ${SEGGER_DIR}/SEGGER/SEGGER_RTT.c
    SEGGER_RTT_zephyr.c
    )
  zephyr_library_sources_ifdef(CONFIG_SEGGER_SYSTEMVIEW ${SEGGER_DIR}/SEGGER/SEGGER_SYSVIEW.c)
endif()
