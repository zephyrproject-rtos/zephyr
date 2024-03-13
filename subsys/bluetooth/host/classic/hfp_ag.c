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

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "l2cap_br_internal.h"
#include "rfcomm_internal.h"
#include "at.h"
#include "sco_internal.h"

#include "hfp_internal.h"
#include "hfp_ag_internal.h"

#define LOG_LEVEL CONFIG_BT_HFP_AG_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hfp_ag);

typedef int (*bt_hfp_ag_parse_command_t)(struct bt_hfp_ag *ag, struct net_buf *buf,
					 enum at_cme *err);

struct bt_hfp_ag_at_cmd_handler {
	char *cmd;
	bt_hfp_ag_parse_command_t handler;
};

static const struct {
	char *name;
	char *connector;
	uint32_t min;
	uint32_t max;
} ag_ind[] = {
	{"service", "-", 0, 1},   /* BT_HFP_AG_SERVICE_IND */
	{"call", ",", 0, 1},      /* BT_HFP_AG_CALL_IND */
	{"callsetup", "-", 0, 3}, /* BT_HFP_AG_CALL_SETUP_IND */
	{"callheld", "-", 0, 2},  /* BT_HFP_AG_CALL_HELD_IND */
	{"signal", "-", 0, 5},    /* BT_HFP_AG_SINGNAL_IND */
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
};

static void bt_hfp_ag_ringing_cb(struct bt_hfp_ag *ag, void *user_data);

static int hfp_ag_send_data(struct bt_hfp_ag *ag, bt_hfp_ag_tx_cb_t cb, void *user_data,
			    const char *format, ...);
static int hfp_ag_next_step(struct bt_hfp_ag *ag, bt_hfp_ag_tx_cb_t cb, void *user_data);

static int bt_hfp_ag_brsf_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err);
static int bt_hfp_ag_bac_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err);
static int bt_hfp_ag_cind_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err);
static int bt_hfp_ag_cmer_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err);
static int bt_hfp_ag_chld_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err);
static int bt_hfp_ag_bind_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err);
static int bt_hfp_ag_cmee_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err);
static int bt_hfp_ag_chup_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err);
static int bt_hfp_ag_clcc_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err);
static int bt_hfp_ag_bia_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err);
static int bt_hfp_ag_ata_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err);
static int bt_hfp_ag_cops_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err);
static int bt_hfp_ag_bcc_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err);
static int bt_hfp_ag_bcs_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err);
static int bt_hfp_ag_atd_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err);
static int bt_hfp_ag_bldn_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err);
static int bt_hfp_ag_clip_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err);

static struct bt_hfp_ag_at_cmd_handler cmd_handlers[] = {
	{"AT+BRSF", bt_hfp_ag_brsf_handler}, {"AT+BAC", bt_hfp_ag_bac_handler},
	{"AT+CIND", bt_hfp_ag_cind_handler}, {"AT+CMER", bt_hfp_ag_cmer_handler},
	{"AT+CHLD", bt_hfp_ag_chld_handler}, {"AT+BIND", bt_hfp_ag_bind_handler},
	{"AT+CMEE", bt_hfp_ag_cmee_handler}, {"AT+CHUP", bt_hfp_ag_chup_handler},
	{"AT+CLCC", bt_hfp_ag_clcc_handler}, {"AT+BIA", bt_hfp_ag_bia_handler},
	{"ATA", bt_hfp_ag_ata_handler},      {"AT+COPS", bt_hfp_ag_cops_handler},
	{"AT+BCC", bt_hfp_ag_bcc_handler},   {"AT+BCS", bt_hfp_ag_bcs_handler},
	{"ATD", bt_hfp_ag_atd_handler},      {"AT+BLDN", bt_hfp_ag_bldn_handler},
	{"AT+CLIP", bt_hfp_ag_clip_handler},
};

NET_BUF_POOL_FIXED_DEFINE(ag_pool, CONFIG_BT_HFP_AG_TX_BUF_COUNT,
			  BT_RFCOMM_BUF_SIZE(BT_HF_CLIENT_MAX_PDU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_hfp_ag bt_hfp_ag_pool[CONFIG_BT_MAX_CONN];

static struct bt_hfp_ag_cb *bt_ag;

/* Sent but not acknowledged TX packets with a callback */
static struct bt_ag_tx ag_tx[CONFIG_BT_HFP_AG_TX_BUF_COUNT * 2];
static K_FIFO_DEFINE(ag_tx_free);
static K_FIFO_DEFINE(ag_tx_notify);

struct k_thread ag_thread;
static K_KERNEL_STACK_MEMBER(ag_thread_stack, CONFIG_BT_HFP_AG_THREAD_STACK_SIZE);
static k_tid_t ag_thread_id;

static struct bt_conn *bt_hfp_ag_create_sco(struct bt_hfp_ag *ag);

static void bt_hfp_ag_set_call_state(struct bt_hfp_ag *ag, bt_hfp_call_state_t state);

static void bt_hfp_ag_set_state(struct bt_hfp_ag *ag, bt_hfp_state_t state)
{
	ag->state = state;

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
			bt_ag->connected(ag);
		}
		break;
	case BT_HFP_DISCONNECTING:
		break;
	default:
		LOG_WRN("no valid (%u) state was set", state);
		break;
	}
}

static void bt_hfp_ag_set_call_state(struct bt_hfp_ag *ag, bt_hfp_call_state_t state)
{
	ag->call_state = state;

	switch (state) {
	case BT_HFP_CALL_TERMINATE:
		atomic_clear(ag->flags);
		k_work_cancel_delayable(&ag->deferred_work);
		break;
	case BT_HFP_CALL_OUTGOING:
		k_work_reschedule(&ag->deferred_work, K_SECONDS(CONFIG_BT_HFP_AG_OUTGOING_TIMEOUT));
		break;
	case BT_HFP_CALL_INCOMING:
		k_work_reschedule(&ag->deferred_work, K_SECONDS(CONFIG_BT_HFP_AG_INCOMING_TIMEOUT));
		break;
	case BT_HFP_CALL_ALERTING:
		k_work_reschedule(&ag->ringing_work, K_NO_WAIT);
		k_work_reschedule(&ag->deferred_work, K_SECONDS(CONFIG_BT_HFP_AG_ALERTING_TIMEOUT));
		break;
	case BT_HFP_CALL_ACTIVE:
		k_work_cancel_delayable(&ag->deferred_work);
		break;
	case BT_HFP_CALL_HOLD:
		break;
	default:
		/* Invalid state */
		break;
	}

	if (ag->state == BT_HFP_DISCONNECTING) {
		int err = bt_rfcomm_dlc_disconnect(&ag->rfcomm_dlc);

		if (err) {
			LOG_ERR("Fail to disconnect DLC %p", &ag->rfcomm_dlc);
		}
	}
}

static void skip_space(struct net_buf *buf)
{
	while ((buf->len > 0) && (buf->data[0] == ' ')) {
		(void)net_buf_pull(buf, 1);
	}
}

