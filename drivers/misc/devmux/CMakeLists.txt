# Copyright (c) 2023, Meta
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_syscall_header(
  ${ZEPHYR_BASE}/include/zephyr/drivers/misc/devmux/devmux.h
)

zephyr_library_sources(devmux.c)
