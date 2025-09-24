# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(soc.c)
zephyr_include_directories(.)

if(CONFIG_SOC_NRF52832)
  if(NOT CONFIG_NRF52_ANOMALY_109_WORKAROUND)
    if (CONFIG_NRFX_SPIS OR CONFIG_NRFX_SPIM OR CONFIG_NRFX_TWIM OR CONFIG_NRFX_PWM)
      message(WARNING "NRF52_ANOMALY_109_WORKAROUND disabled with SPIS, SPIM, TWIM or PWM enabled. This will occasionally cause the first byte transmitted to be incorrect")
    endif()
  endif()
endif()
