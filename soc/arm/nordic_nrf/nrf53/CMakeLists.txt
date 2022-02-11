# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(
  soc.c
  )

zephyr_library_sources_ifdef(CONFIG_PM
  power.c
  )

zephyr_library_sources_ifdef(CONFIG_NRF53_SYNC_RTC
  sync_rtc.c
  )

if (CONFIG_BUILD_WITH_TFM)
  zephyr_library_link_libraries(tfm_api)
endif()
