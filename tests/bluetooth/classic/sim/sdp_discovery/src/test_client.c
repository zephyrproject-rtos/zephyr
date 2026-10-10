/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

#include "test_common.h"

#define LOG_MODULE_NAME test_sdp_discovery_client
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_TEST_SDP_DISCOVERY_LOG_LEVEL);

bt_addr_t peer_addr;
static K_SEM_DEFINE(br_connect_changed_sem, 0, 1);

static struct bt_br_discovery_param br_discover_param;
#define BR_DISCOVER_RESULT_COUNT 10
static struct bt_br_discovery_result br_discover_result[BR_DISCOVER_RESULT_COUNT];
static K_SEM_DEFINE(br_discovery_found_sem, 0, 1);

#define TEST_FLAG_DEVICE_FOUND 0
#define TEST_FLAG_CASE_PASSED  1

static ATOMIC_DEFINE(test_flags, 32);

struct bt_conn *br_conn;

static void br_connected(struct bt_conn *conn, uint8_t conn_err)
{
	LOG_DBG("connected: conn %p err 0x%02x", (void *)conn, conn_err);
	if (conn_err == 0 && br_conn == NULL) {
		br_conn = bt_conn_ref(conn);
		k_sem_give(&br_connect_changed_sem);
	} else {
		LOG_ERR("Connection failed");
	}
}

static void br_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_DBG("disconnected: conn %p reason 0x%02x", (void *)conn, reason);
	if (br_conn != NULL) {
		bt_conn_unref(br_conn);
		br_conn = NULL;
	}
	k_sem_give(&br_connect_changed_sem);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = br_connected,
	.disconnected = br_disconnected,
};

static void br_connect_to_peer(void)
{
	struct bt_conn *conn;
	int err;

	if (br_conn != NULL) {
		return;
	}

	if (!atomic_test_bit(test_flags, TEST_FLAG_DEVICE_FOUND)) {
		err = k_sem_take(&br_discovery_found_sem,
				 K_MSEC(ARRAY_SIZE(br_discover_result) * 1280));
		zassert_equal(err, 0, "Failed to found peer device (err %d)", err);
		zassert_true(atomic_test_bit(test_flags, TEST_FLAG_DEVICE_FOUND),
			     "Peer device not found");

		err = bt_br_discovery_stop();
		if ((err != 0) && (err != -EALREADY)) {
			LOG_ERR("Failed to stop GAP discovery procedure (err %d)", err);
		}
	}

	LOG_DBG("Connecting to peer device");
	k_sem_reset(&br_connect_changed_sem);
	conn = bt_conn_create_br(&peer_addr, BT_BR_CONN_PARAM_DEFAULT);
	zassert_true(conn != NULL, "BR connection creating failed");

	err = k_sem_take(&br_connect_changed_sem, K_SECONDS(CONFIG_BT_CREATE_CONN_TIMEOUT));
	zassert_equal(err, 0, "Connection timeout (err %d)", err);
	zassert_true(br_conn != NULL, "Connect handle is NULL");
	zassert_equal(br_conn, conn, "Connect mismatched %p != %p", br_conn, conn);

	LOG_DBG("BR connection established");
	bt_conn_unref(conn);
}

