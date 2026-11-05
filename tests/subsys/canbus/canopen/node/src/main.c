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

#define TEST_NODE_ID     0x10U
#define TEST_COB_SDO_RX  (0x600U + TEST_NODE_ID)
#define TEST_COB_SDO_TX  (0x580U + TEST_NODE_ID)
#define TEST_COB_NMT_ERR (0x700U + TEST_NODE_ID)
#define TEST_TIMEOUT     K_MSEC(100)
#define TEST_SERIAL      0x0000CAFEU

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

CAN_MSGQ_DEFINE(sdo_msgq, 4);
CAN_MSGQ_DEFINE(err_msgq, 8);
K_SEM_DEFINE(nmt_sem, 0, 16);

static atomic_t reset_app_count;
static atomic_t last_state;

static uint8_t test_u8;
static int16_t test_i16;
static uint32_t test_u32;
static int32_t test_i24;
static uint64_t test_u64;
static uint32_t test_ro = 0x12345678U;
static uint32_t test_wo;
static char test_str[16];
static uint16_t test_array[3];
static uint8_t test_rec_a;
static uint32_t test_rec_b;
static uint8_t test_hooked;

static uint32_t test_hook(const struct canopen_od_entry *entry, uint8_t sub, const void *value,
			  size_t len, void *user_data)
{
	ARG_UNUSED(entry);
	ARG_UNUSED(sub);
	ARG_UNUSED(len);
	ARG_UNUSED(user_data);

	return (*(const uint8_t *)value > 100U) ? CANOPEN_SDO_ABORT_VALUE_TOO_HIGH
						: CANOPEN_SDO_ABORT_NONE;
}

CANOPEN_OD_DEFINE(test_od,
	CANOPEN_OD_VAR(0x2000, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_RW, test_u8),
	CANOPEN_OD_VAR(0x2001, CANOPEN_OD_TYPE_INTEGER16, CANOPEN_OD_RW, test_i16),
	CANOPEN_OD_VAR(0x2002, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_RW, test_u32),
	CANOPEN_OD_VAR(0x2003, CANOPEN_OD_TYPE_INTEGER24, CANOPEN_OD_RW, test_i24),
	CANOPEN_OD_VAR(0x2004, CANOPEN_OD_TYPE_UNSIGNED64, CANOPEN_OD_RW, test_u64),
	CANOPEN_OD_VAR(0x2005, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_RO, test_ro),
	CANOPEN_OD_VAR(0x2006, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_WO, test_wo),
	CANOPEN_OD_VAR(0x2007, CANOPEN_OD_TYPE_VISIBLE_STRING, CANOPEN_OD_RW, test_str),
	CANOPEN_OD_ARRAY(0x2008, CANOPEN_OD_TYPE_UNSIGNED16, CANOPEN_OD_RW, test_array),
	CANOPEN_OD_RECORD(0x2009,
		CANOPEN_OD_SUB(1U, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_RW, test_rec_a),
		CANOPEN_OD_SUB(3U, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_RW, test_rec_b)),
	CANOPEN_OD_VAR_HOOK(0x200A, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_RW, test_hooked,
			    test_hook, NULL));

static void test_nmt_state_cb(struct canopen_node *node, enum canopen_nmt_state state,
			      void *user_data)
{
	ARG_UNUSED(node);
	ARG_UNUSED(user_data);

	atomic_set(&last_state, state);
	k_sem_give(&nmt_sem);
}

static void test_reset_app_cb(struct canopen_node *node, void *user_data)
{
	ARG_UNUSED(node);
	ARG_UNUSED(user_data);

	test_u8 = 0U;
	atomic_inc(&reset_app_count);
}

