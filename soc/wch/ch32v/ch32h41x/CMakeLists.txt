# Copyright (c) 2026 Liu Changjie
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc_irq.S
  vector.S
)

zephyr_sources_ifdef(CONFIG_SOC_CH32H417_V3F soc_v3f.c)

zephyr_include_directories(.)
