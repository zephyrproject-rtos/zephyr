/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/hfp_hf.h>
#include <zephyr/settings/settings.h>

#include "pcm.h"
#include "codec.h"

static struct bt_conn *active_sco_conn;

static void hf_connected(struct bt_conn *conn, struct bt_hfp_hf *hf)
{
	printk("HFP HF Connected!\n");
}

static void hf_disconnected(struct bt_hfp_hf *hf)
{
	printk("HFP HF Disconnected!\n");
}

static void pcm_rx_cb(const uint8_t *data, uint32_t len)
{
	int err;

	if (active_sco_conn == NULL) {
		return;
	}

	err = codec_tx(data, len);
	if (err != 0) {
		printk("Failed to transmit PCM data: %d\n", err);
	}
}

static void codec_rx_cb(const uint8_t *data, uint32_t len)
{
	int err;

	if (active_sco_conn == NULL) {
		return;
	}

	err = pcm_tx(data, len);
	if (err != 0) {
		printk("Failed to transmit Codec data: %d\n", err);
	}
}

static void hf_sco_connected(struct bt_hfp_hf *hf, struct bt_conn *sco_conn)
{
	struct bt_conn_info info;
	int err;

	printk("HF SCO connected\n");
	active_sco_conn = bt_conn_ref(sco_conn);

	err = bt_conn_get_info(sco_conn, &info);
	if (err != 0) {
		printk("Failed to get sco conn %p info\n", sco_conn);
		return;
	}

	printk("SCO air mode %u\n", info.sco.air_mode);

	err = pcm_init(info.sco.air_mode);
	if (err != 0) {
		printk("Failed to initialize PCM for air mode %u\n", info.sco.air_mode);
		return;
	}

	err = codec_init(info.sco.air_mode);
	if (err != 0) {
		printk("Failed to initialize CODEC for air mode %u\n", info.sco.air_mode);
		return;
	}

	err = pcm_rx_start(pcm_rx_cb);
	if (err != 0) {
		printk("Failed to start PCM\n");
		return;
	}

	err = codec_rx_start(codec_rx_cb);
	if (err != 0) {
		printk("Failed to start CODEC\n");
		return;
	}
}

static void hf_sco_disconnected(struct bt_conn *sco_conn, uint8_t reason)
{
	int err;

	if (active_sco_conn != sco_conn) {
		return;
	}

	bt_conn_unref(active_sco_conn);
	active_sco_conn = NULL;

	err = pcm_rx_stop();
	if (err != 0) {
		printk("Failed to stop PCM\n");
	}

	err = codec_rx_stop();
	if (err != 0) {
		printk("Failed to stop CODEC\n");
	}

	printk("HF SCO disconnected\n");
}

static void hf_service(struct bt_hfp_hf *hf, uint32_t value)
{
	printk("Service indicator value: %u\n", value);
}

static void hf_outgoing(struct bt_hfp_hf *hf, struct bt_hfp_hf_call *call)
{
	printk("HF call %p outgoing\n", call);
}

static void hf_remote_ringing(struct bt_hfp_hf_call *call)
{
	printk("HF remote call %p start ringing\n", call);
}

static void hf_incoming(struct bt_hfp_hf *hf, struct bt_hfp_hf_call *call)
{
	printk("HF call %p incoming\n", call);
}

static void hf_incoming_held(struct bt_hfp_hf_call *call)
{
	printk("HF call %p is held\n", call);
}

static void hf_accept(struct bt_hfp_hf_call *call)
{
	printk("HF call %p accepted\n", call);
}

static void hf_reject(struct bt_hfp_hf_call *call)
{
	printk("HF call %p rejected\n", call);
}

static void hf_terminate(struct bt_hfp_hf_call *call)
{
	printk("HF call %p terminated\n", call);
}

static void hf_signal(struct bt_hfp_hf *hf, uint32_t value)
{
	printk("Signal indicator value: %u\n", value);
}

static void hf_roam(struct bt_hfp_hf *hf, uint32_t value)
{
	printk("Roaming indicator value: %u\n", value);
}

static void hf_battery(struct bt_hfp_hf *hf, uint32_t value)
{
	printk("Battery indicator value: %u\n", value);
}

static void hf_ring_indication(struct bt_hfp_hf_call *call)
{
	printk("HF call %p ring\n", call);
}

#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
static void hf_codec_negotiate(struct bt_hfp_hf *hf, uint8_t id)
{
	int err;

	printk("HF codec negotiate 0x%02x\n", id);

	if (id == BT_HFP_HF_CODEC_CVSD) {
		printk("HF codec negotiate CVSD\n");
	} else if (IS_ENABLED(CONFIG_BT_HFP_HF_CODEC_MSBC) && id == BT_HFP_HF_CODEC_MSBC) {
		printk("HF codec negotiate mSBC\n");
	} else if (IS_ENABLED(CONFIG_BT_HFP_HF_CODEC_LC3_SWB) && id == BT_HFP_HF_CODEC_LC3_SWB) {
		printk("HF codec negotiate LC3 SWB\n");
	} else {
		printk("HF codec negotiate unknown codec\n");
		return;
	}

	err = bt_hfp_hf_select_codec(hf, id);
	if (err != 0) {
		printk("Failed to send codec id: %d\n", err);
	}
}
#endif /* defined(CONFIG_BT_HFP_HF_CODEC_NEG) */

static struct bt_hfp_hf_cb hf_cb = {
	.connected = hf_connected,
	.disconnected = hf_disconnected,
	.sco_connected = hf_sco_connected,
	.sco_disconnected = hf_sco_disconnected,
	.service = hf_service,
	.outgoing = hf_outgoing,
	.remote_ringing = hf_remote_ringing,
	.incoming = hf_incoming,
	.incoming_held = hf_incoming_held,
	.accept = hf_accept,
	.reject = hf_reject,
	.terminate = hf_terminate,
	.signal = hf_signal,
	.roam = hf_roam,
	.battery = hf_battery,
	.ring_indication = hf_ring_indication,
#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
	.codec_negotiate = hf_codec_negotiate,
#endif /* defined(CONFIG_BT_HFP_HF_CODEC_NEG) */
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	printk("Bluetooth initialized\n");

	err = bt_br_set_connectable(true, NULL);
	if (err) {
		printk("BR/EDR set/rest connectable failed (err %d)\n", err);
		return;
	}
	err = bt_br_set_discoverable(true, false);
	if (err) {
		printk("BR/EDR set discoverable failed (err %d)\n", err);
		return;
	}

	printk("BR/EDR set connectable and discoverable done\n");
}

static void handsfree_enable(void)
{
	int err;

	err = bt_hfp_hf_register(&hf_cb);
	if (err < 0) {
		printk("HFP HF Registration failed (err %d)\n", err);
	}
}

int main(void)
{
	int err;

	handsfree_enable();

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
	return 0;
}
