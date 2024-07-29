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
#include "hfp_internal.h"

#define LOG_LEVEL CONFIG_BT_HFP_HF_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hfp_hf);

#define MAX_IND_STR_LEN 17

struct bt_hfp_hf_cb *bt_hf;

NET_BUF_POOL_FIXED_DEFINE(hf_pool, CONFIG_BT_MAX_CONN + 1,
			  BT_RFCOMM_BUF_SIZE(BT_HF_CLIENT_MAX_PDU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_hfp_hf bt_hfp_hf_pool[CONFIG_BT_MAX_CONN];

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
		LOG_DBG("Remove completed buf %p on %p", k_fifo_peek_head(&hf->tx_pending), hf);
		(void)k_fifo_get(&hf->tx_pending, K_NO_WAIT);
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

	if (atomic_test_bit(hf->flags, BT_HFP_HF_FLAG_TX_ONGOING)) {
		return;
	}

	buf = k_fifo_peek_head(&hf->tx_pending);
	if (!buf) {
		return;
	}

	atomic_set_bit(hf->flags, BT_HFP_HF_FLAG_TX_ONGOING);

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

void ag_indicator_handle_values(struct at_client *hf_at, uint32_t index,
				uint32_t value)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	struct bt_conn *conn = hf->acl;

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
			bt_hf->service(conn, value);
		}
		break;
	case HF_CALL_IND:
		if (bt_hf->call) {
			bt_hf->call(conn, value);
		}
		break;
	case HF_CALL_SETUP_IND:
		atomic_set_bit_to(hf->flags, BT_HFP_HF_FLAG_INCOMING,
		    value == BT_HFP_CALL_SETUP_INCOMING);

		if (bt_hf->call_setup) {
			bt_hf->call_setup(conn, value);
		}
		break;
	case HF_CALL_HELD_IND:
		if (bt_hf->call_held) {
			bt_hf->call_held(conn, value);
		}
		break;
	case HF_SINGNAL_IND:
		if (bt_hf->signal) {
			bt_hf->signal(conn, value);
		}
		break;
	case HF_ROAM_IND:
		if (bt_hf->roam) {
			bt_hf->roam(conn, value);
		}
		break;
	case HF_BATTERY_IND:
		if (bt_hf->battery) {
			bt_hf->battery(conn, value);
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
	struct bt_conn *conn = hf->acl;

	if (bt_hf->ring_indication) {
		bt_hf->ring_indication(conn);
	}

	return 0;
}

#if defined(CONFIG_BT_HFP_HF_CLI)
int clip_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	struct bt_conn *conn = hf->acl;
	char *number;
	uint32_t type;
	int err;

	number = at_get_string(hf_at);

	err = at_get_number(hf_at, &type);
	if (err) {
		LOG_WRN("could not get the type");
	} else {
		type = 0;
	}

	if (bt_hf->clip) {
		bt_hf->clip(conn, number, (uint8_t)type);
	}

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_CLI */

#if defined(CONFIG_BT_HFP_HF_VOLUME)
int vgm_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	struct bt_conn *conn = hf->acl;
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
		bt_hf->vgm(conn, (uint8_t)gain);
	}

	return 0;
}

int vgs_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	struct bt_conn *conn = hf->acl;
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
		bt_hf->vgs(conn, (uint8_t)gain);
	}

	return 0;
}
#endif /* CONFIG_BT_HFP_HF_VOLUME */

int bsir_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	struct bt_conn *conn = hf->acl;
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
		bt_hf->inband_ring(conn, (bool)inband);
	}

	return 0;
}

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

int cmd_complete(struct at_client *hf_at, enum at_result result,
	       enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	struct bt_conn *conn = hf->acl;
	struct bt_hfp_hf_cmd_complete cmd = { 0 };

	LOG_DBG("");

	switch (result) {
	case AT_RESULT_OK:
		cmd.type = HFP_HF_CMD_OK;
		break;
	case AT_RESULT_ERROR:
		cmd.type = HFP_HF_CMD_ERROR;
		break;
	case AT_RESULT_CME_ERROR:
		cmd.type = HFP_HF_CMD_CME_ERROR;
		cmd.cme = cme_err;
		break;
	default:
		LOG_ERR("Unknown error code");
		cmd.type = HFP_HF_CMD_UNKNOWN_ERROR;
		break;
	}

	if (bt_hf->cmd_complete_cb) {
		bt_hf->cmd_complete_cb(conn, &cmd);
	}

	return 0;
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
		bt_hf->connected(conn);
	}

	atomic_set_bit(hf->flags, BT_HFP_HF_FLAG_CONNECTED);

	/* Start with first AT command */
	hf->cmd_init_seq = 0;
	if (at_cmd_init_start(hf)) {
		LOG_ERR("Fail to start AT command initialization");
	}
}

