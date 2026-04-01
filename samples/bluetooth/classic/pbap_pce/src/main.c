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

static struct bt_conn *default_conn;
static uint16_t l2cap_psm;
static uint16_t rfcomm_channel;

/* Work queue for PBAP connection management */
static struct k_work_delayable pbap_connect_work;
static atomic_t sdp_ready = ATOMIC_INIT(0);

#define PBAP_POOL_BUF_SIZE                                                                         \
	MAX(BT_RFCOMM_BUF_SIZE(CONFIG_BT_GOEP_RFCOMM_MTU),                                         \
	    BT_L2CAP_BUF_SIZE(CONFIG_BT_GOEP_L2CAP_MTU))

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN, PBAP_POOL_BUF_SIZE,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

NET_BUF_POOL_DEFINE(sdp_discover_pool, 10, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

enum __packed bt_pbap_app_state {
	PBAP_STATE_CONNECTED = 0,
	PBAP_STATE_DISCONNECTED_END = 1,
	PBAP_STATE_PULL_PHONEPHOOK_END = 2,
	PBAP_STATE_SET_PHONE_BOOK_FOR_VCARD_LISTING_END = 3,
	PBAP_STATE_SET_PHONE_BOOK_FOR_VCARD_ENTRY_END = 4,
	PBAP_STATE_PULL_VCARD_LISTING_END = 5,
	PBAP_STATE_PULL_VCARD_ENTRY_END = 6,
};

struct app_pbap_pce {
	struct bt_pbap_pce pbap_pce;
	uint16_t peer_mopl;
	uint32_t conn_id;
	struct net_buf *tx_buf;
	enum bt_pbap_app_state current_state;
};
static struct app_pbap_pce app_pce;

static struct bt_sdp_attribute pbap_pce_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_PBAP_PCE_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_PBAP_PCE_SVCLASS)
		},
		)
	),
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
	BT_SDP_SERVICE_NAME("Phonebook Access PCE"),
};

static struct bt_sdp_record pbap_pce_rec = BT_SDP_RECORD(pbap_pce_attrs);

struct pbap_pce_test_state {
	enum bt_pbap_app_state current_state;
	char *name;
	void (*pbap_pce_test_func)(struct app_pbap_pce *pce, char *name);
};

static uint8_t pbap_pce_ascii_to_unicode(const char *src, uint8_t *dest, uint16_t dest_size,
					 bool big_endian)
{
	uint8_t src_len;
	uint8_t required_size;
	uint8_t i;

	if (src == NULL || dest == NULL) {
		return 0;
	}

	src_len = strlen(src);
	required_size = (src_len + 1U) * 2U;

	if (dest_size < required_size) {
		printk("Buffer too small: required %u, available %u", required_size, dest_size);
		return 0;
	}

	if (big_endian) {
		for (i = 0; i < src_len; i++) {
			dest[i * 2U] = 0x00;
			dest[i * 2U + 1U] = (uint8_t)src[i];
		}
	} else {
		for (i = 0; i < src_len; i++) {
			dest[i * 2U] = (uint8_t)src[i];
			dest[i * 2U + 1U] = 0x00;
		}
	}

	dest[src_len * 2] = 0x00;
	dest[src_len * 2 + 1] = 0x00;

	return required_size;
}

static int pbap_alloc_tx_buf(struct app_pbap_pce *app_pce)
{

	if (app_pce->tx_buf != NULL) {
		printk("Buf %p is using\n", app_pce->tx_buf);
		return -EBUSY;
	}

	app_pce->tx_buf = bt_pbap_pce_create_pdu(&app_pce->pbap_pce, &tx_pool);

	if (app_pce->tx_buf == NULL) {
		printk("Fail to allocate tx buffer\n");
		return -ENOBUFS;
	}
	return 0;
}

