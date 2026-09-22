set(SI32_HAL_DIR ${ZEPHYR_HAL_SILABS_MODULE_DIR}/si32/si32Hal)

zephyr_include_directories(
  ${SI32_HAL_DIR}/sim3u1xx
  ${SI32_HAL_DIR}/SI32_Modules
)

if(CONFIG_SOC_SERIES_SIM3U)
  zephyr_library_sources(${SI32_HAL_DIR}/sim3u1xx/SI32_PBCFG_A_Type.c)
  zephyr_library_sources_ifdef(CONFIG_DMA ${SI32_HAL_DIR}/sim3u1xx/SI32_DMAXBAR_A_Type.c)
  zephyr_library_sources_ifdef(CONFIG_DMA ${SI32_HAL_DIR}/SI32_Modules/SI32_DMADESC_A_Type.c)
  zephyr_library_sources_ifdef(CONFIG_SPI ${SI32_HAL_DIR}/SI32_Modules/SI32_SPI_A_Type.c)
endif()
