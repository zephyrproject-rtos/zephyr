# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_RTC_STM32           rtc_stm32.c)

zephyr_library_sources_ifdef(CONFIG_USERSPACE   rtc_handlers.c)
