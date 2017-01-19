/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <atomic.h>

#include <zephyr.h>
#include <device.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_NBLE_DEBUG_GAP)
#include <bluetooth/log.h>
#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>

#include "gap_internal.h"
#include "conn_internal.h"
#include "gatt_internal.h"
#include "uart.h"
#include "conn.h"
#include "rpc.h"
#include "smp.h"

#define NBLE_VERSION(a, b, c)						\
	((((a) & 0xFF) << 16) | (((b) & 0xFF) << 8) | ((c) & 0xFF))

#define NBLE_VERSION_MAJOR(v) (((v) >> 16) & 0xFF)
#define NBLE_VERSION_MINOR(v) (((v) >> 8)  & 0xFF)
#define NBLE_VERSION_PATCH(v) (((v) >> 0)  & 0xFF)

/* Set the firmware compatible with Nordic BLE RPC */
static const uint32_t compatible_firmware = NBLE_VERSION(4, 0, 31);

static bt_ready_cb_t bt_ready_cb;
static bt_le_scan_cb_t *scan_dev_found_cb;

struct nble nble;

static const struct bt_storage *storage;

#if defined(CONFIG_NBLE_DEBUG_GAP)
static const char *bt_addr_le_str(const bt_addr_le_t *addr)
{
	static char str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, str, sizeof(str));

	return str;
}
#else
static inline const char *bt_addr_le_str(const bt_addr_le_t *addr)
{
	return NULL;
}
#endif /* CONFIG_BLUETOOTH_DEBUG */

static void clear_bonds(const bt_addr_le_t *addr)
{
	struct nble_sm_clear_bonds_req params = { { 0 }, };

	bt_addr_le_copy(&params.addr, addr);

	nble_sm_clear_bonds_req(&params);
}

int bt_enable(bt_ready_cb_t cb)
{
	int ret;

	BT_DBG("");

	if (!cb) {
		/* With nble the callback is mandatory */
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&nble.flags, NBLE_FLAG_ENABLE)) {
		return -EALREADY;
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
	if (!(param->options & BT_LE_ADV_OPT_CONNECTABLE)) {
		/*
		 * BT Core 4.2 [Vol 2, Part E, 7.8.5]
		 * The Advertising_Interval_Min and Advertising_Interval_Max
		 * shall not be set to less than 0x00A0 (100 ms) if the
		 * Advertising_Type is set to ADV_SCAN_IND or ADV_NONCONN_IND.
		 */

		if (param->interval_min < 0x00a0) {
			return false;
		}
	}

	if (param->interval_min > param->interval_max ||
	    param->interval_min < 0x0020 || param->interval_max > 0x4000) {
		return false;
	}

	return true;
}

static int set_ad(struct nble_eir_data *eir, const struct bt_data *ad,
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
	struct nble_gap_set_adv_params_req params = { 0 };
	struct nble_gap_set_adv_data_req data = { 0 };
	int err;

	if (!valid_adv_param(param)) {
		return -EINVAL;
	}

	err = set_ad(&data.ad, ad, ad_len);
	if (err) {
		BT_ERR("Error setting ad data %d", err);
		return err;
	}

	err = set_ad(&data.sd, sd, sd_len);
	if (err) {
		BT_ERR("Error setting scan response data %d", err);
		return err;
	}

	/* Set advertising data */
	nble_gap_set_adv_data_req(&data);

	/* TODO: Timeout is handled by application timer */
	params.timeout = 0;
	/* forced to none currently (no whitelist support) */
	params.filter_policy = 0;
	params.interval_max = param->interval_max;
	params.interval_min = param->interval_min;

	if (param->options & BT_LE_ADV_OPT_CONNECTABLE) {
		params.type =  BT_LE_ADV_IND;
	} else {
		if (sd) {
			params.type = BT_LE_ADV_SCAN_IND;
		} else {
			params.type = BT_LE_ADV_NONCONN_IND;
		}
	}

	/* Set advertising parameters */
	nble_gap_set_adv_params_req(&params);

	/* Start advertising */
	nble_gap_start_adv_req();

	return 0;
}

