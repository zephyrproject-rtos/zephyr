# Copyright (c) 2026 BayLibre SAS
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(led_player.c)

if(CONFIG_STATUS_LED)
  zephyr_library_sources(status_led.c)
  zephyr_linker_sources(DATA_SECTIONS status_led.ld)
  zephyr_iterable_section(NAME status_led GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT})
endif()
