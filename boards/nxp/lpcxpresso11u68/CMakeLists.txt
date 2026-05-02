# SPDX-License-Identifier: Apache-2.0

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  # Insert checksum (verified by the bootloader) into the zephyr.bin
  # and zephyr.hex images.
  COMMAND lpc_checksum -f hex ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_HEX_NAME}
  COMMAND lpc_checksum -f bin ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_BIN_NAME}
  )
