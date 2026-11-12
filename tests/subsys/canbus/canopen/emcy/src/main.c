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

#define TEST_NODE_ID     0x30U
#define TEST_REMOTE_ID   0x05U
#define TEST_REMOTE_HB   50U
#define TEST_RPDO_TIMER  30U
#define TEST_COB_SYNC    0x080U
#define TEST_COB_EMCY    (0x080U + TEST_NODE_ID)
#define TEST_COB_RPDO1   (0x200U + TEST_NODE_ID)
#define TEST_COB_SDO_RX  (0x600U + TEST_NODE_ID)
#define TEST_COB_SDO_TX  (0x580U + TEST_NODE_ID)
#define TEST_COB_NMT_ERR (0x700U + TEST_NODE_ID)
#define TEST_COB_REMOTE  (0x700U + TEST_REMOTE_ID)
#define TEST_TIMEOUT     K_MSEC(100)
#define TEST_QUIET       K_MSEC(50)

#define TEST_REG_COMM (CANOPEN_ERR_REG_GENERIC | CANOPEN_ERR_REG_COMMUNICATION)

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

CAN_MSGQ_DEFINE(emcy_msgq, 16);
CAN_MSGQ_DEFINE(sdo_msgq, 4);
CAN_MSGQ_DEFINE(err_msgq, 8);
K_SEM_DEFINE(nmt_sem, 0, 8);
K_SEM_DEFINE(hb_sem, 0, 8);

static atomic_t last_hb_event;
static atomic_t last_hb_state;

static uint16_t rx_u16;

CANOPEN_OD_DEFINE(test_od,
	CANOPEN_OD_VAR(0x2000, CANOPEN_OD_TYPE_UNSIGNED16, CANOPEN_OD_RW | CANOPEN_OD_ATTR_RPDO,
		       rx_u16));

static const uint32_t rpdo1_map[] = {CANOPEN_PDO_MAP(0x2000, 0, 16)};

static const struct canopen_pdo_config test_rpdo[] = {
	{
		.map = rpdo1_map,
		.map_count = ARRAY_SIZE(rpdo1_map),
		.event_timer = TEST_RPDO_TIMER,
		.transmission_type = CANOPEN_PDO_TYPE_EVENT,
	},
};

static const uint32_t test_hb_consumer[] = {
	((uint32_t)TEST_REMOTE_ID << 16) | TEST_REMOTE_HB,
};

static void test_nmt_state_cb(struct canopen_node *node, enum canopen_nmt_state state,
			      void *user_data)
{
	ARG_UNUSED(node);
	ARG_UNUSED(state);
	ARG_UNUSED(user_data);

	k_sem_give(&nmt_sem);
}

static void test_hb_cb(struct canopen_node *node, uint8_t remote_id, enum canopen_hb_event event,
		       enum canopen_nmt_state state, void *user_data)
{
	ARG_UNUSED(node);
	ARG_UNUSED(user_data);

	zassert_equal(remote_id, TEST_REMOTE_ID);
	atomic_set(&last_hb_event, event);
	atomic_set(&last_hb_state, state);
	k_sem_give(&hb_sem);
}

