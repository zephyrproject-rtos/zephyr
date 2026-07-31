/*
 * Copyright 2023 NXP
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/assigned_numbers.h>
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
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>
#include <zephyr/types.h>

#include "le_audio_playback.h"
#include "tmap_peripheral.h"

/* Advertised and registered TMAP role mask derived from Kconfig. */
#define TMAP_PERIPHERAL_ROLE_MASK ( \
	(IS_ENABLED(CONFIG_TMAP_PERIPHERAL_ROLE_CT)  ? BT_TMAP_ROLE_CT  : 0) | \
	(IS_ENABLED(CONFIG_TMAP_PERIPHERAL_ROLE_UMR) ? BT_TMAP_ROLE_UMR : 0))

BUILD_ASSERT(TMAP_PERIPHERAL_ROLE_MASK != 0,
	     "At least one of CONFIG_TMAP_PERIPHERAL_ROLE_CT / _ROLE_UMR must be set");

static struct bt_conn *default_conn;
#if defined(CONFIG_TMAP_PERIPHERAL_ROLE_CT)
static struct k_work_delayable call_terminate_set_work;
#endif /* CONFIG_TMAP_PERIPHERAL_ROLE_CT */
#if defined(CONFIG_TMAP_PERIPHERAL_ROLE_UMR)
static struct k_work_delayable media_pause_set_work;
#endif /* CONFIG_TMAP_PERIPHERAL_ROLE_UMR */
static struct bt_le_ext_adv *g_adv;

static uint8_t unicast_server_addata[] = {
	BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL),    /* ASCS UUID */
	BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED, /* Target Announcement */
	BT_BYTES_LIST_LE16(AVAILABLE_SINK_CONTEXT),
	BT_BYTES_LIST_LE16(AVAILABLE_SOURCE_CONTEXT),
	0x00U, /* Metadata length */
};

static const uint8_t cap_addata[] = {
	BT_UUID_16_ENCODE(BT_UUID_CAS_VAL),
	BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED,
};

static uint8_t tmap_addata[] = {
	BT_UUID_16_ENCODE(BT_UUID_TMAS_VAL),                    /* TMAS UUID */
	BT_BYTES_LIST_LE16(TMAP_PERIPHERAL_ROLE_MASK),          /* TMAP Role */
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

static K_SEM_DEFINE(sem_connected, 0U, 1U);
static K_SEM_DEFINE(sem_security_updated, 0U, 1U);
static K_SEM_DEFINE(sem_disconnected, 0U, 1U);
static K_SEM_DEFINE(sem_discovery_done, 0U, 1U);

void tmap_discovery_complete(enum bt_tmap_role peer_role, struct bt_conn *conn, int err)
{
	if (conn != default_conn) {
		return;
	}

	if (err != 0) {
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
	if (err != 0) {
		printk("Failed to connect to %s %u %s\n", bt_conn_dst_str(conn),
		       err, bt_hci_err_to_str(err));

		default_conn = NULL;
		return;
	}

	printk("Connected: %s\n", bt_conn_dst_str(conn));
	default_conn = bt_conn_ref(conn);
	k_sem_give(&sem_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn != default_conn) {
		return;
	}

	printk("Disconnected: %s, reason 0x%02x %s\n", bt_conn_dst_str(conn),
	       reason, bt_hci_err_to_str(reason));

	bt_conn_drop(&default_conn);

	k_sem_give(&sem_disconnected);
}

/* Restart advertising once the connection object is freed. Using the recycled
 * callback avoids the -ENOMEM race seen when restarting from disconnected(),
 * where the host is still tearing down conn_tx / ISO contexts.
 */
static void recycled_cb(void)
{
	int err;

	if (g_adv == NULL) {
		return;
	}

	err = bt_le_ext_adv_start(g_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0 && err != -EALREADY) {
		printk("Failed to restart advertising (err %d)\n", err);
		return;
	}

	printk("Advertising restarted\n");
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(level);

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
	.recycled = recycled_cb,
	.security_changed = security_changed,
};

/* Peers such as Pixel phones require MITM for LE Audio unicast. Expose
 * DisplayYesNo IO caps (passkey_display + passkey_confirm) and auto-accept
 * numeric comparison so bonding completes without a UI on the device.
 *
 * WARNING: auto-confirming the passkey provides no real MITM protection.
 * A production build with a UI must show the passkey to the user and only
 * call bt_conn_auth_passkey_confirm() on explicit user acceptance.
 */
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	printk("Passkey for %s: %06u\n", bt_conn_dst_str(conn), passkey);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	int err;

	printk("Confirming passkey for %s: %06u\n", bt_conn_dst_str(conn), passkey);
	err = bt_conn_auth_passkey_confirm(conn);
	if (err != 0) {
		printk("Failed to confirm passkey (err %d)\n", err);
	}
}

static void auth_cancel(struct bt_conn *conn)
{
	printk("Pairing cancelled: %s\n", bt_conn_dst_str(conn));
}

static struct bt_conn_auth_cb auth_cb = {
	.passkey_display = auth_passkey_display,
	.passkey_confirm = auth_passkey_confirm,
	.cancel = auth_cancel,
};

static void auth_pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	printk("Pairing failed with %s: reason %u\n", bt_conn_dst_str(conn), reason);
}

