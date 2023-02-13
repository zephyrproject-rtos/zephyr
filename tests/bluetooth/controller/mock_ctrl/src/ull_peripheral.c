/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/types.h"
#include <zephyr/ztest.h>
#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "hal/ccm.h"
#include "ull_tx_queue.h"
#include "lll.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "ull_conn_types.h"

void ull_periph_setup(memq_link_t *link, struct node_rx_hdr *rx, struct node_rx_ftr *ftr,
		     struct lll_conn *lll)
{
}

void ull_periph_ticker_cb(uint32_t ticks_at_expire, uint32_t remainder, uint16_t lazy, void *param)
{
}

void ull_periph_latency_cancel(struct ll_conn *conn, uint16_t handle)
{
}
