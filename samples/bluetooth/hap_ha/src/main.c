/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/capabilities.h>
#include <zephyr/bluetooth/audio/csis.h>
#include <zephyr/bluetooth/services/ias.h>

#include "hap_ha.h"

#define MANDATORY_SINK_CONTEXT (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | \
				BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | \
				BT_AUDIO_CONTEXT_TYPE_MEDIA | \
				BT_AUDIO_CONTEXT_TYPE_LIVE)

#define AVAILABLE_SINK_CONTEXT CONFIG_BT_PACS_SNK_CONTEXT
#define AVAILABLE_SOURCE_CONTEXT CONFIG_BT_PACS_SRC_CONTEXT

BUILD_ASSERT((CONFIG_BT_PACS_SNK_CONTEXT & MANDATORY_SINK_CONTEXT) == MANDATORY_SINK_CONTEXT,
	     "Need to support mandatory Supported_Sink_Contexts");

static uint8_t unicast_server_addata[] = {
	BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL), /* ASCS UUID */
	BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED, /* Target Announcement */
	(((AVAILABLE_SINK_CONTEXT) >>  0) & 0xFF),
	(((AVAILABLE_SINK_CONTEXT) >>  8) & 0xFF),
	(((AVAILABLE_SOURCE_CONTEXT) >>  0) & 0xFF),
	(((AVAILABLE_SOURCE_CONTEXT) >>  8) & 0xFF),
	0x00, /* Metadata length */
};

static uint8_t csis_rsi_addata[BT_CSIS_RSI_SIZE];

/* TODO: Expand with BAP data */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL)),
#if defined(CONFIG_BT_CSIS)
	BT_DATA(BT_DATA_CSIS_RSI, csis_rsi_addata, ARRAY_SIZE(csis_rsi_addata)),
#endif /* CONFIG_BT_CSIS */
	BT_DATA(BT_DATA_SVC_DATA16, unicast_server_addata, ARRAY_SIZE(unicast_server_addata)),
};

static struct k_work_delayable adv_work;
static struct bt_le_ext_adv *adv;

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	/* Restart advertising after disconnection */
	k_work_schedule(&adv_work, K_SECONDS(1));
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
};

static void rsi_changed_cb(const uint8_t *rsi)
{
	char rsi_str[13];

	snprintk(rsi_str, 163, "%02x%02x%02x%02x%02x%02x",
		 rsi[0], rsi[1], rsi[2], rsi[3], rsi[4], rsi[5]);

	printk("PRSI: 0x%s\n", rsi_str);

	memcpy(csis_rsi_addata, rsi, sizeof(csis_rsi_addata));

	if (adv != NULL) {
		int err;

		err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
		if (err) {
			printk("Failed to set advertising data (err %d)\n", err);
		}
	}
}

#if defined(CONFIG_BT_PRIVACY) && defined(CONFIG_BT_CSIS)
static bool adv_rpa_expired_cb(struct bt_le_ext_adv *adv)
{
	printk("%s", __func__);

	/* TODO: Generate new RSI and update the advertisement data */

	return true;
}
#endif /* CONFIG_BT_PRIVACY && CONFIG_BT_CSIS */

static const struct bt_le_ext_adv_cb adv_cb = {
#if defined(CONFIG_BT_PRIVACY) && defined(CONFIG_BT_CSIS)
	.rpa_expired = adv_rpa_expired_cb,
#endif /* CONFIG_BT_PRIVACY && CONFIG_BT_CSIS */
};

static void adv_work_handler(struct k_work *work)
{
	int err;

	if (adv == NULL) {
		/* Create a non-connectable non-scannable advertising set */
		err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN_NAME, &adv_cb, &adv);
		if (err) {
			printk("Failed to create advertising set (err %d)\n", err);
		}

		err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
		if (err) {
			printk("Failed to set advertising data (err %d)\n", err);
		}

		__ASSERT_NO_MSG(err == 0);
	}

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
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

void main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = has_server_init();
	if (err != 0) {
		printk("HAS Server init failed (err %d)\n", err);
		return;
	}

	err = bap_unicast_sr_init();
	if (err != 0) {
		printk("BAP Unicast Server init failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_HAS_HEARING_AID_BINAURAL)) {
		err = csip_set_member_init(rsi_changed_cb);
		if (err != 0) {
			printk("CSIP Set Member init failed (err %d)\n", err);
			return;
		}
	}

	err = vcp_vol_renderer_init();
	if (err != 0) {
		printk("VCP Volume Renderer init failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) {
		err = micp_mic_dev_init();
		if (err != 0) {
			printk("MICP Microphone Device init failed (err %d)\n", err);
			return;
		}
	}

	if (IS_ENABLED(CONFIG_BT_TBS_CLIENT)) {
		err = ccp_call_ctrl_init();
		if (err != 0) {
			printk("MICP Microphone Device init failed (err %d)\n", err);
			return;
		}
	}

	if (IS_ENABLED(CONFIG_BT_HAS_HEARING_AID_BANDED)) {
		/* HAP_d1.0r00; 3.7 BAP Unicast Server role requirements
		 * A Banded Hearing Aid in the HA role shall set the
		 * Front Left and the Front Right bits to a value of 0b1
		 * in the Sink Audio Locations characteristic value.
		 */
		bt_audio_capability_set_location(BT_AUDIO_DIR_SINK,
						 (BT_AUDIO_LOCATION_FRONT_LEFT |
						  BT_AUDIO_LOCATION_FRONT_RIGHT));
	} else {
		bt_audio_capability_set_location(BT_AUDIO_DIR_SINK,
						 BT_AUDIO_LOCATION_FRONT_LEFT);
	}

	bt_audio_capability_set_available_contexts(BT_AUDIO_DIR_SINK,
						   AVAILABLE_SINK_CONTEXT);

	if (IS_ENABLED(CONFIG_BT_ASCS_ASE_SRC)) {
		bt_audio_capability_set_location(BT_AUDIO_DIR_SOURCE,
						 BT_AUDIO_LOCATION_FRONT_LEFT);
		bt_audio_capability_set_available_contexts(BT_AUDIO_DIR_SOURCE,
							   AVAILABLE_SOURCE_CONTEXT);
	}

	k_work_init_delayable(&adv_work, adv_work_handler);
	k_work_schedule(&adv_work, K_NO_WAIT);
}
