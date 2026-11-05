/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/canbus/canopen.h>
#include <zephyr/drivers/can.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define TEST_NODE_ID     0x40U
#define TEST_COB_RPDO1   (0x200U + TEST_NODE_ID)
#define TEST_COB_SDO_RX  (0x600U + TEST_NODE_ID)
#define TEST_COB_SDO_TX  (0x580U + TEST_NODE_ID)
#define TEST_COB_NMT_ERR (0x700U + TEST_NODE_ID)
#define TEST_TIMEOUT     K_MSEC(100)

#define TEST_SAVE 0x65766173U
#define TEST_LOAD 0x64616F6CU

#define TEST_ATTR_PERSIST (CANOPEN_OD_RW | CANOPEN_OD_ATTR_PERSIST)
#define TEST_ATTR_MAP                                                                              \
	(CANOPEN_OD_RW | CANOPEN_OD_ATTR_PERSIST | CANOPEN_OD_ATTR_TPDO | CANOPEN_OD_ATTR_RPDO)

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

CAN_MSGQ_DEFINE(sdo_msgq, 4);
CAN_MSGQ_DEFINE(err_msgq, 8);
K_SEM_DEFINE(rpdo_sem, 0, 4);

static uint32_t persist_u32;
static uint16_t plain_u16;
static uint8_t persist_arr[2];
static uint8_t rec_persist;
static uint8_t rec_plain;
static uint16_t profile_u16;

CANOPEN_OD_DEFINE(test_od,
	CANOPEN_OD_VAR(0x2000, CANOPEN_OD_TYPE_UNSIGNED32, TEST_ATTR_PERSIST, persist_u32),
	CANOPEN_OD_VAR(0x2001, CANOPEN_OD_TYPE_UNSIGNED16, CANOPEN_OD_RW | CANOPEN_OD_ATTR_RPDO,
		       plain_u16),
	CANOPEN_OD_ARRAY(0x2002, CANOPEN_OD_TYPE_UNSIGNED8, TEST_ATTR_MAP, persist_arr),
	CANOPEN_OD_RECORD(0x2003,
		CANOPEN_OD_SUB(1U, CANOPEN_OD_TYPE_UNSIGNED8, TEST_ATTR_PERSIST, rec_persist),
		CANOPEN_OD_SUB(2U, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_RW, rec_plain)),
	CANOPEN_OD_VAR(0x6000, CANOPEN_OD_TYPE_UNSIGNED16, TEST_ATTR_PERSIST, profile_u16));

static const uint32_t rpdo1_map[] = {CANOPEN_PDO_MAP(0x2001, 0, 16)};

static const struct canopen_pdo_config test_rpdo[] = {
	{
		.map = rpdo1_map,
		.map_count = ARRAY_SIZE(rpdo1_map),
		.transmission_type = CANOPEN_PDO_TYPE_EVENT,
	},
};

static void test_reset_app_cb(struct canopen_node *node, void *user_data)
{
	ARG_UNUSED(node);
	ARG_UNUSED(user_data);

	persist_u32 = 0U;
	plain_u16 = 0U;
	memset(persist_arr, 0, sizeof(persist_arr));
	rec_persist = 0U;
	rec_plain = 0U;
	profile_u16 = 0U;
}

static void test_rpdo_cb(struct canopen_node *node, uint16_t pdo, void *user_data)
{
	ARG_UNUSED(node);
	ARG_UNUSED(pdo);
	ARG_UNUSED(user_data);

	k_sem_give(&rpdo_sem);
}

static const struct canopen_node_config test_cfg = {
	.can = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
	.od = &test_od,
	.reset_app_cb = test_reset_app_cb,
	.rpdo_cb = test_rpdo_cb,
	.rpdo = test_rpdo,
	.rpdo_count = ARRAY_SIZE(test_rpdo),
	.node_id = TEST_NODE_ID,
};

static struct canopen_node test_node;

static void send_frame(uint32_t id, const uint8_t *data, uint8_t dlc)
{
	struct can_frame frame = {
		.id = id,
		.dlc = dlc,
	};

	memcpy(frame.data, data, dlc);
	zassert_ok(can_send(can_dev, &frame, TEST_TIMEOUT, NULL, NULL));
}

static void expect_bootup(void)
{
	struct can_frame frame;

	while (k_msgq_get(&err_msgq, &frame, TEST_TIMEOUT) == 0) {
		if ((frame.dlc == 1U) && (frame.data[0] == 0x00U)) {
			return;
		}
	}

	zassert_unreachable("no boot-up frame");
}

static void send_nmt(uint8_t cmd)
{
	const uint8_t data[] = {cmd, TEST_NODE_ID};

	send_frame(0x000U, data, sizeof(data));
}

static void reset(uint8_t cmd)
{
	send_nmt(cmd);
	expect_bootup();
}

