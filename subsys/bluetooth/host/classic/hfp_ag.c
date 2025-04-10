/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/conn.h>

#include "common/assert.h"

#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/hfp_ag.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "l2cap_br_internal.h"
#include "rfcomm_internal.h"
#include "at.h"
#include "sco_internal.h"
#include "hfp_ag_internal.h"

#define LOG_LEVEL CONFIG_BT_HFP_AG_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hfp_ag);

typedef int (*bt_hfp_ag_parse_command_t)(struct bt_hfp_ag *ag, struct net_buf *buf);

struct bt_hfp_ag_at_cmd_handler {
	const char *cmd;
	bt_hfp_ag_parse_command_t handler;
};

static const struct {
	const char *name;
	const char *connector;
	uint32_t min;
	uint32_t max;
} ag_ind[] = {
	{"service", "-", 0, 1},   /* BT_HFP_AG_SERVICE_IND */
	{"call", ",", 0, 1},      /* BT_HFP_AG_CALL_IND */
	{"callsetup", "-", 0, 3}, /* BT_HFP_AG_CALL_SETUP_IND */
	{"callheld", "-", 0, 2},  /* BT_HFP_AG_CALL_HELD_IND */
	{"signal", "-", 0, 5},    /* BT_HFP_AG_SIGNAL_IND */
	{"roam", ",", 0, 1},      /* BT_HFP_AG_ROAM_IND */
	{"battchg", "-", 0, 5}    /* BT_HFP_AG_BATTERY_IND */
};

typedef void (*bt_hfp_ag_tx_cb_t)(struct bt_hfp_ag *ag, void *user_data);

struct bt_ag_tx {
	sys_snode_t node;

	struct bt_hfp_ag *ag;
	struct net_buf *buf;
	bt_hfp_ag_tx_cb_t cb;
	void *user_data;
	int err;
};

NET_BUF_POOL_FIXED_DEFINE(ag_pool, CONFIG_BT_HFP_AG_TX_BUF_COUNT,
			  BT_RFCOMM_BUF_SIZE(BT_HF_CLIENT_MAX_PDU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_hfp_ag bt_hfp_ag_pool[CONFIG_BT_MAX_CONN];

static struct bt_hfp_ag_cb *bt_ag;

#define AG_SUPT_FEAT(ag, _feature) ((ag->ag_features & (_feature)) != 0)
#define HF_SUPT_FEAT(ag, _feature) ((ag->hf_features & (_feature)) != 0)
#define BOTH_SUPT_FEAT(ag, _hf_feature, _ag_feature) \
	(HF_SUPT_FEAT(ag, _hf_feature) && AG_SUPT_FEAT(ag, _ag_feature))

/* Sent but not acknowledged TX packets with a callback */
static struct bt_ag_tx ag_tx[CONFIG_BT_HFP_AG_TX_BUF_COUNT * 2];
static K_FIFO_DEFINE(ag_tx_free);
static K_FIFO_DEFINE(ag_tx_notify);

/* HFP Gateway SDP record */
static struct bt_sdp_attribute hfp_ag_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_HANDSFREE_AGW_SVCLASS)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_GENERIC_AUDIO_SVCLASS)
		}
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				BT_SDP_ARRAY_8(BT_RFCOMM_CHAN_HFP_AG)
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_HANDSFREE_SVCLASS)
		},
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			BT_SDP_ARRAY_16(0x0109)
		},
		)
	),

	BT_SDP_LIST(
		BT_SDP_ATTR_NETWORK,
		BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
		BT_SDP_ARRAY_8(IS_ENABLED(CONFIG_BT_HFP_AG_REJECT_CALL))
	),
	/* The values of the “SupportedFeatures” bitmap shall be the same as the
	 * values of the Bits 0 to 4 of the AT-command AT+BRSF (see Section 5.3).
	 */
	BT_SDP_SUPPORTED_FEATURES(BT_HFP_AG_SDP_SUPPORTED_FEATURES),
};

static struct bt_sdp_record hfp_ag_rec = BT_SDP_RECORD(hfp_ag_attrs);

static enum at_cme bt_hfp_ag_get_cme_err(int err)
{
	enum at_cme cme_err;

	switch (err) {
	case -ENOEXEC:
		cme_err = CME_ERROR_OPERATION_NOT_SUPPORTED;
		break;
	case -EFAULT:
		cme_err = CME_ERROR_AG_FAILURE;
		break;
	case -ENOSR:
		cme_err = CME_ERROR_MEMORY_FAILURE;
		break;
	case -ENOMEM:
	case -ENOBUFS:
		cme_err = CME_ERROR_MEMORY_FULL;
		break;
	case -ENAMETOOLONG:
		cme_err = CME_ERROR_DIAL_STRING_TO_LONG;
		break;
	case -EINVAL:
		cme_err = CME_ERROR_INVALID_INDEX;
		break;
	case -ENOTSUP:
		cme_err = CME_ERROR_OPERATION_NOT_ALLOWED;
		break;
	case -ENOTCONN:
		cme_err = CME_ERROR_NO_CONNECTION_TO_PHONE;
		break;
	default:
		cme_err = CME_ERROR_AG_FAILURE;
		break;
	}

	return cme_err;
}

static void hfp_ag_lock(struct bt_hfp_ag *ag)
{
	k_sem_take(&ag->lock, K_FOREVER);
}

static void hfp_ag_unlock(struct bt_hfp_ag *ag)
{
	k_sem_give(&ag->lock);
}

static void bt_hfp_ag_set_state(struct bt_hfp_ag *ag, bt_hfp_state_t state)
{
	bt_hfp_state_t old_state;

	hfp_ag_lock(ag);
	old_state = ag->state;
	ag->state = state;
	hfp_ag_unlock(ag);

	LOG_DBG("update state %p, old %d -> new %d", ag, old_state, state);

	if (old_state == state) {
		return;
	}

	switch (state) {
	case BT_HFP_DISCONNECTED:
		if (bt_ag && bt_ag->disconnected) {
			bt_ag->disconnected(ag);
		}
		break;
	case BT_HFP_CONNECTING:
		break;
	case BT_HFP_CONFIG:
		break;
	case BT_HFP_CONNECTED:
		if (bt_ag && bt_ag->connected) {
			bt_ag->connected(ag->acl_conn, ag);
		}
		break;
	case BT_HFP_DISCONNECTING:
		break;
	default:
		LOG_WRN("no valid (%u) state was set", state);
		break;
	}
}

static struct bt_hfp_ag_call *get_call_from_number(struct bt_hfp_ag *ag, const char *number,
						   uint8_t type)
{
	struct bt_hfp_ag_call *call;

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			continue;
		}

		if (call->type != type) {
			continue;
		}

		if (strcmp(call->number, number)) {
			continue;
		}

		return call;
	}

	return NULL;
}

static void bt_ag_deferred_work(struct k_work *work);
static void bt_ag_ringing_work(struct k_work *work);

static struct bt_hfp_ag_call *get_new_call(struct bt_hfp_ag *ag, const char *number, uint8_t type)
{
	struct bt_hfp_ag_call *call;

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (!atomic_test_and_set_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			/* Copy number to ag->number including null-character */
			strcpy(call->number, number);
			call->type = type;
			call->ag = ag;
			k_work_init_delayable(&call->deferred_work, bt_ag_deferred_work);
			k_work_init_delayable(&call->ringing_work, bt_ag_ringing_work);
			return call;
		}
	}

	return NULL;
}

static int get_none_released_calls(struct bt_hfp_ag *ag)
{
	struct bt_hfp_ag_call *call;
	int count = 0;

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			count++;
		}
	}

	return count;
}

static struct bt_ag_tx *bt_ag_tx_alloc(void)
{
	/* The TX context always get freed in the system workqueue,
	 * so if we're in the same workqueue but there are no immediate
	 * contexts available, there's no chance we'll get one by waiting.
	 */
	if (k_current_get() == &k_sys_work_q.thread) {
		return k_fifo_get(&ag_tx_free, K_NO_WAIT);
	}

	if (IS_ENABLED(CONFIG_BT_HFP_AG_LOG_LEVEL_DBG)) {
		struct bt_ag_tx *tx = k_fifo_get(&ag_tx_free, K_NO_WAIT);

		if (tx) {
			return tx;
		}

		LOG_WRN("Unable to get an immediate free bt_ag_tx");
	}

	return k_fifo_get(&ag_tx_free, K_FOREVER);
}

static void bt_ag_tx_free(struct bt_ag_tx *tx)
{
	LOG_DBG("Free tx buffer %p", tx);

	(void)memset(tx, 0, sizeof(*tx));

	k_fifo_put(&ag_tx_free, tx);
}

static int hfp_ag_next_step(struct bt_hfp_ag *ag, bt_hfp_ag_tx_cb_t cb, void *user_data)
{
	struct bt_ag_tx *tx;

	LOG_DBG("cb %p user_data %p", cb, user_data);

	tx = bt_ag_tx_alloc();
	if (tx == NULL) {
		LOG_ERR("No tx buffers!");
		return -ENOMEM;
	}

	LOG_DBG("Alloc tx buffer %p", tx);

	tx->ag = ag;
	tx->buf = NULL;
	tx->cb = cb;
	tx->user_data = user_data;
	tx->err = 0;

	k_fifo_put(&ag_tx_notify, tx);

	return 0;
}

static int hfp_ag_send_data(struct bt_hfp_ag *ag, bt_hfp_ag_tx_cb_t cb, void *user_data,
			    const char *format, ...)
{
	struct net_buf *buf = NULL;
	struct bt_ag_tx *tx = NULL;
	va_list vargs;
	int err;
	bt_hfp_state_t state;

	LOG_DBG("AG %p sending data cb %p user_data %p", ag, cb, user_data);

	hfp_ag_lock(ag);
	state = ag->state;
	hfp_ag_unlock(ag);
	if ((state == BT_HFP_DISCONNECTED) || (state == BT_HFP_DISCONNECTING)) {
		LOG_ERR("AG %p is not connected", ag);
		err = -ENOTCONN;
		goto failed;
	}

	buf = bt_rfcomm_create_pdu(&ag_pool);
	if (!buf) {
		LOG_ERR("No Buffers!");
		err = -ENOMEM;
		goto failed;
	}

	tx = bt_ag_tx_alloc();
	if (tx == NULL) {
		LOG_ERR("No tx buffers!");
		err = -ENOMEM;
		goto failed;
	}

	LOG_DBG("buf %p tx %p", buf, tx);

	tx->ag = ag;
	tx->buf = buf;
	tx->cb = cb;
	tx->user_data = user_data;

	va_start(vargs, format);
	err = vsnprintk((char *)buf->data, (net_buf_tailroom(buf) - 1), format, vargs);
	va_end(vargs);

	if (err < 0) {
		LOG_ERR("Unable to format variable arguments");
		goto failed;
	}

	net_buf_add(buf, err);

	LOG_HEXDUMP_DBG(buf->data, buf->len, "Sending:");

	hfp_ag_lock(ag);
	sys_slist_append(&ag->tx_pending, &tx->node);
	hfp_ag_unlock(ag);

	/* Always active tx work */
	k_work_reschedule(&ag->tx_work, K_NO_WAIT);

	return 0;

failed:
	if (buf) {
		net_buf_unref(buf);
	}

	if (tx) {
		bt_ag_tx_free(tx);
	}
	bt_hfp_ag_disconnect(ag);
	return err;
}

static int hfp_ag_update_indicator(struct bt_hfp_ag *ag, enum bt_hfp_ag_indicator index,
				   uint8_t value, bt_hfp_ag_tx_cb_t cb, void *user_data)
{
	int err;
	uint8_t old_value;

	if (index >= BT_HFP_AG_IND_MAX) {
		return -EINVAL;
	}

	if ((ag_ind[index].max < value) || (ag_ind[index].min > value)) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	old_value = ag->indicator_value[index];
	if (value == old_value) {
		LOG_ERR("Duplicate value setting, indicator %d, old %d -> new %d", index, old_value,
			value);
		hfp_ag_unlock(ag);
		if (cb) {
			cb(ag, user_data);
		}
		return -EINVAL;
	}

	ag->indicator_value[index] = value;
	hfp_ag_unlock(ag);

	LOG_DBG("indicator %d, old %d -> new %d", index, old_value, value);

	if (!(ag->indicator & BIT(index))) {
		LOG_INF("The indicator %d is deactivated", index);
		/* If the indicator is deactivated, consider it a successful set. */
		return 0;
	}

	err = hfp_ag_send_data(ag, cb, user_data, "\r\n+CIEV:%d,%d\r\n", (uint8_t)index + 1, value);
	if (err) {
		hfp_ag_lock(ag);
		ag->indicator_value[index] = old_value;
		hfp_ag_unlock(ag);
		LOG_ERR("Fail to update indicator %d, current %d", index, old_value);
	}

	return err;
}

static int hfp_ag_force_update_indicator(struct bt_hfp_ag *ag, enum bt_hfp_ag_indicator index,
					 uint8_t value, bt_hfp_ag_tx_cb_t cb, void *user_data)
{
	int err;
	uint8_t old_value;

	hfp_ag_lock(ag);
	old_value = ag->indicator_value[index];
	ag->indicator_value[index] = value;
	hfp_ag_unlock(ag);

	LOG_DBG("indicator %d, old %d -> new %d", index, old_value, value);

	err = hfp_ag_send_data(ag, cb, user_data, "\r\n+CIEV:%d,%d\r\n", (uint8_t)index + 1, value);
	if (err) {
		hfp_ag_lock(ag);
		ag->indicator_value[index] = old_value;
		hfp_ag_unlock(ag);
		LOG_ERR("Fail to update indicator %d, current %d", index, old_value);
	}

	return err;
}

static void clear_call_setup_ind_cb(struct bt_hfp_ag *ag, void *user_data)
{
	uint8_t call_ind;
	int err;

	hfp_ag_lock(ag);
	call_ind = ag->indicator_value[BT_HFP_AG_CALL_IND];
	hfp_ag_unlock(ag);

	if (call_ind) {
		err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 0, NULL, NULL);
		if (err) {
			LOG_ERR("Fail to clear call_setup ind");
		}
	}
}

static void clear_call_ind_cb(struct bt_hfp_ag *ag, void *user_data)
{
	uint8_t call_setup_ind;
	int err;

	hfp_ag_lock(ag);
	call_setup_ind = ag->indicator_value[BT_HFP_AG_CALL_SETUP_IND];
	hfp_ag_unlock(ag);

	if (call_setup_ind) {
		err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_NONE,
					      NULL, NULL);
		if (err) {
			LOG_ERR("Fail to clear call_setup ind");
		}
	}
}

static void clear_call_and_call_setup_ind(struct bt_hfp_ag *ag)
{
	uint8_t call_setup_ind;
	uint8_t call_ind;
	int err;

	hfp_ag_lock(ag);
	call_setup_ind = ag->indicator_value[BT_HFP_AG_CALL_SETUP_IND];
	call_ind = ag->indicator_value[BT_HFP_AG_CALL_IND];
	hfp_ag_unlock(ag);

	if (call_setup_ind) {
		err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_NONE,
					      clear_call_setup_ind_cb, NULL);
		if (err) {
			LOG_ERR("Fail to clear call_setup ind");
		}
	}

	if (call_ind) {
		err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 0, clear_call_ind_cb, NULL);
		if (err) {
			LOG_ERR("Fail to clear call ind");
		}
	}
}

static int get_active_calls(struct bt_hfp_ag *ag)
{
	struct bt_hfp_ag_call *call;
	int count = 0;

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			hfp_ag_lock(ag);
			if ((call->call_state == BT_HFP_CALL_ACTIVE) &&
			    (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING_HELD))) {
				count++;
			}
			hfp_ag_unlock(ag);
		}
	}

	return count;
}

#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
static int get_active_held_calls(struct bt_hfp_ag *ag)
{
	struct bt_hfp_ag_call *call;
	int count = 0;

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			hfp_ag_lock(ag);
			if ((call->call_state == BT_HFP_CALL_HOLD) ||
			    ((call->call_state == BT_HFP_CALL_ACTIVE) &&
			     (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING_HELD)))) {
				count++;
			}
			hfp_ag_unlock(ag);
		}
	}

	return count;
}
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */

static int get_held_calls(struct bt_hfp_ag *ag)
{
	struct bt_hfp_ag_call *call;
	int count = 0;

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			hfp_ag_lock(ag);
			if ((call->call_state == BT_HFP_CALL_ACTIVE) &&
			    (atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING_HELD))) {
				count++;
			} else if (call->call_state == BT_HFP_CALL_HOLD) {
				count++;
			}
			hfp_ag_unlock(ag);
		}
	}

	return count;
}

