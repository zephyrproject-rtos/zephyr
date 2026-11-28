# Copyright (c) 2026 BayLibre SAS
# SPDX-License-Identifier: Apache-2.0

zephyr_constants_library(
  NAME arm64_efi_constants
  INCLUDES ${ZEPHYR_BASE}/arch/common/include
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/efi_constants.c
)

zephyr_library()
zephyr_library_sources(efi.c efi_stub.S)

zephyr_linker_sources(SECTIONS efi.ld)
zephyr_linker_sources(SECTIONS SORT_KEY z_end efi_pad.ld)
# Only the PE header refers to the entry point, through a linker script symbol
zephyr_link_libraries($<TARGET_PROPERTY:linker,undefined>arm64_efi_entry)

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${CMAKE_COMMAND} -E copy
  ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.bin
  ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.efi
)
set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts
  ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.efi
)
