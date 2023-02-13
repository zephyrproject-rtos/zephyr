# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(board.c)

if((CONFIG_BOARD_ARTY_A7_ARM_DESIGNSTART_M1) AND (CONFIG_BUILD_OUTPUT_BIN))
  # Generate zephyr.mem verilog memory hex dump file for initialising ITCM in
  # Xilinx Vivado.
  #
  # This ought to be done using the objcopy verilog bfd, but it contains a bug
  # affecting endianness: https://sourceware.org/bugzilla/show_bug.cgi?id=25202
  #
  # Instead we use bin2hex from the SiFive elf2hex package, if available.
  # https://github.com/sifive/elf2hex
  find_program(BIN2HEX ${CROSS_COMPILE_TARGET}-bin2hex)

  if(NOT ${BIN2HEX} STREQUAL BIN2HEX-NOTFOUND)
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${BIN2HEX}
      ARGS --bit-width 32
      ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.bin
      ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.mem
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      )
    message(STATUS "Verilog memory hex dump will be written to: ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.mem")
  else()
    message(STATUS "The bin2hex (${CROSS_COMPILE_TARGET}-bin2hex) utility was not found, verilog memory hex dump file cannot be generated")
  endif()
endif()
