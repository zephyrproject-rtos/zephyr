# Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
# Copyright 2024-2025 openEuler SIG-Zephyr
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  cpu.c
  mmu.c
  timer.c
  switch.c
  hyp_entry.S
  hyp_vector.S
)