static uint32_t sdo_request(uint8_t cmd, uint16_t index, uint8_t sub, uint32_t value,
			    uint32_t *result)
{
	struct can_frame frame;
	uint8_t req[8] = {cmd};

	sys_put_le16(index, &req[1]);
	req[3] = sub;
	sys_put_le32(value, &req[4]);
	send_frame(TEST_COB_SDO_RX, req, sizeof(req));
	zassert_ok(k_msgq_get(&sdo_msgq, &frame, K_SECONDS(1)), "no SDO response");

	if (frame.data[0] == 0x80U) {
		return sys_get_le32(&frame.data[4]);
	}

	if (result != NULL) {
		*result = sys_get_le32(&frame.data[4]) &
			  GENMASK(31U - (8U * ((frame.data[0] >> 2) & 0x3U)), 0U);
	}

	return 0U;
}

static uint32_t sdo_write(uint16_t index, uint8_t sub, uint32_t value, uint8_t len)
{
	return sdo_request(0x23U | ((4U - len) << 2), index, sub, value, NULL);
}

static void sdo_write_ok(uint16_t index, uint8_t sub, uint32_t value, uint8_t len)
{
	uint32_t abort = sdo_write(index, sub, value, len);

	zassert_equal(abort, 0U, "write %04x:%02x aborted 0x%08x", index, sub, abort);
}

static void expect_read(uint16_t index, uint8_t sub, uint32_t expected)
{
	uint32_t value = 0U;

	zassert_ok(sdo_request(0x40U, index, sub, 0U, &value), "read %04x:%02x aborted", index,
		   sub);
	zassert_equal(value, expected, "%04x:%02x = 0x%08x, expected 0x%08x", index, sub, value,
		      expected);
}

static void add_filter(struct k_msgq *msgq, uint32_t id)
{
	struct can_filter filter = {
		.id = id,
		.mask = CAN_STD_ID_MASK,
	};

	zassert_true(can_add_rx_filter_msgq(can_dev, msgq, &filter) >= 0);
}

static void *storage_setup(void)
{
	zassert_true(device_is_ready(can_dev));
	zassert_ok(can_set_mode(can_dev, CAN_MODE_LOOPBACK));

	add_filter(&sdo_msgq, TEST_COB_SDO_TX);
	add_filter(&err_msgq, TEST_COB_NMT_ERR);

	zassert_ok(canopen_node_init(&test_node, &test_cfg));
	zassert_ok(canopen_node_start(&test_node));
	expect_bootup();

	return NULL;
}

static void storage_before(void *fixture)
{
	ARG_UNUSED(fixture);

	sdo_write_ok(0x1011, 1U, TEST_LOAD, 4U);
	reset(0x81U);
	k_msgq_purge(&sdo_msgq);
	k_sem_reset(&rpdo_sem);
}

ZTEST(canopen_storage, test_objects)
{
	uint32_t value;

	expect_read(0x1010, 0U, 4U);
	expect_read(0x1011, 0U, 4U);

	for (uint8_t sub = 1U; sub <= 4U; sub++) {
		expect_read(0x1010, sub, 1U);
		expect_read(0x1011, sub, 1U);
	}

	zassert_equal(sdo_request(0x40U, 0x1010, 5U, 0U, &value), CANOPEN_SDO_ABORT_NO_SUB_INDEX);
	zassert_equal(sdo_write(0x1010, 0U, 3U, 1U), CANOPEN_SDO_ABORT_READ_ONLY);
}

ZTEST(canopen_storage, test_wrong_signature)
{
	zassert_equal(sdo_write(0x1010, 1U, 0x12345678U, 4U), CANOPEN_SDO_ABORT_DATA_TRANSFER);
	zassert_equal(sdo_write(0x1011, 1U, TEST_SAVE, 4U), CANOPEN_SDO_ABORT_DATA_TRANSFER);
	zassert_equal(canopen_od_write(&test_node, 0x1010, 2U, &(uint32_t){TEST_LOAD}, 4U),
		      -EINVAL);
}

ZTEST(canopen_storage, test_comm_save_restore)
{
	const uint32_t hb_entry = (0x05U << 16) | 200U;

	sdo_write_ok(0x1017, 0U, 500U, 2U);
	sdo_write_ok(0x1800, 5U, 100U, 2U);
	sdo_write_ok(0x1016, 1U, hb_entry, 4U);
	sdo_write_ok(0x1029, 1U, 1U, 1U);
	sdo_write_ok(0x1006, 0U, 10000U, 4U);
	sdo_write_ok(0x1012, 0U, 0x00000100U, 4U);
	sdo_write_ok(0x1012, 0U, 0x400001A0U, 4U);
	sdo_write_ok(0x100C, 0U, 300U, 2U);
	sdo_write_ok(0x100D, 0U, 4U, 1U);
	sdo_write_ok(0x1010, 2U, TEST_SAVE, 4U);

	sdo_write_ok(0x1017, 0U, 0U, 2U);
	reset(0x82U);
	expect_read(0x1017, 0U, 500U);
	expect_read(0x1800, 5U, 100U);
	expect_read(0x1016, 1U, hb_entry);
	expect_read(0x1029, 1U, 1U);
	expect_read(0x1006, 0U, 10000U);
	expect_read(0x1012, 0U, 0x400001A0U);
	expect_read(0x100C, 0U, 300U);
	expect_read(0x100D, 0U, 4U);

	sdo_write_ok(0x1011, 2U, TEST_LOAD, 4U);
	expect_read(0x1017, 0U, 500U);

	reset(0x82U);
	expect_read(0x1017, 0U, CONFIG_CANOPEN_HEARTBEAT_PRODUCER_MS);
	expect_read(0x1800, 5U, 0U);
	expect_read(0x1016, 1U, 0U);
	expect_read(0x1029, 1U, 0U);
	expect_read(0x1006, 0U, 0U);
	expect_read(0x1012, 0U, 0x80000100U);
	expect_read(0x100C, 0U, 0U);
	expect_read(0x100D, 0U, 0U);
}

