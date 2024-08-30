/* hfp_hf.c - Hands free Profile - Handsfree side handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
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
#include <zephyr/bluetooth/classic/hfp_hf.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "l2cap_br_internal.h"
#include "rfcomm_internal.h"
#include "at.h"
#include "sco_internal.h"
#include "hfp_hf_internal.h"

#define LOG_LEVEL CONFIG_BT_HFP_HF_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hfp_hf);

#define MAX_IND_STR_LEN 17

struct bt_hfp_hf_cb *bt_hf;

NET_BUF_POOL_FIXED_DEFINE(hf_pool, CONFIG_BT_MAX_CONN + 1,
			  BT_RFCOMM_BUF_SIZE(BT_HF_CLIENT_MAX_PDU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_hfp_hf bt_hfp_hf_pool[CONFIG_BT_MAX_CONN];

#define HF_ENHANCED_CALL_STATUS_TIMEOUT 50 /* ms */

struct at_callback_set {
	void *resp;
	void *finish;
} __packed;

static inline void make_at_callback_set(void *storage, void *resp, void *finish)
{
	((struct at_callback_set *)storage)->resp = resp;
	((struct at_callback_set *)storage)->finish = finish;
}

static inline void *at_callback_set_resp(void *storage)
{
	return ((struct at_callback_set *)storage)->resp;
}

static inline void *at_callback_set_finish(void *storage)
{
	return ((struct at_callback_set *)storage)->finish;
}

/* The order should follow the enum hfp_hf_ag_indicators */
static const struct {
	char *name;
	uint32_t min;
	uint32_t max;
} ag_ind[] = {
	{"service", 0, 1}, /* HF_SERVICE_IND */
	{"call", 0, 1}, /* HF_CALL_IND */
	{"callsetup", 0, 3}, /* HF_CALL_SETUP_IND */
	{"callheld", 0, 2}, /* HF_CALL_HELD_IND */
	{"signal", 0, 5}, /* HF_SIGNAL_IND */
	{"roam", 0, 1}, /* HF_ROAM_IND */
	{"battchg", 0, 5} /* HF_BATTERY_IND */
};

/* HFP Hands-Free SDP record */
static struct bt_sdp_attribute hfp_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_HANDSFREE_SVCLASS)
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
				BT_SDP_ARRAY_8(BT_RFCOMM_CHAN_HFP_HF)
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
	/* The values of the “SupportedFeatures” bitmap shall be the same as the
	 * values of the Bits 0 to 4 of the AT-command AT+BRSF (see Section 5.3).
	 */
	BT_SDP_SUPPORTED_FEATURES(BT_HFP_HF_SDP_SUPPORTED_FEATURES),
};

static struct bt_sdp_record hfp_rec = BT_SDP_RECORD(hfp_attrs);

void hf_slc_error(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	int err;

	LOG_ERR("SLC error: disconnecting");
	err = bt_rfcomm_dlc_disconnect(&hf->rfcomm_dlc);
	if (err) {
		LOG_ERR("Rfcomm: Unable to disconnect :%d", -err);
	}
}

static void hfp_hf_send_failed(struct bt_hfp_hf *hf)
{
	int err;

	LOG_ERR("SLC error: disconnecting");

	err = bt_rfcomm_dlc_disconnect(&hf->rfcomm_dlc);
	if (err) {
		LOG_ERR("Fail to disconnect: %d", err);
	}
}

static void hfp_hf_send_data(struct bt_hfp_hf *hf);

static int hfp_hf_common_finish(struct at_client *at, enum at_result result,
			  enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(at, struct bt_hfp_hf, at);
	int err = 0;

	if (result != AT_RESULT_OK) {
		LOG_WRN("Fail to send AT command (result %d, cme err %d) on %p",
		    result, cme_err, hf);
	}

	if (hf->backup_finish) {
		err = hf->backup_finish(at, result, cme_err);
		hf->backup_finish = NULL;
	}

	if (atomic_test_and_clear_bit(hf->flags, BT_HFP_HF_FLAG_TX_ONGOING)) {
		LOG_DBG("TX is done on %p", hf);
	} else {
		LOG_WRN("Tx is not ongoing on %p", hf);
	}

	hfp_hf_send_data(hf);
	return err;
}

static void hfp_hf_send_data(struct bt_hfp_hf *hf)
{
	struct net_buf *buf;
	at_resp_cb_t resp;
	at_finish_cb_t finish;
	int err;

	if (atomic_test_bit(hf->flags, BT_HFP_HF_FLAG_RX_ONGOING)) {
		return;
	}

	if (atomic_test_and_set_bit(hf->flags, BT_HFP_HF_FLAG_TX_ONGOING)) {
		return;
	}

	buf = k_fifo_get(&hf->tx_pending, K_NO_WAIT);
	if (!buf) {
		atomic_clear_bit(hf->flags, BT_HFP_HF_FLAG_TX_ONGOING);
		return;
	}

	resp = (at_resp_cb_t)at_callback_set_resp(buf->user_data);
	finish = (at_finish_cb_t)at_callback_set_finish(buf->user_data);

	/*
	 * Backup the `finish` callback.
	 * Provide a default finish callback to drive the next sending.
	 */
	hf->backup_finish = finish;
	finish = hfp_hf_common_finish;

	make_at_callback_set(buf->user_data, NULL, NULL);
	at_register(&hf->at, resp, finish);

	err = bt_rfcomm_dlc_send(&hf->rfcomm_dlc, buf);
	if (err < 0) {
		LOG_ERR("Rfcomm send error :(%d)", err);
		atomic_clear_bit(hf->flags, BT_HFP_HF_FLAG_TX_ONGOING);
		net_buf_unref(buf);
		hfp_hf_send_failed(hf);
	}
}

int hfp_hf_send_cmd(struct bt_hfp_hf *hf, at_resp_cb_t resp,
		    at_finish_cb_t finish, const char *format, ...)
{
	struct net_buf *buf;
	va_list vargs;
	int ret;

	buf = bt_rfcomm_create_pdu(&hf_pool);
	if (!buf) {
		LOG_ERR("No Buffers!");
		return -ENOMEM;
	}

	make_at_callback_set(buf->user_data, resp, finish);

	va_start(vargs, format);
	ret = vsnprintk(buf->data, (net_buf_tailroom(buf) - 1), format, vargs);
	if (ret < 0) {
		LOG_ERR("Unable to format variable arguments");
		return ret;
	}
	va_end(vargs);

	net_buf_add(buf, ret);
	net_buf_add_u8(buf, '\r');

	LOG_DBG("HF %p, DLC %p sending buf %p", hf, &hf->rfcomm_dlc, buf);

	k_fifo_put(&hf->tx_pending, buf);
	hfp_hf_send_data(hf);

	return 0;
}

int brsf_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	uint32_t val;
	int ret;

	ret = at_get_number(hf_at, &val);
	if (ret < 0) {
		LOG_ERR("Error getting value");
		return ret;
	}

	hf->ag_features = val;

	return 0;
}

int brsf_resp(struct at_client *hf_at, struct net_buf *buf)
{
	int err;

	LOG_DBG("");

	err = at_parse_cmd_input(hf_at, buf, "BRSF", brsf_handle,
				 AT_CMD_TYPE_NORMAL);
	if (err < 0) {
		/* Returning negative value is avoided before SLC connection
		 * established.
		 */
		LOG_ERR("Error parsing CMD input");
		hf_slc_error(hf_at);
	}

	return 0;
}

static void cind_handle_values(struct at_client *hf_at, uint32_t index,
			       char *name, uint32_t min, uint32_t max)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	int i;

	LOG_DBG("index: %u, name: %s, min: %u, max:%u", index, name, min, max);

	for (i = 0; i < ARRAY_SIZE(ag_ind); i++) {
		if (strcmp(name, ag_ind[i].name) != 0) {
			continue;
		}
		if (min != ag_ind[i].min || max != ag_ind[i].max) {
			LOG_ERR("%s indicator min/max value not matching", name);
		}

		hf->ind_table[index] = i;
		break;
	}
}

int cind_handle(struct at_client *hf_at)
{
	uint32_t index = 0U;

	/* Parsing Example: CIND: ("call",(0,1)) etc.. */
	while (at_has_next_list(hf_at)) {
		char name[MAX_IND_STR_LEN];
		uint32_t min, max;

		if (at_open_list(hf_at) < 0) {
			LOG_ERR("Could not get open list");
			goto error;
		}

		if (at_list_get_string(hf_at, name, sizeof(name)) < 0) {
			LOG_ERR("Could not get string");
			goto error;
		}

		if (at_open_list(hf_at) < 0) {
			LOG_ERR("Could not get open list");
			goto error;
		}

		if (at_list_get_range(hf_at, &min, &max) < 0) {
			LOG_ERR("Could not get range");
			goto error;
		}

		if (at_close_list(hf_at) < 0) {
			LOG_ERR("Could not get close list");
			goto error;
		}

		if (at_close_list(hf_at) < 0) {
			LOG_ERR("Could not get close list");
			goto error;
		}

		cind_handle_values(hf_at, index, name, min, max);
		index++;
	}

	return 0;
error:
	LOG_ERR("Error on CIND response");
	hf_slc_error(hf_at);
	return -EINVAL;
}

int cind_resp(struct at_client *hf_at, struct net_buf *buf)
{
	int err;

	err = at_parse_cmd_input(hf_at, buf, "CIND", cind_handle,
				 AT_CMD_TYPE_NORMAL);
	if (err < 0) {
		LOG_ERR("Error parsing CMD input");
		hf_slc_error(hf_at);
	}

	return 0;
}

static void free_call(struct bt_hfp_hf_call *call)
{
	memset(call, 0, sizeof(*call));
}

#if defined(CONFIG_BT_HFP_HF_ECS)
static struct bt_hfp_hf_call *get_call_with_index(struct bt_hfp_hf *hf, uint8_t index)
{
	struct bt_hfp_hf_call *call;

	for (size_t i = 0; i < ARRAY_SIZE(hf->calls); i++) {
		call = &hf->calls[i];

		if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_IN_USING)) {
			continue;
		}

		if (call->index == index) {
			return call;
		}
	}

	return NULL;
}

static struct bt_hfp_hf_call *get_call_without_index(struct bt_hfp_hf *hf)
{
	struct bt_hfp_hf_call *call;

	for (size_t index = 0; index < ARRAY_SIZE(hf->calls); index++) {
		call = &hf->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_IN_USING)) {
			continue;
		}

		if (!call->index) {
			return call;
		}
	}

	return NULL;
}
#endif /* CONFIG_BT_HFP_HF_ECS */

static void hf_reject_call(struct bt_hfp_hf_call *call)
{
	if (bt_hf->reject) {
		bt_hf->reject(call);
	}
	free_call(call);
}

static void hf_terminate_call(struct bt_hfp_hf_call *call)
{
	if (bt_hf->terminate) {
		bt_hf->terminate(call);
	}
	free_call(call);
}

static void clear_call_without_clcc(struct bt_hfp_hf *hf)
{
	struct bt_hfp_hf_call *call;

	for (size_t index = 0; index < ARRAY_SIZE(hf->calls); index++) {
		call = &hf->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_IN_USING)) {
			continue;
		}

		if (atomic_test_and_clear_bit(call->flags, BT_HFP_HF_CALL_CLCC)) {
			continue;
		}

		switch (atomic_get(call->state)) {
		case BT_HFP_HF_CALL_STATE_TERMINATE:
			break;
		case BT_HFP_HF_CALL_STATE_OUTGOING:
		case BT_HFP_HF_CALL_STATE_INCOMING:
		case BT_HFP_HF_CALL_STATE_ALERTING:
		case BT_HFP_HF_CALL_STATE_WAITING:
			hf_reject_call(call);
			break;
		case BT_HFP_HF_CALL_STATE_ACTIVE:
		case BT_HFP_HF_CALL_STATE_HELD:
			hf_terminate_call(call);
			break;
		default:
			free_call(call);
			break;
		}
	}
}

static int clcc_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+CLCC (result %d) on %p", result, hf);

	if (result == AT_RESULT_OK) {
		clear_call_without_clcc(hf);
	}

	atomic_clear_bit(hf->flags, BT_HFP_HF_FLAG_CLCC_PENDING);

	return 0;
}

static void clear_call_clcc_state(struct bt_hfp_hf *hf)
{
	struct bt_hfp_hf_call *call;

	for (size_t index = 0; index < ARRAY_SIZE(hf->calls); index++) {
		call = &hf->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_IN_USING)) {
			continue;
		}

		atomic_clear_bit(call->flags, BT_HFP_HF_CALL_CLCC);
	}
}

static void hf_query_current_calls(struct bt_hfp_hf *hf)
{
	int err;

	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return;
	}

	if (!atomic_test_bit(hf->flags, BT_HFP_HF_FLAG_CONNECTED)) {
		return;
	}

	if (!(hf->ag_features & BT_HFP_AG_FEATURE_ECS)) {
		return;
	}

	if (!(hf->hf_features & BT_HFP_HF_FEATURE_ECS)) {
		return;
	}

	if (atomic_test_bit(hf->flags, BT_HFP_HF_FLAG_CLCC_PENDING)) {
		k_work_reschedule(&hf->deferred_work, K_MSEC(HF_ENHANCED_CALL_STATUS_TIMEOUT));
		return;
	}

	clear_call_clcc_state(hf);

	err = hfp_hf_send_cmd(hf, NULL, clcc_finish, "AT+CLCC");
	if (err < 0) {
		LOG_ERR("Fail to query current calls on %p", hf);
	}
}