static void ag_notify_call_held_ind(struct bt_hfp_ag *ag, void *user_data)
{
	int active_call_count;
	int held_call_count;
	int err;
	uint8_t value;

	active_call_count = get_active_calls(ag);
	held_call_count = get_held_calls(ag);
	if (active_call_count && held_call_count) {
		value = BT_HFP_CALL_HELD_ACTIVE_HELD;
	} else if (held_call_count) {
		value = BT_HFP_CALL_HELD_HELD;
	} else {
		value = BT_HFP_CALL_HELD_NONE;
	}

	err = hfp_ag_force_update_indicator(ag, BT_HFP_AG_CALL_HELD_IND, value, NULL, NULL);
	if (err) {
		LOG_ERR("Fail to notify call_held ind :(%d)", err);
	}
}

static void free_call(struct bt_hfp_ag_call *call)
{
	int call_count;
	struct bt_hfp_ag *ag;
	int err;

	ag = call->ag;

	k_work_cancel_delayable(&call->ringing_work);
	k_work_cancel_delayable(&call->deferred_work);
	memset(call, 0, sizeof(*call));

	call_count = get_none_released_calls(ag);

	if (!call_count) {
		clear_call_and_call_setup_ind(ag);
	} else {
		err = hfp_ag_next_step(ag, ag_notify_call_held_ind, NULL);
		if (err) {
			LOG_ERR("Fail to notify call_held ind :(%d)", err);
		}
	}
}

static void bt_hfp_ag_set_call_state(struct bt_hfp_ag_call *call, bt_hfp_call_state_t call_state)
{
	bt_hfp_state_t state;
	struct bt_hfp_ag *ag = call->ag;

	LOG_DBG("update call state %p, old %d -> new %d", call, call->call_state, call_state);

	hfp_ag_lock(ag);
	call->call_state = call_state;
	state = ag->state;
	hfp_ag_unlock(ag);

	switch (call_state) {
	case BT_HFP_CALL_TERMINATE:
		atomic_clear_bit(ag->flags, BT_HFP_AG_CREATING_SCO);
		free_call(call);
		break;
	case BT_HFP_CALL_OUTGOING:
		k_work_reschedule(&call->deferred_work,
				  K_SECONDS(CONFIG_BT_HFP_AG_OUTGOING_TIMEOUT));
		break;
	case BT_HFP_CALL_INCOMING:
		k_work_reschedule(&call->deferred_work,
				  K_SECONDS(CONFIG_BT_HFP_AG_INCOMING_TIMEOUT));
		break;
	case BT_HFP_CALL_ALERTING:
		if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING_3WAY)) {
			k_work_reschedule(&call->ringing_work, K_NO_WAIT);
		} else {
			k_work_cancel_delayable(&call->ringing_work);
		}
		k_work_reschedule(&call->deferred_work,
				  K_SECONDS(CONFIG_BT_HFP_AG_ALERTING_TIMEOUT));
		break;
	case BT_HFP_CALL_ACTIVE:
		k_work_cancel_delayable(&call->ringing_work);
		k_work_cancel_delayable(&call->deferred_work);
		break;
	case BT_HFP_CALL_HOLD:
		break;
	default:
		/* Invalid state */
		break;
	}

	if (state == BT_HFP_DISCONNECTING) {
		int err = bt_rfcomm_dlc_disconnect(&ag->rfcomm_dlc);

		if (err) {
			LOG_ERR("Fail to disconnect DLC %p", &ag->rfcomm_dlc);
		}
	}
}

static void hfp_ag_close_sco(struct bt_hfp_ag *ag)
{
	struct bt_conn *sco;
	int call_count;

	LOG_DBG("");

	hfp_ag_lock(ag);
	call_count = get_none_released_calls(ag);
	if (call_count > 0) {
		LOG_INF("Do not close sco because not all calls are released");
		hfp_ag_unlock(ag);
		return;
	}

	sco = ag->sco_chan.sco;
	ag->sco_chan.sco = NULL;
	hfp_ag_unlock(ag);
	if (sco != NULL) {
		LOG_DBG("Disconnect sco %p", sco);
		bt_conn_disconnect(sco, BT_HCI_ERR_LOCALHOST_TERM_CONN);
	}
}

static void ag_reject_call(struct bt_hfp_ag_call *call)
{
	struct bt_hfp_ag *ag;

	ag = call->ag;

	if (bt_ag && bt_ag->reject) {
		bt_ag->reject(call);
	}

	bt_hfp_ag_set_call_state(call, BT_HFP_CALL_TERMINATE);

	if (!atomic_test_bit(ag->flags, BT_HFP_AG_AUDIO_CONN)) {
		LOG_DBG("It is not audio connection");
		hfp_ag_close_sco(ag);
	}
}

static void ag_terminate_call(struct bt_hfp_ag_call *call)
{
	struct bt_hfp_ag *ag;

	ag = call->ag;

	if (bt_ag && bt_ag->terminate) {
		bt_ag->terminate(call);
	}

	bt_hfp_ag_set_call_state(call, BT_HFP_CALL_TERMINATE);

	if (!atomic_test_bit(ag->flags, BT_HFP_AG_AUDIO_CONN)) {
		LOG_DBG("It is not audio connection");
		hfp_ag_close_sco(ag);
	}
}

static int hfp_ag_send(struct bt_hfp_ag *ag, struct bt_ag_tx *tx)
{
	int err = bt_rfcomm_dlc_send(&ag->rfcomm_dlc, tx->buf);

	if (err < 0) {
		net_buf_unref(tx->buf);
	}

	return err;
}

static void bt_ag_tx_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_hfp_ag *ag = CONTAINER_OF(dwork, struct bt_hfp_ag, tx_work);
	sys_snode_t *node;
	struct bt_ag_tx *tx;
	bt_hfp_state_t state;

	hfp_ag_lock(ag);
	state = ag->state;
	if ((state == BT_HFP_DISCONNECTED) || (state == BT_HFP_DISCONNECTING)) {
		LOG_ERR("AG %p is not connected", ag);
		goto unlock;
	}

	node = sys_slist_peek_head(&ag->tx_pending);
	if (!node) {
		LOG_DBG("No pending tx");
		goto unlock;
	}

	tx = CONTAINER_OF(node, struct bt_ag_tx, node);

	if (!atomic_test_and_set_bit(ag->flags, BT_HFP_AG_TX_ONGOING)) {
		LOG_DBG("AG %p sending tx %p", ag, tx);
		int err = hfp_ag_send(ag, tx);

		if (err < 0) {
			LOG_ERR("Rfcomm send error :(%d)", err);
			sys_slist_find_and_remove(&ag->tx_pending, &tx->node);
			tx->err = err;
			k_fifo_put(&ag_tx_notify, tx);
			/* Clear the tx ongoing flag */
			if (!atomic_test_and_clear_bit(ag->flags, BT_HFP_AG_TX_ONGOING)) {
				LOG_WRN("tx ongoing flag is not set");
			}
			/* Due to the work is failed, restart the tx work */
			k_work_reschedule(&ag->tx_work, K_NO_WAIT);
		}
	}

unlock:
	hfp_ag_unlock(ag);
}

static void skip_space(struct net_buf *buf)
{
	size_t count = 0;

	while ((buf->len > count) && (buf->data[count] == ' ')) {
		count++;
	}

	if (count > 0) {
		(void)net_buf_pull(buf, count);
	}
}

static int get_number(struct net_buf *buf, uint32_t *number)
{
	int err = -EINVAL;

	*number = 0;

	skip_space(buf);
	while (buf->len > 0) {
		if ((buf->data[0] >= '0') && (buf->data[0] <= '9')) {
			*number = *number * 10 + buf->data[0] - '0';
			(void)net_buf_pull(buf, 1);
			err = 0;
		} else {
			break;
		}
	}
	skip_space(buf);

	return err;
}

static int get_char(struct net_buf *buf, char *c)
{
	int err = -EINVAL;

	skip_space(buf);
	if (buf->len > 0) {
		*c = (char)buf->data[0];
		(void)net_buf_pull(buf, 1);
		err = 0;
	}
	skip_space(buf);

	return err;
}

static bool is_char(struct net_buf *buf, uint8_t c)
{
	bool found = false;

	skip_space(buf);
	if (buf->len > 0) {
		if (buf->data[0] == c) {
			(void)net_buf_pull(buf, 1);
			found = true;
		}
	}
	skip_space(buf);

	return found;
}

static int bt_hfp_ag_brsf_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	uint32_t hf_features;
	int err;

	if (!is_char(buf, '=')) {
		return -ENOTSUP;
	}

	err = get_number(buf, &hf_features);
	if (err != 0) {
		return -ENOTSUP;
	}

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	ag->hf_features = hf_features;

	return hfp_ag_send_data(ag, NULL, NULL, "\r\n+BRSF:%d\r\n", ag->ag_features);
}

static int bt_hfp_ag_bac_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	uint32_t codec;
	uint32_t codec_ids = 0U;
	int err = 0;

	if (!is_char(buf, '=')) {
		return -ENOTSUP;
	}

	if (!BOTH_SUPT_FEAT(ag, BT_HFP_HF_FEATURE_CODEC_NEG, BT_HFP_AG_FEATURE_CODEC_NEG)) {
		return -ENOEXEC;
	}

	while (buf->len > 0) {
		err = get_number(buf, &codec);
		if (err != 0) {
			return -ENOTSUP;
		}

		if (!is_char(buf, ',')) {
			if (!is_char(buf, '\r')) {
				return -ENOTSUP;
			}
		}

		if (codec < NUM_BITS(sizeof(codec_ids))) {
			codec_ids |= BIT(codec);
		}
	}

	if (!(codec_ids & BIT(BT_HFP_AG_CODEC_CVSD))) {
		return -ENOEXEC;
	}

	hfp_ag_lock(ag);
	ag->hf_codec_ids = codec_ids;
	ag->selected_codec_id = 0;
	hfp_ag_unlock(ag);

	atomic_set_bit(ag->flags, BT_HFP_AG_CODEC_CHANGED);

	if (atomic_test_and_clear_bit(ag->flags, BT_HFP_AG_CODEC_CONN)) {
		/* Codec connection is ended. It needs to be restarted. */
		LOG_DBG("Codec connection is ended. It needs to be restarted.");
		if (bt_ag && bt_ag->codec_negotiate) {
			bt_ag->codec_negotiate(ag, -EAGAIN);
		}
	}

	if (bt_ag && bt_ag->codec) {
		bt_ag->codec(ag, ag->hf_codec_ids);
	}

	return 0;
}

static int bt_hfp_ag_cind_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	int err;
	bool inquiry_status = true;

	if (is_char(buf, '=')) {
		inquiry_status = false;
	}

	if (!is_char(buf, '?')) {
		return -ENOTSUP;
	}

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	if (!inquiry_status) {
		err = hfp_ag_send_data(
			ag, NULL, NULL,
			"\r\n+CIND:(\"%s\",(%d%s%d)),(\"%s\",(%d%s%d)),(\"%s\",(%d%s%d)),(\"%s\",(%"
			"d%s%d)),(\"%s\",(%d%s%d)),(\"%s\",(%d%s%d)),(\"%s\",(%d%s%d))\r\n",
			ag_ind[BT_HFP_AG_SERVICE_IND].name, ag_ind[BT_HFP_AG_SERVICE_IND].min,
			ag_ind[BT_HFP_AG_SERVICE_IND].connector, ag_ind[BT_HFP_AG_SERVICE_IND].max,
			ag_ind[BT_HFP_AG_CALL_IND].name, ag_ind[BT_HFP_AG_CALL_IND].min,
			ag_ind[BT_HFP_AG_CALL_IND].connector, ag_ind[BT_HFP_AG_CALL_IND].max,
			ag_ind[BT_HFP_AG_CALL_SETUP_IND].name, ag_ind[BT_HFP_AG_CALL_SETUP_IND].min,
			ag_ind[BT_HFP_AG_CALL_SETUP_IND].connector,
			ag_ind[BT_HFP_AG_CALL_SETUP_IND].max, ag_ind[BT_HFP_AG_CALL_HELD_IND].name,
			ag_ind[BT_HFP_AG_CALL_HELD_IND].min,
			ag_ind[BT_HFP_AG_CALL_HELD_IND].connector,
			ag_ind[BT_HFP_AG_CALL_HELD_IND].max, ag_ind[BT_HFP_AG_SIGNAL_IND].name,
			ag_ind[BT_HFP_AG_SIGNAL_IND].min, ag_ind[BT_HFP_AG_SIGNAL_IND].connector,
			ag_ind[BT_HFP_AG_SIGNAL_IND].max, ag_ind[BT_HFP_AG_ROAM_IND].name,
			ag_ind[BT_HFP_AG_ROAM_IND].min, ag_ind[BT_HFP_AG_ROAM_IND].connector,
			ag_ind[BT_HFP_AG_ROAM_IND].max, ag_ind[BT_HFP_AG_BATTERY_IND].name,
			ag_ind[BT_HFP_AG_BATTERY_IND].min, ag_ind[BT_HFP_AG_BATTERY_IND].connector,
			ag_ind[BT_HFP_AG_BATTERY_IND].max);
	} else {
		err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+CIND:%d,%d,%d,%d,%d,%d,%d\r\n",
					 ag->indicator_value[BT_HFP_AG_SERVICE_IND],
					 ag->indicator_value[BT_HFP_AG_CALL_IND],
					 ag->indicator_value[BT_HFP_AG_CALL_SETUP_IND],
					 ag->indicator_value[BT_HFP_AG_CALL_HELD_IND],
					 ag->indicator_value[BT_HFP_AG_SIGNAL_IND],
					 ag->indicator_value[BT_HFP_AG_ROAM_IND],
					 ag->indicator_value[BT_HFP_AG_BATTERY_IND]);
	}

	return err;
}

static void bt_hfp_ag_set_in_band_ring(struct bt_hfp_ag *ag, void *user_data)
{
	bool is_inband_ringtone;

	is_inband_ringtone = AG_SUPT_FEAT(ag, BT_HFP_AG_FEATURE_INBAND_RINGTONE) ? true : false;

	if (is_inband_ringtone && !atomic_test_bit(ag->flags, BT_HFP_AG_INBAND_RING)) {
		int err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+BSIR:1\r\n");

		atomic_set_bit_to(ag->flags, BT_HFP_AG_INBAND_RING, err == 0);
	}
}

static void bt_hfp_ag_slc_connected(struct bt_hfp_ag *ag, void *user_data)
{
	bt_hfp_ag_set_state(ag, BT_HFP_CONNECTED);
	(void)hfp_ag_next_step(ag, bt_hfp_ag_set_in_band_ring, NULL);
}

static int bt_hfp_ag_cmer_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	uint32_t number;
	int err;
	static const uint32_t command_line_prefix[] = {3, 0, 0};

	if (!is_char(buf, '=')) {
		return -ENOTSUP;
	}

	for (int i = 0; i < ARRAY_SIZE(command_line_prefix); i++) {
		err = get_number(buf, &number);
		if (err != 0) {
			return -ENOTSUP;
		}

		if (!is_char(buf, ',')) {
			return -ENOTSUP;
		}

		if (command_line_prefix[i] != number) {
			return -ENOTSUP;
		}
	}

	err = get_number(buf, &number);
	if (err != 0) {
		return -ENOTSUP;
	}

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	if (number == 1) {
		atomic_set_bit(ag->flags, BT_HFP_AG_CMER_ENABLE);
		if (BOTH_SUPT_FEAT(ag, BT_HFP_HF_FEATURE_3WAY_CALL, BT_HFP_AG_FEATURE_3WAY_CALL)) {
			LOG_DBG("Waiting for AT+CHLD=?");
			return 0;
		}

		err = hfp_ag_next_step(ag, bt_hfp_ag_slc_connected, NULL);
		return err;
	} else if (number == 0) {
		atomic_clear_bit(ag->flags, BT_HFP_AG_CMER_ENABLE);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static void bt_hfp_ag_accept_cb(struct bt_hfp_ag *ag, void *user_data)
{
	struct bt_hfp_ag_call *call = (struct bt_hfp_ag_call *)user_data;

	bt_hfp_ag_set_call_state(call, BT_HFP_CALL_ACTIVE);

	if (bt_ag && bt_ag->accept) {
		bt_ag->accept(call);
	}
}

#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
static int chld_release_all(struct bt_hfp_ag *ag)
{
	struct bt_hfp_ag_call *call;
	bt_hfp_call_state_t call_state;

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			continue;
		}

		hfp_ag_lock(ag);
		call_state = call->call_state;
		hfp_ag_unlock(ag);

		if ((call_state != BT_HFP_CALL_ACTIVE) ||
		    ((call_state == BT_HFP_CALL_ACTIVE) &&
		     atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING_HELD))) {
			ag_terminate_call(call);
		}
	}

	ag_notify_call_held_ind(ag, NULL);

	return 0;
}

