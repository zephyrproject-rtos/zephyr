# Copyright (c) 2025, Applied Materials
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_syscall_header(
  ${ZEPHYR_BASE}/include/zephyr/drivers/misc/usb3803/usb3803.h
)

zephyr_library_sources(usb3803.c)
