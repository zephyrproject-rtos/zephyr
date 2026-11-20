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

#include "self_reception.h"

#define TEST_NODE_ID     0x20U
#define TEST_COB_SYNC    0x080U
#define TEST_COB_TPDO1   (0x180U + TEST_NODE_ID)
#define TEST_COB_TPDO2   (0x280U + TEST_NODE_ID)
#define TEST_COB_TPDO3   (0x380U + TEST_NODE_ID)
#define TEST_COB_TPDO4   (0x480U + TEST_NODE_ID)
#define TEST_COB_RPDO1   (0x200U + TEST_NODE_ID)
#define TEST_COB_RPDO2   (0x300U + TEST_NODE_ID)
#define TEST_COB_SDO_RX  (0x600U + TEST_NODE_ID)
#define TEST_COB_SDO_TX  (0x580U + TEST_NODE_ID)
#define TEST_COB_NMT_ERR (0x700U + TEST_NODE_ID)
#define TEST_TIMEOUT     K_MSEC(100)
#define TEST_QUIET       K_MSEC(50)

#define TEST_ATTR_MAP (CANOPEN_OD_RW | CANOPEN_OD_ATTR_TPDO | CANOPEN_OD_ATTR_RPDO)
#define TEST_ATTR_RX  (CANOPEN_OD_RW | CANOPEN_OD_ATTR_RPDO)

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

CAN_MSGQ_DEFINE(pdo_msgq, 16);
CAN_MSGQ_DEFINE(sync_msgq, 8);
CAN_MSGQ_DEFINE(sdo_msgq, 4);
CAN_MSGQ_DEFINE(err_msgq, 8);
K_SEM_DEFINE(rpdo_sem, 0, 8);
K_SEM_DEFINE(sync_sem, 0, 8);
K_SEM_DEFINE(nmt_sem, 0, 8);

static int sync_filter = -1;
static atomic_t last_rpdo;
static atomic_t last_sync_counter;

static uint16_t app_u16;
static uint8_t app_u8;
static uint32_t app_u32;
static uint32_t app_nomap;
static uint16_t rx_u16;
static uint8_t rx_u8;
static uint32_t rx_u32;

CANOPEN_OD_DEFINE(test_od,
	CANOPEN_OD_VAR(0x2000, CANOPEN_OD_TYPE_UNSIGNED16, TEST_ATTR_MAP, app_u16),
	CANOPEN_OD_VAR(0x2001, CANOPEN_OD_TYPE_UNSIGNED8, TEST_ATTR_MAP, app_u8),
	CANOPEN_OD_VAR(0x2002, CANOPEN_OD_TYPE_UNSIGNED32, TEST_ATTR_MAP, app_u32),
	CANOPEN_OD_VAR(0x2003, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_RW, app_nomap),
	CANOPEN_OD_VAR(0x2100, CANOPEN_OD_TYPE_UNSIGNED16, TEST_ATTR_RX, rx_u16),
	CANOPEN_OD_VAR(0x2101, CANOPEN_OD_TYPE_UNSIGNED8, TEST_ATTR_RX, rx_u8),
	CANOPEN_OD_VAR(0x2102, CANOPEN_OD_TYPE_UNSIGNED32, TEST_ATTR_RX, rx_u32));

static const uint32_t tpdo1_map[] = {
	CANOPEN_PDO_MAP(0x2000, 0, 16),
	CANOPEN_PDO_MAP(0x2001, 0, 8),
	CANOPEN_PDO_MAP(0x2002, 0, 32),
};
static const uint32_t tpdo2_map[] = {CANOPEN_PDO_MAP(0x2001, 0, 8)};
static const uint32_t tpdo3_map[] = {CANOPEN_PDO_MAP(0x2000, 0, 16)};
static const uint32_t rpdo1_map[] = {
	CANOPEN_PDO_MAP(0x2100, 0, 16),
	CANOPEN_PDO_MAP(CANOPEN_OD_TYPE_UNSIGNED8, 0, 8),
	CANOPEN_PDO_MAP(0x2101, 0, 8),
};
static const uint32_t rpdo2_map[] = {CANOPEN_PDO_MAP(0x2102, 0, 32)};
static const uint32_t bad_map[] = {CANOPEN_PDO_MAP(0x2003, 0, 32)};

