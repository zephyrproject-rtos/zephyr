# Copyright (c) 2021-2025 ATL Electronics
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(
  soc_irq.S
  soc_common_irq.c
  vector.S
  )
