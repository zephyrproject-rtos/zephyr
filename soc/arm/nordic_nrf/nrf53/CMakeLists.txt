# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
  )

zephyr_library_sources_ifdef(CONFIG_PM
  power.c
  )

zephyr_library_sources_ifdef(CONFIG_NRF53_SYNC_RTC
  sync_rtc.c
  )