static void hf_call_state_update(struct bt_hfp_hf_call *call, int state)
{
	int old_state;

	old_state = atomic_get(call->state);
	atomic_set(call->state, state);

	LOG_DBG("Call %p state update %d->%d", call, old_state, state);

	switch (state) {
	case BT_HFP_HF_CALL_STATE_TERMINATE:
		free_call(call);
		break;
	case BT_HFP_HF_CALL_STATE_OUTGOING:
		break;
	case BT_HFP_HF_CALL_STATE_INCOMING:
		break;
	case BT_HFP_HF_CALL_STATE_ALERTING:
		k_work_reschedule(&call->hf->deferred_work,
				  K_MSEC(HF_ENHANCED_CALL_STATUS_TIMEOUT));
		break;
	case BT_HFP_HF_CALL_STATE_WAITING:
		k_work_reschedule(&call->hf->deferred_work,
				  K_MSEC(HF_ENHANCED_CALL_STATUS_TIMEOUT));
		break;
	case BT_HFP_HF_CALL_STATE_ACTIVE:
		break;
	case BT_HFP_HF_CALL_STATE_HELD:
		break;
	}
}

#if defined(CONFIG_BT_HFP_HF_ECS)
static void call_state_update(struct bt_hfp_hf_call *call, uint32_t status)
{
	switch (status) {
	case BT_HFP_CLCC_STATUS_ACTIVE:
		if ((atomic_get(call->state) != BT_HFP_HF_CALL_STATE_ACTIVE) ||
		    ((atomic_get(call->state) == BT_HFP_HF_CALL_STATE_ACTIVE) &&
		    atomic_test_and_clear_bit(call->flags, BT_HFP_HF_CALL_INCOMING_HELD))) {
			atomic_val_t state;

			state = atomic_get(call->state);
			hf_call_state_update(call, BT_HFP_HF_CALL_STATE_ACTIVE);
			if (state == BT_HFP_HF_CALL_STATE_HELD) {
				if (bt_hf->retrieve) {
					bt_hf->retrieve(call);
				}
			} else {
				if (bt_hf->accept) {
					bt_hf->accept(call);
				}
			}
		}
		break;
	case BT_HFP_CLCC_STATUS_HELD:
		if ((atomic_get(call->state) == BT_HFP_HF_CALL_STATE_ACTIVE) &&
		    !atomic_test_and_clear_bit(call->flags, BT_HFP_HF_CALL_INCOMING_HELD)) {
			hf_call_state_update(call, BT_HFP_HF_CALL_STATE_HELD);
			if (bt_hf->held) {
				bt_hf->held(call);
			}
		}
		break;
	case BT_HFP_CLCC_STATUS_DIALING:
		break;
	case BT_HFP_CLCC_STATUS_ALERTING:
		break;
	case BT_HFP_CLCC_STATUS_INCOMING:
		break;
	case BT_HFP_CLCC_STATUS_WAITING:
		break;
	case BT_HFP_CLCC_STATUS_CALL_HELD_HOLD:
		if ((atomic_get(call->state) == BT_HFP_HF_CALL_STATE_INCOMING) ||
		    (atomic_get(call->state) == BT_HFP_HF_CALL_STATE_WAITING)) {
			atomic_set_bit(call->flags, BT_HFP_HF_CALL_INCOMING_HELD);
			hf_call_state_update(call, BT_HFP_HF_CALL_STATE_ACTIVE);
			if (bt_hf->incoming_held) {
				bt_hf->incoming_held(call);
			}
		}
		break;
	}
}

static int clcc_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	int err;
	struct bt_hfp_hf_call *call;
	uint32_t index;
	uint32_t dir;
	uint32_t status;
	uint32_t mode;
	uint32_t mpty;
	char *number = NULL;
	uint32_t type = 0;
	bool incoming = false;

	err = at_get_number(hf_at, &index);
	if (err < 0) {
		LOG_ERR("Error getting index");
		return err;
	}

	call = get_call_with_index(hf, (uint8_t)index);
	if (!call) {
		LOG_INF("Valid call with index %d not found", index);
		call = get_call_without_index(hf);
		if (!call) {
			LOG_INF("Not available call");
			return 0;
		}
		call->index = (uint8_t)index;
	}

	atomic_set_bit(call->flags, BT_HFP_HF_CALL_CLCC);

	err = at_get_number(hf_at, &dir);
	if (err < 0) {
		LOG_ERR("Error getting dir");
		return err;
	}

	if (atomic_test_bit(call->flags, BT_HFP_HF_CALL_INCOMING) ||
		atomic_test_bit(call->flags, BT_HFP_HF_CALL_INCOMING_3WAY)) {
		incoming = true;
	}

	if (incoming != !!dir) {
		LOG_ERR("Call dir of HF is not aligned with AG");
		return 0;
	}

	err = at_get_number(hf_at, &status);
	if (err < 0) {
		LOG_ERR("Error getting status");
		return err;
	}

	err = at_get_number(hf_at, &mode);
	if (err < 0) {
		LOG_ERR("Error getting mode");
		return err;
	}

	err = at_get_number(hf_at, &mpty);
	if (err < 0) {
		LOG_ERR("Error getting mpty");
		return err;
	}

	number = at_get_string(hf_at);

	if (number) {
		(void)at_get_number(hf_at, &type);
	}

	call_state_update(call, status);

	LOG_DBG("CLCC idx %d dir %d status %d mode %d mpty %d number %s type %d",
		index, dir, status, mode, mpty, number, type);

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_ECS */

#if defined(CONFIG_BT_HFP_HF_VOICE_RECG)
static int bvra_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	int err;
	uint32_t activate;
	uint32_t state;
	char *id;
	char text_id[BT_HFP_BVRA_TEXT_ID_MAX_LEN + 1];
	size_t id_len;
	uint32_t type;
	uint32_t operation;
	char *text;

	err = at_get_number(hf_at, &activate);
	if (err < 0) {
		LOG_ERR("Error getting activate");
		return err;
	}

	if (activate) {
		if (!atomic_test_and_set_bit(hf->flags, BT_HFP_HF_FLAG_VRE_ACTIVATE)) {
			if (bt_hf->voice_recognition) {
				bt_hf->voice_recognition(hf, true);
			}
		}
	} else {
		if (atomic_test_and_clear_bit(hf->flags, BT_HFP_HF_FLAG_VRE_ACTIVATE)) {
			if (bt_hf->voice_recognition) {
				bt_hf->voice_recognition(hf, false);
			}
		}
	}

#if defined(CONFIG_BT_HFP_HF_ENH_VOICE_RECG)
	err = at_get_number(hf_at, &state);
	if (err < 0) {
		LOG_INF("Error getting VRE state");
		return 0;
	}

	if (bt_hf->vre_state) {
		bt_hf->vre_state(hf, (uint8_t)state);
	}
#endif /* CONFIG_BT_HFP_HF_ENH_VOICE_RECG */

#if defined(CONFIG_BT_HFP_HF_VOICE_RECG_TEXT)
	id = at_get_raw_string(hf_at, &id_len);
	if (!id) {
		LOG_INF("Error getting text ID");
		return 0;
	}

	if (id_len > BT_HFP_BVRA_TEXT_ID_MAX_LEN) {
		LOG_ERR("Invalid text ID length %d", id_len);
		return -ENOTSUP;
	}

	strncpy(text_id, id, MIN(id_len, BT_HFP_BVRA_TEXT_ID_MAX_LEN));
	text_id[MIN(id_len, BT_HFP_BVRA_TEXT_ID_MAX_LEN)] = '\0';

	err = at_get_number(hf_at, &type);
	if (err < 0) {
		LOG_INF("Error getting text type");
		return 0;
	}

	err = at_get_number(hf_at, &operation);
	if (err < 0) {
		LOG_INF("Error getting text operation");
		return 0;
	}

	text = at_get_string(hf_at);
	if (!text) {
		LOG_INF("Error getting text string");
		return 0;
	}

	if (bt_hf->textual_representation) {
		bt_hf->textual_representation(hf, text_id, (uint8_t)type,
			(uint8_t)operation, text);
	}
#endif /* CONFIG_BT_HFP_HF_VOICE_RECG_TEXT */
	return 0;
}
#endif /* CONFIG_BT_HFP_HF_VOICE_RECG */

static struct bt_hfp_hf_call *get_dialing_call(struct bt_hfp_hf *hf)
{
	struct bt_hfp_hf_call *call;

	for (size_t index = 0; index < ARRAY_SIZE(hf->calls); index++) {
		call = &hf->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_IN_USING)) {
			continue;
		}

		switch (atomic_get(call->state)) {
		case BT_HFP_HF_CALL_STATE_OUTGOING:
		case BT_HFP_HF_CALL_STATE_INCOMING:
		case BT_HFP_HF_CALL_STATE_ALERTING:
		case BT_HFP_HF_CALL_STATE_WAITING:
			return call;
		}
	}

	return NULL;
}

static struct bt_hfp_hf_call *get_call_with_state(struct bt_hfp_hf *hf, int state)
{
	struct bt_hfp_hf_call *call;

	for (size_t index = 0; index < ARRAY_SIZE(hf->calls); index++) {
		call = &hf->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_IN_USING)) {
			continue;
		}

		if (atomic_get(call->state) == state) {
			return call;
		}
	}

	return NULL;
}

static struct bt_hfp_hf_call *get_call_with_flag(struct bt_hfp_hf *hf, int flag)
{
	struct bt_hfp_hf_call *call;

	for (size_t index = 0; index < ARRAY_SIZE(hf->calls); index++) {
		call = &hf->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_IN_USING)) {
			continue;
		}

		if (atomic_test_bit(call->flags, flag)) {
			return call;
		}
	}

	return NULL;
}

static struct bt_hfp_hf_call *get_call_with_state_and_flag(struct bt_hfp_hf *hf,
		int state, int flag)
{
	struct bt_hfp_hf_call *call;

	for (size_t index = 0; index < ARRAY_SIZE(hf->calls); index++) {
		call = &hf->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_IN_USING)) {
			continue;
		}

		if ((atomic_get(call->state) == state) &&
			atomic_test_bit(call->flags, flag)) {
			return call;
		}
	}

	return NULL;
}

static struct bt_hfp_hf_call *get_using_call(struct bt_hfp_hf *hf)
{
	struct bt_hfp_hf_call *call;

	for (size_t index = 0; index < ARRAY_SIZE(hf->calls); index++) {
		call = &hf->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_IN_USING)) {
			continue;
		}

		return call;
	}

	return NULL;
}

static void bt_hf_deferred_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_hfp_hf *hf = CONTAINER_OF(dwork, struct bt_hfp_hf, deferred_work);

	hf_query_current_calls(hf);
}

static struct bt_hfp_hf_call *get_new_call(struct bt_hfp_hf *hf)
{
	struct bt_hfp_hf_call *call;

	for (size_t index = 0; index < ARRAY_SIZE(hf->calls); index++) {
		call = &hf->calls[index];

		if (atomic_test_and_set_bit(call->flags, BT_HFP_HF_CALL_IN_USING)) {
			continue;
		}

		call->hf = hf;

		return call;
	}

	return NULL;
}

static int get_using_call_count(struct bt_hfp_hf *hf)
{
	struct bt_hfp_hf_call *call;
	int count = 0;

	for (size_t index = 0; index < ARRAY_SIZE(hf->calls); index++) {
		call = &hf->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_IN_USING)) {
			continue;
		}

		count++;
	}

	return count;
}

static void set_all_calls_held_state(struct bt_hfp_hf *hf, bool held)
{
	struct bt_hfp_hf_call *call;

	for (size_t index = 0; index < ARRAY_SIZE(hf->calls); index++) {
		call = &hf->calls[index];

		if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_IN_USING)) {
			continue;
		}

		if (held && (atomic_get(call->state) == BT_HFP_HF_CALL_STATE_ACTIVE)) {
			hf_call_state_update(call, BT_HFP_HF_CALL_STATE_HELD);
			if (bt_hf->held) {
				bt_hf->held(call);
			}
		}

		if (!held && (atomic_get(call->state) == BT_HFP_HF_CALL_STATE_HELD)) {
			hf_call_state_update(call, BT_HFP_HF_CALL_STATE_ACTIVE);
			if (bt_hf->retrieve) {
				bt_hf->retrieve(call);
			}
		}
	}
}

