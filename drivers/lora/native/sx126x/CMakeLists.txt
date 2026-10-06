# Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(../sx12xx_common)

zephyr_library_sources_ifdef(CONFIG_LORA_SX126X_NATIVE
  sx126x.c
)

zephyr_library_sources_ifdef(CONFIG_LORA_GFSK
  sx126x_gfsk.c
)

zephyr_library_sources_ifdef(CONFIG_LORA_SX126X_NATIVE_STANDALONE
  sx126x_hal.c
)

zephyr_library_sources_ifdef(CONFIG_LORA_SX126X_NATIVE_STM32WL
  sx126x_hal_stm32wl.c
)
