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

#define TEST_NODE_ID   0x60U
#define FAKE_NODE_ID   0x05U
#define FAKE_COB_RX    (0x600U + FAKE_NODE_ID)
#define FAKE_COB_TX    (0x580U + FAKE_NODE_ID)
#define TEST_TIMEOUT   K_MSEC(100)

enum fake_mode {
	FAKE_SILENT,
	FAKE_WRONG_MUX,
	FAKE_ABORT,
};

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

CAN_MSGQ_DEFINE(fake_msgq, 8);

static atomic_t fake_mode;
static atomic_t cb_result;
static atomic_t cb_try;

static uint32_t app_u32;
static char app_str[20];

CANOPEN_OD_DEFINE(test_od,
	CANOPEN_OD_VAR(0x2000, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_RW, app_u32),
	CANOPEN_OD_VAR(0x2001, CANOPEN_OD_TYPE_VISIBLE_STRING, CANOPEN_OD_RW, app_str));

static void test_nmt_state_cb(struct canopen_node *node, enum canopen_nmt_state state,
			      void *user_data)
{
	uint8_t buf[4];
	size_t len = sizeof(buf);

	ARG_UNUSED(state);
	ARG_UNUSED(user_data);

	if (atomic_cas(&cb_try, 1, 0)) {
		atomic_set(&cb_result, canopen_sdo_upload(node, TEST_NODE_ID, 0x1000, 0U, buf,
							  &len, NULL));
	}
}

static const struct canopen_node_config test_cfg = {
	.can = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
	.od = &test_od,
	.nmt_state_cb = test_nmt_state_cb,
	.node_id = TEST_NODE_ID,
};

static struct canopen_node test_node;

static void fake_reply(uint8_t cs, uint16_t index, uint32_t value)
{
	struct can_frame frame = {
		.id = FAKE_COB_TX,
		.dlc = 8U,
	};

	frame.data[0] = cs;
	sys_put_le16(index, &frame.data[1]);
	frame.data[3] = 0U;
	sys_put_le32(value, &frame.data[4]);
	(void)can_send(can_dev, &frame, K_FOREVER, NULL, NULL);
}

static void fake_server(void *p1, void *p2, void *p3)
{
	struct can_frame req;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		if (k_msgq_peek(&fake_msgq, &req) != 0) {
			k_msleep(1);
			continue;
		}

		switch (atomic_get(&fake_mode)) {
		case FAKE_WRONG_MUX:
			(void)k_msgq_get(&fake_msgq, &req, K_NO_WAIT);
			if (req.data[0] != 0x80U) {
				fake_reply(0x43U, sys_get_le16(&req.data[1]) + 1U, 0U);
			}
			break;
		case FAKE_ABORT:
			(void)k_msgq_get(&fake_msgq, &req, K_NO_WAIT);
			fake_reply(0x80U, sys_get_le16(&req.data[1]), CANOPEN_SDO_ABORT_HARDWARE);
			break;
		default:
			k_msleep(1);
			break;
		}
	}
}

K_THREAD_DEFINE(fake_tid, 1024, fake_server, NULL, NULL, NULL, 5, 0, 0);

static void expect_client_abort(uint32_t code)
{
	struct can_frame frame;

	while (k_msgq_get(&fake_msgq, &frame, TEST_TIMEOUT) == 0) {
		if (frame.data[0] == 0x80U) {
			zassert_equal(sys_get_le32(&frame.data[4]), code, "abort 0x%08x",
				      sys_get_le32(&frame.data[4]));
			return;
		}
	}

	zassert_unreachable("no abort from the SDO client");
}

static uint32_t od_u32(uint16_t index, uint8_t sub)
{
	uint32_t value = 0U;

	zassert_ok(canopen_od_read(&test_node, index, sub, &value, sizeof(value)));

	return value;
}

static void *master_setup(void)
{
	struct can_filter filter = {
		.id = FAKE_COB_RX,
		.mask = CAN_STD_ID_MASK,
	};
	uint8_t buf[4];
	size_t len = sizeof(buf);

	zassert_true(device_is_ready(can_dev));
	zassert_ok(can_set_mode(can_dev, CAN_MODE_LOOPBACK));
	zassert_true(can_add_rx_filter_msgq(can_dev, &fake_msgq, &filter) >= 0);

	zassert_ok(canopen_node_init(&test_node, &test_cfg));
	zassert_ok(canopen_node_start(&test_node));
	zassert_equal(canopen_nmt_get_state(&test_node), CANOPEN_NMT_PRE_OPERATIONAL);
	zassert_ok(canopen_sdo_upload(&test_node, TEST_NODE_ID, 0x1000, 0U, buf, &len, NULL));

	return NULL;
}

static void master_before(void *fixture)
{
	ARG_UNUSED(fixture);

	atomic_set(&fake_mode, FAKE_SILENT);
	zassert_ok(canopen_nmt_local_cmd(&test_node, CANOPEN_NMT_CMD_RESET_NODE));
	k_msgq_purge(&fake_msgq);
	app_u32 = 0U;
	memset(app_str, 0, sizeof(app_str));
}

