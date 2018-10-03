zephyr_sources(
  soc.c
  wdog.S
  )
zephyr_sources_ifdef(
  CONFIG_ARM_MPU
  nxp_mpu_regions.c
  )
