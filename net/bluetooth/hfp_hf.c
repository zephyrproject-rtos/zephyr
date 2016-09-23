/* hfp_hf.c - Hands free Profile - Handsfree side handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
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
#include <nanokernel.h>
#include <errno.h>
#include <atomic.h>
#include <misc/byteorder.h>
#include <misc/util.h>
#include <misc/nano_work.h>

#include <bluetooth/log.h>
#include <bluetooth/conn.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hfp_hf.h>

#include "l2cap_internal.h"
#include "rfcomm_internal.h"
#include "hfp_internal.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_HFP_HF)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

struct bt_hfp_hf_cb *bt_hf;

static struct nano_fifo hf_buf;
static NET_BUF_POOL(hf_pool, CONFIG_BLUETOOTH_MAX_CONN + 1,
		    BT_RFCOMM_BUF_SIZE(BLUETOOTH_HFP_MAX_PDU),
		    &hf_buf, NULL, BT_BUF_USER_DATA_MIN);

static struct bt_hfp_hf bt_hfp_hf_pool[CONFIG_BLUETOOTH_MAX_CONN];

static void hfp_hf_connected(struct bt_rfcomm_dlc *dlc)
{
	BT_DBG("hf connected");
}

static void hfp_hf_disconnected(struct bt_rfcomm_dlc *dlc)
{
	BT_DBG("hf disconnected!");
}

static void hfp_hf_recv(struct bt_rfcomm_dlc *dlc, struct net_buf *buf)
{
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

		if (hf->rfcomm_dlc.session) {
			continue;
		}

		hf->rfcomm_dlc.ops = &ops;
		hf->rfcomm_dlc.mtu = BLUETOOTH_HFP_MAX_PDU;

		*dlc = &hf->rfcomm_dlc;

		/* Set the supported features*/
		hf->hf_features = BT_HFP_HF_SUPPORTED_FEATURES;

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

	net_buf_pool_init(hf_pool);

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