static const struct canopen_pdo_config test_tpdo[] = {
	{
		.map = tpdo1_map,
		.map_count = ARRAY_SIZE(tpdo1_map),
		.transmission_type = CANOPEN_PDO_TYPE_EVENT,
	},
	{
		.map = tpdo2_map,
		.map_count = ARRAY_SIZE(tpdo2_map),
		.transmission_type = 1U,
	},
	{
		.map = tpdo3_map,
		.map_count = ARRAY_SIZE(tpdo3_map),
		.transmission_type = CANOPEN_PDO_TYPE_SYNC_ACYCLIC,
	},
};

static const struct canopen_pdo_config test_rpdo[] = {
	{
		.map = rpdo1_map,
		.map_count = ARRAY_SIZE(rpdo1_map),
		.transmission_type = CANOPEN_PDO_TYPE_EVENT,
	},
	{
		.map = rpdo2_map,
		.map_count = ARRAY_SIZE(rpdo2_map),
		.transmission_type = 1U,
	},
};

static void test_nmt_state_cb(struct canopen_node *node, enum canopen_nmt_state state,
			      void *user_data)
{
	ARG_UNUSED(node);
	ARG_UNUSED(state);
	ARG_UNUSED(user_data);

	k_sem_give(&nmt_sem);
}

static void test_rpdo_cb(struct canopen_node *node, uint16_t pdo, void *user_data)
{
	ARG_UNUSED(node);
	ARG_UNUSED(user_data);

	atomic_set(&last_rpdo, pdo);
	k_sem_give(&rpdo_sem);
}

static void test_sync_cb(struct canopen_node *node, uint8_t counter, void *user_data)
{
	ARG_UNUSED(node);
	ARG_UNUSED(user_data);

	atomic_set(&last_sync_counter, counter);
	k_sem_give(&sync_sem);
}

static const struct canopen_node_config test_cfg = {
	.can = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
	.od = &test_od,
	.nmt_state_cb = test_nmt_state_cb,
	.rpdo_cb = test_rpdo_cb,
	.sync_cb = test_sync_cb,
	.tpdo = test_tpdo,
	.rpdo = test_rpdo,
	.tpdo_count = ARRAY_SIZE(test_tpdo),
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

	if (dlc > 0U) {
		memcpy(frame.data, data, dlc);
	}
	zassert_ok(can_send(can_dev, &frame, TEST_TIMEOUT, NULL, NULL));
}

static void send_nmt(uint8_t cmd)
{
	const uint8_t data[] = {cmd, TEST_NODE_ID};

	send_frame(0x000U, data, sizeof(data));
}

static void send_sync(void)
{
	send_frame(TEST_COB_SYNC, NULL, 0U);
}

static void send_sync_counter(uint8_t counter)
{
	send_frame(TEST_COB_SYNC, &counter, 1U);
}

static void expect_frame(struct k_msgq *msgq, uint32_t id, struct can_frame *frame)
{
	zassert_ok(k_msgq_get(msgq, frame, TEST_TIMEOUT), "no frame 0x%03x", id);
	zassert_equal(frame->id, id, "frame 0x%03x, expected 0x%03x", frame->id, id);
}

static void expect_quiet(struct k_msgq *msgq)
{
	struct can_frame frame = {0};

	zassert_equal(k_msgq_get(msgq, &frame, TEST_QUIET), -EAGAIN, "unexpected frame 0x%03x",
		      frame.id);
}

static void sdo_request(uint8_t cmd, uint16_t index, uint8_t sub, uint32_t value, uint8_t *resp)
{
	struct can_frame frame;
	uint8_t req[8] = {cmd};

	sys_put_le16(index, &req[1]);
	req[3] = sub;
	sys_put_le32(value, &req[4]);
	send_frame(TEST_COB_SDO_RX, req, sizeof(req));
	zassert_ok(k_msgq_get(&sdo_msgq, &frame, TEST_TIMEOUT), "no SDO response");
	memcpy(resp, frame.data, 8U);
}

