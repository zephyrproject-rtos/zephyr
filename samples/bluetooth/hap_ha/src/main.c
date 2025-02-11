/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/services/ias.h>

#include "hap_ha.h"

#define MANDATORY_SINK_CONTEXT (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | \
				BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | \
				BT_AUDIO_CONTEXT_TYPE_MEDIA | \
				BT_AUDIO_CONTEXT_TYPE_LIVE)

#define AVAILABLE_SINK_CONTEXT   MANDATORY_SINK_CONTEXT
#define AVAILABLE_SOURCE_CONTEXT MANDATORY_SINK_CONTEXT

static uint8_t unicast_server_addata[] = {
	BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL), /* ASCS UUID */
	BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED, /* Target Announcement */
	BT_BYTES_LIST_LE16(AVAILABLE_SINK_CONTEXT),
	BT_BYTES_LIST_LE16(AVAILABLE_SOURCE_CONTEXT),
	0x00, /* Metadata length */
};

static uint8_t csis_rsi_addata[BT_CSIP_RSI_SIZE];

/* TODO: Expand with BAP data */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL)),
#if defined(CONFIG_BT_CSIP_SET_MEMBER)
	BT_DATA(BT_DATA_CSIS_RSI, csis_rsi_addata, ARRAY_SIZE(csis_rsi_addata)),
#endif /* CONFIG_BT_CSIP_SET_MEMBER */
	BT_DATA(BT_DATA_SVC_DATA16, unicast_server_addata, ARRAY_SIZE(unicast_server_addata)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static struct k_work_delayable adv_work;
static struct bt_le_ext_adv *ext_adv;

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	/* Restart advertising after disconnection */
	k_work_schedule(&adv_work, K_SECONDS(1));
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
};

#if defined(CONFIG_BT_PRIVACY) && defined(CONFIG_BT_CSIP_SET_MEMBER)
static bool adv_rpa_expired_cb(struct bt_le_ext_adv *adv)
{
	char rsi_str[13];
	int err;

	err = csip_generate_rsi(csis_rsi_addata);
	if (err != 0) {
		printk("Failed to generate RSI (err %d)\n", err);
		return false;
	}

	snprintk(rsi_str, ARRAY_SIZE(rsi_str), "%02x%02x%02x%02x%02x%02x",
		 csis_rsi_addata[0], csis_rsi_addata[1], csis_rsi_addata[2],
		 csis_rsi_addata[3], csis_rsi_addata[4], csis_rsi_addata[5]);

	printk("PRSI: 0x%s\n", rsi_str);

	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Failed to set advertising data (err %d)\n", err);
		return false;
	}

	return true;
}
#endif /* CONFIG_BT_PRIVACY && CONFIG_BT_CSIP_SET_MEMBER */

static const struct bt_le_ext_adv_cb adv_cb = {
#if defined(CONFIG_BT_PRIVACY) && defined(CONFIG_BT_CSIP_SET_MEMBER)
	.rpa_expired = adv_rpa_expired_cb,
#endif /* CONFIG_BT_PRIVACY && CONFIG_BT_CSIP_SET_MEMBER */
};

static void adv_work_handler(struct k_work *work)
{
	int err;

	if (ext_adv == NULL) {
		/* Create a connectable advertising set */
		err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN, &adv_cb, &ext_adv);
		if (err) {
			printk("Failed to create advertising set (err %d)\n", err);
		}

		err = bt_le_ext_adv_set_data(ext_adv, ad, ARRAY_SIZE(ad), NULL, 0);
		if (err) {
			printk("Failed to set advertising data (err %d)\n", err);
		}

		__ASSERT_NO_MSG(err == 0);
	}

	err = bt_le_ext_adv_start(ext_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start advertising set (err %d)\n", err);
	} else {
		printk("Advertising successfully started\n");
	}
}

#if defined(CONFIG_BT_IAS)
static void alert_stop(void)
{
	printk("Alert stopped\n");
}

static void alert_start(void)
{
	printk("Mild alert started\n");
}

static void alert_high_start(void)
{
	printk("High alert started\n");
}

BT_IAS_CB_DEFINE(ias_callbacks) = {
	.no_alert = alert_stop,
	.mild_alert = alert_start,
	.high_alert = alert_high_start,
};
#endif /* CONFIG_BT_IAS */

int main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	err = has_server_init();
	if (err != 0) {
		printk("HAS Server init failed (err %d)\n", err);
		return 0;
	}

	err = bap_unicast_sr_init();
	if (err != 0) {
		printk("BAP Unicast Server init failed (err %d)\n", err);
		return 0;
	}

	if (IS_ENABLED(CONFIG_HAP_HA_HEARING_AID_BINAURAL)) {
		err = csip_set_member_init();
		if (err != 0) {
			printk("CSIP Set Member init failed (err %d)\n", err);
			return 0;
		}

		err = csip_generate_rsi(csis_rsi_addata);
		if (err != 0) {
			printk("Failed to generate RSI (err %d)\n", err);
			return 0;
		}
	}

	err = vcp_vol_renderer_init();
	if (err != 0) {
		printk("VCP Volume Renderer init failed (err %d)\n", err);
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) {
		err = micp_mic_dev_init();
		if (err != 0) {
			printk("MICP Microphone Device init failed (err %d)\n", err);
			return 0;
		}
	}

	if (IS_ENABLED(CONFIG_BT_TBS_CLIENT)) {
		err = ccp_call_ctrl_init();
		if (err != 0) {
			printk("MICP Microphone Device init failed (err %d)\n", err);
			return 0;
		}
	}

	if (IS_ENABLED(CONFIG_HAP_HA_HEARING_AID_BANDED)) {
		/* HAP_d1.0r00; 3.7 BAP Unicast Server role requirements
		 * A Banded Hearing Aid in the HA role shall set the
		 * Front Left and the Front Right bits to a value of 0b1
		 * in the Sink Audio Locations characteristic value.
		 */
		bt_pacs_set_location(BT_AUDIO_DIR_SINK,
				    (BT_AUDIO_LOCATION_FRONT_LEFT |
				     BT_AUDIO_LOCATION_FRONT_RIGHT));
	} else {
		bt_pacs_set_location(BT_AUDIO_DIR_SINK,
				     BT_AUDIO_LOCATION_FRONT_LEFT);
	}

	bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK,
				       AVAILABLE_SINK_CONTEXT);

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) {
		bt_pacs_set_location(BT_AUDIO_DIR_SOURCE,
				     BT_AUDIO_LOCATION_FRONT_LEFT);
		bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE,
					       AVAILABLE_SOURCE_CONTEXT);
	}

	k_work_init_delayable(&adv_work, adv_work_handler);
	k_work_schedule(&adv_work, K_NO_WAIT);
	return 0;
}
