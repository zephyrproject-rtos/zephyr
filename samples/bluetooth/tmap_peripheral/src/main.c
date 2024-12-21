/*
 * Copyright 2023 NXP
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/audio/tmap.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/mcs.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>

#include "tmap_peripheral.h"

static struct bt_conn *default_conn;
static struct k_work_delayable call_terminate_set_work;
static struct k_work_delayable media_pause_set_work;

static uint8_t unicast_server_addata[] = {
	BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL),    /* ASCS UUID */
	BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED, /* Target Announcement */
	BT_BYTES_LIST_LE16(AVAILABLE_SINK_CONTEXT),
	BT_BYTES_LIST_LE16(AVAILABLE_SOURCE_CONTEXT),
	0x00, /* Metadata length */
};

static const uint8_t cap_addata[] = {
	BT_UUID_16_ENCODE(BT_UUID_CAS_VAL),
	BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED,
};

static uint8_t tmap_addata[] = {
	BT_UUID_16_ENCODE(BT_UUID_TMAS_VAL),                    /* TMAS UUID */
	BT_BYTES_LIST_LE16(BT_TMAP_ROLE_UMR | BT_TMAP_ROLE_CT), /* TMAP Role */
};

static uint8_t csis_rsi_addata[BT_CSIP_RSI_SIZE];
static bool peer_is_cg;
static bool peer_is_ums;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
		      BT_BYTES_LIST_LE16(BT_APPEARANCE_WEARABLE_AUDIO_DEVICE_EARBUD)),
	BT_DATA_BYTES(BT_DATA_UUID16_SOME, BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_CAS_VAL), BT_UUID_16_ENCODE(BT_UUID_TMAS_VAL)),
#if defined(CONFIG_BT_CSIP_SET_MEMBER)
	BT_DATA(BT_DATA_CSIS_RSI, csis_rsi_addata, ARRAY_SIZE(csis_rsi_addata)),
#endif /* CONFIG_BT_CSIP_SET_MEMBER */
	BT_DATA(BT_DATA_SVC_DATA16, tmap_addata, ARRAY_SIZE(tmap_addata)),
	BT_DATA(BT_DATA_SVC_DATA16, cap_addata, ARRAY_SIZE(cap_addata)),
	BT_DATA(BT_DATA_SVC_DATA16, unicast_server_addata, ARRAY_SIZE(unicast_server_addata)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static K_SEM_DEFINE(sem_connected, 0, 1);
static K_SEM_DEFINE(sem_security_updated, 0, 1);
static K_SEM_DEFINE(sem_disconnected, 0, 1);
static K_SEM_DEFINE(sem_discovery_done, 0, 1);

void tmap_discovery_complete(enum bt_tmap_role peer_role, struct bt_conn *conn, int err)
{
	if (conn != default_conn) {
		return;
	}

	if (err) {
		printk("TMAS discovery failed! (err %d)\n", err);
		return;
	}

	peer_is_cg = (peer_role & BT_TMAP_ROLE_CG) != 0;
	peer_is_ums = (peer_role & BT_TMAP_ROLE_UMS) != 0;
	printk("TMAP discovery done\n");
	k_sem_give(&sem_discovery_done);
}

static struct bt_tmap_cb tmap_callbacks = {
	.discovery_complete = tmap_discovery_complete
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		printk("Failed to connect to %s %u %s\n", addr, err, bt_hci_err_to_str(err));

		default_conn = NULL;
		return;
	}

	printk("Connected: %s\n", addr);
	default_conn = bt_conn_ref(conn);
	k_sem_give(&sem_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	bt_conn_unref(default_conn);
	default_conn = NULL;

	k_sem_give(&sem_disconnected);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	if (err == 0) {
		printk("Security changed: %u\n", err);
		k_sem_give(&sem_security_updated);
	} else {
		printk("Failed to set security level: %s(%u)", bt_security_err_to_str(err), err);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
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

static void audio_timer_timeout(struct k_work *work)
{
	int err = ccp_terminate_call();

	if (err != 0) {
		printk("Error sending call terminate command!\n");
	}
}

static void media_play_timeout(struct k_work *work)
{
	int err = mcp_send_cmd(BT_MCS_OPC_PAUSE);

	if (err != 0) {
		printk("Error sending pause command!\n");
	}
}

int main(void)
{
	int err;
	struct bt_le_ext_adv *adv;

	err = bt_enable(NULL);
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return err;
	}

	printk("Bluetooth initialized\n");

	k_work_init_delayable(&call_terminate_set_work, audio_timer_timeout);
	k_work_init_delayable(&media_pause_set_work, media_play_timeout);

	printk("Initializing TMAP and setting role\n");
	err = bt_tmap_register(BT_TMAP_ROLE_CT | BT_TMAP_ROLE_UMR);
	if (err != 0) {
		return err;
	}

	if (IS_ENABLED(CONFIG_TMAP_PERIPHERAL_DUO)) {
		err = csip_set_member_init();
		if (err != 0) {
			printk("CSIP Set Member init failed (err %d)\n", err);
			return err;
		}

		err = csip_generate_rsi(csis_rsi_addata);
		if (err != 0) {
			printk("Failed to generate RSI (err %d)\n", err);
			return err;
		}
	}

	err = vcp_vol_renderer_init();
	if (err != 0) {
		return err;
	}
	printk("VCP initialized\n");

	err = bap_unicast_sr_init();
	if (err != 0) {
		return err;
	}
	printk("BAP initialized\n");

	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN, &adv_cb, &adv);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return err;
	}

	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Failed to set advertising data (err %d)\n", err);
		return err;
	}

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start advertising set (err %d)\n", err);
		return err;
	}

	printk("Advertising successfully started\n");
	k_sem_take(&sem_connected, K_FOREVER);
	k_sem_take(&sem_security_updated, K_FOREVER);

	err = bt_tmap_discover(default_conn, &tmap_callbacks);
	if (err != 0) {
		return err;
	}
	k_sem_take(&sem_discovery_done, K_FOREVER);

	err = ccp_call_ctrl_init(default_conn);
	if (err != 0) {
		return err;
	}
	printk("CCP initialized\n");

	err = mcp_ctlr_init(default_conn);
	if (err != 0) {
		return err;
	}
	printk("MCP initialized\n");

	if (peer_is_cg) {
		/* Initiate a call with CCP */
		err = ccp_originate_call();
		if (err != 0) {
			printk("Error sending call originate command!\n");
		}
		/* Start timer to send terminate call command */
		k_work_schedule(&call_terminate_set_work, K_MSEC(2000));
	}

	if (peer_is_ums) {
		/* Play media with MCP */
		err = mcp_send_cmd(BT_MCS_OPC_PLAY);
		if (err != 0) {
			printk("Error sending media play command!\n");
		}

		/* Start timer to send media pause command */
		k_work_schedule(&media_pause_set_work, K_MSEC(2000));

		err = k_sem_take(&sem_disconnected, K_FOREVER);
		if (err != 0) {
			printk("failed to take sem_disconnected (err %d)\n", err);
		}
	}

	return 0;
}
