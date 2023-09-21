# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_PM)
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${CMAKE_OBJCOPY} -O binary --remove-section=aon
      ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME} ${PROJECT_BINARY_DIR}/ish_kernel.bin

  COMMAND ${CMAKE_OBJCOPY} -O binary --only-section=aon
      ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME} ${PROJECT_BINARY_DIR}/ish_aon.bin

  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/build_ish_firmware.py
  -k ${PROJECT_BINARY_DIR}/ish_kernel.bin
  -a ${PROJECT_BINARY_DIR}/ish_aon.bin
  -o ${PROJECT_BINARY_DIR}/ish_fw.bin
)
else()
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/build_ish_firmware.py
  ARGS -k ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.bin
  -o ${PROJECT_BINARY_DIR}/ish_fw.bin
)
endif()
