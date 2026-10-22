# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NXP_SPECIFIC_MPU_SETTINGS)
  zephyr_sources(${ZEPHYR_BASE}/boards/nxp/common/mpu/rt11xx_cm7_mpu_regions.c)
endif()
