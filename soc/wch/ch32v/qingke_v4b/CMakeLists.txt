# Copyright (c) 2024 Michael Hope
# Copyright (c) 2024 Jianxiong Gu
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  ../common/qingke_v4/soc_irq.S
  ../common/qingke_v4/vector.S
)

zephyr_include_directories(../common/qingke_v4)