static void br_disconnect_from_peer(void)
{
	int err;

	if (br_conn == NULL) {
		return;
	}

	LOG_DBG("Disconnecting from peer device");
	k_sem_reset(&br_connect_changed_sem);
	err = bt_conn_disconnect(br_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	zassert_equal(err, 0, "Failed to disconnect (err %d)", err);

	err = k_sem_take(&br_connect_changed_sem, K_SECONDS(30));
	zassert_equal(err, 0, "Disconnection timeout (err %d)", err);
	zassert_equal(br_conn, NULL, "br_conn is not NULL after disconnect");
}

struct test_bt_sdp_discover_params {
	struct bt_sdp_discover_params params;
	uint8_t found_count;
	uint8_t expected_count;
};

static struct test_bt_sdp_discover_params sdp_discover_params;

#define TEST_SDP_DISOVER_PARAMS(_params) \
	CONTAINER_OF(_params, struct test_bt_sdp_discover_params, params)

#define SDP_CLIENT_USER_BUF_LEN 4096
NET_BUF_POOL_FIXED_DEFINE(sdp_client_pool, 1, SDP_CLIENT_USER_BUF_LEN,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
static struct bt_uuid_any sdp_uuid;

static K_SEM_DEFINE(br_sdp_discover_sem, 0, 1);

static uint8_t test_01_sdp_discover_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
				       const struct bt_sdp_discover_params *params)
{
	LOG_DBG("SDP discovery callback");
	if (result == NULL || result->resp_buf == NULL || result->resp_buf->len == 0) {
		atomic_set_bit(test_flags, TEST_FLAG_CASE_PASSED);
	}
	k_sem_give(&br_sdp_discover_sem);
	return BT_SDP_DISCOVER_UUID_STOP;
}

static void test_sdp_discover_service_search(struct bt_conn *conn, struct bt_uuid *uuid,
					     bt_sdp_discover_func_t func)
{
	int err;

	atomic_clear_bit(test_flags, TEST_FLAG_CASE_PASSED);

	sdp_discover_params.found_count = 0;
	sdp_discover_params.params.uuid = uuid;
	sdp_discover_params.params.func = func;
	sdp_discover_params.params.pool = &sdp_client_pool;
	sdp_discover_params.params.type = BT_SDP_DISCOVER_SERVICE_SEARCH;
	sdp_discover_params.params.ids = NULL;

	k_sem_reset(&br_sdp_discover_sem);
	err = bt_sdp_discover(conn, &sdp_discover_params.params);
	zassert_equal(err, 0, "Failed to start sdp discovery (err %d)", err);

	err = k_sem_take(&br_sdp_discover_sem, K_SECONDS(10));
	zassert_equal(err, 0, "Wait for sdp discover done event timeout (err %d)", err);

	zassert_true(atomic_test_bit(test_flags, TEST_FLAG_CASE_PASSED),
				     "Test case passed bit not set");
}

static void test_sdp_discover_service_attr(struct bt_conn *conn, uint32_t handle,
					   bt_sdp_discover_func_t func)
{
	int err;

	atomic_clear_bit(test_flags, TEST_FLAG_CASE_PASSED);

	sdp_discover_params.found_count = 0;
	sdp_discover_params.params.handle = handle;
	sdp_discover_params.params.func = func;
	sdp_discover_params.params.pool = &sdp_client_pool;
	sdp_discover_params.params.type = BT_SDP_DISCOVER_SERVICE_ATTR;
	sdp_discover_params.params.ids = NULL;

	k_sem_reset(&br_sdp_discover_sem);
	err = bt_sdp_discover(conn, &sdp_discover_params.params);
	zassert_equal(err, 0, "Failed to start sdp discovery (err %d)", err);

	err = k_sem_take(&br_sdp_discover_sem, K_SECONDS(10));
	zassert_equal(err, 0, "Wait for sdp discover done event timeout (err %d)", err);

	zassert_true(atomic_test_bit(test_flags, TEST_FLAG_CASE_PASSED),
				     "Test case passed bit not set");
}

static void test_sdp_discover_service_search_attr(struct bt_conn *conn, struct bt_uuid *uuid,
						  bt_sdp_discover_func_t func,
						  struct bt_sdp_attribute_id_list *ids)
{
	int err;

	atomic_clear_bit(test_flags, TEST_FLAG_CASE_PASSED);

	sdp_discover_params.found_count = 0;
	sdp_discover_params.params.uuid = uuid;
	sdp_discover_params.params.func = func;
	sdp_discover_params.params.pool = &sdp_client_pool;
	sdp_discover_params.params.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;
	sdp_discover_params.params.ids = ids;

	k_sem_reset(&br_sdp_discover_sem);
	err = bt_sdp_discover(conn, &sdp_discover_params.params);
	zassert_equal(err, 0, "Failed to start sdp discovery (err %d)", err);

	err = k_sem_take(&br_sdp_discover_sem, K_SECONDS(10));
	zassert_equal(err, 0, "Wait for sdp discover done event timeout (err %d)", err);

	zassert_true(atomic_test_bit(test_flags, TEST_FLAG_CASE_PASSED),
				     "Test case passed bit not set");
}

ZTEST(sdp_client, test_01_no_sdp_records)
{
	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	sdp_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_uuid.u16.val = BT_SDP_PROTO_L2CAP;
	test_sdp_discover_service_search(br_conn, &sdp_uuid.uuid, test_01_sdp_discover_cb);

	br_disconnect_from_peer();
}

static int test_02_get_attr_value_u16(const struct net_buf *buf, uint16_t attr_id,
				      const struct bt_uuid *uuid, uint16_t *value)
{
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value val;
	int err;

	err = bt_sdp_get_attr(buf, attr_id, &attr);
	if (err != 0) {
		LOG_ERR("Record handle not found (err %d)", err);
		return err;
	}

	err = bt_sdp_attr_read(&attr, uuid, &val);
	if (err != 0) {
		LOG_ERR("Failed to read attribute (err %d)", err);
		return err;
	}

	if (val.type != BT_SDP_ATTR_VALUE_TYPE_UINT || val.uint.size != sizeof(*value)) {
		LOG_ERR("Invalid attribute value type or size");
		return -EINVAL;
	}

	*value = val.uint.u16;
	return 0;
}

static int test_02_get_attr_addl_value_u16(const struct net_buf *buf, uint16_t index,
					   const struct bt_uuid *uuid, uint16_t *value)
{
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value val;
	ssize_t count;
	int err;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_ADD_PROTO_DESC_LIST, &attr);
	if (err != 0) {
		LOG_ERR("Record handle not found (err %d)", err);
		return err;
	}

	count = bt_sdp_attr_addl_proto_count(&attr);
	if (count < 0 || index >= count) {
		LOG_ERR("Invalid additional protocol index (err %zd)", count);
		return -EINVAL;
	}

	err = bt_sdp_attr_addl_proto_read(&attr, index, uuid, &val);
	if (err != 0) {
		LOG_ERR("Failed to read attribute (err %d)", err);
		return err;
	}

	if (val.type != BT_SDP_ATTR_VALUE_TYPE_UINT || val.uint.size != sizeof(*value)) {
		LOG_ERR("Invalid attribute value type or size");
		return -EINVAL;
	}

	*value = val.uint.u16;
	return 0;
}

