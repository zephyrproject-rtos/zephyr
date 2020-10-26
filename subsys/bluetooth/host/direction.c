/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <assert.h>

#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <sys/byteorder.h>

#include "conn_internal.h"

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

/* @brief Function sets CTE parameters for connection object
 *
 * @param[in] cte_types         Allowed response CTE types
 * @param[in] num_ant_id        Number of available antenna identification
 *                              patterns in @p ant_id array.
 * @param[in] ant_id            Array with antenna identification patterns.
 *
 * @return Zero in case of success, other value in case of failure.
 */
static int hci_df_set_conn_cte_tx_param(struct bt_conn *conn, uint8_t cte_types,
					uint8_t num_ant_id, uint8_t *ant_id)
{
	__ASSERT_NO_MSG(conn);
	__ASSERT_NO_MSG(cte_types != 0);

	struct bt_hci_cp_le_set_conn_cte_tx_params *cp;
	struct bt_hci_rp_le_set_conn_cte_tx_params *rp;
	struct net_buf *buf, *rsp;
	int err;

	/* If AoD is not enabled, ant_ids are ignored by controller:
	 * BT Core spec 5.2 Vol 4, Part E sec. 7.8.84.
	 */
	if (cte_types & BT_HCI_LE_AOD_CTE_RSP_1US ||
	    cte_types & BT_HCI_LE_AOD_CTE_RSP_2US) {

		if (num_ant_id < BT_HCI_LE_SWITCH_PATTERN_LEN_MIN ||
		    num_ant_id > BT_HCI_LE_SWITCH_PATTERN_LEN_MAX ||
		    !ant_id) {
			return -EINVAL;
		}
		__ASSERT_NO_MSG((sizeof(*cp) + num_ant_id) <  UINT8_MAX);
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_CONN_CTE_TX_PARAMS,
				sizeof(*cp) + num_ant_id);
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(conn->handle);
	cp->cte_types = cte_types;

	if (num_ant_id) {
		uint8_t *dest_ant_id = net_buf_add(buf, num_ant_id);

		memcpy(dest_ant_id, ant_id, num_ant_id);
		cp->switch_pattern_len = num_ant_id;
	} else {
		cp->switch_pattern_len = 0;
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_CONN_CTE_TX_PARAMS,
				   buf, &rsp);
	if (err) {
		return err;
	}

	rp = (void *)rsp->data;
	if (conn->handle != sys_le16_to_cpu(rp->handle)) {
		err = -EIO;
	}

	net_buf_unref(rsp);

	return err;
}
