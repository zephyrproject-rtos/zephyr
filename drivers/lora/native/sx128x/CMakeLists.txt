# Copyright (c) 2026 Giuseppe Fabiano <gfabiano40@gmail.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(../common)
zephyr_library_sources(
  sx128x.c
  sx128x_hal.c
  sx128x_spi.c
)

zephyr_library_sources_ifdef(CONFIG_LORA_GFSK
  sx128x_gfsk.c
)
