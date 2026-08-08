# SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_GRAPHIC_STM32_DMA2D graphic_stm32_dma2d.c)
# zephyr-keep-sorted-stop
