# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_include_directories(${ZEPHYR_BASE}/soc/st/stm32/common)

zephyr_sources_ifdef(CONFIG_STM32_GPIOPORT_MGR
  ${ZEPHYR_BASE}/soc/st/stm32/common/gpioport_mgr.c
)

zephyr_sources(stm32cube_hal.c)
