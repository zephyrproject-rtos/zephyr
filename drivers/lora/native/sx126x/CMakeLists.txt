# Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LORA_SX126X_NATIVE
  sx126x.c
  sx126x_hal_common.c
)

zephyr_library_sources_ifdef(CONFIG_LORA_SX126X_NATIVE_STANDALONE
  sx126x_hal.c
)

zephyr_library_sources_ifdef(CONFIG_LORA_SX126X_NATIVE_STM32WL
  sx126x_hal_stm32wl.c
)