int cmer_finish(struct at_client *hf_at, enum at_result result,
		enum at_cme cme_err)
{
	if (result != AT_RESULT_OK) {
		LOG_ERR("SLC Connection ERROR in response");
		hf_slc_error(hf_at);
		return -EINVAL;
	}

	slc_completed(hf_at);

	return 0;
}

int cind_status_finish(struct at_client *hf_at, enum at_result result,
		       enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	int err;

	if (result != AT_RESULT_OK) {
		LOG_ERR("SLC Connection ERROR in response");
		hf_slc_error(hf_at);
		return -EINVAL;
	}

	at_register_unsolicited(hf_at, unsolicited_cb);
	err = hfp_hf_send_cmd(hf, NULL, cmer_finish, "AT+CMER=3,0,0,1");
	if (err < 0) {
		hf_slc_error(hf_at);
		return err;
	}

	return 0;
}

int cind_finish(struct at_client *hf_at, enum at_result result,
		enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	int err;

	if (result != AT_RESULT_OK) {
		LOG_ERR("SLC Connection ERROR in response");
		hf_slc_error(hf_at);
		return -EINVAL;
	}

	err = hfp_hf_send_cmd(hf, cind_status_resp, cind_status_finish,
			      "AT+CIND?");
	if (err < 0) {
		hf_slc_error(hf_at);
		return err;
	}

	return 0;
}

int brsf_finish(struct at_client *hf_at, enum at_result result,
		enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	int err;

	if (result != AT_RESULT_OK) {
		LOG_ERR("SLC Connection ERROR in response");
		hf_slc_error(hf_at);
		return -EINVAL;
	}

	err = hfp_hf_send_cmd(hf, cind_resp, cind_finish, "AT+CIND=?");
	if (err < 0) {
		hf_slc_error(hf_at);
		return err;
	}

	return 0;
}

int hf_slc_establish(struct bt_hfp_hf *hf)
{
	int err;

	LOG_DBG("");

	err = hfp_hf_send_cmd(hf, brsf_resp, brsf_finish, "AT+BRSF=%u",
			      hf->hf_features);
	if (err < 0) {
		hf_slc_error(&hf->at);
		return err;
	}

	return 0;
}

static struct bt_hfp_hf *bt_hfp_hf_lookup_bt_conn(struct bt_conn *conn)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_hfp_hf_pool); i++) {
		struct bt_hfp_hf *hf = &bt_hfp_hf_pool[i];

		if (hf->acl == conn) {
			return hf;
		}
	}

	return NULL;
}

