# Copyright (c) 2025-2026, Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
    cpu_idle.c
    exc.S
    fault.c
    irq_manage.c
    reboot.c
    reset.S
    stacks.c
    thread.c
    prep_c.c
    vector_table.S
  )

zephyr_library_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE isr_wrapper.S)
zephyr_library_sources_ifdef(CONFIG_USE_SWITCH switch.S)
