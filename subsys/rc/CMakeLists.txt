# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/rc.h)

zephyr_library()

zephyr_library_sources(
  rc_core.c
  rc_keystate.c
  rc_protocol.c
  rc_raw.c
  rc_rtio.c
  rc_sync.c
)
