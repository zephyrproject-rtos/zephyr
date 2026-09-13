# SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
    reset.S
    boot_stubs.S
    cpu_idle.c
    switch.S
    syscall_wrapper.S
    isr_wrapper.S
    fatal.c
    irq_manage.c
    prep_c.c
    reboot.c
    thread.c
    vectors.c
)
zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)

zephyr_linker_sources(ROM_START SORT_KEY 0x0reset reset.ld)
