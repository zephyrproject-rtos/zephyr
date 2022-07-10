/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/net/buf.h>


static uint8_t *fixed_data_alloc(struct net_buf *buf, size_t *size,
			      k_timeout_t timeout)
{
	return NULL;
}

static void fixed_data_unref(struct net_buf *buf, uint8_t *data)
{
	/* Nothing needed for fixed-size data pools */
}

const struct net_buf_data_cb net_buf_fixed_cb = {
	.alloc = fixed_data_alloc,
	.unref = fixed_data_unref,
};

__attribute__((weak)) struct net_buf *net_buf_alloc_fixed(struct net_buf_pool *pool,
				    k_timeout_t timeout)
{
	return NULL;
}

__attribute__((weak)) void net_buf_simple_reserve(struct net_buf_simple *buf, size_t reserve)
{
}

__attribute__((weak)) struct net_buf *net_buf_ref(struct net_buf *buf)
{
	return NULL;
}
