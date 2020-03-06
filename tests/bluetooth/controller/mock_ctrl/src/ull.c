/*
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "memq.h"
#include "lll.h"
#include "pdu.h"
#include "lll_conn.h"
#include "ull_internal.h"

void *ll_rx_link_alloc(void)
{
	return NULL;
}

void ll_rx_link_release(void *link)
{
}

void *ll_rx_alloc(void)
{
	return NULL;
}

void ll_rx_release(void *node_rx)
{
}

void *ll_pdu_rx_alloc_peek(u8_t count)
{
	return NULL;
}

void *ll_pdu_rx_alloc(void)
{
	return NULL;
}

void ll_rx_put(memq_link_t *link, void *rx)
{
}

void ll_rx_sched(void)
{
}

void ll_tx_ack_put(u16_t handle, struct node_tx *node_tx)
{
}

void ull_ticker_status_give(u32_t status, void *param)
{
}

u32_t ull_ticker_status_take(u32_t ret, u32_t volatile *ret_cb)
{
	return 0;
}

void *ull_disable_mark(void *param)
{
	return NULL;
}

void *ull_disable_unmark(void *param)
{
	return NULL;
}

void *ull_disable_mark_get(void)
{
	return NULL;
}

void *ull_update_mark(void *param)
{
	return NULL;
}

void *ull_update_unmark(void *param)
{
	return NULL;
}

void *ull_update_mark_get(void)
{
	return NULL;
}

int ull_disable(void *param)
{
	return 0;
}
