/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <autoconf.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/lc3.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "cap_acceptor.h"

LOG_MODULE_REGISTER(cap_acceptor, LOG_LEVEL_INF);

#define SUPPORTED_DURATION  (BT_AUDIO_CODEC_CAP_DURATION_7_5 | BT_AUDIO_CODEC_CAP_DURATION_10)
#define MAX_CHAN_PER_STREAM BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(2)
#define SUPPORTED_FREQ      BT_AUDIO_CODEC_CAP_FREQ_ANY
#define SEM_TIMEOUT         K_SECONDS(5)
#define MAX_SDU             155U
#define MIN_SDU             30U
#define FRAMES_PER_SDU      2

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
	BT_DATA_BYTES(BT_DATA_UUID16_SOME,
		      BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_CAS_VAL)),
	BT_DATA_BYTES(BT_DATA_SVC_DATA16,
		      BT_UUID_16_ENCODE(BT_UUID_CAS_VAL),
		      BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED),
	IF_ENABLED(CONFIG_BT_BAP_UNICAST_SERVER,
		   (BT_DATA_BYTES(BT_DATA_SVC_DATA16,
				  BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL),
				  BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED,
				  BT_BYTES_LIST_LE16(SINK_CONTEXT),
				  BT_BYTES_LIST_LE16(SOURCE_CONTEXT),
				  0x00, /* Metadata length */),
	))
	IF_ENABLED(CONFIG_BT_BAP_SCAN_DELEGATOR,
		   (BT_DATA_BYTES(BT_DATA_SVC_DATA16,
				  BT_UUID_16_ENCODE(BT_UUID_BASS_VAL)),
	))
};

static struct bt_le_ext_adv *adv;
static struct peer_config peer;

static K_SEM_DEFINE(sem_state_change, 0, 1);

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected: %s", addr);

	peer.conn = bt_conn_ref(conn);
	k_sem_give(&sem_state_change);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != peer.conn) {
		return;
	}

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Disconnected: %s, reason 0x%02x %s", addr, reason, bt_hci_err_to_str(reason));

	bt_conn_unref(peer.conn);
	peer.conn = NULL;
	k_sem_give(&sem_state_change);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

static int advertise(void)
{
	int err;

	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN, NULL, &adv);
	if (err) {
		LOG_ERR("Failed to create advertising set: %d", err);

		return err;
	}

	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Failed to set advertising data: %d", err);

		return err;
	}

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		LOG_ERR("Failed to start advertising set: %d", err);

		return err;
	}

	LOG_INF("Advertising successfully started");

	/* Wait for connection*/
	err = k_sem_take(&sem_state_change, K_FOREVER);
	if (err != 0) {
		LOG_ERR("Failed to take sem_state_change: err %d", err);

		return err;
	}

	return 0;
}

struct bt_cap_stream *stream_alloc(enum bt_audio_dir dir)
{
	if (dir == BT_AUDIO_DIR_SINK && peer.sink_stream.bap_stream.ep == NULL) {
		return &peer.sink_stream;
	} else if (dir == BT_AUDIO_DIR_SOURCE && peer.source_stream.bap_stream.ep == NULL) {
		return &peer.source_stream;
	}

	return NULL;
}

void stream_released(const struct bt_cap_stream *cap_stream)
{
	if (cap_stream == &peer.source_stream) {
		k_sem_give(&peer.source_stream_sem);
	} else if (cap_stream == &peer.sink_stream) {
		k_sem_give(&peer.sink_stream_sem);
	}
}

static int reset_cap_acceptor(void)
{
	int err;

	LOG_INF("Resetting");

	if (peer.conn != NULL) {
		err = bt_conn_disconnect(peer.conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err != 0) {
			return err;
		}

		err = k_sem_take(&sem_state_change, K_FOREVER);
		if (err != 0) {
			LOG_ERR("Timeout on disconnect: %d", err);
			return err;
		}
	}

	if (adv != NULL) {
		err = bt_le_ext_adv_stop(adv);
		if (err != 0) {
			LOG_ERR("Failed to stop advertiser: %d", err);
			return err;
		}

		err = bt_le_ext_adv_delete(adv);
		if (err != 0) {
			LOG_ERR("Failed to delete advertiser: %d", err);
			return err;
		}

		adv = NULL;
	}

	if (peer.source_stream.bap_stream.ep != NULL) {
		err = k_sem_take(&peer.source_stream_sem, SEM_TIMEOUT);
		if (err != 0) {
			LOG_ERR("Timeout on source_stream_sem: %d", err);
			return err;
		}
	}

	if (peer.sink_stream.bap_stream.ep != NULL) {
		err = k_sem_take(&peer.sink_stream_sem, SEM_TIMEOUT);
		if (err != 0) {
			LOG_ERR("Timeout on sink_stream_sem: %d", err);
			return err;
		}
	}

	k_sem_reset(&sem_state_change);

	return 0;
}

