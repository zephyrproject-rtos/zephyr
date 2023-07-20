# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  soc.c
  )

zephyr_library_sources_ifdef(CONFIG_PM
  power.c
  )

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/arm/include
  )

if(CONFIG_SOC_NRF52832_ALLOW_SPIM_DESPITE_PAN_58 AND CONFIG_SPI_NRFX_SPIM)
  message(WARNING "Both SOC_NRF52832_ALLOW_SPIM_DESPITE_PAN_58 and an NRF SPIM driver are enabled, therefore PAN 58 will apply if RXD.MAXCNT == 1 and TXD.MAXCNT <= 1")
endif()
