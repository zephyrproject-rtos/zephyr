/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

extern enum bst_result_t bst_result;

CREATE_FLAG(flag_is_chrc_ctx_validated);

static struct bt_conn *g_conn;

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);

	g_conn = bt_conn_ref(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != g_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(g_conn);

	g_conn = NULL;
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

struct test_chrc_ctx {
	uint16_t auth_read_cnt;
	uint16_t read_cnt;
	uint16_t auth_write_cnt;
	uint16_t write_cnt;
	uint8_t data[CHRC_SIZE];
};

static ssize_t read_test_chrc(struct test_chrc_ctx *chrc_ctx,
			      struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      void *buf, uint16_t len, uint16_t offset)
{
	chrc_ctx->read_cnt++;

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 (void *)chrc_ctx->data,
				 sizeof(chrc_ctx->data));
}

static ssize_t write_test_chrc(struct test_chrc_ctx *chrc_ctx,
			       const void *buf, uint16_t len,
			       uint16_t offset, uint8_t flags)
{
	chrc_ctx->write_cnt++;

	if (len != sizeof(chrc_ctx->data)) {
		printk("Invalid chrc length\n");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (offset != 0) {
		printk("Invalid chrc offset and length\n");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (flags != 0) {
		FAIL("Invalid flags %u\n", flags);
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	(void)memcpy(chrc_ctx->data, buf, len);

	return len;
}

static struct test_chrc_ctx unhandled_chrc_ctx;

static ssize_t read_test_unhandled_chrc(struct bt_conn *conn,
					 const struct bt_gatt_attr *attr,
					 void *buf, uint16_t len, uint16_t offset)
{
	return read_test_chrc(&unhandled_chrc_ctx, conn, attr, buf, len, offset);
}

static ssize_t write_test_unhandled_chrc(struct bt_conn *conn,
					  const struct bt_gatt_attr *attr,
					  const void *buf, uint16_t len,
					  uint16_t offset, uint8_t flags)
{
	printk("unhandled chrc len %u offset %u\n", len, offset);

	return write_test_chrc(&unhandled_chrc_ctx, buf, len, offset, flags);
}

static struct test_chrc_ctx unauthorized_chrc_ctx;

static ssize_t read_test_unauthorized_chrc(struct bt_conn *conn,
					   const struct bt_gatt_attr *attr,
					   void *buf, uint16_t len, uint16_t offset)
{
	return read_test_chrc(&unauthorized_chrc_ctx, conn, attr, buf, len, offset);
}

static ssize_t write_test_unauthorized_chrc(struct bt_conn *conn,
					    const struct bt_gatt_attr *attr,
					    const void *buf, uint16_t len,
					    uint16_t offset, uint8_t flags)
{
	printk("unauthorized chrc len %u offset %u\n", len, offset);

	return write_test_chrc(&unauthorized_chrc_ctx, buf, len, offset, flags);
}

static struct test_chrc_ctx authorized_chrc_ctx;

static ssize_t read_test_authorized_chrc(struct bt_conn *conn,
					 const struct bt_gatt_attr *attr,
					 void *buf, uint16_t len, uint16_t offset)
{
	return read_test_chrc(&authorized_chrc_ctx, conn, attr, buf, len, offset);
}

static ssize_t write_test_authorized_chrc(struct bt_conn *conn,
					  const struct bt_gatt_attr *attr,
					  const void *buf, uint16_t len,
					  uint16_t offset, uint8_t flags)
{
	printk("authorized chrc len %u offset %u\n", len, offset);

	return write_test_chrc(&authorized_chrc_ctx, buf, len, offset, flags);
}

static const struct test_chrc_ctx zeroed_chrc_ctx;

static bool unhandled_chrc_operation_validate(void)
{
	if (memcmp(&unauthorized_chrc_ctx, &zeroed_chrc_ctx, sizeof(zeroed_chrc_ctx)) != 0) {
		return false;
	}

	if (memcmp(&authorized_chrc_ctx, &zeroed_chrc_ctx, sizeof(zeroed_chrc_ctx)) != 0) {
		return false;
	}

	if ((unhandled_chrc_ctx.read_cnt != 1) && (unhandled_chrc_ctx.write_cnt != 1)) {
		return false;
	}

	if ((unhandled_chrc_ctx.auth_read_cnt != 0) &&
	    (unhandled_chrc_ctx.auth_write_cnt != 0)) {
		return false;
	}

	return true;
}

static bool unauthorized_chrc_operation_validate(void)
{
	if (memcmp(&unhandled_chrc_ctx, &zeroed_chrc_ctx, sizeof(zeroed_chrc_ctx)) != 0) {
		return false;
	}

	if (memcmp(&authorized_chrc_ctx, &zeroed_chrc_ctx, sizeof(zeroed_chrc_ctx)) != 0) {
		return false;
	}

	if ((unauthorized_chrc_ctx.read_cnt != 0) && (unauthorized_chrc_ctx.write_cnt != 0)) {
		return false;
	}

	if ((unauthorized_chrc_ctx.auth_read_cnt != 1) &&
	    (unauthorized_chrc_ctx.auth_write_cnt != 1)) {
		return false;
	}

	return true;
}

static bool authorized_chrc_operation_validate(void)
{
	if (memcmp(&unhandled_chrc_ctx, &zeroed_chrc_ctx, sizeof(zeroed_chrc_ctx)) != 0) {
		return false;
	}

	if (memcmp(&unauthorized_chrc_ctx, &zeroed_chrc_ctx, sizeof(zeroed_chrc_ctx)) != 0) {
		return false;
	}

	if ((authorized_chrc_ctx.read_cnt != 1) && (authorized_chrc_ctx.write_cnt != 1)) {
		return false;
	}

	if ((authorized_chrc_ctx.auth_read_cnt != 1) &&
	    (authorized_chrc_ctx.auth_write_cnt != 1)) {
		return false;
	}

	return true;
}

static ssize_t write_cp_chrc(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     const void *buf, uint16_t len,
			     uint16_t offset, uint8_t flags)
{
	static uint16_t cp_write_cnt;
	bool pass;
	char *log_str;

	if (cp_write_cnt == 0) {
		pass = unhandled_chrc_operation_validate();
		log_str = "unhandled";
	} else if (cp_write_cnt == 1) {
		pass = unauthorized_chrc_operation_validate();
		log_str = "unauthorized";
	} else if (cp_write_cnt == 2) {
		pass = authorized_chrc_operation_validate();
		log_str = "authorized";
	} else {
		FAIL("Invalid value of CP write counter %u\n", cp_write_cnt);
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	if (pass) {
		printk("Correct context for %s chrc\n", log_str);
	} else {
		FAIL("Invalid context for %s chrc\n", log_str);
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	memset(&unhandled_chrc_ctx, 0, sizeof(unhandled_chrc_ctx));
	memset(&unauthorized_chrc_ctx, 0, sizeof(unauthorized_chrc_ctx));
	memset(&authorized_chrc_ctx, 0, sizeof(authorized_chrc_ctx));

	cp_write_cnt++;

	if (cp_write_cnt == 3) {
		SET_FLAG(flag_is_chrc_ctx_validated);
	}

	return len;
}

BT_GATT_SERVICE_DEFINE(test_svc,
	BT_GATT_PRIMARY_SERVICE(TEST_SERVICE_UUID),
	BT_GATT_CHARACTERISTIC(TEST_UNHANDLED_CHRC_UUID,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_READ,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_READ,
			       read_test_unhandled_chrc,
			       write_test_unhandled_chrc, NULL),
	BT_GATT_CHARACTERISTIC(TEST_UNAUTHORIZED_CHRC_UUID,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_READ,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_READ,
			       read_test_unauthorized_chrc,
			       write_test_unauthorized_chrc, NULL),
	BT_GATT_CHARACTERISTIC(TEST_AUTHORIZED_CHRC_UUID,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_READ,
			       BT_GATT_PERM_WRITE | BT_GATT_PERM_READ,
			       read_test_authorized_chrc,
			       write_test_authorized_chrc, NULL),
	BT_GATT_CHARACTERISTIC(TEST_CP_CHRC_UUID,
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE,
			       NULL, write_cp_chrc, NULL),
);

static bool gatt_read_authorize(struct bt_conn *conn, const struct bt_gatt_attr *attr)
{
	if (bt_uuid_cmp(attr->uuid, TEST_UNAUTHORIZED_CHRC_UUID) == 0) {
		unauthorized_chrc_ctx.auth_read_cnt++;
		return false;
	} else if (bt_uuid_cmp(attr->uuid, TEST_AUTHORIZED_CHRC_UUID) == 0) {
		authorized_chrc_ctx.auth_read_cnt++;
		return true;
	} else {
		return true;
	}
}

static bool gatt_write_authorize(struct bt_conn *conn, const struct bt_gatt_attr *attr)
{
	if (bt_uuid_cmp(attr->uuid, TEST_UNAUTHORIZED_CHRC_UUID) == 0) {
		unauthorized_chrc_ctx.auth_write_cnt++;
		return false;
	} else if (bt_uuid_cmp(attr->uuid, TEST_AUTHORIZED_CHRC_UUID) == 0) {
		authorized_chrc_ctx.auth_write_cnt++;
		return true;
	} else {
		return true;
	}
}

static const struct bt_gatt_authorization_cb gatt_authorization_callbacks = {
	.read_authorize = gatt_read_authorize,
	.write_authorize = gatt_write_authorize,
};

static void test_main(void)
{
	int err;
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR))
	};

	err = bt_gatt_authorization_cb_register(&gatt_authorization_callbacks);
	if (err) {
		FAIL("Registering GATT authorization callbacks failed (err %d)\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err != 0) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	WAIT_FOR_FLAG(flag_is_chrc_ctx_validated);

	PASS("GATT server passed\n");
}

static const struct bst_test_instance test_gatt_server[] = {
	{
		.test_id = "gatt_server",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_gatt_server_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_gatt_server);
}
