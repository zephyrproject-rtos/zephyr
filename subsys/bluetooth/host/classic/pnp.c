/* hfp_ag.c - Hands free Profile - Audio Gateway side handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/pnp.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include "host/hci_core.h"
#include "host/conn_internal.h"
#include "host/l2cap_internal.h"

#define LOG_LEVEL CONFIG_BT_PNP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_pnp);

NET_BUF_POOL_FIXED_DEFINE(sdp_pool, CONFIG_BT_MAX_CONN, 256, 8, NULL);

enum {
	PNP_STATE_IDLE,
	PNP_STATE_DISCOVERING,
};

static bt_pnp_discover_func_t bt_pnp_cb;
static uint8_t bt_pnp_state;

static uint8_t bt_pnp_sdp_handle(struct bt_conn *conn, struct bt_sdp_client_result *result)
{
	int ret;
	uint16_t vendor_id, product_id;

	bt_pnp_state = PNP_STATE_IDLE;
	if (!result || !result->resp_buf) {
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	if (!bt_pnp_cb) {
		return BT_SDP_DISCOVER_UUID_STOP;
	}

	ret = bt_sdp_get_vendor_id(result->resp_buf, &vendor_id);
	if (!ret) {
		LOG_DBG("vendor_id 0x%x\n", vendor_id);
		bt_pnp_cb(conn, BT_SDP_ATTR_VENDOR_ID, vendor_id);
	}

	ret = bt_sdp_get_product_id(result->resp_buf, &product_id);
	if (!ret) {
		LOG_DBG("product_id 0x%x\n", product_id);
		bt_pnp_cb(conn, BT_SDP_ATTR_PRODUCT_ID, product_id);
	}

	return BT_SDP_DISCOVER_UUID_STOP;
}

static struct bt_sdp_discover_params sdp_params = {
	.uuid = BT_UUID_DECLARE_16(BT_SDP_PNP_INFO_SVCLASS),
	.func = bt_pnp_sdp_handle,
	.pool = &sdp_pool,
};

int bt_pnp_discover(struct bt_conn *conn, bt_pnp_discover_func_t cb)
{
	int ret;

	if (bt_pnp_state == PNP_STATE_DISCOVERING) {
		LOG_ERR("Pnp is busy");
		return -EBUSY;
	}

	if (!cb) {
		LOG_ERR("Pnp  cb is null");
		return -EINVAL;
	}

	bt_pnp_cb = cb;
	bt_pnp_state = PNP_STATE_DISCOVERING;
	ret = bt_sdp_discover(conn, &sdp_params);
	if (ret) {
		LOG_ERR("PNP sdp discover, err:%d", ret);
		return ret;
	}

	return ret;
}