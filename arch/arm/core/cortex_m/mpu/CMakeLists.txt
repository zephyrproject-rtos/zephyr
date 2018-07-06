zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ARM_CORE_MPU arm_core_mpu.c)
zephyr_library_sources_if_kconfig(               arm_mpu.c)
zephyr_library_sources_if_kconfig(               nxp_mpu.c)

zephyr_library_include_directories(
  .
  ../../../include/cortex_m
)
