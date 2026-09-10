# Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_LORA_LR11XX_NATIVE lr11xx)
add_subdirectory_ifdef(CONFIG_LORA_SX126X_NATIVE sx126x)
# zephyr-keep-sorted-stop
