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

if (CONFIG_SOC_NRF53_ANOMALY_160_WORKAROUND_NEEDED AND
    NOT CONFIG_SYS_CLOCK_EXISTS)
  message(WARNING "
    Your application may be affected by the anomaly 160 that concerns the
    nRF5340 SoC. The related workaround cannot be applied, because your
    application has the system clock disabled (CONFIG_SYS_CLOCK_EXISTS=n).
    Consider enabling the system clock to apply the workaround.
    " "
    At your own risk, you can suppress this warning by setting
    CONFIG_SOC_NRF53_ANOMALY_160_WORKAROUND_NEEDED=n.")
endif()