/** Register the PAC records for PACS */
static int register_pac(enum bt_audio_dir dir, enum bt_audio_context context,
			struct bt_pacs_cap *cap)
{
	int err;

	err = bt_pacs_cap_register(dir, cap);
	if (err != 0) {
		LOG_ERR("Failed to register capabilities: %d", err);

		return err;
	}

	err = bt_pacs_set_location(dir, BT_AUDIO_LOCATION_MONO_AUDIO);
	if (err != 0) {
		LOG_ERR("Failed to set location: %d", err);

		return err;
	}

	err = bt_pacs_set_supported_contexts(dir, context);
	if (err != 0 && err != -EALREADY) {
		LOG_ERR("Failed to set supported contexts: %d", err);

		return err;
	}

	err = bt_pacs_set_available_contexts(dir, context);
	if (err != 0 && err != -EALREADY) {
		LOG_ERR("Failed to set available contexts: %d", err);

		return err;
	}

	return 0;
}

static int init_cap_acceptor(void)
{
	static const struct bt_audio_codec_cap lc3_codec_cap = BT_AUDIO_CODEC_CAP_LC3(
		SUPPORTED_FREQ, SUPPORTED_DURATION, MAX_CHAN_PER_STREAM, MIN_SDU, MAX_SDU,
		FRAMES_PER_SDU, (SINK_CONTEXT | SOURCE_CONTEXT));
	const struct bt_bap_pacs_register_param pacs_param = {
		.snk_pac = true,
		.snk_loc = true,
		.src_pac = true,
		.src_loc = true
	};
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		LOG_ERR("Bluetooth enable failed: %d", err);

		return 0;
	}

	LOG_INF("Bluetooth initialized");

	err = bt_pacs_register(&pacs_param);
	if (err) {
		LOG_ERR("Could not register PACS (err %d)", err);
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SNK)) {
		static struct bt_pacs_cap sink_cap = {
			.codec_cap = &lc3_codec_cap,
		};
		int err;

		err = register_pac(BT_AUDIO_DIR_SINK, SINK_CONTEXT, &sink_cap);
		if (err != 0) {
			LOG_ERR("Failed to register sink capabilities: %d", err);

			return -ENOEXEC;
		}
	}

	if (IS_ENABLED(CONFIG_BT_PAC_SRC)) {
		static struct bt_pacs_cap source_cap = {
			.codec_cap = &lc3_codec_cap,
		};
		int err;

		err = register_pac(BT_AUDIO_DIR_SOURCE, SOURCE_CONTEXT, &source_cap);
		if (err != 0) {
			LOG_ERR("Failed to register sink capabilities: %d", err);

			return -ENOEXEC;
		}
	}

	return 0;
}

int main(void)
{
	int err;

	err = init_cap_acceptor();
	if (err != 0) {
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_SERVER)) {
		err = init_cap_acceptor_unicast(&peer);
		if (err != 0) {
			return 0;
		}
	}

	if (IS_ENABLED(CONFIG_BT_BAP_BROADCAST_SINK)) {
		err = init_cap_acceptor_broadcast();
		if (err != 0) {
			return 0;
		}
	}

	LOG_INF("CAP Acceptor initialized");

	while (true) {
		err = reset_cap_acceptor();
		if (err != 0) {
			LOG_ERR("Failed to reset");

			break;
		}

		/* Start advertising as a CAP Acceptor, which includes setting the required
		 * advertising data based on the roles we support. The Common Audio Service data is
		 * always advertised, as CAP Initiators and CAP Commanders will use this to identify
		 * our device as a CAP Acceptor.
		 */
		err = advertise();
		if (err != 0) {
			continue;
		}

		/* After advertising we expect CAP Initiators to connect to us and setup streams,
		 * and eventually disconnect again. As a CAP Acceptor we just need to react to their
		 * requests and not do anything else.
		 */

		/* Reset if disconnected */
		err = k_sem_take(&sem_state_change, K_FOREVER);
		if (err != 0) {
			LOG_ERR("Failed to take sem_state_change: err %d", err);

			break;
		}
	}

	return 0;
}
