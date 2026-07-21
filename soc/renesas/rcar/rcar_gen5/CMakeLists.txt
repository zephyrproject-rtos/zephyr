# Copyright (c) 2026 BayLibre, SAS
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_R8A78000_R52)
  zephyr_include_directories(r52)
  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
endif()

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND
  ${CMAKE_OBJCOPY} -O srec --srec-forceS3
  ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.elf
  ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.srec
)
