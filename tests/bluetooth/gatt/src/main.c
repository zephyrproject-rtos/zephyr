/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stddef.h>
#include <ztest.h>

#include <bluetooth/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>

/* Semaphore to test when exchange MTU completes  */
static K_SEM_DEFINE(exch_sem, 0, 1);

/* Semaphore to test when discover completes  */
static K_SEM_DEFINE(disc_sem, 0, 1);

/* Semaphore to test when read completes  */
static K_SEM_DEFINE(read_sem, 0, 1);

/* Semaphore to test when write completes  */
static K_SEM_DEFINE(write_sem, 0, 1);

/* Semaphore to test when a notification is received */
static K_SEM_DEFINE(nfy_sem, 0, 1);

/* Semaphore to test when a confirmation is received */
static K_SEM_DEFINE(ind_sem, 0, 1);

static struct bt_conn *conn;

/* Custom Service Variables */
static struct bt_uuid_128 test_uuid = BT_UUID_INIT_128(
	0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);
static struct bt_uuid_128 test_chrc_uuid = BT_UUID_INIT_128(
	0xf2, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static u8_t test_value[] = { 'T', 'e', 's', 't', '\0' };

static struct bt_uuid_128 test1_uuid = BT_UUID_INIT_128(
	0xf4, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static const struct bt_uuid_128 test1_nfy_uuid = BT_UUID_INIT_128(
	0xf5, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static const struct bt_uuid_128 test1_ind_uuid = BT_UUID_INIT_128(
	0xf6, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static struct bt_gatt_ccc_cfg test1_nfy_cfg[BT_GATT_CCC_MAX] = {};
static u8_t nfy_enabled;

static void test1_nfy_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	nfy_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static struct bt_gatt_ccc_cfg test1_ind_cfg[BT_GATT_CCC_MAX] = {};
static u8_t ind_enabled;

static void test1_ind_cfg_changed(const struct bt_gatt_attr *attr, u16_t value)
{
	ind_enabled = (value == BT_GATT_CCC_INDICATE) ? 1 : 0;
}

static ssize_t read_test(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, u16_t len, u16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

static ssize_t write_test(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, u16_t len, u16_t offset,
			 u8_t flags)
{
	u8_t *value = attr->user_data;

	if (offset + len > sizeof(test_value)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

static struct bt_gatt_attr test_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&test_uuid),

	BT_GATT_CHARACTERISTIC(&test_chrc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_test, write_test, test_value),
};

static struct bt_gatt_service test_svc = BT_GATT_SERVICE(test_attrs);

static struct bt_gatt_attr test1_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&test1_uuid),

	BT_GATT_CHARACTERISTIC(&test1_nfy_uuid.uuid,
			       BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE,
			       NULL, NULL, &nfy_enabled),
	BT_GATT_CCC(test1_nfy_cfg, test1_nfy_cfg_changed),

	BT_GATT_CHARACTERISTIC(&test1_ind_uuid.uuid,
			       BT_GATT_CHRC_INDICATE, BT_GATT_PERM_NONE,
			       NULL, NULL, &ind_enabled),
	BT_GATT_CCC(test1_ind_cfg, test1_ind_cfg_changed),
};

static struct bt_gatt_service test1_svc = BT_GATT_SERVICE(test1_attrs);

void test_gatt_register(void)
{
	/* Attempt to register services */
	zassert_false(bt_gatt_service_register(&test_svc),
		     "Test service registration failed");
	zassert_false(bt_gatt_service_register(&test1_svc),
		     "Test service1 registration failed");

	/* Attempt to register already registered services */
	zassert_true(bt_gatt_service_register(&test_svc),
		     "Test service duplicate succeeded");
	zassert_true(bt_gatt_service_register(&test1_svc),
		     "Test service1 duplicate succeeded");
}

void test_gatt_unregister(void)
{
	/* Attempt to unregister last */
	zassert_false(bt_gatt_service_unregister(&test1_svc),
		     "Test service1 unregister failed");
	zassert_false(bt_gatt_service_register(&test1_svc),
		     "Test service1 re-registration failed");

	/* Attempt to unregister first/middle */
	zassert_false(bt_gatt_service_unregister(&test_svc),
		     "Test service unregister failed");
	zassert_false(bt_gatt_service_register(&test_svc),
		     "Test service re-registration failed");

	/* Attempt to unregister all reverse order */
	zassert_false(bt_gatt_service_unregister(&test1_svc),
		     "Test service1 unregister failed");
	zassert_false(bt_gatt_service_unregister(&test_svc),
		     "Test service unregister failed");

	zassert_false(bt_gatt_service_register(&test_svc),
		     "Test service registration failed");
	zassert_false(bt_gatt_service_register(&test1_svc),
		     "Test service1 registration failed");

	/* Attempt to unregister all same order */
	zassert_false(bt_gatt_service_unregister(&test_svc),
		     "Test service1 unregister failed");
	zassert_false(bt_gatt_service_unregister(&test1_svc),
		     "Test service unregister failed");
}

static u8_t count_attr(const struct bt_gatt_attr *attr, void *user_data)
{
	u16_t *count = user_data;

	(*count)++;

	return BT_GATT_ITER_CONTINUE;
}

static u8_t find_attr(const struct bt_gatt_attr *attr, void *user_data)
{
	const struct bt_gatt_attr **tmp = user_data;

	*tmp = attr;

	return BT_GATT_ITER_CONTINUE;
}

void test_gatt_foreach(void)
{
	const struct bt_gatt_attr *attr;
	u16_t num = 0;

	/* Attempt to register services */
	zassert_false(bt_gatt_service_register(&test_svc),
		     "Test service registration failed");
	zassert_false(bt_gatt_service_register(&test1_svc),
		     "Test service1 registration failed");

	/* Iterate attributes */
	bt_gatt_foreach_attr(test_attrs[0].handle, 0xffff, count_attr, &num);
	zassert_equal(num, 10, "Number of attributes don't match");

	/* Iterate 1 attribute */
	num = 0;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff, NULL, NULL, 1,
				  count_attr, &num);
	zassert_equal(num, 1, "Number of attributes don't match");

	/* Find attribute by UUID */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  &test_chrc_uuid.uuid, NULL, 0, find_attr,
				  &attr);
	zassert_not_null(attr, "Attribute don't match");
	if (attr) {
		zassert_equal(attr->uuid, &test_chrc_uuid.uuid,
			      "Attribute UUID don't match");
	}

	/* Find attribute by DATA */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff, NULL,
				  test_value, 0, find_attr, &attr);
	zassert_not_null(attr, "Attribute don't match");
	if (attr) {
		zassert_equal(attr->user_data, test_value,
			      "Attribute value don't match");
	}

	/* Find all characteristics */
	num = 0;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  BT_UUID_GATT_CHRC, NULL, 0, count_attr, &num);
	zassert_equal(num, 3, "Number of attributes don't match");

	/* Find 1 characteristic */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  BT_UUID_GATT_CHRC, NULL, 1, find_attr, &attr);
	zassert_not_null(attr, "Attribute don't match");

	/* Find attribute by UUID and DATA */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  &test1_nfy_uuid.uuid, &nfy_enabled, 1,
				  find_attr, &attr);
	zassert_not_null(attr, "Attribute don't match");
	if (attr) {
		zassert_equal(attr->uuid, &test1_nfy_uuid.uuid,
			      "Attribute UUID don't match");
		zassert_equal(attr->user_data, &nfy_enabled,
			      "Attribute value don't match");
	}
}

