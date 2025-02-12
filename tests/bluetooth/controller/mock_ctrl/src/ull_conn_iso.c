/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <zephyr/kernel.h>

#include "hal/ccm.h"
#include "hal/ticker.h"
#include "hal/cpu.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/dbuf.h"
#include "util/mayfly.h"
#include "util/mfifo.h"
#include "ticker/ticker.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"
#include "lll.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_types.h"
#include "lll_conn_iso.h"
#include "ull_conn_iso_types.h"
#include "ull_conn_internal.h"
#include "ull_conn_iso_internal.h"
#include "ull_internal.h"
#include "lll/lll_vendor.h"

#include "hal/debug.h"

static struct ll_conn_iso_group cig = { 0 };
static struct ll_conn_iso_stream cis = { .established = 1, .group = &cig };

__weak struct ll_conn_iso_stream *ll_conn_iso_stream_get_by_acl(struct ll_conn *conn,
								uint16_t *cis_iter)
{
	return NULL;
}

__weak struct ll_conn_iso_stream *ll_conn_iso_stream_get(uint16_t handle)
{
	return &cis;
}

struct ll_conn_iso_stream *ll_iso_stream_connected_get(uint16_t handle)
{
	return NULL;
}

struct ll_conn_iso_group *ll_conn_iso_group_get_by_id(uint8_t id)
{
	return &cig;
}

struct ll_conn_iso_stream *ll_conn_iso_stream_get_by_group(struct ll_conn_iso_group *cig,
							   uint16_t *handle_iter)
{
	return NULL;
}

void ull_conn_iso_cis_stop(struct ll_conn_iso_stream *cis,
			   ll_iso_stream_released_cb_t cis_released_cb,
			   uint8_t reason)
{

}

void ull_conn_iso_cis_stop_by_id(uint8_t cig_id, uint8_t cis_id, uint8_t reason)
{

}

void ull_conn_iso_start(struct ll_conn *conn, uint16_t cis_handle,
			uint32_t ticks_at_expire, uint32_t remainder,
			uint16_t instant_latency)
{

}
