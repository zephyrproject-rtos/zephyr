# Copyright (c) 2023 Antmicro <www.antmicro.com>
# Copyright (c) 2025-2026 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
  loader_param.c
  ../common/loader_program.S
)

zephyr_sources_ifdef(CONFIG_ARM_MPU mpu_regions.c)

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/soc/renesas/rz/scripts/linker.ld CACHE INTERNAL "")
