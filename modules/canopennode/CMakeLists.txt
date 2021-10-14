# SPDX-License-Identifier: Apache-2.0

if(CONFIG_CANOPENNODE)

  set(CANOPENNODE_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

  zephyr_library()

  zephyr_include_directories(
    ${CANOPENNODE_DIR}
    ${CANOPENNODE_DIR}/stack
    .
  )

  zephyr_library_sources(
    ${CANOPENNODE_DIR}/CANopen.c
    ${CANOPENNODE_DIR}/stack/CO_Emergency.c
    ${CANOPENNODE_DIR}/stack/CO_HBconsumer.c
    ${CANOPENNODE_DIR}/stack/CO_LSSmaster.c
    ${CANOPENNODE_DIR}/stack/CO_LSSslave.c
    ${CANOPENNODE_DIR}/stack/CO_NMT_Heartbeat.c
    ${CANOPENNODE_DIR}/stack/CO_PDO.c
    ${CANOPENNODE_DIR}/stack/CO_SDO.c
    ${CANOPENNODE_DIR}/stack/CO_SDOmaster.c
    ${CANOPENNODE_DIR}/stack/CO_SYNC.c
    ${CANOPENNODE_DIR}/stack/CO_TIME.c
    ${CANOPENNODE_DIR}/stack/CO_trace.c
    CO_driver.c
  )

  zephyr_library_sources_ifdef(CONFIG_CANOPENNODE_SYNC_THREAD canopen_sync.c)
  zephyr_library_sources_ifdef(CONFIG_CANOPENNODE_STORAGE canopen_storage.c)
  zephyr_library_sources_ifdef(CONFIG_CANOPENNODE_LEDS canopen_leds.c)
  zephyr_library_sources_ifdef(CONFIG_CANOPENNODE_PROGRAM_DOWNLOAD canopen_program.c)

endif()
