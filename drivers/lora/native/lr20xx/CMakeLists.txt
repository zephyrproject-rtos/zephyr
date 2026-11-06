# Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LORA_LR20XX_NATIVE
  lr20xx.c
  lr20xx_hal.c
)
