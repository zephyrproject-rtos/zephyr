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
