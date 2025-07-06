# Copyright (c) 2021,2025 Henrik Brix Andersen <henrik@brixandersen.dk>
# SPDX-License-Identifier: Apache-2.0

if((CONFIG_BOARD_NEORV32) AND (CONFIG_BUILD_OUTPUT_BIN))
  # Generate NEORV32 image formats for initialising IMEM.
  find_program(IMAGE_GEN image_gen)

  if(NOT ${IMAGE_GEN} STREQUAL IMAGE_GEN-NOTFOUND)
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${IMAGE_GEN}
      ARGS -app_bin
      ${CONFIG_KERNEL_BIN_NAME}.bin
      ${CONFIG_KERNEL_BIN_NAME}_exe.bin
      ${PROJECT_BINARY_DIR}
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      )
    message(STATUS "neorv32 binary will be written to: ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}_exe.bin")

    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${IMAGE_GEN}
      ARGS -app_vhd
      ${CONFIG_KERNEL_BIN_NAME}.bin
      ${CONFIG_KERNEL_BIN_NAME}.vhd
      ${PROJECT_BINARY_DIR}
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      )
    message(STATUS "neorv32 VHDL will be written to: ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.vhd")
  else()
    message(STATUS "The neorv32 image_gen utility was not found, neorv32 image files cannot be generated")
  endif()
endif()
