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
#include <zephyr/bluetooth/classic/pbap.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/settings/settings.h>

static uint8_t rfcomm_channel = CONFIG_BT_PBAP_PSE_RFCOMM_CHANNEL;
static uint16_t l2cap_psm = CONFIG_BT_PBAP_PSE_L2CAP_PSM;
static uint8_t supported_repositories = CONFIG_BT_PBAP_PSE_SUPPORTED_REPOSITORIES;
static uint32_t supported_features = CONFIG_BT_PBAP_PSE_SUPPORTED_FEATURES;

static struct bt_pbap_pse_l2cap l2cap_server;
static struct bt_conn *default_conn;
static struct bt_br_discovery_param br_discover;
static struct bt_br_discovery_result scan_result[CONFIG_BT_PBAP_PSE_DISCOVER_RESULT_COUNT];

static struct k_work discover_work;

#define PBAP_POOL_BUF_SIZE                                                                         \
	MAX(BT_RFCOMM_BUF_SIZE(CONFIG_BT_GOEP_RFCOMM_MTU),                                         \
	    BT_L2CAP_BUF_SIZE(CONFIG_BT_GOEP_L2CAP_MTU))

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN, PBAP_POOL_BUF_SIZE,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

struct app_pbap_pse {
	struct bt_pbap_pse pbap_pse;
	uint32_t conn_id;
	struct net_buf *tx_buf;
};
static struct app_pbap_pse app_pse;

static struct bt_sdp_attribute pbap_pse_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_PBAP_PSE_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 17),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
				&rfcomm_channel
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_OBEX)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("Phonebook Access PSE"),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PBAP_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0102)
			},
			)
		},
		)
	),
	{
		BT_SDP_ATTR_GOEP_L2CAP_PSM,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			&l2cap_psm
		}
	},
	{
		BT_SDP_ATTR_SUPPORTED_REPOSITORIES,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
			&supported_repositories
		}
	},
	{
		BT_SDP_ATTR_PBAP_SUPPORTED_FEATURES,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT32),
			&supported_features
		}
	},
};

