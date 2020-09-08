/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

#include "util/mem.h"
#include "util/memq.h"
/*
 * just a big number
 */
#define PDU_RX_POOL_SIZE 16384


static struct {
	void *free;
	uint8_t pool[PDU_RX_POOL_SIZE];
} mem_pdu_rx;

/*
 * just a big number
 */
#define LINK_RX_POOL_SIZE 16384

static struct {
	uint8_t quota_pdu; /* Number of un-utilized buffers */

	void *free;
	uint8_t pool[LINK_RX_POOL_SIZE];
} mem_link_rx;



void ull_ticker_status_give(uint32_t status, void *param)
{

}

uint32_t ull_ticker_status_take(uint32_t ret, uint32_t volatile *ret_cb)
{
	return *ret_cb;
}

void *ull_disable_mark(void *param)
{
	return NULL;
}

void *ull_disable_unmark(void *param)
{
	return NULL;
}

int ull_disable(void *lll)
{
	return 0;
}

void *ll_rx_link_alloc(void)
{
	return mem_acquire(&mem_link_rx.free);
}

void ll_rx_link_release(void *link)
{
	mem_release(link, &mem_link_rx.free);
}

void *ll_rx_alloc(void)
{
	return mem_acquire(&mem_pdu_rx.free);
}

void ll_rx_release(void *node_rx)
{
	mem_release(node_rx, &mem_pdu_rx.free);
}
