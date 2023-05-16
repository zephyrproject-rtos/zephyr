# SPDX-License-Identifier: Apache-2.0

if(CONFIG_CANOPENNODE)

  set(CANOPENNODE_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

  zephyr_library()

  zephyr_include_directories(
    ${CANOPENNODE_DIR}
    ${CANOPENNODE_DIR}/301
    ${CANOPENNODE_DIR}/303
    ${CANOPENNODE_DIR}/304
    ${CANOPENNODE_DIR}/305
    ${CANOPENNODE_DIR}/309
    .
  )

  zephyr_library_sources(
    ${CANOPENNODE_DIR}/CANopen.c
    ${CANOPENNODE_DIR}/301/CO_TIME.c
    ${CANOPENNODE_DIR}/301/CO_SDOserver.c
    ${CANOPENNODE_DIR}/301/CO_SDOclient.c
    ${CANOPENNODE_DIR}/301/CO_Emergency.c
    ${CANOPENNODE_DIR}/301/CO_SYNC.c
    ${CANOPENNODE_DIR}/301/CO_HBconsumer.c
    ${CANOPENNODE_DIR}/301/CO_NMT_Heartbeat.c
    ${CANOPENNODE_DIR}/301/CO_fifo.c
    ${CANOPENNODE_DIR}/301/crc16-ccitt.c
    ${CANOPENNODE_DIR}/301/CO_PDO.c
    ${CANOPENNODE_DIR}/303/CO_LEDs.c
    ${CANOPENNODE_DIR}/304/CO_SRDO.c
    ${CANOPENNODE_DIR}/304/CO_GFC.c
    ${CANOPENNODE_DIR}/305/CO_LSSmaster.c
    ${CANOPENNODE_DIR}/305/CO_LSSslave.c
    ${CANOPENNODE_DIR}/309/CO_gateway_ascii.c
    CO_driver.c
  )

  zephyr_library_sources_ifdef(CONFIG_CANOPENNODE_SYNC_THREAD canopen_sync.c)
  zephyr_library_sources_ifdef(CONFIG_CANOPENNODE_STORAGE canopen_storage.c)
  zephyr_library_sources_ifdef(CONFIG_CANOPENNODE_LEDS_ENABLE canopen_leds.c)
  zephyr_library_sources_ifdef(CONFIG_CANOPENNODE_PROGRAM_DOWNLOAD canopen_program.c)
  zephyr_library_sources_ifdef(CONFIG_CANOPENNODE_CRC16_EXTERNAL canopen_crc16.c)

endif()
