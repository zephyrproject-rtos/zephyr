# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0
#
# Generate an eFuse image for Espressif QEMU.
#
# Everything generated lands in the build tree: the source tree is never
# written to, so read-only checkouts and concurrent builds of different boards
# keep working. A caller can point ESPRESSIF_QEMU_EFUSE_HEX_FILE at a hex file
# of its own; otherwise ESP32 defaults to ECO3 and other machines stay empty
# (no eFuse drive attached).
#
# ESP32 ECO3 layout:
# https://github.com/espressif/esp-toolchain-docs/blob/main/qemu/esp32/README.md#emulating-esp32-eco3

if(NOT PYTHON_EXECUTABLE)
  message(FATAL_ERROR
    "Espressif QEMU: PYTHON_EXECUTABLE is required to generate an eFuse image")
endif()

set(ESPRESSIF_QEMU_EFUSE_HEX
  ${CMAKE_BINARY_DIR}/zephyr/qemu_efuse_${ESPRESSIF_QEMU_MACHINE}.hex
)

# A caller can point ESPRESSIF_QEMU_EFUSE_HEX_FILE at a hex file of its own.
if(ESPRESSIF_QEMU_EFUSE_HEX_FILE)
  if(NOT EXISTS ${ESPRESSIF_QEMU_EFUSE_HEX_FILE})
    message(FATAL_ERROR
      "Espressif QEMU: ESPRESSIF_QEMU_EFUSE_HEX_FILE does not exist: "
      "${ESPRESSIF_QEMU_EFUSE_HEX_FILE}")
  endif()
  configure_file(
    ${ESPRESSIF_QEMU_EFUSE_HEX_FILE}
    ${ESPRESSIF_QEMU_EFUSE_HEX}
    COPYONLY
  )
else()
  # ESP32 defaults to ECO3; an empty file elsewhere means "do not attach eFuse
  # storage".
  if(NOT DEFINED ESPRESSIF_QEMU_EFUSE_HEX_CONTENT)
    if(CONFIG_SOC_SERIES_ESP32)
      # Plain hex (no whitespace), 248 digits / 124 bytes.
      string(CONCAT ESPRESSIF_QEMU_EFUSE_HEX_CONTENT
        "00000000000000000000000000800000000000000000100000000000000000"
        "00000000000000000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000000000000000"
      )
    else()
      set(ESPRESSIF_QEMU_EFUSE_HEX_CONTENT "")
    endif()
  endif()
  file(WRITE ${ESPRESSIF_QEMU_EFUSE_HEX}
    "${ESPRESSIF_QEMU_EFUSE_HEX_CONTENT}")
endif()

file(READ ${ESPRESSIF_QEMU_EFUSE_HEX} ESPRESSIF_QEMU_EFUSE_HEX_CONTENT)
string(REGEX REPLACE "[ \t\r\n]" ""
  ESPRESSIF_QEMU_EFUSE_HEX_CONTENT "${ESPRESSIF_QEMU_EFUSE_HEX_CONTENT}")

# Empty files mean "do not attach eFuse storage".
if(NOT ESPRESSIF_QEMU_EFUSE_HEX_CONTENT)
  return()
endif()

if(NOT ESPRESSIF_QEMU_EFUSE_HEX_CONTENT MATCHES "^[0-9A-Fa-f]+$")
  message(FATAL_ERROR
    "Espressif QEMU: eFuse hex contains non-hex characters "
    "(see ${ESPRESSIF_QEMU_EFUSE_HEX})")
endif()
string(LENGTH "${ESPRESSIF_QEMU_EFUSE_HEX_CONTENT}" _esp_qemu_efuse_hex_len)
math(EXPR _esp_qemu_efuse_hex_remainder "${_esp_qemu_efuse_hex_len} % 2")
if(_esp_qemu_efuse_hex_remainder)
  message(FATAL_ERROR
    "Espressif QEMU: eFuse hex has an odd number of hex digits "
    "(see ${ESPRESSIF_QEMU_EFUSE_HEX})")
endif()

if(CONFIG_SOC_SERIES_ESP32)
  set(ESPRESSIF_QEMU_EFUSE_BIN
    ${CMAKE_BINARY_DIR}/zephyr/qemu_efuse_eco3.bin
  )
  set(ESPRESSIF_QEMU_EFUSE_SIZE 124)
else()
  set(ESPRESSIF_QEMU_EFUSE_BIN
    ${CMAKE_BINARY_DIR}/zephyr/qemu_efuse_${ESPRESSIF_QEMU_MACHINE}.bin
  )
  set(ESPRESSIF_QEMU_EFUSE_SIZE 1024)
endif()

math(EXPR _esp_qemu_efuse_size "${_esp_qemu_efuse_hex_len} / 2")
if(_esp_qemu_efuse_size GREATER ESPRESSIF_QEMU_EFUSE_SIZE)
  message(FATAL_ERROR
    "Espressif QEMU: eFuse hex exceeds ${ESPRESSIF_QEMU_EFUSE_SIZE} bytes "
    "(see ${ESPRESSIF_QEMU_EFUSE_HEX})")
endif()

# QEMU may rewrite the backing file at runtime. Regenerate every build from
# the hex (same pattern as cmake/emu/qemu/nvme.cmake) so a dirty or overwritten
# .bin cannot stick around across rebuilds.
string(CONCAT _esp_qemu_efuse_py
  "from pathlib import Path; p=Path; "
  "data=bytes.fromhex(''.join("
  "p(r'${ESPRESSIF_QEMU_EFUSE_HEX}').read_text().split())); "
  "p(r'${ESPRESSIF_QEMU_EFUSE_BIN}').write_bytes("
  "data.ljust(${ESPRESSIF_QEMU_EFUSE_SIZE}, b'\\0'))"
)
add_custom_target(espressif_qemu_efuse_image ALL
  COMMAND ${PYTHON_EXECUTABLE} -c "${_esp_qemu_efuse_py}"
  BYPRODUCTS ${ESPRESSIF_QEMU_EFUSE_BIN}
  COMMENT "Espressif QEMU: generating ${ESPRESSIF_QEMU_MACHINE} eFuse image"
  VERBATIM
)

list(APPEND ESPRESSIF_QEMU_EXTRA_FLAGS
  -drive "file=${ESPRESSIF_QEMU_EFUSE_BIN},if=none,format=raw,id=efuse"
  -global "driver=${ESPRESSIF_QEMU_EFUSE_DRIVER},property=drive,value=efuse"
)
