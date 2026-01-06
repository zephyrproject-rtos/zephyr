# Copyright (C) 2017-2024 Alibaba Group Holding Limited
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
  .
)

zephyr_sources(
  soc_irq.S
)

zephyr_ld_options(-Wl,--no-warn-rwx-segments)
