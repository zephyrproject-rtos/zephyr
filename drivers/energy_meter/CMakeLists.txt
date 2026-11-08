# Copyright (c) 2025 Pedro Ramos
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/energy_meter.h)

zephyr_library()

zephyr_library_sources(energy_meter_common.c)
zephyr_library_sources_ifdef(CONFIG_ENERGY_METER_SHELL energy_meter_shell.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE energy_meter_syscall_handlers.c)

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_ATM90E32AS atm90e32as)
# zephyr-keep-sorted-stop
