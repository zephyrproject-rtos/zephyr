zephyr_sources(
  soc.c
  wdog.S
  )
zephyr_sources_ifdef(
  CONFIG_NXP_MPU
  nxp_mpu_regions.c
  )
