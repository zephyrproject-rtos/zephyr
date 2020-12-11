# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  vector_table.S
  reset.S
  exc.S
  exc_exit.S
  fault.c
  irq_init.c
  reboot.c
  stacks.c
  )
