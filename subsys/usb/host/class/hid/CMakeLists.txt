# Copyright (c) 2026 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/usb/class/usbh_hid.h)

zephyr_library_sources(
  usbh_hid.c
  hid_report.c
)

zephyr_library_sources_ifdef(CONFIG_USERSPACE usbh_hid_handlers.c)
