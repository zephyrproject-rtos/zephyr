/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <assert.h>
#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/direction.h>

#include "hci_core.h"
#include "conn_internal.h"
#include "direction_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_DF)
#define LOG_MODULE_NAME bt_df
#include "common/log.h"

/* @brief Antenna information for LE Direction Finding */
struct bt_le_df_ant_info {
	/* Bitfield holding optional switching and sampling rates */
	uint8_t switch_sample_rates;
	/* Available antennae number */
	uint8_t num_ant;
	/* Maximum supported antennae switching pattern length */
	uint8_t max_switch_pattern_len;
	/* Maximum length of CTE in 8[us] units */
	uint8_t max_cte_len;
};

static struct bt_le_df_ant_info df_ant_info;

#define DF_SUPP_TEST(feat, n)                   ((feat) & BIT((n)))

#define DF_AOD_TX_1US_SUPPORT(supp)             (DF_SUPP_TEST(supp, \
						BT_HCI_LE_1US_AOD_TX))
#define DF_AOD_RX_1US_SUPPORT(supp)             (DF_SUPP_TEST(supp, \
						BT_HCI_LE_1US_AOD_RX))
#define DF_AOA_RX_1US_SUPPORT(supp)             (DF_SUPP_TEST(supp, \
						BT_HCI_LE_1US_AOA_RX))

static int hci_df_set_cl_cte_tx_params(const struct bt_le_ext_adv *adv,
				    const struct bt_df_adv_cte_tx_param *params)
{
	struct bt_hci_cp_le_set_cl_cte_tx_params *cp;
	struct net_buf *buf;

	/* If AoD is not enabled, ant_ids are ignored by controller:
	 * BT Core spec 5.2 Vol 4, Part E sec. 7.8.80.
	 */
	if (params->cte_type == BT_HCI_LE_AOD_CTE_1US ||
	    params->cte_type == BT_HCI_LE_AOD_CTE_2US) {

		if (!BT_FEAT_LE_ANT_SWITCH_TX_AOD(bt_dev.le.features)) {
			return -EINVAL;
		}

		if (params->cte_type == BT_HCI_LE_AOD_CTE_1US &&
		    !DF_AOD_TX_1US_SUPPORT(df_ant_info.switch_sample_rates)) {
			return -EINVAL;
		}

		if (params->num_ant_ids < BT_HCI_LE_SWITCH_PATTERN_LEN_MIN ||
		    params->num_ant_ids > BT_HCI_LE_SWITCH_PATTERN_LEN_MAX ||
		    !params->ant_ids) {
			return -EINVAL;
		}
	} else if (params->cte_type != BT_HCI_LE_AOA_CTE) {
		return -EINVAL;
	}

	if (params->cte_len < BT_HCI_LE_CTE_LEN_MIN ||
	    params->cte_len > BT_HCI_LE_CTE_LEN_MAX) {
		return -EINVAL;
	}

	if (params->cte_count < BT_HCI_LE_CTE_COUNT_MIN ||
	    params->cte_count > BT_HCI_LE_CTE_COUNT_MAX) {
		return -EINVAL;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_CL_CTE_TX_PARAMS,
				sizeof(*cp) + params->num_ant_ids);
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = adv->handle;
	cp->cte_len = params->cte_len;
	cp->cte_type = params->cte_type;
	cp->cte_count = params->cte_count;

	if (params->num_ant_ids) {
		uint8_t *dest_ant_ids = net_buf_add(buf, params->num_ant_ids);

		memcpy(dest_ant_ids, params->ant_ids, params->num_ant_ids);
		cp->switch_pattern_len = params->num_ant_ids;
	} else {
		cp->switch_pattern_len = 0;
	}

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_CL_CTE_TX_PARAMS,
				    buf, NULL);
}

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

/* @brief Function handles send of HCI commnad to enable or disables CTE
 *        transmission for given advertising set.
 *
 * @param[in] adv               Pointer to advertising set
 * @param[in] enable            Enable or disable CTE TX
 *
 * @return Zero in case of success, other value in case of failure.
 */
static int hci_df_set_adv_cte_tx_enable(struct bt_le_ext_adv *adv,
					bool enable)
{
	struct bt_hci_cp_le_set_cl_cte_tx_enable *cp;
	struct bt_hci_cmd_state_set state;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_CL_CTE_TX_ENABLE, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	(void)memset(cp, 0, sizeof(*cp));

	cp->handle = adv->handle;
	cp->cte_enable = enable ? 1 : 0;

	bt_hci_cmd_state_set_init(&state, adv->flags, BT_PER_ADV_CTE_ENABLED,
				  enable);
	bt_hci_cmd_data_state_set(buf, &state);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_CL_CTE_TX_ENABLE,
				   buf, NULL);
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

/* @brief Function initializes Direction Finding in Host
 *
 * @return Zero in case of success, other value in case of failure.
 */
int le_df_init(void)
{
	uint8_t max_switch_pattern_len;
	uint8_t switch_sample_rates;
	uint8_t max_cte_len;
	uint8_t num_ant;
	int err;

	err = hci_df_read_ant_info(&switch_sample_rates, &num_ant,
			     &max_switch_pattern_len, &max_cte_len);
	if (err) {
		return err;
	}

	df_ant_info.max_switch_pattern_len = max_switch_pattern_len;
	df_ant_info.switch_sample_rates = switch_sample_rates;
	df_ant_info.max_cte_len = max_cte_len;
	df_ant_info.num_ant = num_ant;

	BT_DBG("DF initialized.");
	return 0;
}

int bt_df_set_adv_cte_tx_param(struct bt_le_ext_adv *adv,
				const struct bt_df_adv_cte_tx_param *params)
{
	__ASSERT_NO_MSG(adv);
	__ASSERT_NO_MSG(params);

	int err;

	if (!BT_FEAT_LE_CONNECTIONLESS_CTE_TX(bt_dev.le.features)) {
		return -ENOTSUP;
	}

	/* Check if BT_ADV_PARAMS_SET is set, because it implies the set
	 * has already been created.
	 */
	if (!atomic_test_bit(adv->flags, BT_ADV_PARAMS_SET)) {
		return -EINVAL;
	}

	if (atomic_test_bit(adv->flags, BT_PER_ADV_CTE_ENABLED)) {
		return -EINVAL;
	}

	err = hci_df_set_cl_cte_tx_params(adv, params);
	if (err) {
		return err;
	}

	atomic_set_bit(adv->flags, BT_PER_ADV_CTE_PARAMS_SET);

	return 0;
}

static int bt_df_set_adv_cte_tx_enabled(struct bt_le_ext_adv *adv, bool enable)
{
	if (!atomic_test_bit(adv->flags, BT_PER_ADV_PARAMS_SET)) {
		return -EINVAL;
	}

	if (!atomic_test_bit(adv->flags, BT_PER_ADV_CTE_PARAMS_SET)) {
		return -EINVAL;
	}

	if (enable == atomic_test_bit(adv->flags, BT_PER_ADV_CTE_ENABLED)) {
		return -EALREADY;
	}

	return hci_df_set_adv_cte_tx_enable(adv, enable);
}

int bt_df_adv_cte_tx_enable(struct bt_le_ext_adv *adv)
{
	__ASSERT_NO_MSG(adv);
	return bt_df_set_adv_cte_tx_enabled(adv, true);
}

int bt_df_adv_cte_tx_disable(struct bt_le_ext_adv *adv)
{
	__ASSERT_NO_MSG(adv);
	return bt_df_set_adv_cte_tx_enabled(adv, false);
}
