/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stddef.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>

/* Custom Service Variables */
static const struct bt_uuid_128 test_uuid = BT_UUID_INIT_128(
	0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);
static const struct bt_uuid_128 test_chrc_uuid = BT_UUID_INIT_128(
	0xf2, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static uint8_t test_value[] = { 'T', 'e', 's', 't', '\0' };

static const struct bt_uuid_128 test1_uuid = BT_UUID_INIT_128(
	0xf4, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static const struct bt_uuid_128 test1_nfy_uuid = BT_UUID_INIT_128(
	0xf5, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static uint8_t nfy_enabled;

static void test1_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	nfy_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t test1_ccc_cfg_write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr, uint16_t value)
{
	return sizeof(value);
}

static ssize_t read_test(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

static ssize_t write_test(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags)
{
	uint8_t *value = attr->user_data;

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
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ_AUTHEN |
			       BT_GATT_PERM_WRITE_AUTHEN,
			       read_test, write_test, test_value),
};

static struct bt_gatt_service test_svc = BT_GATT_SERVICE(test_attrs);

static struct bt_gatt_attr test1_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&test1_uuid),

	BT_GATT_CHARACTERISTIC(&test1_nfy_uuid.uuid,
			       BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE,
			       NULL, NULL, &nfy_enabled),
	BT_GATT_CCC(test1_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};

static struct bt_gatt_service test1_svc = BT_GATT_SERVICE(test1_attrs);

ZTEST_SUITE(test_gatt, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_gatt, test_gatt_register)
{
	/* Ensure our test services are not already registered */
	bt_gatt_service_unregister(&test_svc);
	bt_gatt_service_unregister(&test1_svc);

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

ZTEST(test_gatt, test_gatt_unregister)
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

/* Test that a service A can be re-registered after registering it once, unregistering it, and then
 * registering another service B.
 * No pre-allocated handles. Repeat the process multiple times.
 */
ZTEST(test_gatt, test_gatt_reregister)
{
	struct bt_gatt_attr local_test_attrs[] = {
		/* Vendor Primary Service Declaration */
		BT_GATT_PRIMARY_SERVICE(&test_uuid),

		BT_GATT_CHARACTERISTIC(&test_chrc_uuid.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
				       BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN,
				       read_test, write_test, test_value),
	};

	struct bt_gatt_attr local_test1_attrs[] = {
		/* Vendor Primary Service Declaration */
		BT_GATT_PRIMARY_SERVICE(&test1_uuid),

		BT_GATT_CHARACTERISTIC(&test1_nfy_uuid.uuid, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE,
				       NULL, NULL, &nfy_enabled),
		BT_GATT_CCC(test1_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	};
	struct bt_gatt_service local_test_svc = BT_GATT_SERVICE(local_test_attrs);
	struct bt_gatt_service local_test1_svc = BT_GATT_SERVICE(local_test1_attrs);

	/* Check that the procedure is successful for a few iterations to verify stability and
	 * detect residual state or memory issues.
	 */
	for (int i = 0; i < 10; i++) {

		/* Check that the handles are initially 0x0000 before registering the service */
		for (int j = 0; j < local_test_svc.attr_count; j++) {
			zassert_equal(local_test_svc.attrs[j].handle, 0x0000,
				      "Test service A handle not initially reset");
		}

		zassert_false(bt_gatt_service_register(&local_test_svc),
			      "Test service A registration failed");

		zassert_false(bt_gatt_service_unregister(&local_test_svc),
			      "Test service A unregister failed");

		/* Check that the handles are the same as before registering the service */
		for (int j = 0; j < local_test_svc.attr_count; j++) {
			zassert_equal(local_test_svc.attrs[j].handle, 0x0000,
				      "Test service A handle not reset");
		}

		zassert_false(bt_gatt_service_register(&local_test1_svc),
			      "Test service B registration failed");

		zassert_false(bt_gatt_service_register(&local_test_svc),
			      "Test service A re-registering failed...");

		/* Clean up */
		zassert_false(bt_gatt_service_unregister(&local_test_svc),
			      "Test service A unregister failed");
		zassert_false(bt_gatt_service_unregister(&local_test1_svc),
			      "Test service B unregister failed");
	}
}

/* Test that a service A can be re-registered after registering it once, unregistering it, and then
 * registering another service B.
 * Service A and B both have pre-allocated handles for their attributes.
 * Check that pre-allocated handles are the same after unregistering as they were before
 * registering the service.
 */
ZTEST(test_gatt, test_gatt_reregister_pre_allocated_handles)
{
	struct bt_gatt_attr local_test_attrs[] = {
		/* Vendor Primary Service Declaration */
		BT_GATT_PRIMARY_SERVICE(&test_uuid),

		BT_GATT_CHARACTERISTIC(&test_chrc_uuid.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
				       BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN,
				       read_test, write_test, test_value),
	};

	struct bt_gatt_attr local_test1_attrs[] = {
		/* Vendor Primary Service Declaration */
		BT_GATT_PRIMARY_SERVICE(&test1_uuid),

		BT_GATT_CHARACTERISTIC(&test1_nfy_uuid.uuid, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE,
				       NULL, NULL, &nfy_enabled),
		BT_GATT_CCC(test1_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	};

	struct bt_gatt_service prealloc_test_svc = BT_GATT_SERVICE(local_test_attrs);
	struct bt_gatt_service prealloc_test1_svc = BT_GATT_SERVICE(local_test1_attrs);

	/* Pre-allocate handles for both services */
	for (int i = 0; i < prealloc_test_svc.attr_count; i++) {
		prealloc_test_svc.attrs[i].handle = 0x0100 + i;
	}
	for (int i = 0; i < prealloc_test1_svc.attr_count; i++) {
		prealloc_test1_svc.attrs[i].handle = 0x0200 + i;
	}

	zassert_false(bt_gatt_service_register(&prealloc_test_svc),
		      "Test service A registration failed");

	zassert_false(bt_gatt_service_unregister(&prealloc_test_svc),
		      "Test service A unregister failed");

	/* Check that the handles are the same as before registering the service */
	for (int i = 0; i < prealloc_test_svc.attr_count; i++) {
		zassert_equal(prealloc_test_svc.attrs[i].handle, 0x0100 + i,
			      "Test service A handle not reset");
	}

	zassert_false(bt_gatt_service_register(&prealloc_test1_svc),
		      "Test service B registration failed");

	zassert_false(bt_gatt_service_register(&prealloc_test_svc),
		      "Test service A re-registering failed...");

	/* Clean up */
	zassert_false(bt_gatt_service_unregister(&prealloc_test_svc),
		      "Test service A unregister failed");
	zassert_false(bt_gatt_service_unregister(&prealloc_test1_svc),
		      "Test service B unregister failed");
}

/* Test that a service A can be re-registered after registering it once, unregistering it, and then
 * registering another service B.
 * Service A has pre-allocated handles for its attributes, while Service B has handles assigned by
 * the stack when registered.
 * Check that pre-allocated handles are the same after unregistering as they were before
 * registering the service.
 */
ZTEST(test_gatt, test_gatt_reregister_pre_allocated_handle_single)
{
	struct bt_gatt_attr local_test_attrs[] = {
		/* Vendor Primary Service Declaration */
		BT_GATT_PRIMARY_SERVICE(&test_uuid),

		BT_GATT_CHARACTERISTIC(&test_chrc_uuid.uuid, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
				       BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN,
				       read_test, write_test, test_value),
	};

	struct bt_gatt_attr local_test1_attrs[] = {
		/* Vendor Primary Service Declaration */
		BT_GATT_PRIMARY_SERVICE(&test1_uuid),

		BT_GATT_CHARACTERISTIC(&test1_nfy_uuid.uuid, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE,
				       NULL, NULL, &nfy_enabled),
		BT_GATT_CCC(test1_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	};

	struct bt_gatt_service prealloc_test_svc = BT_GATT_SERVICE(local_test_attrs);
	struct bt_gatt_service auto_test_svc = BT_GATT_SERVICE(local_test1_attrs);

	/* Pre-allocate handles for one service only */
	for (int j = 0; j < prealloc_test_svc.attr_count; j++) {
		prealloc_test_svc.attrs[j].handle = 0x0100 + j;
	}

	zassert_false(bt_gatt_service_register(&prealloc_test_svc),
		      "Test service A registration failed");

	zassert_false(bt_gatt_service_unregister(&prealloc_test_svc),
		      "Test service A unregister failed");

	/* Check that the handles are the same as before registering the service */
	for (int i = 0; i < prealloc_test_svc.attr_count; i++) {
		zassert_equal(prealloc_test_svc.attrs[i].handle, 0x0100 + i,
			      "Test service A handle not reset");
	}

	zassert_false(bt_gatt_service_register(&auto_test_svc),
		      "Test service B registration failed");

	zassert_false(bt_gatt_service_register(&prealloc_test_svc),
		      "Test service A re-registering failed...");

	/* Clean up */
	zassert_false(bt_gatt_service_unregister(&prealloc_test_svc),
		      "Test service A unregister failed");
	zassert_false(bt_gatt_service_unregister(&auto_test_svc),
		      "Test service B unregister failed");
}

static uint8_t count_attr(const struct bt_gatt_attr *attr, uint16_t handle,
			  void *user_data)
{
	uint16_t *count = user_data;

	(*count)++;

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t find_attr(const struct bt_gatt_attr *attr, uint16_t handle,
			 void *user_data)
{
	const struct bt_gatt_attr **tmp = user_data;

	*tmp = attr;

	return BT_GATT_ITER_CONTINUE;
}

ZTEST(test_gatt, test_gatt_foreach)
{
	const struct bt_gatt_attr *attr;
	uint16_t num = 0;

	/* Attempt to register services */
	zassert_false(bt_gatt_service_register(&test_svc),
		     "Test service registration failed");
	zassert_false(bt_gatt_service_register(&test1_svc),
		     "Test service1 registration failed");

	/* Iterate attributes */
	bt_gatt_foreach_attr(test_attrs[0].handle, 0xffff, count_attr, &num);
	zassert_equal(num, 7, "Number of attributes don't match");

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
	zassert_equal(num, 2, "Number of attributes don't match");

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

ZTEST(test_gatt, test_gatt_read)
{
	const struct bt_gatt_attr *attr;
	uint8_t buf[256];
	ssize_t ret;

	/* Find attribute by UUID */
	attr = NULL;
	bt_gatt_foreach_attr_type(test_attrs[0].handle, 0xffff,
				  &test_chrc_uuid.uuid, NULL, 0, find_attr,
				  &attr);
	zassert_not_null(attr, "Attribute don't match");
	zassert_equal(attr->uuid, &test_chrc_uuid.uuid,
			      "Attribute UUID don't match");

	ret = attr->read(NULL, attr, (void *)buf, sizeof(buf), 0);
	zassert_equal(ret, strlen(test_value),
		      "Attribute read unexpected return");
	zassert_mem_equal(buf, test_value, ret,
			  "Attribute read value don't match");
}

ZTEST(test_gatt, test_gatt_write)
{
	const struct bt_gatt_attr *attr;
	char *value = "    ";
	ssize_t ret;

	/* Need our service to be registered */
	zassert_false(bt_gatt_service_register(&test_svc),
		     "Test service registration failed");

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

ZTEST(test_gatt, test_bt_att_err_to_str)
{
	/* Test a couple of entries */
	zassert_str_equal(bt_att_err_to_str(BT_ATT_ERR_SUCCESS),
			  "BT_ATT_ERR_SUCCESS");
	zassert_str_equal(bt_att_err_to_str(BT_ATT_ERR_INSUFFICIENT_ENCRYPTION),
			  "BT_ATT_ERR_INSUFFICIENT_ENCRYPTION");
	zassert_str_equal(bt_att_err_to_str(BT_ATT_ERR_OUT_OF_RANGE),
			  "BT_ATT_ERR_OUT_OF_RANGE");

	/* Test a entries that is not used */
	zassert_mem_equal(bt_att_err_to_str(0x14),
			  "(unknown)", strlen("(unknown)"));
	zassert_mem_equal(bt_att_err_to_str(0xFB),
			  "(unknown)", strlen("(unknown)"));

	for (uint16_t i = 0; i <= UINT8_MAX; i++) {
		zassert_not_null(bt_att_err_to_str(i), ": %d", i);
	}
}

ZTEST(test_gatt, test_bt_gatt_err_to_str)
{
	/* Test a couple of entries */
	zassert_str_equal(bt_gatt_err_to_str(BT_GATT_ERR(BT_ATT_ERR_SUCCESS)),
			  "BT_ATT_ERR_SUCCESS");
	zassert_str_equal(bt_gatt_err_to_str(BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_ENCRYPTION)),
			  "BT_ATT_ERR_INSUFFICIENT_ENCRYPTION");
	zassert_str_equal(bt_gatt_err_to_str(BT_GATT_ERR(BT_ATT_ERR_OUT_OF_RANGE)),
			  "BT_ATT_ERR_OUT_OF_RANGE");

	/* Test entries that are not used */
	zassert_mem_equal(bt_gatt_err_to_str(BT_GATT_ERR(0x14)),
			  "(unknown)", strlen("(unknown)"));
	zassert_mem_equal(bt_gatt_err_to_str(BT_GATT_ERR(0xFB)),
			  "(unknown)", strlen("(unknown)"));

	/* Test positive values */
	for (uint16_t i = 0; i <= UINT8_MAX; i++) {
		zassert_not_null(bt_gatt_err_to_str(i), ": %d", i);
	}

	/* Test negative values */
	for (uint16_t i = 0; i <= UINT8_MAX; i++) {
		zassert_not_null(bt_gatt_err_to_str(-i), ": %d", i);
	}
}

ZTEST(test_gatt, test_gatt_ccc_write_cb)
{
	struct bt_gatt_attr test_write_cb_attrs[] = {
		/* Vendor Primary Service Declaration */
		BT_GATT_PRIMARY_SERVICE(&test1_uuid),

		BT_GATT_CHARACTERISTIC(&test1_nfy_uuid.uuid,
				       BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE,
				       NULL, NULL, &nfy_enabled),
		BT_GATT_CCC_WITH_WRITE_CB(test1_ccc_cfg_changed,
			test1_ccc_cfg_write_cb,
			BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	};

	struct bt_gatt_service test_write_cb_svc = BT_GATT_SERVICE(test_write_cb_attrs);

	zassert_false(bt_gatt_service_register(&test_write_cb_svc),
		     "Test service registration failed");
	zassert_false(bt_gatt_service_unregister(&test_write_cb_svc),
		     "Test service1 unregister failed");
}
