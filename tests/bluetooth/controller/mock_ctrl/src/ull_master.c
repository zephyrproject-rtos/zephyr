/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/types.h"
#include "util/mem.h"
#include "util/memq.h"

#include "pdu.h"

#include "hal/ccm.h"
#include "lll.h"
#include "lll_conn.h"


void ull_master_setup(memq_link_t *link, struct node_rx_hdr *rx,
		      struct node_rx_ftr *ftr, struct lll_conn *lll)
{
}