static void ag_indicator_handle_call(struct bt_hfp_hf *hf, uint32_t value)
{
	struct bt_hfp_hf_call *call;

	LOG_DBG("call %d", value);

	if (value) {
		call = get_dialing_call(hf);
		if (!call) {
			return;
		}

		hf_call_state_update(call, BT_HFP_HF_CALL_STATE_ACTIVE);

		if (atomic_test_bit(call->flags, BT_HFP_HF_CALL_INCOMING_HELD)) {
			if (bt_hf->incoming_held) {
				bt_hf->incoming_held(call);
			}
		} else {
			if (bt_hf->accept) {
				bt_hf->accept(call);
			}
		}
	} else {
		do {
			call = get_using_call(hf);
			if (!call) {
				return;
			}

			switch (atomic_get(call->state)) {
			case BT_HFP_HF_CALL_STATE_OUTGOING:
			case BT_HFP_HF_CALL_STATE_INCOMING:
			case BT_HFP_HF_CALL_STATE_ALERTING:
			case BT_HFP_HF_CALL_STATE_WAITING:
				hf_reject_call(call);
				break;
			case BT_HFP_HF_CALL_STATE_ACTIVE:
				if (atomic_test_and_clear_bit(call->flags,
							      BT_HFP_HF_CALL_INCOMING_HELD)) {
					hf_reject_call(call);
					break;
				}
				__fallthrough;
			case BT_HFP_HF_CALL_STATE_HELD:
				hf_terminate_call(call);
				break;
			default:
				free_call(call);
				break;
			}
		} while (call);
	}
}

static void ag_indicator_handle_call_setup(struct bt_hfp_hf *hf, uint32_t value)
{
	struct bt_hfp_hf_call *call;
	int call_count;

	call_count = get_using_call_count(hf);

	LOG_DBG("call setup %d", value);

	switch (value) {
	case BT_HFP_CALL_SETUP_NONE:
		if (call_count == 1) {
			call = get_using_call(hf);
			if (!call) {
				break;
			}

			if ((atomic_get(call->state) == BT_HFP_HF_CALL_STATE_ACTIVE) ||
				(atomic_get(call->state) == BT_HFP_HF_CALL_STATE_HELD)) {
				break;
			}

			hf_reject_call(call);
		} else {
			call = get_dialing_call(hf);
			if (!call) {
				break;
			}

			if (atomic_get(call->state) == BT_HFP_HF_CALL_STATE_OUTGOING) {
				LOG_INF("The outgoing is not alerted");
				hf_reject_call(call);
			} else {
				LOG_INF("Waiting for +CIEV: (callheld = 1)");
				k_work_reschedule(&hf->deferred_work,
						  K_MSEC(HF_ENHANCED_CALL_STATUS_TIMEOUT));
			}
		}
		break;
	case BT_HFP_CALL_SETUP_INCOMING:
		call = get_call_with_state(hf, BT_HFP_HF_CALL_STATE_INCOMING);
		if (!call) {
			call = get_new_call(hf);
			if (!call) {
				break;
			}
			hf_call_state_update(call, BT_HFP_HF_CALL_STATE_INCOMING);
		}

		if (call_count == 0) {
			atomic_set_bit(call->flags, BT_HFP_HF_CALL_INCOMING);
		} else {
			atomic_set_bit(call->flags, BT_HFP_HF_CALL_INCOMING_3WAY);
		}
		if (bt_hf->incoming) {
			bt_hf->incoming(hf, call);
		}
		hf_call_state_update(call, BT_HFP_HF_CALL_STATE_WAITING);
		break;
	case BT_HFP_CALL_SETUP_OUTGOING:
		call = get_call_with_state(hf, BT_HFP_HF_CALL_STATE_OUTGOING);
		if (!call) {
			call = get_new_call(hf);
			if (!call) {
				break;
			}
			hf_call_state_update(call, BT_HFP_HF_CALL_STATE_OUTGOING);
		}

		if (call_count) {
			atomic_set_bit(call->flags, BT_HFP_HF_CALL_OUTGOING_3WAY);
		}
		if (bt_hf->outgoing) {
			bt_hf->outgoing(hf, call);
		}
		break;
	case BT_HFP_CALL_SETUP_REMOTE_ALERTING:
		call = get_call_with_state(hf, BT_HFP_HF_CALL_STATE_OUTGOING);
		if (!call) {
			break;
		}

		hf_call_state_update(call, BT_HFP_HF_CALL_STATE_ALERTING);
		if (bt_hf->remote_ringing) {
			bt_hf->remote_ringing(call);
		}
		break;
	default:
		break;
	}
}

static void ag_indicator_handle_call_held(struct bt_hfp_hf *hf, uint32_t value)
{
	struct bt_hfp_hf_call *call;

	k_work_reschedule(&hf->deferred_work, K_MSEC(HF_ENHANCED_CALL_STATUS_TIMEOUT));

	LOG_DBG("call setup %d", value);

	switch (value) {
	case BT_HFP_CALL_HELD_NONE:
		set_all_calls_held_state(hf, false);
		break;
	case BT_HFP_CALL_HELD_ACTIVE_HELD:
		call = get_call_with_state(hf, BT_HFP_HF_CALL_STATE_ALERTING);
		if (!call) {
			call = get_call_with_state(hf, BT_HFP_HF_CALL_STATE_WAITING);
			if (!call) {
				break;
			}
		}

		hf_call_state_update(call, BT_HFP_HF_CALL_STATE_ACTIVE);
		if (bt_hf->accept) {
			bt_hf->accept(call);
		}
		break;
	case BT_HFP_CALL_HELD_HELD:
		set_all_calls_held_state(hf, true);
		break;
	default:
		break;
	}
}

void ag_indicator_handle_values(struct at_client *hf_at, uint32_t index,
				uint32_t value)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("Index :%u, Value :%u", index, value);

	if (index >= ARRAY_SIZE(ag_ind)) {
		LOG_ERR("Max only %zu indicators are supported", ARRAY_SIZE(ag_ind));
		return;
	}

	if (value > ag_ind[hf->ind_table[index]].max ||
	    value < ag_ind[hf->ind_table[index]].min) {
		LOG_ERR("Indicators out of range - value: %u", value);
		return;
	}

	switch (hf->ind_table[index]) {
	case HF_SERVICE_IND:
		if (bt_hf->service) {
			bt_hf->service(hf, value);
		}
		break;
	case HF_CALL_IND:
		ag_indicator_handle_call(hf, value);
		break;
	case HF_CALL_SETUP_IND:
		ag_indicator_handle_call_setup(hf, value);
		break;
	case HF_CALL_HELD_IND:
		ag_indicator_handle_call_held(hf, value);
		break;
	case HF_SIGNAL_IND:
		if (bt_hf->signal) {
			bt_hf->signal(hf, value);
		}
		break;
	case HF_ROAM_IND:
		if (bt_hf->roam) {
			bt_hf->roam(hf, value);
		}
		break;
	case HF_BATTERY_IND:
		if (bt_hf->battery) {
			bt_hf->battery(hf, value);
		}
		break;
	default:
		LOG_ERR("Unknown AG indicator");
		break;
	}
}

int cind_status_handle(struct at_client *hf_at)
{
	uint32_t index = 0U;

	while (at_has_next_list(hf_at)) {
		uint32_t value;
		int ret;

		ret = at_get_number(hf_at, &value);
		if (ret < 0) {
			LOG_ERR("could not get the value");
			return ret;
		}

		ag_indicator_handle_values(hf_at, index, value);

		index++;
	}

	return 0;
}

int cind_status_resp(struct at_client *hf_at, struct net_buf *buf)
{
	int err;

	err = at_parse_cmd_input(hf_at, buf, "CIND", cind_status_handle,
				 AT_CMD_TYPE_NORMAL);
	if (err < 0) {
		LOG_ERR("Error parsing CMD input");
		hf_slc_error(hf_at);
	}

	return 0;
}

int ciev_handle(struct at_client *hf_at)
{
	uint32_t index, value;
	int ret;

	ret = at_get_number(hf_at, &index);
	if (ret < 0) {
		LOG_ERR("could not get the Index");
		return ret;
	}
	/* The first element of the list shall have 1 */
	if (!index) {
		LOG_ERR("Invalid index value '0'");
		return 0;
	}

	ret = at_get_number(hf_at, &value);
	if (ret < 0) {
		LOG_ERR("could not get the value");
		return ret;
	}

	ag_indicator_handle_values(hf_at, (index - 1), value);

	return 0;
}

int ring_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	struct bt_hfp_hf_call *call;

	call = get_call_with_state(hf, BT_HFP_HF_CALL_STATE_WAITING);
	if (!call) {
		return 0;
	}

	if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_INCOMING)) {
		LOG_WRN("Invalid call dir (outgoing)");
	}

	if (bt_hf->ring_indication) {
		bt_hf->ring_indication(call);
	}

	return 0;
}

#if defined(CONFIG_BT_HFP_HF_CLI)
int clip_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	char *number;
	uint32_t type;
	int err;
	struct bt_hfp_hf_call *call;

	number = at_get_string(hf_at);

	err = at_get_number(hf_at, &type);
	if (err) {
		LOG_WRN("could not get the type");
	} else {
		type = 0;
	}


	call = get_call_with_state(hf, BT_HFP_HF_CALL_STATE_WAITING);
	if (!call) {
		return 0;
	}

	if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_INCOMING)) {
		LOG_WRN("Invalid call dir (outgoing)");
	}

	if (bt_hf->clip) {
		bt_hf->clip(call, number, (uint8_t)type);
	}

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_CLI */

#if defined(CONFIG_BT_HFP_HF_VOLUME)
int vgm_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	uint32_t gain;
	int err;

	err = at_get_number(hf_at, &gain);
	if (err) {
		LOG_ERR("could not get the microphone gain");
		return err;
	}

	if (gain > BT_HFP_HF_VGM_GAIN_MAX) {
		LOG_ERR("Invalid microphone gain (%d > %d)", gain, BT_HFP_HF_VGM_GAIN_MAX);
		return -EINVAL;
	}

	if (bt_hf->vgm) {
		bt_hf->vgm(hf, (uint8_t)gain);
	}

	return 0;
}

int vgs_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	uint32_t gain;
	int err;

	err = at_get_number(hf_at, &gain);
	if (err) {
		LOG_ERR("could not get the speaker gain");
		return err;
	}

	if (gain > BT_HFP_HF_VGS_GAIN_MAX) {
		LOG_ERR("Invalid speaker gain (%d > %d)", gain, BT_HFP_HF_VGS_GAIN_MAX);
		return -EINVAL;
	}

	if (bt_hf->vgs) {
		bt_hf->vgs(hf, (uint8_t)gain);
	}

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_VOLUME */

int bsir_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	uint32_t inband;
	int err;

	err = at_get_number(hf_at, &inband);
	if (err) {
		LOG_ERR("could not get bsir value");
		return err;
	}

	if (inband > 1) {
		LOG_ERR("Invalid %d bsir value", inband);
		return -EINVAL;
	}

	if (bt_hf->inband_ring) {
		bt_hf->inband_ring(hf, (bool)inband);
	}

	return 0;
}

#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
int bcs_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	uint32_t codec_id;
	int err;

	err = at_get_number(hf_at, &codec_id);
	if (err) {
		LOG_ERR("could not get bcs value");
		return err;
	}

	if (!(hf->hf_codec_ids & BIT(codec_id))) {
		LOG_ERR("Invalid codec id %d", codec_id);
		err = bt_hfp_hf_set_codecs(hf, hf->hf_codec_ids);
		return err;
	}

	atomic_set_bit(hf->flags, BT_HFP_HF_FLAG_CODEC_CONN);

	if (bt_hf->codec_negotiate) {
		bt_hf->codec_negotiate(hf, codec_id);
		return 0;
	}

	err = bt_hfp_hf_select_codec(hf, codec_id);
	return err;
}
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */

static int btrh_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	uint32_t on_hold;
	int err;
	struct bt_hfp_hf_call *call;

	err = at_get_number(hf_at, &on_hold);
	if (err < 0) {
		LOG_ERR("Error getting value");
		return err;
	}

	if (on_hold == BT_HFP_BTRH_ON_HOLD) {
		call = get_call_with_state(hf, BT_HFP_HF_CALL_STATE_WAITING);
		if (!call) {
			return 0;
		}

		if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_INCOMING)) {
			LOG_WRN("Invalid call dir (outgoing)");
		}
		atomic_set_bit(call->flags, BT_HFP_HF_CALL_INCOMING_HELD);
	} else {
		call = get_call_with_state_and_flag(hf,
			BT_HFP_HF_CALL_STATE_ACTIVE, BT_HFP_HF_CALL_INCOMING_HELD);
		if (!call) {
			return 0;
		}

		if (on_hold == BT_HFP_BTRH_ACCEPTED) {
			atomic_clear_bit(call->flags, BT_HFP_HF_CALL_INCOMING_HELD);
			if (bt_hf && bt_hf->accept) {
				bt_hf->accept(call);
			}
		} else if (on_hold == BT_HFP_BTRH_REJECTED) {
			hf_reject_call(call);
		} else {
			return -EINVAL;
		}
	}

	return 0;
}

#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
static int ccwa_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	char *number;
	uint32_t type;
	int err;
	struct bt_hfp_hf_call *call;

	number = at_get_string(hf_at);

	err = at_get_number(hf_at, &type);
	if (err) {
		LOG_WRN("could not get the type");
	} else {
		type = 0;
	}

	call = get_new_call(hf);
	if (!call) {
		LOG_ERR("Not available call object");
		return 0;
	}

	hf_call_state_update(call, BT_HFP_HF_CALL_STATE_INCOMING);
	if (bt_hf->call_waiting) {
		bt_hf->call_waiting(call, number, (uint8_t)type);
	}

	return 0;
}

