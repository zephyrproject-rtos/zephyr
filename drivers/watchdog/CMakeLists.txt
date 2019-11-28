# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_IWDG_STM32 wdt_iwdg_stm32.c)
zephyr_sources_ifdef(CONFIG_WWDG_STM32 wdt_wwdg_stm32.c)
zephyr_sources_ifdef(CONFIG_WDOG_CMSDK_APB wdt_cmsdk_apb.c)
zephyr_sources_ifdef(CONFIG_WDT_SAM wdt_sam.c)
zephyr_sources_ifdef(CONFIG_WDT_ESP32 wdt_esp32.c)
zephyr_sources_ifdef(CONFIG_WDT_SAM0 wdt_sam0.c)
zephyr_sources_ifdef(CONFIG_WDT_NRFX wdt_nrfx.c)
zephyr_sources_ifdef(CONFIG_WDT_MCUX_WDOG wdt_mcux_wdog.c)
zephyr_sources_ifdef(CONFIG_WDT_MCUX_WDOG32 wdt_mcux_wdog32.c)
zephyr_sources_ifdef(CONFIG_WDT_XEC wdt_mchp_xec.c)