static void bt_hfp_ag_accept_other_cb(struct bt_hfp_ag *ag, void *user_data)
{
	struct bt_hfp_ag_call *call = (struct bt_hfp_ag_call *)user_data;
	bt_hfp_call_state_t call_state;
	int err;

	if (!call) {
		return;
	}

	hfp_ag_lock(ag);
	call_state = call->call_state;
	hfp_ag_unlock(ag);

	bt_hfp_ag_set_call_state(call, BT_HFP_CALL_ACTIVE);

	if (call_state == BT_HFP_CALL_HOLD) {
		err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_HELD_IND,
					      BT_HFP_CALL_HELD_ACTIVE_HELD, NULL, NULL);
		if (err) {
			LOG_ERR("Fail to send err :(%d)", err);
		}
	}

	if (call_state == BT_HFP_CALL_HOLD) {
		if (bt_ag && bt_ag->retrieve) {
			bt_ag->retrieve(call);
		}
	} else {
		if (bt_ag && bt_ag->accept) {
			bt_ag->accept(call);
		}
	}

	ag_notify_call_held_ind(ag, NULL);
}

static void bt_hfp_ag_deactivate_accept_other_cb(struct bt_hfp_ag *ag, void *user_data)
{
	int err;
	uint8_t call_setup;

	hfp_ag_lock(ag);
	call_setup = ag->indicator_value[BT_HFP_AG_CALL_SETUP_IND];
	hfp_ag_unlock(ag);

	if (call_setup) {
		err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_NONE,
					      bt_hfp_ag_accept_other_cb, user_data);
		if (err) {
			LOG_ERR("Fail to send err :(%d)", err);
		}
	} else {
		bt_hfp_ag_accept_other_cb(ag, user_data);
	}
}

static struct bt_hfp_ag_call *get_none_accept_calls(struct bt_hfp_ag *ag)
{
	struct bt_hfp_ag_call *call;
	bt_hfp_call_state_t call_state;

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			continue;
		}

		hfp_ag_lock(ag);
		call_state = call->call_state;
		hfp_ag_unlock(ag);

		if ((call_state == BT_HFP_CALL_OUTGOING) || (call_state == BT_HFP_CALL_INCOMING) ||
		    (call_state == BT_HFP_CALL_ALERTING)) {
			return call;
		}
	}

	return NULL;
}

static struct bt_hfp_ag_call *get_none_active_calls(struct bt_hfp_ag *ag)
{
	struct bt_hfp_ag_call *call;
	bt_hfp_call_state_t call_state;

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			continue;
		}

		hfp_ag_lock(ag);
		call_state = call->call_state;
		hfp_ag_unlock(ag);

		if (call_state != BT_HFP_CALL_ACTIVE) {
			return call;
		} else if ((call_state == BT_HFP_CALL_ACTIVE) &&
			   atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING_HELD)) {
			return call;
		}
	}

	return NULL;
}

static int chld_deactivate_calls_and_accept_other_call(struct bt_hfp_ag *ag, bool release)
{
	struct bt_hfp_ag_call *call;
	bt_hfp_call_state_t call_state;
	struct bt_hfp_ag_call *none_active_call;

	none_active_call = get_none_accept_calls(ag);

	if (!none_active_call) {
		none_active_call = get_none_active_calls(ag);
	} else {
		if (!(atomic_test_bit(none_active_call->flags, BT_HFP_AG_CALL_INCOMING_3WAY) ||
		      atomic_test_bit(none_active_call->flags, BT_HFP_AG_CALL_INCOMING))) {
			return -ENOTSUP;
		}
	}

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			continue;
		}

		hfp_ag_lock(ag);
		call_state = call->call_state;
		hfp_ag_unlock(ag);

		if ((call_state == BT_HFP_CALL_ACTIVE) &&
		    !atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING_HELD)) {
			if (release) {
				ag_terminate_call(call);
			} else {
				bt_hfp_ag_set_call_state(call, BT_HFP_CALL_HOLD);

				if (bt_ag && bt_ag->held) {
					bt_ag->held(call);
				}
			}
		}
	}

	return hfp_ag_next_step(ag, bt_hfp_ag_deactivate_accept_other_cb, none_active_call);
}

static int chld_activate_held_call(struct bt_hfp_ag *ag)
{
	struct bt_hfp_ag_call *call;
	bt_hfp_call_state_t call_state;

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			continue;
		}

		hfp_ag_lock(ag);
		call_state = call->call_state;
		hfp_ag_unlock(ag);

		if (call_state == BT_HFP_CALL_HOLD) {
			bt_hfp_ag_set_call_state(call, BT_HFP_CALL_ACTIVE);

			if (bt_ag && bt_ag->retrieve) {
				bt_ag->retrieve(call);
			}
		}
	}

	ag_notify_call_held_ind(ag, NULL);

	return 0;
}

static int chld_drop_conversation(struct bt_hfp_ag *ag)
{
	struct bt_hfp_ag_call *call;
	bt_hfp_call_state_t call_state;

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			continue;
		}

		hfp_ag_lock(ag);
		call_state = call->call_state;
		hfp_ag_unlock(ag);

		if (call_state != BT_HFP_CALL_ACTIVE) {
			return -ENOTSUP;
		}
	}

	if (!bt_ag || !bt_ag->explicit_call_transfer) {
		return -ENOEXEC;
	}

	bt_ag->explicit_call_transfer(ag);

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			continue;
		}

		bt_hfp_ag_set_call_state(call, BT_HFP_CALL_TERMINATE);
	}

	if (!atomic_test_bit(ag->flags, BT_HFP_AG_AUDIO_CONN)) {
		LOG_DBG("Close SCO connection");
		hfp_ag_close_sco(ag);
	}

	ag_notify_call_held_ind(ag, NULL);

	return 0;
}

static int chld_release_call(struct bt_hfp_ag *ag, uint8_t call_index)
{
	struct bt_hfp_ag_call *call;
	bt_hfp_call_state_t call_state;

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			continue;
		}

		if (index != call_index) {
			continue;
		}

		hfp_ag_lock(ag);
		call_state = call->call_state;
		hfp_ag_unlock(ag);

		if ((call_state == BT_HFP_CALL_HOLD) ||
		    ((call_state == BT_HFP_CALL_ACTIVE) &&
		     !atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING_HELD))) {
			ag_terminate_call(call);
		} else {
			ag_reject_call(call);
		}
	}

	ag_notify_call_held_ind(ag, NULL);

	return 0;
}

static int chld_held_other_calls(struct bt_hfp_ag *ag, uint8_t call_index)
{
	struct bt_hfp_ag_call *call;
	bt_hfp_call_state_t call_state;

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			continue;
		}

		if (index == call_index) {
			continue;
		}

		hfp_ag_lock(ag);
		call_state = call->call_state;
		hfp_ag_unlock(ag);

		if ((call_state == BT_HFP_CALL_ACTIVE) &&
		    !atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING_HELD)) {
			bt_hfp_ag_set_call_state(call, BT_HFP_CALL_HOLD);

			if (bt_ag && bt_ag->held) {
				bt_ag->held(call);
			}
		}
	}

	call = &ag->calls[call_index];
	hfp_ag_lock(ag);
	call_state = call->call_state;
	hfp_ag_unlock(ag);

	if (call_state == BT_HFP_CALL_HOLD) {
		bt_hfp_ag_set_call_state(call, BT_HFP_CALL_ACTIVE);
		if (bt_ag && bt_ag->retrieve) {
			bt_ag->retrieve(call);
		}
	}

	ag_notify_call_held_ind(ag, NULL);

	return 0;
}

static int chld_other(struct bt_hfp_ag *ag, uint32_t value)
{
	uint8_t index;
	uint8_t command;

	index = value % 10;
	command = value / 10;

	if (!index) {
		return -ENOEXEC;
	}

	index = index - 1;

	if (index >= ARRAY_SIZE(ag->calls)) {
		return -ENOEXEC;
	}

	switch (command) {
	case BT_HFP_CHLD_RELEASE_ACTIVE_ACCEPT_OTHER:
		return chld_release_call(ag, index);
	case BT_HFP_CALL_HOLD_ACTIVE_ACCEPT_OTHER:
		return chld_held_other_calls(ag, index);
	}

	return -ENOEXEC;
}
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */

static int bt_hfp_ag_chld_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
	uint32_t value;
	int err;
	char *response;

	if (!BOTH_SUPT_FEAT(ag, BT_HFP_HF_FEATURE_3WAY_CALL, BT_HFP_AG_FEATURE_3WAY_CALL)) {
		return -ENOEXEC;
	}

	if (!is_char(buf, '=')) {
		return -ENOTSUP;
	}

	if (is_char(buf, '?')) {
		if (!is_char(buf, '\r')) {
			return -ENOTSUP;
		}

#if defined(CONFIG_BT_HFP_AG_ECC)
		response = "+CHLD:(0,1,1x,2,2x,3,4)";
#else
		response = "+CHLD:(0,1,2,3,4)";
#endif /* CONFIG_BT_HFP_AG_ECC */
		err = hfp_ag_send_data(ag, bt_hfp_ag_slc_connected, NULL, "\r\n%s\r\n", response);
		return err;
	}

	err = get_number(buf, &value);
	if (err != 0) {
		return -ENOTSUP;
	}

	switch (value) {
	case BT_HFP_CHLD_RELEASE_ALL:
		return chld_release_all(ag);
	case BT_HFP_CHLD_RELEASE_ACTIVE_ACCEPT_OTHER:
		return chld_deactivate_calls_and_accept_other_call(ag, true);
	case BT_HFP_CALL_HOLD_ACTIVE_ACCEPT_OTHER:
		return chld_deactivate_calls_and_accept_other_call(ag, false);
	case BT_HFP_CALL_ACTIVE_HELD:
		return chld_activate_held_call(ag);
	case BT_HFP_CALL_QUITE:
		return chld_drop_conversation(ag);
	}

	return chld_other(ag, value);
#else
	return -ENOEXEC;
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */
}

static int bt_hfp_ag_bind_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	uint32_t indicator;
	uint32_t hf_indicators = 0U;
	uint32_t supported_indicators = 0U;
	int err;
	char *data;
	uint32_t len;

	if (!BOTH_SUPT_FEAT(ag, BT_HFP_HF_FEATURE_HF_IND, BT_HFP_AG_FEATURE_HF_IND)) {
		return -ENOEXEC;
	}

	if (is_char(buf, '?')) {
		if (!is_char(buf, '\r')) {
			return -ENOTSUP;
		}

		hf_indicators = ag->hf_indicators;
		supported_indicators = ag->hf_indicators_of_ag & ag->hf_indicators_of_hf;
		len = MIN(NUM_BITS(sizeof(supported_indicators)), HFP_HF_IND_MAX);
		for (int i = 1; i < len; i++) {
			bool enabled;

			if (!(BIT(i) & supported_indicators)) {
				continue;
			}

			enabled = BIT(i) & hf_indicators;
			err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+BIND:%d,%d\r\n", i,
				enabled ? 1 : 0);
			if (err) {
				return err;
			}

			supported_indicators &= ~BIT(i);

			if (!supported_indicators) {
				break;
			}
		}
		return 0;
	}

	if (!is_char(buf, '=')) {
		return -ENOTSUP;
	}

	if (is_char(buf, '?')) {
		if (!is_char(buf, '\r')) {
			return -ENOTSUP;
		}

		data = &ag->buffer[0];
		*data = '(';
		data++;
		hf_indicators = ag->hf_indicators_of_ag;
		len = MIN(NUM_BITS(sizeof(hf_indicators)), HFP_HF_IND_MAX);
		for (int i = 1; (i < len) && (hf_indicators != 0); i++) {
			if (BIT(i) & hf_indicators) {
				int length = snprintk(
					data, (char *)&ag->buffer[HF_MAX_BUF_LEN - 1] - data - 2,
					"%d", i);
				data += length;
				hf_indicators &= ~BIT(i);
			}
			if (hf_indicators != 0) {
				*data = ',';
				data++;
			}
		}
		*data = ')';
		data++;
		*data = '\0';
		data++;

		err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+BIND:%s\r\n", &ag->buffer[0]);
		return err;
	}

	while (buf->len > 0) {
		err = get_number(buf, &indicator);
		if (err != 0) {
			return -ENOTSUP;
		}

		if (!is_char(buf, ',')) {
			if (!is_char(buf, '\r')) {
				return -ENOTSUP;
			}
		}

		if (indicator < NUM_BITS(sizeof(hf_indicators))) {
			hf_indicators |= BIT(indicator);
		}
	}

	ag->hf_indicators_of_hf = hf_indicators;

	return 0;
}

static int bt_hfp_ag_cmee_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	uint32_t cmee;
	int err;

	if (!is_char(buf, '=')) {
		return -ENOTSUP;
	}

	err = get_number(buf, &cmee);
	if (err != 0) {
		return -ENOTSUP;
	}

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	if (cmee > 1) {
		return -ENOTSUP;
	}

	if (BT_HFP_AG_FEATURE_EXT_ERR_ENABLE) {
		atomic_set_bit_to(ag->flags, BT_HFP_AG_CMEE_ENABLE, cmee == 1);
		return 0;
	}

	return -ENOTSUP;
}

static void bt_hfp_ag_reject_cb(struct bt_hfp_ag *ag, void *user_data)
{
	struct bt_hfp_ag_call *call = (struct bt_hfp_ag_call *)user_data;

	if (call) {
		ag_reject_call(call);
	}

	if (!atomic_test_bit(ag->flags, BT_HFP_AG_AUDIO_CONN)) {
		LOG_DBG("It is not audio connection");
		hfp_ag_close_sco(ag);
	}
}

static void bt_hfp_ag_call_reject(struct bt_hfp_ag *ag, void *user_data)
{
	int err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_NONE,
					    bt_hfp_ag_reject_cb, user_data);
	if (err != 0) {
		LOG_ERR("Fail to send err :(%d)", err);
	}
}

static void bt_hfp_ag_terminate_cb(struct bt_hfp_ag *ag, void *user_data)
{
	struct bt_hfp_ag_call *call = (struct bt_hfp_ag_call *)user_data;

	if (call) {
		ag_terminate_call(call);
	}

	if (!atomic_test_bit(ag->flags, BT_HFP_AG_AUDIO_CONN)) {
		LOG_DBG("It is not audio connection");
		hfp_ag_close_sco(ag);
	}
}

static void bt_hfp_ag_unit_call_terminate(struct bt_hfp_ag *ag, void *user_data)
{
	int err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 0, bt_hfp_ag_terminate_cb,
					    user_data);
	if (err != 0) {
		LOG_ERR("Fail to send err :(%d)", err);
	}
}

static void bt_hfp_ag_active_terminate_cb(struct bt_hfp_ag *ag, void *user_data)
{
	bt_hfp_ag_unit_call_terminate(ag, user_data);
}

static void bt_hfp_ag_call_terminate(struct bt_hfp_ag *ag, void *user_data)
{
	int err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_NONE,
					  bt_hfp_ag_active_terminate_cb, user_data);
	if (err) {
		LOG_ERR("Fail to send err :(%d)", err);
	}
}

struct bt_hfp_ag_call *get_call_with_flag(struct bt_hfp_ag *ag, int bit, bool clear)
{
	struct bt_hfp_ag_call *call;

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			continue;
		}

		if (clear) {
			if (atomic_test_and_set_bit(call->flags, bit)) {
				return call;
			}
		} else {
			if (atomic_test_bit(call->flags, bit)) {
				return call;
			}
		}
	}

	return NULL;
}

struct bt_hfp_ag_call *get_call_with_flag_and_state(struct bt_hfp_ag *ag, int bit,
						    bt_hfp_call_state_t state)
{
	struct bt_hfp_ag_call *call;

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			continue;
		}

		if ((call->call_state & state) && atomic_test_bit(call->flags, bit)) {
			return call;
		}
	}

	return NULL;
}

static int bt_hfp_ag_chup_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	int err;
	bt_hfp_call_state_t call_state;
	struct bt_hfp_ag_call *call;
	int call_count;

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	call_count = get_none_released_calls(ag);

	if (call_count == 1) {
		bt_hfp_ag_tx_cb_t next_step = NULL;

		call = get_call_with_flag(ag, BT_HFP_AG_CALL_IN_USING, false);
		if (!call) {
			return 0;
		}

		hfp_ag_lock(ag);
		call_state = call->call_state;
		hfp_ag_unlock(ag);

		if (call_state == BT_HFP_CALL_ALERTING) {
			if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING)) {
				next_step = bt_hfp_ag_call_terminate;
			} else {
				next_step = bt_hfp_ag_call_reject;
			}
		} else if (call_state == BT_HFP_CALL_ACTIVE) {
			next_step = bt_hfp_ag_unit_call_terminate;
		}
		if (next_step) {
			err = hfp_ag_next_step(ag, next_step, call);
			return err;
		}

		return -ENOTSUP;
	}

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			continue;
		}

		hfp_ag_lock(ag);
		call_state = call->call_state;
		hfp_ag_unlock(ag);

		if (call_state == BT_HFP_CALL_ACTIVE) {
			err = hfp_ag_next_step(ag, bt_hfp_ag_unit_call_terminate, call);
		}
	}
	return 0;
}

