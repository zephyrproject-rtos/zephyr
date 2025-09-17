/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/sys/byteorder.h>

#include "util/util.h"
#include "util/memq.h"
#include "util/dbuf.h"
#include "util/mayfly.h"

#include "hal/ccm.h"
#include "hal/ticker.h"

#include "ticker/ticker.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll/lll_vendor.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_iso_types.h"

#include "ull_conn_types.h"
#include "ull_conn_iso_types.h"
#include "ull_internal.h"

#include "ull_conn_internal.h"
#include "ull_conn_iso_internal.h"
#include "lll_peripheral_iso.h"

void ull_peripheral_iso_release(uint16_t cis_handle)
{
}

uint8_t ull_peripheral_iso_acquire(struct ll_conn *acl,
				   struct pdu_data_llctrl_cis_req *req,
				   uint16_t *cis_handle)
{
	return 0;
}

uint8_t ull_peripheral_iso_setup(struct pdu_data_llctrl_cis_ind *ind,
				 uint8_t cig_id, uint16_t cis_handle)
{
	return 0;
}

void ull_peripheral_iso_update_peer_sca(struct ll_conn *acl)
{
}
