/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "memq.h"
#include "lll.h"
#include "pdu.h"
#include "lll_conn.h"
#include "ull_internal.h"

void ull_master_setup(memq_link_t *link, struct node_rx_hdr *rx,
		      struct node_rx_ftr *ftr, struct lll_conn *lll)
{
}

void ull_master_ticker_cb(u32_t ticks_at_expire, u32_t remainder, u16_t lazy,
			  void *param)
{
}