static uint8_t bt_hfp_get_call_state(struct bt_hfp_ag_call *call)
{
	uint8_t status = 0;
	struct bt_hfp_ag *ag = call->ag;

	hfp_ag_lock(ag);
	switch (call->call_state) {
	case BT_HFP_CALL_TERMINATE:
		if (atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING)) {
			status = BT_HFP_CLCC_STATUS_INCOMING;
		} else {
			status = BT_HFP_CLCC_STATUS_DIALING;
		}
		break;
	case BT_HFP_CALL_OUTGOING:
		status = BT_HFP_CLCC_STATUS_DIALING;
		break;
	case BT_HFP_CALL_INCOMING:
		status = BT_HFP_CLCC_STATUS_INCOMING;
		break;
	case BT_HFP_CALL_ALERTING:
		if (atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING)) {
			status = BT_HFP_CLCC_STATUS_WAITING;
		} else {
			status = BT_HFP_CLCC_STATUS_ALERTING;
		}
		break;
	case BT_HFP_CALL_ACTIVE:
		if (atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING_HELD)) {
			status = BT_HFP_CLCC_STATUS_CALL_HELD_HOLD;
		} else {
			status = BT_HFP_CLCC_STATUS_ACTIVE;
		}
		break;
	case BT_HFP_CALL_HOLD:
		status = BT_HFP_CLCC_STATUS_HELD;
		break;
	default:
		break;
	}
	hfp_ag_unlock(ag);

	return status;
}

static int bt_hfp_ag_clcc_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	int err;
	struct bt_hfp_ag_call *call;
	uint8_t dir;
	uint8_t status;
	uint8_t mode;
	uint8_t mpty;
	int active_calls;

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	active_calls = get_active_calls(ag);

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			continue;
		}

		dir = atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING) ? 1 : 0;
		status = bt_hfp_get_call_state(call);
		mode = 0;
		mpty = (status == BT_HFP_CLCC_STATUS_ACTIVE) && (active_calls > 1) ? 1 : 0;
		err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+CLCC:%d,%d,%d,%d,%d,\"%s\",%d\r\n",
				       index + 1, dir, status, mode, mpty, call->number,
				       call->type);
		if (err) {
			LOG_ERR("Fail to send err :(%d)", err);
		}
	}

	/* AG shall always send OK response to HF */
	return 0;
}

static int bt_hfp_ag_bia_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	uint32_t number;
	int err;
	int index = 0;
	uint32_t indicator;

	if (!is_char(buf, '=')) {
		return -ENOTSUP;
	}

	hfp_ag_lock(ag);
	indicator = ag->indicator;
	hfp_ag_unlock(ag);

	while (buf->len > 0) {
		err = get_number(buf, &number);
		if (err == 0) {
			/* Valid number */
			if (number) {
				indicator |= BIT(index);
			} else {
				indicator &= ~BIT(index);
			}
		}

		if (is_char(buf, ',')) {
			index++;
		} else {
			if (!is_char(buf, '\r')) {
				return -ENOTSUP;
			}
			if (buf->len != 0) {
				return -ENOTSUP;
			}
		}
	}

	/* Force call, call setup and held call indicators are enabled. */
	indicator |= BIT(BT_HFP_AG_CALL_IND) | BIT(BT_HFP_AG_CALL_SETUP_IND) |
		    BIT(BT_HFP_AG_CALL_HELD_IND);

	hfp_ag_lock(ag);
	ag->indicator = indicator;
	hfp_ag_unlock(ag);

	return 0;
}

static void bt_hfp_ag_call_ringing_cb(struct bt_hfp_ag_call *call, bool in_bond)
{
	if (bt_ag && bt_ag->ringing) {
		bt_ag->ringing(call, in_bond);
	}
}

static void hfp_ag_sco_connected(struct bt_sco_chan *chan)
{
	struct bt_hfp_ag *ag = CONTAINER_OF(chan, struct bt_hfp_ag, sco_chan);
	bt_hfp_call_state_t call_state;
	struct bt_hfp_ag_call *call;

	call = get_call_with_flag(ag, BT_HFP_AG_CALL_OPEN_SCO, true);

	if (call) {
		hfp_ag_lock(ag);
		call_state = call->call_state;
		hfp_ag_unlock(ag);
		if (call_state == BT_HFP_CALL_INCOMING) {
			bt_hfp_ag_set_call_state(call, BT_HFP_CALL_ALERTING);
			bt_hfp_ag_call_ringing_cb(call, true);
		}
	}

	if ((bt_ag) && bt_ag->sco_connected) {
		bt_ag->sco_connected(ag, chan->sco);
	}
}

static void hfp_ag_sco_disconnected(struct bt_sco_chan *chan, uint8_t reason)
{
	struct bt_hfp_ag *ag = CONTAINER_OF(chan, struct bt_hfp_ag, sco_chan);
	bt_hfp_call_state_t call_state;
	struct bt_hfp_ag_call *call;

	call = get_call_with_flag(ag, BT_HFP_AG_CALL_OPEN_SCO, true);

	if ((bt_ag != NULL) && bt_ag->sco_disconnected) {
		bt_ag->sco_disconnected(chan->sco, reason);
	}

	if (!call) {
		return;
	}

	hfp_ag_lock(ag);
	call_state = call->call_state;
	hfp_ag_unlock(ag);
	if ((call_state == BT_HFP_CALL_INCOMING) || (call_state == BT_HFP_CALL_OUTGOING)) {
		bt_hfp_ag_call_reject(ag, call);
	}
}

static struct bt_conn *bt_hfp_ag_create_sco(struct bt_hfp_ag *ag)
{
	struct bt_conn *sco_conn;

	static struct bt_sco_chan_ops ops = {
		.connected = hfp_ag_sco_connected,
		.disconnected = hfp_ag_sco_disconnected,
	};

	LOG_DBG("");

	if (ag->sco_chan.sco == NULL) {
		ag->sco_chan.ops = &ops;

		/* create SCO connection*/
		sco_conn = bt_conn_create_sco(&ag->acl_conn->br.dst, &ag->sco_chan);
		if (sco_conn != NULL) {
			LOG_DBG("Created sco %p", sco_conn);
			bt_conn_unref(sco_conn);
		}
	} else {
		sco_conn = ag->sco_chan.sco;
	}

	return sco_conn;
}

static int hfp_ag_open_sco(struct bt_hfp_ag *ag, struct bt_hfp_ag_call *call)
{
	bool create_sco;
	bt_hfp_call_state_t call_state;

	if (atomic_test_bit(ag->flags, BT_HFP_AG_CREATING_SCO)) {
		LOG_WRN("SCO connection is creating!");
		return 0;
	}

	hfp_ag_lock(ag);
	create_sco = (ag->sco_chan.sco == NULL) ? true : false;
	if (create_sco) {
		atomic_set_bit(ag->flags, BT_HFP_AG_CREATING_SCO);
	}
	hfp_ag_unlock(ag);

	if (create_sco) {
		struct bt_conn *sco_conn = bt_hfp_ag_create_sco(ag);

		atomic_clear_bit(ag->flags, BT_HFP_AG_CREATING_SCO);
		atomic_clear_bit(ag->flags, BT_HFP_AG_AUDIO_CONN);

		if (sco_conn == NULL) {
			LOG_ERR("Fail to create sco connection!");
			return -ENOTCONN;
		}

		atomic_set_bit_to(ag->flags, BT_HFP_AG_AUDIO_CONN, call == NULL);
		if (call) {
			atomic_set_bit(call->flags, BT_HFP_AG_CALL_OPEN_SCO);
		}

		LOG_DBG("SCO connection created (%p)", sco_conn);
	} else {
		if (call) {
			hfp_ag_lock(ag);
			call_state = call->call_state;
			hfp_ag_unlock(ag);

			if (call_state == BT_HFP_CALL_INCOMING) {
				bt_hfp_ag_set_call_state(call, BT_HFP_CALL_ALERTING);
				bt_hfp_ag_call_ringing_cb(call, true);
			}
		}
	}

	return 0;
}

static int bt_hfp_ag_codec_select(struct bt_hfp_ag *ag)
{
	int err;

	LOG_DBG("");

	hfp_ag_lock(ag);
	if (ag->selected_codec_id == 0) {
		LOG_WRN("Codec is invalid, set default value");
		ag->selected_codec_id = BT_HFP_AG_CODEC_CVSD;
	}

	if (!(ag->hf_codec_ids & BIT(ag->selected_codec_id))) {
		LOG_ERR("Codec is unsupported (codec id %d)", ag->selected_codec_id);
		hfp_ag_unlock(ag);
		return -EINVAL;
	}
	hfp_ag_unlock(ag);

	err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+BCS:%d\r\n", ag->selected_codec_id);
	if (err != 0) {
		LOG_ERR("Fail to send err :(%d)", err);
	}
	return err;
}

static int bt_hfp_ag_create_audio_connection(struct bt_hfp_ag *ag, struct bt_hfp_ag_call *call)
{
	int err;
	uint32_t hf_codec_ids;

	hfp_ag_lock(ag);
	hf_codec_ids = ag->hf_codec_ids;
	hfp_ag_unlock(ag);

	if ((hf_codec_ids != 0) && atomic_test_bit(ag->flags, BT_HFP_AG_CODEC_CHANGED)) {
		err = bt_hfp_ag_codec_select(ag);
		atomic_set_bit_to(ag->flags, BT_HFP_AG_CODEC_CONN, err == 0);
		if (call) {
			atomic_set_bit_to(call->flags, BT_HFP_AG_CALL_OPEN_SCO, err == 0);
		}
	} else {
		err = hfp_ag_open_sco(ag, call);
	}

	return err;
}

static void bt_hfp_ag_audio_connection(struct bt_hfp_ag *ag, void *user_data)
{
	int err;
	struct bt_hfp_ag_call *call = (struct bt_hfp_ag_call *)user_data;

	err = bt_hfp_ag_create_audio_connection(ag, call);
	if (err) {
		bt_hfp_ag_unit_call_terminate(ag, user_data);
	}
}

static void bt_hfp_ag_call_setup_none_cb(struct bt_hfp_ag *ag, void *user_data)
{
	int err;
	struct bt_hfp_ag_call *call = (struct bt_hfp_ag_call *)user_data;

	if (atomic_test_bit(call->flags, BT_HFP_AG_CALL_OUTGOING_3WAY)) {
		err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_HELD_IND,
					      BT_HFP_CALL_HELD_ACTIVE_HELD, bt_hfp_ag_accept_cb,
					      call);
		if (err) {
			bt_hfp_ag_unit_call_terminate(ag, user_data);
		}
		return;
	}

	bt_hfp_ag_audio_connection(ag, user_data);
}

static void bt_hfp_ag_unit_call_accept(struct bt_hfp_ag *ag, void *user_data)
{
	int err;

	err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 1, bt_hfp_ag_accept_cb, user_data);
	if (err) {
		LOG_ERR("Fail to send err :(%d)", err);
	}

	err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_NONE,
				      bt_hfp_ag_call_setup_none_cb, user_data);
	if (err) {
		LOG_ERR("Fail to send err :(%d)", err);
	}
}

static int bt_hfp_ag_ata_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	int err;
	int call_count;
	struct bt_hfp_ag_call *call;

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	hfp_ag_lock(ag);
	call_count = get_none_released_calls(ag);
	if (call_count != 1) {
		hfp_ag_unlock(ag);
		return -ENOTSUP;
	}

	call = get_call_with_flag(ag, BT_HFP_AG_CALL_IN_USING, false);
	__ASSERT(call, "Invalid call object");

	if (call->call_state != BT_HFP_CALL_ALERTING) {
		hfp_ag_unlock(ag);
		return -ENOTSUP;
	}
	hfp_ag_unlock(ag);

	if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING)) {
		return -ENOTSUP;
	}

	err = hfp_ag_next_step(ag, bt_hfp_ag_unit_call_accept, call);

	return err;
}

static int bt_hfp_ag_cops_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	int err;
	uint32_t number;

	if (is_char(buf, '=')) {
		static const uint32_t command_line_prefix[] = {3, 0};

		for (int i = 0; i < ARRAY_SIZE(command_line_prefix); i++) {
			err = get_number(buf, &number);
			if (err != 0) {
				return -ENOTSUP;
			}

			if (command_line_prefix[i] != number) {
				return -ENOTSUP;
			}

			if (!is_char(buf, ',')) {
				if (!is_char(buf, '\r')) {
					return -ENOTSUP;
				}
			}
		}

		atomic_set_bit(ag->flags, BT_HFP_AG_COPS_SET);
		return 0;
	}

	if (!atomic_test_bit(ag->flags, BT_HFP_AG_COPS_SET)) {
		return -ENOTSUP;
	}

	if (!is_char(buf, '?')) {
		return -ENOTSUP;
	}

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+COPS:%d,%d,\"%s\"\r\n", ag->mode, 0,
			       ag->operator);
	if (err) {
		LOG_ERR("Fail to send err :(%d)", err);
	}

	return err;
}

static int bt_hfp_ag_bcc_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
#if BT_HFP_AG_FEATURE_CODEC_NEG_ENABLE
	int err;

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	hfp_ag_lock(ag);
	if (ag->selected_codec_id && (!(ag->hf_codec_ids & BIT(ag->selected_codec_id)))) {
		hfp_ag_unlock(ag);
		return -ENOTSUP;
	}

	if (ag->sco_chan.sco) {
		hfp_ag_unlock(ag);
		return -ECONNREFUSED;
	}
	hfp_ag_unlock(ag);

	if (bt_ag && bt_ag->audio_connect_req) {
		bt_ag->audio_connect_req(ag);
		return 0;
	}

	err = hfp_ag_next_step(ag, bt_hfp_ag_audio_connection, NULL);

	return err;
#else
	return -ENOTSUP;
#endif /* BT_HFP_AG_FEATURE_CODEC_NEG_ENABLE */
}

static void bt_hfp_ag_unit_codec_conn_setup(struct bt_hfp_ag *ag, void *user_data)
{
	int err = hfp_ag_open_sco(ag, user_data);

	if (err) {
		bt_hfp_ag_call_reject(ag, user_data);
	}
}

static int bt_hfp_ag_bcs_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	int err;
	uint32_t number;
	bool codec_conn;
	struct bt_hfp_ag_call *call;

	if (!is_char(buf, '=')) {
		return -ENOTSUP;
	}

	err = get_number(buf, &number);
	if (err != 0) {
		return -ENOTSUP;
	}

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	if (!atomic_test_bit(ag->flags, BT_HFP_AG_CODEC_CONN)) {
		return -ESRCH;
	}

	hfp_ag_lock(ag);
	if (ag->selected_codec_id != number) {
		LOG_ERR("Received codec id %d is not aligned with selected %d", number,
			ag->selected_codec_id);
		err = -ENOTSUP;
	} else if (!(ag->hf_codec_ids & BIT(ag->selected_codec_id))) {
		LOG_ERR("Selected codec id %d is unsupported %d", ag->selected_codec_id,
			ag->hf_codec_ids);
		err = -ENOTSUP;
	}
	hfp_ag_unlock(ag);

	codec_conn = atomic_test_and_clear_bit(ag->flags, BT_HFP_AG_CODEC_CONN);
	atomic_clear_bit(ag->flags, BT_HFP_AG_CODEC_CHANGED);

	call = get_call_with_flag(ag, BT_HFP_AG_CALL_OPEN_SCO, false);

	if (err == 0) {
		if (codec_conn && bt_ag && bt_ag->codec_negotiate) {
			bt_ag->codec_negotiate(ag, err);
		}
		err = hfp_ag_next_step(ag, bt_hfp_ag_unit_codec_conn_setup, call);
	} else {
		bt_hfp_call_state_t call_state;

		if (codec_conn && bt_ag && bt_ag->codec_negotiate) {
			bt_ag->codec_negotiate(ag, err);
		}

		if (call) {
			hfp_ag_lock(ag);
			call_state = call->call_state;
			hfp_ag_unlock(ag);
			if (call_state != BT_HFP_CALL_TERMINATE) {
				(void)hfp_ag_next_step(ag, bt_hfp_ag_unit_call_terminate, call);
			}
		}
	}

	return err;
}

static void bt_hfp_ag_outgoing_cb(struct bt_hfp_ag *ag, void *user_data)
{
	struct bt_hfp_ag_call *call = (struct bt_hfp_ag_call *)user_data;

	bt_hfp_ag_set_call_state(call, BT_HFP_CALL_OUTGOING);

	if (bt_ag && bt_ag->outgoing) {
		bt_ag->outgoing(ag, call, call->number);
	}

	if (atomic_test_bit(ag->flags, BT_HFP_AG_INBAND_RING) ||
	    atomic_test_bit(call->flags, BT_HFP_AG_CALL_OUTGOING_3WAY)) {
		int err;

		err = bt_hfp_ag_create_audio_connection(ag, call);
		if (err) {
			bt_hfp_ag_call_reject(ag, user_data);
		}
	}
}

