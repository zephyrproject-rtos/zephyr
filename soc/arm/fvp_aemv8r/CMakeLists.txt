# Copyright (c) 2021 Arm Limited (or its affiliates). All rights reserved.
# SPDX-License-Identifier: Apache-2.0
if(CONFIG_SOC_FVP_AEMV8R_AARCH64)
  zephyr_library_sources(aarch64/soc.c)
  zephyr_library_sources_ifdef(CONFIG_ARM_MPU aarch64/arm_mpu_regions.c)

  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
elseif(CONFIG_SOC_FVP_AEMV8R_AARCH32)
  zephyr_library_sources_ifdef(CONFIG_ARM_MPU aarch32/arm_mpu_regions.c)
  zephyr_library_sources(aarch32/soc.c)
  zephyr_include_directories(aarch32)

  set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
endif()
