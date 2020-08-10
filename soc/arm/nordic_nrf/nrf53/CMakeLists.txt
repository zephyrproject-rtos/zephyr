# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
  )

zephyr_library_sources_ifdef(CONFIG_SYS_POWER_MANAGEMENT
  power.c
  )

zephyr_library_sources_ifdef(CONFIG_NRF5340_ENABLE_CPUNET
  nrf5340_cpunet_reset.c
  )