static const struct canopen_node_config test_cfg = {
	.can = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
	.od = &test_od,
	.nmt_state_cb = test_nmt_state_cb,
	.reset_app_cb = test_reset_app_cb,
	.serial_number = TEST_SERIAL,
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

static void send_nmt(uint8_t cmd, uint8_t node_id)
{
	const uint8_t data[] = {cmd, node_id};

	send_frame(0x000U, data, sizeof(data));
}

static void expect_err_frame(uint8_t state, k_timeout_t timeout)
{
	struct can_frame frame;

	zassert_ok(k_msgq_get(&err_msgq, &frame, timeout), "no NMT error control frame");
	zassert_equal(frame.dlc, 1U);
	zassert_equal(frame.data[0], state, "state 0x%02x", frame.data[0]);
}

static void expect_state(enum canopen_nmt_state state)
{
	zassert_ok(k_sem_take(&nmt_sem, TEST_TIMEOUT), "no NMT state change");
	zassert_equal(atomic_get(&last_state), state);
	zassert_equal(canopen_nmt_get_state(&test_node), state);
}

static void sdo_request(const uint8_t *req, uint8_t *resp)
{
	struct can_frame frame;

	send_frame(TEST_COB_SDO_RX, req, 8U);
	zassert_ok(k_msgq_get(&sdo_msgq, &frame, TEST_TIMEOUT), "no SDO response");
	zassert_equal(frame.dlc, 8U);
	memcpy(resp, frame.data, 8U);
}

static void sdo_init_req(uint8_t *req, uint8_t cmd, uint16_t index, uint8_t sub)
{
	memset(req, 0, 8U);
	req[0] = cmd;
	sys_put_le16(index, &req[1]);
	req[3] = sub;
}

static uint32_t sdo_abort_code(const uint8_t *resp)
{
	return (resp[0] == 0x80U) ? sys_get_le32(&resp[4]) : 0U;
}

static uint32_t sdo_download(uint16_t index, uint8_t sub, const void *data, size_t len)
{
	const uint8_t *src = data;
	uint8_t req[8];
	uint8_t resp[8];
	uint8_t toggle = 0U;
	size_t pos = 0U;

	if (len <= 4U) {
		sdo_init_req(req, 0x23U | ((4U - len) << 2), index, sub);
		memcpy(&req[4], src, len);
		sdo_request(req, resp);
		if (sdo_abort_code(resp) != 0U) {
			return sdo_abort_code(resp);
		}
		zassert_equal(resp[0], 0x60U);
		zassert_mem_equal(&resp[1], &req[1], 3U);
		return 0U;
	}

	sdo_init_req(req, 0x21U, index, sub);
	sys_put_le32(len, &req[4]);
	sdo_request(req, resp);
	if (sdo_abort_code(resp) != 0U) {
		return sdo_abort_code(resp);
	}
	zassert_equal(resp[0], 0x60U);

	while (pos < len) {
		size_t n = MIN(len - pos, 7U);

		memset(req, 0, sizeof(req));
		req[0] = toggle | ((7U - n) << 1) | (((pos + n) == len) ? 1U : 0U);
		memcpy(&req[1], &src[pos], n);
		sdo_request(req, resp);
		if (sdo_abort_code(resp) != 0U) {
			return sdo_abort_code(resp);
		}
		zassert_equal(resp[0], 0x20U | toggle);
		pos += n;
		toggle ^= 0x10U;
	}

	return 0U;
}

static uint32_t sdo_upload(uint16_t index, uint8_t sub, void *buf, size_t max, size_t *len)
{
	uint8_t *dst = buf;
	uint8_t req[8];
	uint8_t resp[8];
	uint8_t toggle = 0U;
	size_t size;
	size_t pos = 0U;

	sdo_init_req(req, 0x40U, index, sub);
	sdo_request(req, resp);
	if (sdo_abort_code(resp) != 0U) {
		return sdo_abort_code(resp);
	}
	zassert_equal(resp[0] & 0xE1U, 0x41U);
	zassert_mem_equal(&resp[1], &req[1], 3U);

	if ((resp[0] & 0x02U) != 0U) {
		*len = 4U - ((resp[0] >> 2) & 0x3U);
		zassert_true(*len <= max);
		memcpy(dst, &resp[4], *len);
		return 0U;
	}

	size = sys_get_le32(&resp[4]);
	zassert_true(size <= max);

	while (true) {
		size_t n;

		memset(req, 0, sizeof(req));
		req[0] = 0x60U | toggle;
		sdo_request(req, resp);
		if (sdo_abort_code(resp) != 0U) {
			return sdo_abort_code(resp);
		}
		zassert_equal(resp[0] & 0xF0U, toggle);
		n = 7U - ((resp[0] >> 1) & 0x7U);
		zassert_true((pos + n) <= size);
		memcpy(&dst[pos], &resp[1], n);
		pos += n;
		if ((resp[0] & 0x01U) != 0U) {
			break;
		}
		toggle ^= 0x10U;
	}

	zassert_equal(pos, size);
	*len = pos;

	return 0U;
}

static uint32_t sdo_upload_u32(uint16_t index, uint8_t sub, uint32_t *value)
{
	uint8_t buf[4] = {0};
	size_t len = 0U;
	uint32_t abort = sdo_upload(index, sub, buf, sizeof(buf), &len);

	*value = sys_get_le32(buf);

	return abort;
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

static void reset_communication(void)
{
	send_nmt(0x82U, TEST_NODE_ID);
	expect_bootup();
	expect_state(CANOPEN_NMT_PRE_OPERATIONAL);
}

static void *canopen_setup(void)
{
	struct can_filter filter = {
		.mask = CAN_STD_ID_MASK,
	};

	zassert_true(device_is_ready(can_dev));
	zassert_ok(can_set_mode(can_dev, CAN_MODE_LOOPBACK));

	filter.id = TEST_COB_SDO_TX;
	zassert_true(can_add_rx_filter_msgq(can_dev, &sdo_msgq, &filter) >= 0);
	filter.id = TEST_COB_NMT_ERR;
	zassert_true(can_add_rx_filter_msgq(can_dev, &err_msgq, &filter) >= 0);

	zassert_ok(canopen_node_init(&test_node, &test_cfg));
	zassert_ok(canopen_node_start(&test_node));

	expect_bootup();
	expect_state(CANOPEN_NMT_PRE_OPERATIONAL);
	zassert_equal(atomic_get(&reset_app_count), 1);

	return NULL;
}

static void canopen_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_reset(&nmt_sem);
	k_msgq_purge(&sdo_msgq);
	reset_communication();
	k_msgq_purge(&err_msgq);
}

ZTEST(canopen_node, test_init_invalid)
{
	static struct canopen_node node;
	struct canopen_node_config cfg = test_cfg;

	cfg.node_id = 0U;
	zassert_equal(canopen_node_init(&node, &cfg), -EINVAL);
	cfg.node_id = 128U;
	zassert_equal(canopen_node_init(&node, &cfg), -EINVAL);
}

ZTEST(canopen_node, test_comm_objects)
{
	uint32_t value;

	zassert_ok(sdo_upload_u32(0x1000, 0U, &value));
	zassert_equal(value, CONFIG_CANOPEN_DEVICE_TYPE);
	zassert_ok(sdo_upload_u32(0x1001, 0U, &value));
	zassert_equal(value, 0U);
	zassert_ok(sdo_upload_u32(0x1018, 0U, &value));
	zassert_equal(value, 4U);
	zassert_ok(sdo_upload_u32(0x1018, 1U, &value));
	zassert_equal(value, CONFIG_CANOPEN_VENDOR_ID);
	zassert_ok(sdo_upload_u32(0x1018, 4U, &value));
	zassert_equal(value, TEST_SERIAL);
	zassert_ok(sdo_upload_u32(0x1200, 0U, &value));
	zassert_equal(value, 2U);
	zassert_ok(sdo_upload_u32(0x1200, 1U, &value));
	zassert_equal(value, TEST_COB_SDO_RX);
	zassert_ok(sdo_upload_u32(0x1200, 2U, &value));
	zassert_equal(value, TEST_COB_SDO_TX);
}

ZTEST(canopen_node, test_comm_aborts)
{
	uint32_t value = 0U;

	zassert_equal(sdo_upload_u32(0x1009, 0U, &value), CANOPEN_SDO_ABORT_NO_OBJECT);
	zassert_equal(sdo_upload_u32(0x1018, 5U, &value), CANOPEN_SDO_ABORT_NO_SUB_INDEX);
	zassert_equal(sdo_download(0x1000, 0U, &value, sizeof(value)),
		      CANOPEN_SDO_ABORT_READ_ONLY);
	zassert_equal(sdo_download(0x1018, 1U, &value, sizeof(value)),
		      CANOPEN_SDO_ABORT_READ_ONLY);
}

ZTEST(canopen_node, test_device_name_segmented)
{
	char buf[32] = {0};
	size_t len = 0U;

	zassert_ok(sdo_upload(0x1008, 0U, buf, sizeof(buf), &len));
	zassert_equal(len, strlen(CONFIG_CANOPEN_DEVICE_NAME));
	zassert_mem_equal(buf, CONFIG_CANOPEN_DEVICE_NAME, len);
}

ZTEST(canopen_node, test_expedited_numeric)
{
	const uint8_t u8 = 0xA5U;
	const uint8_t i16[] = {0xF6U, 0xFEU};
	const uint8_t u32[] = {0x78U, 0x56U, 0x34U, 0x12U};
	const uint8_t i24[] = {0x00U, 0x00U, 0x80U};
	uint8_t buf[4];
	size_t len;

	zassert_ok(sdo_download(0x2000, 0U, &u8, sizeof(u8)));
	zassert_equal(test_u8, 0xA5U);
	zassert_ok(sdo_download(0x2001, 0U, i16, sizeof(i16)));
	zassert_equal(test_i16, -266);
	zassert_ok(sdo_download(0x2002, 0U, u32, sizeof(u32)));
	zassert_equal(test_u32, 0x12345678U);
	zassert_ok(sdo_download(0x2003, 0U, i24, sizeof(i24)));
	zassert_equal(test_i24, -8388608);

	zassert_ok(sdo_upload(0x2001, 0U, buf, sizeof(buf), &len));
	zassert_equal(len, sizeof(i16));
	zassert_mem_equal(buf, i16, sizeof(i16));
	zassert_ok(sdo_upload(0x2003, 0U, buf, sizeof(buf), &len));
	zassert_equal(len, sizeof(i24));
	zassert_mem_equal(buf, i24, sizeof(i24));
}

ZTEST(canopen_node, test_segmented_numeric)
{
	const uint8_t u64[] = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U};
	uint8_t buf[8];
	size_t len;

	zassert_ok(sdo_download(0x2004, 0U, u64, sizeof(u64)));
	zassert_equal(test_u64, 0x0807060504030201ULL);
	zassert_ok(sdo_upload(0x2004, 0U, buf, sizeof(buf), &len));
	zassert_equal(len, sizeof(u64));
	zassert_mem_equal(buf, u64, sizeof(u64));
}

