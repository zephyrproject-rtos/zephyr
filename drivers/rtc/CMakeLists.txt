# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_RTC_STM32         rtc_hal_stm32u5.c)

