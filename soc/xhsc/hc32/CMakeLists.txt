#
# Copyright (C) 2022-2025, Xiaohua Semiconductor Co., Ltd.
#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(common)
add_subdirectory(${SOC_SERIES})

# This is for access to pinctrl macros
#zephyr_include_directories(common)

zephyr_linker_sources(
  ROM_START
  SORT_KEY ${CONFIG_HC32_FLASH_CONFIG_OFFSET}
  flash_config.ld
  )

zephyr_linker_sources(
	RAM_SECTIONS
	SORT_KEY _noinit_user
	user_noinit.ld
  )