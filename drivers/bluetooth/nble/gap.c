/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>

#include <nanokernel.h>
#include <device.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/log.h>

#include "gap_internal.h"
#include "uart.h"
#include "conn.h"
#include "rpc.h"

/* Set the firmware compatible with Nordic BLE RPC */
static uint8_t compatible_firmware[4] = { '0', '4', '2', '5' };

#if !defined(CONFIG_NBLE_DEBUG_GAP)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

static bt_ready_cb_t bt_ready_cb;
static bt_le_scan_cb_t *scan_dev_found_cb;

struct nble nble;

static struct bt_storage *storage;

#define BT_SMP_IO_DISPLAY_ONLY			0x00
#define BT_SMP_IO_DISPLAY_YESNO			0x01
#define BT_SMP_IO_KEYBOARD_ONLY			0x02
#define BT_SMP_IO_NO_INPUT_OUTPUT		0x03
#define BT_SMP_IO_KEYBOARD_DISPLAY		0x04

#define BT_SMP_OOB_NOT_PRESENT			0x00
#define BT_SMP_OOB_PRESENT			0x01

#define BT_SMP_MAX_ENC_KEY_SIZE			16

#if defined(CONFIG_NBLE_DEBUG_GAP)
static const char *bt_addr_le_str(const bt_addr_le_t *addr)
{
	static char str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, str, sizeof(str));

	return str;
}
#endif /* CONFIG_BLUETOOTH_DEBUG */

void on_nble_get_version_rsp(const struct nble_version_response *rsp)
{
	BT_DBG("VERSION: %d.%d.%d %.20s", rsp->ver.major, rsp->ver.minor,
	       rsp->ver.patch, rsp->ver.version_string);

	if (memcmp(&rsp->ver.version_string[sizeof(rsp->ver.version_string) -
		   sizeof(compatible_firmware)], compatible_firmware,
		   sizeof(compatible_firmware))) {
		BT_ERR("\n\n"
		       "Incompatible firmware: %.20s, please use version %.4s"
		       "\n\n",
		       rsp->ver.version_string, compatible_firmware);
		/* TODO: shall we allow to continue */
	}

	if (bt_ready_cb) {
		bt_ready_cb(0);
		bt_ready_cb = NULL;
	}
}

int bt_enable(bt_ready_cb_t cb)
{
	int ret;

	BT_DBG("");

	if (!cb) {
		/* With nble the callback is mandatory */
		return -EINVAL;
	}

	ret = nble_open();
	if (ret) {
		return ret;
	}

	bt_ready_cb = cb;

	return 0;
}

static bool valid_adv_param(const struct bt_le_adv_param *param)
{
	switch (param->type) {
	case BT_LE_ADV_IND:
		break;
	case BT_LE_ADV_SCAN_IND:
	case BT_LE_ADV_NONCONN_IND:
		/*
		 * BT Core 4.2 [Vol 2, Part E, 7.8.5]
		 * The Advertising_Interval_Min and Advertising_Interval_Max
		 * shall not be set to less than 0x00A0 (100 ms) if the
		 * Advertising_Type is set to ADV_SCAN_IND or ADV_NONCONN_IND.
		 */
		if (param->interval_min < 0x00a0) {
			return false;
		}
		break;
	default:
		return false;
	}

	switch (param->addr_type) {
	case BT_LE_ADV_ADDR_IDENTITY:
	case BT_LE_ADV_ADDR_NRPA:
		break;
	default:
		return false;
	}

	if (param->interval_min > param->interval_max ||
	    param->interval_min < 0x0020 || param->interval_max > 0x4000) {
		return false;
	}

	return true;
}

static int set_ad(struct bt_eir_data *eir, const struct bt_data *ad,
		  size_t ad_len)
{
	int i;

	for (i = 0; i < ad_len; i++) {
		uint8_t *p;

		/* Check if ad fit in the remaining buffer */
		if (eir->len + ad[i].data_len + 2 > 31) {
			return -ENOMEM;
		}

		p = &eir->data[eir->len];
		*p++ = ad[i].data_len + 1;
		*p++ = ad[i].type;
		memcpy(p, ad[i].data, ad[i].data_len);
		eir->len += ad[i].data_len + 2;
	}

	return 0;
}