ZTEST(canopen_node, test_string)
{
	static const char str[] = "CANopen string";
	char buf[16];
	size_t len;

	zassert_ok(sdo_download(0x2007, 0U, str, strlen(str)));
	zassert_mem_equal(test_str, str, sizeof(str));
	zassert_ok(sdo_upload(0x2007, 0U, buf, sizeof(buf), &len));
	zassert_equal(len, strlen(str));
	zassert_mem_equal(buf, str, len);

	zassert_ok(sdo_download(0x2007, 0U, "ab", 2U));
	zassert_equal(strlen(test_str), 2U);
	zassert_equal(sdo_download(0x2007, 0U, "0123456789abcdefg", 17U),
		      CANOPEN_SDO_ABORT_LENGTH_TOO_HIGH);
}

ZTEST(canopen_node, test_array_record)
{
	const uint8_t u16[] = {0x34U, 0x12U};
	uint32_t value;

	zassert_ok(sdo_upload_u32(0x2008, 0U, &value));
	zassert_equal(value, ARRAY_SIZE(test_array));
	zassert_ok(sdo_download(0x2008, 3U, u16, sizeof(u16)));
	zassert_equal(test_array[2], 0x1234U);
	zassert_equal(sdo_upload_u32(0x2008, 4U, &value), CANOPEN_SDO_ABORT_NO_SUB_INDEX);
	zassert_equal(sdo_download(0x2008, 0U, u16, 1U), CANOPEN_SDO_ABORT_READ_ONLY);

	zassert_ok(sdo_upload_u32(0x2009, 0U, &value));
	zassert_equal(value, 3U);
	zassert_ok(sdo_download(0x2009, 3U, &(uint32_t){sys_cpu_to_le32(0xAABBCCDDU)}, 4U));
	zassert_equal(test_rec_b, 0xAABBCCDDU);
	zassert_equal(sdo_upload_u32(0x2009, 2U, &value), CANOPEN_SDO_ABORT_NO_SUB_INDEX);
}

