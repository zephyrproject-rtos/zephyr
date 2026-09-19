# Copyright (c) 2026 Ana Clara Forcelli <ana.forcelli@lsitec.org.br>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_syscall_header(
  ${ZEPHYR_BASE}/include/zephyr/drivers/hwaccel.h
)

zephyr_library_sources_ifdef(CONFIG_USERSPACE   hwaccel_handlers.c)
zephyr_library_sources_ifdef(CONFIG_puppy_UDMA_FILTER puppy_udma_filter.c)
