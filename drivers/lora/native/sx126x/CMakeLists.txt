# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LORA_SX126X_NATIVE
  sx126x.c
  sx126x_hal.c
)