static struct _chld_feature {
	const char *name;
	int value;
} chld_feature_map[] = {
	{ "1x", BT_HFP_CALL_RELEASE_SPECIFIED_ACTIVE },
	{ "2x", BT_HFP_CALL_PRIVATE_CNLTN_MODE },
	{ "0",  BT_HFP_CHLD_RELEASE_ALL },
	{ "1",  BT_HFP_CHLD_RELEASE_ACTIVE_ACCEPT_OTHER },
	{ "2",  BT_HFP_CALL_HOLD_ACTIVE_ACCEPT_OTHER },
	{ "3",  BT_HFP_CALL_ACTIVE_HELD },
	{ "4",  BT_HFP_CALL_QUITE },
};

static int get_chld_feature(const char *name)
{
	struct _chld_feature *chld;

	for (size_t index = 0; index < ARRAY_SIZE(chld_feature_map); index++) {
		chld = &chld_feature_map[index];
		if (!strncmp(name, chld->name, strlen(chld->name))) {
			return chld->value;
		}
	}

	return -EINVAL;
}

int chld_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	char *value;
	uint32_t chld_features = 0;
	int err;

	/* Parsing Example: CHLD: (0,1,2,3,4) */
	if (at_open_list(hf_at) < 0) {
		LOG_ERR("Could not get open list");
		goto error;
	}

	while (at_has_next_list(hf_at)) {
		value = at_get_raw_string(hf_at, NULL);
		if (!value) {
			LOG_ERR("Could not get value");
			goto error;
		}

		err = get_chld_feature(value);
		if (err < 0) {
			LOG_ERR("Cannot parse the value %s", value);
			goto error;
		}

		if (NUM_BITS(sizeof(chld_features)) > err) {
			chld_features |= BIT(err);
		}
	}

	if (at_close_list(hf_at) < 0) {
		LOG_ERR("Could not get close list");
		goto error;
	}

	if (!((chld_features & BIT(BT_HFP_CHLD_RELEASE_ACTIVE_ACCEPT_OTHER)) &&
	    (chld_features & BIT(BT_HFP_CALL_HOLD_ACTIVE_ACCEPT_OTHER)))) {
		LOG_ERR("AT+CHLD values 1 and 2 should be supported by AG");
		goto error;
	}

	hf->chld_features = chld_features;

	return 0;

error:
	LOG_ERR("Error on AT+CHLD=? response");
	hf_slc_error(hf_at);
	return -EINVAL;
}
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */

static int cnum_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	int err;
	char *alpha;
	char *number;
	uint32_t type;
	char *speed;
	uint32_t service = 4;

	alpha = at_get_raw_string(hf_at, NULL);
	number = at_get_string(hf_at);
	if (!number) {
		LOG_INF("Cannot get number");
		return -EINVAL;
	}

	err = at_get_number(hf_at, &type);
	if (err) {
		LOG_INF("Cannot get type");
		return -EINVAL;
	}

	speed = at_get_raw_string(hf_at, NULL);

	err = at_get_number(hf_at, &service);
	if (err) {
		LOG_INF("Cannot get service");
	}

	if (bt_hf->subscriber_number) {
		bt_hf->subscriber_number(hf, number, (uint8_t)type, (uint8_t)service);
	}

	LOG_DBG("CNUM number %s type %d service %d", number, type, service);

	return 0;
}

#if defined(CONFIG_BT_HFP_HF_HF_INDICATORS)
static int bind_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	int err;
	uint32_t index;
	uint32_t value;
	uint32_t ind = 0;
	uint32_t ind_enable = hf->ind_enable;

	err = at_open_list(hf_at);
	if (!err) {
		/* It is a list. */
		while (at_has_next_list(hf_at)) {
			err = at_get_number(hf_at, &index);
			if (err) {
				LOG_INF("Cannot get indicator");
				goto failed;
			}

			ind |= BIT(index);
		}

		if (at_close_list(hf_at) < 0) {
			LOG_ERR("Could not get close list");
			goto failed;
		}

		hf->ag_ind = ind;
		return 0;
	}

	err = at_get_number(hf_at, &index);
	if (err) {
		LOG_INF("Cannot get indicator");
		goto failed;
	}

	err = at_get_number(hf_at, &value);
	if (err) {
		LOG_INF("Cannot get status");
		goto failed;
	}

	if (!value) {
		ind_enable &= ~BIT(index);
	} else {
		ind_enable |= BIT(index);
	}

	hf->ind_enable = ind_enable;
	return 0;

failed:
	LOG_ERR("Error on AT+BIND response");
	hf_slc_error(hf_at);
	return -EINVAL;
}
#endif /* CONFIG_BT_HFP_HF_HF_INDICATORS */

static const struct unsolicited {
	const char *cmd;
	enum at_cmd_type type;
	int (*func)(struct at_client *hf_at);
} handlers[] = {
	{ "CIEV", AT_CMD_TYPE_UNSOLICITED, ciev_handle },
	{ "RING", AT_CMD_TYPE_OTHER, ring_handle },
#if defined(CONFIG_BT_HFP_HF_CLI)
	{ "CLIP", AT_CMD_TYPE_UNSOLICITED, clip_handle },
#endif /* CONFIG_BT_HFP_HF_CLI */
#if defined(CONFIG_BT_HFP_HF_VOLUME)
	{ "VGM", AT_CMD_TYPE_UNSOLICITED, vgm_handle },
	{ "VGS", AT_CMD_TYPE_UNSOLICITED, vgs_handle },
#endif /* CONFIG_BT_HFP_HF_VOLUME */
	{ "BSIR", AT_CMD_TYPE_UNSOLICITED, bsir_handle },
#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
	{ "BCS", AT_CMD_TYPE_UNSOLICITED, bcs_handle },
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */
	{ "BTRH", AT_CMD_TYPE_UNSOLICITED, btrh_handle },
#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
	{ "CCWA", AT_CMD_TYPE_UNSOLICITED, ccwa_handle },
	{ "CHLD", AT_CMD_TYPE_UNSOLICITED, chld_handle },
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */
#if defined(CONFIG_BT_HFP_HF_ECS)
	{ "CLCC", AT_CMD_TYPE_UNSOLICITED, clcc_handle },
#endif /* CONFIG_BT_HFP_HF_ECS */
#if defined(CONFIG_BT_HFP_HF_VOICE_RECG)
	{ "BVRA", AT_CMD_TYPE_UNSOLICITED, bvra_handle },
#endif /* CONFIG_BT_HFP_HF_VOICE_RECG */
	{ "CNUM", AT_CMD_TYPE_UNSOLICITED, cnum_handle },
#if defined(CONFIG_BT_HFP_HF_HF_INDICATORS)
	{ "BIND", AT_CMD_TYPE_UNSOLICITED, bind_handle },
#endif /* CONFIG_BT_HFP_HF_HF_INDICATORS */
};

static const struct unsolicited *hfp_hf_unsol_lookup(struct at_client *hf_at)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(handlers); i++) {
		if (!strncmp(hf_at->buf, handlers[i].cmd,
			     strlen(handlers[i].cmd))) {
			return &handlers[i];
		}
	}

	return NULL;
}

int unsolicited_cb(struct at_client *hf_at, struct net_buf *buf)
{
	const struct unsolicited *handler;

	handler = hfp_hf_unsol_lookup(hf_at);
	if (!handler) {
		LOG_ERR("Unhandled unsolicited response");
		return -ENOMSG;
	}

	if (!at_parse_cmd_input(hf_at, buf, handler->cmd, handler->func,
				handler->type)) {
		return 0;
	}

	return -ENOMSG;
}

static int send_at_cmee(struct bt_hfp_hf *hf, at_finish_cb_t cb)
{
	if (hf->ag_features & BT_HFP_AG_FEATURE_EXT_ERR) {
		return hfp_hf_send_cmd(hf, NULL, cb, "AT+CMEE=1");
	} else {
		return -ENOTSUP;
	}
}

static int at_cmee_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("CMEE set (result %d) on %p", result, hf);

	return 0;
}

static int send_at_cops(struct bt_hfp_hf *hf, at_finish_cb_t cb)
{
	return hfp_hf_send_cmd(hf, NULL, cb, "AT+COPS=3,0");
}

static int at_cops_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("COPS set (result %d) on %p", result, hf);

	return 0;
}

#if defined(CONFIG_BT_HFP_HF_CLI)
static int send_at_clip(struct bt_hfp_hf *hf, at_finish_cb_t cb)
{
	return hfp_hf_send_cmd(hf, NULL, cb, "AT+CLIP=1");
}

static int at_clip_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("CLIP set (result %d) on %p", result, hf);

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_CLI */

#if defined(CONFIG_BT_HFP_HF_VOLUME)
static int send_at_vgm(struct bt_hfp_hf *hf, at_finish_cb_t cb)
{
	return hfp_hf_send_cmd(hf, NULL, cb, "AT+VGM=%d", hf->vgm);
}

static int at_vgm_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("VGM set (result %d) on %p", result, hf);

	return 0;
}

static int send_at_vgs(struct bt_hfp_hf *hf, at_finish_cb_t cb)
{
	return hfp_hf_send_cmd(hf, NULL, cb, "AT+VGS=%d", hf->vgs);
}

static int at_vgs_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("VGS set (result %d) on %p", result, hf);

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_VOLUME */

#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
static int send_at_ccwa(struct bt_hfp_hf *hf, at_finish_cb_t cb)
{
	if (!(hf->ag_features & BT_HFP_AG_FEATURE_3WAY_CALL)) {
		return -ENOTSUP;
	}

	if (!(hf->hf_features & BT_HFP_HF_FEATURE_3WAY_CALL)) {
		return -ENOTSUP;
	}

	return hfp_hf_send_cmd(hf, NULL, cb, "AT+CCWA=1");
}

static int at_ccwa_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("CCWA set (result %d) on %p", result, hf);

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */

typedef int (*at_send_t)(struct bt_hfp_hf *hf, at_finish_cb_t cb);

static struct at_cmd_init
{
	at_send_t send;
	at_finish_cb_t finish;
	bool disconnect; /* Disconnect if command failed. */
} cmd_init_list[] = {
#if defined(CONFIG_BT_HFP_HF_VOLUME)
	{send_at_vgm, at_vgm_finish, false},
	{send_at_vgs, at_vgs_finish, false},
#endif /* CONFIG_BT_HFP_HF_VOLUME */
	{send_at_cmee, at_cmee_finish, false},
	{send_at_cops, at_cops_finish, false},
#if defined(CONFIG_BT_HFP_HF_CLI)
	{send_at_clip, at_clip_finish, false},
#endif /* CONFIG_BT_HFP_HF_CLI */
#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
	{send_at_ccwa, at_ccwa_finish, false},
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */
};

static int at_cmd_init_start(struct bt_hfp_hf *hf);

static int at_cmd_init_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	at_finish_cb_t finish;

	if (result != AT_RESULT_OK) {
		LOG_WRN("It is ERROR response of AT command %d.", hf->cmd_init_seq);
	}

	if (ARRAY_SIZE(cmd_init_list) > hf->cmd_init_seq) {
		finish = cmd_init_list[hf->cmd_init_seq].finish;
		if (finish) {
			(void)finish(hf_at, result, cme_err);
		}
	} else {
		LOG_ERR("Invalid indicator (%d>=%d)", hf->cmd_init_seq,
		    ARRAY_SIZE(cmd_init_list));
	}

	/* Goto next AT command */
	hf->cmd_init_seq++;
	(void)at_cmd_init_start(hf);
	return 0;
}

static int at_cmd_init_start(struct bt_hfp_hf *hf)
{
	at_send_t send;
	int err = -EINVAL;

	while (ARRAY_SIZE(cmd_init_list) > hf->cmd_init_seq) {
		LOG_DBG("Fetch AT command (%d)", hf->cmd_init_seq);
		send = cmd_init_list[hf->cmd_init_seq].send;
		if (send) {
			LOG_DBG("Send AT command");
			err = send(hf, at_cmd_init_finish);
		} else {
			LOG_WRN("Invalid send func of AT command");
		}

		if (!err) {
			break;
		}

		LOG_WRN("AT command sending failed");
		if (cmd_init_list[hf->cmd_init_seq].disconnect) {
			hfp_hf_send_failed(hf);
			break;
		}
		/* Goto next AT command */
		LOG_WRN("Send next AT command");
		hf->cmd_init_seq++;
	}
	return err;
}

static void slc_completed(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	struct bt_conn *conn = hf->acl;

	if (bt_hf->connected) {
		bt_hf->connected(conn, hf);
	}

	atomic_set_bit(hf->flags, BT_HFP_HF_FLAG_CONNECTED);

	/* Start with first AT command */
	hf->cmd_init_seq = 0;
	if (at_cmd_init_start(hf)) {
		LOG_ERR("Fail to start AT command initialization");
	}
}