static struct bt_sdp_record pbap_pse_rec = BT_SDP_RECORD(pbap_pse_attrs);

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		if (default_conn != NULL) {
			default_conn = NULL;
		}
		printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
	} else {
		if (default_conn == conn) {
			struct bt_conn_info info;

			bt_conn_get_info(conn, &info);
			if (info.type != BT_CONN_TYPE_BR) {
				return;
			}

			printk("Connected\n");
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));

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

	printk("Security changed: %s level %u, err %s(%d)\n", addr, level,
	       bt_security_err_to_str(err), err);
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
		printk("Device[%d]: %s, rssi %d, cod 0x%02x%02x%02x", i, addr, results[i].rssi,
		       results[i].cod[0], results[i].cod[1], results[i].cod[2]);

		major_device = (uint8_t)BT_COD_MAJOR_DEVICE_CLASS(results[i].cod);
		minor_device = (uint8_t)BT_COD_MINOR_DEVICE_CLASS(results[i].cod);

		if ((major_device & BT_COD_MAJOR_DEVICE_CLASS_AUDIO_VIDEO) &&
		    (minor_device & BT_COD_MINOR_DEVICE_CLASS_AUDIO_VIDEO_HANDSFREE)) {
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
			default:
				/* Skip the EIR */
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

static struct bt_br_discovery_cb discovery_cb = {
	.recv = discovery_recv_cb,
	.timeout = discovery_timeout_cb,
};

static void discover_work_handler(struct k_work *work)
{
	int err;

	br_discover.length = 10;
	br_discover.limited = false;

	err = bt_br_discovery_start(&br_discover, scan_result,
				    CONFIG_BT_PBAP_PSE_DISCOVER_RESULT_COUNT);
	if (err != 0) {
		printk("Fail to start discovery (err %d)\n", err);
		return;
	}
}

static int pbap_alloc_tx_buf(struct app_pbap_pse *app_pse)
{
	if (app_pse->tx_buf != NULL) {
		printk("Buf %p is using\n", app_pse->tx_buf);
		return -EBUSY;
	}

	app_pse->tx_buf = bt_pbap_pse_create_pdu(&app_pse->pbap_pse, &tx_pool);

	if (app_pse->tx_buf == NULL) {
		printk("Fail to allocate tx buffer\n");
		return -ENOBUFS;
	}
	return 0;
}

static void pbap_pse_connected(struct bt_pbap_pse *pbap_pse, uint8_t version, uint16_t mopl,
			       struct net_buf *buf)
{
        int err;
	struct app_pbap_pse *app_pse = CONTAINER_OF(pbap_pse, struct app_pbap_pse, pbap_pse);

	printk("pbap connect version %u, mopl %d\n", version, mopl);

        err = pbap_alloc_tx_buf(app_pse);
        if (err != 0) {
                printk("Fail to allocate tx buffer\n");
                return;
        }

	err = bt_pbap_pse_connect_rsp(&app_pse->pbap_pse, 300, BT_PBAP_RSP_CODE_SUCCESS, app_pse->tx_buf);
	if (err != 0) {
		printk("Fail to send conn rsp %d\n", err);
	}
        app_pse->tx_buf = NULL;
}

static const char pbap_pse_phonebook_example[] = "BEGIN:VCARD\n"
						 "VERSION:2.1\n"
						 "FN;CHARSET=UTF-8:descvs\n"
						 "N;CHARSET=UTF-8:descvs\n"
						 "END:VCARD";

static const char pbap_pse_vcard_listing[] =
	"<?xml version=\"1.0\"?><!DOCTYPE vcard-listing SYSTEM \"vcard-listing.dtd\">"
	"<vCard-listing version=\"1.0\"><card handle=\"1.vcf\" name=\"qwe\"/>"
	"<card handle=\"2.vcf\" name=\"qwe\"/>/vCard-listing>";

static const char pbap_pse_vcard_entry[] = "BEGIN:VCARD\n"
					   "VERSION:2.1\n"
					   "FN:\n"
					   "N:\n"
					   "TEL;X-0:1155\n"
					   "X-IRMC-CALL-DATETIME;DIALED:20220913T110607\n"
					   "END:VCARD";

typedef int (*operation_func)(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code, struct net_buf *buf);

static int send_body_chunk(struct app_pbap_pse *app_pse, uint16_t *sent_length,
				uint16_t *remain_length, uint16_t chunk_size, operation_func func, const char data[])
{
	uint8_t rsp_code;
	uint16_t bytes_to_send;
	int err;

	if (*remain_length <= chunk_size) {
		bytes_to_send = *remain_length;
		rsp_code = BT_PBAP_RSP_CODE_SUCCESS;
	} else {
		bytes_to_send = chunk_size;
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

	if (rsp_code == BT_PBAP_RSP_CODE_SUCCESS) {
		err = bt_obex_add_header_end_body(app_pse->tx_buf, bytes_to_send,
						  (uint8_t *)&data[*sent_length]);
	} else {
		err = bt_obex_add_header_body(app_pse->tx_buf, bytes_to_send,
					      (uint8_t *)&data[*sent_length]);
	}

	if (err != 0) {
		printk("Fail to add body header %d\n", err);
		return err;
	}

	err = func(&app_pse->pbap_pse, rsp_code, app_pse->tx_buf);
	if (err != 0) {
		printk("Fail to send pull phonebook rsp %d\n", err);
		return err;
	}

	*sent_length += bytes_to_send;
	*remain_length -= bytes_to_send;
	app_pse->tx_buf = NULL;

	return 0;
}

static void pbap_pse_pull_rsp(struct bt_pbap_pse *pbap_pse, struct net_buf *buf, operation_func func, const char data[])
{
	struct app_pbap_pse *app_pse = CONTAINER_OF(pbap_pse, struct app_pbap_pse, pbap_pse);
	const uint16_t body_length = strlen(data);
	const uint16_t chunk_size = 255;
	uint16_t sent_length = 0;
	uint16_t remain_length = body_length;
	int err;

	err = pbap_alloc_tx_buf(app_pse);
	if (err != 0) {
		printk("Fail to allocate tx buffer\n");
		return;
	}

	if (pbap_pse->_goep._goep_v2 && (atomic_get(&pbap_pse->_flags) == 0)) {
		err = bt_obex_add_header_srm(app_pse->tx_buf, 0x01);
		if (err != 0) {
			printk("Fail to add SRM header %d\n", err);
			goto fail;
		}
	}
	err = send_body_chunk(app_pse, &sent_length, &remain_length, chunk_size, func, data);
	if (err != 0) {
		goto fail;
	}

	if (remain_length > 0 && pbap_pse->_goep._goep_v2) {
		while (remain_length > 0) {
			err = pbap_alloc_tx_buf(app_pse);
			if (err != 0) {
				printk("Fail to allocate tx buffer\n");
				return;
			}

			err = send_body_chunk(app_pse, &sent_length, &remain_length, chunk_size, func, data);
			if (err != 0) {
				goto fail;
			}
		}
	}

	return;

fail:
	if (app_pse->tx_buf != NULL) {
		net_buf_unref(app_pse->tx_buf);
		app_pse->tx_buf = NULL;
	}
}


static void pbap_pse_pull_phonebook_cb(struct bt_pbap_pse *pbap_pse, struct net_buf *buf)
{
	printk("pbap_pse get pull_phone_book request\n");
        pbap_pse_pull_rsp(pbap_pse, buf, bt_pbap_pse_pull_phone_book_rsp, pbap_pse_phonebook_example);
}

static void pbap_pse_pull_vcard_listing_cb(struct bt_pbap_pse *pbap_pse, struct net_buf *buf)
{
	printk("pbap_pse get pull_vcard_listing request\n");
        pbap_pse_pull_rsp(pbap_pse, buf, bt_pbap_pse_pull_vcard_listing_rsp, pbap_pse_vcard_listing);
}

static void pbap_pse_pull_vcard_entry_cb(struct bt_pbap_pse *pbap_pse, struct net_buf *buf)
{
	printk("pbap_pse get pull_vcard_entry request\n");
        pbap_pse_pull_rsp(pbap_pse, buf, bt_pbap_pse_pull_vcard_entry_rsp, pbap_pse_vcard_entry);
}

static void pbap_pse_set_phone_book_cb(struct bt_pbap_pse *pbap_pse, uint8_t flags,
				       struct net_buf *buf)
{
        int err;

	err = bt_pbap_pse_set_phone_book_rsp(pbap_pse, BT_PBAP_RSP_CODE_SUCCESS, NULL);
	if (err != 0) {
		printk("Fail to send set phone book rsp %d\n", err);
	}
}

static void pbap_pse_disconnected(struct bt_pbap_pse *pbap_pse, struct net_buf *buf)
{

        int err;

	printk("pbap disconnect requested by pce\n");

	err = bt_pbap_pse_disconnect_rsp(pbap_pse, BT_PBAP_RSP_CODE_SUCCESS, NULL);
	if (err != 0) {
		printk("Fail to send set phone book rsp %d\n", err);
	}
}

static void pbap_pse_l2cap_transport_connected(struct bt_conn *conn, struct bt_pbap_pse *pbap_pse)
{
	printk("PBAP PSE %p l2cap transport connected on %p\n", pbap_pse, conn);
}

static void pbap_pse_l2cap_transport_disconnected(struct bt_pbap_pse *pbap_pse)
{
	printk("PBAP PSE %p l2cap transport disconnected\n", pbap_pse);
}


static struct bt_pbap_pse_cb pse_cb = {
	.l2cap_connected = pbap_pse_l2cap_transport_connected,
	.l2cap_disconnected = pbap_pse_l2cap_transport_disconnected,
	.connect = pbap_pse_connected,
	.disconnect = pbap_pse_disconnected,
	.pull_phone_book = pbap_pse_pull_phonebook_cb,
	.pull_vcard_listing = pbap_pse_pull_vcard_listing_cb,
	.pull_vcard_entry = pbap_pse_pull_vcard_entry_cb,
	.set_phone_book = pbap_pse_set_phone_book_cb,
};


static int l2cap_accept(struct bt_conn *conn, struct bt_pbap_pse_l2cap *pbap_pse_l2cap,
			struct bt_pbap_pse **pbap_pse)
{
	*pbap_pse = &app_pse.pbap_pse;

	return 0;
}

static int pbap_register()
{
	int err;

	err = bt_sdp_register_service(&pbap_pse_rec);
	if (err != 0) {
		printk("Failed to register SDP record\n");
		return err;
	}

	if (l2cap_server.server.l2cap.psm != 0) {
		printk("l2cap has been registered\n");
		return -EBUSY;
	}

	l2cap_server.server.l2cap.psm = CONFIG_BT_PBAP_PSE_L2CAP_PSM;
	l2cap_server.accept = l2cap_accept;
	err = bt_pbap_pse_l2cap_register(&l2cap_server);
	if (err != 0) {
		printk("Fail to register L2CAP server (error %d)\n", err);
		l2cap_server.server.l2cap.psm = 0;
		return err;
	}
	printk("L2cap server (psm %02x) is registered\n", l2cap_server.server.l2cap.psm);

	err = bt_pbap_pse_register(&app_pse.pbap_pse, &pse_cb);
	if (err != 0) {
		printk("Fail to register server %d\n", err);
                return err;
	}
	printk("PBAP PSE is registered\n");
	return 0;
}

static void bt_ready(int err)
{
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	printk("Bluetooth initialized\n");

        err = pbap_register();
        if (err != 0) {
                return;
        }

	bt_conn_cb_register(&conn_callbacks);

	bt_br_discovery_cb_register(&discovery_cb);

	k_work_init(&discover_work, discover_work_handler);

	(void)k_work_submit(&discover_work);

	err = bt_br_set_connectable(true, NULL);
	if (err != 0) {
		printk("BR/EDR set/rest connectable failed (err %d)\n", err);
		return;
	}
	err = bt_br_set_discoverable(true, false);
	if (err != 0) {
		printk("BR/EDR set discoverable failed (err %d)\n", err);
		return;
	}

	printk("BR/EDR set connectable and discoverable done\n");
}

int main(void)
{
	int err;

	err = bt_enable(bt_ready);
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
	return 0;
}