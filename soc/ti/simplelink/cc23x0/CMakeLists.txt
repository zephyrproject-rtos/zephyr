# Copyright (c) 2024,2026 Texas Instruments Incorporated
# Copyright (c) 2024 BayLibre, SAS
# Copyright (c) 2025 Fabian Pflug
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources(ccfg.c)
zephyr_sources(power.c)
zephyr_include_directories(.)

zephyr_linker_sources_ifdef(CONFIG_HAS_TI_CCFG SECTIONS ccfg.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

set_property(GLOBAL APPEND PROPERTY post_build_patch_elf_commands
  # Patch .elf file with correct CRC32 fields
  COMMAND crc_tool patch-image
      --elf ${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}
      --symbol-prefix CRC_CCFG
      --output ${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}
  )