static int pbap_pce_add_initial_headers(struct app_pbap_pce *app_pce, const char *type,
					const char *name)
{
	int err;
	int length;
	uint8_t unicode_trans[60] = {0};
	bool is_goep_v2;

	is_goep_v2 = app_pce->pbap_pce._goep._goep_v2;

	err = pbap_alloc_tx_buf(app_pce);
	if (err != 0) {
		printk("Fail to allocate tx buffer %d\n", err);
		goto failed;
	}

	err = bt_obex_add_header_conn_id(app_pce->tx_buf, app_pce->conn_id);
	if (err != 0) {
		printk("Fail to add connection id %d", err);
		goto failed;
	}

	if (is_goep_v2) {
		err = bt_obex_add_header_srm(app_pce->tx_buf, 0x01);
		if (err != 0) {
			printk("Fail to add srm header %d", err);
			goto failed;
		}
	}

	length = pbap_pce_ascii_to_unicode(name, unicode_trans, sizeof(unicode_trans), true);
	err = bt_obex_add_header_name(app_pce->tx_buf, length, unicode_trans);
	if (err != 0) {
		printk("Fail to add name header %d", err);
		goto failed;
	}

	/* Add type header */
	err = bt_obex_add_header_type(app_pce->tx_buf, strlen(type) + 1U, type);
	if (err != 0) {
		printk("Fail to add type header %d", err);
		goto failed;
	}

	return 0;

failed:
	net_buf_unref(app_pce->tx_buf);
	app_pce->tx_buf = NULL;
	return err;
}

static void pbap_pce_test_func_pull_phonebook(struct app_pbap_pce *app_pce, char *name)
{
	int err;

	err = pbap_pce_add_initial_headers(app_pce, BT_PBAP_PULL_PHONE_BOOK_TYPE, name);
	if (err != 0) {
		return;
	}

	err = bt_pbap_pce_pull_phonebook(&app_pce->pbap_pce, app_pce->tx_buf);
	if (err != 0) {
		printk("Fail to pull phonebook %d\n", err);
	}

	app_pce->tx_buf = NULL;
	return;
}

static void pbap_pce_test_func_set_phonebook(struct app_pbap_pce *app_pce, char *name)
{
	uint16_t len = 0;
	int err;
	uint8_t unicode_trans[50] = {0};

	err = pbap_alloc_tx_buf(app_pce);
	if (err != 0) {
		printk("Fail to allocate tx buffer %d\n", err);
		return;
	}

	len = pbap_pce_ascii_to_unicode((uint8_t *)name, (uint8_t *)unicode_trans,
					sizeof(unicode_trans), true);
	err = bt_obex_add_header_conn_id(app_pce->tx_buf, app_pce->conn_id);
	if (err != 0) {
		printk("Fail to add connection id %d", err);
		goto failed;
	}

	err = bt_obex_add_header_name(app_pce->tx_buf, len, unicode_trans);
	if (err != 0) {
		printk("Fail to add name header %d", err);
		goto failed;
	}

	err = bt_pbap_pce_set_phone_book(
		&app_pce->pbap_pce, BT_PBAP_SET_PHONE_BOOK_FLAGS_DOWN_OR_ROOT, app_pce->tx_buf);
	if (err != 0) {
		printk("Fail to create set path cmd %d", err);
		goto failed;
	}

	app_pce->tx_buf = NULL;
	return;
failed:
	net_buf_unref(app_pce->tx_buf);
	app_pce->tx_buf = NULL;
	return;
}

static void pbap_pce_test_func_pull_vcard_listing(struct app_pbap_pce *app_pce, char *name)
{
	int err;

	err = pbap_pce_add_initial_headers(app_pce, BT_PBAP_PULL_VCARD_LISTING_TYPE, name);
	if (err != 0) {
		return;
	}

	err = bt_pbap_pce_pull_vcard_listing(&app_pce->pbap_pce, app_pce->tx_buf);
	if (err != 0) {
		printk("Fail to pull vcard listing %d\n", err);
	}

	app_pce->tx_buf = NULL;
	return;
}

static void pbap_pce_test_func_pull_vcard_entry(struct app_pbap_pce *app_pce, char *name)
{
	int err;

	err = pbap_pce_add_initial_headers(app_pce, BT_PBAP_PULL_VCARD_ENTRY_TYPE, name);
	if (err != 0) {
		return;
	}

	err = bt_pbap_pce_pull_vcard_entry(&app_pce->pbap_pce, app_pce->tx_buf);
	if (err != 0) {
		printk("Fail to pull vcard entry %d\n", err);
	}

	app_pce->tx_buf = NULL;
	return;
}

