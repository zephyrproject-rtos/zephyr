zephyr_sources(
  soc.c
  wdog.S
  )
zephyr_sources_ifdef(
  CONFIG_HAS_SYSMPU
  nxp_mpu_regions.c
  )
