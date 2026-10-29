/*
 * Copyright (c) 2026 Kent Hauser
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>

#include <zephyr/types.h>
#include <zephyr/sys/slist.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/dbuf.h"
#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"
#include "lll.h"
#include "lll_conn.h"
#include "ull_tx_queue.h"
#include "ull_conn_types.h"
#include "ull_conn_internal.h"

#include "conn_meta.h"

/* Vendor initializer: the native members, including the parent set by lll_hdr_init(), are set
 * before it is called.
 */
void lll_conn_meta_init(struct lll_conn *lll)
{
	lll->conn_meta.inited = (lll->hdr.parent != NULL) ? 1U : 0U;
}

bool conn_meta_inited(uint16_t handle)
{
	struct ll_conn *conn = ll_conn_get(handle);

	return (conn != NULL) && (conn->lll.conn_meta.inited == 1U);
}