#define PBAP_PCE_PULL_PHONE_BOOK_NAME                  "telecom/pb.vcf"
#define PBAP_PCE_PULL_VCARD_LISTING_NAME               "pb"
#define PBAP_PCE_SET_PHONE_BOOK_NAME_FOR_VCARD_LISTING "telecom"
#define PBAP_PCE_SET_PHONE_BOOK_NAME_FOR_VCARD_ENTRY   "pb"
#define PBAP_PCE_PULL_VCARD_ENTRY_NAME                 "0.vcf"

struct pbap_pce_test_state test_case[] = {
	{PBAP_STATE_CONNECTED, PBAP_PCE_PULL_PHONE_BOOK_NAME, pbap_pce_test_func_pull_phonebook},
	{PBAP_STATE_PULL_PHONEPHOOK_END, PBAP_PCE_SET_PHONE_BOOK_NAME_FOR_VCARD_LISTING,
	 pbap_pce_test_func_set_phonebook},
	{PBAP_STATE_SET_PHONE_BOOK_FOR_VCARD_LISTING_END, PBAP_PCE_PULL_VCARD_LISTING_NAME,
	 pbap_pce_test_func_pull_vcard_listing},
	{PBAP_STATE_PULL_VCARD_LISTING_END, PBAP_PCE_SET_PHONE_BOOK_NAME_FOR_VCARD_ENTRY,
	 pbap_pce_test_func_set_phonebook},
	{PBAP_STATE_SET_PHONE_BOOK_FOR_VCARD_ENTRY_END, PBAP_PCE_PULL_VCARD_ENTRY_NAME,
	 pbap_pce_test_func_pull_vcard_entry},
};

static void pbap_pce_process_test()
{
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(test_case); i++) {
		if (app_pce.current_state == test_case[i].current_state) {
			if (test_case[i].pbap_pce_test_func != NULL) {
				test_case[i].pbap_pce_test_func(&app_pce, test_case[i].name);
			}
			break;
		}
	}
}

static void pbap_pce_connected(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code, uint8_t version,
			       uint16_t mopl, struct net_buf *buf)
{
	int err;
	struct app_pbap_pce *app_pce = CONTAINER_OF(pbap_pce, struct app_pbap_pce, pbap_pce);

	printk("pbap connect result %s, mopl %d\n", bt_obex_rsp_code_to_str(rsp_code), mopl);
	app_pce->peer_mopl = mopl;
	err = bt_obex_get_header_conn_id(buf, &app_pce->conn_id);
	if (err != 0) {
		printk("No connection ID header found (err: %d)", err);
		return;
	}
	app_pce->current_state = PBAP_STATE_CONNECTED;
	pbap_pce_process_test();
}

struct pbap_hdr {
	const uint8_t *value;
	uint16_t length;
};

static int pce_get_body_data(uint8_t rsp_code, struct net_buf *buf, struct pbap_hdr *body)
{
	int err;

	if (rsp_code == BT_PBAP_RSP_CODE_CONTINUE) {
		err = bt_obex_get_header_body(buf, &body->length, &body->value);
	} else {
		err = bt_obex_get_header_end_body(buf, &body->length, &body->value);
	}

	return err;
}

static void pbap_pce_pull_phonebook_cb(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
				       struct net_buf *buf)
{
	struct pbap_hdr body;
	int err;
	struct app_pbap_pce *app_pce = CONTAINER_OF(pbap_pce, struct app_pbap_pce, pbap_pce);

	body.length = 0;
	body.value = NULL;

	printk("pbap pull phone book result %s\n", bt_obex_rsp_code_to_str(rsp_code));
	if (rsp_code != BT_PBAP_RSP_CODE_SUCCESS && rsp_code != BT_PBAP_RSP_CODE_CONTINUE) {
		return;
	}

	pce_get_body_data(rsp_code, buf, &body);
	if (body.length > 0 && body.value != NULL) {
		printk("=========body=========\n");
		printk("%.*s\n", body.length, body.value);
		printk("=========body=========\n");
	}