int bt_le_adv_start(const struct bt_le_adv_param *param,
		    const struct bt_data *ad, size_t ad_len,
		    const struct bt_data *sd, size_t sd_len)
{
	struct nble_gap_adv_params params = { 0 };
	struct nble_gap_ad_data_params data = { 0 };
	int err;

	if (!valid_adv_param(param)) {
		return -EINVAL;
	}

	err = set_ad(&data.ad, ad, ad_len);
	if (err) {
		BT_ERR("Error setting ad data %d", err);
		return err;
	}

	/*
	 * Don't bother with scan response if the advertising type isn't
	 * a scannable one.
	 */
	if (param->type != BT_LE_ADV_IND && param->type != BT_LE_ADV_SCAN_IND) {
		goto set_adv;
	}

	err = set_ad(&data.sd, sd, sd_len);
	if (err) {
		BT_ERR("Error setting scan response data %d", err);
		return err;
	}

set_adv:
	/* Set advertising data */
	nble_gap_set_adv_data_req(&data);

	/* TODO: Timeout is handled by application timer */
	params.timeout = 0;
	/* forced to none currently (no whitelist support) */
	params.filter_policy = 0;
	params.interval_max = param->interval_max;
	params.interval_min = param->interval_min;
	params.type = param->type;

	/* Set advertising parameters */
	nble_gap_set_adv_params_req(&params);

	/* Start advertising */
	nble_gap_start_adv_req();

	return 0;
}

void on_nble_gap_start_advertise_rsp(const struct nble_response *rsp)
{
	if (rsp->status) {
		BT_ERR("Start advertise falied, status %d", rsp->status);
		return;
	}

	nble.keep_adv = true;

	BT_DBG("status %u", rsp->status);
}

int bt_le_adv_stop(void)
{
	BT_DBG("");

	nble_gap_stop_adv_req(NULL);

	return 0;
}

void on_nble_gap_stop_advertise_rsp(const struct nble_response *rsp)
{
	if (rsp->status) {
		BT_ERR("Stop advertise failed, status %d", rsp->status);
		return;
	}

	nble.keep_adv = false;

	BT_DBG("status %d", rsp->status);
}

static bool valid_le_scan_param(const struct bt_le_scan_param *param)
{
	if (param->type != BT_HCI_LE_SCAN_PASSIVE &&
	    param->type != BT_HCI_LE_SCAN_ACTIVE) {
		return false;
	}

	if (param->filter_dup != BT_HCI_LE_SCAN_FILTER_DUP_DISABLE &&
	    param->filter_dup != BT_HCI_LE_SCAN_FILTER_DUP_ENABLE) {
		return false;
	}

	if (param->interval < 0x0004 || param->interval > 0x4000) {
		return false;
	}

	if (param->window < 0x0004 || param->window > 0x4000) {
		return false;
	}

	if (param->window > param->interval) {
		return false;
	}

	return true;
}

int bt_le_scan_start(const struct bt_le_scan_param *param, bt_le_scan_cb_t cb)
{
	struct nble_gap_scan_params nble_params;

	BT_DBG("");

	/* Check that the parameters have valid values */
	if (!valid_le_scan_param(param)) {
		return -EINVAL;
	}

	nble_params.interval = param->interval;
	nble_params.window = param->window;
	nble_params.scan_type = param->type;
	nble_params.use_whitelist = 0;

	/* Check is scan already enabled */

	scan_dev_found_cb = cb;

	nble_gap_start_scan_req(&nble_params);

	return 0;
}

void on_nble_gap_adv_report_evt(const struct nble_gap_adv_report_evt *evt,
				const uint8_t *buf, uint8_t len)
{
	BT_DBG("");

	if (scan_dev_found_cb) {
		scan_dev_found_cb(&evt->addr, evt->rssi, evt->adv_type,
				  buf, len);
	}
}

int bt_le_scan_stop(void)
{
	BT_DBG("");

	scan_dev_found_cb = NULL;

	nble_gap_stop_scan_req();

	return 0;
}

void on_nble_gap_scan_start_stop_rsp(const struct nble_response *rsp)
{
	if (rsp->status) {
		BT_ERR("Unable to stop scan, status %d", rsp->status);
		return;
	}

	BT_DBG("");
}

void nble_log(const struct nble_log_s *param, char *format, uint8_t len)
{
#if defined(CONFIG_BLUETOOTH_DEBUG)
	/* Build meaningful output */
	printf("nble: ");
	printf(format, param->param0, param->param1, param->param2,
	       param->param3);
	printf("\n");
#endif
}

void on_nble_gap_read_bda_rsp(const struct nble_service_read_bda_response *rsp)
{
	struct nble_gap_get_version_param params;

	if (rsp->status) {
		BT_ERR("Read bdaddr failed, status %d", rsp->status);
		return;
	}

	bt_addr_le_copy(&nble.addr, &rsp->bd);

	BT_DBG("Local bdaddr: %s", bt_addr_le_str(&nble.addr));

	params.cb = NULL;
	nble_get_version_req(&params);
}

void on_nble_common_rsp(const struct nble_response *rsp)
{
	if (rsp->status) {
		BT_ERR("Last request failed, error %d", rsp->status);
		return;
	}

	BT_DBG("status %d", rsp->status);
}

/* Security Manager event handling */

