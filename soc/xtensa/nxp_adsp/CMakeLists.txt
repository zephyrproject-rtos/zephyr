# NXP i.MX8 SoC family CMake file
#
# Copyright (c) 2021 NXP
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(common)

#  west sign

# See detailed comments in soc/xtensa/intel_adsp/common/CMakeLists.txt
# where most of this code is for now duplicated.

add_custom_target(zephyr.ri ALL
  DEPENDS ${CMAKE_BINARY_DIR}/zephyr/zephyr.ri
)

# sign.py supports `zephyr.elf` only. Copy/normalize any weird
# ${CONFIG_KERNEL_BIN_NAME}.elf to zephyr.elf if different.
if(NOT CONFIG_KERNEL_BIN_NAME STREQUAL "zephyr")
  add_custom_command(OUTPUT  ${CMAKE_BINARY_DIR}/zephyr/zephyr.elf
    DEPENDS ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
    ${CMAKE_BINARY_DIR}/zephyr/zephyr.elf
)
endif()

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/zephyr/zephyr.ri
  COMMENT "west sign --if-tool-available --tool rimage ..."
  COMMAND  west sign --if-tool-available --tool rimage --build-dir ${CMAKE_BINARY_DIR}
  DEPENDS ${CMAKE_BINARY_DIR}/zephyr/zephyr.elf
)