static int get_number(struct net_buf *buf, uint32_t *number)
{
	int err = -EINVAL;

	*number = 0;

	skip_space(buf);
	while ((buf->len > 0)) {
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

static int is_char(struct net_buf *buf, uint8_t c)
{
	int err = -EINVAL;

	skip_space(buf);
	while ((buf->len > 0)) {
		if (buf->data[0] == c) {
			(void)net_buf_pull(buf, 1);
			err = 0;
		} else {
			break;
		}
	}
	skip_space(buf);

	return err;
}

static int bt_hfp_ag_brsf_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err)
{
	uint32_t number;
	int error;

	*err = CME_ERROR_AG_FAILURE;

	error = is_char(buf, '=');
	if (error != 0) {
		return error;
	}

	error = get_number(buf, &number);

	if (error != 0) {
		return error;
	}

	error = is_char(buf, '\r');
	if (error != 0) {
		return error;
	}

	ag->hf_features = number;

	return hfp_ag_send_data(ag, NULL, NULL, "\r\n+BRSF:%d\r\n", ag->ag_features);
}

static int bt_hfp_ag_bac_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err)
{
	uint32_t number;
	uint32_t codec_ids = 0U;
	int error;

	*err = CME_ERROR_AG_FAILURE;

	error = is_char(buf, '=');
	if (error != 0) {
		return error;
	}

	if (!(ag->hf_features & BT_HFP_HF_FEATURE_CODEC_NEG) ||
	    !(ag->ag_features & BT_HFP_AG_FEATURE_CODEC_NEG)) {
		return -EOPNOTSUPP;
	}

	while (error == 0) {
		error = get_number(buf, &number);

		if (error != 0) {
			return error;
		}

		error = is_char(buf, ',');
		if (error != 0) {
			if (0 != is_char(buf, '\r')) {
				return error;
			}
		}

		if (number < (sizeof(codec_ids) * 8)) {
			codec_ids |= BIT(number);
		}
	}

	ag->hf_codec_ids = codec_ids;

	if (bt_ag && bt_ag->codec) {
		bt_ag->codec(ag, ag->hf_codec_ids);
	}

	return 0;
}

static int bt_hfp_ag_cind_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err)
{
	int error;
	bool inquiry_status = true;

	*err = CME_ERROR_AG_FAILURE;

	error = is_char(buf, '=');
	if (error == 0) {
		inquiry_status = false;
	}

	error = is_char(buf, '?');
	if (error != 0) {
		return error;
	}

	error = is_char(buf, '\r');
	if (error != 0) {
		return error;
	}

	if (false == inquiry_status) {
		error = hfp_ag_send_data(
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
			ag_ind[BT_HFP_AG_CALL_HELD_IND].max, ag_ind[BT_HFP_AG_SINGNAL_IND].name,
			ag_ind[BT_HFP_AG_SINGNAL_IND].min, ag_ind[BT_HFP_AG_SINGNAL_IND].connector,
			ag_ind[BT_HFP_AG_SINGNAL_IND].max, ag_ind[BT_HFP_AG_ROAM_IND].name,
			ag_ind[BT_HFP_AG_ROAM_IND].min, ag_ind[BT_HFP_AG_ROAM_IND].connector,
			ag_ind[BT_HFP_AG_ROAM_IND].max, ag_ind[BT_HFP_AG_BATTERY_IND].name,
			ag_ind[BT_HFP_AG_BATTERY_IND].min, ag_ind[BT_HFP_AG_BATTERY_IND].connector,
			ag_ind[BT_HFP_AG_BATTERY_IND].max);
	} else {
		error = hfp_ag_send_data(ag, NULL, NULL, "\r\n+CIND:%d,%d,%d,%d,%d,%d,%d\r\n",
					 ag->indicator_value[BT_HFP_AG_SERVICE_IND],
					 ag->indicator_value[BT_HFP_AG_CALL_IND],
					 ag->indicator_value[BT_HFP_AG_CALL_SETUP_IND],
					 ag->indicator_value[BT_HFP_AG_CALL_HELD_IND],
					 ag->indicator_value[BT_HFP_AG_SINGNAL_IND],
					 ag->indicator_value[BT_HFP_AG_ROAM_IND],
					 ag->indicator_value[BT_HFP_AG_BATTERY_IND]);
	}

	return error;
}

static void bt_hfp_ag_set_in_band_ring(struct bt_hfp_ag *ag, void *user_data)
{
	if (0 != (ag->ag_features & BT_HFP_AG_FEATURE_INBAND_RINGTONE)) {
		int error = hfp_ag_send_data(ag, NULL, NULL, "\r\n+BSIR:1\r\n");

		if (error == 0) {
			atomic_set_bit(ag->flags, BT_HFP_AG_INBAND_RING);
		} else {
			atomic_clear_bit(ag->flags, BT_HFP_AG_INBAND_RING);
		}
	}
}

