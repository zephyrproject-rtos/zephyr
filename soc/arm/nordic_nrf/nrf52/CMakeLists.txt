zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF52810 NRF52810_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF52832 NRF52832_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF52840 NRF52840_XXAA)

zephyr_sources(
  power.c
  soc.c
  )

zephyr_sources_ifdef(CONFIG_ARM_MPU_NRF52X mpu_regions.c)
