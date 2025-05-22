# Copyright (c) 2023, Meta
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_syscall_header(
  ${ZEPHYR_BASE}/include/zephyr/drivers/misc/anx7533/anx7533.h
)

zephyr_library_sources(
	anx7533.c
	anx7533_flash.c
)