ZTEST(canopen_node, test_access_aborts)
{
	uint32_t value = 0U;

	zassert_equal(sdo_upload_u32(0x2FFF, 0U, &value), CANOPEN_SDO_ABORT_NO_OBJECT);
	zassert_equal(sdo_upload_u32(0x2000, 1U, &value), CANOPEN_SDO_ABORT_NO_SUB_INDEX);
	zassert_equal(sdo_download(0x2005, 0U, &value, sizeof(value)),
		      CANOPEN_SDO_ABORT_READ_ONLY);
	zassert_equal(sdo_upload_u32(0x2006, 0U, &value), CANOPEN_SDO_ABORT_WRITE_ONLY);
	zassert_equal(sdo_download(0x2000, 0U, &value, 2U), CANOPEN_SDO_ABORT_LENGTH_TOO_HIGH);
	zassert_equal(sdo_download(0x2002, 0U, &value, 2U), CANOPEN_SDO_ABORT_LENGTH_TOO_LOW);
	zassert_equal(sdo_download(0x2004, 0U, "0123456789", 10U),
		      CANOPEN_SDO_ABORT_LENGTH_TOO_HIGH);
}

ZTEST(canopen_node, test_write_hook)
{
	const uint8_t ok = 100U;
	const uint8_t bad = 101U;

	zassert_ok(sdo_download(0x200A, 0U, &ok, sizeof(ok)));
	zassert_equal(test_hooked, ok);
	zassert_equal(sdo_download(0x200A, 0U, &bad, sizeof(bad)),
		      CANOPEN_SDO_ABORT_VALUE_TOO_HIGH);
	zassert_equal(test_hooked, ok);
}

