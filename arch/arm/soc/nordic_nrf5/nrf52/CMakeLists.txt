zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF52832 NRF52832_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF52840 NRF52840_XXAA)

zephyr_sources(
  soc.c
  mpu_regions.c
  )