static uint32_t sdo_write(uint16_t index, uint8_t sub, uint32_t value, uint8_t len)
{
	uint8_t resp[8];

	sdo_request(0x23U | ((4U - len) << 2), index, sub, value, resp);
	if (resp[0] == 0x80U) {
		return sys_get_le32(&resp[4]);
	}
	zassert_equal(resp[0], 0x60U);

	return 0U;
}

static uint32_t sdo_read(uint16_t index, uint8_t sub, uint32_t *value)
{
	uint8_t resp[8];

	sdo_request(0x40U, index, sub, 0U, resp);
	if (resp[0] == 0x80U) {
		return sys_get_le32(&resp[4]);
	}
	zassert_equal(resp[0] & 0xF3U, 0x43U);
	*value = sys_get_le32(&resp[4]) & GENMASK(31U - (8U * ((resp[0] >> 2) & 0x3U)), 0U);

	return 0U;
}

static void sdo_write_ok(uint16_t index, uint8_t sub, uint32_t value, uint8_t len)
{
	uint32_t sdo_abort = sdo_write(index, sub, value, len);

	zassert_equal(sdo_abort, 0U, "write %04x:%02x aborted 0x%08x", index, sub, sdo_abort);
}

static void expect_read(uint16_t index, uint8_t sub, uint32_t expected)
{
	uint32_t value = 0U;

	zassert_ok(sdo_read(index, sub, &value));
	zassert_equal(value, expected, "%04x:%02x = 0x%08x", index, sub, value);
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

static void set_state(uint8_t cmd, enum canopen_nmt_state state)
{
	send_nmt(cmd);
	zassert_ok(k_sem_take(&nmt_sem, TEST_TIMEOUT), "no NMT state change");
	zassert_equal(canopen_nmt_get_state(&test_node), state);
}

static void start_node(void)
{
	set_state(0x01U, CANOPEN_NMT_OPERATIONAL);
}

static int add_filter(struct k_msgq *msgq, uint32_t id)
{
	struct can_filter filter = {
		.id = id,
		.mask = CAN_STD_ID_MASK,
	};
	int filter_id = can_add_rx_filter_msgq(can_dev, msgq, &filter);

	zassert_true(filter_id >= 0);

	return filter_id;
}

static void monitor_sync(void)
{
	sync_filter = add_filter(&sync_msgq, TEST_COB_SYNC);
}

static void *pdo_setup(void)
{
	zassert_true(device_is_ready(can_dev));
	zassert_ok(can_set_mode(can_dev, CAN_MODE_LOOPBACK));
	if (!canopen_test_self_reception(can_dev)) {
		ztest_test_skip();
		return NULL;
	}

	(void)add_filter(&pdo_msgq, TEST_COB_TPDO1);
	(void)add_filter(&pdo_msgq, TEST_COB_TPDO2);
	(void)add_filter(&pdo_msgq, TEST_COB_TPDO3);
	(void)add_filter(&pdo_msgq, TEST_COB_TPDO4);
	(void)add_filter(&sdo_msgq, TEST_COB_SDO_TX);
	(void)add_filter(&err_msgq, TEST_COB_NMT_ERR);

	zassert_ok(canopen_node_init(&test_node, &test_cfg));
	zassert_ok(canopen_node_start(&test_node));
	expect_bootup();

	return NULL;
}

static void pdo_before(void *fixture)
{
	ARG_UNUSED(fixture);

	send_nmt(0x82U);
	expect_bootup();
	k_msgq_purge(&pdo_msgq);
	k_msgq_purge(&sync_msgq);
	k_msgq_purge(&sdo_msgq);
	k_sem_reset(&rpdo_sem);
	k_sem_reset(&sync_sem);
	k_sem_reset(&nmt_sem);
}

static void pdo_after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (sync_filter >= 0) {
		can_remove_rx_filter(can_dev, sync_filter);
		sync_filter = -1;
	}
}