	if (rsp_code == BT_PBAP_RSP_CODE_SUCCESS) {
		app_pce->current_state = PBAP_STATE_PULL_PHONEPHOOK_END;
		pbap_pce_process_test();
	} else {
		if (!pbap_pce->_goep._goep_v2) {
			err = pbap_alloc_tx_buf(app_pce);
			if (err != 0) {
				printk("Fail to allocate tx buffer %d\n", err);
				return;
			}

			err = bt_pbap_pce_pull_phonebook(&app_pce->pbap_pce, app_pce->tx_buf);
			if (err != 0) {
				net_buf_unref(app_pce->tx_buf);
				printk("Fail to pull phonebook %d\n", err);
			}
			app_pce->tx_buf = NULL;
			return;
		}
	}
}

static void pbap_pce_pull_vcardlisting_cb(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
					  struct net_buf *buf)
{
	struct pbap_hdr body;
	int err;
	struct app_pbap_pce *app_pce = CONTAINER_OF(pbap_pce, struct app_pbap_pce, pbap_pce);

	body.length = 0;
	body.value = NULL;

	printk("pbap pull vcard listing result %s\n", bt_obex_rsp_code_to_str(rsp_code));
	if (rsp_code != BT_PBAP_RSP_CODE_SUCCESS && rsp_code != BT_PBAP_RSP_CODE_CONTINUE) {
		return;
	}

	pce_get_body_data(rsp_code, buf, &body);
	if (body.length > 0 && body.value != NULL) {
		printk("=========body=========\n");
		printk("%.*s\n", body.length, body.value);
		printk("=========body=========\n");
	}

	if (rsp_code == BT_PBAP_RSP_CODE_SUCCESS) {
		app_pce->current_state = PBAP_STATE_PULL_VCARD_LISTING_END;
		pbap_pce_process_test();
	} else {
		if (!pbap_pce->_goep._goep_v2) {
			err = pbap_alloc_tx_buf(app_pce);
			if (err != 0) {
				printk("Fail to allocate tx buffer %d\n", err);
				return;
			}

			err = bt_pbap_pce_pull_vcard_listing(&app_pce->pbap_pce, app_pce->tx_buf);
			if (err != 0) {
				net_buf_unref(app_pce->tx_buf);
				printk("Fail to pull vcard listing %d\n", err);
			}
			app_pce->tx_buf = NULL;
			return;
		}
	}
}

static void pbap_pce_pull_vcardentry_cb(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
					struct net_buf *buf)
{
	struct pbap_hdr body;
	int err;
	struct app_pbap_pce *app_pce = CONTAINER_OF(pbap_pce, struct app_pbap_pce, pbap_pce);

	body.length = 0;
	body.value = NULL;

	printk("pbap pull vcard entry result %s\n", bt_obex_rsp_code_to_str(rsp_code));
	if (rsp_code != BT_PBAP_RSP_CODE_SUCCESS && rsp_code != BT_PBAP_RSP_CODE_CONTINUE) {
		return;
	}

	pce_get_body_data(rsp_code, buf, &body);
	if (body.length > 0 && body.value != NULL) {
		printk("=========body=========\n");
		printk("%.*s\n", body.length, body.value);
		printk("=========body=========\n");
	}

	if (rsp_code == BT_PBAP_RSP_CODE_SUCCESS) {
		err = bt_pbap_pce_disconnect(&app_pce->pbap_pce, NULL);
		if (err != 0) {
			printk("Fail to disconnect %d\n", err);
		}
	} else {
		if (!pbap_pce->_goep._goep_v2) {
			err = pbap_alloc_tx_buf(app_pce);
			if (err != 0) {
				printk("Fail to allocate tx buffer %d\n", err);
				return;
			}
			err = bt_pbap_pce_pull_vcard_entry(&app_pce->pbap_pce, app_pce->tx_buf);
			if (err != 0) {
				net_buf_unref(app_pce->tx_buf);
				printk("Fail to pull vcard entry %d\n", err);
			}
			app_pce->tx_buf = NULL;
			return;
		}
	}
}

static void pbap_pce_set_phone_book_cb(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
				       struct net_buf *buf)
{
	static uint8_t set_phone_book_count = 0;
	struct app_pbap_pce *app_pce = CONTAINER_OF(pbap_pce, struct app_pbap_pce, pbap_pce);

	printk("PBAP set phonebook result %s\n", bt_obex_rsp_code_to_str(rsp_code));
	if (set_phone_book_count == 0) {
		set_phone_book_count++;
		app_pce->current_state = PBAP_STATE_SET_PHONE_BOOK_FOR_VCARD_LISTING_END;
	} else {
		set_phone_book_count = 0;
		app_pce->current_state = PBAP_STATE_SET_PHONE_BOOK_FOR_VCARD_ENTRY_END;
	}
	pbap_pce_process_test();
}

