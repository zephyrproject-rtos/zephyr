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
#include "lll.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"

void ull_central_setup(memq_link_t *link, struct node_rx_hdr *rx, struct node_rx_ftr *ftr,
		      struct lll_conn *lll)
{
}

void ull_central_ticker_cb(uint32_t ticks_at_expire, uint32_t remainder, uint16_t lazy, void *param)
{
}


uint8_t ull_central_chm_update(void)
{
	return 0;
}


int ull_central_reset(void)
{
	return 0;
}

uint16_t ull_central_iso_cis_offset_get(uint16_t cis_handle, uint32_t *cis_offset_min,
					uint32_t *cis_offset_max)
{
	return 0;
}

uint8_t ull_central_iso_setup(uint16_t cis_handle,
			      uint32_t *cig_sync_delay,
			      uint32_t *cis_sync_delay,
			      uint32_t *cis_offset_min,
			      uint32_t *cis_offset_max,
			      uint16_t *conn_event_count,
			      uint8_t  *access_addr)
{
	return 0;
}
