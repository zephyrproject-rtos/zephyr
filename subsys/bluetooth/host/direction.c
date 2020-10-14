/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <assert.h>

#include <bluetooth/hci.h>
#include <sys/byteorder.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_CORE)
#define LOG_MODULE_NAME bt_df
#include "common/log.h"

/* @brief Function provides information about DF antennae numer and
 *	  controller capabilities related with Constant Tone Extension.
 *
 * @param[out] switch_sample_rates      Optional switching and sampling rates.
 * @param[out] num_ant                  Antennae number.
 * @param[out] max_switch_pattern_len   Maximum supported antennae switching
 *                                      paterns length.
 * @param[out] max_cte_len              Maximum length of CTE in 8[us] units.
 *
 * @return Zero in case of success, other value in case of failure.
 */
static int hci_df_read_ant_info(uint8_t *switch_sample_rates,
				uint8_t *num_ant,
				uint8_t *max_switch_pattern_len,
				uint8_t *max_cte_len)
{
	__ASSERT_NO_MSG(switch_sample_rates);
	__ASSERT_NO_MSG(num_ant);
	__ASSERT_NO_MSG(max_switch_pattern_len);
	__ASSERT_NO_MSG(max_cte_len);

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
	       rp->max_switch_pattern_len, rp->max_cte_len);

	*switch_sample_rates = rp->switch_sample_rates;
	*num_ant = rp->num_ant;
	*max_switch_pattern_len = rp->max_switch_pattern_len;
	*max_cte_len = rp->max_cte_len;

	net_buf_unref(rsp);

	return 0;
}
