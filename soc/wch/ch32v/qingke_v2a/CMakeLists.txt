# Copyright (c) 2024 Michael Hope
# Copyright (c) 2024 Jianxiong Gu
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc_idle.c
  soc_irq.S
  vector.S
)

zephyr_include_directories(.)