ZTEST(canopen_node, test_sdo_protocol_errors)
{
	uint8_t req[8];
	uint8_t resp[8];

	sdo_init_req(req, 0xE0U, 0x2000, 0U);
	sdo_request(req, resp);
	zassert_equal(sdo_abort_code(resp), CANOPEN_SDO_ABORT_COMMAND);

	sdo_init_req(req, 0x00U, 0x0000, 0U);
	sdo_request(req, resp);
	zassert_equal(sdo_abort_code(resp), CANOPEN_SDO_ABORT_COMMAND);

	sdo_init_req(req, 0x21U, 0x2004, 0U);
	sys_put_le32(8U, &req[4]);
	sdo_request(req, resp);
	zassert_equal(resp[0], 0x60U);
	memset(req, 0, sizeof(req));
	req[0] = 0x10U;
	sdo_request(req, resp);
	zassert_equal(sdo_abort_code(resp), CANOPEN_SDO_ABORT_TOGGLE);
}

ZTEST(canopen_node, test_sdo_timeout)
{
	struct can_frame frame;
	uint8_t req[8];
	uint8_t resp[8];

	sdo_init_req(req, 0x21U, 0x2004, 0U);
	sys_put_le32(8U, &req[4]);
	sdo_request(req, resp);
	zassert_equal(resp[0], 0x60U);

	zassert_ok(k_msgq_get(&sdo_msgq, &frame,
			      K_MSEC(CONFIG_CANOPEN_SDO_SERVER_TIMEOUT_MS * 2)));
	zassert_equal(sdo_abort_code(frame.data), CANOPEN_SDO_ABORT_TIMEOUT);
	zassert_equal(sys_get_le16(&frame.data[1]), 0x2004);
}

ZTEST(canopen_node, test_sdo_abort_by_client)
{
	struct can_frame frame;
	uint8_t req[8];
	uint8_t resp[8];

	sdo_init_req(req, 0x21U, 0x2004, 0U);
	sys_put_le32(8U, &req[4]);
	sdo_request(req, resp);
	zassert_equal(resp[0], 0x60U);

	sdo_init_req(req, 0x80U, 0x2004, 0U);
	send_frame(TEST_COB_SDO_RX, req, sizeof(req));
	zassert_equal(k_msgq_get(&sdo_msgq, &frame,
				 K_MSEC(CONFIG_CANOPEN_SDO_SERVER_TIMEOUT_MS * 2)),
		      -EAGAIN);
}

ZTEST(canopen_node, test_nmt_states)
{
	struct can_frame frame;
	uint8_t req[8];

	send_nmt(0x01U, TEST_NODE_ID);
	expect_state(CANOPEN_NMT_OPERATIONAL);

	send_nmt(0x02U, 0U);
	expect_state(CANOPEN_NMT_STOPPED);

	sdo_init_req(req, 0x40U, 0x1000, 0U);
	send_frame(TEST_COB_SDO_RX, req, sizeof(req));
	zassert_equal(k_msgq_get(&sdo_msgq, &frame, TEST_TIMEOUT), -EAGAIN);

	send_nmt(0x80U, TEST_NODE_ID);
	expect_state(CANOPEN_NMT_PRE_OPERATIONAL);

	send_nmt(0x01U, TEST_NODE_ID + 1U);
	zassert_equal(k_sem_take(&nmt_sem, TEST_TIMEOUT), -EAGAIN);
	zassert_equal(canopen_nmt_get_state(&test_node), CANOPEN_NMT_PRE_OPERATIONAL);

	send_frame(0x000U, (const uint8_t[]){0x01U}, 1U);
	zassert_equal(k_sem_take(&nmt_sem, TEST_TIMEOUT), -EAGAIN);
}