ZTEST(canopen_pdo, test_config_invalid)
{
	static struct canopen_node node;
	struct canopen_pdo_config bad = test_tpdo[0];
	struct canopen_node_config cfg = test_cfg;

	cfg.tpdo = &bad;
	cfg.tpdo_count = 1U;

	bad.map = bad_map;
	bad.map_count = ARRAY_SIZE(bad_map);
	zassert_equal(canopen_node_init(&node, &cfg), -EINVAL);

	bad = test_tpdo[0];
	bad.transmission_type = 0xFCU;
	zassert_equal(canopen_node_init(&node, &cfg), -EINVAL);

	bad = test_tpdo[0];
	bad.cob_id = 0x40000000U | 0x701U;
	zassert_equal(canopen_node_init(&node, &cfg), -EINVAL);

	cfg = test_cfg;
	cfg.tpdo_count = CONFIG_CANOPEN_TPDO_COUNT + 1U;
	zassert_equal(canopen_node_init(&node, &cfg), -EINVAL);

	zassert_equal(canopen_tpdo_trigger(&test_node, 0U), -EINVAL);
	zassert_equal(canopen_tpdo_trigger(&test_node, CONFIG_CANOPEN_TPDO_COUNT + 1U), -EINVAL);
}

ZTEST(canopen_pdo, test_config_predefined_invalid)
{
	static struct canopen_node node;
	struct canopen_pdo_config pdo = test_tpdo[0];
	struct canopen_node_config cfg = test_cfg;
	uint32_t value;
	uint8_t count;

	pdo.cob_id = CANOPEN_PDO_COB_ID_INVALID;
	cfg.tpdo = &pdo;
	cfg.tpdo_count = 1U;
	cfg.rpdo_count = 0U;
	zassert_ok(canopen_node_init(&node, &cfg));
	zassert_ok(canopen_od_read(&node, 0x1800, 1U, &value, sizeof(value)));
	zassert_equal(value, 0xC0000000U | TEST_COB_TPDO1);
	zassert_ok(canopen_od_read(&node, 0x1A00, 0U, &count, sizeof(count)));
	zassert_equal(count, pdo.map_count);
}

ZTEST(canopen_pdo, test_pdo_objects)
{
	uint32_t value;

	expect_read(0x1800, 0U, 6U);
	expect_read(0x1800, 1U, 0x40000000U | TEST_COB_TPDO1);
	expect_read(0x1800, 2U, CANOPEN_PDO_TYPE_EVENT);
	expect_read(0x1801, 2U, 1U);
	expect_read(0x1803, 1U, 0xC0000000U | TEST_COB_TPDO4);
	expect_read(0x1A00, 0U, ARRAY_SIZE(tpdo1_map));
	expect_read(0x1A00, 1U, tpdo1_map[0]);
	expect_read(0x1A00, 3U, tpdo1_map[2]);
	expect_read(0x1400, 0U, 5U);
	expect_read(0x1400, 1U, TEST_COB_RPDO1);
	expect_read(0x1401, 2U, 1U);
	expect_read(0x1600, 2U, rpdo1_map[1]);

	zassert_equal(sdo_read(0x1800 + CONFIG_CANOPEN_TPDO_COUNT, 0U, &value),
		      CANOPEN_SDO_ABORT_NO_OBJECT);
	zassert_equal(sdo_read(0x1800, 4U, &value), CANOPEN_SDO_ABORT_NO_SUB_INDEX);
	zassert_equal(sdo_read(0x1400, 6U, &value), CANOPEN_SDO_ABORT_NO_SUB_INDEX);
	zassert_equal(sdo_read(0x1A00, CONFIG_CANOPEN_PDO_MAPPED_MAX + 1U, &value),
		      CANOPEN_SDO_ABORT_NO_SUB_INDEX);
	zassert_equal(sdo_write(0x1800, 0U, 5U, 1U), CANOPEN_SDO_ABORT_READ_ONLY);
}

ZTEST(canopen_pdo, test_no_pdo_in_pre_operational)
{
	const uint8_t data[] = {0x11U, 0x22U, 0x00U, 0x33U};

	rx_u16 = 0U;
	zassert_ok(canopen_tpdo_trigger(&test_node, 1U));
	expect_quiet(&pdo_msgq);

	send_frame(TEST_COB_RPDO1, data, sizeof(data));
	zassert_equal(k_sem_take(&rpdo_sem, TEST_QUIET), -EAGAIN);
	zassert_equal(rx_u16, 0U);
}