ZTEST(canopen_storage, test_pdo_mapping_saved)
{
	const uint32_t map = CANOPEN_PDO_MAP(0x2002, 2, 8);
	const uint8_t data[] = {0x77U};

	sdo_write_ok(0x1400, 1U, 0x80000000U | TEST_COB_RPDO1, 4U);
	sdo_write_ok(0x1600, 0U, 0U, 1U);
	sdo_write_ok(0x1600, 1U, map, 4U);
	sdo_write_ok(0x1600, 0U, 1U, 1U);
	sdo_write_ok(0x1400, 1U, TEST_COB_RPDO1, 4U);
	sdo_write_ok(0x1010, 2U, TEST_SAVE, 4U);

	reset(0x82U);
	expect_read(0x1600, 0U, 1U);
	expect_read(0x1600, 1U, map);
	expect_read(0x1400, 1U, TEST_COB_RPDO1);

	send_nmt(0x01U);
	k_msleep(1);
	send_frame(TEST_COB_RPDO1, data, sizeof(data));
	zassert_ok(k_sem_take(&rpdo_sem, TEST_TIMEOUT));
	zassert_equal(persist_arr[1], 0x77U);
}

ZTEST(canopen_storage, test_manufacturer_save_restore)
{
	sdo_write_ok(0x2000, 0U, 0xA5A5A5A5U, 4U);
	sdo_write_ok(0x2001, 0U, 0x1234U, 2U);
	sdo_write_ok(0x2002, 1U, 0x11U, 1U);
	sdo_write_ok(0x2002, 2U, 0x22U, 1U);
	sdo_write_ok(0x2003, 1U, 0x33U, 1U);
	sdo_write_ok(0x2003, 2U, 0x44U, 1U);
	sdo_write_ok(0x6000, 0U, 0x5566U, 2U);
	sdo_write_ok(0x1010, 4U, TEST_SAVE, 4U);

	reset(0x81U);
	zassert_equal(persist_u32, 0xA5A5A5A5U);
	zassert_equal(plain_u16, 0U);
	zassert_equal(persist_arr[0], 0x11U);
	zassert_equal(persist_arr[1], 0x22U);
	zassert_equal(rec_persist, 0x33U);
	zassert_equal(rec_plain, 0U);
	zassert_equal(profile_u16, 0U);

	sdo_write_ok(0x1011, 4U, TEST_LOAD, 4U);
	zassert_equal(persist_u32, 0xA5A5A5A5U);
	reset(0x81U);
	zassert_equal(persist_u32, 0U);
	zassert_equal(rec_persist, 0U);
}

ZTEST(canopen_storage, test_profile_save_restore)
{
	sdo_write_ok(0x6000, 0U, 0x5566U, 2U);
	sdo_write_ok(0x2000, 0U, 0xA5A5A5A5U, 4U);
	sdo_write_ok(0x1010, 3U, TEST_SAVE, 4U);

	reset(0x81U);
	zassert_equal(profile_u16, 0x5566U);
	zassert_equal(persist_u32, 0U);

	sdo_write_ok(0x1011, 3U, TEST_LOAD, 4U);
	reset(0x81U);
	zassert_equal(profile_u16, 0U);
}

ZTEST(canopen_storage, test_groups)
{
	sdo_write_ok(0x2000, 0U, 0x01020304U, 4U);
	sdo_write_ok(0x1017, 0U, 700U, 2U);

	sdo_write_ok(0x1010, 2U, TEST_SAVE, 4U);
	reset(0x81U);
	expect_read(0x1017, 0U, 700U);
	zassert_equal(persist_u32, 0U);

	sdo_write_ok(0x2000, 0U, 0x01020304U, 4U);
	sdo_write_ok(0x6000, 0U, 0x0506U, 2U);
	zassert_ok(canopen_od_write(&test_node, 0x1010, 1U, &(uint32_t){TEST_SAVE}, 4U));
	reset(0x81U);
	expect_read(0x1017, 0U, 700U);
	zassert_equal(persist_u32, 0x01020304U);
	zassert_equal(profile_u16, 0x0506U);

	sdo_write_ok(0x1011, 1U, TEST_LOAD, 4U);
	reset(0x81U);
	expect_read(0x1017, 0U, CONFIG_CANOPEN_HEARTBEAT_PRODUCER_MS);
	zassert_equal(persist_u32, 0U);
	zassert_equal(profile_u16, 0U);
}

ZTEST_SUITE(canopen_storage, NULL, storage_setup, storage_before, NULL, NULL);
