zephyr_include_directories(.)
zephyr_sources(
  soc_pmc.c
  soc_gpio.c
  )

zephyr_sources_ifdef(
  CONFIG_ARM_MPU
  arm_mpu_regions.c
  )
