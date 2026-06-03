# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/pulse_io.h)

zephyr_library()

zephyr_library_sources(pulse_io_encoder.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE pulse_io_handlers.c)
