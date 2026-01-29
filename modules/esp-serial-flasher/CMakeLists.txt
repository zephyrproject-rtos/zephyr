# Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ESP_SERIAL_FLASHER)

  set(ZEPHYR_CURRENT_LIBRARY esp_serial_flasher)
  set(ESF_DIR ${ZEPHYR_ESP_SERIAL_FLASHER_MODULE_DIR})
  message("ESP Serial Flasher dir: ${ESF_DIR}")

  zephyr_interface_library_named(esp_serial_flasher)
  zephyr_library()

  zephyr_include_directories(
    ${ESF_DIR}/include
    ${ESF_DIR}/private_include
    src
    )

  zephyr_library_sources(
    ${ESF_DIR}/src/esp_loader.c
    ${ESF_DIR}/src/esp_targets.c
    ${ESF_DIR}/src/esp_stubs.c
    ${ESF_DIR}/src/protocol_serial.c
    ${ESF_DIR}/src/protocol_uart.c
    ${ESF_DIR}/src/slip.c
    ${ESF_DIR}/src/md5_hash.c
    src/loader_port.c
    src/common.c
    )

  target_compile_definitions(esp_serial_flasher INTERFACE SERIAL_FLASHER_INTERFACE_UART)

  zephyr_library_link_libraries(esp_serial_flasher)

  if(DEFINED MD5_ENABLED OR CONFIG_SERIAL_FLASHER_MD5_ENABLED)
      target_compile_definitions(esp_serial_flasher INTERFACE -DMD5_ENABLED=1)
  endif()

  if(DEFINED SERIAL_FLASHER_DEBUG_TRACE OR CONFIG_SERIAL_FLASHER_DEBUG_TRACE)
      target_compile_definitions(esp_serial_flasher INTERFACE SERIAL_FLASHER_DEBUG_TRACE=1)
  endif()

  target_compile_definitions(esp_serial_flasher
  INTERFACE
      SERIAL_FLASHER_WRITE_BLOCK_RETRIES=${CONFIG_SERIAL_FLASHER_WRITE_BLOCK_RETRIES}
  )

  if((DEFINED SERIAL_FLASHER_RESET_INVERT AND SERIAL_FLASHER_RESET_INVERT) OR CONFIG_SERIAL_FLASHER_RESET_INVERT)
      target_compile_definitions(esp_serial_flasher INTERFACE SERIAL_FLASHER_RESET_INVERT=1)
  else()
      target_compile_definitions(esp_serial_flasher INTERFACE SERIAL_FLASHER_RESET_INVERT=0)
  endif()

  if((DEFINED SERIAL_FLASHER_BOOT_INVERT AND SERIAL_FLASHER_BOOT_INVERT) OR CONFIG_SERIAL_FLASHER_BOOT_INVERT)
      target_compile_definitions(esp_serial_flasher INTERFACE SERIAL_FLASHER_BOOT_INVERT=1)
  else()
      target_compile_definitions(esp_serial_flasher INTERFACE SERIAL_FLASHER_BOOT_INVERT=0)
  endif()

endif()
