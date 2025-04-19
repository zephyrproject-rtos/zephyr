if(CONFIG_SOC_CH32V003 OR CONFIG_SOC_CH32V208)
  zephyr_include_directories(${ZEPHYR_HAL_WCH_MODULE_DIR}/ch32v003fun .)
endif()
