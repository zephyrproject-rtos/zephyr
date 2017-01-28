/* hfp_hf.c - Hands free Profile - Handsfree side handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <errno.h>
#include <atomic.h>
#include <misc/byteorder.h>
#include <misc/util.h>
#include <misc/printk.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BLUETOOTH_DEBUG_HFP_HF)
#include <bluetooth/log.h>
#include <bluetooth/conn.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hfp_hf.h>

#include "l2cap_internal.h"
#include "rfcomm_internal.h"
#include "at.h"
#include "hfp_internal.h"

#define MAX_IND_STR_LEN 17

struct bt_hfp_hf_cb *bt_hf;

NET_BUF_POOL_DEFINE(hf_pool, CONFIG_BLUETOOTH_MAX_CONN + 1,
		    BT_RFCOMM_BUF_SIZE(BLUETOOTH_HF_CLIENT_MAX_PDU),
		    BT_BUF_USER_DATA_MIN, NULL);

static struct bt_hfp_hf bt_hfp_hf_pool[CONFIG_BLUETOOTH_MAX_CONN];

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
	{"signal", 0, 5}, /* HF_SINGNAL_IND */
	{"roam", 0, 1}, /* HF_ROAM_IND */
	{"battchg", 0, 5} /* HF_BATTERY_IND */
};

void hf_slc_error(struct at_client *hf_at)
{
	BT_ERR("SLC error: disconnecting");
}

int hfp_hf_send_cmd(struct bt_hfp_hf *hf, at_resp_cb_t resp,
		    at_finish_cb_t finish, const char *format, ...)
{
	struct net_buf *buf;
	va_list vargs;
	int ret;

	/* register the callbacks */
	at_register(&hf->at, resp, finish);

	buf = bt_rfcomm_create_pdu(&hf_pool);
	if (!buf) {
		BT_ERR("No Buffers!");
		return -ENOMEM;
	}

	va_start(vargs, format);
	ret = vsnprintk(buf->data, (net_buf_tailroom(buf) - 1), format, vargs);
	if (ret < 0) {
		BT_ERR("Unable to format variable arguments");
		return ret;
	}
	va_end(vargs);

	net_buf_add(buf, ret);
	net_buf_add_u8(buf, '\r');

	ret = bt_rfcomm_dlc_send(&hf->rfcomm_dlc, buf);
	if (ret < 0) {
		BT_ERR("Rfcomm send error :(%d)", ret);
		return ret;
	}

	return 0;
}

int brsf_handle(struct at_client *hf_at)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	uint32_t val;
	int ret;

	ret = at_get_number(hf_at, &val);
	if (ret < 0) {
		BT_ERR("Error getting value");
		return ret;
	}

	hf->ag_features = val;

	return 0;
}

int brsf_resp(struct at_client *hf_at, struct net_buf *buf)
{
	int err;

	BT_DBG("");

	err = at_parse_cmd_input(hf_at, buf, "BRSF", brsf_handle,
				 AT_CMD_TYPE_NORMAL);
	if (err < 0) {
		/* Returning negative value is avoided before SLC connection
		 * established.
		 */
		BT_ERR("Error parsing CMD input");
		hf_slc_error(hf_at);
	}

	return 0;
}

static void cind_handle_values(struct at_client *hf_at, uint32_t index,
			       char *name, uint32_t min, uint32_t max)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	int i;

	BT_DBG("index: %u, name: %s, min: %u, max:%u", index, name, min, max);

	for (i = 0; i < ARRAY_SIZE(ag_ind); i++) {
		if (strcmp(name, ag_ind[i].name) != 0) {
			continue;
		}
		if (min != ag_ind[i].min || max != ag_ind[i].max) {
			BT_ERR("%s indicator min/max value not matching", name);
		}

		hf->ind_table[index] = i;
		break;
	}
}

int cind_handle(struct at_client *hf_at)
{
	uint32_t index = 0;

	/* Parsing Example: CIND: ("call",(0,1)) etc.. */
	while (at_has_next_list(hf_at)) {
		char name[MAX_IND_STR_LEN];
		uint32_t min, max;

		if (at_open_list(hf_at) < 0) {
			BT_ERR("Could not get open list");
			goto error;
		}

		if (at_list_get_string(hf_at, name, sizeof(name)) < 0) {
			BT_ERR("Could not get string");
			goto error;
		}

		if (at_open_list(hf_at) < 0) {
			BT_ERR("Could not get open list");
			goto error;
		}

		if (at_list_get_range(hf_at, &min, &max) < 0) {
			BT_ERR("Could not get range");
			goto error;
		}

		if (at_close_list(hf_at) < 0) {
			BT_ERR("Could not get close list");
			goto error;
		}

		if (at_close_list(hf_at) < 0) {
			BT_ERR("Could not get close list");
			goto error;
		}

		cind_handle_values(hf_at, index, name, min, max);
		index++;
	}

	return 0;
error:
	BT_ERR("Error on CIND response");
	hf_slc_error(hf_at);
	return -EINVAL;
}

int cind_resp(struct at_client *hf_at, struct net_buf *buf)
{
	int err;

	err = at_parse_cmd_input(hf_at, buf, "CIND", cind_handle,
				 AT_CMD_TYPE_NORMAL);
	if (err < 0) {
		BT_ERR("Error parsing CMD input");
		hf_slc_error(hf_at);
	}

	return 0;
}

