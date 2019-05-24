/*
 * Copyright (c) 2016-2019 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <toolchain.h>
#include <soc.h>
#include <bluetooth/hci.h>

#include "hal/ccm.h"
#include "hal/radio.h"

#include "util/memq.h"

#include "pdu.h"

#include "lll.h"
#include "lll_conn.h"
#include "ull_conn_internal.h"

#if defined(CONFIG_BT_LL_SW_SPLIT)
u8_t ll_tx_pwr_lvl_get(u16_t handle, u8_t type, s8_t *tx_pwr_lvl)
{
	struct ll_conn *conn;

	conn = ll_connected_get(handle);
	if (!conn) {
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	/* TODO: check type here for current or maximum */

	/* TODO: Support TX Power Level other than default when dynamic
	 *       updates is implemented.
	 */
	*tx_pwr_lvl = RADIO_TXP_DEFAULT;

	return 0;
}
#endif

void ll_tx_pwr_get(s8_t *min, s8_t *max)
{
	/* TODO: Support TX Power Level other than default when dynamic
	 *       updates is implemented.
	 */
	*min = RADIO_TXP_DEFAULT;
	*max = RADIO_TXP_DEFAULT;
}