ZTEST(canopen_pdo, test_tpdo_event)
{
	struct can_frame frame;

	app_u16 = 0x1234U;
	app_u8 = 0x56U;
	app_u32 = 0x9ABCDEF0U;

	start_node();
	expect_frame(&pdo_msgq, TEST_COB_TPDO1, &frame);
	zassert_equal(frame.dlc, 7U);
	zassert_equal(sys_get_le16(&frame.data[0]), 0x1234U);
	zassert_equal(frame.data[2], 0x56U);
	zassert_equal(sys_get_le32(&frame.data[3]), 0x9ABCDEF0U);
	expect_quiet(&pdo_msgq);

	app_u8 = 0x78U;
	zassert_ok(canopen_tpdo_trigger(&test_node, 1U));
	expect_frame(&pdo_msgq, TEST_COB_TPDO1, &frame);
	zassert_equal(frame.data[2], 0x78U);
	expect_quiet(&pdo_msgq);
}

ZTEST(canopen_pdo, test_tpdo_inhibit_time)
{
	struct can_frame frame;
	int64_t start;

	sdo_write_ok(0x1800, 1U, 0xC0000000U | TEST_COB_TPDO1, 4U);
	sdo_write_ok(0x1800, 3U, 100U, 2U);
	sdo_write_ok(0x1800, 1U, 0x40000000U | TEST_COB_TPDO1, 4U);
	zassert_equal(sdo_write(0x1800, 3U, 50U, 2U), CANOPEN_SDO_ABORT_INVALID_VALUE);

	start_node();
	expect_frame(&pdo_msgq, TEST_COB_TPDO1, &frame);
	start = k_uptime_get();

	zassert_ok(canopen_tpdo_trigger(&test_node, 1U));
	zassert_ok(canopen_tpdo_trigger(&test_node, 1U));
	expect_frame(&pdo_msgq, TEST_COB_TPDO1, &frame);
	zassert_true(k_uptime_delta(&start) >= 10, "inhibit time not respected");
	expect_quiet(&pdo_msgq);
}

ZTEST(canopen_pdo, test_tpdo_event_timer)
{
	struct can_frame frame;
	int64_t start;

	sdo_write_ok(0x1800, 5U, 20U, 2U);

	start_node();
	expect_frame(&pdo_msgq, TEST_COB_TPDO1, &frame);
	start = k_uptime_get();
	expect_frame(&pdo_msgq, TEST_COB_TPDO1, &frame);
	zassert_within(k_uptime_delta(&start), 20, 2);
	expect_frame(&pdo_msgq, TEST_COB_TPDO1, &frame);
	zassert_within(k_uptime_delta(&start), 20, 2);

	sdo_write_ok(0x1800, 5U, 0U, 2U);
	k_msgq_purge(&pdo_msgq);
	expect_quiet(&pdo_msgq);
}

ZTEST(canopen_pdo, test_tpdo_sync)
{
	struct can_frame frame;

	start_node();
	expect_frame(&pdo_msgq, TEST_COB_TPDO1, &frame);

	send_sync();
	expect_frame(&pdo_msgq, TEST_COB_TPDO2, &frame);
	expect_quiet(&pdo_msgq);

	app_u16 = 0xBEEFU;
	zassert_ok(canopen_tpdo_trigger(&test_node, 3U));
	expect_quiet(&pdo_msgq);

	send_sync();
	expect_frame(&pdo_msgq, TEST_COB_TPDO2, &frame);
	expect_frame(&pdo_msgq, TEST_COB_TPDO3, &frame);
	zassert_equal(sys_get_le16(frame.data), 0xBEEFU);

	send_sync();
	expect_frame(&pdo_msgq, TEST_COB_TPDO2, &frame);
	expect_quiet(&pdo_msgq);
	zassert_ok(k_sem_take(&sync_sem, K_NO_WAIT));
}

ZTEST(canopen_pdo, test_tpdo_sync_cyclic)
{
	struct can_frame frame;
	int count = 0;

	sdo_write_ok(0x1801, 2U, 3U, 1U);
	start_node();
	expect_frame(&pdo_msgq, TEST_COB_TPDO1, &frame);

	for (int i = 1; i <= 6; i++) {
		send_sync();
		if ((i % 3) == 0) {
			expect_frame(&pdo_msgq, TEST_COB_TPDO2, &frame);
			count++;
		} else {
			expect_quiet(&pdo_msgq);
		}
	}

	zassert_equal(count, 2);
}