static int bt_hfp_ag_cmer_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err)
{
	uint32_t number;
	int error;
	static const uint32_t command_line_prefix[] = {3, 0, 0};

	*err = CME_ERROR_AG_FAILURE;

	error = is_char(buf, '=');
	if (error != 0) {
		return error;
	}

	for (int i = 0; i < ARRAY_SIZE(command_line_prefix); i++) {
		error = get_number(buf, &number);

		if (error != 0) {
			return error;
		}

		error = is_char(buf, ',');
		if (error != 0) {
			return error;
		}

		if (command_line_prefix[i] != number) {
			return -EINVAL;
		}
	}

	error = get_number(buf, &number);
	if (error != 0) {
		return error;
	}

	error = is_char(buf, '\r');
	if (error != 0) {
		return error;
	}

	if (number == 1) {
		atomic_set_bit(ag->flags, BT_HFP_AG_CMER_ENABLE);
		bt_hfp_ag_set_state(ag, BT_HFP_CONNECTED);
		error = hfp_ag_next_step(ag, bt_hfp_ag_set_in_band_ring, NULL);
		return error;
	} else if (number == 0) {
		atomic_clear_bit(ag->flags, BT_HFP_AG_CMER_ENABLE);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int bt_hfp_ag_chld_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err)
{
	return -EINVAL;
}

static int bt_hfp_ag_bind_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err)
{
	uint32_t number;
	uint32_t hf_indicators = 0U;
	int error;
	char *data;
	uint32_t len;

	if (!((ag->ag_features & BT_HFP_AG_FEATURE_HF_IND) &&
	      (ag->hf_features & BT_HFP_HF_FEATURE_HF_IND))) {
		*err = CME_ERROR_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	*err = CME_ERROR_AG_FAILURE;

	error = is_char(buf, '?');
	if (error == 0) {
		error = is_char(buf, '\r');
		if (error != 0) {
			return error;
		}

		hf_indicators = ag->hf_indicators_of_hf & ag->hf_indicators_of_ag;
		len = (sizeof(hf_indicators) * 8) > HFP_HF_IND_MAX ? HFP_HF_IND_MAX
								   : (sizeof(hf_indicators) * 8);
		for (int i = 1; i < len; i++) {
			if (BIT(i) & hf_indicators) {
				error = hfp_ag_send_data(ag, NULL, NULL, "\r\n+BIND:%d,%d\r\n", i,
							 1);
			} else {
				error = hfp_ag_send_data(ag, NULL, NULL, "\r\n+BIND:%d,%d\r\n", i,
							 0);
			}
			if (error != 0) {
				return -EINVAL;
			}
			if (hf_indicators == 0) {
				break;
			}
		}
		return 0;
	}

	error = is_char(buf, '=');
	if (error != 0) {
		return error;
	}

	error = is_char(buf, '?');
	if (error == 0) {
		error = is_char(buf, '\r');
		if (error != 0) {
			return error;
		}

		data = &ag->buffer[0];
		*data = '(';
		data++;
		hf_indicators = ag->hf_indicators_of_ag;
		len = (sizeof(hf_indicators) * 8) > HFP_HF_IND_MAX ? HFP_HF_IND_MAX
								   : (sizeof(hf_indicators) * 8);
		for (int i = 1; (i < len) && (hf_indicators != 0); i++) {
			if (BIT(i) & hf_indicators) {
				int length = snprintk(
					data, (char *)&ag->buffer[HF_MAX_BUF_LEN - 1] - data - 3,
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
		*data = '\r';
		data++;
		*data = '\0';
		data++;

		error = hfp_ag_send_data(ag, NULL, NULL, "\r\n+BIND:%s\r\n", &ag->buffer[0]);
		return error;
	}

	error = 0;
	while (buf->len > 0) {
		error = get_number(buf, &number);

		if (error != 0) {
			return error;
		}

		error = is_char(buf, ',');
		if (error != 0) {
			if (0 != is_char(buf, '\r')) {
				return error;
			}
		}

		if (number < (sizeof(hf_indicators) * 8)) {
			hf_indicators |= BIT(number);
		}
	}

	ag->hf_indicators_of_hf = hf_indicators;

	return 0;
}

static int bt_hfp_ag_cmee_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err)
{
	uint32_t number;
	int error;

	*err = CME_ERROR_AG_FAILURE;

	error = is_char(buf, '=');
	if (error != 0) {
		return error;
	}

	error = get_number(buf, &number);

	if (error != 0) {
		return error;
	}

	error = is_char(buf, '\r');
	if (error != 0) {
		return error;
	}

	if (number == 1) {
		atomic_set_bit(ag->flags, BT_HFP_AG_CMEE_ENABLE);
	} else if (number == 0) {
		atomic_clear_bit(ag->flags, BT_HFP_AG_CMEE_ENABLE);
	} else {
		return -EINVAL;
	}
	return 0;
}

static int hfp_ag_update_indicator(struct bt_hfp_ag *ag, enum bt_hfp_ag_indicator index,
				   uint8_t value, bt_hfp_ag_tx_cb_t cb, void *user_data)
{
	int error;
	unsigned int key;
	uint8_t old_value;

	key = irq_lock();
	old_value = ag->indicator_value[index];
	if (value == old_value) {
		LOG_ERR("Duplicate value setting, indicator %d, old %d -> new %d", index, old_value,
			value);
		irq_unlock(key);
		return -EINVAL;
	}

	ag->indicator_value[index] = value;

	irq_unlock(key);

	LOG_DBG("indicator %d, old %d -> new %d", index, old_value, value);

	error = hfp_ag_send_data(ag, cb, user_data, "\r\n+CIEV:%d,%d\r\n", (uint8_t)index + 1,
				 value);
	if (error) {
		key = irq_lock();
		ag->indicator_value[index] = old_value;
		irq_unlock(key);
	}

	return error;
}

static void hfp_ag_close_sco(struct bt_hfp_ag *ag)
{
	LOG_DBG("");
	if (NULL != ag->sco_chan.sco) {
		bt_conn_disconnect(ag->sco_chan.sco, BT_HCI_ERR_LOCALHOST_TERM_CONN);
		ag->sco_chan.sco = NULL;
	}
}

static void bt_hfp_ag_reject_cb(struct bt_hfp_ag *ag, void *user_data)
{
	hfp_ag_close_sco(ag);
	bt_hfp_ag_set_call_state(ag, BT_HFP_CALL_TERMINATE);

	if (bt_ag && (bt_ag->reject)) {
		bt_ag->reject(ag);
	}
}

static void bt_hfp_ag_call_reject(struct bt_hfp_ag *ag, void *user_data)
{
	int error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_NONE,
					    bt_hfp_ag_reject_cb, user_data);
	if (error != 0) {
		LOG_ERR("Fail to send err :(%d)", error);
	}
}

static void bt_hfp_ag_terminate_cb(struct bt_hfp_ag *ag, void *user_data)
{
	hfp_ag_close_sco(ag);
	bt_hfp_ag_set_call_state(ag, BT_HFP_CALL_TERMINATE);

	if (bt_ag && (bt_ag->terminate)) {
		bt_ag->terminate(ag);
	}
}

static void bt_hfp_ag_unit_call_terminate(struct bt_hfp_ag *ag, void *user_data)
{
	int error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 0, bt_hfp_ag_terminate_cb,
					    user_data);
	if (error != 0) {
		LOG_ERR("Fail to send err :(%d)", error);
	}
}

static int bt_hfp_ag_chup_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err)
{
	int error;

	*err = CME_ERROR_AG_FAILURE;

	error = is_char(buf, '\r');
	if (error != 0) {
		return error;
	}

	if (ag->call_state == BT_HFP_CALL_ALERTING) {
		if (!atomic_test_bit(ag->flags, BT_HFP_AG_INCOMING_CALL)) {
			return -EINVAL;
		}
		error = hfp_ag_next_step(ag, bt_hfp_ag_call_reject, NULL);
	} else if ((ag->call_state == BT_HFP_CALL_ACTIVE) || (ag->call_state == BT_HFP_CALL_HOLD)) {
		error = hfp_ag_next_step(ag, bt_hfp_ag_unit_call_terminate, NULL);
	} else {
		return -EINVAL;
	}
	return error;
}

static uint8_t bt_hfp_get_call_state(struct bt_hfp_ag *ag)
{
	uint8_t status = 0;

	switch (ag->call_state) {
	case BT_HFP_CALL_TERMINATE:
		break;
	case BT_HFP_CALL_OUTGOING:
		status = 2;
		break;
	case BT_HFP_CALL_INCOMING:
		status = 4;
		break;
	case BT_HFP_CALL_ALERTING:
		if (atomic_test_bit(ag->flags, BT_HFP_AG_INCOMING_CALL)) {
			status = 5;
		} else {
			status = 3;
		}
		break;
	case BT_HFP_CALL_ACTIVE:
		status = 0;
		break;
	case BT_HFP_CALL_HOLD:
		status = 1;
		break;
	default:
		break;
	}

	return status;
}

static int bt_hfp_ag_clcc_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err)
{
	int error;
	uint8_t dir;
	uint8_t status;
	uint8_t mode;
	uint8_t mpty;

	*err = CME_ERROR_AG_FAILURE;

	error = is_char(buf, '\r');
	if (error != 0) {
		return error;
	}

	if (ag->call_state == BT_HFP_CALL_TERMINATE) {
		return 0;
	}

	dir = (atomic_test_bit(ag->flags, BT_HFP_AG_INCOMING_CALL)) ? 1 : 0;
	status = bt_hfp_get_call_state(ag);
	mode = 0;
	mpty = 0;
	error = hfp_ag_send_data(ag, NULL, NULL, "\r\n+CLCC:%d,%d,%d,%d,%d\r\n", 1, dir, status,
				 mode, mpty);
	if (error != 0) {
		LOG_ERR("Fail to send err :(%d)", error);
	}

	return error;
}

static int bt_hfp_ag_bia_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err)
{
	uint32_t number;
	int error;
	int index = 0;
	uint32_t indicator = ag->indicator;

	*err = CME_ERROR_AG_FAILURE;

	error = is_char(buf, '=');
	if (error != 0) {
		return error;
	}

	while (buf->len > 0) {
		error = get_number(buf, &number);

		if (error == 0) {
			/* Valid number */
			if (number) {
				indicator |= BIT(index);
			} else {
				indicator &= ~BIT(index);
			}
		}

		error = is_char(buf, ',');
		if (error == 0) {
			index++;
		} else {
			error = is_char(buf, '\r');
			if (error != 0) {
				return error;
			}
			if (buf->len != 0) {
				return -EINVAL;
			}
		}
	}

	/* Force call, call setup and held call indicators are enabled. */
	indicator = BIT(BT_HFP_AG_CALL_IND) | BIT(BT_HFP_AG_CALL_SETUP_IND) |
		    BIT(BT_HFP_AG_CALL_HELD_IND);
	ag->indicator = indicator;

	return 0;
}

