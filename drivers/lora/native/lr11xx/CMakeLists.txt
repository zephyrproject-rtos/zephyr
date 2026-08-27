# Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LORA_LR11XX_NATIVE
  lr11xx.c
  lr11xx_hal.c
)
