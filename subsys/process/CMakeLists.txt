# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/process/process.h)

zephyr_library_sources(
  builtin.c
  process.c
)

zephyr_library_sources_ifdef(CONFIG_USERSPACE handlers.c)
zephyr_library_sources_ifdef(CONFIG_LLEXT llext.c)
