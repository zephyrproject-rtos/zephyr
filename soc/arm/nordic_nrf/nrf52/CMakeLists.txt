# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
  )

zephyr_sources_ifdef(CONFIG_SYS_POWER_MANAGEMENT
  power.c
  )

zephyr_sources_ifdef(CONFIG_ARM_MPU
  mpu_regions.c
  )

if(CONFIG_SOC_NRF52832)
  if(CONFIG_SOC_NRF52832_ALLOW_SPIM_DESPITE_PAN_58)
    if(CONFIG_SPI_0_NRF_SPIM OR CONFIG_SPI_1_NRF_SPIM OR CONFIG_SPI_2_NRF_SPIM)
      message(WARNING "Both SOC_NRF52832_ALLOW_SPIM_DESPITE_PAN_58 and an NRF SPIM driver are enabled, therefore PAN 58 will apply if RXD.MAXCNT == 1 and TXD.MAXCNT <= 1")
    endif()
  endif()
endif()