static void bt_hfp_ag_accept_cb(struct bt_hfp_ag *ag, void *user_data)
{
	bt_hfp_ag_set_call_state(ag, BT_HFP_CALL_ACTIVE);

	if (bt_ag && (bt_ag->accept)) {
		bt_ag->accept(ag);
	}
}

static int hfp_ag_open_sco(struct bt_hfp_ag *ag)
{
	if (ag->sco_chan.sco == NULL) {
		struct bt_conn *sco_conn = bt_hfp_ag_create_sco(ag);

		if (NULL == sco_conn) {
			LOG_ERR("Fail to create sco connection!");
			return -ENOTCONN;
		}

		LOG_DBG("SCO connection created (%p)", sco_conn);
	}

	return 0;
}

static int bt_hfp_ag_codec_select(struct bt_hfp_ag *ag)
{
	int error;

	if (ag->selected_codec_id == 0) {
		LOG_ERR("Codec is invalid");
		return -EINVAL;
	}

	if (!(ag->hf_codec_ids & BIT(ag->selected_codec_id))) {
		LOG_ERR("Codec is unsupported (codec id %d)", ag->selected_codec_id);
		return -EINVAL;
	}

	error = hfp_ag_send_data(ag, NULL, NULL, "\r\n+BCS:%d\r\n", ag->selected_codec_id);
	if (error != 0) {
		LOG_ERR("Fail to send err :(%d)", error);
	}
	return error;
}

static int bt_hfp_ag_create_audio_connection(struct bt_hfp_ag *ag)
{
	int error;

	if ((ag->hf_codec_ids != 0) && atomic_test_bit(ag->flags, BT_HFP_AG_CODEC_CHANGED)) {
		atomic_set_bit(ag->flags, BT_HFP_AG_CODEC_CONN);
		error = bt_hfp_ag_codec_select(ag);
	} else {
		error = hfp_ag_open_sco(ag);
	}

	return error;
}

static void bt_hfp_ag_audio_connection(struct bt_hfp_ag *ag, void *user_data)
{
	int error;

	error = bt_hfp_ag_create_audio_connection(ag);
	if (error) {
		bt_hfp_ag_unit_call_terminate(ag, user_data);
	}
}

static void bt_hfp_ag_unit_call_accept(struct bt_hfp_ag *ag, void *user_data)
{
	int error;

	error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 1, bt_hfp_ag_accept_cb, user_data);
	if (error != 0) {
		LOG_ERR("Fail to send err :(%d)", error);
	}

	error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_NONE,
					bt_hfp_ag_audio_connection, user_data);
	if (error != 0) {
		LOG_ERR("Fail to send err :(%d)", error);
	}
}

static int bt_hfp_ag_ata_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err)
{
	int error;

	*err = CME_ERROR_AG_FAILURE;

	error = is_char(buf, '\r');
	if (error != 0) {
		return error;
	}

	if (ag->call_state != BT_HFP_CALL_ALERTING) {
		return -EINVAL;
	}

	if (!atomic_test_bit(ag->flags, BT_HFP_AG_INCOMING_CALL)) {
		return -EINVAL;
	}

	error = hfp_ag_next_step(ag, bt_hfp_ag_unit_call_accept, NULL);

	return error;
}

static int bt_hfp_ag_cops_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err)
{
	int error;
	uint32_t number;

	*err = CME_ERROR_AG_FAILURE;

	error = is_char(buf, '=');
	if (error == 0) {
		static const uint32_t command_line_prefix[] = {3, 0};

		for (int i = 0; i < ARRAY_SIZE(command_line_prefix); i++) {
			error = get_number(buf, &number);
			if (error != 0) {
				return error;
			}

			if (command_line_prefix[i] != number) {
				return -EINVAL;
			}

			error = is_char(buf, ',');
			if (error != 0) {
				error = is_char(buf, '\r');
				if (error != 0) {
					return error;
				}
			}
		}

		atomic_set_bit(ag->flags, BT_HFP_AG_COPS_SET);
		return 0;
	}

	if (!atomic_test_bit(ag->flags, BT_HFP_AG_COPS_SET)) {
		return -EINVAL;
	}

	error = is_char(buf, '?');
	if (error != 0) {
		return error;
	}

	error = is_char(buf, '\r');
	if (error != 0) {
		return error;
	}

	error = hfp_ag_send_data(ag, NULL, NULL, "\r\n+COPS:%d,%d,\"%s\"\r\n", 0, 0, ag->operator);
	if (error != 0) {
		LOG_ERR("Fail to send err :(%d)", error);
	}

	return error;
}

static int bt_hfp_ag_bcc_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err)
{
	int error;

	*err = CME_ERROR_AG_FAILURE;

	error = is_char(buf, '\r');
	if (error != 0) {
		return error;
	}

	if (ag->selected_codec_id == 0) {
		return -EINVAL;
	}

	if (!(ag->hf_codec_ids & BIT(ag->selected_codec_id))) {
		return -EINVAL;
	}

	if (ag->call_state == BT_HFP_CALL_TERMINATE) {
		return -EINVAL;
	}

	if (ag->sco_chan.sco != NULL) {
		return -EINVAL;
	}

	error = hfp_ag_next_step(ag, bt_hfp_ag_audio_connection, NULL);

	return 0;
}

static void bt_hfp_ag_unit_codec_conn_setup(struct bt_hfp_ag *ag, void *user_data)
{
	int error = hfp_ag_open_sco(ag);

	if (error) {
		bt_hfp_ag_call_reject(ag, user_data);
	}
}

static int bt_hfp_ag_bcs_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err)
{
	int error;
	uint32_t number;

	*err = CME_ERROR_AG_FAILURE;

	error = is_char(buf, '=');
	if (error != 0) {
		return error;
	}

	error = get_number(buf, &number);
	if (error != 0) {
		return error;
	}

	error = is_char(buf, '\r');
	if (error != 0) {
		return error;
	}

	if (!atomic_test_bit(ag->flags, BT_HFP_AG_CODEC_CONN)) {
		return -EINVAL;
	}

	if ((ag->selected_codec_id != number) ||
	    (!(ag->hf_codec_ids & BIT(ag->selected_codec_id)))) {
		error = -EINVAL;
	}

	if (error == 0) {
		atomic_clear_bit(ag->flags, BT_HFP_AG_CODEC_CONN);
		atomic_clear_bit(ag->flags, BT_HFP_AG_CODEC_CHANGED);
		error = hfp_ag_next_step(ag, bt_hfp_ag_unit_codec_conn_setup, NULL);
	} else {
		if (ag->call_state != BT_HFP_CALL_TERMINATE) {
			(void)hfp_ag_next_step(ag, bt_hfp_ag_unit_call_terminate, NULL);
		}
	}

	return error;
}

static void bt_hfp_ag_unit_call_outgoing(struct bt_hfp_ag *ag, void *user_data)
{
	int error = bt_hfp_ag_outgoing(ag, ag->number);

	if (error != 0) {
		LOG_ERR("Fail to send err :(%d)", error);
	}
}

