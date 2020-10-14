/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <assert.h>

#include <bluetooth/hci.h>
#include <host/direction.h>
#include <sys/byteorder.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_CORE)
#define LOG_MODULE_NAME bt_df
#include "common/log.h"

#if IS_ENABLED(CONFIG_TEST)
#define STATIC
#else
#define STATIC static
#endif /* CONFIG_TEST */

STATIC int hci_df_read_ant_info(struct bt_le_df_ant_info *ant_info)
{
	assert(ant_info != NULL);

	struct bt_hci_rp_le_read_ant_info *rp;
	struct net_buf *rsp;
	int err;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_ANT_INFO, NULL, &rsp);
	if (err) {
		BT_ERR("Failed to read antenna information");
		return err;
	}

	rp = (void *)rsp->data;

	BT_DBG("DF: sw. sampl rates: %x ant num: %u , max sw. pattern len: %u,"
	       "max CTE len %d", rp->switch_sample_rates, rp->num_ant,
	       rp->max_switch_pattern_len, rp->max_CTE_len);

	ant_info->switch_sample_rates = rp->switch_sample_rates;
	ant_info->num_ant = rp->num_ant;
	ant_info->max_switch_pattern_len = rp->max_switch_pattern_len;
	ant_info->max_CTE_len = rp->max_CTE_len;

	net_buf_unref(rsp);

	return 0;
}