void test_gatt_read(void)
{
	const struct bt_gatt_attr *attr;
	u8_t buf[256];
	ssize_t ret;

	/* Find attribute by UUID */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  &test_chrc_uuid.uuid, NULL, 0, find_attr,
				  &attr);
	zassert_not_null(attr, "Attribute don't match");
	if (attr) {
		zassert_equal(attr->uuid, &test_chrc_uuid.uuid,
			      "Attribute UUID don't match");
	}

	ret = attr->read(NULL, attr, (void *)buf, sizeof(buf), 0);
	zassert_equal(ret, strlen(test_value),
		      "Attribute read unexpected return");
	zassert_mem_equal(buf, test_value, ret,
			  "Attribute read value don't match");
}

void test_gatt_write(void)
{
	const struct bt_gatt_attr *attr;
	char *value = "    ";
	ssize_t ret;

	/* Find attribute by UUID */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  &test_chrc_uuid.uuid, NULL, 0, find_attr,
				  &attr);
	zassert_not_null(attr, "Attribute don't match");

	ret = attr->write(NULL, attr, (void *)value, strlen(value), 0, 0);
	zassert_equal(ret, strlen(value), "Attribute write unexpected return");
	zassert_mem_equal(value, test_value, ret,
			  "Attribute write value don't match");
}