ZTEST(canopen_master, test_client_objects)
{
	uint8_t u8;

	zassert_ok(canopen_od_read(&test_node, 0x1280, 0U, &u8, sizeof(u8)));
	zassert_equal(u8, 3U);
	zassert_equal(od_u32(0x1280, 1U), 0x80000000U);
	zassert_equal(od_u32(0x1280, 2U), 0x80000000U);

	zassert_equal(canopen_od_write(&test_node, 0x1280, 3U, &(uint8_t){0U}, 1U), -EINVAL);
	zassert_equal(canopen_od_write(&test_node, 0x1280, 1U, &(uint32_t){0x20000600U}, 4U),
		      -EINVAL);
	zassert_ok(canopen_od_write(&test_node, 0x1280, 1U, &(uint32_t){0x600U + TEST_NODE_ID},
				    4U));
	zassert_ok(canopen_od_write(&test_node, 0x1280, 2U, &(uint32_t){0x580U + TEST_NODE_ID},
				    4U));
	zassert_ok(canopen_od_write(&test_node, 0x1280, 3U, &(uint8_t){TEST_NODE_ID}, 1U));
	zassert_equal(canopen_od_write(&test_node, 0x1280, 1U, &(uint32_t){0x601U}, 4U), -EINVAL);
}

ZTEST(canopen_master, test_upload_expedited)
{
	uint8_t buf[4];
	size_t len = sizeof(buf);

	zassert_ok(canopen_sdo_upload(&test_node, TEST_NODE_ID, 0x1018, 0U, buf, &len, NULL));
	zassert_equal(len, 1U);
	zassert_equal(buf[0], 4U);

	len = sizeof(buf);
	zassert_ok(canopen_sdo_upload(&test_node, TEST_NODE_ID, 0x1200, 1U, buf, &len, NULL));
	zassert_equal(len, 4U);
	zassert_equal(sys_get_le32(buf), 0x600U + TEST_NODE_ID);

	zassert_equal(od_u32(0x1280, 1U), 0x600U + TEST_NODE_ID);
	zassert_equal(od_u32(0x1280, 2U), 0x580U + TEST_NODE_ID);
}

ZTEST(canopen_master, test_upload_segmented)
{
	char buf[40] = {0};
	size_t len = sizeof(buf);

	zassert_ok(canopen_sdo_upload(&test_node, TEST_NODE_ID, 0x1008, 0U, buf, &len, NULL));
	zassert_equal(len, strlen(CONFIG_CANOPEN_DEVICE_NAME));
	zassert_mem_equal(buf, CONFIG_CANOPEN_DEVICE_NAME, len);

	len = 4U;
	zassert_equal(canopen_sdo_upload(&test_node, TEST_NODE_ID, 0x1008, 0U, buf, &len, NULL),
		      -ENOSPC);
}

ZTEST(canopen_master, test_download)
{
	static const char str[] = "segmented value";
	uint8_t value[4];

	sys_put_le32(0xCAFEF00DU, value);
	zassert_ok(canopen_sdo_download(&test_node, TEST_NODE_ID, 0x2000, 0U, value,
					sizeof(value), NULL));
	zassert_equal(app_u32, 0xCAFEF00DU);

	zassert_ok(canopen_sdo_download(&test_node, TEST_NODE_ID, 0x2001, 0U, str, strlen(str),
					NULL));
	zassert_mem_equal(app_str, str, sizeof(str));
}

ZTEST(canopen_master, test_server_abort)
{
	uint8_t buf[4];
	size_t len = sizeof(buf);
	uint32_t abort = 0U;

	zassert_equal(canopen_sdo_upload(&test_node, TEST_NODE_ID, 0x2FFF, 0U, buf, &len, &abort),
		      -ECANCELED);
	zassert_equal(abort, CANOPEN_SDO_ABORT_NO_OBJECT);

	zassert_equal(canopen_sdo_download(&test_node, TEST_NODE_ID, 0x1000, 0U, buf, 4U, &abort),
		      -ECANCELED);
	zassert_equal(abort, CANOPEN_SDO_ABORT_READ_ONLY);

	atomic_set(&fake_mode, FAKE_ABORT);
	len = sizeof(buf);
	zassert_equal(canopen_sdo_upload(&test_node, FAKE_NODE_ID, 0x2000, 0U, buf, &len, &abort),
		      -ECANCELED);
	zassert_equal(abort, CANOPEN_SDO_ABORT_HARDWARE);
}

