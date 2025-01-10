if(CONFIG_USE_SEGGER_RTT)
  zephyr_library()
  zephyr_library_compile_definitions(SEGGER_RTT_ALIGNMENT=CONFIG_SEGGER_RTT_CB_ALIGNMENT)
  set(SEGGER_DIR ${ZEPHYR_CURRENT_MODULE_DIR})
  zephyr_include_directories_ifdef(CONFIG_USE_SEGGER_RTT
    ${SEGGER_DIR}/SEGGER
    ${SEGGER_DIR}/Config
  )
  zephyr_library_sources(
    ${SEGGER_DIR}/SEGGER/SEGGER_RTT.c
    SEGGER_RTT_zephyr.c
  )
  zephyr_library_sources_ifdef(CONFIG_SEGGER_SYSTEMVIEW ${SEGGER_DIR}/SEGGER/SEGGER_SYSVIEW.c)
  # Using sort key AAA to ensure that we are placed at start of RAM
  zephyr_linker_sources_ifdef(CONFIG_SEGGER_RTT_SECTION_CUSTOM RAM_SECTIONS SORT_KEY aaa segger_rtt.ld)
endif()

if(CONFIG_SEGGER_DEBUGMON)
  if(!CONFIG_USE_SEGGER_RTT)
    zephyr_library()
  endif()

  set(SEGGER_DIR ${ZEPHYR_CURRENT_MODULE_DIR})
  zephyr_include_directories_ifdef(CONFIG_SEGGER_DEBUGMON
    ${SEGGER_DIR}/SEGGER/DebugMon/include
  )
  zephyr_library_sources(
    ${SEGGER_DIR}/SEGGER/DebugMon/JLINK_MONITOR.c
    ${SEGGER_DIR}/SEGGER/DebugMon/JLINK_MONITOR_ISR_SES.s
  )
endif()