ZTEST(canopen_pdo, test_tpdo_sync_start)
{
	struct can_frame frame;

	sdo_write_ok(0x1019, 0U, 4U, 1U);
	sdo_write_ok(0x1801, 1U, 0xC0000000U | TEST_COB_TPDO2, 4U);
	sdo_write_ok(0x1801, 6U, 2U, 1U);
	sdo_write_ok(0x1801, 1U, 0x40000000U | TEST_COB_TPDO2, 4U);
	zassert_equal(sdo_write(0x1801, 6U, 3U, 1U), CANOPEN_SDO_ABORT_INVALID_VALUE);

	start_node();
	expect_frame(&pdo_msgq, TEST_COB_TPDO1, &frame);

	send_sync_counter(1U);
	expect_quiet(&pdo_msgq);
	send_sync_counter(2U);
	expect_frame(&pdo_msgq, TEST_COB_TPDO2, &frame);
	send_sync_counter(3U);
	expect_frame(&pdo_msgq, TEST_COB_TPDO2, &frame);
	zassert_ok(k_sem_take(&sync_sem, K_NO_WAIT));
	zassert_equal(atomic_get(&last_sync_counter), 3);
}

ZTEST(canopen_pdo, test_rpdo_event)
{
	const uint8_t data[] = {0x34U, 0x12U, 0xFFU, 0x56U};
	const uint8_t longer[] = {0x78U, 0x56U, 0xFFU, 0x9AU, 0x01U, 0x02U, 0x03U, 0x04U};

	rx_u16 = 0U;
	rx_u8 = 0U;
	start_node();

	send_frame(TEST_COB_RPDO1, data, sizeof(data));
	zassert_ok(k_sem_take(&rpdo_sem, TEST_TIMEOUT));
	zassert_equal(atomic_get(&last_rpdo), 1);
	zassert_equal(rx_u16, 0x1234U);
	zassert_equal(rx_u8, 0x56U);

	send_frame(TEST_COB_RPDO1, longer, 3U);
	zassert_equal(k_sem_take(&rpdo_sem, TEST_QUIET), -EAGAIN);
	zassert_equal(rx_u16, 0x1234U);

	send_frame(TEST_COB_RPDO1, longer, sizeof(longer));
	zassert_ok(k_sem_take(&rpdo_sem, TEST_TIMEOUT));
	zassert_equal(rx_u16, 0x5678U);
	zassert_equal(rx_u8, 0x9AU);
}

ZTEST(canopen_pdo, test_rpdo_sync)
{
	const uint8_t data[] = {0x44U, 0x33U, 0x22U, 0x11U};

	rx_u32 = 0U;
	start_node();

	send_frame(TEST_COB_RPDO2, data, sizeof(data));
	zassert_equal(k_sem_take(&rpdo_sem, TEST_QUIET), -EAGAIN);
	zassert_equal(rx_u32, 0U);

	send_sync();
	zassert_ok(k_sem_take(&rpdo_sem, TEST_TIMEOUT));
	zassert_equal(atomic_get(&last_rpdo), 2);
	zassert_equal(rx_u32, 0x11223344U);
}

ZTEST(canopen_pdo, test_rpdo_sync_window)
{
	const uint8_t data[] = {0x01U, 0x00U, 0x00U, 0x00U};

	rx_u32 = 0U;
	sdo_write_ok(0x1007, 0U, 5000U, 4U);
	start_node();

	send_sync();
	k_msleep(10);
	send_frame(TEST_COB_RPDO2, data, sizeof(data));
	send_sync();
	zassert_equal(k_sem_take(&rpdo_sem, TEST_QUIET), -EAGAIN);
	zassert_equal(rx_u32, 0U);

	send_sync();
	send_frame(TEST_COB_RPDO2, data, sizeof(data));
	send_sync();
	zassert_ok(k_sem_take(&rpdo_sem, TEST_TIMEOUT));
	zassert_equal(rx_u32, 1U);
}