#if defined(CONFIG_BT_HFP_HF_HF_INDICATORS)
static int send_at_bind_status(struct bt_hfp_hf *hf, at_finish_cb_t cb)
{
	return hfp_hf_send_cmd(hf, NULL, cb, "AT+BIND?");
}

static int send_at_bind_hf_supported(struct bt_hfp_hf *hf, at_finish_cb_t cb)
{
	char buffer[4];
	char *bind;

	hf->hf_ind = 0;

	bind = &buffer[0];
	if (IS_ENABLED(CONFIG_BT_HFP_HF_HF_INDICATOR_ENH_SAFETY)) {
		*bind = '0' + HFP_HF_ENHANCED_SAFETY_IND;
		bind++;
		*bind = ',';
		bind++;
		hf->hf_ind |= BIT(HFP_HF_ENHANCED_SAFETY_IND);
	}

	if (IS_ENABLED(CONFIG_BT_HFP_HF_HF_INDICATOR_BATTERY)) {
		*bind = '0' + HFP_HF_BATTERY_LEVEL_IND;
		bind++;
		*bind = ',';
		bind++;
		hf->hf_ind |= BIT(HFP_HF_BATTERY_LEVEL_IND);
	}

	if (bind <= &buffer[0]) {
		return -EINVAL;
	}

	bind--;
	*bind = '\0';
	return hfp_hf_send_cmd(hf, NULL, cb, "AT+BIND=%s", buffer);
}

static int send_at_bind_supported(struct bt_hfp_hf *hf, at_finish_cb_t cb)
{
	return hfp_hf_send_cmd(hf, NULL, cb, "AT+BIND=?");
}
#endif /* CONFIG_BT_HFP_HF_HF_INDICATORS */

#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
static int send_at_chld_supported(struct bt_hfp_hf *hf, at_finish_cb_t cb)
{
	return hfp_hf_send_cmd(hf, NULL, cb, "AT+CHLD=?");
}
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */

static int send_at_cmer(struct bt_hfp_hf *hf, at_finish_cb_t cb)
{
	at_register_unsolicited(&hf->at, unsolicited_cb);
	return hfp_hf_send_cmd(hf, NULL, cb, "AT+CMER=3,0,0,1");
}

static int send_at_cind_status(struct bt_hfp_hf *hf, at_finish_cb_t cb)
{
	return hfp_hf_send_cmd(hf, cind_status_resp, cb, "AT+CIND?");
}

static int send_at_cind_supported(struct bt_hfp_hf *hf, at_finish_cb_t cb)
{
	return hfp_hf_send_cmd(hf, cind_resp, cb, "AT+CIND=?");
}

#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
static void get_codec_ids(struct bt_hfp_hf *hf, char *buffer, size_t buffer_len)
{
	size_t len = 0;
	uint8_t ids = hf->hf_codec_ids;
	int index = 0;

	while (ids && (len < (buffer_len-2))) {
		if (ids & 0x01) {
			buffer[len++] = index + '0';
			buffer[len++] = ',';
		}
		index++;
		ids = ids >> 1;
	}

	if (len > 0) {
		len--;
	}

	buffer[len] = '\0';
}

static int send_at_bac(struct bt_hfp_hf *hf, at_finish_cb_t cb)
{
	char ids[sizeof(hf->hf_codec_ids)*2*8 + 1];

	get_codec_ids(hf, &ids[0], ARRAY_SIZE(ids));
	return hfp_hf_send_cmd(hf, NULL, cb, "AT+BAC=%s", ids);
}
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */

static int send_at_brsf(struct bt_hfp_hf *hf, at_finish_cb_t cb)
{
	return hfp_hf_send_cmd(hf, brsf_resp, cb, "AT+BRSF=%u", hf->hf_features);
}

static struct slc_init
{
	at_send_t send;
	bool disconnect; /* Disconnect if command failed. */
	uint32_t ag_feature_mask; /* AG feature mask */
} slc_init_list[] = {
	{send_at_brsf, true, 0},
#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
	{send_at_bac, true, BT_HFP_AG_FEATURE_CODEC_NEG},
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */
	{send_at_cind_supported, true, 0},
	{send_at_cind_status, true, 0},
	{send_at_cmer, true, 0},
#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
	{send_at_chld_supported, true, BT_HFP_AG_FEATURE_3WAY_CALL},
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */
#if defined(CONFIG_BT_HFP_HF_HF_INDICATORS)
	{send_at_bind_hf_supported, true, BT_HFP_AG_FEATURE_HF_IND},
	{send_at_bind_supported, true, BT_HFP_AG_FEATURE_HF_IND},
	{send_at_bind_status, true, BT_HFP_AG_FEATURE_HF_IND},
#endif /* CONFIG_BT_HFP_HF_HF_INDICATORS */
};

static int slc_init_start(struct bt_hfp_hf *hf);

static int slc_init_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	if (result != AT_RESULT_OK) {
		LOG_WRN("It is ERROR response of AT command %d.", hf->cmd_init_seq);
		if (slc_init_list[hf->cmd_init_seq].disconnect) {
			hf_slc_error(&hf->at);
			return 0;
		}
	}

	if (ARRAY_SIZE(slc_init_list) <= hf->cmd_init_seq) {
		LOG_ERR("Invalid indicator (%d>=%d)", hf->cmd_init_seq,
		    ARRAY_SIZE(slc_init_list));
	}

	/* Goto next AT command */
	hf->cmd_init_seq++;
	(void)slc_init_start(hf);
	return 0;
}

static int slc_init_start(struct bt_hfp_hf *hf)
{
	at_send_t send;
	uint32_t feture_mask;
	int err = -EINVAL;

	while (ARRAY_SIZE(slc_init_list) > hf->cmd_init_seq) {
		LOG_DBG("Fetch AT command (%d)", hf->cmd_init_seq);
		feture_mask = slc_init_list[hf->cmd_init_seq].ag_feature_mask;
		if (feture_mask && (!(feture_mask & hf->ag_features))) {
			/* The feature is not supported by AG. Skip the step. */
			LOG_INF("Skip SLC init step %d", hf->cmd_init_seq);
			hf->cmd_init_seq++;
			continue;
		}

		send = slc_init_list[hf->cmd_init_seq].send;
		if (send) {
			LOG_DBG("Send AT command");
			err = send(hf, slc_init_finish);
		} else {
			LOG_WRN("Invalid send func of AT command");
		}

		if (!err) {
			break;
		}

		LOG_WRN("AT command sending failed");
		if (slc_init_list[hf->cmd_init_seq].disconnect) {
			hfp_hf_send_failed(hf);
			break;
		}
		/* Goto next AT command */
		LOG_WRN("Send next AT command");
		hf->cmd_init_seq++;
	}

	if (ARRAY_SIZE(slc_init_list) <= hf->cmd_init_seq) {
		slc_completed(&hf->at);
	}

	return err;
}

int hf_slc_establish(struct bt_hfp_hf *hf)
{
	int err;

	LOG_DBG("");

	hf->cmd_init_seq = 0;
	err = slc_init_start(hf);
	if (err < 0) {
		hf_slc_error(&hf->at);
		return err;
	}

	return 0;
}

#if defined(CONFIG_BT_HFP_HF_CLI)
static int cli_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("CLIP set (result %d) on %p", result, hf);

	/* AT+CLI is done. */
	return 0;
}
#endif /* CONFIG_BT_HFP_HF_CLI */

int bt_hfp_hf_cli(struct bt_hfp_hf *hf, bool enable)
{
#if defined(CONFIG_BT_HFP_HF_CLI)
	int err;

	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!atomic_test_bit(hf->flags, BT_HFP_HF_FLAG_CONNECTED)) {
		LOG_ERR("SLC is not established on %p", hf);
		return -EINVAL;
	}

	err = hfp_hf_send_cmd(hf, NULL, cli_finish, "AT+CLIP=%d", enable ? 1 : 0);
	if (err < 0) {
		LOG_ERR("HFP HF CLI set failed on %p", hf);
	}

	return err;
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_CLI */
}

#if defined(CONFIG_BT_HFP_HF_VOLUME)
static int vgm_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("VGM set (result %d) on %p", result, hf);

	/* AT+VGM is done. */
	return 0;
}
#endif /* CONFIG_BT_HFP_HF_VOLUME */

int bt_hfp_hf_vgm(struct bt_hfp_hf *hf, uint8_t gain)
{
#if defined(CONFIG_BT_HFP_HF_VOLUME)
	int err;

	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (gain > BT_HFP_HF_VGM_GAIN_MAX) {
		LOG_ERR("Invalid gain %d>%d", gain, BT_HFP_HF_VGM_GAIN_MAX);
		return -EINVAL;
	}

	hf->vgm = gain;

	if (!atomic_test_bit(hf->flags, BT_HFP_HF_FLAG_CONNECTED)) {
		return 0;
	}

	err = hfp_hf_send_cmd(hf, NULL, vgm_finish, "AT+VGM=%d", gain);
	if (err < 0) {
		LOG_ERR("HFP HF VGM set failed on %p", hf);
	}

	return err;
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_VOLUME */
}

#if defined(CONFIG_BT_HFP_HF_VOLUME)
static int vgs_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("VGS set (result %d) on %p", result, hf);

	/* AT+VGS is done. */
	return 0;
}
#endif /* CONFIG_BT_HFP_HF_VOLUME */

int bt_hfp_hf_vgs(struct bt_hfp_hf *hf, uint8_t gain)
{
#if defined(CONFIG_BT_HFP_HF_VOLUME)
	int err;

	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (gain > BT_HFP_HF_VGM_GAIN_MAX) {
		LOG_ERR("Invalid gain %d>%d", gain, BT_HFP_HF_VGM_GAIN_MAX);
		return -EINVAL;
	}

	hf->vgs = gain;

	if (!atomic_test_bit(hf->flags, BT_HFP_HF_FLAG_CONNECTED)) {
		return 0;
	}

	err = hfp_hf_send_cmd(hf, NULL, vgs_finish, "AT+VGS=%d", gain);
	if (err < 0) {
		LOG_ERR("HFP HF VGS set failed on %p", hf);
	}

	return err;
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_VOLUME */
}

static int cops_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	uint32_t mode;
	uint32_t format;
	char *operator;
	int err;

	err = at_get_number(hf_at, &mode);
	if (err < 0) {
		LOG_ERR("Error getting value");
		return err;
	}

	err = at_get_number(hf_at, &format);
	if (err < 0) {
		LOG_ERR("Error getting value");
		return err;
	}

	operator = at_get_string(hf_at);

	if (bt_hf && bt_hf->operator) {
		bt_hf->operator(hf, (uint8_t)mode, (uint8_t)format, operator);
	}

	return 0;
}

static int cops_resp(struct at_client *hf_at, struct net_buf *buf)
{
	int err;

	LOG_DBG("");

	err = at_parse_cmd_input(hf_at, buf, "COPS", cops_handle,
				 AT_CMD_TYPE_NORMAL);
	if (err < 0) {
		LOG_ERR("Cannot parse response of AT+COPS?");
		return err;
	}

	return 0;
}

static int cops_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+COPS? (result %d) on %p", result, hf);

	return 0;
}

int bt_hfp_hf_get_operator(struct bt_hfp_hf *hf)
{
	int err;

	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!atomic_test_bit(hf->flags, BT_HFP_HF_FLAG_CONNECTED)) {
		return 0;
	}

	err = hfp_hf_send_cmd(hf, cops_resp, cops_finish, "AT+COPS?");
	if (err < 0) {
		LOG_ERR("Fail to read the currently selected operator on %p", hf);
	}

	return err;
}

static int binp_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	char *number;

	number = at_get_string(hf_at);

	atomic_set_bit(hf->flags, BT_HFP_HF_FLAG_BINP);

	if (bt_hf && bt_hf->request_phone_number) {
		bt_hf->request_phone_number(hf, number);
	}

	return 0;
}

static int binp_resp(struct at_client *hf_at, struct net_buf *buf)
{
	int err;

	LOG_DBG("");

	err = at_parse_cmd_input(hf_at, buf, "BINP", binp_handle,
				 AT_CMD_TYPE_NORMAL);
	if (err < 0) {
		LOG_ERR("Cannot parse response of AT+BINP=1");
		return err;
	}

	return 0;
}

static int binp_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+BINP=1 (result %d) on %p", result, hf);

	if (!atomic_test_and_clear_bit(hf->flags, BT_HFP_HF_FLAG_BINP)) {
		if (bt_hf && bt_hf->request_phone_number) {
			bt_hf->request_phone_number(hf, NULL);
		}
	}

	return 0;
}

int bt_hfp_hf_request_phone_number(struct bt_hfp_hf *hf)
{
	int err;

	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!atomic_test_bit(hf->flags, BT_HFP_HF_FLAG_CONNECTED)) {
		return 0;
	}

	atomic_clear_bit(hf->flags, BT_HFP_HF_FLAG_BINP);

	err = hfp_hf_send_cmd(hf, binp_resp, binp_finish, "AT+BINP=1");
	if (err < 0) {
		LOG_ERR("Fail to request phone number to the AG on %p", hf);
	}

	return err;
}

static int vts_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+VTS (result %d) on %p", result, hf);

	return 0;
}