ZTEST(canopen_master, test_timeout)
{
	uint8_t buf[4];
	size_t len = sizeof(buf);
	int64_t start = k_uptime_get();

	zassert_equal(canopen_sdo_upload(&test_node, FAKE_NODE_ID, 0x1000, 0U, buf, &len, NULL),
		      -ETIMEDOUT);
	zassert_within(k_uptime_delta(&start), CONFIG_CANOPEN_SDO_CLIENT_TIMEOUT_MS, 20);
	expect_client_abort(CANOPEN_SDO_ABORT_TIMEOUT);
}

ZTEST(canopen_master, test_protocol_error)
{
	uint8_t buf[4];
	size_t len = sizeof(buf);

	atomic_set(&fake_mode, FAKE_WRONG_MUX);
	zassert_equal(canopen_sdo_upload(&test_node, FAKE_NODE_ID, 0x1000, 0U, buf, &len, NULL),
		      -EPROTO);
	atomic_set(&fake_mode, FAKE_SILENT);
	expect_client_abort(CANOPEN_SDO_ABORT_COMMAND);
}

ZTEST(canopen_master, test_invalid_use)
{
	uint8_t buf[4];
	size_t len = sizeof(buf);

	zassert_equal(canopen_sdo_upload(&test_node, 0U, 0x1000, 0U, buf, &len, NULL), -ENOTCONN);
	zassert_equal(canopen_sdo_upload(&test_node, 128U, 0x1000, 0U, buf, &len, NULL), -EINVAL);
	zassert_equal(canopen_sdo_upload(&test_node, TEST_NODE_ID, 0x1000, 0U, NULL, &len, NULL),
		      -EINVAL);
	zassert_equal(canopen_sdo_download(&test_node, TEST_NODE_ID, 0x2000, 0U, buf, 0U, NULL),
		      -EINVAL);

	zassert_ok(canopen_sdo_upload(&test_node, TEST_NODE_ID, 0x1000, 0U, buf, &len, NULL));
	len = sizeof(buf);
	zassert_ok(canopen_sdo_upload(&test_node, 0U, 0x1000, 0U, buf, &len, NULL));

	atomic_set(&cb_try, 1);
	zassert_ok(canopen_nmt_local_cmd(&test_node, CANOPEN_NMT_CMD_START));
	zassert_equal(atomic_get(&cb_result), -EDEADLK);
}

ZTEST(canopen_master, test_nmt_send)
{
	zassert_equal(canopen_nmt_send(&test_node, 128U, CANOPEN_NMT_CMD_START), -EINVAL);
	zassert_equal(canopen_nmt_send(&test_node, 0U, (enum canopen_nmt_cmd)0x55), -EINVAL);

	zassert_ok(canopen_nmt_send(&test_node, FAKE_NODE_ID, CANOPEN_NMT_CMD_START));
	k_msleep(5);
	zassert_equal(canopen_nmt_get_state(&test_node), CANOPEN_NMT_PRE_OPERATIONAL);

	zassert_ok(canopen_nmt_send(&test_node, TEST_NODE_ID, CANOPEN_NMT_CMD_START));
	k_msleep(5);
	zassert_equal(canopen_nmt_get_state(&test_node), CANOPEN_NMT_OPERATIONAL);

	zassert_ok(canopen_nmt_send(&test_node, 0U, CANOPEN_NMT_CMD_STOP));
	k_msleep(5);
	zassert_equal(canopen_nmt_get_state(&test_node), CANOPEN_NMT_STOPPED);
}

ZTEST(canopen_master, test_boot)
{
	static const struct canopen_boot_entry config[] = {
		{.index = 0x1017, .sub = 0U, .len = 2U, .value = 100U},
		{.index = 0x2000, .sub = 0U, .len = 4U, .value = 0x12345678U},
	};
	static const struct canopen_boot_slave slaves[] = {
		{.node_id = TEST_NODE_ID, .config = config, .config_count = ARRAY_SIZE(config)},
	};
	uint16_t hb = 0U;

	zassert_ok(canopen_master_boot(&test_node, slaves, ARRAY_SIZE(slaves), K_MSEC(500)));
	k_msleep(5);
	zassert_equal(canopen_nmt_get_state(&test_node), CANOPEN_NMT_OPERATIONAL);
	zassert_ok(canopen_od_read(&test_node, 0x1017, 0U, &hb, sizeof(hb)));
	zassert_equal(hb, 100U);
	zassert_equal(app_u32, 0x12345678U);
}

ZTEST(canopen_master, test_boot_missing_slave)
{
	static const struct canopen_boot_slave slaves[] = {
		{.node_id = FAKE_NODE_ID},
	};
	int64_t start = k_uptime_get();

	zassert_equal(canopen_master_boot(&test_node, slaves, ARRAY_SIZE(slaves), K_MSEC(300)),
		      -ETIMEDOUT);
	zassert_true(k_uptime_delta(&start) >= 300);
	zassert_true(k_uptime_delta(&start) < (300 + (2 * CONFIG_CANOPEN_SDO_CLIENT_TIMEOUT_MS)));
}

ZTEST_SUITE(canopen_master, NULL, master_setup, master_before, NULL, NULL);