static void bt_hfp_ag_unit_call_outgoing(struct bt_hfp_ag *ag, void *user_data)
{
	struct bt_hfp_ag_call *call = (struct bt_hfp_ag_call *)user_data;
	int err;

	err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_OUTGOING,
				      bt_hfp_ag_outgoing_cb, call);

	if (err) {
		LOG_ERR("Fail to send err :(%d)", err);
	}
}

#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
static void bt_hfp_ag_call_held_cb(struct bt_hfp_ag *ag, void *user_data)
{
	struct bt_hfp_ag_call *call = (struct bt_hfp_ag_call *)user_data;
	bt_hfp_call_state_t call_state;

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			continue;
		}

		hfp_ag_lock(ag);
		call_state = call->call_state;
		hfp_ag_unlock(ag);

		if ((call_state == BT_HFP_CALL_ACTIVE) &&
		    !atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING_HELD)) {
			bt_hfp_ag_set_call_state(call, BT_HFP_CALL_HOLD);
			if (bt_ag && bt_ag->held) {
				bt_ag->held(call);
			}
		}
	}

	bt_hfp_ag_unit_call_outgoing(ag, user_data);
}

static void bt_hfp_ag_unit_call_outgoing_3way(struct bt_hfp_ag *ag, void *user_data)
{
	struct bt_hfp_ag_call *call = (struct bt_hfp_ag_call *)user_data;
	int err;
	uint8_t old_value;

	atomic_set_bit(call->flags, BT_HFP_AG_CALL_OUTGOING_3WAY);

	hfp_ag_lock(ag);
	old_value = ag->indicator_value[BT_HFP_AG_CALL_HELD_IND];
	hfp_ag_unlock(ag);
	if (old_value == BT_HFP_CALL_HELD_HELD) {
		LOG_WRN("No active call");
		bt_hfp_ag_call_held_cb(ag, user_data);
		return;
	}

	err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_HELD_IND, BT_HFP_CALL_HELD_HELD,
				      bt_hfp_ag_call_held_cb, call);
	if (err) {
		LOG_ERR("Fail to send err :(%d)", err);
	}
}
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */

static int bt_hfp_ag_outgoing_call(struct bt_hfp_ag *ag, const char *number, uint8_t type)
{
	int err;
	size_t len;
	struct bt_hfp_ag_call *call;
	int call_count;

	len = strlen(number);
	if (len == 0) {
		return -ENOTSUP;
	}

	if (len > CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN) {
		return -ENAMETOOLONG;
	}

	hfp_ag_lock(ag);
	(void)strcpy(ag->last_number, number);
	ag->type = type;
	hfp_ag_unlock(ag);

	call = get_call_from_number(ag, number, type);
	if (call) {
		return -EBUSY;
	}

	call_count = get_none_released_calls(ag);

	if (call_count) {
#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
		if (!BOTH_SUPT_FEAT(ag, BT_HFP_HF_FEATURE_3WAY_CALL,
				    BT_HFP_AG_FEATURE_3WAY_CALL)) {
			return -ENOEXEC;
		}

		if (!get_active_held_calls(ag)) {
			LOG_WRN("The first call is not accepted");
			return -EBUSY;
		}
#else
		return -EBUSY;
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */
	}

	call = get_new_call(ag, number, type);
	if (!call) {
		LOG_WRN("Cannot allocate a new call object");
		return -ENOMEM;
	}

	atomic_clear_bit(call->flags, BT_HFP_AG_CALL_INCOMING);
	bt_hfp_ag_set_call_state(call, BT_HFP_CALL_OUTGOING);

#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
	if (call_count) {
		err = hfp_ag_next_step(ag, bt_hfp_ag_unit_call_outgoing_3way, call);
		if (err) {
			free_call(call);
		}
		return err;
	}
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */

	err = hfp_ag_next_step(ag, bt_hfp_ag_unit_call_outgoing, call);
	if (err) {
		free_call(call);
	}
	return err;
}

static int bt_hfp_ag_atd_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	int err;
	char *number = NULL;
	bool is_memory_dial = false;

	if (buf->data[buf->len - 1] != '\r') {
		return -ENOTSUP;
	}

	if (is_char(buf, '>')) {
		is_memory_dial = true;
	}

	if ((buf->len - 1) > CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN) {
		return -ENAMETOOLONG;
	}

	buf->data[buf->len - 1] = '\0';

	if (is_memory_dial) {
		if (bt_ag && bt_ag->memory_dial) {
			err = bt_ag->memory_dial(ag, &buf->data[0], &number);
			if ((err != 0) || (number == NULL)) {
				return -ENOTSUP;
			}
		} else {
			return -ENOTSUP;
		}
	} else {
		number = &buf->data[0];
		if (bt_ag && bt_ag->number_call) {
			err = bt_ag->number_call(ag, &buf->data[0]);
			if (err) {
				return err;
			}
		} else {
			return -ENOTSUP;
		}
	}

	return bt_hfp_ag_outgoing_call(ag, number, 0);
}

static int bt_hfp_ag_bldn_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	return bt_hfp_ag_outgoing_call(ag, ag->last_number, ag->type);
}

static int bt_hfp_ag_clip_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	int err;
	uint32_t clip;

	if (!is_char(buf, '=')) {
		return -ENOTSUP;
	}

	err = get_number(buf, &clip);
	if (err != 0) {
		return -ENOTSUP;
	}

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	if (clip > 1) {
		return -ENOTSUP;
	}

	atomic_set_bit_to(ag->flags, BT_HFP_AG_CLIP_ENABLE, clip == 1);

	return err;
}

static int bt_hfp_ag_vgm_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	int err;
	uint32_t vgm;

	if (!is_char(buf, '=')) {
		return -ENOTSUP;
	}

	err = get_number(buf, &vgm);
	if (err != 0) {
		return -ENOTSUP;
	}

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	if (vgm > BT_HFP_HF_VGM_GAIN_MAX) {
		LOG_ERR("Invalid vgm (%d>%d)", vgm, BT_HFP_HF_VGM_GAIN_MAX);
		return -ENOTSUP;
	}

	if (bt_ag && bt_ag->vgm) {
		bt_ag->vgm(ag, (uint8_t)vgm);
	}
	return err;
}

static int bt_hfp_ag_vgs_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	int err;
	uint32_t vgs;

	if (!is_char(buf, '=')) {
		return -ENOTSUP;
	}

	err = get_number(buf, &vgs);
	if (err != 0) {
		return -ENOTSUP;
	}

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	if (vgs > BT_HFP_HF_VGS_GAIN_MAX) {
		LOG_ERR("Invalid vgs (%d>%d)", vgs, BT_HFP_HF_VGS_GAIN_MAX);
		return -ENOTSUP;
	}

	if (bt_ag && bt_ag->vgs) {
		bt_ag->vgs(ag, (uint8_t)vgs);
	}
	return err;
}

static int bt_hfp_ag_nrec_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	int err;
	uint32_t disable;

	if (!is_char(buf, '=')) {
		return -ENOTSUP;
	}

	err = get_number(buf, &disable);
	if (err != 0) {
		return -ENOTSUP;
	}

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	if (!AG_SUPT_FEAT(ag, BT_HFP_AG_FEATURE_ECNR)) {
		return -ENOTSUP;
	}

	if (disable) {
		LOG_ERR("Only support EC NR disable");
		return -ENOTSUP;
	}

#if defined(CONFIG_BT_HFP_AG_ECNR)
	if (bt_ag && bt_ag->ecnr_turn_off) {
		bt_ag->ecnr_turn_off(ag);
		return 0;
	}
#endif /* CONFIG_BT_HFP_AG_ECNR */
	return -ENOTSUP;
}

static void btrh_held_call_updated_cb(struct bt_hfp_ag *ag, void *user_data)
{
	struct bt_hfp_ag_call *call = (struct bt_hfp_ag_call *)user_data;

	bt_hfp_ag_set_call_state(call, BT_HFP_CALL_ACTIVE);
}

static void btrh_held_call_setup_updated_cb(struct bt_hfp_ag *ag, void *user_data)
{
	struct bt_hfp_ag_call *call = (struct bt_hfp_ag_call *)user_data;

	if (bt_ag && bt_ag->incoming_held) {
		bt_ag->incoming_held(call);
	}
}

static void btrh_held_cb(struct bt_hfp_ag *ag, void *user_data)
{
	int err;

	err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 1, btrh_held_call_updated_cb,
				      user_data);
	if (err) {
		LOG_ERR("Fail to send err :(%d)", err);
		bt_hfp_ag_unit_call_terminate(ag, user_data);
	}

	err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_NONE,
				      btrh_held_call_setup_updated_cb, user_data);
	if (err) {
		LOG_ERR("Fail to send err :(%d)", err);
		bt_hfp_ag_unit_call_terminate(ag, user_data);
	}
}

static void btrh_accept_cb(struct bt_hfp_ag *ag, void *user_data)
{
	int err;
	struct bt_hfp_ag_call *call = (struct bt_hfp_ag_call *)user_data;

	if (bt_ag && bt_ag->accept) {
		bt_ag->accept(call);
	}

	err = bt_hfp_ag_create_audio_connection(ag, call);
	if (err) {
		bt_hfp_ag_unit_call_terminate(ag, user_data);
	}
}

static void btrh_reject_cb(struct bt_hfp_ag *ag, void *user_data)
{
	int err;

	err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 0, bt_hfp_ag_reject_cb, user_data);
	if (err) {
		bt_hfp_ag_unit_call_terminate(ag, user_data);
	}
}

static int bt_hfp_ag_btrh_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	int err;
	uint32_t action;
	bt_hfp_call_state_t call_state;
	struct bt_hfp_ag_call *call;

	if (is_char(buf, '?')) {
		call = get_call_with_flag(ag, BT_HFP_AG_CALL_INCOMING_HELD, false);
		hfp_ag_lock(ag);
		call_state = call->call_state;
		hfp_ag_unlock(ag);
		if (call && (call_state == BT_HFP_CALL_ACTIVE)) {
			err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+BTRH:%d\r\n",
					       BT_HFP_BTRH_ON_HOLD);
			if (err) {
				LOG_ERR("Fail to send err :(%d)", err);
			}
			return err;
		}
		return 0;
	}

	if (!is_char(buf, '=')) {
		return -ENOTSUP;
	}

	err = get_number(buf, &action);
	if (err != 0) {
		return -ENOTSUP;
	}

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	if (action == BT_HFP_BTRH_ON_HOLD) {
		call = get_call_with_flag_and_state(ag, BT_HFP_AG_CALL_INCOMING,
						    BT_HFP_CALL_ALERTING | BT_HFP_CALL_INCOMING);
		if (call) {
			atomic_set_bit(call->flags, BT_HFP_AG_CALL_INCOMING_HELD);
			err = hfp_ag_send_data(ag, btrh_held_cb, call, "\r\n+BTRH:%d\r\n",
					       BT_HFP_BTRH_ON_HOLD);
			if (err) {
				LOG_ERR("Fail to send err :(%d)", err);
			}
			return err;
		}
	} else if (action == BT_HFP_BTRH_ACCEPTED) {
		call = get_call_with_flag_and_state(ag, BT_HFP_AG_CALL_INCOMING_HELD,
						    BT_HFP_CALL_ACTIVE);
		if (call) {
			atomic_clear_bit(call->flags, BT_HFP_AG_CALL_INCOMING_HELD);
			err = hfp_ag_send_data(ag, btrh_accept_cb, call, "\r\n+BTRH:%d\r\n",
					       BT_HFP_BTRH_ACCEPTED);
			if (err) {
				LOG_ERR("Fail to send err :(%d)", err);
			}
			return err;
		}
	} else if (action == BT_HFP_BTRH_REJECTED) {
		call = get_call_with_flag_and_state(ag, BT_HFP_AG_CALL_INCOMING_HELD,
						    BT_HFP_CALL_ACTIVE);
		if (call) {
			atomic_clear_bit(call->flags, BT_HFP_AG_CALL_INCOMING_HELD);
			err = hfp_ag_send_data(ag, btrh_reject_cb, call, "\r\n+BTRH:%d\r\n",
					       BT_HFP_BTRH_REJECTED);
			if (err) {
				LOG_ERR("Fail to send err :(%d)", err);
			}
			return err;
		}
	}

	return -ENOTSUP;
}

static int bt_hfp_ag_ccwa_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	int err;
	uint32_t value;

	if (!BOTH_SUPT_FEAT(ag, BT_HFP_HF_FEATURE_3WAY_CALL, BT_HFP_AG_FEATURE_3WAY_CALL)) {
		return -ENOEXEC;
	}

	if (!is_char(buf, '=')) {
		return -ENOTSUP;
	}

	err = get_number(buf, &value);
	if (err != 0) {
		return -ENOTSUP;
	}

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	if (value > 1) {
		return -ENOTSUP;
	}

	atomic_set_bit_to(ag->flags, BT_HFP_AG_CCWA_ENABLE, value);
	return 0;
}

static void bt_hfp_ag_vr_activate(struct bt_hfp_ag *ag, void *user_data)
{
	bool feature;

	feature = BOTH_SUPT_FEAT(ag, BT_HFP_HF_FEATURE_ENH_VOICE_RECG,
				 BT_HFP_AG_FEATURE_ENH_VOICE_RECG);

#if defined(CONFIG_BT_HFP_AG_VOICE_RECG)
	if (bt_ag && bt_ag->voice_recognition) {
		bt_ag->voice_recognition(ag, true);
	} else {
		(void)bt_hfp_ag_audio_connect(ag, BT_HFP_AG_CODEC_CVSD);
	}
#endif /* CONFIG_BT_HFP_AG_VOICE_RECG */

	atomic_set_bit_to(ag->flags, BT_HFP_AG_VRE_R2A, !feature);

#if defined(CONFIG_BT_HFP_AG_ENH_VOICE_RECG)
	if (atomic_test_bit(ag->flags, BT_HFP_AG_VRE_R2A)) {
		if (bt_ag && bt_ag->ready_to_accept_audio) {
			bt_ag->ready_to_accept_audio(ag);
		}
	}
#endif /* CONFIG_BT_HFP_AG_ENH_VOICE_RECG */
}

static void bt_hfp_ag_vr_deactivate(struct bt_hfp_ag *ag, void *user_data)
{
#if defined(CONFIG_BT_HFP_AG_ENH_VOICE_RECG)
	if (bt_ag && bt_ag->voice_recognition) {
		bt_ag->voice_recognition(ag, false);
	}
#endif /* CONFIG_BT_HFP_AG_ENH_VOICE_RECG */
}

static void bt_hfp_ag_vr_ready2accept(struct bt_hfp_ag *ag, void *user_data)
{
	atomic_set_bit(ag->flags, BT_HFP_AG_VRE_R2A);

#if defined(CONFIG_BT_HFP_AG_ENH_VOICE_RECG)
	if (bt_ag && bt_ag->ready_to_accept_audio) {
		bt_ag->ready_to_accept_audio(ag);
	}
#endif /* CONFIG_BT_HFP_AG_ENH_VOICE_RECG */
}

static int bt_hfp_ag_bvra_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	int err;
	uint32_t value;

	if (!BOTH_SUPT_FEAT(ag, BT_HFP_HF_FEATURE_VOICE_RECG, BT_HFP_AG_FEATURE_VOICE_RECG)) {
		return -ENOEXEC;
	}

	if (!is_char(buf, '=')) {
		return -ENOTSUP;
	}

	err = get_number(buf, &value);
	if (err != 0) {
		return -ENOTSUP;
	}

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	switch (value) {
	case BT_HFP_BVRA_DEACTIVATION:
		if (!atomic_test_and_clear_bit(ag->flags, BT_HFP_AG_VRE_ACTIVATE)) {
			LOG_WRN("VR is not activated");
			return -ENOTSUP;
		}
		err = hfp_ag_next_step(ag, bt_hfp_ag_vr_deactivate, NULL);
		break;
	case BT_HFP_BVRA_ACTIVATION:
		if (atomic_test_and_set_bit(ag->flags, BT_HFP_AG_VRE_ACTIVATE)) {
			LOG_WRN("VR has been activated");
			return -ENOTSUP;
		}
		atomic_clear_bit(ag->flags, BT_HFP_AG_VRE_R2A);
		err = hfp_ag_next_step(ag, bt_hfp_ag_vr_activate, NULL);
		break;
	case BT_HFP_BVRA_READY_TO_ACCEPT:
		if (!BOTH_SUPT_FEAT(ag, BT_HFP_HF_FEATURE_ENH_VOICE_RECG,
				    BT_HFP_AG_FEATURE_ENH_VOICE_RECG)) {
			LOG_WRN("Enhance voice recognition is not supported");
			return -ENOEXEC;
		}
		if (!atomic_test_bit(ag->flags, BT_HFP_AG_VRE_ACTIVATE)) {
			LOG_WRN("Voice recognition is not activated");
			return -ENOTSUP;
		}
		err = hfp_ag_next_step(ag, bt_hfp_ag_vr_ready2accept, NULL);
		break;
	default:
		return -ENOTSUP;
	}

	return err;
}