int bt_hfp_hf_transmit_dtmf_code(struct bt_hfp_hf_call *call, char code)
{
	struct bt_hfp_hf *hf;
	int err;

	LOG_DBG("");

	if (!call) {
		LOG_ERR("Invalid call");
		return -ENOTCONN;
	}

	hf = call->hf;
	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!atomic_test_bit(hf->flags, BT_HFP_HF_FLAG_CONNECTED)) {
		LOG_ERR("SLC is not established on %p", hf);
		return -ENOTCONN;
	}

	if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_IN_USING) ||
		(!(!atomic_test_bit(call->flags, BT_HFP_HF_CALL_INCOMING_HELD) &&
		(atomic_get(call->state) == BT_HFP_HF_CALL_STATE_ACTIVE)))) {
		LOG_ERR("Invalid call status");
		return -EINVAL;
	}

	if (!IS_VALID_DTMF(code)) {
		LOG_ERR("Invalid code");
		return -EINVAL;
	}

	err = hfp_hf_send_cmd(hf, NULL, vts_finish, "AT+VTS=%c", code);
	if (err < 0) {
		LOG_ERR("Fail to tramsit DTMF Codes on %p", hf);
	}

	return err;
}

static int cnum_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+CNUM (result %d) on %p", result, hf);

	return 0;
}

int bt_hfp_hf_query_subscriber(struct bt_hfp_hf *hf)
{
	int err;

	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!atomic_test_bit(hf->flags, BT_HFP_HF_FLAG_CONNECTED)) {
		LOG_ERR("SLC is not established on %p", hf);
		return -ENOTCONN;
	}

	err = hfp_hf_send_cmd(hf, NULL, cnum_finish, "AT+CNUM");
	if (err < 0) {
		LOG_ERR("Fail to query subscriber number information on %p", hf);
	}

	return err;
}

static int bia_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+BIA (result %d) on %p", result, hf);

	return 0;
}

int bt_hfp_hf_indicator_status(struct bt_hfp_hf *hf, uint8_t status)
{
	int err;
	size_t index;
	char buffer[HF_MAX_AG_INDICATORS * 2 + 1];
	char *bia_status;

	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!atomic_test_bit(hf->flags, BT_HFP_HF_FLAG_CONNECTED)) {
		LOG_ERR("SLC is not established on %p", hf);
		return -ENOTCONN;
	}

	bia_status = &buffer[0];
	for (index = 0; index < ARRAY_SIZE(hf->ind_table); index++) {
		if ((hf->ind_table[index] != -1) && (index < NUM_BITS(sizeof(status)))) {
			if (status & BIT(hf->ind_table[index])) {
				*bia_status = '1';
			} else {
				*bia_status = '0';
			}
			bia_status++;
			*bia_status = ',';
			bia_status++;
		} else {
			break;
		}
	}

	if (bia_status <= &buffer[0]) {
		LOG_ERR("Not found valid AG indicator on %p", hf);
		return -EINVAL;
	}

	bia_status--;
	*bia_status = '\0';

	err = hfp_hf_send_cmd(hf, NULL, bia_finish, "AT+BIA=%s", buffer);
	if (err < 0) {
		LOG_ERR("Fail to activated/deactivated AG indicators on %p", hf);
	}

	return err;
}

#if defined(CONFIG_BT_HFP_HF_HF_INDICATOR_ENH_SAFETY)
static int biev_enh_safety_finish(struct at_client *hf_at,
		   enum at_result result, enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+BIEV (result %d) on %p", result, hf);

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_HF_INDICATOR_ENH_SAFETY */

int bt_hfp_hf_enhanced_safety(struct bt_hfp_hf *hf, bool enable)
{
#if defined(CONFIG_BT_HFP_HF_HF_INDICATOR_ENH_SAFETY)
	int err;

	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!atomic_test_bit(hf->flags, BT_HFP_HF_FLAG_CONNECTED)) {
		LOG_ERR("SLC is not established on %p", hf);
		return -ENOTCONN;
	}

	if (!((hf->hf_ind & BIT(HFP_HF_ENHANCED_SAFETY_IND)) &&
		(hf->ag_ind & BIT(HFP_HF_ENHANCED_SAFETY_IND)))) {
		LOG_ERR("The indicator is unsupported");
		return -ENOTSUP;
	}

	if (!(hf->ind_enable & BIT(HFP_HF_ENHANCED_SAFETY_IND))) {
		LOG_ERR("The indicator is disabled");
		return -EINVAL;
	}

	err = hfp_hf_send_cmd(hf, NULL, biev_enh_safety_finish, "AT+BIEV=%d,%d",
		HFP_HF_ENHANCED_SAFETY_IND, enable ? 1 : 0);
	if (err < 0) {
		LOG_ERR("Fail to transfer enhanced safety value on %p", hf);
	}

	return err;
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_HF_INDICATOR_ENH_SAFETY */
}

#if defined(CONFIG_BT_HFP_HF_HF_INDICATOR_BATTERY)
static int biev_battery_finish(struct at_client *hf_at,
		   enum at_result result, enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+BIEV (result %d) on %p", result, hf);

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_HF_INDICATOR_BATTERY */

int bt_hfp_hf_battery(struct bt_hfp_hf *hf, uint8_t level)
{
#if defined(CONFIG_BT_HFP_HF_HF_INDICATOR_BATTERY)
	int err;

	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!atomic_test_bit(hf->flags, BT_HFP_HF_FLAG_CONNECTED)) {
		LOG_ERR("SLC is not established on %p", hf);
		return -ENOTCONN;
	}

	if (!((hf->hf_ind & BIT(HFP_HF_BATTERY_LEVEL_IND)) &&
		(hf->ag_ind & BIT(HFP_HF_BATTERY_LEVEL_IND)))) {
		LOG_ERR("The indicator is unsupported");
		return -ENOTSUP;
	}

	if (!(hf->ind_enable & BIT(HFP_HF_BATTERY_LEVEL_IND))) {
		LOG_ERR("The indicator is disabled");
		return -EINVAL;
	}

	if (!IS_VALID_BATTERY_LEVEL(level)) {
		LOG_ERR("Invalid battery level %d", level);
		return -EINVAL;
	}

	err = hfp_hf_send_cmd(hf, NULL, biev_battery_finish, "AT+BIEV=%d,%d",
		HFP_HF_BATTERY_LEVEL_IND, level);
	if (err < 0) {
		LOG_ERR("Fail to transfer remaining battery level on %p", hf);
	}

	return err;
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_HF_INDICATOR_BATTERY */
}

static int ata_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("ATA (result %d) on %p", result, hf);

	return 0;
}

static int btrh_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+BTRH (result %d) on %p", result, hf);

	return 0;
}

int bt_hfp_hf_accept(struct bt_hfp_hf_call *call)
{
	int err;
	struct bt_hfp_hf *hf;
	int count;

	LOG_DBG("");

	if (!call) {
		LOG_ERR("Invalid call");
		return -EINVAL;
	}

	hf = call->hf;
	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_IN_USING)) {
		LOG_ERR("No valid call");
		return -EINVAL;
	}

	count = get_using_call_count(call->hf);
	if (count > 1) {
		LOG_ERR("Unsupported 3Way call");
		return -EINVAL;
	}

	if (atomic_get(call->state) == BT_HFP_HF_CALL_STATE_WAITING) {
		err = hfp_hf_send_cmd(hf, NULL, ata_finish, "ATA");
		if (err < 0) {
			LOG_ERR("Fail to accept the incoming call on %p", hf);
		}
		return err;
	}

	if (atomic_test_bit(call->flags, BT_HFP_HF_CALL_INCOMING_HELD) &&
		(atomic_get(call->state) == BT_HFP_HF_CALL_STATE_ACTIVE)) {
		err = hfp_hf_send_cmd(hf, NULL, btrh_finish, "AT+BTRH=%d",
			BT_HFP_BTRH_ACCEPTED);
		if (err < 0) {
			LOG_ERR("Fail to accept the held incoming call on %p", hf);
		}
		return err;
	}

	return -EINVAL;
}

static int chup_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+CHUP (result %d) on %p", result, hf);

	return 0;
}

int bt_hfp_hf_reject(struct bt_hfp_hf_call *call)
{
	int err;
	struct bt_hfp_hf *hf;
	int count;

	LOG_DBG("");

	if (!call) {
		LOG_ERR("Invalid call");
		return -EINVAL;
	}

	hf = call->hf;
	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_IN_USING)) {
		LOG_ERR("No valid call");
		return -EINVAL;
	}

	count = get_using_call_count(call->hf);
	if (count > 1) {
		LOG_ERR("Unsupported 3Way call");
		return -EINVAL;
	}

	if (!(hf->ag_features & BT_HFP_AG_FEATURE_REJECT_CALL)) {
		LOG_ERR("AG has not ability to reject call");
		return -ENOTSUP;
	}

	if (atomic_get(call->state) == BT_HFP_HF_CALL_STATE_WAITING) {
		err = hfp_hf_send_cmd(hf, NULL, chup_finish, "AT+CHUP");
		if (err < 0) {
			LOG_ERR("Fail to reject the incoming call on %p", hf);
		}
		return err;
	}

	if (atomic_test_bit(call->flags, BT_HFP_HF_CALL_INCOMING_HELD) &&
		(atomic_get(call->state) == BT_HFP_HF_CALL_STATE_ACTIVE)) {
		err = hfp_hf_send_cmd(hf, NULL, btrh_finish, "AT+BTRH=%d",
			BT_HFP_BTRH_REJECTED);
		if (err < 0) {
			LOG_ERR("Fail to reject the held incoming call on %p", hf);
		}
		return err;
	}

	return -EINVAL;
}

int bt_hfp_hf_terminate(struct bt_hfp_hf_call *call)
{
	int err;
	struct bt_hfp_hf *hf;
	int count;

	LOG_DBG("");

	if (!call) {
		LOG_ERR("Invalid call");
		return -EINVAL;
	}

	hf = call->hf;
	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_IN_USING)) {
		LOG_ERR("No valid call");
		return -EINVAL;
	}

	count = get_using_call_count(call->hf);
	if (count > 1) {
		LOG_ERR("Unsupported 3Way call");
		return -EINVAL;
	}

	if (atomic_get(call->state) == BT_HFP_HF_CALL_STATE_HELD) {
		LOG_ERR("Held call cannot be terminated");
		return -EINVAL;
	}

	err = hfp_hf_send_cmd(hf, NULL, chup_finish, "AT+CHUP");
	if (err < 0) {
		LOG_ERR("Fail to terminate the none held call on %p", hf);
	}

	return err;
}

int bt_hfp_hf_hold_incoming(struct bt_hfp_hf_call *call)
{
	int err;
	struct bt_hfp_hf *hf;
	int count;

	LOG_DBG("");

	if (!call) {
		LOG_ERR("Invalid call");
		return -EINVAL;
	}

	hf = call->hf;
	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!atomic_test_bit(call->flags, BT_HFP_HF_CALL_IN_USING)) {
		LOG_ERR("No valid call");
		return -EINVAL;
	}

	count = get_using_call_count(call->hf);
	if (count > 1) {
		LOG_ERR("Unsupported 3Way call");
		return -EINVAL;
	}

	if (!(atomic_get(call->state) == BT_HFP_HF_CALL_STATE_WAITING)) {
		LOG_ERR("No incoming call setup in progress");
		return -EINVAL;
	}

	err = hfp_hf_send_cmd(hf, NULL, btrh_finish, "AT+BTRH=%d",
		BT_HFP_BTRH_ON_HOLD);
	if (err < 0) {
		LOG_ERR("Fail to hold the incoming call on %p", hf);
	}

	return err;
}

static int query_btrh_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	int err;
	struct bt_hfp_hf_call *call;
	uint32_t value;

	err = at_get_number(hf_at, &value);
	if (err < 0) {
		LOG_ERR("Cannot get value");
		return err;
	}

	if (value) {
		LOG_ERR("Only support value 0");
		return 0;
	}

	call = get_call_with_flag(hf, BT_HFP_HF_CALL_INCOMING_HELD);
	if (!call) {
		LOG_ERR("Held incoming call is not found");
		return -EINVAL;
	}

	if (bt_hf->incoming_held) {
		bt_hf->incoming_held(call);
	}

	return 0;
}

static int query_btrh_resp(struct at_client *hf_at, struct net_buf *buf)
{
	int err;

	err = at_parse_cmd_input(hf_at, buf, "BTRH", query_btrh_handle,
				 AT_CMD_TYPE_NORMAL);
	if (err < 0) {
		LOG_ERR("Error parsing CMD input");
		hf_slc_error(hf_at);
	}

	return 0;
}

static int query_btrh_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+BTRH? (result %d) on %p", result, hf);

	return 0;
}

int bt_hfp_hf_query_respond_hold_status(struct bt_hfp_hf *hf)
{
	int err;

	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	err = hfp_hf_send_cmd(hf, query_btrh_resp, query_btrh_finish, "AT+BTRH?");
	if (err < 0) {
		LOG_ERR("Fail to query respond and hold status of AG on %p", hf);
	}

	return err;
}

