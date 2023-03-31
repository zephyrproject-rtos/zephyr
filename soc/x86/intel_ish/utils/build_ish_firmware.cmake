# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/build_ish_firmware.py
  ARGS -k ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.bin
  -o ${PROJECT_BINARY_DIR}/ish_fw.bin
)