ZTEST(canopen_node, test_nmt_reset)
{
	const uint8_t u8 = 0x5AU;
	const uint16_t hb = 50U;
	atomic_val_t count = atomic_get(&reset_app_count);
	uint16_t value;

	zassert_ok(sdo_download(0x2000, 0U, &u8, sizeof(u8)));
	zassert_ok(canopen_od_write(&test_node, 0x1017, 0U, &hb, sizeof(hb)));
	expect_err_frame(CANOPEN_NMT_PRE_OPERATIONAL, K_MSEC(hb * 2));
	k_msgq_purge(&err_msgq);

	reset_communication();
	zassert_ok(canopen_od_read(&test_node, 0x1017, 0U, &value, sizeof(value)));
	zassert_equal(value, CONFIG_CANOPEN_HEARTBEAT_PRODUCER_MS);
	zassert_equal(test_u8, u8);
	zassert_equal(atomic_get(&reset_app_count), count);

	send_nmt(0x81U, 0U);
	expect_bootup();
	expect_state(CANOPEN_NMT_PRE_OPERATIONAL);
	zassert_equal(test_u8, 0U);
	zassert_equal(atomic_get(&reset_app_count), count + 1);
}

ZTEST(canopen_node, test_heartbeat_producer)
{
	const uint8_t hb[] = {20U, 0U};
	const uint16_t off = 0U;
	struct can_frame frame;
	int64_t start;

	zassert_ok(sdo_download(0x1017, 0U, hb, sizeof(hb)));
	expect_err_frame(CANOPEN_NMT_PRE_OPERATIONAL, K_MSEC(40));
	start = k_uptime_get();
	expect_err_frame(CANOPEN_NMT_PRE_OPERATIONAL, K_MSEC(40));
	zassert_within(k_uptime_delta(&start), 20, 2);

	send_nmt(0x01U, TEST_NODE_ID);
	expect_state(CANOPEN_NMT_OPERATIONAL);
	expect_err_frame(CANOPEN_NMT_OPERATIONAL, K_MSEC(40));

	zassert_ok(canopen_od_write(&test_node, 0x1017, 0U, &off, sizeof(off)));
	k_msgq_purge(&err_msgq);
	zassert_equal(k_msgq_get(&err_msgq, &frame, K_MSEC(60)), -EAGAIN);
}

ZTEST(canopen_node, test_local_api)
{
	uint8_t u8 = 0x42U;
	uint32_t value;

	zassert_ok(canopen_od_write(&test_node, 0x2005, 0U, &(uint32_t){0xDEADBEEFU}, 4U));
	zassert_ok(sdo_upload_u32(0x2005, 0U, &value));
	zassert_equal(value, 0xDEADBEEFU);

	zassert_ok(canopen_od_write(&test_node, 0x2000, 0U, &u8, sizeof(u8)));
	u8 = 0U;
	zassert_ok(canopen_od_read(&test_node, 0x2000, 0U, &u8, sizeof(u8)));
	zassert_equal(u8, 0x42U);

	zassert_equal(canopen_od_read(&test_node, 0x2000, 0U, &value, sizeof(value)), -EINVAL);
	zassert_equal(canopen_od_read(&test_node, 0x2FFF, 0U, &u8, sizeof(u8)), -ENOENT);
	zassert_equal(canopen_od_write(&test_node, 0x1000, 0U, &value, sizeof(value)), -EACCES);

	zassert_ok(canopen_nmt_local_cmd(&test_node, CANOPEN_NMT_CMD_START));
	expect_state(CANOPEN_NMT_OPERATIONAL);
	zassert_equal(canopen_nmt_local_cmd(&test_node, (enum canopen_nmt_cmd)0x55), -EINVAL);
}

ZTEST_SUITE(canopen_node, NULL, canopen_setup, canopen_before, NULL, NULL);