void on_nble_gap_start_adv_rsp(const struct nble_common_rsp *rsp)
{
	if (rsp->status) {
		BT_ERR("Start advertise falied, status %d", rsp->status);
		return;
	}

	atomic_set_bit(&nble.flags, NBLE_FLAG_KEEP_ADVERTISING);

	BT_DBG("status %u", rsp->status);
}

int bt_le_adv_stop(void)
{
	BT_DBG("");

	nble_gap_stop_adv_req(NULL);

	return 0;
}

void on_nble_gap_stop_advertise_rsp(const struct nble_common_rsp *rsp)
{
	if (rsp->status) {
		BT_ERR("Stop advertise failed, status %d", rsp->status);
		return;
	}

	atomic_clear_bit(&nble.flags, NBLE_FLAG_KEEP_ADVERTISING);

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
	struct nble_gap_start_scan_req nble_params;

	BT_DBG("");

	/* Check that the parameters have valid values */
	if (!valid_le_scan_param(param)) {
		return -EINVAL;
	}

	nble_params.scan_params.interval = param->interval;
	nble_params.scan_params.window = param->window;
	nble_params.scan_params.scan_type = param->type;
	nble_params.scan_params.use_whitelist = 0;

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
		struct net_buf_simple *data = NET_BUF_SIMPLE(31);

		net_buf_simple_init(data, 0);
		memcpy(net_buf_simple_add(data, len), buf, len);

		scan_dev_found_cb(&evt->addr, evt->rssi, evt->adv_type, data);
	}
}

int bt_le_scan_stop(void)
{
	BT_DBG("");

	scan_dev_found_cb = NULL;

	nble_gap_stop_scan_req();

	return 0;
}

void on_nble_gap_scan_start_stop_rsp(const struct nble_common_rsp *rsp)
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

void on_nble_get_bda_rsp(const struct nble_get_bda_rsp *rsp)
{
	bt_addr_le_copy(&nble.addr, &rsp->bda);

	BT_DBG("Local bdaddr: %s", bt_addr_le_str(&nble.addr));

	/* Make sure the nRF51 persistent memory is cleared */
	clear_bonds(BT_ADDR_LE_ANY);

	if (bt_ready_cb) {
		bt_ready_cb(0);
		bt_ready_cb = NULL;
	}
}

void on_nble_common_rsp(const struct nble_common_rsp *rsp)
{
	if (rsp->status) {
		BT_ERR("Last request failed, error %d", rsp->status);
		return;
	}

	BT_DBG("status %d", rsp->status);
}

void rpc_init_cb(uint32_t version, bool compatible)
{
	BT_DBG("");

	if (!compatible) {
		BT_ERR("\n\n"
		       "RPC reported incompatible firmware"
		       "\n\n");
	}
	if (version != compatible_firmware) {
		BT_ERR("\n\n"
		       "Incompatible firmware: %d.%d.%d, "
		       "please use version %d.%d.%d"
		       "\n\n",
		       NBLE_VERSION_MAJOR(version),
		       NBLE_VERSION_MINOR(version),
		       NBLE_VERSION_PATCH(version),
		       NBLE_VERSION_MAJOR(compatible_firmware),
		       NBLE_VERSION_MINOR(compatible_firmware),
		       NBLE_VERSION_PATCH(compatible_firmware));
		/* TODO: shall we allow to continue */
	}

	bt_smp_init();
	bt_gatt_init();
}

void bt_storage_register(const struct bt_storage *bt_storage)
{
	storage = bt_storage;
}

int bt_storage_clear(const bt_addr_le_t *addr)
{
	clear_bonds(addr);
	/* FIXME: make the necessary storage callbacks too. */
	return 0;
}

int bt_le_oob_get_local(struct bt_le_oob *oob)
{
	bt_addr_le_copy(&oob->addr, &nble.addr);

	return 0;
}