static void gatt_exchange(struct bt_conn *conn, u8_t err,
			  struct bt_gatt_exchange_params *parms)
{
	zassert_equal(err, 0, "Failed to exchange MTU");

	k_sem_give(&exch_sem);
}

void test_gatt_exchange_mtu(void)
{
	struct bt_gatt_exchange_params params = {
		.func = gatt_exchange,
	};

	conn = bt_conn_create_le(BT_ADDR_LE_NONE, BT_LE_CONN_PARAM_DEFAULT);

	zassert_not_null(conn, "Test loopback connection failed");

	/* Attempt to exchange MTU */
	zassert_false(bt_gatt_exchange_mtu(conn, &params) < 0,
		     "Failed to exchange MTU");

	zassert_false(k_sem_take(&exch_sem, K_MSEC(100)),
		      "Exchange func not called within timeout");
}

static u16_t attr_count;

static u8_t gatt_discover(struct bt_conn *conn,	const struct bt_gatt_attr *attr,
			  struct bt_gatt_discover_params *params)
{
	if (attr) {
		if (params->type == BT_GATT_DISCOVER_DESCRIPTOR) {
			zassert_true(bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CHRC),
				     "Discovered a characteristic with "
				     "descriptor discovery");
		}
		attr_count++;
		return BT_GATT_ITER_CONTINUE;
	}

	k_sem_give(&disc_sem);

	return BT_GATT_ITER_STOP;
}

void test_gatt_discover(void)
{
	struct bt_gatt_discover_params params = {
		.func = gatt_discover,
	};

	/* Start with test_attr handle */
	params.start_handle = test_attrs[0].handle;
	params.end_handle = 0xffff;
	params.type = BT_GATT_DISCOVER_PRIMARY;

	/* Attempt to discover primary services */
	zassert_false(bt_gatt_discover(conn, &params) < 0,
		     "Failed to discover primary services");

	zassert_false(k_sem_take(&disc_sem, K_MSEC(100)),
		      "Discover func not called within timeout");

	zassert_equal(attr_count, 2,
		      "Unexpected number of primary services found");

	params.type = BT_GATT_DISCOVER_SECONDARY;

	/* Attempt to discover secondary services */
	zassert_false(bt_gatt_discover(conn, &params) < 0,
		     "Failed to discover secondary services");

	zassert_false(k_sem_take(&disc_sem, K_MSEC(100)),
		      "Discover func not called within timeout");

	zassert_equal(attr_count, 2,
		      "Unexpected number of secondary services found");

	params.type = BT_GATT_DISCOVER_INCLUDE;

	/* Attempt to discover secondary services */
	zassert_false(bt_gatt_discover(conn, &params) < 0,
		     "Failed to discover included services");

	zassert_false(k_sem_take(&disc_sem, K_MSEC(100)),
		      "Discover func not called within timeout");

	zassert_equal(attr_count, 2,
		      "Unexpected number of included services found");

	params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	/* Attempt to discover characteristics */
	zassert_false(bt_gatt_discover(conn, &params) < 0,
		     "Failed to discover characteristics");

	zassert_false(k_sem_take(&disc_sem, K_MSEC(100)),
		      "Discover func not called within timeout");

	zassert_equal(attr_count, 5,
		      "Unexpected number of characteristics found");

	params.type = BT_GATT_DISCOVER_DESCRIPTOR;

	/* Attempt to discover descriptors */
	zassert_false(bt_gatt_discover(conn, &params) < 0,
		     "Failed to discover descriptor");

	zassert_false(k_sem_take(&disc_sem, K_MSEC(100)),
		      "Discover func not called within timeout");

	zassert_equal(attr_count, 10,
		      "Unexpected number of descriptors found");

	params.type = BT_GATT_DISCOVER_ATTRIBUTE;

	/* Attempt to discover attributes */
	zassert_false(bt_gatt_discover(conn, &params) < 0,
		     "Failed to discover attributes");

	zassert_false(k_sem_take(&disc_sem, K_MSEC(100)),
		      "Discover func not called within timeout");

	zassert_equal(attr_count, 20,
		      "Unexpected number of attributes found");

	params.uuid = &test_chrc_uuid.uuid;

	/* Attempt to discover attributes by UUID */
	zassert_false(bt_gatt_discover(conn, &params) < 0,
		     "Failed to discover attributes");

	zassert_false(k_sem_take(&disc_sem, K_MSEC(100)),
		      "Discover func not called within timeout");

	zassert_equal(attr_count, 21,
		      "Unexpected number of attributes found");
}