static void pbap_pce_disconnected(struct bt_pbap_pce *pbap, uint8_t rsp_code, struct net_buf *buf)
{
	printk("pbap disconnect result %s\n", bt_obex_rsp_code_to_str(rsp_code));
}

static int pbap_pce_obex_connect(struct app_pbap_pce *app_pbap_pce, uint16_t mopl)
{
	int err;
	struct bt_obex_tlv appl_param;

	err = pbap_alloc_tx_buf(app_pbap_pce);
	if (err != 0) {
		printk("Fail to allocate tx buffer %d\n", err);
		return err;
	}

	printk("create pbap obex connection\n");

	if (app_pbap_pce->pbap_pce._goep._goep_v2) {
		uint32_t supported_features = 0x0000003ff;
		appl_param.type = BT_PBAP_APPL_PARAM_TAG_ID_SUPPORTED_FEATURES;
		appl_param.data_len = 4;
		appl_param.data = (uint8_t *)&supported_features;
		err = bt_obex_add_header_app_param(app_pbap_pce->tx_buf, 1, &appl_param);
		if (err != 0) {
			printk("Fail to add application parameter %d\n", err);
			return err;
		}
	}

	err = bt_pbap_pce_connect(&app_pbap_pce->pbap_pce, mopl, app_pbap_pce->tx_buf);
	if (err != 0) {
		printk("Fail to connect %d\n", err);
	}

	app_pbap_pce->tx_buf = NULL;
	return err;
}

static void pbap_pce_rfcomm_transport_connected(struct bt_conn *conn, struct bt_pbap_pce *pbap_pce)
{
	int err;
	struct app_pbap_pce *app_pce = CONTAINER_OF(pbap_pce, struct app_pbap_pce, pbap_pce);

	printk("PBAP PCE %p rfcomm transport connected on %p\n", pbap_pce, conn);
	err = pbap_pce_obex_connect(app_pce, 300);
	if (err != 0) {
		printk("Fail to obex connect %d\n", err);
	}

	return;
}

static void pbap_pce_rfcomm_transport_disconnected(struct bt_pbap_pce *pbap_pce)
{
	struct app_pbap_pce *app_pce = CONTAINER_OF(pbap_pce, struct app_pbap_pce, pbap_pce);

	printk("PBAP PCE %p rfcomm transport disconnected\n", pbap_pce);
	app_pce->tx_buf = NULL;
}

static void pbap_pce_l2cap_transport_connected(struct bt_conn *conn, struct bt_pbap_pce *pbap_pce)
{
	int err;
	struct app_pbap_pce *app_pce = CONTAINER_OF(pbap_pce, struct app_pbap_pce, pbap_pce);

	printk("PBAP PCE %p l2cap transport connected on %p\n", pbap_pce, conn);
	err = pbap_pce_obex_connect(app_pce, 256);
	if (err != 0) {
		printk("Fail to obex connect %d\n", err);
	}

	return;
}

static void pbap_pce_l2cap_transport_disconnected(struct bt_pbap_pce *pbap_pce)
{
	struct app_pbap_pce *app_pce = CONTAINER_OF(pbap_pce, struct app_pbap_pce, pbap_pce);

	printk("PBAP PCE %p l2cap transport disconnected\n", pbap_pce);
	app_pce->tx_buf = NULL;
}

static struct bt_pbap_pce_cb pce_cb = {
	.rfcomm_connected = pbap_pce_rfcomm_transport_connected,
	.rfcomm_disconnected = pbap_pce_rfcomm_transport_disconnected,
	.l2cap_connected = pbap_pce_l2cap_transport_connected,
	.l2cap_disconnected = pbap_pce_l2cap_transport_disconnected,
	.connect = pbap_pce_connected,
	.disconnect = pbap_pce_disconnected,
	.pull_phone_book = pbap_pce_pull_phonebook_cb,
	.pull_vcard_listing = pbap_pce_pull_vcardlisting_cb,
	.pull_vcard_entry = pbap_pce_pull_vcardentry_cb,
	.set_phone_book = pbap_pce_set_phone_book_cb,
};