static void auth_pairing_complete(struct bt_conn *conn, bool bonded)
{
	printk("Pairing complete with %s (bonded=%d)\n", bt_conn_dst_str(conn), bonded);
}

static struct bt_conn_auth_info_cb auth_info_cb = {
	.pairing_failed = auth_pairing_failed,
	.pairing_complete = auth_pairing_complete,
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
	if (err != 0) {
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

#if defined(CONFIG_TMAP_PERIPHERAL_ROLE_CT)
static void audio_timer_timeout(struct k_work *work)
{
	int err = ccp_terminate_call();

	ARG_UNUSED(work);

	if (err != 0) {
		printk("Error sending call terminate command!\n");
	}
}
#endif /* CONFIG_TMAP_PERIPHERAL_ROLE_CT */

#if defined(CONFIG_TMAP_PERIPHERAL_ROLE_UMR)
static void media_play_timeout(struct k_work *work)
{
	int err = mcp_send_cmd(BT_MCS_OPC_PAUSE);

	ARG_UNUSED(work);

	if (err != 0) {
		printk("Error sending pause command!\n");
	}
}
#endif /* CONFIG_TMAP_PERIPHERAL_ROLE_UMR */

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

	err = bt_conn_auth_cb_register(&auth_cb);
	if (err != 0) {
		printk("Failed to register auth callbacks (err %d)\n", err);
		return err;
	}

	err = bt_conn_auth_info_cb_register(&auth_info_cb);
	if (err != 0) {
		printk("Failed to register auth info callbacks (err %d)\n", err);
		return err;
	}

#if defined(CONFIG_TMAP_PERIPHERAL_ROLE_CT)
	k_work_init_delayable(&call_terminate_set_work, audio_timer_timeout);
#endif /* CONFIG_TMAP_PERIPHERAL_ROLE_CT */
#if defined(CONFIG_TMAP_PERIPHERAL_ROLE_UMR)
	k_work_init_delayable(&media_pause_set_work, media_play_timeout);
#endif /* CONFIG_TMAP_PERIPHERAL_ROLE_UMR */

	printk("Initializing TMAP and setting role\n");
	err = bt_tmap_register(TMAP_PERIPHERAL_ROLE_MASK);
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

	err = le_audio_playback_init();
	if (err != 0) {
		printk("LE Audio playback init failed (err %d)\n", err);
		/* Not fatal - continue without playback. */
	} else {
		printk("LE Audio playback initialized\n");
	}

	err = bt_le_ext_adv_create(BT_BAP_ADV_PARAM_CONN_QUICK, &adv_cb, &adv);
	if (err != 0) {
		printk("Failed to create advertising set (err %d)\n", err);
		return err;
	}
	g_adv = adv;

	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		printk("Failed to set advertising data (err %d)\n", err);
		return err;
	}

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		printk("Failed to start advertising set (err %d)\n", err);
		return err;
	}

	printk("Advertising successfully started\n");
	err = k_sem_take(&sem_connected, K_FOREVER);
	__ASSERT_NO_MSG(err == 0);

	err = k_sem_take(&sem_security_updated, K_FOREVER);
	__ASSERT_NO_MSG(err == 0);

	err = bt_tmap_discover(default_conn, &tmap_callbacks);
	if (err != 0) {
		return err;
	}

	err = k_sem_take(&sem_discovery_done, K_FOREVER);
	__ASSERT_NO_MSG(err == 0);

#if defined(CONFIG_TMAP_PERIPHERAL_ROLE_CT)
	err = ccp_call_ctrl_init(default_conn);
	if (err != 0) {
		return err;
	}
	printk("CCP initialized\n");
#endif /* CONFIG_TMAP_PERIPHERAL_ROLE_CT */

#if defined(CONFIG_TMAP_PERIPHERAL_ROLE_UMR)
	err = mcp_ctlr_init(default_conn);
	if (err != 0) {
		return err;
	}
	printk("MCP initialized\n");
#endif /* CONFIG_TMAP_PERIPHERAL_ROLE_UMR */

#if defined(CONFIG_TMAP_PERIPHERAL_ROLE_CT)
	if (IS_ENABLED(CONFIG_TMAP_PERIPHERAL_AUTO_CTRL) && peer_is_cg) {
		/* Initiate a call with CCP */
		err = ccp_originate_call();
		if (err != 0) {
			printk("Error sending call originate command!\n");
		}
		/* Start timer to send terminate call command */
		k_work_schedule(&call_terminate_set_work, K_MSEC(2000));
	}
#endif /* CONFIG_TMAP_PERIPHERAL_ROLE_CT */

#if defined(CONFIG_TMAP_PERIPHERAL_ROLE_UMR)
	if (IS_ENABLED(CONFIG_TMAP_PERIPHERAL_AUTO_CTRL) && peer_is_ums) {
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
#endif /* CONFIG_TMAP_PERIPHERAL_ROLE_UMR */

	return 0;
}