static int bt_hfp_ag_binp_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	int err;
	uint32_t value;
#if defined(CONFIG_BT_HFP_AG_VOICE_TAG)
	char *number = NULL;
#endif /* CONFIG_BT_HFP_AG_VOICE_TAG */

	if (!AG_SUPT_FEAT(ag, BT_HFP_AG_FEATURE_VOICE_TAG)) {
		return -ENOEXEC;
	}

	if (!is_char(buf, '=')) {
		return -ENOTSUP;
	}

	err = get_number(buf, &value);
	if (err != 0) {
		return -ENOTSUP;
	}

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	if (value != 1) {
		return -ENOTSUP;
	}

#if defined(CONFIG_BT_HFP_AG_VOICE_TAG)
	if (bt_ag && bt_ag->request_phone_number) {
		err = bt_ag->request_phone_number(ag, &number);
		if (err) {
			LOG_DBG("Cannot request phone number :(%d)", err);
			return err;
		}

		err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+BINP:\"%s\"\r\n", number);
		if (err) {
			LOG_ERR("Fail to send err :(%d)", err);
		}
		return err;
	}
#endif /* CONFIG_BT_HFP_AG_VOICE_TAG */

	return -ENOTSUP;
}

static int bt_hfp_ag_vts_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	int err;
	char code = 0;

	if (!is_char(buf, '=')) {
		return -ENOTSUP;
	}

	err = get_char(buf, &code);
	if (err != 0) {
		return -ENOTSUP;
	}

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	if (!IS_VALID_DTMF(code)) {
		LOG_ERR("Invalid code");
		return -EINVAL;
	}

	if (!get_active_calls(ag)) {
		LOG_ERR("Not valid ongoing call");
		return -ENOTSUP;
	}

	if (bt_ag && bt_ag->transmit_dtmf_code) {
		bt_ag->transmit_dtmf_code(ag, code);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int send_subscriber_number(struct bt_hfp_ag *ag, char *number,
		uint8_t type, uint8_t service)
{
	int err;

	err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+CNUM:,\"%s\",%d,,%d\r\n",
		number, type, service);
	if (err) {
		LOG_ERR("Fail to send subscriber number :(%d)", err);
	}
	return err;
}

static int bt_hfp_ag_cnum_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	int err;

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	if (bt_ag && bt_ag->subscriber_number) {
		err = bt_ag->subscriber_number(ag, send_subscriber_number);
		return err;
	}

	return 0;
}

static int bt_hfp_ag_biev_handler(struct bt_hfp_ag *ag, struct net_buf *buf)
{
	uint32_t indicator;
	uint32_t value;

	if (!is_char(buf, '=')) {
		return -ENOTSUP;
	}

	if (get_number(buf, &indicator)) {
		return -ENOTSUP;
	}

	if (!is_char(buf, ',')) {
		return -ENOTSUP;
	}

	if (get_number(buf, &value)) {
		return -ENOTSUP;
	}

	if (!is_char(buf, '\r')) {
		return -ENOTSUP;
	}

	hfp_ag_lock(ag);
	if (!(ag->hf_indicators_of_ag & BIT(indicator))) {
		hfp_ag_unlock(ag);
		return -ENOTSUP;
	}
	hfp_ag_unlock(ag);

#if defined(CONFIG_BT_HFP_AG_HF_INDICATORS)
	if (bt_ag && bt_ag->hf_indicator_value) {
		bt_ag->hf_indicator_value(ag, indicator, value);
	}
#endif /* CONFIG_BT_HFP_HF_HF_INDICATORS */

	return 0;
}

static struct bt_hfp_ag_at_cmd_handler cmd_handlers[] = {
	{"AT+BRSF", bt_hfp_ag_brsf_handler}, {"AT+BAC", bt_hfp_ag_bac_handler},
	{"AT+CIND", bt_hfp_ag_cind_handler}, {"AT+CMER", bt_hfp_ag_cmer_handler},
	{"AT+CHLD", bt_hfp_ag_chld_handler}, {"AT+BIND", bt_hfp_ag_bind_handler},
	{"AT+CMEE", bt_hfp_ag_cmee_handler}, {"AT+CHUP", bt_hfp_ag_chup_handler},
	{"AT+CLCC", bt_hfp_ag_clcc_handler}, {"AT+BIA", bt_hfp_ag_bia_handler},
	{"ATA", bt_hfp_ag_ata_handler},      {"AT+COPS", bt_hfp_ag_cops_handler},
	{"AT+BCC", bt_hfp_ag_bcc_handler},   {"AT+BCS", bt_hfp_ag_bcs_handler},
	{"ATD", bt_hfp_ag_atd_handler},      {"AT+BLDN", bt_hfp_ag_bldn_handler},
	{"AT+CLIP", bt_hfp_ag_clip_handler}, {"AT+VGM", bt_hfp_ag_vgm_handler},
	{"AT+VGS", bt_hfp_ag_vgs_handler},   {"AT+NREC", bt_hfp_ag_nrec_handler},
	{"AT+BTRH", bt_hfp_ag_btrh_handler}, {"AT+CCWA", bt_hfp_ag_ccwa_handler},
	{"AT+BVRA", bt_hfp_ag_bvra_handler}, {"AT+BINP", bt_hfp_ag_binp_handler},
	{"AT+VTS", bt_hfp_ag_vts_handler},   {"AT+CNUM", bt_hfp_ag_cnum_handler},
	{"AT+BIEV", bt_hfp_ag_biev_handler},
};

static void hfp_ag_connected(struct bt_rfcomm_dlc *dlc)
{
	struct bt_hfp_ag *ag = CONTAINER_OF(dlc, struct bt_hfp_ag, rfcomm_dlc);

	bt_hfp_ag_set_state(ag, BT_HFP_CONFIG);

	LOG_DBG("AG %p", ag);
}

static void hfp_ag_disconnected(struct bt_rfcomm_dlc *dlc)
{
	struct bt_hfp_ag *ag = CONTAINER_OF(dlc, struct bt_hfp_ag, rfcomm_dlc);
	bt_hfp_call_state_t call_state;
	sys_snode_t *node;
	struct bt_ag_tx *tx;
	struct bt_hfp_ag_call *call;

	k_work_cancel_delayable(&ag->tx_work);

	hfp_ag_lock(ag);
	node = sys_slist_get(&ag->tx_pending);
	hfp_ag_unlock(ag);
	tx = CONTAINER_OF(node, struct bt_ag_tx, node);
	while (tx) {
		if (tx->buf && !atomic_test_and_clear_bit(ag->flags, BT_HFP_AG_TX_ONGOING)) {
			net_buf_unref(tx->buf);
		}
		tx->err = -ESHUTDOWN;
		k_fifo_put(&ag_tx_notify, tx);
		hfp_ag_lock(ag);
		node = sys_slist_get(&ag->tx_pending);
		hfp_ag_unlock(ag);
		tx = CONTAINER_OF(node, struct bt_ag_tx, node);
	}

	bt_hfp_ag_set_state(ag, BT_HFP_DISCONNECTED);

	for (size_t index = 0; index < ARRAY_SIZE(ag->calls); index++) {
		call = &ag->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_IN_USING)) {
			continue;
		}

		hfp_ag_lock(ag);
		call_state = call->call_state;
		hfp_ag_unlock(ag);
		if ((call_state == BT_HFP_CALL_ALERTING) || (call_state == BT_HFP_CALL_ACTIVE) ||
		    (call_state == BT_HFP_CALL_HOLD)) {
			bt_hfp_ag_terminate_cb(ag, call);
		}
	}

	ag->acl_conn = NULL;

	LOG_DBG("AG %p", ag);
}

static void hfp_ag_recv(struct bt_rfcomm_dlc *dlc, struct net_buf *buf)
{
	struct bt_hfp_ag *ag = CONTAINER_OF(dlc, struct bt_hfp_ag, rfcomm_dlc);
	uint8_t *data = buf->data;
	uint16_t len = buf->len;
	enum at_cme cme_err;
	int err = -ENOEXEC;

	LOG_HEXDUMP_DBG(data, len, "Received:");

	for (uint32_t index = 0; index < ARRAY_SIZE(cmd_handlers); index++) {
		if (strlen(cmd_handlers[index].cmd) > len) {
			continue;
		}
		if (strncmp((char *)data, cmd_handlers[index].cmd,
				 strlen(cmd_handlers[index].cmd)) != 0) {
			continue;
		}
		if (NULL != cmd_handlers[index].handler) {
			(void)net_buf_pull(buf, strlen(cmd_handlers[index].cmd));
			err = cmd_handlers[index].handler(ag, buf);
			LOG_DBG("AT commander is handled (err %d)", err);
			break;
		}
	}

	if ((err != 0) && atomic_test_bit(ag->flags, BT_HFP_AG_CMEE_ENABLE)) {
		cme_err = bt_hfp_ag_get_cme_err(err);
		err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+CME ERROR:%d\r\n", (uint32_t)cme_err);
	} else {
		err = hfp_ag_send_data(ag, NULL, NULL, "\r\n%s\r\n", (err == 0) ? "OK" : "ERROR");
	}

	if (err != 0) {
		LOG_ERR("HFP AG send response err :(%d)", err);
	}
}

static void bt_hfp_ag_thread(void *p1, void *p2, void *p3)
{
	struct bt_ag_tx *tx;
	bt_hfp_ag_tx_cb_t cb;
	struct bt_hfp_ag *ag;
	void *user_data;
	bt_hfp_state_t state;
	int err;

	while (true) {
		tx = (struct bt_ag_tx *)k_fifo_get(&ag_tx_notify, K_FOREVER);

		if (tx == NULL) {
			continue;
		}

		cb = tx->cb;
		ag = tx->ag;
		user_data = tx->user_data;
		err = tx->err;

		bt_ag_tx_free(tx);

		if (err < 0) {
			hfp_ag_lock(ag);
			state = ag->state;
			hfp_ag_unlock(ag);
			if ((state != BT_HFP_DISCONNECTED) && (state != BT_HFP_DISCONNECTING)) {
				bt_hfp_ag_set_state(ag, BT_HFP_DISCONNECTING);
				bt_rfcomm_dlc_disconnect(&ag->rfcomm_dlc);
			}
		}

		if (cb) {
			cb(ag, user_data);
		}
	}
}

static void hfp_ag_sent(struct bt_rfcomm_dlc *dlc, int err)
{
	struct bt_hfp_ag *ag = CONTAINER_OF(dlc, struct bt_hfp_ag, rfcomm_dlc);
	sys_snode_t *node;
	struct bt_ag_tx *tx;

	hfp_ag_lock(ag);
	/* Clear the tx ongoing flag */
	if (!atomic_test_and_clear_bit(ag->flags, BT_HFP_AG_TX_ONGOING)) {
		LOG_WRN("tx ongoing flag is not set");
		hfp_ag_unlock(ag);
		return;
	}

	node = sys_slist_get(&ag->tx_pending);
	hfp_ag_unlock(ag);
	if (!node) {
		LOG_ERR("No pending tx");
		return;
	}

	tx = CONTAINER_OF(node, struct bt_ag_tx, node);
	LOG_DBG("Completed pending tx %p", tx);

	/* Restart the tx work */
	k_work_reschedule(&ag->tx_work, K_NO_WAIT);

	tx->err = err;
	k_fifo_put(&ag_tx_notify, tx);
}

static const char *bt_ag_get_call_state_string(bt_hfp_call_state_t call_state)
{
	const char *s;

	switch (call_state) {
	case BT_HFP_CALL_TERMINATE:
		s = "terminate";
		break;
	case BT_HFP_CALL_ACTIVE:
		s = "active";
		break;
	case BT_HFP_CALL_HOLD:
		s = "hold";
		break;
	case BT_HFP_CALL_OUTGOING:
		s = "outgoing";
		break;
	case BT_HFP_CALL_INCOMING:
		s = "incoming";
		break;
	case BT_HFP_CALL_ALERTING:
		s = "alerting";
		break;
	default:
		s = "unknown";
		break;
	}

	return s;
}

static void bt_ag_deferred_work_cb(struct bt_hfp_ag *ag, void *user_data)
{
	int err;
	bt_hfp_call_state_t call_state;
	uint8_t call_setup_ind;
	uint8_t call_ind;
	struct bt_hfp_ag_call *call = (struct bt_hfp_ag_call *)user_data;

	LOG_DBG("");

	hfp_ag_lock(ag);
	call_state = call->call_state;
	call_setup_ind = ag->indicator_value[BT_HFP_AG_CALL_SETUP_IND];
	call_ind = ag->indicator_value[BT_HFP_AG_CALL_IND];
	hfp_ag_unlock(ag);

	switch (call_state) {
	case BT_HFP_CALL_TERMINATE:
		break;
	case BT_HFP_CALL_ACTIVE:
		break;
	case BT_HFP_CALL_HOLD:
		break;
	case BT_HFP_CALL_OUTGOING:
	case BT_HFP_CALL_INCOMING:
	case BT_HFP_CALL_ALERTING:
	default:
		LOG_WRN("Call timeout, status %s", bt_ag_get_call_state_string(call_state));

		if (atomic_test_bit(call->flags, BT_HFP_AG_CALL_OUTGOING_3WAY) ||
		    atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING_3WAY)) {
			if (!call_setup_ind) {
				return;
			}

			err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND,
						      BT_HFP_CALL_SETUP_NONE,
						      bt_hfp_ag_reject_cb, call);
			if (err) {
				LOG_ERR("Fail to send indicator");
				bt_hfp_ag_terminate_cb(ag, call);
			}
			return;
		}

		if (call_setup_ind && call_ind) {
			err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND,
						      BT_HFP_CALL_SETUP_NONE, NULL, NULL);
			if (err) {
				LOG_ERR("Fail to send indicator");
				bt_hfp_ag_terminate_cb(ag, call);
			} else {
				err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 0,
							      bt_hfp_ag_terminate_cb, call);
				if (err) {
					LOG_ERR("Fail to send indicator");
					bt_hfp_ag_terminate_cb(ag, call);
				}
			}
		} else if (call_setup_ind) {
			err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND,
						      BT_HFP_CALL_SETUP_NONE, bt_hfp_ag_reject_cb,
						      call);
			if (err) {
				LOG_ERR("Fail to send indicator");
				bt_hfp_ag_terminate_cb(ag, call);
			}
		} else if (call_ind) {
			err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 0,
						      bt_hfp_ag_terminate_cb, call);
			if (err) {
				LOG_ERR("Fail to send indicator");
				bt_hfp_ag_terminate_cb(ag, call);
			}
		}
		break;
	}
}

static void bt_ag_deferred_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_hfp_ag_call *call = CONTAINER_OF(dwork, struct bt_hfp_ag_call, deferred_work);
	struct bt_hfp_ag *ag = call->ag;

	(void)hfp_ag_next_step(ag, bt_ag_deferred_work_cb, call);
}

static void bt_ag_ringing_work_cb(struct bt_hfp_ag *ag, void *user_data)
{
	int err;
	bt_hfp_call_state_t call_state;
	struct bt_hfp_ag_call *call = (struct bt_hfp_ag_call *)user_data;

	LOG_DBG("");

	hfp_ag_lock(ag);
	call_state = call->call_state;
	hfp_ag_unlock(ag);
	if (call_state == BT_HFP_CALL_ALERTING) {

		if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING)) {
			return;
		}

		k_work_reschedule(&call->ringing_work,
				  K_SECONDS(CONFIG_BT_HFP_AG_RING_NOTIFY_INTERVAL));

		err = hfp_ag_send_data(ag, NULL, NULL, "\r\nRING\r\n");
		if (err) {
			LOG_ERR("Fail to send RING %d", err);
		} else {
			if (atomic_test_bit(ag->flags, BT_HFP_AG_CLIP_ENABLE)) {
				err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+CLIP:\"%s\",%d\r\n",
						       call->number, 0);
				if (err) {
					LOG_ERR("Fail to send CLIP %d", err);
				}
			}
		}
	}
}

static void bt_ag_ringing_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_hfp_ag_call *call = CONTAINER_OF(dwork, struct bt_hfp_ag_call, ringing_work);

	(void)hfp_ag_next_step(call->ag, bt_ag_ringing_work_cb, call);
}

static K_KERNEL_STACK_MEMBER(ag_thread_stack, CONFIG_BT_HFP_AG_THREAD_STACK_SIZE);

static struct bt_hfp_ag *hfp_ag_create(struct bt_conn *conn)
{
	static struct bt_rfcomm_dlc_ops ops = {
		.connected = hfp_ag_connected,
		.disconnected = hfp_ag_disconnected,
		.recv = hfp_ag_recv,
		.sent = hfp_ag_sent,
	};
	static k_tid_t ag_thread_id;
	static struct k_thread ag_thread;
	size_t index;
	struct bt_hfp_ag *ag;