static int pbap_sdp_get_goep_l2cap_psm(const struct net_buf *buf, uint16_t *psm)
{
	int err;
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_GOEP_L2CAP_PSM, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if ((value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) || (value.uint.size != sizeof(*psm))) {
		return -EINVAL;
	}

	*psm = value.uint.u16;
	return 0;
}

static uint8_t sdp_discover_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
			       const struct bt_sdp_discover_params *params)
{
	int err;

	printk("Discover done\n");

	if (result->resp_buf != NULL) {
		l2cap_psm = 0;
		rfcomm_channel = 0;

		err = pbap_sdp_get_goep_l2cap_psm(result->resp_buf, &l2cap_psm);
		if (err != 0) {
			printk("PSE l2cap PSM not found, err %d\n", err);
		}

		err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM,
					     &rfcomm_channel);
		if (err != 0) {
			printk("PSE Rfcomm channel not found, err %d\n", err);
			return BT_SDP_DISCOVER_UUID_CONTINUE;
		}

		if (l2cap_psm != 0) {
			printk("Creating transport connection via L2CAP PSM: %d\n", l2cap_psm);
			err = bt_pbap_pce_l2cap_connect(default_conn, &app_pce.pbap_pce, &pce_cb,
							l2cap_psm);
			if (err != 0) {
				printk("Failed to connect PBAP PCE over L2CAP, err %d\n", err);
			}
		} else {
			printk("Creating transport connection via RFCOMM channel: %d\n",
			       rfcomm_channel);
			err = bt_pbap_pce_rfcomm_connect(default_conn, &app_pce.pbap_pce, &pce_cb,
							 rfcomm_channel);
			if (err != 0) {
				printk("Failed to connect PBAP PCE over RFCOMM, err %d\n", err);
			}
		}
	}

	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

static struct bt_sdp_discover_params sdp_discover = {
	.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR,
	.func = sdp_discover_cb,
	.pool = &sdp_discover_pool,
	.uuid = BT_UUID_DECLARE_16(BT_SDP_PBAP_PSE_SVCLASS),
};

static void pbap_sdp_discovery_work_handler(struct k_work *work)
{
	int err;
	bool sdp_ok;

	sdp_ok = atomic_get(&sdp_ready);
	if (sdp_ok) {
		return;
	}

	err = bt_sdp_discover(default_conn, &sdp_discover);
	if (err != 0) {
		printk("SDP discovery failed (err %d)\r\n", err);
	} else {
		printk("SDP discovery started\r\n");
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info;

	if (err != 0) {
		printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
	}

	bt_conn_get_info(conn, &info);
	if (info.type != BT_CONN_TYPE_BR) {
		return;
	}

	printk("ACL Connected\n");
	default_conn = conn;

	atomic_set(&sdp_ready, 0);
	l2cap_psm = 0;
	rfcomm_channel = 0;

	k_work_schedule(&pbap_connect_work, K_SECONDS(5));
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

static void hf_connected(struct bt_conn *conn, struct bt_hfp_hf *hf)
{
	int err;
	printk("HFP HF Connected!\n");

	err = bt_sdp_discover(conn, &sdp_discover);
	if (err != 0) {
		printk("SDP discovery failed (err %d)\r\n", err);
		return;
	} else {
		printk("SDP discovery started\r\n");
	}
	atomic_set(&sdp_ready, 1);
}

static void hf_disconnected(struct bt_hfp_hf *hf)
{
	printk("HFP HF Disconnected!\n");
}

static struct bt_hfp_hf_cb hf_cb = {
	.connected = hf_connected,
	.disconnected = hf_disconnected,
};

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

	err = bt_hfp_hf_register(&hf_cb);
	if (err < 0) {
		printk("HFP HF Registration failed (err %d)\n", err);
	}

	err = bt_sdp_register_service(&pbap_pce_rec);
	if (err != 0) {
		printk("Failed to register SDP record");
		return;
	}

	bt_conn_cb_register(&conn_callbacks);

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

	k_work_init_delayable(&pbap_connect_work, pbap_sdp_discovery_work_handler);
}

int main(void)
{
	int err;

        printk("PBAP PCE demo started\n");
	err = bt_enable(bt_ready);
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
	return 0;
}
