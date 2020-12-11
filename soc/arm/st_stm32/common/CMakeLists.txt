# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(stm32cube_hal.c)

zephyr_linker_sources_ifdef(CONFIG_STM32_CCM SECTIONS ccm.ld)