static int bt_hfp_ag_atd_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err)
{
	int error;
	char *number = NULL;
	bool is_memory_dial = false;

	*err = CME_ERROR_AG_FAILURE;

	if (buf->data[buf->len - 1] != '\r') {
		*err = CME_ERROR_OPERATION_NOT_ALLOWED;
		return -EINVAL;
	}

	error = is_char(buf, '>');
	if (error == 0) {
		is_memory_dial = true;
	}

	if ((buf->len - 1) > CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN) {
		*err = CME_ERROR_DIAL_STRING_TO_LONG;
		return -EINVAL;
	}

	buf->data[buf->len - 1] = '\0';

	if (is_memory_dial) {
		if (bt_ag && bt_ag->memory_dial) {
			error = bt_ag->memory_dial(ag, &buf->data[0], &number);
			if ((error != 0) || (number == NULL)) {
				*err = CME_ERROR_INVALID_INDEX;
				return -EINVAL;
			}
		}
	} else {
		number = &buf->data[0];
	}

	if (strlen(number) == 0) {
		return -EINVAL;
	}

	if (strlen(number) > CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN) {
		*err = CME_ERROR_DIAL_STRING_TO_LONG;
		return -EINVAL;
	}

	if (ag->call_state != BT_HFP_CALL_TERMINATE) {
		return -EBUSY;
	}

	memcpy(ag->number, number, strlen(number));
	ag->number[strlen(number)] = '\0';

	error = hfp_ag_next_step(ag, bt_hfp_ag_unit_call_outgoing, NULL);

	return error;
}

static int bt_hfp_ag_bldn_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err)
{
	int error;

	*err = CME_ERROR_AG_FAILURE;

	error = is_char(buf, '\r');
	if (error != 0) {
		return -EINVAL;
	}

	if (strlen(ag->number) == 0) {
		*err = CME_ERROR_MEMORY_FAILURE;
		return -EINVAL;
	}

	if (ag->call_state != BT_HFP_CALL_TERMINATE) {
		return -EBUSY;
	}

	error = hfp_ag_next_step(ag, bt_hfp_ag_unit_call_outgoing, NULL);

	return error;
}

static int bt_hfp_ag_clip_handler(struct bt_hfp_ag *ag, struct net_buf *buf, enum at_cme *err)
{
	int error;
	uint32_t number;

	*err = CME_ERROR_AG_FAILURE;

	error = get_number(buf, &number);
	if (error != 0) {
		return error;
	}

	error = is_char(buf, '\r');
	if (error != 0) {
		return error;
	}

	if (number == 0) {
		atomic_clear_bit(ag->flags, BT_HFP_AG_CLIP_ENABLE);
	} else {
		atomic_set_bit(ag->flags, BT_HFP_AG_CLIP_ENABLE);
	}

	return error;
}

static void hfp_ag_connected(struct bt_rfcomm_dlc *dlc)
{
	struct bt_hfp_ag *ag = CONTAINER_OF(dlc, struct bt_hfp_ag, rfcomm_dlc);

	bt_hfp_ag_set_state(ag, BT_HFP_CONFIG);

	LOG_DBG("AG %p", ag);
}

static void hfp_ag_disconnected(struct bt_rfcomm_dlc *dlc)
{
	struct bt_hfp_ag *ag = CONTAINER_OF(dlc, struct bt_hfp_ag, rfcomm_dlc);

	bt_hfp_ag_set_state(ag, BT_HFP_DISCONNECTED);

	if ((ag->call_state == BT_HFP_CALL_ALERTING) || (ag->call_state == BT_HFP_CALL_ACTIVE) ||
	    (ag->call_state == BT_HFP_CALL_HOLD)) {
		bt_hfp_ag_terminate_cb(ag, NULL);
	}

	LOG_DBG("AG %p", ag);
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
	struct bt_ag_tx *tx = NULL;

	tx = bt_ag_tx_alloc();

	if (tx == NULL) {
		LOG_ERR("No Buffers!");
		return -ENOMEM;
	}

	tx->ag = ag;
	tx->buf = NULL;
	tx->cb = cb;
	tx->user_data = user_data;

	k_fifo_put(&ag_tx_notify, tx);

	return 0;
}

static int hfp_ag_send(struct bt_hfp_ag *ag, struct bt_ag_tx *tx)
{
	return bt_rfcomm_dlc_send(&ag->rfcomm_dlc, tx->buf);
}

static int hfp_ag_send_data(struct bt_hfp_ag *ag, bt_hfp_ag_tx_cb_t cb, void *user_data,
			    const char *format, ...)
{
	struct net_buf *buf;
	struct bt_ag_tx *tx = NULL;
	va_list vargs;
	int ret;
	unsigned int key;

	LOG_DBG("AG %p sending data cb %p user_data %p", ag, cb, user_data);

	buf = bt_rfcomm_create_pdu(&ag_pool);
	if (!buf) {
		LOG_ERR("No Buffers!");
		return -ENOMEM;
	}

	tx = bt_ag_tx_alloc();

	if (tx == NULL) {
		LOG_ERR("No Buffers!");
		net_buf_unref(buf);
		return -ENOMEM;
	}

	LOG_DBG("buf %p tx %p", buf, tx);

	tx->ag = ag;
	tx->buf = buf;
	tx->cb = cb;
	tx->user_data = user_data;

	va_start(vargs, format);
	ret = vsnprintk((char *)buf->data, (net_buf_tailroom(buf) - 1), format, vargs);
	va_end(vargs);

	if (ret < 0) {
		LOG_ERR("Unable to format variable arguments");
		net_buf_unref(buf);
		bt_ag_tx_free(tx);
		return ret;
	}

	net_buf_add(buf, ret);

	key = irq_lock();
	sys_slist_append(&ag->tx_pending, &tx->node);
	irq_unlock(key);

	ret = hfp_ag_send(ag, tx);

	if (ret < 0) {
		LOG_ERR("Rfcomm send error :(%d)", ret);
		key = irq_lock();
		sys_slist_find_and_remove(&ag->tx_pending, &tx->node);
		irq_unlock(key);
		net_buf_unref(buf);
		bt_ag_tx_free(tx);
		return ret;
	}

	return 0;
}

static void hfp_ag_recv(struct bt_rfcomm_dlc *dlc, struct net_buf *buf)
{
	struct bt_hfp_ag *ag = CONTAINER_OF(dlc, struct bt_hfp_ag, rfcomm_dlc);
	uint8_t *data = buf->data;
	uint16_t len = buf->len;
	enum at_cme err = CME_ERROR_OPERATION_NOT_SUPPORTED;
	int ret = -EOPNOTSUPP;

	LOG_DBG("%p %d", data, len);

	for (uint32_t index = 0; index < ARRAY_SIZE(cmd_handlers); index++) {
		if (strlen(cmd_handlers[index].cmd) > len) {
			continue;
		}
		if (0 != strncmp((char *)data, cmd_handlers[index].cmd,
				 strlen(cmd_handlers[index].cmd))) {
			continue;
		}
		if (NULL != cmd_handlers[index].handler) {
			(void)net_buf_pull(buf, strlen(cmd_handlers[index].cmd));
			ret = cmd_handlers[index].handler(ag, buf, &err);
			break;
		}
	}

	if ((ret != 0) && atomic_test_bit(ag->flags, BT_HFP_AG_CMEE_ENABLE)) {
		ret = hfp_ag_send_data(ag, NULL, NULL, "\r\n+CME ERROR:%d\r\n", (uint32_t)err);
	} else {
		ret = hfp_ag_send_data(ag, NULL, NULL, "\r\n%s\r\n", (0 == ret) ? "OK" : "ERROR");
	}

	if (ret != 0) {
		LOG_ERR("HFP AG send response err :(%d)", ret);
	}
}

static void bt_hfp_ag_thread(void *p1, void *p2, void *p3)
{
	struct bt_ag_tx *tx;
	bt_hfp_ag_tx_cb_t cb;
	struct bt_hfp_ag *ag;
	void *user_data;

	while (true) {
		tx = (struct bt_ag_tx *)k_fifo_get(&ag_tx_notify, K_FOREVER);

		if (tx == NULL) {
			continue;
		}

		cb = tx->cb;
		ag = tx->ag;
		user_data = tx->user_data;

		bt_ag_tx_free(tx);

		if (cb) {
			cb(ag, user_data);
		}
	}
}

