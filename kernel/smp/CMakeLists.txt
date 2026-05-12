# Copyright (c) Zephyr Project contributors
# SPDX-License-Identifier: Apache-2.0

target_sources(kernel PRIVATE
  smp.c
  ipi.c
  )

if(CONFIG_SCHED_CPU_MASK OR CONFIG_SCHED_CPU_MASK_PIN_ONLY)
  target_sources(kernel PRIVATE cpu_mask.c)
endif()
