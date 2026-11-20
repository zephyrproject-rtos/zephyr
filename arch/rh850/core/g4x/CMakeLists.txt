# Copyright (c) 2026 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  reset.S
  switch.S
  vector_table.S
  fatal_s.S
  thread.c
  irq_manage.c
  isr_wrapper.S
  prep_c.c
  irq_offload.c
  reboot.c
  fatal.c
)
