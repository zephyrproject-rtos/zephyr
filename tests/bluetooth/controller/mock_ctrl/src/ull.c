/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>



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
