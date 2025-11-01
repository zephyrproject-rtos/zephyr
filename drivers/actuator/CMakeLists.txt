# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/actuator.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USERSPACE actuator_handlers.c)

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_ACTUATOR_FAKE actuator_fake.c)
# zephyr-keep-sorted-stop
