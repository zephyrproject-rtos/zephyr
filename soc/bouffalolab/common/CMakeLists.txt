# Copyright (c) 2021-2024 ATL Electronics
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_RISCV_PRIVILEGED
  soc_common_irq_privileged.c
  )

zephyr_sources_ifndef(CONFIG_RISCV_PRIVILEGED
  soc_irq.S
  soc_common_irq.c
  vector.S
  )