#define SDP_SERVICE_HANDLE_BASE 0x10000

static uint8_t test_02_sdp_discover_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
				       const struct bt_sdp_discover_params *params)
{
	int err;
	struct bt_uuid_16 uuid16;
	struct bt_uuid_128 uuid128;

	LOG_DBG("SDP discovery callback");
	if (result == NULL || result->resp_buf == NULL || result->resp_buf->len == 0) {
		LOG_ERR("N/A response received");
		goto failed;
	}

	LOG_HEXDUMP_DBG(result->resp_buf->data, result->resp_buf->len, "SDP Record:");

	if (params->type == BT_SDP_DISCOVER_SERVICE_SEARCH) {
		uint32_t service_handle;

		if (result->resp_buf->len != sizeof(service_handle)) {
			LOG_ERR("Invalid response buffer length");
			goto failed;
		}

		service_handle = net_buf_pull_be32(result->resp_buf);
		if (service_handle != SDP_SERVICE_HANDLE_BASE) {
			LOG_ERR("Invalid service handle value");
			goto failed;
		}
	}

	if (params->type == BT_SDP_DISCOVER_SERVICE_ATTR ||
	    params->type == BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR) {
		uint16_t value;

		uuid16.uuid.type = BT_UUID_TYPE_16;
		uuid16.val = BT_SDP_PROTO_L2CAP;
		err = test_02_get_attr_value_u16(result->resp_buf, BT_SDP_ATTR_PROTO_DESC_LIST,
						 &uuid16.uuid, &value);
		if (err != 0) {
			goto failed;
		}

		if (value != 0xfff0) {
			LOG_ERR("Invalid service attr value");
			goto failed;
		}

		uuid16.uuid.type = BT_UUID_TYPE_16;
		uuid16.val = 0xfff1;
		err = test_02_get_attr_value_u16(result->resp_buf, BT_SDP_ATTR_PROTO_DESC_LIST,
						 &uuid16.uuid, &value);
		if (err != 0) {
			goto failed;
		}

		if (value != 0x1001) {
			LOG_ERR("Invalid service attr value");
			goto failed;
		}

		uuid16.uuid.type = BT_UUID_TYPE_16;
		uuid16.val = BT_SDP_PROTO_L2CAP;
		err = test_02_get_attr_addl_value_u16(result->resp_buf, 0, &uuid16.uuid, &value);
		if (err != 0) {
			goto failed;
		}

		if (value != 0xffff) {
			LOG_ERR("Invalid service attr value");
			goto failed;
		}

		uuid16.uuid.type = BT_UUID_TYPE_16;
		uuid16.val = 0xfff2;
		err = test_02_get_attr_addl_value_u16(result->resp_buf, 0, &uuid16.uuid, &value);
		if (err != 0) {
			goto failed;
		}

		if (value != 0x1002) {
			LOG_ERR("Invalid service attr value");
			goto failed;
		}

		uuid16.uuid.type = BT_UUID_TYPE_16;
		uuid16.val = BT_SDP_PROTO_L2CAP;
		err = test_02_get_attr_addl_value_u16(result->resp_buf, 1, &uuid16.uuid, &value);
		if (err != 0) {
			goto failed;
		}

		if (value != 0xfffe) {
			LOG_ERR("Invalid service attr value");
			goto failed;
		}

		uuid128.uuid.type = BT_UUID_TYPE_128;
		memcpy(uuid128.val, TEST_PROFILE_UUID, sizeof(uuid128.val));
		err = test_02_get_attr_value_u16(result->resp_buf, BT_SDP_ATTR_PROFILE_DESC_LIST,
						 &uuid128.uuid, &value);
		if (err != 0) {
			goto failed;
		}

		if (value != 0x1003) {
			LOG_ERR("Invalid service attr value");
			goto failed;
		}


		err = test_02_get_attr_value_u16(result->resp_buf, BT_SDP_ATTR_SUPPORTED_FEATURES,
						 NULL, &value);
		if (err != 0) {
			goto failed;
		}

		if (value != TEST_SUPPORTED_FEATURE) {
			LOG_ERR("Invalid service attr value");
			goto failed;
		}
	}

	atomic_set_bit(test_flags, TEST_FLAG_CASE_PASSED);
failed:
	k_sem_give(&br_sdp_discover_sem);
	return BT_SDP_DISCOVER_UUID_STOP;
}