static const struct canopen_node_config test_cfg = {
	.can = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
	.od = &test_od,
	.nmt_state_cb = test_nmt_state_cb,
	.hb_cb = test_hb_cb,
	.rpdo = test_rpdo,
	.rpdo_count = ARRAY_SIZE(test_rpdo),
	.hb_consumer = test_hb_consumer,
	.hb_consumer_count = ARRAY_SIZE(test_hb_consumer),
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

static void set_state(uint8_t cmd, enum canopen_nmt_state state)
{
	send_nmt(cmd);
	zassert_ok(k_sem_take(&nmt_sem, TEST_TIMEOUT), "no NMT state change");
	zassert_equal(canopen_nmt_get_state(&test_node), state);
}

static void send_remote_hb(uint8_t state)
{
	send_frame(TEST_COB_REMOTE, &state, 1U);
}

static void expect_emcy(uint16_t code, uint8_t reg, const uint8_t *msef)
{
	static const uint8_t zero[5];
	struct can_frame frame;

	zassert_ok(k_msgq_get(&emcy_msgq, &frame, TEST_TIMEOUT), "no EMCY 0x%04x", code);
	zassert_equal(frame.id, TEST_COB_EMCY);
	zassert_equal(frame.dlc, 8U);
	zassert_equal(sys_get_le16(frame.data), code, "EMCY code 0x%04x, expected 0x%04x",
		      sys_get_le16(frame.data), code);
	zassert_equal(frame.data[2], reg, "error register 0x%02x, expected 0x%02x",
		      frame.data[2], reg);
	zassert_mem_equal(&frame.data[3], (msef != NULL) ? msef : zero, 5U);
}

static void expect_no_emcy(void)
{
	struct can_frame frame = {0};

	zassert_equal(k_msgq_get(&emcy_msgq, &frame, TEST_QUIET), -EAGAIN,
		      "unexpected EMCY 0x%04x", sys_get_le16(frame.data));
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
	zassert_ok(k_msgq_get(&sdo_msgq, &frame, TEST_TIMEOUT), "no SDO response");

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
	uint32_t sdo_abort = sdo_write(index, sub, value, len);

	zassert_equal(sdo_abort, 0U, "write %04x:%02x aborted 0x%08x", index, sub, sdo_abort);
}

static uint32_t sdo_read(uint16_t index, uint8_t sub, uint32_t *value)
{
	return sdo_request(0x40U, index, sub, 0U, value);
}

static void expect_read(uint16_t index, uint8_t sub, uint32_t expected)
{
	uint32_t value = 0U;

	zassert_ok(sdo_read(index, sub, &value), "read %04x:%02x aborted", index, sub);
	zassert_equal(value, expected, "%04x:%02x = 0x%08x, expected 0x%08x", index, sub, value,
		      expected);
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

static void add_filter(struct k_msgq *msgq, uint32_t id)
{
	struct can_filter filter = {
		.id = id,
		.mask = CAN_STD_ID_MASK,
	};

	zassert_true(can_add_rx_filter_msgq(can_dev, msgq, &filter) >= 0);
}

static void *emcy_setup(void)
{
	zassert_true(device_is_ready(can_dev));
	zassert_ok(can_set_mode(can_dev, CAN_MODE_LOOPBACK));
	if (!canopen_test_self_reception(can_dev)) {
		ztest_test_skip();
		return NULL;
	}

	add_filter(&emcy_msgq, TEST_COB_EMCY);
	add_filter(&sdo_msgq, TEST_COB_SDO_TX);
	add_filter(&err_msgq, TEST_COB_NMT_ERR);

	zassert_ok(canopen_node_init(&test_node, &test_cfg));
	zassert_ok(canopen_node_start(&test_node));
	expect_bootup();

	return NULL;
}

static void emcy_before(void *fixture)
{
	ARG_UNUSED(fixture);

	send_nmt(0x81U);
	expect_bootup();
	k_msgq_purge(&emcy_msgq);
	k_msgq_purge(&sdo_msgq);
	k_sem_reset(&nmt_sem);
	k_sem_reset(&hb_sem);
}

ZTEST(canopen_emcy, test_objects)
{
	uint32_t value;

	expect_read(0x1001, 0U, 0U);
	expect_read(0x1003, 0U, 0U);
	expect_read(0x1014, 0U, TEST_COB_EMCY);
	expect_read(0x1015, 0U, 0U);
	expect_read(0x1016, 0U, CONFIG_CANOPEN_HB_CONSUMER_COUNT);
	expect_read(0x1016, 1U, test_hb_consumer[0]);
	expect_read(0x1016, 2U, 0U);
	expect_read(0x1029, 0U, 1U);
	expect_read(0x1029, 1U, 0U);

	zassert_equal(sdo_read(0x1003, 1U, &value), CANOPEN_SDO_ABORT_NO_DATA);
	zassert_equal(sdo_read(0x1003, CONFIG_CANOPEN_EMCY_HISTORY_SIZE + 1U, &value),
		      CANOPEN_SDO_ABORT_NO_SUB_INDEX);
	zassert_equal(sdo_write(0x1001, 0U, 1U, 1U), CANOPEN_SDO_ABORT_READ_ONLY);
	zassert_equal(sdo_write(0x1003, 1U, 1U, 4U), CANOPEN_SDO_ABORT_NO_DATA);
	zassert_equal(sdo_write(0x1029, 1U, 3U, 1U), CANOPEN_SDO_ABORT_INVALID_VALUE);
	zassert_equal(canopen_od_write(&test_node, 0x1001, 0U, &(uint8_t){1U}, 1U), -EACCES);
}

ZTEST(canopen_emcy, test_report_clear)
{
	static const uint8_t msef[5] = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U};
	const uint8_t reg_current = CANOPEN_ERR_REG_GENERIC | CANOPEN_ERR_REG_CURRENT;
	const uint8_t reg_both = reg_current | CANOPEN_ERR_REG_TEMPERATURE;

	zassert_equal(canopen_emcy_report(&test_node, 0x0000U, 0U, NULL), -EINVAL);

	zassert_ok(canopen_emcy_report(&test_node, 0x2100U, CANOPEN_ERR_REG_CURRENT, msef));
	expect_emcy(0x2100U, reg_current, msef);
	expect_read(0x1001, 0U, reg_current);
	expect_read(0x1003, 0U, 1U);
	expect_read(0x1003, 1U, 0x02012100U);

	zassert_equal(canopen_emcy_report(&test_node, 0x2100U, CANOPEN_ERR_REG_CURRENT, msef),
		      -EALREADY);
	expect_no_emcy();

	zassert_ok(canopen_emcy_report(&test_node, 0x4200U, CANOPEN_ERR_REG_TEMPERATURE, NULL));
	expect_emcy(0x4200U, reg_both, NULL);
	expect_read(0x1003, 0U, 2U);
	expect_read(0x1003, 1U, 0x00004200U);
	expect_read(0x1003, 2U, 0x02012100U);

	zassert_ok(canopen_emcy_clear(&test_node, 0x2100U));
	expect_emcy(CANOPEN_EMCY_NO_ERROR,
		    CANOPEN_ERR_REG_GENERIC | CANOPEN_ERR_REG_TEMPERATURE, NULL);
	zassert_ok(canopen_emcy_clear(&test_node, 0x4200U));
	expect_emcy(CANOPEN_EMCY_NO_ERROR, 0U, NULL);
	expect_read(0x1001, 0U, 0U);
	expect_read(0x1003, 0U, 2U);

	zassert_equal(canopen_emcy_clear(&test_node, 0x4200U), -ENOENT);
	expect_no_emcy();
}

ZTEST(canopen_emcy, test_active_limit)
{
	for (uint16_t i = 0U; i < CONFIG_CANOPEN_EMCY_ACTIVE_MAX; i++) {
		zassert_ok(canopen_emcy_report(&test_node, 0xFF00U + i, 0U, NULL));
	}

	zassert_equal(canopen_emcy_report(&test_node, 0xFFF0U, 0U, NULL), -ENOSPC);
	expect_read(0x1003, 0U, MIN(CONFIG_CANOPEN_EMCY_ACTIVE_MAX,
				     CONFIG_CANOPEN_EMCY_HISTORY_SIZE));
}

ZTEST(canopen_emcy, test_history_clear)
{
	zassert_ok(canopen_emcy_report(&test_node, 0x1000U, 0U, NULL));
	expect_emcy(0x1000U, CANOPEN_ERR_REG_GENERIC, NULL);
	expect_read(0x1003, 0U, 1U);

	zassert_equal(sdo_write(0x1003, 0U, 1U, 1U), CANOPEN_SDO_ABORT_INVALID_VALUE);
	sdo_write_ok(0x1003, 0U, 0U, 1U);
	expect_read(0x1003, 0U, 0U);
	expect_read(0x1001, 0U, CANOPEN_ERR_REG_GENERIC);
}

ZTEST(canopen_emcy, test_reset_node_clears_errors)
{
	zassert_ok(canopen_emcy_report(&test_node, 0x1000U, 0U, NULL));
	expect_emcy(0x1000U, CANOPEN_ERR_REG_GENERIC, NULL);

	send_nmt(0x82U);
	expect_bootup();
	expect_read(0x1001, 0U, CANOPEN_ERR_REG_GENERIC);
	expect_read(0x1003, 0U, 0U);

	send_nmt(0x81U);
	expect_bootup();
	expect_read(0x1001, 0U, 0U);
}

ZTEST(canopen_emcy, test_pending_while_stopped)
{
	set_state(0x02U, CANOPEN_NMT_STOPPED);
	zassert_ok(canopen_emcy_report(&test_node, 0x1000U, 0U, NULL));
	expect_no_emcy();

	set_state(0x80U, CANOPEN_NMT_PRE_OPERATIONAL);
	expect_emcy(0x1000U, CANOPEN_ERR_REG_GENERIC, NULL);
}

ZTEST(canopen_emcy, test_pending_order)
{
	set_state(0x02U, CANOPEN_NMT_STOPPED);
	for (uint16_t i = 0U; i < (CONFIG_CANOPEN_EMCY_QUEUE_SIZE / 2U); i++) {
		zassert_ok(canopen_emcy_report(&test_node, 0x1000U + i, 0U, NULL));
		zassert_ok(canopen_emcy_clear(&test_node, 0x1000U + i));
	}
	expect_no_emcy();

	set_state(0x80U, CANOPEN_NMT_PRE_OPERATIONAL);
	for (uint16_t i = 0U; i < (CONFIG_CANOPEN_EMCY_QUEUE_SIZE / 2U); i++) {
		expect_emcy(0x1000U + i, CANOPEN_ERR_REG_GENERIC, NULL);
		expect_emcy(CANOPEN_EMCY_NO_ERROR, 0U, NULL);
	}
}

ZTEST(canopen_emcy, test_inhibit_time)
{
	struct can_frame frame;
	int64_t start;

	sdo_write_ok(0x1015, 0U, 100U, 2U);
	expect_read(0x1015, 0U, 100U);

	zassert_ok(canopen_emcy_report(&test_node, 0x1000U, 0U, NULL));
	zassert_ok(canopen_emcy_report(&test_node, 0x2000U, 0U, NULL));

	zassert_ok(k_msgq_get(&emcy_msgq, &frame, TEST_TIMEOUT));
	start = k_uptime_get();
	zassert_equal(sys_get_le16(frame.data), 0x1000U);
	zassert_ok(k_msgq_get(&emcy_msgq, &frame, TEST_TIMEOUT));
	zassert_equal(sys_get_le16(frame.data), 0x2000U);
	zassert_true(k_uptime_delta(&start) >= 10, "inhibit time not respected");
}

ZTEST(canopen_emcy, test_cob_id)
{
	zassert_equal(sdo_write(0x1014, 0U, 0x40000000U | TEST_COB_EMCY, 4U),
		      CANOPEN_SDO_ABORT_INVALID_VALUE);
	zassert_equal(sdo_write(0x1014, 0U, TEST_COB_EMCY + 1U, 4U),
		      CANOPEN_SDO_ABORT_INVALID_VALUE);

	sdo_write_ok(0x1014, 0U, 0x80000000U | TEST_COB_EMCY, 4U);
	zassert_ok(canopen_emcy_report(&test_node, 0x1000U, 0U, NULL));
	expect_no_emcy();
	expect_read(0x1001, 0U, CANOPEN_ERR_REG_GENERIC);

	sdo_write_ok(0x1014, 0U, TEST_COB_EMCY, 4U);
	expect_no_emcy();
	zassert_ok(canopen_emcy_clear(&test_node, 0x1000U));
	expect_emcy(CANOPEN_EMCY_NO_ERROR, 0U, NULL);
}

ZTEST(canopen_emcy, test_hb_consumer_timeout)
{
	enum canopen_nmt_state state;

	zassert_equal(canopen_hb_consumer_state(&test_node, TEST_REMOTE_ID, &state), -EAGAIN);
	zassert_equal(canopen_hb_consumer_state(&test_node, TEST_REMOTE_ID + 1U, &state),
		      -ENOENT);

	set_state(0x01U, CANOPEN_NMT_OPERATIONAL);

	send_remote_hb(CANOPEN_NMT_OPERATIONAL);
	zassert_ok(k_sem_take(&hb_sem, TEST_TIMEOUT));
	zassert_equal(atomic_get(&last_hb_event), CANOPEN_HB_EVENT_STATE);
	zassert_equal(atomic_get(&last_hb_state), CANOPEN_NMT_OPERATIONAL);
	zassert_ok(canopen_hb_consumer_state(&test_node, TEST_REMOTE_ID, &state));
	zassert_equal(state, CANOPEN_NMT_OPERATIONAL);

	send_remote_hb(CANOPEN_NMT_OPERATIONAL);
	zassert_equal(k_sem_take(&hb_sem, K_MSEC(TEST_REMOTE_HB / 2)), -EAGAIN);

	zassert_ok(k_sem_take(&hb_sem, K_MSEC(TEST_REMOTE_HB * 2)));
	zassert_equal(atomic_get(&last_hb_event), CANOPEN_HB_EVENT_TIMEOUT);
	expect_emcy(CANOPEN_EMCY_HEARTBEAT, TEST_REG_COMM, NULL);
	zassert_ok(k_sem_take(&nmt_sem, TEST_TIMEOUT));
	zassert_equal(canopen_nmt_get_state(&test_node), CANOPEN_NMT_PRE_OPERATIONAL);
	zassert_equal(canopen_hb_consumer_state(&test_node, TEST_REMOTE_ID, &state),
		      -ETIMEDOUT);

	send_remote_hb(CANOPEN_NMT_PRE_OPERATIONAL);
	zassert_ok(k_sem_take(&hb_sem, TEST_TIMEOUT));
	zassert_equal(atomic_get(&last_hb_event), CANOPEN_HB_EVENT_STATE);
	zassert_equal(atomic_get(&last_hb_state), CANOPEN_NMT_PRE_OPERATIONAL);
	expect_emcy(CANOPEN_EMCY_NO_ERROR, 0U, NULL);
}

ZTEST(canopen_emcy, test_error_behavior)
{
	sdo_write_ok(0x1029, 1U, 2U, 1U);
	set_state(0x01U, CANOPEN_NMT_OPERATIONAL);
	send_remote_hb(CANOPEN_NMT_OPERATIONAL);
	zassert_ok(k_sem_take(&nmt_sem, K_MSEC(TEST_REMOTE_HB * 2)));
	zassert_equal(canopen_nmt_get_state(&test_node), CANOPEN_NMT_STOPPED);
	expect_no_emcy();

	set_state(0x80U, CANOPEN_NMT_PRE_OPERATIONAL);
	expect_emcy(CANOPEN_EMCY_HEARTBEAT, TEST_REG_COMM, NULL);

	sdo_write_ok(0x1029, 1U, 1U, 1U);
	set_state(0x01U, CANOPEN_NMT_OPERATIONAL);
	send_remote_hb(CANOPEN_NMT_OPERATIONAL);
	expect_emcy(CANOPEN_EMCY_NO_ERROR, 0U, NULL);
	zassert_equal(k_sem_take(&nmt_sem, K_MSEC(TEST_REMOTE_HB * 2)), -EAGAIN);
	expect_emcy(CANOPEN_EMCY_HEARTBEAT, TEST_REG_COMM, NULL);
	zassert_equal(canopen_nmt_get_state(&test_node), CANOPEN_NMT_OPERATIONAL);
}

ZTEST(canopen_emcy, test_hb_consumer_config)
{
	const uint32_t entry = ((uint32_t)(TEST_REMOTE_ID + 1U) << 16) | 100U;

	zassert_equal(sdo_write(0x1016, 2U, ((uint32_t)TEST_REMOTE_ID << 16) | 100U, 4U),
		      CANOPEN_SDO_ABORT_PARAM_INCOMPATIBLE);
	zassert_equal(sdo_write(0x1016, 2U, BIT(24) | entry, 4U),
		      CANOPEN_SDO_ABORT_INVALID_VALUE);
	sdo_write_ok(0x1016, 2U, entry, 4U);
	expect_read(0x1016, 2U, entry);
	sdo_write_ok(0x1016, 1U, 0U, 4U);
	sdo_write_ok(0x1016, 1U, ((uint32_t)TEST_REMOTE_ID << 16) | 100U, 4U);
}

ZTEST(canopen_emcy, test_rpdo_length)
{
	const uint8_t data[] = {0x34U, 0x12U};

	set_state(0x01U, CANOPEN_NMT_OPERATIONAL);

	send_frame(TEST_COB_RPDO1, data, 1U);
	expect_emcy(CANOPEN_EMCY_PDO_LENGTH, TEST_REG_COMM, NULL);
	send_frame(TEST_COB_RPDO1, data, 1U);
	expect_no_emcy();

	send_frame(TEST_COB_RPDO1, data, sizeof(data));
	expect_emcy(CANOPEN_EMCY_NO_ERROR, 0U, NULL);
	zassert_equal(rx_u16, 0x1234U);
}

ZTEST(canopen_emcy, test_rpdo_timeout)
{
	const uint8_t data[] = {0x78U, 0x56U};
	struct can_frame frame;
	int64_t start;

	set_state(0x01U, CANOPEN_NMT_OPERATIONAL);
	expect_no_emcy();

	send_frame(TEST_COB_RPDO1, data, sizeof(data));
	start = k_uptime_get();
	zassert_ok(k_msgq_get(&emcy_msgq, &frame, K_MSEC(TEST_RPDO_TIMER * 2)));
	zassert_within(k_uptime_delta(&start), TEST_RPDO_TIMER, 2);
	zassert_equal(sys_get_le16(frame.data), CANOPEN_EMCY_RPDO_TIMEOUT);
	expect_no_emcy();

	send_frame(TEST_COB_RPDO1, data, sizeof(data));
	expect_emcy(CANOPEN_EMCY_NO_ERROR, 0U, NULL);
	expect_emcy(CANOPEN_EMCY_RPDO_TIMEOUT, TEST_REG_COMM, NULL);

	set_state(0x80U, CANOPEN_NMT_PRE_OPERATIONAL);
	expect_emcy(CANOPEN_EMCY_NO_ERROR, 0U, NULL);
}

ZTEST(canopen_emcy, test_sync_length)
{
	const uint8_t counter = 1U;

	send_frame(TEST_COB_SYNC, &counter, 1U);
	expect_emcy(CANOPEN_EMCY_SYNC_LENGTH, TEST_REG_COMM, NULL);
	send_frame(TEST_COB_SYNC, &counter, 1U);
	expect_no_emcy();
	send_frame(TEST_COB_SYNC, NULL, 0U);
	expect_emcy(CANOPEN_EMCY_NO_ERROR, 0U, NULL);
}

ZTEST_SUITE(canopen_emcy, NULL, emcy_setup, emcy_before, NULL, NULL);