static u8_t gatt_read(struct bt_conn *conn, u8_t err,
		      struct bt_gatt_read_params *params,
		      const void *data, u16_t length)
{
	if (!data) {
		k_sem_give(&read_sem);
		return BT_GATT_ITER_STOP;
	}

	switch (params->handle_count) {
	case 0:
	case 1:
		zassert_equal(err, 0, "Attribute read return an error");
		zassert_equal(length, strlen(test_value),
			      "Attribute read length don't match");
		zassert_mem_equal(data, test_value, length,
				  "Attribute value don't match");
		break;
	}

	return BT_GATT_ITER_CONTINUE;
}

void test_gatt_client_read(void)
{
	struct bt_gatt_read_params params = {
		.func = gatt_read,
	};

	params.handle_count = 1;
	params.single.handle = test_attrs[2].handle;

	/* Attempt to read attribute */
	zassert_false(bt_gatt_read(conn, &params) < 0,
		      "Failed to read attribute");

	zassert_false(k_sem_take(&read_sem, K_MSEC(100)),
		      "Read func not called within timeout");

	params.handle_count = 0;
	params.by_uuid.start_handle = test_attrs[2].handle;
	params.by_uuid.end_handle = 0xffff;
	params.by_uuid.uuid = &test_chrc_uuid.uuid;

	/* Attempt to read attribute by UUID */
	zassert_false(bt_gatt_read(conn, &params) < 0,
		      "Failed to read attribute");

	zassert_false(k_sem_take(&read_sem, K_MSEC(100)),
		      "Read func not called within timeout");
}

static void gatt_write(struct bt_conn *conn, u8_t err,
		       struct bt_gatt_write_params *params)
{
	zassert_equal(err, 0, "Attribute write return an error");
	k_sem_give(&write_sem);
}

void test_gatt_client_write(void)
{
	struct bt_gatt_write_params params = {
		.func = gatt_write,
	};
	char *value = "Test";

	params.handle = test_attrs[2].handle;
	params.data = value;
	params.length = strlen(value);

	/* Attempt to write attribute */
	zassert_false(bt_gatt_write(conn, &params) < 0,
		      "Failed to write attribute");

	zassert_false(k_sem_take(&write_sem, K_MSEC(100)),
		      "Write func not called within timeout");

	zassert_equal(strlen(value), strlen(test_value),
		      "Attribute value length don't match");
	zassert_mem_equal(value, test_value, strlen(value),
			  "Attribute value don't match");

	value = "    ";

	/* Attempt to write attribute without response */
	zassert_false(bt_gatt_write_without_response(conn,
						     test_attrs[2].handle,
						     value, strlen(value),
						     false) < 0,
		      "Failed to write attribute");

	k_sleep(K_MSEC(100));

	zassert_equal(strlen(value), strlen(test_value),
		      "Attribute value length don't match");
	zassert_mem_equal(value, test_value, strlen(value),
			  "Attribute value don't match");
}