ZTEST(canopen_pdo, test_remap)
{
	struct can_frame frame;

	zassert_equal(sdo_write(0x1A03, 1U, CANOPEN_PDO_MAP(0x2003, 0, 32), 4U),
		      CANOPEN_SDO_ABORT_NOT_MAPPABLE);
	zassert_equal(sdo_write(0x1A03, 1U, CANOPEN_PDO_MAP(0x2002, 0, 8), 4U),
		      CANOPEN_SDO_ABORT_NOT_MAPPABLE);
	zassert_equal(sdo_write(0x1A03, 1U, CANOPEN_PDO_MAP(0x2FFF, 0, 8), 4U),
		      CANOPEN_SDO_ABORT_NO_OBJECT);
	zassert_equal(sdo_write(0x1A03, 1U, CANOPEN_PDO_MAP(0x2100, 0, 16), 4U),
		      CANOPEN_SDO_ABORT_NOT_MAPPABLE);
	zassert_equal(sdo_write(0x1A03, 1U, CANOPEN_PDO_MAP(CANOPEN_OD_TYPE_UNSIGNED8, 0, 8), 4U),
		      CANOPEN_SDO_ABORT_NOT_MAPPABLE);

	sdo_write_ok(0x1A03, 1U, CANOPEN_PDO_MAP(0x2002, 0, 32), 4U);
	sdo_write_ok(0x1A03, 0U, 1U, 1U);
	zassert_equal(sdo_write(0x1A03, 1U, CANOPEN_PDO_MAP(0x2001, 0, 8), 4U),
		      CANOPEN_SDO_ABORT_UNSUPPORTED_ACCESS);
	sdo_write_ok(0x1803, 1U, 0x40000000U | TEST_COB_TPDO4, 4U);
	zassert_equal(sdo_write(0x1A03, 0U, 0U, 1U), CANOPEN_SDO_ABORT_UNSUPPORTED_ACCESS);

	zassert_equal(sdo_write(0x1602, 1U, CANOPEN_PDO_MAP(0x2102, 0, 32), 4U), 0U);
	zassert_equal(sdo_write(0x1602, 2U, CANOPEN_PDO_MAP(0x2102, 0, 32), 4U), 0U);
	zassert_equal(sdo_write(0x1602, 3U, CANOPEN_PDO_MAP(0x2102, 0, 32), 4U), 0U);
	zassert_equal(sdo_write(0x1602, 0U, 3U, 1U), CANOPEN_SDO_ABORT_MAP_LENGTH);
	zassert_equal(sdo_write(0x1602, 0U, CONFIG_CANOPEN_PDO_MAPPED_MAX + 1U, 1U),
		      CANOPEN_SDO_ABORT_MAP_LENGTH);

	app_u32 = 0x01020304U;
	start_node();
	expect_frame(&pdo_msgq, TEST_COB_TPDO1, &frame);
	expect_frame(&pdo_msgq, TEST_COB_TPDO4, &frame);
	zassert_equal(frame.dlc, 4U);
	zassert_equal(sys_get_le32(frame.data), 0x01020304U);
}

ZTEST(canopen_pdo, test_cob_id_rules)
{
	zassert_equal(sdo_write(0x1800, 1U, 0x40000000U | (TEST_COB_TPDO1 + 1U), 4U),
		      CANOPEN_SDO_ABORT_INVALID_VALUE);
	zassert_equal(sdo_write(0x1800, 1U, TEST_COB_TPDO1, 4U), CANOPEN_SDO_ABORT_INVALID_VALUE);
	zassert_equal(sdo_write(0x1800, 1U, 0x60000000U | TEST_COB_TPDO1, 4U),
		      CANOPEN_SDO_ABORT_INVALID_VALUE);
	zassert_equal(sdo_write(0x1800, 2U, 0xFCU, 1U), CANOPEN_SDO_ABORT_INVALID_VALUE);
	zassert_equal(sdo_write(0x1801, 3U, 10U, 2U), CANOPEN_SDO_ABORT_INVALID_VALUE);

	sdo_write_ok(0x1800, 1U, 0xC0000000U | TEST_COB_TPDO1, 4U);
	zassert_equal(sdo_write(0x1800, 1U, 0x40000050U, 4U), CANOPEN_SDO_ABORT_INVALID_VALUE);
	zassert_equal(sdo_write(0x1800, 1U, 0x40000701U, 4U), CANOPEN_SDO_ABORT_INVALID_VALUE);
	sdo_write_ok(0x1800, 1U, 0x40000190U, 4U);
	expect_read(0x1800, 1U, 0x40000190U);

	zassert_equal(sdo_write(0x1400, 1U, 0x000005A0U, 4U), CANOPEN_SDO_ABORT_INVALID_VALUE);
	zassert_equal(sdo_write(0x1400, 2U, 0xF1U, 1U), CANOPEN_SDO_ABORT_INVALID_VALUE);
}

