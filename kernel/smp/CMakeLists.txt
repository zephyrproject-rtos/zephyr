# Copyright (c) Zephyr Project contributors
# SPDX-License-Identifier: Apache-2.0

target_sources(kernel PRIVATE
  smp.c
  ipi.c
  )

target_sources_ifdef(CONFIG_SCHED_CPU_MASK kernel PRIVATE
  cpu_mask.c
  )
