# Copyright (c) 2026 Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  reboot.c
  soc_irq.S
  vector.S
)

zephyr_include_directories(.)