	LOG_DBG("conn %p", conn);

	if (ag_thread_id == NULL) {

		k_fifo_init(&ag_tx_free);
		k_fifo_init(&ag_tx_notify);

		for (index = 0; index < ARRAY_SIZE(ag_tx); index++) {
			k_fifo_put(&ag_tx_free, &ag_tx[index]);
		}

		ag_thread_id = k_thread_create(
			&ag_thread, ag_thread_stack, K_KERNEL_STACK_SIZEOF(ag_thread_stack),
			bt_hfp_ag_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_HFP_AG_THREAD_PRIO), 0, K_NO_WAIT);
		__ASSERT(ag_thread_id, "Cannot create thread for AG");
		k_thread_name_set(ag_thread_id, "HFP AG");
	}

	index = (size_t)bt_conn_index(conn);
	ag = &bt_hfp_ag_pool[index];
	if (ag->acl_conn) {
		LOG_ERR("AG connection (%p) is established", conn);
		return NULL;
	}

	(void)memset(ag, 0, sizeof(struct bt_hfp_ag));

	sys_slist_init(&ag->tx_pending);

	k_sem_init(&ag->lock, 1, 1);

	ag->rfcomm_dlc.ops = &ops;
	ag->rfcomm_dlc.mtu = BT_HFP_MAX_MTU;

	/* Set the supported features*/
	ag->ag_features = BT_HFP_AG_SUPPORTED_FEATURES;

	/* Support HF indicators */
	if (IS_ENABLED(CONFIG_BT_HFP_AG_HF_INDICATOR_ENH_SAFETY)) {
		ag->hf_indicators_of_ag |= BIT(HFP_HF_ENHANCED_SAFETY_IND);
	}

	if (IS_ENABLED(CONFIG_BT_HFP_AG_HF_INDICATOR_BATTERY)) {
		ag->hf_indicators_of_ag |= BIT(HFP_HF_BATTERY_LEVEL_IND);
	}

	ag->hf_indicators = ag->hf_indicators_of_ag;

	/* If supported codec ids cannot be notified, disable codec negotiation. */
	if (!(bt_ag && bt_ag->codec)) {
		ag->ag_features &= ~BT_HFP_AG_FEATURE_CODEC_NEG;
	}

	ag->hf_features = 0;
	ag->hf_codec_ids = 0;

	ag->acl_conn = conn;

	/* Set AG indicator value */
	ag->indicator_value[BT_HFP_AG_SERVICE_IND] = 0;
	ag->indicator_value[BT_HFP_AG_CALL_IND] = 0;
	ag->indicator_value[BT_HFP_AG_CALL_SETUP_IND] = 0;
	ag->indicator_value[BT_HFP_AG_CALL_HELD_IND] = 0;
	ag->indicator_value[BT_HFP_AG_SIGNAL_IND] = 0;
	ag->indicator_value[BT_HFP_AG_ROAM_IND] = 0;
	ag->indicator_value[BT_HFP_AG_BATTERY_IND] = 0;

	/* Set AG indicator status */
	ag->indicator = BIT(BT_HFP_AG_SERVICE_IND) | BIT(BT_HFP_AG_CALL_IND) |
				BIT(BT_HFP_AG_CALL_SETUP_IND) | BIT(BT_HFP_AG_CALL_HELD_IND) |
				BIT(BT_HFP_AG_SIGNAL_IND) | BIT(BT_HFP_AG_ROAM_IND) |
				BIT(BT_HFP_AG_BATTERY_IND);

	/* Set AG operator */
	memcpy(ag->operator, "UNKNOWN", sizeof("UNKNOWN"));

	/* Set Codec ID*/
	ag->selected_codec_id = BT_HFP_AG_CODEC_CVSD;

	/* Init delay work */
	k_work_init_delayable(&ag->tx_work, bt_ag_tx_work);

	return ag;
}

int bt_hfp_ag_connect(struct bt_conn *conn, struct bt_hfp_ag **ag, uint8_t channel)
{
	struct bt_hfp_ag *new_ag;
	int err;

	LOG_DBG("");

	if (!conn || !ag || !channel) {
		return -EINVAL;
	}

	if (!bt_ag) {
		return -EFAULT;
	}

	new_ag = hfp_ag_create(conn);
	if (!new_ag) {
		return -ECONNREFUSED;
	}

	err = bt_rfcomm_dlc_connect(conn, &new_ag->rfcomm_dlc, channel);
	if (err != 0) {
		(void)memset(new_ag, 0, sizeof(*new_ag));
		*ag = NULL;
	} else {
		*ag = new_ag;
		bt_hfp_ag_set_state(*ag, BT_HFP_CONNECTING);
	}

	return err;
}

int bt_hfp_ag_disconnect(struct bt_hfp_ag *ag)
{
	LOG_DBG("");

	if (ag == NULL) {
		return -EINVAL;
	}

	bt_hfp_ag_set_state(ag, BT_HFP_DISCONNECTING);

	return bt_rfcomm_dlc_disconnect(&ag->rfcomm_dlc);
}

static int hfp_ag_accept(struct bt_conn *conn, struct bt_rfcomm_server *server,
			 struct bt_rfcomm_dlc **dlc)
{
	struct bt_hfp_ag *ag;

	ag = hfp_ag_create(conn);

	if (!ag) {
		return -ECONNREFUSED;
	}

	*dlc = &ag->rfcomm_dlc;

	return 0;
}

static int bt_hfp_ag_sco_accept(const struct bt_sco_accept_info *info,
		struct bt_sco_chan **chan)
{
	static struct bt_sco_chan_ops ops = {
		.connected = hfp_ag_sco_connected,
		.disconnected = hfp_ag_sco_disconnected,
	};
	size_t index;
	struct bt_hfp_ag *ag;

	LOG_DBG("conn %p", info->acl);

	index = (size_t)bt_conn_index(info->acl);
	ag = &bt_hfp_ag_pool[index];
	if (ag->acl_conn != info->acl) {
		LOG_ERR("ACL %p of AG is unaligned with SCO's %p", ag->acl_conn, info->acl);
		return -EINVAL;
	}

	if (ag->sco_chan.sco) {
		return -ECONNREFUSED;
	}

	ag->sco_chan.ops = &ops;

	*chan = &ag->sco_chan;

	return 0;
}

static void hfp_ag_init(void)
{
	static struct bt_rfcomm_server chan = {
		.channel = BT_RFCOMM_CHAN_HFP_AG,
		.accept = hfp_ag_accept,
	};

	bt_rfcomm_server_register(&chan);

	static struct bt_sco_server sco_server = {
		.sec_level = BT_SECURITY_L0,
		.accept = bt_hfp_ag_sco_accept,
	};

	bt_sco_server_register(&sco_server);

	bt_sdp_register_service(&hfp_ag_rec);
}

int bt_hfp_ag_register(struct bt_hfp_ag_cb *cb)
{
	if (!cb) {
		return -EINVAL;
	}

	if (bt_ag) {
		return -EALREADY;
	}

	bt_ag = cb;

	hfp_ag_init();

	return 0;
}

static void bt_hfp_ag_incoming_cb(struct bt_hfp_ag *ag, void *user_data)
{
	struct bt_hfp_ag_call *call = (struct bt_hfp_ag_call *)user_data;

	__ASSERT(call, "Invalid call object");

	bt_hfp_ag_set_call_state(call, BT_HFP_CALL_INCOMING);

	if (bt_ag && bt_ag->incoming) {
		bt_ag->incoming(ag, call, call->number);
	}

	if (atomic_test_bit(ag->flags, BT_HFP_AG_INBAND_RING)) {
		int err;

		err = bt_hfp_ag_create_audio_connection(ag, call);
		if (err) {
			bt_hfp_ag_call_reject(ag, user_data);
		}
	} else {
		bt_hfp_ag_set_call_state(call, BT_HFP_CALL_ALERTING);
		bt_hfp_ag_call_ringing_cb(call, false);
	}
}

#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
static void bt_hfp_ag_2nd_incoming_cb(struct bt_hfp_ag *ag, void *user_data)
{
	struct bt_hfp_ag_call *call;

	call = (struct bt_hfp_ag_call *)user_data;

	__ASSERT(call, "Invalid call object");

	if (bt_ag && bt_ag->incoming) {
		bt_ag->incoming(ag, call, call->number);
	}

	bt_hfp_ag_set_call_state(call, BT_HFP_CALL_ALERTING);
}

static void bt_hfp_ag_ccwa_cb(struct bt_hfp_ag *ag, void *user_data)
{
	int err;

	err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_INCOMING,
				      bt_hfp_ag_2nd_incoming_cb, user_data);
	if (err) {
		LOG_ERR("Fail to send call setup");
	}
}
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */

int bt_hfp_ag_remote_incoming(struct bt_hfp_ag *ag, const char *number)
{
	int err = 0;
	size_t len;
	struct bt_hfp_ag_call *call;
	int call_count;

	LOG_DBG("");

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}
	hfp_ag_unlock(ag);

	len = strlen(number);
	if ((len == 0) || (len > CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN)) {
		return -EINVAL;
	}

	call = get_call_from_number(ag, number, 0);
	if (call) {
		return -EBUSY;
	}

	call_count = get_none_released_calls(ag);
	if (call_count) {
#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
		if (!BOTH_SUPT_FEAT(ag, BT_HFP_HF_FEATURE_3WAY_CALL,
				    BT_HFP_AG_FEATURE_3WAY_CALL)) {
			LOG_WRN("3 Way call feature is not supported on both sides");
			return -ENOEXEC;
		}

		if (!atomic_test_bit(ag->flags, BT_HFP_AG_CCWA_ENABLE)) {
			LOG_WRN("Call waiting notification is not enabled");
			return -ENOTSUP;
		}

		if (!get_active_held_calls(ag)) {
			LOG_WRN("The first call is not accepted");
			return -EBUSY;
		}
#else
		return -EBUSY;
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */
	}

	call = get_new_call(ag, number, 0);
	if (!call) {
		LOG_WRN("Cannot allocate a new call object");
		return -ENOMEM;
	}

	atomic_set_bit(call->flags, BT_HFP_AG_CALL_INCOMING);
	bt_hfp_ag_set_call_state(call, BT_HFP_CALL_INCOMING);

#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
	if (call_count) {
		atomic_set_bit(call->flags, BT_HFP_AG_CALL_INCOMING_3WAY);
		err = hfp_ag_send_data(ag, bt_hfp_ag_ccwa_cb, call, "\r\n+CCWA:\"%s\",%d\r\n",
				       call->number, call->type);
		if (err) {
			LOG_WRN("Fail to send call waiting notification");
			free_call(call);
		}

		return err;
	}
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */

	err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_INCOMING,
				      bt_hfp_ag_incoming_cb, call);
	if (err) {
		free_call(call);
	}

	return err;
}

int bt_hfp_ag_hold_incoming(struct bt_hfp_ag_call *call)
{
	int err = 0;
	struct bt_hfp_ag *ag;
	bt_hfp_call_state_t call_state;

	LOG_DBG("");

	if (call == NULL) {
		return -EINVAL;
	}

	ag = call->ag;

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}

	call_state = call->call_state;
	hfp_ag_unlock(ag);

	if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING)) {
		return -EINVAL;
	}

	if ((call_state == BT_HFP_CALL_ALERTING) || (call_state == BT_HFP_CALL_INCOMING)) {
		atomic_set_bit(call->flags, BT_HFP_AG_CALL_INCOMING_HELD);
		err = hfp_ag_send_data(ag, btrh_held_cb, call, "\r\n+BTRH:%d\r\n",
				       BT_HFP_BTRH_ON_HOLD);
		if (err) {
			LOG_ERR("Fail to send err :(%d)", err);
		}
		return err;
	}

	return -EINVAL;
}

int bt_hfp_ag_reject(struct bt_hfp_ag_call *call)
{
	int err = 0;
	struct bt_hfp_ag *ag;
	bt_hfp_call_state_t call_state;

	LOG_DBG("");

	if (call == NULL) {
		return -EINVAL;
	}

	ag = call->ag;

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}

	call_state = call->call_state;
	hfp_ag_unlock(ag);

	if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING)) {
		return -EINVAL;
	}

	if (!AG_SUPT_FEAT(ag, BT_HFP_AG_FEATURE_REJECT_CALL)) {
		LOG_ERR("AG has not ability to reject call");
		return -ENOTSUP;
	}

	if (atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING_3WAY)) {
		if ((call_state == BT_HFP_CALL_ALERTING) || (call_state == BT_HFP_CALL_INCOMING)) {
			uint8_t call_setup;

			hfp_ag_lock(ag);
			call_setup = ag->indicator_value[BT_HFP_AG_CALL_SETUP_IND];
			hfp_ag_unlock(ag);

			if (call_setup) {
				err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND,
							      BT_HFP_CALL_SETUP_NONE, NULL, NULL);
				if (err) {
					return err;
				}
			}

			ag_reject_call(call);

			return 0;
		}
	}

	if ((call_state == BT_HFP_CALL_ALERTING) || (call_state == BT_HFP_CALL_INCOMING)) {
		err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_NONE,
					      bt_hfp_ag_reject_cb, call);

		return err;
	}

	if ((call_state == BT_HFP_CALL_ACTIVE) &&
	    atomic_test_and_clear_bit(call->flags, BT_HFP_AG_CALL_INCOMING_HELD)) {
		err = hfp_ag_send_data(ag, btrh_reject_cb, call, "\r\n+BTRH:%d\r\n",
				       BT_HFP_BTRH_REJECTED);
		if (err) {
			LOG_ERR("Fail to send err :(%d)", err);
		}
		return err;
	}

	return -EINVAL;
}

int bt_hfp_ag_accept(struct bt_hfp_ag_call *call)
{
	int err = 0;
	struct bt_hfp_ag *ag;
	bt_hfp_call_state_t call_state;

	LOG_DBG("");

	if (call == NULL) {
		return -EINVAL;
	}

	ag = call->ag;

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}

	call_state = call->call_state;
	hfp_ag_unlock(ag);

	if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING)) {
		return -EINVAL;
	}

#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
	if (atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING_3WAY)) {
		if ((call_state == BT_HFP_CALL_ALERTING) || (call_state == BT_HFP_CALL_INCOMING)) {
			return chld_deactivate_calls_and_accept_other_call(ag, false);
		}
	}
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */

	if (call_state == BT_HFP_CALL_ALERTING) {
		err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 1, bt_hfp_ag_accept_cb, call);
		if (err) {
			LOG_ERR("Fail to send err :(%d)", err);
		}

		err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_NONE,
					      bt_hfp_ag_call_setup_none_cb, call);
		if (err) {
			LOG_ERR("Fail to send err :(%d)", err);
		}

		return err;
	}

	if (atomic_test_and_clear_bit(call->flags, BT_HFP_AG_CALL_INCOMING_HELD) &&
	    (call_state == BT_HFP_CALL_ACTIVE)) {
		err = hfp_ag_send_data(ag, btrh_accept_cb, call, "\r\n+BTRH:%d\r\n",
				       BT_HFP_BTRH_ACCEPTED);
		if (err) {
			LOG_ERR("Fail to send err :(%d)", err);
		}
		return err;
	}

	return -EINVAL;
}

int bt_hfp_ag_terminate(struct bt_hfp_ag_call *call)
{
	int err = 0;
	struct bt_hfp_ag *ag;
	bt_hfp_call_state_t call_state;

	LOG_DBG("");

	if (call == NULL) {
		return -EINVAL;
	}

	ag = call->ag;

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}

	call_state = call->call_state;
	hfp_ag_unlock(ag);

	if (atomic_test_and_clear_bit(call->flags, BT_HFP_AG_CALL_INCOMING_HELD) &&
	    atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING) &&
	    (call_state == BT_HFP_CALL_ACTIVE)) {
		err = hfp_ag_send_data(ag, btrh_reject_cb, call, "\r\n+BTRH:%d\r\n",
				       BT_HFP_BTRH_REJECTED);
		if (err) {
			LOG_ERR("Fail to send err :(%d)", err);
		}
		return err;
	}

	if ((call_state != BT_HFP_CALL_ACTIVE) && (call_state != BT_HFP_CALL_HOLD)) {
		return -EINVAL;
	}

	ag_terminate_call(call);

	return 0;
}

int bt_hfp_ag_retrieve(struct bt_hfp_ag_call *call)
{
	struct bt_hfp_ag *ag;
	bt_hfp_call_state_t call_state;

	LOG_DBG("");

	if (call == NULL) {
		return -EINVAL;
	}

	ag = call->ag;

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}

	call_state = call->call_state;
	hfp_ag_unlock(ag);

	if (call_state != BT_HFP_CALL_HOLD) {
		return -EINVAL;
	}

	bt_hfp_ag_set_call_state(call, BT_HFP_CALL_ACTIVE);

	if (bt_ag && bt_ag->retrieve) {
		bt_ag->retrieve(call);
	}

	ag_notify_call_held_ind(ag, NULL);

	return 0;
}