int bt_hfp_hf_send_cmd(struct bt_conn *conn, enum bt_hfp_hf_at_cmd cmd)
{
	struct bt_hfp_hf *hf;
	int err;

	LOG_DBG("");

	if (!conn) {
		LOG_ERR("Invalid connection");
		return -ENOTCONN;
	}

	hf = bt_hfp_hf_lookup_bt_conn(conn);
	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	switch (cmd) {
	case BT_HFP_HF_ATA:
		err = hfp_hf_send_cmd(hf, NULL, cmd_complete, "ATA");
		if (err < 0) {
			LOG_ERR("Failed ATA");
			return err;
		}
		break;
	case BT_HFP_HF_AT_CHUP:
		err = hfp_hf_send_cmd(hf, NULL, cmd_complete, "AT+CHUP");
		if (err < 0) {
			LOG_ERR("Failed AT+CHUP");
			return err;
		}
		break;
	default:
		LOG_ERR("Invalid AT Command");
		return -EINVAL;
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

int bt_hfp_hf_cli(struct bt_conn *conn, bool enable)
{
#if defined(CONFIG_BT_HFP_HF_CLI)
	struct bt_hfp_hf *hf;
	int err;

	LOG_DBG("");

	if (!conn) {
		LOG_ERR("Invalid connection");
		return -ENOTCONN;
	}

	hf = bt_hfp_hf_lookup_bt_conn(conn);
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

int bt_hfp_hf_vgm(struct bt_conn *conn, uint8_t gain)
{
#if defined(CONFIG_BT_HFP_HF_VOLUME)
	struct bt_hfp_hf *hf;
	int err;

	LOG_DBG("");

	if (gain > BT_HFP_HF_VGM_GAIN_MAX) {
		LOG_ERR("Invalid gain %d>%d", gain, BT_HFP_HF_VGM_GAIN_MAX);
		return -EINVAL;
	}

	if (!conn) {
		LOG_ERR("Invalid connection");
		return -ENOTCONN;
	}

	hf = bt_hfp_hf_lookup_bt_conn(conn);
	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
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

int bt_hfp_hf_vgs(struct bt_conn *conn, uint8_t gain)
{
#if defined(CONFIG_BT_HFP_HF_VOLUME)
	struct bt_hfp_hf *hf;
	int err;

	LOG_DBG("");

	if (gain > BT_HFP_HF_VGM_GAIN_MAX) {
		LOG_ERR("Invalid gain %d>%d", gain, BT_HFP_HF_VGM_GAIN_MAX);
		return -EINVAL;
	}

	if (!conn) {
		LOG_ERR("Invalid connection");
		return -ENOTCONN;
	}

	hf = bt_hfp_hf_lookup_bt_conn(conn);
	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
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
		bt_hf->operator(hf->acl, (uint8_t)mode, (uint8_t)format, operator);
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

int bt_hfp_hf_get_operator(struct bt_conn *conn)
{
	struct bt_hfp_hf *hf;
	int err;

	LOG_DBG("");

	if (!conn) {
		LOG_ERR("Invalid connection");
		return -ENOTCONN;
	}

	hf = bt_hfp_hf_lookup_bt_conn(conn);
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

static int ata_finish(struct at_client *hf_at, enum at_result result,
		   enum at_cme cme_err)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);

	LOG_DBG("ATA (result %d) on %p", result, hf);

	return 0;
}

int bt_hfp_hf_accept(struct bt_conn *conn)
{
	struct bt_hfp_hf *hf;
	int err;

	LOG_DBG("");

	if (!conn) {
		LOG_ERR("Invalid connection");
		return -ENOTCONN;
	}

	hf = bt_hfp_hf_lookup_bt_conn(conn);
	if (!hf) {
		LOG_ERR("No HF connection found");
		return -ENOTCONN;
	}

	if (!atomic_test_bit(hf->flags, BT_HFP_HF_FLAG_INCOMING)) {
		LOG_ERR("No incoming call setup in progress");
		return -EINVAL;
	}

	err = hfp_hf_send_cmd(hf, NULL, ata_finish, "ATA");
	if (err < 0) {
		LOG_ERR("Fail to accept the incoming call on %p", hf);
	}

	return err;
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
	struct bt_conn *conn = hf->acl;

	LOG_DBG("hf disconnected!");
	if (bt_hf->disconnected) {
		bt_hf->disconnected(conn);
	}
}

static void hfp_hf_recv(struct bt_rfcomm_dlc *dlc, struct net_buf *buf)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(dlc, struct bt_hfp_hf, rfcomm_dlc);

	if (at_parse_input(&hf->at, buf) < 0) {
		LOG_ERR("Parsing failed");
	}
}

static void hfp_hf_sent(struct bt_rfcomm_dlc *dlc, int err)
{
	LOG_DBG("DLC %p sent cb (err %d)", dlc, err);
}

static int hfp_hf_accept(struct bt_conn *conn, struct bt_rfcomm_dlc **dlc)
{
	int i;
	static struct bt_rfcomm_dlc_ops ops = {
		.connected = hfp_hf_connected,
		.disconnected = hfp_hf_disconnected,
		.recv = hfp_hf_recv,
		.sent = hfp_hf_sent,
	};

	LOG_DBG("conn %p", conn);

	for (i = 0; i < ARRAY_SIZE(bt_hfp_hf_pool); i++) {
		struct bt_hfp_hf *hf = &bt_hfp_hf_pool[i];
		int j;

		if (hf->rfcomm_dlc.session) {
			continue;
		}

		memset(hf, 0, sizeof(*hf));

		hf->acl = conn;
		hf->at.buf = hf->hf_buffer;
		hf->at.buf_max_len = HF_MAX_BUF_LEN;

		hf->rfcomm_dlc.ops = &ops;
		hf->rfcomm_dlc.mtu = BT_HFP_MAX_MTU;

		*dlc = &hf->rfcomm_dlc;

		/* Set the supported features*/
		hf->hf_features = BT_HFP_HF_SUPPORTED_FEATURES;

		k_fifo_init(&hf->tx_pending);

		for (j = 0; j < HF_MAX_AG_INDICATORS; j++) {
			hf->ind_table[j] = -1;
		}

		return 0;
	}

	LOG_ERR("Unable to establish HF connection (%p)", conn);

	return -ENOMEM;
}

static void hfp_hf_sco_connected(struct bt_sco_chan *chan)
{
	if ((bt_hf != NULL) && (bt_hf->sco_connected)) {
		bt_hf->sco_connected(chan->sco->sco.acl, chan->sco);
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
	int i;
	static struct bt_sco_chan_ops ops = {
		.connected = hfp_hf_sco_connected,
		.disconnected = hfp_hf_sco_disconnected,
	};

	LOG_DBG("conn %p", info->acl);

	for (i = 0; i < ARRAY_SIZE(bt_hfp_hf_pool); i++) {
		struct bt_hfp_hf *hf = &bt_hfp_hf_pool[i];

		if (NULL == hf->rfcomm_dlc.session) {
			continue;
		}

		if (info->acl != hf->rfcomm_dlc.session->br_chan.chan.conn) {
			continue;
		}

		hf->chan.ops = &ops;

		*chan = &hf->chan;

		return 0;
	}

	LOG_ERR("Unable to establish HF connection (%p)", info->acl);

	return -ENOMEM;
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
