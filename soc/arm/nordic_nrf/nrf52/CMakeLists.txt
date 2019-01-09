zephyr_sources(
  soc.c
  )

zephyr_sources_ifdef(CONFIG_SYS_POWER_MANAGEMENT
  power.c
  )

zephyr_sources_ifdef(CONFIG_ARM_MPU
  mpu_regions.c
  )