int bt_hfp_ag_hold(struct bt_hfp_ag_call *call)
{
	struct bt_hfp_ag *ag;
	bt_hfp_call_state_t call_state;

	LOG_DBG("");

	if (call == NULL) {
		return -EINVAL;
	}

	ag = call->ag;

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}

	call_state = call->call_state;
	hfp_ag_unlock(ag);

	if (call_state != BT_HFP_CALL_ACTIVE) {
		return -EINVAL;
	}

	if ((call_state == BT_HFP_CALL_ACTIVE) &&
	    (atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING_HELD))) {
		return -EINVAL;
	}

	bt_hfp_ag_set_call_state(call, BT_HFP_CALL_HOLD);

	if (bt_ag && bt_ag->held) {
		bt_ag->held(call);
	}

	ag_notify_call_held_ind(ag, NULL);

	return 0;
}

int bt_hfp_ag_outgoing(struct bt_hfp_ag *ag, const char *number)
{
	LOG_DBG("");

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}
	hfp_ag_unlock(ag);

	return bt_hfp_ag_outgoing_call(ag, number, 0);
}

static void bt_hfp_ag_ringing_cb(struct bt_hfp_ag *ag, void *user_data)
{
	struct bt_hfp_ag_call *call = (struct bt_hfp_ag_call *)user_data;

	bt_hfp_ag_set_call_state(call, BT_HFP_CALL_ALERTING);

	if (atomic_test_bit(ag->flags, BT_HFP_AG_INBAND_RING)) {
		bt_hfp_ag_call_ringing_cb(call, true);
	} else {
		bt_hfp_ag_call_ringing_cb(call, false);
	}
}

int bt_hfp_ag_remote_ringing(struct bt_hfp_ag_call *call)
{
	int err = 0;
	struct bt_hfp_ag *ag;

	LOG_DBG("");

	if (call == NULL) {
		return -EINVAL;
	}

	ag = call->ag;

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}

	if (call->call_state != BT_HFP_CALL_OUTGOING) {
		hfp_ag_unlock(ag);
		return -EBUSY;
	}

	if (atomic_test_bit(ag->flags, BT_HFP_AG_INBAND_RING)) {
		if (ag->sco_chan.sco == NULL) {
			hfp_ag_unlock(ag);
			return -ENOTCONN;
		}
	}
	hfp_ag_unlock(ag);

	err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND,
				      BT_HFP_CALL_SETUP_REMOTE_ALERTING, bt_hfp_ag_ringing_cb,
				      call);

	return err;
}

int bt_hfp_ag_remote_reject(struct bt_hfp_ag_call *call)
{
	int err;
	struct bt_hfp_ag *ag;

	LOG_DBG("");

	if (call == NULL) {
		return -EINVAL;
	}

	ag = call->ag;

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}

	if ((call->call_state != BT_HFP_CALL_ALERTING) &&
	    (call->call_state != BT_HFP_CALL_OUTGOING)) {
		hfp_ag_unlock(ag);
		return -EINVAL;
	}
	hfp_ag_unlock(ag);

	if (atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING)) {
		return -EINVAL;
	}

	err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_NONE,
				      bt_hfp_ag_reject_cb, call);

	return err;
}

int bt_hfp_ag_remote_accept(struct bt_hfp_ag_call *call)
{
	int err;
	struct bt_hfp_ag *ag;

	LOG_DBG("");

	if (call == NULL) {
		return -EINVAL;
	}

	ag = call->ag;

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}

	if (call->call_state != BT_HFP_CALL_ALERTING) {
		hfp_ag_unlock(ag);
		return -EINVAL;
	}
	hfp_ag_unlock(ag);

	if (atomic_test_bit(call->flags, BT_HFP_AG_CALL_INCOMING)) {
		return -EINVAL;
	}

	if (!atomic_test_bit(call->flags, BT_HFP_AG_CALL_OUTGOING_3WAY)) {
		err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 1, bt_hfp_ag_accept_cb, call);
		if (err) {
			LOG_ERR("Fail to send err :(%d)", err);
		}
	}

	err = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_NONE,
				      bt_hfp_ag_call_setup_none_cb, call);
	if (err) {
		LOG_ERR("Fail to send err :(%d)", err);
	}

	return err;
}

int bt_hfp_ag_remote_terminate(struct bt_hfp_ag_call *call)
{
	struct bt_hfp_ag *ag;

	LOG_DBG("");

	if (call == NULL) {
		return -EINVAL;
	}

	ag = call->ag;

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}

	if ((call->call_state != BT_HFP_CALL_ACTIVE) && (call->call_state != BT_HFP_CALL_HOLD)) {
		hfp_ag_unlock(ag);
		return -EINVAL;
	}
	hfp_ag_unlock(ag);

	ag_terminate_call(call);

	return 0;
}

int bt_hfp_ag_explicit_call_transfer(struct bt_hfp_ag *ag)
{
#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}
	hfp_ag_unlock(ag);

	return chld_drop_conversation(ag);
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */
}

int bt_hfp_ag_set_indicator(struct bt_hfp_ag *ag, enum bt_hfp_ag_indicator index, uint8_t value)
{
	int err;

	LOG_DBG("");

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}
	hfp_ag_unlock(ag);

	switch (index) {
	case BT_HFP_AG_SERVICE_IND:
	case BT_HFP_AG_SIGNAL_IND:
	case BT_HFP_AG_ROAM_IND:
	case BT_HFP_AG_BATTERY_IND:
		if ((ag_ind[(uint8_t)index].min > value) || (ag_ind[(uint8_t)index].max < value)) {
			return -EINVAL;
		}
		break;
	case BT_HFP_AG_CALL_IND:
	case BT_HFP_AG_CALL_SETUP_IND:
	case BT_HFP_AG_CALL_HELD_IND:
	default:
		return -EINVAL;
	}

	err = hfp_ag_update_indicator(ag, index, value, NULL, NULL);

	return err;
}

int bt_hfp_ag_set_operator(struct bt_hfp_ag *ag, uint8_t mode, char *name)
{
	int len;

	LOG_DBG("");

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}

	len = strlen(name);
	len = MIN(sizeof(ag->operator) - 1, len);
	memcpy(ag->operator, name, len);
	ag->operator[len] = '\0';
	ag->mode = mode;
	hfp_ag_unlock(ag);

	return 0;
}

int bt_hfp_ag_audio_connect(struct bt_hfp_ag *ag, uint8_t id)
{
	int err;

	LOG_DBG("");

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}

	if (ag->hf_codec_ids) {
		if (!(ag->hf_codec_ids & BIT(id))) {
			hfp_ag_unlock(ag);
			return -EINVAL;
		}
	} else {
		if (id != BT_HFP_AG_CODEC_CVSD) {
			hfp_ag_unlock(ag);
			return -ENOTSUP;
		}
	}

	if (ag->sco_chan.sco) {
		LOG_ERR("Audio conenction has been connected");
		hfp_ag_unlock(ag);
		return -ECONNREFUSED;
	}
	hfp_ag_unlock(ag);

	if (atomic_test_bit(ag->flags, BT_HFP_AG_CODEC_CONN)) {
		return -EBUSY;
	}

	hfp_ag_lock(ag);
	if (ag->selected_codec_id != id) {
		atomic_set_bit(ag->flags, BT_HFP_AG_CODEC_CHANGED);
	}
	ag->selected_codec_id = id;
	hfp_ag_unlock(ag);

	err = bt_hfp_ag_create_audio_connection(ag, NULL);

	return err;
}

int bt_hfp_ag_vgm(struct bt_hfp_ag *ag, uint8_t vgm)
{
	int err;

	LOG_DBG("");

	if (ag == NULL) {
		return -EINVAL;
	}

	if (vgm > BT_HFP_HF_VGM_GAIN_MAX) {
		LOG_ERR("Invalid VGM (%d>%d)", vgm, BT_HFP_HF_VGM_GAIN_MAX);
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}
	hfp_ag_unlock(ag);

	if (!HF_SUPT_FEAT(ag, BT_HFP_HF_FEATURE_VOLUME)) {
		LOG_ERR("Remote Audio Volume Control is unsupported");
		return -ENOTSUP;
	}

	err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+VGM=%d\r\n", vgm);
	if (err) {
		LOG_ERR("Fail to notify vgm err :(%d)", err);
	}

	return err;
}

int bt_hfp_ag_vgs(struct bt_hfp_ag *ag, uint8_t vgs)
{
	int err;

	LOG_DBG("");

	if (ag == NULL) {
		return -EINVAL;
	}

	if (vgs > BT_HFP_HF_VGS_GAIN_MAX) {
		LOG_ERR("Invalid VGM (%d>%d)", vgs, BT_HFP_HF_VGS_GAIN_MAX);
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}
	hfp_ag_unlock(ag);

	if (!HF_SUPT_FEAT(ag, BT_HFP_HF_FEATURE_VOLUME)) {
		LOG_ERR("Remote Audio Volume Control is unsupported");
		return -ENOTSUP;
	}

	err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+VGS=%d\r\n", vgs);
	if (err) {
		LOG_ERR("Fail to notify vgs err :(%d)", err);
	}

	return err;
}

int bt_hfp_ag_inband_ringtone(struct bt_hfp_ag *ag, bool inband)
{
	int err;

	LOG_DBG("");

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}
	hfp_ag_unlock(ag);

	err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+BSIR=%d\r\n", inband ? 1 : 0);
	if (err) {
		LOG_ERR("Fail to set inband ringtone err :(%d)", err);
		return err;
	}

	atomic_set_bit_to(ag->flags, BT_HFP_AG_INBAND_RING, inband);
	return 0;
}

int bt_hfp_ag_voice_recognition(struct bt_hfp_ag *ag, bool activate)
{
#if defined(CONFIG_BT_HFP_AG_VOICE_RECG)
	int err;
	bool feature;
	char *bvra;

	LOG_DBG("");

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}
	hfp_ag_unlock(ag);

	if (activate && atomic_test_bit(ag->flags, BT_HFP_AG_VRE_ACTIVATE)) {
		LOG_WRN("VR has been activated");
		return -ENOTSUP;
	} else if (!activate && !atomic_test_bit(ag->flags, BT_HFP_AG_VRE_ACTIVATE)) {
		LOG_WRN("VR is not activated");
		return -ENOTSUP;
	}

	feature = BOTH_SUPT_FEAT(ag, BT_HFP_HF_FEATURE_ENH_VOICE_RECG,
				 BT_HFP_AG_FEATURE_ENH_VOICE_RECG);
	if (!feature) {
		bvra = "";
	} else {
		bvra = ",0";
	}

	err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+BVRA:%d%s\r\n", activate, bvra);
	if (err) {
		LOG_ERR("Fail to notify VR activation :(%d)", err);
		return err;
	}

	atomic_set_bit_to(ag->flags, BT_HFP_AG_VRE_ACTIVATE, activate);
	atomic_clear_bit(ag->flags, BT_HFP_AG_VRE_R2A);
	return 0;
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_AG_VOICE_RECG */
}

int bt_hfp_ag_vre_state(struct bt_hfp_ag *ag, uint8_t state)
{
#if defined(CONFIG_BT_HFP_AG_ENH_VOICE_RECG)
	int err;
	bool feature;

	LOG_DBG("");

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}
	hfp_ag_unlock(ag);

	if (!atomic_test_bit(ag->flags, BT_HFP_AG_VRE_ACTIVATE)) {
		LOG_WRN("Voice Recognition is not activated");
		return -EINVAL;
	}

	feature = BOTH_SUPT_FEAT(ag, BT_HFP_HF_FEATURE_ENH_VOICE_RECG,
				 BT_HFP_AG_FEATURE_ENH_VOICE_RECG);
	if (!feature) {
		return -ENOTSUP;
	}

	if ((state & BT_HFP_BVRA_STATE_SEND_AUDIO) &&
		!atomic_test_bit(ag->flags, BT_HFP_AG_VRE_R2A)) {
		LOG_ERR("HFP HF is not ready to accept audio input");
		return -EINVAL;
	}

	err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+BVRA:1,%d\r\n", state);
	if (err) {
		LOG_ERR("Fail to send state of VRE :(%d)", err);
		return err;
	}

	return 0;
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_AG_ENH_VOICE_RECG */
}

int bt_hfp_ag_vre_textual_representation(struct bt_hfp_ag *ag, uint8_t state, const char *id,
					 uint8_t type, uint8_t operation, const char *text)
{
#if defined(CONFIG_BT_HFP_AG_VOICE_RECG_TEXT)
	int err;
	bool feature;

	LOG_DBG("");

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}
	hfp_ag_unlock(ag);

	if (!atomic_test_bit(ag->flags, BT_HFP_AG_VRE_ACTIVATE)) {
		LOG_WRN("Voice Recognition is not activated");
		return -EINVAL;
	}

	feature = BOTH_SUPT_FEAT(ag, BT_HFP_HF_FEATURE_ENH_VOICE_RECG,
				 BT_HFP_AG_FEATURE_ENH_VOICE_RECG);
	if (!feature) {
		return -ENOTSUP;
	}

	feature = BOTH_SUPT_FEAT(ag, BT_HFP_HF_FEATURE_VOICE_RECG_TEXT,
				 BT_HFP_AG_FEATURE_VOICE_RECG_TEXT);
	if (!feature) {
		return -ENOTSUP;
	}

	if ((state & BT_HFP_BVRA_STATE_SEND_AUDIO) &&
		!atomic_test_bit(ag->flags, BT_HFP_AG_VRE_R2A)) {
		LOG_ERR("HFP HF is not ready to accept audio input");
		return -EINVAL;
	}

	err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+BVRA:1,%d,%s,%d,%d,\"%s\"\r\n",
		state, id, type, operation, text);
	if (err) {
		LOG_ERR("Fail to send state of VRE :(%d)", err);
		return err;
	}

	return 0;
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_AG_VOICE_RECG_TEXT */
}

int bt_hfp_ag_signal_strength(struct bt_hfp_ag *ag, uint8_t strength)
{
	int err;

	LOG_DBG("");

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}
	hfp_ag_unlock(ag);

	err = hfp_ag_update_indicator(ag, BT_HFP_AG_SIGNAL_IND, strength,
						NULL, NULL);
	if (err) {
		LOG_ERR("Fail to set signal strength err :(%d)", err);
	}

	return err;
}

int bt_hfp_ag_roaming_status(struct bt_hfp_ag *ag, uint8_t status)
{
	int err;

	LOG_DBG("");

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}
	hfp_ag_unlock(ag);

	err = hfp_ag_update_indicator(ag, BT_HFP_AG_ROAM_IND, status,
						NULL, NULL);
	if (err) {
		LOG_ERR("Fail to set roaming status err :(%d)", err);
	}

	return err;
}

int bt_hfp_ag_battery_level(struct bt_hfp_ag *ag, uint8_t level)
{
	int err;

	LOG_DBG("");

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}
	hfp_ag_unlock(ag);

	err = hfp_ag_update_indicator(ag, BT_HFP_AG_BATTERY_IND, level,
						NULL, NULL);
	if (err) {
		LOG_ERR("Fail to set battery level err :(%d)", err);
	}

	return err;
}

int bt_hfp_ag_service_availability(struct bt_hfp_ag *ag, bool available)
{
	int err;

	LOG_DBG("");

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}
	hfp_ag_unlock(ag);

	err = hfp_ag_update_indicator(ag, BT_HFP_AG_SERVICE_IND,
		available ? 1 : 0, NULL, NULL);
	if (err) {
		LOG_ERR("Fail to set service availability err :(%d)", err);
	}

	return err;
}

int bt_hfp_ag_hf_indicator(struct bt_hfp_ag *ag, enum hfp_ag_hf_indicators indicator, bool enable)
{
#if defined(CONFIG_BT_HFP_AG_HF_INDICATORS)
	int err;
	uint32_t supported_indicators;

	LOG_DBG("");

	if (ag == NULL) {
		return -EINVAL;
	}

	hfp_ag_lock(ag);
	if (ag->state != BT_HFP_CONNECTED) {
		hfp_ag_unlock(ag);
		return -ENOTCONN;
	}

	supported_indicators = ag->hf_indicators_of_ag & ag->hf_indicators_of_hf;
	hfp_ag_unlock(ag);

	if (!(supported_indicators & BIT(indicator))) {
		LOG_ERR("Unsupported indicator %d", indicator);
		return -ENOTSUP;
	}

	hfp_ag_lock(ag);
	if (enable) {
		ag->hf_indicators |= BIT(indicator);
	} else {
		ag->hf_indicators &= ~BIT(indicator);
	}
	hfp_ag_unlock(ag);

	err = hfp_ag_send_data(ag, NULL, NULL, "\r\n+BIND:%d,%d\r\n",
		indicator, enable ? 1  : 0);
	if (err) {
		LOG_ERR("Fail to update registration status of indicator:(%d)", err);
	}

	return err;
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_HF_INDICATORS */
}
