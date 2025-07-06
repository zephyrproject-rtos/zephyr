/*
 * Common functions and helpers for ISO tests
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/sys/atomic.h>

extern struct bt_conn *default_conn;
extern atomic_t flag_connected;
extern atomic_t flag_conn_updated;