void ag_indicator_handle_values(struct at_client *hf_at, uint32_t index,
				uint32_t value)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	struct bt_conn *conn = hf->rfcomm_dlc.session->br_chan.chan.conn;

	BT_DBG("Index :%u, Value :%u", index, value);

	if (index >= ARRAY_SIZE(ag_ind)) {
		BT_ERR("Max only %lu indicators are supported",
		       ARRAY_SIZE(ag_ind));
		return;
	}

	if (value > ag_ind[hf->ind_table[index]].max ||
	    value < ag_ind[hf->ind_table[index]].min) {
		BT_ERR("Indicators out of range - value: %u", value);
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
		BT_ERR("Unknown AG indicator");
		break;
	}
}

int cind_status_handle(struct at_client *hf_at)
{
	uint32_t index = 0;

	while (at_has_next_list(hf_at)) {
		uint32_t value;
		int ret;

		ret = at_get_number(hf_at, &value);
		if (ret < 0) {
			BT_ERR("could not get the value");
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
		BT_ERR("Error parsing CMD input");
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
		BT_ERR("could not get the Index");
		return ret;
	}
	/* The first element of the list shall have 1 */
	if (!index) {
		BT_ERR("Invalid index value '0'");
		return 0;
	}

	ret = at_get_number(hf_at, &value);
	if (ret < 0) {
		BT_ERR("could not get the value");
		return ret;
	}

	ag_indicator_handle_values(hf_at, (index - 1), value);

	return 0;
}

int unsolicited_cb(struct at_client *hf_at, struct net_buf *buf)
{
	if (!at_parse_cmd_input(hf_at, buf, "CIEV", ciev_handle,
				AT_CMD_TYPE_UNSOLICITED)) {
		return 0;
	}

	return -EINVAL;
}

int cmer_finish(struct at_client *hf_at, struct net_buf *buf,
		enum at_result result)
{
	if (result != AT_RESULT_OK) {
		BT_ERR("SLC Connection ERROR in response");
		hf_slc_error(hf_at);
		return -EINVAL;
	}

	return 0;
}

int cind_status_finish(struct at_client *hf_at, struct net_buf *buf,
		       enum at_result result)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	int err;

	if (result != AT_RESULT_OK) {
		BT_ERR("SLC Connection ERROR in response");
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

int cind_finish(struct at_client *hf_at, struct net_buf *buf,
		enum at_result result)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	int err;

	if (result != AT_RESULT_OK) {
		BT_ERR("SLC Connection ERROR in response");
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

int brsf_finish(struct at_client *hf_at, struct net_buf *buf,
		enum at_result result)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(hf_at, struct bt_hfp_hf, at);
	int err;

	if (result != AT_RESULT_OK) {
		BT_ERR("SLC Connection ERROR in response");
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

	BT_DBG("");

	err = hfp_hf_send_cmd(hf, brsf_resp, brsf_finish, "AT+BRSF=%u",
			      hf->hf_features);
	if (err < 0) {
		hf_slc_error(&hf->at);
		return err;
	}

	return 0;
}

static void hfp_hf_connected(struct bt_rfcomm_dlc *dlc)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(dlc, struct bt_hfp_hf, rfcomm_dlc);

	BT_DBG("hf connected");

	BT_ASSERT(hf);
	hf_slc_establish(hf);
}

static void hfp_hf_disconnected(struct bt_rfcomm_dlc *dlc)
{
	BT_DBG("hf disconnected!");
}

static void hfp_hf_recv(struct bt_rfcomm_dlc *dlc, struct net_buf *buf)
{
	struct bt_hfp_hf *hf = CONTAINER_OF(dlc, struct bt_hfp_hf, rfcomm_dlc);

	if (at_parse_input(&hf->at, buf) < 0) {
		BT_ERR("Parsing failed");
	}
}

static int bt_hfp_hf_accept(struct bt_conn *conn, struct bt_rfcomm_dlc **dlc)
{
	int i;
	static struct bt_rfcomm_dlc_ops ops = {
		.connected = hfp_hf_connected,
		.disconnected = hfp_hf_disconnected,
		.recv = hfp_hf_recv,
	};

	BT_DBG("conn %p", conn);

	for (i = 0; i < ARRAY_SIZE(bt_hfp_hf_pool); i++) {
		struct bt_hfp_hf *hf = &bt_hfp_hf_pool[i];
		int j;

		if (hf->rfcomm_dlc.session) {
			continue;
		}

		hf->at.buf = hf->hf_buffer;
		hf->at.buf_max_len = HF_MAX_BUF_LEN;

		hf->rfcomm_dlc.ops = &ops;
		hf->rfcomm_dlc.mtu = BLUETOOTH_HFP_MAX_MTU;

		*dlc = &hf->rfcomm_dlc;

		/* Set the supported features*/
		hf->hf_features = BT_HFP_HF_SUPPORTED_FEATURES;

		for (j = 0; j < HF_MAX_AG_INDICATORS; j++) {
			hf->ind_table[j] = -1;
		}

		return 0;
	}

	BT_ERR("Unable to establish HF connection (%p)", conn);

	return -ENOMEM;
}

static void hfp_hf_init(void)
{
	static struct bt_rfcomm_server chan = {
		.channel = BT_RFCOMM_CHAN_HFP_HF,
		.accept = bt_hfp_hf_accept,
	};

	bt_rfcomm_server_register(&chan);
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
