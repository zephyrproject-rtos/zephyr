# NXP i.MX8 SoC family CMake file
#
# Copyright (c) 2021 NXP
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(common)

#  west sign

# See detailed comments in soc/xtensa/intel_adsp/common/CMakeLists.txt
add_custom_target(zephyr.ri ALL
  DEPENDS ${CMAKE_BINARY_DIR}/zephyr/zephyr.ri
)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/zephyr/zephyr.ri
  COMMENT "west sign --if-tool-available --tool rimage ..."
  COMMAND  west sign --if-tool-available --tool rimage --build-dir ${CMAKE_BINARY_DIR} ${WEST_SIGN_OPTS}
  DEPENDS ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
)