ZTEST(sdp_client, test_02_one_sdp_record)
{
	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	sdp_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_uuid.u16.val = TEST_PROFILE_UUID_16_VALUE;
	test_sdp_discover_service_search(br_conn, &sdp_uuid.uuid, test_02_sdp_discover_cb);
	LOG_DBG("Test 02 - SDP discover service search with 16-bit UUID passed");

	sdp_uuid.uuid.type = BT_UUID_TYPE_32;
	sdp_uuid.u32.val = TEST_SERVICE_UUID_32_VALUE;
	test_sdp_discover_service_search(br_conn, &sdp_uuid.uuid, test_02_sdp_discover_cb);
	LOG_DBG("Test 02 - SDP discover service search with 32-bit UUID passed");

	sdp_uuid.uuid.type = BT_UUID_TYPE_128;
	memcpy(sdp_uuid.u128.val, TEST_SERVICE_UUID, sizeof(sdp_uuid.u128.val));
	test_sdp_discover_service_search(br_conn, &sdp_uuid.uuid, test_02_sdp_discover_cb);
	LOG_DBG("Test 02 - SDP discover service search with 128-bit UUID passed");

	test_sdp_discover_service_attr(br_conn, SDP_SERVICE_HANDLE_BASE, test_02_sdp_discover_cb);
	LOG_DBG("Test 02 - SDP discover service attr passed");

	sdp_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_uuid.u16.val = TEST_PROFILE_UUID_16_VALUE;
	test_sdp_discover_service_search_attr(br_conn, &sdp_uuid.uuid, test_02_sdp_discover_cb,
					      NULL);
	LOG_DBG("Test 02 - SDP discover service search attr with 16-bit UUID passed");

	sdp_uuid.uuid.type = BT_UUID_TYPE_32;
	sdp_uuid.u32.val = TEST_SERVICE_UUID_32_VALUE;
	test_sdp_discover_service_search_attr(br_conn, &sdp_uuid.uuid, test_02_sdp_discover_cb,
					      NULL);
	LOG_DBG("Test 02 - SDP discover service search attr with 32-bit UUID passed");

	sdp_uuid.uuid.type = BT_UUID_TYPE_128;
	memcpy(sdp_uuid.u128.val, TEST_SERVICE_UUID, sizeof(sdp_uuid.u128.val));
	test_sdp_discover_service_search_attr(br_conn, &sdp_uuid.uuid, test_02_sdp_discover_cb,
					      NULL);
	LOG_DBG("Test 02 - SDP discover service search attr with 128-bit UUID passed");

	br_disconnect_from_peer();
}

struct bt_sdp_attr_id_pair {
	uint16_t attr_id;
	uint8_t hit_count;
	uint8_t found_count;
	bool used;
};

static struct bt_sdp_attr_id_pair attr_id_pairs[CONFIG_TEST_SDP_DISCOVER_ATTR_TRACK_TABLE_SIZE];

static void test_range_attr_id_pair_reset(void)
{
	ARRAY_FOR_EACH(attr_id_pairs, i) {
		attr_id_pairs[i].used = false;
		attr_id_pairs[i].attr_id = 0;
		attr_id_pairs[i].hit_count = 0;
		attr_id_pairs[i].found_count = 0;
	}
}

static int test_range_attr_id_pair_index_get(uint16_t attr_id)
{
	ARRAY_FOR_EACH(attr_id_pairs, i) {
		if (attr_id_pairs[i].used && attr_id_pairs[i].attr_id == attr_id) {
			return i;
		}
	}
	return -EINVAL;
}

static bool test_range_attr_id_pair_found(uint16_t attr_id)
{
	ARRAY_FOR_EACH(attr_id_pairs, i) {
		if (attr_id_pairs[i].used && attr_id_pairs[i].attr_id == attr_id) {
			return true;
		}
	}
	return false;
}

static void test_range_attr_id_pair_found_count_reset(uint16_t attr_id)
{
	ARRAY_FOR_EACH(attr_id_pairs, i) {
		if (attr_id_pairs[i].used && attr_id_pairs[i].attr_id == attr_id) {
			attr_id_pairs[i].found_count = 0;
		}
	}
}

static void test_range_attr_id_pair_range_found_count_reset(uint16_t from, uint16_t to)
{
	ARRAY_FOR_EACH(attr_id_pairs, i) {
		if (attr_id_pairs[i].used && attr_id_pairs[i].attr_id >= from &&
		    attr_id_pairs[i].attr_id <= to) {
			test_range_attr_id_pair_found_count_reset(attr_id_pairs[i].attr_id);
		}
	}
}

static bool test_range_attr_id_pair_discover_done(uint16_t attr_id)
{
	ARRAY_FOR_EACH(attr_id_pairs, i) {
		if (attr_id_pairs[i].used && attr_id_pairs[i].attr_id == attr_id) {
			if (attr_id_pairs[i].hit_count != attr_id_pairs[i].found_count) {
				return false;
			}
		}
	}
	test_range_attr_id_pair_found_count_reset(attr_id);
	return true;
}

