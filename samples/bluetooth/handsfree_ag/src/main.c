/* main.c - Application main entry point */

/*
 * Copyright 2024 NXP
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
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/hfp_ag.h>
#include <zephyr/settings/settings.h>

static struct bt_conn *default_conn;
struct bt_hfp_ag *hfp_ag;

static struct bt_br_discovery_param br_discover;

static struct bt_br_discovery_param br_discover;
static struct bt_br_discovery_result scan_result[CONFIG_BT_HFP_AG_DISCOVER_RESULT_COUNT];

struct k_work discover_work;
struct k_work_delayable call_connect_work;
struct k_work_delayable call_disconnect_work;

struct k_work_delayable call_remote_ringing_work;
struct k_work_delayable call_remote_accept_work;

NET_BUF_POOL_DEFINE(sdp_discover_pool, 10, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static void ag_connected(struct bt_hfp_ag *ag)
{
	printk("HFP AG connected!\n");
	k_work_schedule(&call_connect_work, K_MSEC(CONFIG_BT_HFP_AG_START_CALL_DELAY_TIME));
}

static void ag_disconnected(struct bt_hfp_ag *ag)
{
	printk("HFP AG disconnected!\n");
}

static void ag_sco_connected(struct bt_hfp_ag *ag, struct bt_conn *sco_conn)
{
	printk("HFP AG SCO connected!\n");
}

static void ag_sco_disconnected(struct bt_hfp_ag *ag)
{
	printk("HFP AG SCO disconnected!\n");
}

static void ag_ringing(struct bt_hfp_ag *ag, bool in_band)
{
	printk("Ringing (in bond? %s)\n", in_band ? "Yes" : "No");
}

static void ag_accept(struct bt_hfp_ag *ag)
{
	printk("Call Accepted\n");
	k_work_schedule(&call_disconnect_work, K_SECONDS(10));
}

static void ag_reject(struct bt_hfp_ag *ag)
{
	printk("Call Rejected\n");
	k_work_schedule(&call_disconnect_work, K_SECONDS(1));
}

static void ag_terminate(struct bt_hfp_ag *ag)
{
	printk("Call terminated\n");
	k_work_schedule(&call_disconnect_work, K_SECONDS(1));
}

static void ag_outgoing(struct bt_hfp_ag *ag, const char *number)
{
	printk("Call outgoing, remote number %s\n", number);
	k_work_cancel_delayable(&call_connect_work);
	k_work_schedule(&call_remote_ringing_work, K_SECONDS(1));
}

static void ag_incoming(struct bt_hfp_ag *ag, const char *number)
{
	printk("Incoming call, remote number %s\n", number);
	k_work_cancel_delayable(&call_connect_work);
}

static struct bt_hfp_ag_cb ag_cb = {
	.connected = ag_connected,
	.disconnected = ag_disconnected,
	.sco_connected = ag_sco_connected,
	.sco_disconnected = ag_sco_disconnected,
	.outgoing = ag_outgoing,
	.incoming = ag_incoming,
	.ringing = ag_ringing,
	.accept = ag_accept,
	.reject = ag_reject,
	.terminate = ag_terminate,
};

static uint8_t sdp_discover_cb(struct bt_conn *conn, struct bt_sdp_client_result *result)
{
	int err;
	uint16_t value;

	printk("Discover done\n");

	if (result->resp_buf != NULL) {
		err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &value);

		if (err != 0) {
			printk("Fail to parser RFCOMM the SDP response!\n");
		} else {
			printk("The server channel is %d\n", value);
			err = bt_hfp_ag_connect(conn, &hfp_ag, value);
			if (err != 0) {
				printk("Fail to create hfp AG connection (err %d)\n", err);
			}
		}
	}

	return BT_SDP_DISCOVER_UUID_STOP;
}

static struct bt_sdp_discover_params sdp_discover = {
	.func = sdp_discover_cb,
	.pool = &sdp_discover_pool,
	.uuid = BT_UUID_DECLARE_16(BT_SDP_HANDSFREE_SVCLASS),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	int res;

	if (err) {
		if (default_conn != NULL) {
			default_conn = NULL;
		}
		printk("Connection failed (err 0x%02x)\n", err);
	} else {
		if (default_conn == conn) {
			struct bt_conn_info info;

			bt_conn_get_info(conn, &info);
			if (info.type != BT_CONN_TYPE_BR) {
				return;
			}

			/*
			 * Do an SDP Query on Successful ACL connection complete with the
			 * required device
			 */
			res = bt_sdp_discover(default_conn, &sdp_discover);
			if (res) {
				printk("SDP discovery failed (err %d)\r\n", res);
			} else {
				printk("SDP discovery started\r\n");
			}
			printk("Connected\n");
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);

	if (default_conn != conn) {
		return;
	}

	if (default_conn) {
		default_conn = NULL;
	} else {
		return;
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_conn_info info;

	bt_conn_get_info(conn, &info);

	bt_addr_to_str(info.br.dst, addr, sizeof(addr));

	printk("Security changed: %s level %u (err %d)\n", addr, level, err);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void discovery_recv_cb(const struct bt_br_discovery_result *result)
{
	(void)result;
}

static void discovery_timeout_cb(const struct bt_br_discovery_result *results, size_t count)
{
	char addr[BT_ADDR_LE_STR_LEN];
	const uint8_t *eir;
	bool cod_hf = false;
	static uint8_t temp[240];
	size_t len = sizeof(results->eir);
	uint8_t major_device;
	uint8_t minor_device;
	size_t i;

	for (i = 0; i < count; i++) {
		bt_addr_to_str(&results[i].addr, addr, sizeof(addr));
		printk("Device[%d]: %s, rssi %d, cod 0x%X%X%X", i, addr, results[i].rssi,
		       results[i].cod[0], results[i].cod[1], results[i].cod[2]);

		major_device = (uint8_t)BT_COD_MAJOR_DEVICE_CLASS(results[i].cod);
		minor_device = (uint8_t)BT_COD_MINOR_DEVICE_CLASS(results[i].cod);

		if ((major_device & BT_COD_MAJOR_AUDIO_VIDEO) &&
		    (minor_device & BT_COD_MAJOR_AUDIO_VIDEO_MINOR_HANDS_FREE)) {
			cod_hf = true;
		}

		eir = results[i].eir;

		while ((eir[0] > 2) && (len > eir[0])) {
			switch (eir[1]) {
			case BT_DATA_NAME_SHORTENED:
			case BT_DATA_NAME_COMPLETE:
				memcpy(temp, &eir[2], eir[0] - 1);
				temp[eir[0] - 1] = '\0'; /* Set end flag */
				printk(", name %s", temp);
				break;
			}
			len = len - eir[0] - 1;
			eir = eir + eir[0] + 1;
		}
		printk("\n");

		if (cod_hf) {
			break;
		}
	}

	if (!cod_hf) {
		(void)k_work_submit(&discover_work);
	} else {
		(void)k_work_cancel(&discover_work);
		default_conn = bt_conn_create_br(&results[i].addr, BT_BR_CONN_PARAM_DEFAULT);

		if (default_conn == NULL) {
			printk("Fail to create the connecton\n");
		} else {
			bt_conn_unref(default_conn);
		}
	}
}

static void discover_work_handler(struct k_work *work)
{
	int err;

	br_discover.length = 10;
	br_discover.limited = false;

	err = bt_br_discovery_start(&br_discover, scan_result,
				    CONFIG_BT_HFP_AG_DISCOVER_RESULT_COUNT);
	if (err) {
		printk("Fail to start discovery (err %d)\n", err);
		return;
	}
}

static void call_connect_work_handler(struct k_work *work)
{
#if CONFIG_BT_HFP_AG_CALL_OUTGOING
	int err;

	printk("Dialing\n");

	err = bt_hfp_ag_outgoing(hfp_ag, "test_hf");

	if (err != 0) {
		printk("Fail to dial a call (err %d)\n", err);
	}
#else
	int err = bt_hfp_ag_remote_incoming(hfp_ag, "test_hf");

	if (err != 0) {
		printk("Fail to set remote incoming call (err %d)\n", err);
	}
#endif /* CONFIG_BT_HFP_AG_CALL_OUTGOING */
}

static void call_disconnect_work_handler(struct k_work *work)
{
	int err;

	if (default_conn != NULL) {
		err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

		if (err != 0) {
			printk("Fail to disconnect acl connection (err %d)\n", err);
		}
	}
}

static void call_remote_ringing_work_handler(struct k_work *work)
{
	int err;

	printk("Remote starts ringing\n");

	err = bt_hfp_ag_remote_ringing(hfp_ag);

	if (err != 0) {
		printk("Fail to notify hfp unit that the remote starts ringing (err %d)\n", err);
	} else {
		k_work_schedule(&call_remote_accept_work, K_SECONDS(1));
	}
}

static void call_remote_accept_work_handler(struct k_work *work)
{
	int err;

	printk("Remote accepts the call\n");

	err = bt_hfp_ag_remote_accept(hfp_ag);

	if (err != 0) {
		printk("Fail to notify hfp unit that the remote accepts call (err %d)\n", err);
	}
}

static struct bt_br_discovery_cb discovery_cb = {
	.recv = discovery_recv_cb,
	.timeout = discovery_timeout_cb,
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

	bt_conn_cb_register(&conn_callbacks);

	bt_br_discovery_cb_register(&discovery_cb);

	bt_hfp_ag_register(&ag_cb);

	k_work_init(&discover_work, discover_work_handler);

	(void)k_work_submit(&discover_work);

	k_work_init_delayable(&call_connect_work, call_connect_work_handler);
	k_work_init_delayable(&call_disconnect_work, call_disconnect_work_handler);

	k_work_init_delayable(&call_remote_ringing_work, call_remote_ringing_work_handler);
	k_work_init_delayable(&call_remote_accept_work, call_remote_accept_work_handler);
}

int main(void)
{
	int err;

	printk("Bluetooth Handsfree AG demo start...\n");

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
	return 0;
}
