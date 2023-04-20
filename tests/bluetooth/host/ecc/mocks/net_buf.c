/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/buf.h>
#include <zephyr/kernel.h>
#include <zephyr/net/buf.h>

#include <mocks/net_buf.h>

DEFINE_FAKE_VALUE_FUNC(void *, net_buf_simple_add, struct net_buf_simple *, size_t);