static int test_range_attr_id_pair_found_count_add(uint16_t attr_id)
{
	int err = test_range_attr_id_pair_index_get(attr_id);

	if (err < 0) {
		return err;
	}

	if (attr_id_pairs[err].found_count >= UINT8_MAX) {
		return -EOVERFLOW;
	}

	attr_id_pairs[err].found_count++;

	if (attr_id_pairs[err].found_count > attr_id_pairs[err].hit_count) {
		return -EINVAL;
	}

	return err;
}

static int test_range_attr_id_pair_hit_count_add(uint16_t attr_id)
{
	int err = test_range_attr_id_pair_index_get(attr_id);

	if (err != -EINVAL) {
		if (attr_id_pairs[err].hit_count >= UINT8_MAX) {
			return -EOVERFLOW;
		}

		attr_id_pairs[err].hit_count++;
		return err;
	}

	ARRAY_FOR_EACH(attr_id_pairs, i) {
		if (!attr_id_pairs[i].used) {
			attr_id_pairs[i].used = true;
			attr_id_pairs[i].attr_id = attr_id;
			attr_id_pairs[i].hit_count = 1;
			attr_id_pairs[i].found_count = 0;
			return i;
		}
	}
	return -EINVAL;
}

static int test_range_attr_id_pair_size(void)
{
	int count = 0;

	ARRAY_FOR_EACH(attr_id_pairs, i) {
		if (attr_id_pairs[i].used) {
			count++;
		}
	}

	return count;
}

static void test_range_attr_id_pair_sort(void)
{
	int count = test_range_attr_id_pair_size();

	if (count < 2) {
		return;
	}

	for (int i = 0; i < count; i++) {
		struct bt_sdp_attr_id_pair tmp;

		for (int j = i + 1; j < count; j++) {
			if (attr_id_pairs[i].attr_id > attr_id_pairs[j].attr_id) {
				tmp = attr_id_pairs[i];
				attr_id_pairs[i] = attr_id_pairs[j];
				attr_id_pairs[j] = tmp;
			}
		}
	}
}

static bool test_range_attr_id_pair_in_range(uint16_t start, uint16_t end)
{
	ARRAY_FOR_EACH(attr_id_pairs, i) {
		if (!attr_id_pairs[i].used) {
			continue;
		}

		if (attr_id_pairs[i].attr_id >= start && attr_id_pairs[i].attr_id <= end) {
			return true;
		}
	}

	return false;
}

static bool test_range_sdp_record_parse_cb(const struct bt_sdp_attribute *attr, void *user_data)
{
	int err;

	err = test_range_attr_id_pair_hit_count_add(attr->id);
	if (err < 0) {
		LOG_ERR("Failed to add attr id pair");
		*((int *)user_data) = err;
		return false;
	}

	return true;
}