static void hfp_ag_sent(struct bt_rfcomm_dlc *dlc, struct net_buf *buf, int err)
{
	struct bt_hfp_ag *ag = CONTAINER_OF(dlc, struct bt_hfp_ag, rfcomm_dlc);
	struct bt_ag_tx *t, *tmp, *tx = NULL;
	unsigned int key;
	bool ret;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&ag->tx_pending, t, tmp, node) {
		if (t->buf == buf) {
			tx = t;
			break;
		}
	}

	if (tx == NULL) {
		LOG_ERR("Node %p is not found", tx);
		return;
	}

	key = irq_lock();
	ret = sys_slist_find_and_remove(&ag->tx_pending, &tx->node);
	irq_unlock(key);

	if (ret == false) {
		LOG_ERR("Node %p is not found", tx);
	}

	k_fifo_put(&ag_tx_notify, tx);
}

static void bt_ag_deferred_work_cb(struct bt_hfp_ag *ag, void *user_data)
{
	int error;

	switch (ag->call_state) {
	case BT_HFP_CALL_TERMINATE:
		break;
	case BT_HFP_CALL_ACTIVE:
		break;
	case BT_HFP_CALL_HOLD:
		break;
	case BT_HFP_CALL_OUTGOING:
		__fallthrough;
	case BT_HFP_CALL_INCOMING:
		__fallthrough;
	case BT_HFP_CALL_ALERTING:
		__fallthrough;
	default:
		if (ag->indicator_value[BT_HFP_AG_CALL_SETUP_IND] &&
		    ag->indicator_value[BT_HFP_AG_CALL_IND]) {
			error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND,
							BT_HFP_CALL_SETUP_NONE, NULL, NULL);
			if (error) {
				LOG_ERR("Fail to send indicator");
				bt_hfp_ag_terminate_cb(ag, NULL);
			} else {
				error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 0,
								bt_hfp_ag_terminate_cb, NULL);
				if (error) {
					LOG_ERR("Fail to send indicator");
					bt_hfp_ag_terminate_cb(ag, NULL);
				}
			}
		} else if (ag->indicator_value[BT_HFP_AG_CALL_SETUP_IND]) {
			error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND,
							BT_HFP_CALL_SETUP_NONE, bt_hfp_ag_reject_cb,
							NULL);
			if (error) {
				LOG_ERR("Fail to send indicator");
				bt_hfp_ag_terminate_cb(ag, NULL);
			}
		} else if (ag->indicator_value[BT_HFP_AG_CALL_IND]) {
			error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 0,
							bt_hfp_ag_terminate_cb, NULL);
			if (error) {
				LOG_ERR("Fail to send indicator");
				bt_hfp_ag_terminate_cb(ag, NULL);
			}
		}
		break;
	}
}

static void bt_ag_deferred_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_hfp_ag *ag = CONTAINER_OF(dwork, struct bt_hfp_ag, deferred_work);

	(void)hfp_ag_next_step(ag, bt_ag_deferred_work_cb, NULL);
}

static void bt_ag_ringing_work_cb(struct bt_hfp_ag *ag, void *user_data)
{
	int error;

	if (ag->call_state == BT_HFP_CALL_ALERTING) {

		if (!atomic_test_bit(ag->flags, BT_HFP_AG_INCOMING_CALL)) {
			return;
		}

		k_work_reschedule(&ag->ringing_work,
				  K_SECONDS(CONFIG_BT_HFP_AG_RING_NOTIFY_INTERVAL));

		error = hfp_ag_send_data(ag, NULL, NULL, "\r\nRING\r\n");
		if (error) {
			LOG_ERR("Fail to send RING %d", error);
		} else {
			if (atomic_test_bit(ag->flags, BT_HFP_AG_CLIP_ENABLE)) {
				error = hfp_ag_send_data(ag, NULL, NULL, "\r\n+CLIP:\"%s\",%d\r\n",
							 ag->number, 0);
				if (error) {
					LOG_ERR("Fail to send CLIP %d", error);
				}
			}
		}
	}
}

static void bt_ag_ringing_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_hfp_ag *ag = CONTAINER_OF(dwork, struct bt_hfp_ag, ringing_work);

	(void)hfp_ag_next_step(ag, bt_ag_ringing_work_cb, NULL);
}

int bt_hfp_ag_connect(struct bt_conn *conn, struct bt_hfp_ag **ag, uint8_t channel)
{
	int i;
	int ret;

	static struct bt_rfcomm_dlc_ops ops = {
		.connected = hfp_ag_connected,
		.disconnected = hfp_ag_disconnected,
		.recv = hfp_ag_recv,
		.sent = hfp_ag_sent,
	};

	if (ag == NULL) {
		return -EINVAL;
	}

	*ag = NULL;

	if (ag_thread_id == NULL) {

		k_fifo_init(&ag_tx_free);
		k_fifo_init(&ag_tx_notify);

		for (i = 0; i < ARRAY_SIZE(ag_tx); i++) {
			k_fifo_put(&ag_tx_free, &ag_tx[i]);
		}

		ag_thread_id = k_thread_create(
			&ag_thread, ag_thread_stack, K_KERNEL_STACK_SIZEOF(ag_thread_stack),
			bt_hfp_ag_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_HFP_AG_THREAD_PRIO), 0, K_NO_WAIT);
		if (NULL == ag_thread_id) {
			return -ENOMEM;
		}
		k_thread_name_set(ag_thread_id, "HFP AG");
	}

	for (i = 0; i < ARRAY_SIZE(bt_hfp_ag_pool); i++) {
		struct bt_hfp_ag *_ag = &bt_hfp_ag_pool[i];

		if (_ag->rfcomm_dlc.session) {
			continue;
		}

		(void)memset(_ag, 0, sizeof(struct bt_hfp_ag));

		sys_slist_init(&_ag->tx_pending);

		_ag->rfcomm_dlc.ops = &ops;
		_ag->rfcomm_dlc.mtu = BT_HFP_MAX_MTU;

		/* Set the supported features*/
		_ag->ag_features = BT_HFP_AG_SUPPORTED_FEATURES;

		/* If supported codec ids cannot be notified, disable codec negotiation. */
		if (!(bt_ag && bt_ag->codec)) {
			_ag->ag_features &= ~BT_HFP_AG_FEATURE_CODEC_NEG;
		}

		_ag->hf_features = 0;
		_ag->hf_codec_ids = 0;

		_ag->acl_conn = conn;

		/* Set AG indicator value */
		_ag->indicator_value[BT_HFP_AG_SERVICE_IND] = 0;
		_ag->indicator_value[BT_HFP_AG_CALL_IND] = 0;
		_ag->indicator_value[BT_HFP_AG_CALL_SETUP_IND] = 0;
		_ag->indicator_value[BT_HFP_AG_CALL_HELD_IND] = 0;
		_ag->indicator_value[BT_HFP_AG_SINGNAL_IND] = 0;
		_ag->indicator_value[BT_HFP_AG_ROAM_IND] = 0;
		_ag->indicator_value[BT_HFP_AG_BATTERY_IND] = 0;

		/* Set AG indicator status */
		_ag->indicator = BIT(BT_HFP_AG_SERVICE_IND) | BIT(BT_HFP_AG_CALL_IND) |
				 BIT(BT_HFP_AG_CALL_SETUP_IND) | BIT(BT_HFP_AG_CALL_HELD_IND) |
				 BIT(BT_HFP_AG_SINGNAL_IND) | BIT(BT_HFP_AG_ROAM_IND) |
				 BIT(BT_HFP_AG_BATTERY_IND);

		/* Set AG operator */
		memcpy(_ag->operator, "UNKNOWN", sizeof("UNKNOWN"));

		/* Set Codec ID*/
		_ag->selected_codec_id = BT_HFP_AG_CODEC_CVSD;

		/* Init delay work */
		k_work_init_delayable(&_ag->deferred_work, bt_ag_deferred_work);

		k_work_init_delayable(&_ag->ringing_work, bt_ag_ringing_work);

		atomic_set_bit(_ag->flags, BT_HFP_AG_CODEC_CHANGED);

		*ag = _ag;
	}

	if (*ag == NULL) {
		return -ENOMEM;
	}

	ret = bt_rfcomm_dlc_connect(conn, &(*ag)->rfcomm_dlc, channel);

	if (ret != 0) {
		(void)memset(*ag, 0, sizeof(struct bt_hfp_ag));
		*ag = NULL;
	} else {
		bt_hfp_ag_set_state(*ag, BT_HFP_CONNECTING);
	}

	return ret;
}

