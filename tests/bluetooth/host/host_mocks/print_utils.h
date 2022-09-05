/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <addr.h>

const char *bt_addr_str_real(const bt_addr_t *addr);
const char *bt_addr_le_str_real(const bt_addr_le_t *addr);
const char *bt_hex_real(const void *buf, size_t len);
void z_log_minimal_printk(const char *fmt, ...);

#ifndef bt_addr_le_str
#define bt_addr_le_str(addr) bt_addr_le_str_real(addr)
#endif