static uint8_t test_range_sdp_discover_find_attr_ids_cb(struct bt_conn *conn,
							struct bt_sdp_client_result *result,
							const struct bt_sdp_discover_params *params)
{
	struct test_bt_sdp_discover_params *sdp_discover_params = TEST_SDP_DISOVER_PARAMS(params);
	int err;
	int parse_err = 0;

	LOG_DBG("SDP discovery callback");
	if (result == NULL || result->resp_buf == NULL || result->resp_buf->len == 0) {
		LOG_ERR("N/A response received");
		goto failed;
	}

	err = bt_sdp_record_parse(result->resp_buf, test_range_sdp_record_parse_cb, &parse_err);
	if (err != 0 || parse_err != 0) {
		LOG_ERR("Failed to parse SDP record");
		goto failed;
	}

	err = test_range_attr_id_pair_size();
	if (err == 0) {
		LOG_ERR("No attributes found in SDP record");
		goto failed;
	}

	sdp_discover_params->found_count++;
	if (sdp_discover_params->found_count < sdp_discover_params->expected_count) {
		LOG_DBG("SDP discovery result %u/%u", sdp_discover_params->found_count,
			sdp_discover_params->expected_count);
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	if (sdp_discover_params->found_count > sdp_discover_params->expected_count) {
		LOG_ERR("SDP discovery found more results than expected (%u > %u)",
			sdp_discover_params->found_count, sdp_discover_params->expected_count);
		goto failed;
	}

	atomic_set_bit(test_flags, TEST_FLAG_CASE_PASSED);
failed:
	k_sem_give(&br_sdp_discover_sem);
	return BT_SDP_DISCOVER_UUID_STOP;
}

static struct bt_sdp_attribute_id_list attr_ids;
static struct bt_sdp_attribute_id_range attr_id_ranges[1];

struct attr_id_hit_count {
	uint16_t id;
	uint8_t count;
};

static bool test_range_sdp_record_parse_attr_id_hit_count_cb(const struct bt_sdp_attribute *attr,
							     void *user_data)
{
	struct attr_id_hit_count *count = (struct attr_id_hit_count *)user_data;

	if (count->id == attr->id) {
		count->count++;
	}

	return true;
}

static int test_range_get_attr_id_pair_hit_count(struct net_buf *buf, uint16_t attr_id)
{
	struct attr_id_hit_count count;
	int err;

	count.id = attr_id;
	count.count = 0;
	err = bt_sdp_record_parse(buf, test_range_sdp_record_parse_attr_id_hit_count_cb, &count);
	if (err != 0) {
		LOG_ERR("Failed to parse SDP record");
		return err;
	}

	return count.count;
}

static uint8_t test_range_sdp_discover_with_range_cb(struct bt_conn *conn,
						     struct bt_sdp_client_result *result,
						     const struct bt_sdp_discover_params *params)
{
	int err;
	uint16_t beginning;
	uint16_t ending;
	bool in_range;
	bool discover_done;

	LOG_DBG("SDP discovery callback");

	if (params->ids == NULL || params->ids->count == 0) {
		LOG_ERR("No attribute IDs in params");
		goto failed;
	}

	if (params->ids->count != 1) {
		LOG_ERR("Expected 1 attribute ID range, got %u", params->ids->count);
		goto failed;
	}

	beginning = params->ids->ranges[0].beginning;
	ending = params->ids->ranges[0].ending;

	in_range = test_range_attr_id_pair_in_range(beginning, ending);
	if (result == NULL || result->resp_buf == NULL || result->resp_buf->len == 0) {
		if (in_range) {
			LOG_ERR("N/A response received");
			goto failed;
		}
		goto done;
	} else {
		LOG_HEXDUMP_DBG(result->resp_buf->data, result->resp_buf->len, "SDP response");
	}

	discover_done = true;
	for (uint16_t id = beginning; id <= ending; id++) {
		err = test_range_get_attr_id_pair_hit_count(result->resp_buf, id);
		if (err < 0) {
			LOG_ERR("Failed to get attr id pair hit count");
			goto failed;
		}

		if (err > 0 && !test_range_attr_id_pair_found(id)) {
			LOG_ERR("Attribute ID %u found but not in expected pairs", id);
			goto failed;
		}

		for (int i = 0; i < err; i++) {
			int return_err;

			return_err = test_range_attr_id_pair_found_count_add(id);
			if (return_err < 0) {
				LOG_ERR("Failed to add attr id pair found count (err %d)",
					return_err);
				goto failed;
			}
		}

		if (!test_range_attr_id_pair_discover_done(id)) {
			discover_done = false;
		}
	}

	if (!discover_done) {
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

done:
	atomic_set_bit(test_flags, TEST_FLAG_CASE_PASSED);
failed:
	k_sem_give(&br_sdp_discover_sem);
	return BT_SDP_DISCOVER_UUID_STOP;
}

static void test_range_sdp_discover_with_range(uint16_t beginning, uint16_t ending,
					       int *last_beginning, int *last_ending)
{
	if (*last_beginning >= beginning && *last_ending >= ending) {
		LOG_WRN("Skip the range (0x%04x - 0x%04x) due to the last range (%d - %d)",
			beginning, ending, *last_beginning, *last_ending);
		return;
	}

	*last_beginning = beginning;
	*last_ending = ending;

	test_range_attr_id_pair_range_found_count_reset(beginning, ending);
	attr_id_ranges[0].beginning = beginning;
	attr_id_ranges[0].ending = ending;
	sdp_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_uuid.u16.val = BT_SDP_PROTO_L2CAP;
	test_sdp_discover_service_search_attr(br_conn, &sdp_uuid.uuid,
					      test_range_sdp_discover_with_range_cb,
					      &attr_ids);
	LOG_DBG("SDP discover ssa with range (0x%04x - 0x%04x) passed",
		attr_id_ranges[0].beginning, attr_id_ranges[0].ending);
}

static void test_range_init_points(uint16_t a, uint16_t b, uint16_t point[6])
{
	if (a > 0) {
		point[0] = a - 1;
	} else {
		point[0] = a;
	}

	point[1] = a;

	if (a < UINT16_MAX) {
		point[2] = a + 1;
	} else {
		point[2] = a;
	}

	if (b > 0) {
		point[3] = b - 1;
	} else {
		point[3] = b;
	}

	point[4] = b;

	if (b < UINT16_MAX) {
		point[5] = b + 1;
	} else {
		point[5] = b;
	}
}

static void test_range_sdp_discover_starting(uint16_t a, uint16_t b, int *last_beginning,
					     int *last_ending)
{
	uint16_t point[6];

	if (a > b) {
		return;
	}

	test_range_init_points(a, b, point);

	ARRAY_FOR_EACH(point, i) {
		for (uint16_t j = i; j < ARRAY_SIZE(point); j++) {
			if (point[i] > point[j]) {
				continue;
			}
			test_range_sdp_discover_with_range(point[i], point[j],
							   last_beginning, last_ending);
		}
	}
}

ZTEST(sdp_client, test_03_one_sdp_record_with_range)
{
	int count;
	int last_beginning = -1;
	int last_ending = -1;

	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	sdp_discover_params.expected_count = 1;
	sdp_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_uuid.u16.val = BT_SDP_PROTO_L2CAP;
	test_range_attr_id_pair_reset();
	test_sdp_discover_service_search_attr(br_conn, &sdp_uuid.uuid,
					      test_range_sdp_discover_find_attr_ids_cb, NULL);
	LOG_DBG("Test 03 - SDP discover service search with range passed");

	test_range_attr_id_pair_sort();

	count = test_range_attr_id_pair_size();
	zassert_true(count > 0, "No attributes found");

	for (int i = 0; i < count; i++) {
		zassert_true(attr_id_pairs[i].used, "Attribute pair not used");
		zassert_true(attr_id_pairs[i].hit_count > 0, "Attribute pair hit count is zero");
		LOG_DBG("Attribute ID: 0x%04x, Hit count: %u", attr_id_pairs[i].attr_id,
			attr_id_pairs[i].hit_count);
	}

	attr_ids.count = ARRAY_SIZE(attr_id_ranges);
	attr_ids.ranges = attr_id_ranges;
	for (int i = 0; i < count; i++) {
		/* The attr ID of a should be attr_id_pairs[i].attr_id, and the attr ID of b
		 * should be attr_id_pairs[j].attr_id where j is in range [i + 1 ~ count - 1].
		 * But due to the high time cost, a smaller test set is used here.
		 */
		test_range_sdp_discover_starting(attr_id_pairs[i].attr_id, attr_id_pairs[i].attr_id,
						 &last_beginning, &last_ending);
	}

	br_disconnect_from_peer();
}

static int test_04_read_service_name(struct net_buf *buf, struct bt_sdp_attr_value *value)
{
	struct bt_sdp_attribute attr;
	int err;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_SVCNAME_PRIMARY, &attr);
	if (err < 0) {
		LOG_ERR("Failed to get service name attribute");
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, value);
	if (err < 0) {
		LOG_ERR("Failed to read service name attribute");
		return err;
	}

	if (value->type != BT_SDP_ATTR_VALUE_TYPE_TEXT) {
		LOG_ERR("Service name attribute type is invalid");
		return -EINVAL;
	}

	if (value->text.len <= 0) {
		LOG_ERR("Service name attribute length is invalid");
		return -EINVAL;
	}

	LOG_DBG("Service name: %.*s", value->text.len, value->text.text);

	return 0;
}

static uint8_t test_04_sdp_discover_cb(struct bt_conn *conn, struct bt_sdp_client_result *result,
				       const struct bt_sdp_discover_params *params)
{
	struct test_bt_sdp_discover_params *sdp_discover_params = TEST_SDP_DISOVER_PARAMS(params);

	LOG_DBG("SDP discovery callback");
	if (result == NULL || result->resp_buf == NULL || result->resp_buf->len == 0) {
		LOG_ERR("N/A response received");
		goto failed;
	}

	if (params->type == BT_SDP_DISCOVER_SERVICE_SEARCH) {
		uint32_t service_handle;

		sdp_discover_params->found_count += result->resp_buf->len / sizeof(service_handle);
		if (sdp_discover_params->found_count < sdp_discover_params->expected_count) {
			return BT_SDP_DISCOVER_UUID_CONTINUE;
		}

		if (sdp_discover_params->found_count > sdp_discover_params->expected_count) {
			LOG_ERR("Found %u SDP records, expected %u",
				sdp_discover_params->found_count,
				sdp_discover_params->expected_count);
			goto failed;
		}

		LOG_DBG("Found %u SDP records as expected", sdp_discover_params->found_count);
	}

	if (params->type == BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR) {
		struct bt_sdp_attr_value value;
		int err;

		if (!bt_sdp_has_attr(result->resp_buf, BT_SDP_ATTR_SVCNAME_PRIMARY)) {
			LOG_ERR("Service name attribute not found");
			goto failed;
		}

		err = test_04_read_service_name(result->resp_buf, &value);
		if (err < 0) {
			LOG_ERR("Failed to read service name");
			goto failed;
		}

		sdp_discover_params->found_count++;
		if (sdp_discover_params->found_count < sdp_discover_params->expected_count) {
			return BT_SDP_DISCOVER_UUID_CONTINUE;
		}

		if (sdp_discover_params->found_count > sdp_discover_params->expected_count) {
			LOG_ERR("Found %u SDP record service name, expected %u",
				sdp_discover_params->found_count,
				sdp_discover_params->expected_count);
			goto failed;
		}
	}

	atomic_set_bit(test_flags, TEST_FLAG_CASE_PASSED);
failed:
	k_sem_give(&br_sdp_discover_sem);
	return BT_SDP_DISCOVER_UUID_STOP;
}

ZTEST(sdp_client, test_04_multiple_sdp_records)
{
	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	sdp_discover_params.expected_count = TOTAL_TEST_SDP_RECORD_COUNT;
	sdp_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_uuid.u16.val = BT_SDP_PROTO_L2CAP;
	test_sdp_discover_service_search(br_conn, &sdp_uuid.uuid, test_04_sdp_discover_cb);

	sdp_discover_params.expected_count = TOTAL_TEST_SDP_RECORD_COUNT;
	sdp_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_uuid.u16.val = BT_SDP_PROTO_L2CAP;
	test_sdp_discover_service_search_attr(br_conn, &sdp_uuid.uuid, test_04_sdp_discover_cb,
					      NULL);

	br_disconnect_from_peer();
}

ZTEST(sdp_client, test_05_multiple_sdp_records_with_range)
{
	int count;
	int last_beginning = -1;
	int last_ending = -1;

	br_connect_to_peer();
	zassert_true(br_conn != NULL, "Connect handle is NULL");

	sdp_discover_params.expected_count = TOTAL_TEST_SDP_RECORD_COUNT;
	sdp_uuid.uuid.type = BT_UUID_TYPE_16;
	sdp_uuid.u16.val = BT_SDP_PROTO_L2CAP;
	test_range_attr_id_pair_reset();
	test_sdp_discover_service_search_attr(br_conn, &sdp_uuid.uuid,
					      test_range_sdp_discover_find_attr_ids_cb, NULL);
	LOG_DBG("Test 05 - SDP discover service search with range passed");

	test_range_attr_id_pair_sort();

	count = test_range_attr_id_pair_size();
	zassert_true(count > 0, "No attributes found");

	for (int i = 0; i < count; i++) {
		zassert_true(attr_id_pairs[i].used, "Attribute pair not used");
		zassert_true(attr_id_pairs[i].hit_count > 0, "Attribute pair hit count is zero");
		LOG_DBG("Attribute ID: 0x%04x, Hit count: %u", attr_id_pairs[i].attr_id,
			attr_id_pairs[i].hit_count);
	}

	attr_ids.count = ARRAY_SIZE(attr_id_ranges);
	attr_ids.ranges = attr_id_ranges;
	for (int i = 0; i < count; i++) {
		/* The attr ID of a should be attr_id_pairs[i].attr_id, and the attr ID of b
		 * should be attr_id_pairs[j].attr_id where j is in range [i + 1 ~ count - 1].
		 * But due to the high time cost, a smaller test set is used here.
		 */
		test_range_sdp_discover_starting(attr_id_pairs[i].attr_id, attr_id_pairs[i].attr_id,
						 &last_beginning, &last_ending);
	}

	br_disconnect_from_peer();
}

static void br_discover_timeout(const struct bt_br_discovery_result *results, size_t count)
{
	LOG_DBG("BR discovery done, found %zu devices", count);
}

static void br_discover_recv(const struct bt_br_discovery_result *result)
{
	char br_addr[BT_ADDR_STR_LEN];

	bt_addr_to_str(&result->addr, br_addr, sizeof(br_addr));

	LOG_DBG("[DEVICE]: %s, RSSI %i, COD %u", br_addr, result->rssi, sys_get_le24(result->cod));

	if (!bt_addr_eq(&peer_addr, &result->addr)) {
		return;
	}

	LOG_DBG("  Target %s is found", br_addr);
	atomic_set_bit(test_flags, TEST_FLAG_DEVICE_FOUND);
	k_sem_give(&br_discovery_found_sem);
}

static struct bt_br_discovery_cb br_discover = {
	.recv = br_discover_recv,
	.timeout = br_discover_timeout,
};

static void *setup(void)
{
	int err;

	LOG_DBG("Initializing Bluetooth");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	zassert_equal(err, 0, "Bluetooth init failed (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	LOG_DBG("Register discovery callback");
	bt_br_discovery_cb_register(&br_discover);

	br_discover_param.length = ARRAY_SIZE(br_discover_result);
	br_discover_param.limited = false;

	memset(br_discover_result, 0, sizeof(br_discover_result));

	LOG_DBG("Starting Bluetooth inquiry");
	err = bt_br_discovery_start(&br_discover_param, br_discover_result,
				    ARRAY_SIZE(br_discover_result));
	zassert_equal(err, 0, "Bluetooth inquiry failed (err %d)", err);

	return NULL;
}

static void teardown(void *f)
{
	int err;

	LOG_DBG("Disabling Bluetooth");

	/* De-initialize the Bluetooth Subsystem */
	err = bt_disable();
	zassert_equal(err, 0, "Bluetooth de-init failed (err %d)", err);

	LOG_DBG("Bluetooth de-initialized");
}

static void before(void *f)
{
	atomic_clear_bit(test_flags, TEST_FLAG_DEVICE_FOUND);
	atomic_clear_bit(test_flags, TEST_FLAG_CASE_PASSED);

	k_sem_reset(&br_connect_changed_sem);
	k_sem_reset(&br_discovery_found_sem);
	k_sem_reset(&br_sdp_discover_sem);
}

static void after(void *f)
{
	br_disconnect_from_peer();
}

ZTEST_SUITE(sdp_client, NULL, setup, before, after, teardown);
