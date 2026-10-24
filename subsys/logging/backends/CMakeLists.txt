# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_ADSP
  log_backend_adsp.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_ADSP_MTRACE
  log_backend_adsp_mtrace.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_BLE
  log_backend_ble.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_EFI_CONSOLE
  log_backend_efi_console.c
)

if(CONFIG_LOG_BACKEND_FLASH)
  if("${CONFIG_LOG_BACKEND_FLASH_BUILD_ID}" STREQUAL "")
    message(FATAL_ERROR
      "CONFIG_LOG_BACKEND_FLASH_BUILD_ID is empty. Set it to a C expression "
      "identifying this firmware: a stored log can only be decoded against the "
      "build that wrote it, so the backend has to be able to tell one build "
      "from another. Set it to NULL to store no build id and make no check.")
  endif()

  if("${CONFIG_LOG_BACKEND_FLASH_BUILD_ID}" MATCHES "[\"' ]")
    message(FATAL_ERROR
      "CONFIG_LOG_BACKEND_FLASH_BUILD_ID reaches the compiler as source, so it "
      "cannot contain quotes or spaces. Define a macro in the header named by "
      "CONFIG_LOG_BACKEND_FLASH_BUILD_ID_HEADER and name that macro here.")
  endif()

  zephyr_sources(log_backend_flash.c)
  zephyr_sources_ifdef(CONFIG_LOG_BACKEND_FLASH_SHELL log_backend_flash_shell.c)

  # The Kconfig value is C source, not a string, so it is handed to the
  # compiler as a definition rather than used through autoconf.h.
  zephyr_compile_definitions(
    LOG_BACKEND_FLASH_BUILD_ID=${CONFIG_LOG_BACKEND_FLASH_BUILD_ID})

  if(NOT "${CONFIG_LOG_BACKEND_FLASH_BUILD_ID_HEADER}" STREQUAL "")
    zephyr_compile_definitions(LOG_BACKEND_FLASH_BUILD_ID_HAS_HEADER)
  endif()
endif()

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_FS
  log_backend_fs.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_MQTT
  log_backend_mqtt.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_NATIVE_POSIX
  log_backend_native_posix.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_NET
  log_backend_net.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_WS
  log_backend_ws.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_RTT
  log_backend_rtt.c
)

zephyr_include_directories_ifdef(
  CONFIG_LOG_BACKEND_SPINEL
  ${ZEPHYR_BASE}/subsys/net/lib/openthread/platform/
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_SPINEL
  log_backend_spinel.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_SWO
  log_backend_swo.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_UART
  log_backend_uart.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_XTENSA_SIM
  log_backend_xtensa_sim.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_MULTIDOMAIN_BACKEND
  log_multidomain_backend.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_IPC_SERVICE
  log_backend_ipc_service.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_SEMIHOST
  log_backend_semihost.c
)
