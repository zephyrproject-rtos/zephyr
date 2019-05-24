# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PINMUX_INTEL_S1000)
  zephyr_library()
  zephyr_library_sources(pinmux.c)
endif()

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${PYTHON_EXECUTABLE} ${BOARD_DIR}/support/create_board_img.py
  -i ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.bin
  -o ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}_${BOARD}.bin
  -l $<TARGET_FILE:${ZEPHYR_PREBUILT_EXECUTABLE}>
  -sk
)