static int bt_hfp_ag_get_cme_err(enum at_cme cme_err)
{
	int err;

	switch (cme_err) {
	case CME_ERROR_OPERATION_NOT_SUPPORTED:
		err = -EOPNOTSUPP;
		break;
	case CME_ERROR_AG_FAILURE:
		err = -EFAULT;
		break;
	case CME_ERROR_MEMORY_FAILURE:
		err = -ENOSR;
		break;
	case CME_ERROR_MEMORY_FULL:
		err = -ENOMEM;
		break;
	case CME_ERROR_DIAL_STRING_TO_LONG:
		err = -ENAMETOOLONG;
		break;
	case CME_ERROR_INVALID_INDEX:
		err = -EINVAL;
		break;
	case CME_ERROR_OPERATION_NOT_ALLOWED:
		err = -ENOTSUP;
		break;
	case CME_ERROR_NO_CONNECTION_TO_PHONE:
		err = -ENOTCONN;
		break;
	default:
		err = -ENOTSUP;
		break;
	}

	return err;
}

static int atd_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	int err;
	struct bt_hfp_hf_call *call;

	LOG_DBG("ATD (result %d) on %p", result, hf);

	if (result == AT_RESULT_CME_ERROR) {
		err = bt_hfp_ag_get_cme_err(cme_err);
	} else if (result == AT_RESULT_ERROR) {
		err = -ENOTSUP;
	} else {
		err = 0;
	}

	call = get_call_with_state(hf, BT_HFP_HF_CALL_STATE_OUTGOING);
	if (!call) {
		return -EINVAL;
	}

	if (err) {
		free_call(call);
	}

	if (bt_hf && bt_hf->dialing) {
		bt_hf->dialing(hf, err);
	}
	return 0;
}

int bt_hfp_hf_number_call(struct bt_hfp_hf *hf, const char *number)
{
	struct bt_hfp_hf_call *call;
	int err;

	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	call = get_dialing_call(hf);
	if (call) {
		LOG_ERR("There is a call in alerting or waiting");
		return -EBUSY;
	}

	call = get_new_call(hf);
	if (!call) {
		LOG_ERR("Not available call object");
		return -ENOMEM;
	}

	hf_call_state_update(call, BT_HFP_HF_CALL_STATE_OUTGOING);

	err = hfp_hf_send_cmd(hf, NULL, atd_finish, "ATD%s", number);
	if (err < 0) {
		LOG_ERR("Fail to start phone number call on %p", hf);
	}

	return err;
}

int bt_hfp_hf_memory_dial(struct bt_hfp_hf *hf, const char *location)
{
	struct bt_hfp_hf_call *call;
	int err;

	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	call = get_dialing_call(hf);
	if (call) {
		LOG_ERR("There is a call in alerting or waiting");
		return -EBUSY;
	}

	call = get_new_call(hf);
	if (!call) {
		LOG_ERR("Not available call object");
		return -ENOMEM;
	}

	hf_call_state_update(call, BT_HFP_HF_CALL_STATE_OUTGOING);

	err = hfp_hf_send_cmd(hf, NULL, atd_finish, "ATD>%s", location);
	if (err < 0) {
		LOG_ERR("Fail to last number re-Dial on %p", hf);
	}

	return err;
}

static int bldn_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	int err;
	struct bt_hfp_hf_call *call;

	LOG_DBG("AT+BLDN (result %d) on %p", result, hf);

	if (result == AT_RESULT_CME_ERROR) {
		err = bt_hfp_ag_get_cme_err(cme_err);
	} else if (result == AT_RESULT_ERROR) {
		err = -ENOTSUP;
	} else {
		err = 0;
	}

	call = get_call_with_state(hf, BT_HFP_HF_CALL_STATE_OUTGOING);
	if (!call) {
		return -EINVAL;
	}

	if (err) {
		free_call(call);
	}

	if (bt_hf && bt_hf->dialing) {
		bt_hf->dialing(hf, err);
	}
	return 0;
}

int bt_hfp_hf_redial(struct bt_hfp_hf *hf)
{
	struct bt_hfp_hf_call *call;
	int err;

	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	call = get_dialing_call(hf);
	if (call) {
		LOG_ERR("There is a call in alerting or waiting");
		return -EBUSY;
	}

	call = get_new_call(hf);
	if (!call) {
		LOG_ERR("Not available call object");
		return -ENOMEM;
	}

	hf_call_state_update(call, BT_HFP_HF_CALL_STATE_OUTGOING);

	err = hfp_hf_send_cmd(hf, NULL, bldn_finish, "AT+BLDN");
	if (err < 0) {
		LOG_ERR("Fail to start memory dialing on %p", hf);
	}

	return err;
}

#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
static int bcc_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("BCC (result %d) on %p", result, hf);

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */

int bt_hfp_hf_audio_connect(struct bt_hfp_hf *hf)
{
#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
	int err;

	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (hf->chan.sco) {
		LOG_ERR("Audio conenction has been connected");
		return -ECONNREFUSED;
	}

	err = hfp_hf_send_cmd(hf, NULL, bcc_finish, "AT+BCC");
	if (err < 0) {
		LOG_ERR("Fail to setup audio connection on %p", hf);
	}

	return err;
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */
}

#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
static int bcs_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("BCS (result %d) on %p", result, hf);

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */

int bt_hfp_hf_select_codec(struct bt_hfp_hf *hf, uint8_t codec_id)
{
#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!(hf->hf_codec_ids & BIT(codec_id))) {
		LOG_ERR("Codec ID is unsupported");
		return -ENOTSUP;
	}

	if (!atomic_test_and_clear_bit(hf->flags, BT_HFP_HF_FLAG_CODEC_CONN)) {
		LOG_ERR("Invalid context");
		return -ESRCH;
	}

	return hfp_hf_send_cmd(hf, NULL, bcs_finish, "AT+BCS=%d", codec_id);
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */
}

#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
static int bac_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("BAC (result %d) on %p", result, hf);

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */

int bt_hfp_hf_set_codecs(struct bt_hfp_hf *hf, uint8_t codec_ids)
{
#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
	char ids[sizeof(hf->hf_codec_ids)*2*8 + 1];

	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (codec_ids & BT_HFP_HF_CODEC_CVSD) {
		LOG_ERR("CVSD should be supported");
		return -EINVAL;
	}

	atomic_clear_bit(hf->flags, BT_HFP_HF_FLAG_CODEC_CONN);
	hf->hf_codec_ids = codec_ids;

	get_codec_ids(hf, &ids[0], ARRAY_SIZE(ids));
	return hfp_hf_send_cmd(hf, NULL, bac_finish, "AT+BAC=%s", ids);
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */
}

#if defined(CONFIG_BT_HFP_HF_ECNR)
static int nrec_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	int err;

	LOG_DBG("AT+NREC=0 (result %d) on %p", result, hf);

	if (result == AT_RESULT_CME_ERROR) {
		err = bt_hfp_ag_get_cme_err(cme_err);
	} else if (result == AT_RESULT_ERROR) {
		err = -ENOTSUP;
	} else {
		err = 0;
	}

	if (bt_hf && bt_hf->ecnr_turn_off) {
		bt_hf->ecnr_turn_off(hf, err);
	}
	return 0;
}
#endif /* CONFIG_BT_HFP_HF_ECNR */

int bt_hfp_hf_turn_off_ecnr(struct bt_hfp_hf *hf)
{
#if defined(CONFIG_BT_HFP_HF_ECNR)
	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!(hf->ag_features & BT_HFP_AG_FEATURE_ECNR)) {
		LOG_ERR("EC and/or NR functions is unsupported by AG");
		return -ENOTSUP;
	}

	if (hf->chan.sco) {
		LOG_ERR("Audio conenction has been connected");
		return -EBUSY;
	}

	return hfp_hf_send_cmd(hf, NULL, nrec_finish, "AT+NREC=0");
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_ECNR */
}

#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
static int ccwa_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+CCWA (result %d) on %p", result, hf);

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */

int bt_hfp_hf_call_waiting_notify(struct bt_hfp_hf *hf, bool enable)
{
#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!(hf->ag_features & BT_HFP_AG_FEATURE_3WAY_CALL)) {
		LOG_ERR("Three-way calling is unsupported by AG");
		return -ENOTSUP;
	}

	return hfp_hf_send_cmd(hf, NULL, ccwa_finish, "AT+CCWA=%d", enable ? 1 : 0);
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */
}

#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
static int chld_release_all_held_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+CHLD=0 (result %d) on %p", result, hf);

	k_work_reschedule(&hf->deferred_work, K_MSEC(HF_ENHANCED_CALL_STATUS_TIMEOUT));

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */

int bt_hfp_hf_release_all_held(struct bt_hfp_hf *hf)
{
#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!(hf->ag_features & BT_HFP_AG_FEATURE_3WAY_CALL)) {
		LOG_ERR("Three-way calling is unsupported by AG");
		return -ENOTSUP;
	}

	if (!(hf->chld_features & BIT(BT_HFP_CHLD_RELEASE_ALL))) {
		LOG_ERR("Releasing all held calls is unsupported by AG");
		return -ENOTSUP;
	}

	return hfp_hf_send_cmd(hf, NULL, chld_release_all_held_finish, "AT+CHLD=0");
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */
}

#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
static int chld_set_udub_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+CHLD=0 (result %d) on %p", result, hf);

	k_work_reschedule(&hf->deferred_work, K_MSEC(HF_ENHANCED_CALL_STATUS_TIMEOUT));

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */

int bt_hfp_hf_set_udub(struct bt_hfp_hf *hf)
{
#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!(hf->ag_features & BT_HFP_AG_FEATURE_3WAY_CALL)) {
		LOG_ERR("Three-way calling is unsupported by AG");
		return -ENOTSUP;
	}

	if (!(hf->chld_features & BIT(BT_HFP_CHLD_RELEASE_ALL))) {
		LOG_ERR("UDUB is unsupported by AG");
		return -ENOTSUP;
	}

	return hfp_hf_send_cmd(hf, NULL, chld_set_udub_finish, "AT+CHLD=0");
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */
}

#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
static int chld_release_active_accept_other_finish(struct at_client *hf_at,
		   enum at_result result, enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+CHLD=1 (result %d) on %p", result, hf);

	k_work_reschedule(&hf->deferred_work, K_MSEC(HF_ENHANCED_CALL_STATUS_TIMEOUT));

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */

int bt_hfp_hf_release_active_accept_other(struct bt_hfp_hf *hf)
{
#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!(hf->ag_features & BT_HFP_AG_FEATURE_3WAY_CALL)) {
		LOG_ERR("Three-way calling is unsupported by AG");
		return -ENOTSUP;
	}

	return hfp_hf_send_cmd(hf, NULL, chld_release_active_accept_other_finish,
	    "AT+CHLD=1");
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */
}

#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
static int chld_hold_active_accept_other_finish(struct at_client *hf_at,
		   enum at_result result, enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+CHLD=2 (result %d) on %p", result, hf);

	k_work_reschedule(&hf->deferred_work, K_MSEC(HF_ENHANCED_CALL_STATUS_TIMEOUT));

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */

int bt_hfp_hf_hold_active_accept_other(struct bt_hfp_hf *hf)
{
#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!(hf->ag_features & BT_HFP_AG_FEATURE_3WAY_CALL)) {
		LOG_ERR("Three-way calling is unsupported by AG");
		return -ENOTSUP;
	}

	return hfp_hf_send_cmd(hf, NULL, chld_hold_active_accept_other_finish,
	    "AT+CHLD=2");
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */
}

#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
static int chld_join_conversation_finish(struct at_client *hf_at,
		   enum at_result result, enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+CHLD=3 (result %d) on %p", result, hf);

	k_work_reschedule(&hf->deferred_work, K_MSEC(HF_ENHANCED_CALL_STATUS_TIMEOUT));

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */

int bt_hfp_hf_join_conversation(struct bt_hfp_hf *hf)
{
#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!(hf->ag_features & BT_HFP_AG_FEATURE_3WAY_CALL)) {
		LOG_ERR("Three-way calling is unsupported by AG");
		return -ENOTSUP;
	}

	if (!(hf->chld_features & BIT(BT_HFP_CALL_ACTIVE_HELD))) {
		LOG_ERR("Adding a held call to the conversation is unsupported by AG");
		return -ENOTSUP;
	}

	return hfp_hf_send_cmd(hf, NULL, chld_join_conversation_finish,
	    "AT+CHLD=3");
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */
}

#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
static int chld_explicit_call_transfer_finish(struct at_client *hf_at,
		   enum at_result result, enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+CHLD=4 (result %d) on %p", result, hf);

	k_work_reschedule(&hf->deferred_work, K_MSEC(HF_ENHANCED_CALL_STATUS_TIMEOUT));

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */

int bt_hfp_hf_explicit_call_transfer(struct bt_hfp_hf *hf)
{
#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!(hf->ag_features & BT_HFP_AG_FEATURE_3WAY_CALL)) {
		LOG_ERR("Three-way calling is unsupported by AG");
		return -ENOTSUP;
	}

	if (!(hf->chld_features & BIT(BT_HFP_CALL_QUITE))) {
		LOG_ERR("Expliciting Call Transfer is unsupported by AG");
		return -ENOTSUP;
	}

	return hfp_hf_send_cmd(hf, NULL, chld_explicit_call_transfer_finish,
	    "AT+CHLD=4");
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */
}