int bt_hfp_ag_disconnect(struct bt_hfp_ag *ag)
{
	int ret;

	if (ag == NULL) {
		return -EINVAL;
	}

	bt_hfp_ag_set_state(ag, BT_HFP_DISCONNECTING);

	if ((ag->call_state == BT_HFP_CALL_ACTIVE) || (ag->call_state == BT_HFP_CALL_HOLD)) {
		ret = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 0, bt_hfp_ag_terminate_cb,
					      NULL);
		if (ret != 0) {
			LOG_ERR("HFP AG send response err :(%d)", ret);
		}
		return ret;
	} else if (ag->call_state != BT_HFP_CALL_TERMINATE) {
		ret = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_NONE,
					      bt_hfp_ag_reject_cb, NULL);
		if (ret != 0) {
			LOG_ERR("HFP AG send response err :(%d)", ret);
		}
		return ret;
	}

	return bt_rfcomm_dlc_disconnect(&ag->rfcomm_dlc);
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

	return 0;
}

static void bt_hfp_ag_call_ringing_cb(struct bt_hfp_ag *ag, bool in_bond)
{
	if (bt_ag && (bt_ag->ringing)) {
		bt_ag->ringing(ag, in_bond);
	}
}

static void hfp_ag_sco_connected(struct bt_sco_chan *chan)
{
	struct bt_hfp_ag *ag = CONTAINER_OF(chan, struct bt_hfp_ag, sco_chan);

	if (ag->call_state == BT_HFP_CALL_INCOMING) {
		bt_hfp_ag_set_call_state(ag, BT_HFP_CALL_ALERTING);
		bt_hfp_ag_call_ringing_cb(ag, true);
	}

	if ((bt_ag) && (bt_ag->sco_connected)) {
		bt_ag->sco_connected(ag, chan->sco);
	}
}

static void hfp_ag_sco_disconnected(struct bt_sco_chan *chan, uint8_t reason)
{
	struct bt_hfp_ag *ag = CONTAINER_OF(chan, struct bt_hfp_ag, sco_chan);

	if ((bt_ag) && (bt_ag->sco_disconnected)) {
		bt_ag->sco_disconnected(ag);
	}

	if ((ag->call_state == BT_HFP_CALL_INCOMING) || (ag->call_state == BT_HFP_CALL_OUTGOING)) {
		bt_hfp_ag_call_reject(ag, NULL);
	}
}

static struct bt_conn *bt_hfp_ag_create_sco(struct bt_hfp_ag *ag)
{
	struct bt_conn *sco_conn;

	static struct bt_sco_chan_ops ops = {
		.connected = hfp_ag_sco_connected,
		.disconnected = hfp_ag_sco_disconnected,
	};

	if (NULL == ag->sco_chan.sco) {
		ag->sco_chan.ops = &ops;

		/* create SCO connection*/
		sco_conn = bt_conn_create_sco(&ag->acl_conn->br.dst, &ag->sco_chan);
		if (sco_conn != NULL) {
			bt_conn_unref(sco_conn);
		}
	} else {
		sco_conn = ag->sco_chan.sco;
	}

	return sco_conn;
}

static void bt_hfp_ag_incoming_cb(struct bt_hfp_ag *ag, void *user_data)
{
	bt_hfp_ag_set_call_state(ag, BT_HFP_CALL_INCOMING);

	if (bt_ag && (bt_ag->incoming)) {
		bt_ag->incoming(ag, ag->number);
	}

	if (atomic_test_bit(ag->flags, BT_HFP_AG_INBAND_RING)) {
		int error;

		error = bt_hfp_ag_create_audio_connection(ag);
		if (error) {
			bt_hfp_ag_call_reject(ag, NULL);
		}
	} else {
		bt_hfp_ag_set_call_state(ag, BT_HFP_CALL_ALERTING);
		bt_hfp_ag_call_ringing_cb(ag, false);
	}
}

int bt_hfp_ag_remote_incoming(struct bt_hfp_ag *ag, const char *number)
{
	int error = 0;

	if (ag == NULL) {
		return -EINVAL;
	}

	if (ag->state != BT_HFP_CONNECTED) {
		return -ENOTCONN;
	}

	if (ag->call_state != BT_HFP_CALL_TERMINATE) {
		return -EBUSY;
	}

	if ((strlen(number) == 0) || (strlen(number) > CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN)) {
		return -EINVAL;
	}

	memcpy(ag->number, number, strlen(number));
	ag->number[strlen(number)] = '\0';

	atomic_set_bit(ag->flags, BT_HFP_AG_INCOMING_CALL);

	error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_INCOMING,
					bt_hfp_ag_incoming_cb, NULL);
	;
	if (error != 0) {
		atomic_clear_bit(ag->flags, BT_HFP_AG_INCOMING_CALL);
	}

	return error;
}

int bt_hfp_ag_reject(struct bt_hfp_ag *ag)
{
	int error;

	if (ag == NULL) {
		return -EINVAL;
	}

	if (ag->state != BT_HFP_CONNECTED) {
		return -ENOTCONN;
	}

	if ((ag->call_state != BT_HFP_CALL_ALERTING) && (ag->call_state != BT_HFP_CALL_INCOMING)) {
		return -EINVAL;
	}

	if (!atomic_test_bit(ag->flags, BT_HFP_AG_INCOMING_CALL)) {
		return -EINVAL;
	}

	error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_NONE,
					bt_hfp_ag_reject_cb, NULL);

	return error;
}

int bt_hfp_ag_accept(struct bt_hfp_ag *ag)
{
	int error;

	if (ag == NULL) {
		return -EINVAL;
	}

	if (ag->state != BT_HFP_CONNECTED) {
		return -ENOTCONN;
	}

	if (ag->call_state != BT_HFP_CALL_ALERTING) {
		return -EINVAL;
	}

	if (!atomic_test_bit(ag->flags, BT_HFP_AG_INCOMING_CALL)) {
		return -EINVAL;
	}

	error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 1, bt_hfp_ag_accept_cb, NULL);
	if (error != 0) {
		LOG_ERR("Fail to send err :(%d)", error);
	}

	error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_NONE,
					bt_hfp_ag_audio_connection, NULL);
	if (error != 0) {
		LOG_ERR("Fail to send err :(%d)", error);
	}

	return error;
}