static u8_t gatt_notify(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, u16_t length)
{
	zassert_not_null(data, "Attribute unsubscribed");
	zassert_mem_equal(data, test_value, strlen(test_value),
			  "Notification data don't match");
	k_sem_give(&nfy_sem);

	return BT_GATT_ITER_CONTINUE;
}

static struct bt_gatt_subscribe_params nfy_params = {
	.notify = gatt_notify,
};

static struct bt_gatt_subscribe_params ind_params = {
	.notify = gatt_notify,
};

void test_gatt_subscribe(void)
{
	nfy_params.ccc_handle = test1_attrs[3].handle,
	nfy_params.value_handle = test1_attrs[2].handle,
	nfy_params.value = BT_GATT_CCC_NOTIFY;

	/* Attempt to subscribe attribute for notifications */
	zassert_false(bt_gatt_subscribe(conn, &nfy_params) < 0,
		      "Failed to subscribe attribute");

	k_sleep(K_MSEC(100));

	zassert_true(nfy_enabled, "Notification not enabled");

	ind_params.ccc_handle = test1_attrs[6].handle,
	ind_params.value_handle = test1_attrs[5].handle,
	ind_params.value = BT_GATT_CCC_INDICATE;

	/* Attempt to subscribe attribute for indications */
	zassert_false(bt_gatt_subscribe(conn, &ind_params) < 0,
		      "Failed to subscribe attribute");

	k_sleep(K_MSEC(100));

	zassert_true(ind_enabled, "Indication not enabled");
}

void test_gatt_notify(void)
{
	/* Attempt to notify value */
	zassert_false(bt_gatt_notify(NULL, &test1_attrs[2], test_value,
				     strlen(test_value)) < 0,
		      "Failed to notify value");

	zassert_false(k_sem_take(&nfy_sem, K_MSEC(100)),
		      "Notify func not called within timeout");

	/* Attempt to notify value with connection */
	zassert_false(bt_gatt_notify(NULL, &test1_attrs[2], test_value,
				     strlen(test_value)) < 0,
		      "Failed to notify value");

	zassert_false(k_sem_take(&nfy_sem, K_MSEC(100)),
		      "Notify func not called within timeout");
}

static void gatt_indicate(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  u8_t err)
{
	zassert_equal(err, 0, "Indicate value return an error");
	k_sem_give(&ind_sem);
}

void test_gatt_indicate(void)
{
	struct bt_gatt_indicate_params params = {
		.attr = &test1_attrs[5],
		.func = gatt_indicate,
		.data = test_value,
		.len = strlen(test_value),
	};

	/* Attempt to indicate value */
	zassert_false(bt_gatt_indicate(NULL, &params) < 0,
		      "Failed to indicate value");

	zassert_false(k_sem_take(&nfy_sem, K_MSEC(100)),
		      "Notify func not called within timeout");

	zassert_false(k_sem_take(&ind_sem, K_MSEC(100)),
		      "Indicate func not called within timeout");

	/* Attempt to indicate value with connection */
	zassert_false(bt_gatt_indicate(NULL, &params) < 0,
		      "Failed to indicate value");

	zassert_false(k_sem_take(&nfy_sem, K_MSEC(100)),
		      "Notify func not called within timeout");

	zassert_false(k_sem_take(&ind_sem, K_MSEC(100)),
		      "Indicate func not called within timeout");
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_gatt,
			 ztest_unit_test(test_gatt_register),
			 ztest_unit_test(test_gatt_unregister),
			 ztest_unit_test(test_gatt_foreach),
			 ztest_unit_test(test_gatt_read),
			 ztest_unit_test(test_gatt_write),
			 ztest_unit_test(test_gatt_exchange_mtu),
			 ztest_unit_test(test_gatt_discover),
			 ztest_unit_test(test_gatt_client_read),
			 ztest_unit_test(test_gatt_client_write),
			 ztest_unit_test(test_gatt_subscribe),
			 ztest_unit_test(test_gatt_notify),
			 ztest_unit_test(test_gatt_indicate));
	ztest_run_test_suite(test_gatt);
}