#if defined(CONFIG_BT_HFP_HF_ECC)
static int chld_release_specified_call_finish(struct at_client *hf_at,
		   enum at_result result, enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+CHLD=1<idx> (result %d) on %p", result, hf);

	k_work_reschedule(&hf->deferred_work, K_MSEC(HF_ENHANCED_CALL_STATUS_TIMEOUT));

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_ECC */

int bt_hfp_hf_release_specified_call(struct bt_hfp_hf_call *call)
{
#if defined(CONFIG_BT_HFP_HF_ECC)
	struct bt_hfp_hf *hf;

	LOG_DBG("");

	if (!call) {
		LOG_ERR("Invalid call");
		return -ENOTCONN;
	}

	hf = call->hf;
	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!(hf->ag_features & BT_HFP_AG_FEATURE_3WAY_CALL)) {
		LOG_ERR("Three-way calling is unsupported by AG");
		return -ENOTSUP;
	}

	if (!(hf->ag_features & BT_HFP_AG_FEATURE_ECC)) {
		LOG_ERR("Enhanced Call Control is unsupported by AG");
		return -ENOTSUP;
	}

	if (!(hf->hf_features & BT_HFP_HF_FEATURE_ECC)) {
		LOG_ERR("Enhanced Call Control is unsupported by HF");
		return -ENOTSUP;
	}

	if (!(hf->chld_features & BIT(BT_HFP_CALL_RELEASE_SPECIFIED_ACTIVE))) {
		LOG_ERR("Releasing a specific active call is unsupported by AG");
		return -ENOTSUP;
	}

	if (!call->index) {
		LOG_ERR("Invalid call index");
		return -EINVAL;
	}

	return hfp_hf_send_cmd(hf, NULL, chld_release_specified_call_finish,
	    "AT+CHLD=1%d", call->index);
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_ECC */
}

#if defined(CONFIG_BT_HFP_HF_ECC)
static int chld_private_consultation_mode_finish(struct at_client *hf_at,
		   enum at_result result, enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+CHLD=2<idx> (result %d) on %p", result, hf);

	k_work_reschedule(&hf->deferred_work, K_MSEC(HF_ENHANCED_CALL_STATUS_TIMEOUT));

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_ECC */

int bt_hfp_hf_private_consultation_mode(struct bt_hfp_hf_call *call)
{
#if defined(CONFIG_BT_HFP_HF_ECC)
	struct bt_hfp_hf *hf;

	LOG_DBG("");

	if (!call) {
		LOG_ERR("Invalid call");
		return -ENOTCONN;
	}

	hf = call->hf;
	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!(hf->ag_features & BT_HFP_AG_FEATURE_3WAY_CALL)) {
		LOG_ERR("Three-way calling is unsupported by AG");
		return -ENOTSUP;
	}

	if (!(hf->ag_features & BT_HFP_AG_FEATURE_ECC)) {
		LOG_ERR("Enhanced Call Control is unsupported by AG");
		return -ENOTSUP;
	}

	if (!(hf->hf_features & BT_HFP_HF_FEATURE_ECC)) {
		LOG_ERR("Enhanced Call Control is unsupported by HF");
		return -ENOTSUP;
	}

	if (!(hf->chld_features & BIT(BT_HFP_CALL_PRIVATE_CNLTN_MODE))) {
		LOG_ERR("Private Consultation Mode is unsupported by AG");
		return -ENOTSUP;
	}

	if (!call->index) {
		LOG_ERR("Invalid call index");
		return -EINVAL;
	}

	return hfp_hf_send_cmd(hf, NULL, chld_private_consultation_mode_finish,
			       "AT+CHLD=2%d", call->index);
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_ECC */
}

#if defined(CONFIG_BT_HFP_HF_VOICE_RECG)
static int bvra_1_finish(struct at_client *hf_at,
		   enum at_result result, enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+BVRA=1 (result %d) on %p", result, hf);

	if (result == AT_RESULT_OK) {
		if (!atomic_test_and_set_bit(hf->flags, BT_HFP_HF_FLAG_VRE_ACTIVATE)) {
			if (bt_hf->voice_recognition) {
				bt_hf->voice_recognition(hf, true);
			}
		}
	}

	return 0;
}

static int bvra_0_finish(struct at_client *hf_at,
		   enum at_result result, enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+BVRA=0 (result %d) on %p", result, hf);

	if (atomic_test_and_clear_bit(hf->flags, BT_HFP_HF_FLAG_VRE_ACTIVATE)) {
		if (bt_hf->voice_recognition) {
			bt_hf->voice_recognition(hf, false);
		}
	}

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_VOICE_RECG */

int bt_hfp_hf_voice_recognition(struct bt_hfp_hf *hf, bool activate)
{
#if defined(CONFIG_BT_HFP_HF_VOICE_RECG)
	at_finish_cb_t finish;

	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!(hf->ag_features & BT_HFP_AG_FEATURE_VOICE_RECG)) {
		LOG_ERR("Voice recognition is unsupported by AG");
		return -ENOTSUP;
	}

	if (activate) {
		finish = bvra_1_finish;
	} else {
		finish = bvra_0_finish;
	}

	return hfp_hf_send_cmd(hf, NULL, finish, "AT+BVRA=%d",
		activate ? 1 : 0);
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_VOICE_RECG */
}

#if defined(CONFIG_BT_HFP_HF_ENH_VOICE_RECG)
static int bvra_2_finish(struct at_client *hf_at,
		   enum at_result result, enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("AT+BVRA=2 (result %d) on %p", result, hf);

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_ENH_VOICE_RECG */

int bt_hfp_hf_ready_to_accept_audio(struct bt_hfp_hf *hf)
{
#if defined(CONFIG_BT_HFP_HF_ENH_VOICE_RECG)
	LOG_DBG("");

	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!(hf->ag_features & BT_HFP_AG_FEATURE_VOICE_RECG)) {
		LOG_ERR("Voice recognition is unsupported by AG");
		return -ENOTSUP;
	}

	if (!atomic_test_bit(hf->flags, BT_HFP_HF_FLAG_VRE_ACTIVATE)) {
		LOG_ERR("Voice recognition is not activated");
		return -EINVAL;
	}

	if (!hf->chan.sco) {
		LOG_ERR("SCO channel is not ready");
		return -ENOTCONN;
	}

	return hfp_hf_send_cmd(hf, NULL, bvra_2_finish, "AT+BVRA=2");
#else
	return -ENOTSUP;
#endif /* CONFIG_BT_HFP_HF_ENH_VOICE_RECG */
}

static void hfp_hf_connected(struct bt_rfcomm_dlc *dlc)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(dlc, struct bt_hfp_hf, rfcomm_dlc);

	LOG_DBG("hf connected");

	BT_ASSERT(hf);
	hf_slc_establish(hf);
}

static void hfp_hf_disconnected(struct bt_rfcomm_dlc *dlc)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(dlc, struct bt_hfp_hf, rfcomm_dlc);

	LOG_DBG("hf disconnected!");
	if (bt_hf->disconnected) {
		bt_hf->disconnected(hf);
	}

	k_work_cancel(&hf->work);
	k_work_cancel_delayable(&hf->deferred_work);
	hf->acl = NULL;
}

static void hfp_hf_recv(struct bt_rfcomm_dlc *dlc, struct net_buf *buf)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(dlc, struct bt_hfp_hf, rfcomm_dlc);

	atomic_set_bit(hf->flags, BT_HFP_HF_FLAG_RX_ONGOING);
	if (at_parse_input(&hf->at, buf) < 0) {
		LOG_ERR("Parsing failed");
	}
	atomic_clear_bit(hf->flags, BT_HFP_HF_FLAG_RX_ONGOING);
	k_work_submit(&hf->work);
}

static void hfp_hf_sent(struct bt_rfcomm_dlc *dlc, int err)
{
	LOG_DBG("DLC %p sent cb (err %d)", dlc, err);
}

static void bt_hf_work(struct k_work *work)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(work, struct bt_hfp_hf, work);

	hfp_hf_send_data(hf);
}

static struct bt_hfp_hf *hfp_hf_create(struct bt_conn *conn)
{
	size_t index;
	static struct bt_rfcomm_dlc_ops ops = {
		.connected = hfp_hf_connected,
		.disconnected = hfp_hf_disconnected,
		.recv = hfp_hf_recv,
		.sent = hfp_hf_sent,
	};
	struct bt_hfp_hf *hf;

	LOG_DBG("conn %p", conn);

	index = (size_t)bt_conn_index(conn);
	hf = &bt_hfp_hf_pool[index];
	if (hf->acl) {
		LOG_ERR("HF connection (%p) is established", conn);
		return NULL;
	}

	memset(hf, 0, sizeof(*hf));

	hf->acl = conn;
	hf->at.buf = hf->hf_buffer;
	hf->at.buf_max_len = HF_MAX_BUF_LEN;

	hf->rfcomm_dlc.ops = &ops;
	hf->rfcomm_dlc.mtu = BT_HFP_MAX_MTU;

	/* Set the supported features*/
	hf->hf_features = BT_HFP_HF_SUPPORTED_FEATURES;

	/* Set supported codec ids */
	hf->hf_codec_ids = BT_HFP_HF_SUPPORTED_CODEC_IDS;

	k_fifo_init(&hf->tx_pending);

	k_work_init(&hf->work, bt_hf_work);

	k_work_init_delayable(&hf->deferred_work, bt_hf_deferred_work);

	for (index = 0; index < ARRAY_SIZE(hf->ind_table); index++) {
		hf->ind_table[index] = -1;
	}

	return hf;
}

static int hfp_hf_accept(struct bt_conn *conn, struct bt_rfcomm_server *server,
			 struct bt_rfcomm_dlc **dlc)
{
	struct bt_hfp_hf *hf;

	hf = hfp_hf_create(conn);

	if (!hf) {
		return -ECONNREFUSED;
	}

	*dlc = &hf->rfcomm_dlc;

	return 0;
}

static void hfp_hf_sco_connected(struct bt_sco_chan *chan)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(chan, struct bt_hfp_hf, chan);

	if ((bt_hf != NULL) && (bt_hf->sco_connected)) {
		bt_hf->sco_connected(hf, chan->sco);
	}
}

static void hfp_hf_sco_disconnected(struct bt_sco_chan *chan, uint8_t reason)
{
	if ((bt_hf != NULL) && (bt_hf->sco_disconnected)) {
		bt_hf->sco_disconnected(chan->sco, reason);
	}
}

static int bt_hfp_hf_sco_accept(const struct bt_sco_accept_info *info,
		      struct bt_sco_chan **chan)
{
	static struct bt_sco_chan_ops ops = {
		.connected = hfp_hf_sco_connected,
		.disconnected = hfp_hf_sco_disconnected,
	};
	size_t index;
	struct bt_hfp_hf *hf;

	LOG_DBG("conn %p", info->acl);

	index = (size_t)bt_conn_index(info->acl);
	hf = &bt_hfp_hf_pool[index];
	if (hf->acl != info->acl) {
		LOG_ERR("ACL %p of HF is unaligned with SCO's %p", hf->acl, info->acl);
		return -EINVAL;
	}

	hf->chan.ops = &ops;

	*chan = &hf->chan;

	return 0;
}

static void hfp_hf_init(void)
{
	static struct bt_rfcomm_server chan = {
		.channel = BT_RFCOMM_CHAN_HFP_HF,
		.accept = hfp_hf_accept,
	};

	bt_rfcomm_server_register(&chan);

	static struct bt_sco_server sco_server = {
		.sec_level = BT_SECURITY_L0,
		.accept = bt_hfp_hf_sco_accept,
	};

	bt_sco_server_register(&sco_server);

	bt_sdp_register_service(&hfp_rec);
}

int bt_hfp_hf_register(struct bt_hfp_hf_cb *cb)
{
	if (!cb) {
		return -EINVAL;
	}

	if (bt_hf) {
		return -EALREADY;
	}

	bt_hf = cb;

	hfp_hf_init();

	return 0;
}

int bt_hfp_hf_connect(struct bt_conn *conn, struct bt_hfp_hf **hf, uint8_t channel)
{
	struct bt_hfp_hf *new_hf;
	int err;

	if (!conn || !hf || !channel) {
		return -EINVAL;
	}

	if (!bt_hf) {
		return -EFAULT;
	}

	new_hf = hfp_hf_create(conn);
	if (!new_hf) {
		return -ECONNREFUSED;
	}

	err = bt_rfcomm_dlc_connect(conn, &new_hf->rfcomm_dlc, channel);
	if (err != 0) {
		(void)memset(new_hf, 0, sizeof(*new_hf));
		*hf = NULL;
	} else {
		*hf = new_hf;
	}

	return err;
}

int bt_hfp_hf_disconnect(struct bt_hfp_hf *hf)
{
	LOG_DBG("");

	if (!hf) {
		return -EINVAL;
	}

	return bt_rfcomm_dlc_disconnect(&hf->rfcomm_dlc);
}
