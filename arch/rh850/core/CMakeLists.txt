# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  cpu_idle.c
  exc_entry.S
  fatal.c
  irq_manage.c
  isr_wrapper.S
  prep_c.c
  reboot.c
  reset.S
  switch.S
  thread.c
  vectors.S
)

zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)
zephyr_library_sources_ifdef(CONFIG_FXU fxu.c fxu.S)

zephyr_linker_sources(ROM_START SORT_KEY 0x0vectors vectors.ld)
