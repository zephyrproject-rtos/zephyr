# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ARM_MPU AND NOT CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_MPU_REGIONS)

  zephyr_library_named(soc_cortex_m)

  zephyr_library_sources_ifdef(CONFIG_CPU_HAS_ARM_MPU
    arm_mpu_regions.c
    )
  zephyr_library_sources_ifdef(CONFIG_CPU_HAS_NXP_MPU
    nxp_mpu_regions.c
    )
endif()