int bt_hfp_ag_terminate(struct bt_hfp_ag *ag)
{
	int error;

	if (ag == NULL) {
		return -EINVAL;
	}

	if (ag->state != BT_HFP_CONNECTED) {
		return -ENOTCONN;
	}

	if ((ag->call_state != BT_HFP_CALL_ACTIVE) || (ag->call_state != BT_HFP_CALL_HOLD)) {
		return -EINVAL;
	}

	error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 0, bt_hfp_ag_terminate_cb, NULL);

	return error;
}

static void bt_hfp_ag_outgoing_cb(struct bt_hfp_ag *ag, void *user_data)
{
	bt_hfp_ag_set_call_state(ag, BT_HFP_CALL_OUTGOING);

	if (bt_ag && (bt_ag->outgoing)) {
		bt_ag->outgoing(ag, ag->number);
	}

	if (atomic_test_bit(ag->flags, BT_HFP_AG_INBAND_RING)) {
		int error;

		error = bt_hfp_ag_create_audio_connection(ag);
		if (error) {
			bt_hfp_ag_call_reject(ag, NULL);
		}
	}
}

int bt_hfp_ag_outgoing(struct bt_hfp_ag *ag, const char *number)
{
	int error = 0;

	if (ag == NULL) {
		return -EINVAL;
	}

	if (ag->state != BT_HFP_CONNECTED) {
		return -ENOTCONN;
	}

	if (ag->call_state != BT_HFP_CALL_TERMINATE) {
		return -EBUSY;
	}

	if ((strlen(number) == 0) || (strlen(number) > CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN)) {
		return -EINVAL;
	}

	memcpy(ag->number, number, strlen(number));
	ag->number[strlen(number)] = '\0';

	atomic_clear_bit(ag->flags, BT_HFP_AG_INCOMING_CALL);

	error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_OUTGOING,
					bt_hfp_ag_outgoing_cb, NULL);

	return error;
}

static void bt_hfp_ag_ringing_cb(struct bt_hfp_ag *ag, void *user_data)
{
	bt_hfp_ag_set_call_state(ag, BT_HFP_CALL_ALERTING);

	if (atomic_test_bit(ag->flags, BT_HFP_AG_INBAND_RING)) {
		bt_hfp_ag_call_ringing_cb(ag, true);
	} else {
		bt_hfp_ag_call_ringing_cb(ag, false);
	}
}

int bt_hfp_ag_remote_ringing(struct bt_hfp_ag *ag)
{
	int error = 0;

	if (ag == NULL) {
		return -EINVAL;
	}

	if (ag->state != BT_HFP_CONNECTED) {
		return -ENOTCONN;
	}

	if (BT_HFP_CALL_OUTGOING != ag->call_state) {
		return -EBUSY;
	}

	if (atomic_test_bit(ag->flags, BT_HFP_AG_INBAND_RING)) {
		if (ag->sco_chan.sco == NULL) {
			return -ENOTCONN;
		}
	}

	error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND,
					BT_HFP_CALL_SETUP_REMOTE_ALERTING, bt_hfp_ag_ringing_cb,
					NULL);

	return error;
}

int bt_hfp_ag_remote_reject(struct bt_hfp_ag *ag)
{
	int error;

	if (ag == NULL) {
		return -EINVAL;
	}

	if (ag->state != BT_HFP_CONNECTED) {
		return -ENOTCONN;
	}

	if ((ag->call_state != BT_HFP_CALL_ALERTING) && (ag->call_state != BT_HFP_CALL_OUTGOING)) {
		return -EINVAL;
	}

	if (atomic_test_bit(ag->flags, BT_HFP_AG_INCOMING_CALL)) {
		return -EINVAL;
	}

	error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_NONE,
					bt_hfp_ag_reject_cb, NULL);

	return error;
}

int bt_hfp_ag_remote_accept(struct bt_hfp_ag *ag)
{
	int error;

	if (ag == NULL) {
		return -EINVAL;
	}

	if (ag->state != BT_HFP_CONNECTED) {
		return -ENOTCONN;
	}

	if (ag->call_state != BT_HFP_CALL_ALERTING) {
		return -EINVAL;
	}

	if (atomic_test_bit(ag->flags, BT_HFP_AG_INCOMING_CALL)) {
		return -EINVAL;
	}

	error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 1, bt_hfp_ag_accept_cb, NULL);
	if (error != 0) {
		LOG_ERR("Fail to send err :(%d)", error);
	}

	error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_SETUP_IND, BT_HFP_CALL_SETUP_NONE,
					bt_hfp_ag_audio_connection, NULL);
	if (error != 0) {
		LOG_ERR("Fail to send err :(%d)", error);
	}

	return error;
}

int bt_hfp_ag_remote_terminate(struct bt_hfp_ag *ag)
{
	int error;

	if (ag == NULL) {
		return -EINVAL;
	}

	if (ag->state != BT_HFP_CONNECTED) {
		return -ENOTCONN;
	}

	if ((ag->call_state != BT_HFP_CALL_ACTIVE) || (ag->call_state != BT_HFP_CALL_HOLD)) {
		return -EINVAL;
	}

	error = hfp_ag_update_indicator(ag, BT_HFP_AG_CALL_IND, 0, bt_hfp_ag_terminate_cb, NULL);

	return error;
}

int bt_hfp_ag_set_indicator(struct bt_hfp_ag *ag, enum bt_hfp_ag_indicator index, uint8_t value)
{
	int error;

	if (ag == NULL) {
		return -EINVAL;
	}

	if (ag->state != BT_HFP_CONNECTED) {
		return -ENOTCONN;
	}

	switch (index) {
	case BT_HFP_AG_SERVICE_IND:
		__fallthrough;
	case BT_HFP_AG_SINGNAL_IND:
		__fallthrough;
	case BT_HFP_AG_ROAM_IND:
		__fallthrough;
	case BT_HFP_AG_BATTERY_IND:
		if ((ag_ind[(uint8_t)index].min > value) || (ag_ind[(uint8_t)index].max < value)) {
			return -EINVAL;
		}
		break;
	case BT_HFP_AG_CALL_IND:
		__fallthrough;
	case BT_HFP_AG_CALL_SETUP_IND:
		__fallthrough;
	case BT_HFP_AG_CALL_HELD_IND:
		__fallthrough;
	default:
		return -EINVAL;
	}

	error = hfp_ag_update_indicator(ag, index, value, NULL, NULL);

	return error;
}

int bt_hfp_ag_set_operator(struct bt_hfp_ag *ag, char *name)
{
	int len;

	if (ag == NULL) {
		return -EINVAL;
	}

	if (ag->state != BT_HFP_CONNECTED) {
		return -ENOTCONN;
	}

	len = strlen(name) > AT_COPS_OPERATOR_MAX_LEN ? AT_COPS_OPERATOR_MAX_LEN : strlen(name);
	memcpy(ag->operator, name, len);
	ag->operator[len] = '\0';

	return 0;
}

int bt_hfp_ag_select_codec(struct bt_hfp_ag *ag, uint8_t id)
{
	int error = 0;

	if (ag == NULL) {
		return -EINVAL;
	}

	if (ag->state != BT_HFP_CONNECTED) {
		return -ENOTCONN;
	}

	if (!(ag->hf_codec_ids && BIT(id))) {
		return -ENOTSUP;
	}

	ag->selected_codec_id = id;
	atomic_set_bit(ag->flags, BT_HFP_AG_CODEC_CHANGED);

	if (atomic_test_bit(ag->flags, BT_HFP_AG_CODEC_CONN)) {
		error = bt_hfp_ag_create_audio_connection(ag);
	}

	return error;
}