ZTEST(canopen_pdo, test_rpdo_cob_id_change)
{
	const uint8_t data[] = {0xCDU, 0xABU, 0x00U, 0x01U};

	sdo_write_ok(0x1400, 1U, 0x80000000U | TEST_COB_RPDO1, 4U);
	sdo_write_ok(0x1400, 1U, 0x00000190U, 4U);
	start_node();

	send_frame(TEST_COB_RPDO1, data, sizeof(data));
	zassert_equal(k_sem_take(&rpdo_sem, TEST_QUIET), -EAGAIN);

	send_frame(0x190U, data, sizeof(data));
	zassert_ok(k_sem_take(&rpdo_sem, TEST_TIMEOUT));
	zassert_equal(rx_u16, 0xABCDU);
}

ZTEST(canopen_pdo, test_sync_producer)
{
	struct can_frame frame;
	int64_t start;

	monitor_sync();
	start_node();
	expect_frame(&pdo_msgq, TEST_COB_TPDO1, &frame);

	sdo_write_ok(0x1019, 0U, 3U, 1U);
	zassert_equal(sdo_write(0x1019, 0U, 1U, 1U), CANOPEN_SDO_ABORT_INVALID_VALUE);
	sdo_write_ok(0x1006, 0U, 10000U, 4U);
	zassert_equal(sdo_write(0x1019, 0U, 4U, 1U), CANOPEN_SDO_ABORT_DEVICE_STATE);
	sdo_write_ok(0x1005, 0U, 0x40000000U | TEST_COB_SYNC, 4U);
	zassert_equal(sdo_write(0x1005, 0U, 0x40000081U, 4U), CANOPEN_SDO_ABORT_INVALID_VALUE);

	for (uint8_t i = 0U; i < 4U; i++) {
		expect_frame(&sync_msgq, TEST_COB_SYNC, &frame);
		if (i == 0U) {
			start = k_uptime_get();
		} else {
			zassert_within(k_uptime_delta(&start), 10, 1);
		}
		zassert_equal(frame.dlc, 1U);
		zassert_equal(frame.data[0], (i % 3U) + 1U);
		expect_frame(&pdo_msgq, TEST_COB_TPDO2, &frame);
	}

	sdo_write_ok(0x1005, 0U, TEST_COB_SYNC, 4U);
	k_msgq_purge(&sync_msgq);
	expect_quiet(&sync_msgq);
}

ZTEST(canopen_pdo, test_sync_producer_stopped)
{
	struct can_frame frame;

	monitor_sync();
	sdo_write_ok(0x1006, 0U, 10000U, 4U);
	sdo_write_ok(0x1005, 0U, 0x40000000U | TEST_COB_SYNC, 4U);
	expect_frame(&sync_msgq, TEST_COB_SYNC, &frame);
	zassert_equal(frame.dlc, 0U);

	set_state(0x02U, CANOPEN_NMT_STOPPED);
	k_msgq_purge(&sync_msgq);
	expect_quiet(&sync_msgq);

	set_state(0x80U, CANOPEN_NMT_PRE_OPERATIONAL);
	expect_frame(&sync_msgq, TEST_COB_SYNC, &frame);
}

ZTEST(canopen_pdo, test_stopped)
{
	struct can_frame frame;

	start_node();
	expect_frame(&pdo_msgq, TEST_COB_TPDO1, &frame);

	set_state(0x02U, CANOPEN_NMT_STOPPED);
	zassert_ok(canopen_tpdo_trigger(&test_node, 1U));
	send_sync();
	expect_quiet(&pdo_msgq);
	zassert_equal(k_sem_take(&sync_sem, K_NO_WAIT), -EBUSY);
}

ZTEST_SUITE(canopen_pdo, NULL, pdo_setup, pdo_before, pdo_after, NULL);