static uint8_t get_io_capa(void)
{
	if (!nble.auth) {
		return BT_SMP_IO_NO_INPUT_OUTPUT;
	}

	/* Passkey Confirmation is valid only for LE SC */
	if (nble.auth->passkey_display && nble.auth->passkey_entry &&
	    nble.auth->passkey_confirm) {
		return BT_SMP_IO_KEYBOARD_DISPLAY;
	}

	/* DisplayYesNo is useful only for LE SC */
	if (nble.auth->passkey_display &&
	    nble.auth->passkey_confirm) {
		return BT_SMP_IO_DISPLAY_YESNO;
	}

	if (nble.auth->passkey_entry) {
		return BT_SMP_IO_KEYBOARD_ONLY;
	}

	if (nble.auth->passkey_display) {
		return BT_SMP_IO_DISPLAY_ONLY;
	}

	return BT_SMP_IO_NO_INPUT_OUTPUT;
}

static void send_dm_config(void)
{
	struct nble_gap_sm_config_params config = {
		.key_size = BT_SMP_MAX_ENC_KEY_SIZE,
		.oob_present = BT_SMP_OOB_NOT_PRESENT,
	};

	config.io_caps = get_io_capa();

	if (config.io_caps == BT_SMP_IO_NO_INPUT_OUTPUT) {
		config.options = BT_SMP_AUTH_BONDING;
	} else {
		config.options = BT_SMP_AUTH_BONDING | BT_SMP_AUTH_MITM;
	}

	BT_DBG("io_caps %u options %u", config.io_caps, config.options);

	nble_gap_sm_config_req(&config);
}

void on_nble_gap_sm_config_rsp(struct nble_gap_sm_config_rsp *rsp)
{
	if (rsp->status) {
		BT_ERR("SM config failed, status %d", rsp->status);
		return;
	}

	BT_DBG("status %u", rsp->status);

	/* Get bdaddr queued after SM setup */
	nble_gap_read_bda_req(NULL);
}

void on_nble_gap_sm_common_rsp(const struct nble_gap_sm_response *rsp)
{
	if (rsp->status) {
		BT_ERR("GAP SM request failed:  conn %p err %d", rsp->conn,
		       rsp->status);

		/* TODO: Handle error */
		return;
	}
}

void on_nble_gap_sm_status_evt(const struct nble_gap_sm_status_evt *ev)
{
	struct bt_conn *conn;

	if (ev->status) {
		BT_ERR("SM request failed, status %d", ev->status);
		return;
	}

	conn = bt_conn_lookup_handle(ev->conn_handle);
	if (!conn) {
		BT_ERR("Unable to find conn for handle %u", ev->conn_handle);
		return;
	}

	BT_DBG("conn %p status %d evt_type %d", conn, ev->status, ev->evt_type);

	/* TODO Handle events */
	switch (ev->evt_type) {
	case NBLE_GAP_SM_EVT_START_PAIRING:
		BT_DBG("Start pairing");
		break;
	case NBLE_GAP_SM_EVT_BONDING_COMPLETE:
		BT_DBG("Bonding complete");
		break;
	case NBLE_GAP_SM_EVT_LINK_ENCRYPTED:
		BT_DBG("Link encrypted");
		break;
	case NBLE_GAP_SM_EVT_LINK_SECURITY_CHANGE:
		BT_DBG("Security change");
		break;
	default:
		BT_ERR("Unknown event %d", ev->evt_type);
		break;
	}

	bt_conn_unref(conn);
}

void on_nble_gap_sm_passkey_display_evt(const struct nble_gap_sm_passkey_disp_evt *ev)
{
	struct bt_conn *conn;

	conn = bt_conn_lookup_handle(ev->conn_handle);
	if (!conn) {
		BT_ERR("Unable to find conn for handle %u", ev->conn_handle);
		return;
	}

	BT_DBG("conn %p passkey %u", conn, ev->passkey);

	/* TODO: Check shall we store io_caps globally */
	if (get_io_capa() == BT_SMP_IO_DISPLAY_YESNO) {
		nble.auth->passkey_confirm(conn, ev->passkey);
	} else {
		nble.auth->passkey_display(conn, ev->passkey);
	}

	bt_conn_unref(conn);
}

void on_nble_gap_sm_passkey_req_evt(const struct nble_gap_sm_passkey_req_evt *ev)
{
	struct bt_conn *conn;

	conn = bt_conn_lookup_handle(ev->conn_handle);
	if (!conn) {
		BT_ERR("Unable to find conn for handle %u", ev->conn_handle);
		return;
	}

	if (ev->key_type == NBLE_GAP_SM_PK_PASSKEY) {
		nble.auth->passkey_entry(conn);
	}

	bt_conn_unref(conn);
}

void on_nble_up(void)
{
	BT_DBG("");

	send_dm_config();
}

void bt_storage_register(struct bt_storage *bt_storage)
{
	storage = bt_storage;
}

int bt_storage_clear(bt_addr_le_t *addr)
{
	return -ENOSYS;
}
