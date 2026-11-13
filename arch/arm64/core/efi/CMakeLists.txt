# Copyright (c) 2026 BayLibre SAS
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(efi_entry.S efi_stub.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/arch/common/include)

# The stub runs before the image is at its link address: all symbol references
# must be PC-relative, and nothing may depend on Zephyr state such as the
# stack protector canary. UEFI may use x18 as its platform register.
zephyr_library_compile_options(-mcmodel=small -fno-stack-protector -ffixed-x18)

zephyr_linker_sources(SECTIONS efi.ld)
# Only the PE header refers to the entry point, through a linker script symbol
zephyr_link_libraries($<TARGET_PROPERTY:linker,undefined>arm64_efi_entry)

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${CMAKE_COMMAND} -E copy
  ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.bin
  ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.efi
)
