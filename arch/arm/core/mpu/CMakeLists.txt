# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(                             arm_core_mpu.c)
zephyr_library_sources_ifdef(CONFIG_CPU_HAS_ARM_MPU arm_mpu.c)
zephyr_library_sources_ifdef(CONFIG_CPU_HAS_NXP_MPU nxp_mpu.c)

if (CONFIG_CPU_AARCH32_CORTEX_R)
zephyr_library_include_directories(cortex_a_r)
elseif (CONFIG_CPU_CORTEX_M)
zephyr_library_include_directories(cortex_m)
else ()
message(FATAL_ERROR "CPU is not Cortex-A/R/M")
endif ()
