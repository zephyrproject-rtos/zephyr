/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/canbus/canopen.h>
#include <zephyr/drivers/can.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/ztest.h>

#define TEST_NODE_ID    0x21U
#define TEST_COB_SDO_RX (0x600U + TEST_NODE_ID)
#define TEST_COB_SDO_TX (0x580U + TEST_NODE_ID)
#define TEST_TIMEOUT    K_MSEC(100)
#define TEST_BLKSIZE    CONFIG_CANOPEN_SDO_BLOCK_SIZE
#define DOMAIN_MAX      300U
#define BLOB_LEN        40U

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

CAN_MSGQ_DEFINE(sdo_msgq, 32);

struct test_domain {
	uint8_t data[DOMAIN_MAX];
	size_t len;
	size_t written;
	unsigned int reads;
	bool last;
};

static struct test_domain dom;
static uint8_t blob[BLOB_LEN];
static uint32_t test_u32;

static uint32_t domain_read(const struct canopen_od_entry *entry, size_t offset, uint8_t *buf,
			    size_t *len, void *user_data)
{
	struct test_domain *d = user_data;

	ARG_UNUSED(entry);

	d->reads++;
	*len = MIN(*len, d->len - MIN(offset, d->len));
	memcpy(buf, &d->data[offset], *len);

	return CANOPEN_SDO_ABORT_NONE;
}

static uint32_t domain_write(const struct canopen_od_entry *entry, size_t offset,
			     const uint8_t *buf, size_t len, size_t size, bool last,
			     void *user_data)
{
	struct test_domain *d = user_data;

	ARG_UNUSED(entry);
	ARG_UNUSED(size);

	if ((offset + len) > DOMAIN_MAX) {
		return CANOPEN_SDO_ABORT_OUT_OF_MEMORY;
	}

	zassert_equal(offset, d->written, "data not in order");
	memcpy(&d->data[offset], buf, len);
	d->written = offset + len;
	d->last = last;

	return CANOPEN_SDO_ABORT_NONE;
}

static const struct canopen_od_domain test_domain_ops = {
	.read = domain_read,
	.write = domain_write,
};

CANOPEN_OD_DEFINE(test_od,
	CANOPEN_OD_VAR(0x2000, CANOPEN_OD_TYPE_OCTET_STRING, CANOPEN_OD_RW, blob),
	CANOPEN_OD_VAR(0x2001, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_RW, test_u32),
	CANOPEN_OD_DOMAIN(0x5600, CANOPEN_OD_RW, test_domain_ops, &dom));

static const struct canopen_node_config test_cfg = {
	.can = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
	.od = &test_od,
	.node_id = TEST_NODE_ID,
};

static struct canopen_node test_node;

static void send_sdo(const uint8_t *data)
{
	struct can_frame frame = {
		.id = TEST_COB_SDO_RX,
		.dlc = 8U,
	};

	memcpy(frame.data, data, 8U);
	zassert_ok(can_send(can_dev, &frame, TEST_TIMEOUT, NULL, NULL));
}

static void recv_sdo(uint8_t *resp)
{
	struct can_frame frame;

	zassert_ok(k_msgq_get(&sdo_msgq, &frame, TEST_TIMEOUT), "no SDO response");
	zassert_equal(frame.dlc, 8U);
	memcpy(resp, frame.data, 8U);
}

static void request(const uint8_t *req, uint8_t *resp)
{
	send_sdo(req);
	recv_sdo(resp);
}

static void expect_abort(const uint8_t *resp, uint32_t code)
{
	zassert_equal(resp[0], 0x80U, "no abort, command 0x%02x", resp[0]);
	zassert_equal(sys_get_le32(&resp[4]), code, "abort 0x%08x", sys_get_le32(&resp[4]));
}

static void init_req(uint8_t *req, uint8_t cmd, uint16_t index)
{
	memset(req, 0, 8U);
	req[0] = cmd;
	sys_put_le16(index, &req[1]);
}

static void fill(uint8_t *data, size_t len, uint8_t seed)
{
	for (size_t i = 0U; i < len; i++) {
		data[i] = (uint8_t)(seed + i * 13U);
	}
}

static void download_initiate(uint16_t index, uint8_t cmd, uint32_t size)
{
	uint8_t req[8];
	uint8_t resp[8];

	init_req(req, cmd, index);
	sys_put_le32(size, &req[4]);
	request(req, resp);
	zassert_equal(resp[0], 0xA4U, "initiate response 0x%02x", resp[0]);
	zassert_equal(sys_get_le16(&resp[1]), index);
	zassert_equal(resp[4], TEST_BLKSIZE);
}

static uint8_t download_blocks(const uint8_t *data, size_t len, uint8_t lose)
{
	uint8_t blksize = TEST_BLKSIZE;
	size_t start = 0U;
	uint8_t seqno = 0U;
	bool lost = false;

	while (true) {
		size_t pos = start + (size_t)seqno * 7U;
		size_t chunk = MIN(len - pos, 7U);
		bool last = (pos + chunk) == len;
		uint8_t req[8] = {0};
		uint8_t resp[8];

		seqno++;
		req[0] = seqno | (last ? 0x80U : 0U);
		memcpy(&req[1], &data[pos], chunk);
		if ((seqno == lose) && !lost) {
			lost = true;
		} else {
			send_sdo(req);
		}

		if (!last && (seqno < blksize)) {
			continue;
		}

		recv_sdo(resp);
		zassert_equal(resp[0], 0xA2U, "block response 0x%02x", resp[0]);
		zassert_true(resp[1] <= seqno);
		if ((resp[1] == seqno) && last) {
			return (uint8_t)(7U - chunk);
		}

		start += (size_t)resp[1] * 7U;
		blksize = resp[2];
		seqno = 0U;
	}
}

static void download_end(uint8_t n, uint16_t crc, uint8_t *resp)
{
	uint8_t req[8] = {0xC1U | (n << 2)};

	sys_put_le16(crc, &req[1]);
	request(req, resp);
}

static void download(uint16_t index, const uint8_t *data, size_t len, bool size, uint8_t lose)
{
	uint8_t resp[8];
	uint8_t n;

	download_initiate(index, size ? 0xC6U : 0xC4U, size ? len : 0U);
	n = download_blocks(data, len, lose);
	download_end(n, crc16_itu_t(0U, data, len), resp);
	zassert_equal(resp[0], 0xA1U, "end response 0x%02x", resp[0]);
}

static size_t upload(uint16_t index, uint8_t blksize, uint8_t lose, uint8_t *out, uint8_t *cmd)
{
	uint8_t req[8];
	uint8_t resp[8];
	size_t start = 0U;
	size_t len;
	bool lost = false;

	init_req(req, 0xA4U, index);
	req[4] = blksize;
	request(req, resp);
	*cmd = resp[0];
	zassert_equal(resp[0] & 0xE1U, 0xC0U, "initiate response 0x%02x", resp[0]);
	zassert_equal(sys_get_le16(&resp[1]), index);

	init_req(req, 0xA3U, 0U);
	send_sdo(req);

	while (true) {
		uint8_t seqno = 0U;
		bool last = false;

		while (!last && (seqno < blksize)) {
			recv_sdo(resp);
			seqno++;
			zassert_equal(resp[0] & 0x7FU, seqno, "segment 0x%02x", resp[0]);
			last = (resp[0] & 0x80U) != 0U;
			memcpy(&out[start + (seqno - 1U) * 7U], &resp[1], 7U);
		}

		init_req(req, 0xA2U, 0U);
		req[1] = seqno;
		req[2] = blksize;
		if ((lose != 0U) && !lost && (lose <= seqno)) {
			lost = true;
			req[1] = lose - 1U;
			last = false;
		}

		start += (size_t)req[1] * 7U;
		send_sdo(req);
		if (last) {
			break;
		}
	}

	recv_sdo(resp);
	zassert_equal(resp[0] & 0xE3U, 0xC1U, "end 0x%02x", resp[0]);
	len = start - ((resp[0] >> 2) & 0x7U);
	zassert_equal(sys_get_le16(&resp[1]), crc16_itu_t(0U, out, len), "CRC");

	init_req(req, 0xA1U, 0U);
	send_sdo(req);

	return len;
}

static void *block_setup(void)
{
	struct can_filter filter = {
		.id = TEST_COB_SDO_TX,
		.mask = CAN_STD_ID_MASK,
	};

	zassert_true(device_is_ready(can_dev));
	zassert_ok(can_set_mode(can_dev, CAN_MODE_LOOPBACK));
	zassert_true(can_add_rx_filter_msgq(can_dev, &sdo_msgq, &filter) >= 0);
	zassert_ok(canopen_node_init(&test_node, &test_cfg));
	zassert_ok(canopen_node_start(&test_node));

	return NULL;
}

static void block_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_msleep(10);
	k_msgq_purge(&sdo_msgq);
	memset(&dom, 0, sizeof(dom));
	memset(blob, 0, sizeof(blob));
}

ZTEST(canopen_block, test_crc_check_value)
{
	static const uint8_t check[] = "123456789";

	zassert_equal(crc16_itu_t(0U, check, sizeof(check) - 1U), 0x31C3U);
}

ZTEST(canopen_block, test_download_object)
{
	uint8_t data[BLOB_LEN];

	fill(data, sizeof(data), 3U);
	download(0x2000, data, sizeof(data), true, 0U);
	zassert_mem_equal(blob, data, sizeof(data));
}

ZTEST(canopen_block, test_download_domain)
{
	uint8_t data[200];

	fill(data, sizeof(data), 7U);
	download(0x5600, data, 100U, false, 0U);
	zassert_equal(dom.written, 100U);
	zassert_true(dom.last);
	zassert_mem_equal(dom.data, data, 100U);

	memset(&dom, 0, sizeof(dom));
	download(0x5600, data, sizeof(data), true, 0U);
	zassert_equal(dom.written, sizeof(data));
	zassert_mem_equal(dom.data, data, sizeof(data));

	memset(&dom, 0, sizeof(dom));
	download(0x5600, data, 21U, true, 0U);
	zassert_equal(dom.written, 21U);
	zassert_true(dom.last);
}

ZTEST(canopen_block, test_download_lost_segment)
{
	uint8_t data[150];

	fill(data, sizeof(data), 11U);
	download(0x5600, data, sizeof(data), true, 3U);
	zassert_equal(dom.written, sizeof(data));
	zassert_mem_equal(dom.data, data, sizeof(data));

	memset(&dom, 0, sizeof(dom));
	download(0x5600, data, 20U, true, 1U);
	zassert_equal(dom.written, 20U);
	zassert_mem_equal(dom.data, data, 20U);
}

ZTEST(canopen_block, test_download_errors)
{
	uint8_t data[BLOB_LEN];
	uint8_t resp[8];
	uint8_t req[8];
	uint8_t n;

	fill(data, sizeof(data), 5U);

	download_initiate(0x2000, 0xC6U, sizeof(data));
	n = download_blocks(data, sizeof(data), 0U);
	download_end(n, crc16_itu_t(0U, data, sizeof(data)) ^ 1U, resp);
	expect_abort(resp, CANOPEN_SDO_ABORT_CRC);
	zassert_equal(blob[0], 0U, "object written despite CRC error");

	download_initiate(0x2000, 0xC6U, sizeof(data));
	n = download_blocks(data, 33U, 0U);
	download_end(n, crc16_itu_t(0U, data, 33U), resp);
	expect_abort(resp, CANOPEN_SDO_ABORT_LENGTH_TOO_LOW);

	init_req(req, 0xC6U, 0x2000);
	sys_put_le32(sizeof(blob) + 1U, &req[4]);
	request(req, resp);
	expect_abort(resp, CANOPEN_SDO_ABORT_LENGTH_TOO_HIGH);

	download_initiate(0x2000, 0xC6U, sizeof(data));
	init_req(req, (uint8_t)(TEST_BLKSIZE + 1U), 0U);
	request(req, resp);
	expect_abort(resp, CANOPEN_SDO_ABORT_SEQUENCE);

	download_initiate(0x5600, 0xC4U, 0U);
	init_req(req, 0x80U, 0x5600);
	send_sdo(req);
	init_req(req, 0x40U, 0x2001);
	request(req, resp);
	zassert_equal(resp[0], 0x43U, "no upload after client abort, 0x%02x", resp[0]);
}

ZTEST(canopen_block, test_download_no_crc)
{
	uint8_t data[BLOB_LEN];
	uint8_t resp[8];
	uint8_t req[8];
	uint8_t n;

	fill(data, sizeof(data), 9U);
	init_req(req, 0xC2U, 0x2000);
	sys_put_le32(sizeof(data), &req[4]);
	request(req, resp);
	zassert_equal(resp[0], 0xA4U);
	n = download_blocks(data, sizeof(data), 0U);
	download_end(n, 0U, resp);
	zassert_equal(resp[0], 0xA1U);
	zassert_mem_equal(blob, data, sizeof(data));
}

ZTEST(canopen_block, test_upload_object)
{
	uint8_t out[BLOB_LEN + 8U];
	uint8_t cmd;

	fill(blob, sizeof(blob), 17U);
	zassert_equal(upload(0x2000, 4U, 0U, out, &cmd), sizeof(blob));
	zassert_equal(cmd, 0xC6U, "initiate response 0x%02x", cmd);
	zassert_mem_equal(out, blob, sizeof(blob));

	zassert_equal(upload(0x2000, 127U, 0U, out, &cmd), sizeof(blob));
	zassert_mem_equal(out, blob, sizeof(blob));
}

ZTEST(canopen_block, test_upload_retransmit)
{
	uint8_t out[DOMAIN_MAX + 8U];
	uint8_t cmd;

	fill(blob, sizeof(blob), 23U);
	zassert_equal(upload(0x2000, 4U, 3U, out, &cmd), sizeof(blob));
	zassert_mem_equal(out, blob, sizeof(blob));

	fill(dom.data, DOMAIN_MAX, 29U);
	dom.len = 100U;
	zassert_equal(upload(0x5600, 8U, 1U, out, &cmd), 100U);
	zassert_mem_equal(out, dom.data, 100U);
}

ZTEST(canopen_block, test_upload_domain)
{
	uint8_t out[DOMAIN_MAX + 8U];
	uint8_t cmd;

	fill(dom.data, DOMAIN_MAX, 31U);
	dom.len = 20U;
	zassert_equal(upload(0x5600, 127U, 0U, out, &cmd), 20U);
	zassert_equal(cmd, 0xC4U, "initiate response 0x%02x", cmd);
	zassert_mem_equal(out, dom.data, 20U);

	dom.len = 21U;
	zassert_equal(upload(0x5600, 2U, 0U, out, &cmd), 21U);
	zassert_mem_equal(out, dom.data, 21U);

	dom.len = 0U;
	zassert_equal(upload(0x5600, 5U, 0U, out, &cmd), 0U);

	dom.len = DOMAIN_MAX;
	zassert_equal(upload(0x5600, 30U, 0U, out, &cmd), DOMAIN_MAX);
	zassert_mem_equal(out, dom.data, DOMAIN_MAX);
}

ZTEST(canopen_block, test_upload_small)
{
	uint8_t out[16];
	uint8_t resp[8];
	uint8_t req[8];
	uint8_t cmd;

	test_u32 = 0xA1B2C3D4U;
	init_req(req, 0xA4U, 0x2001);
	req[4] = 10U;
	req[5] = 255U;
	request(req, resp);
	zassert_equal(resp[0], 0xC6U, "no block upload, 0x%02x", resp[0]);
	zassert_equal(sys_get_le32(&resp[4]), 4U);
	init_req(req, 0x80U, 0x2001);
	send_sdo(req);

	zassert_equal(upload(0x2001, 1U, 0U, out, &cmd), 4U);
	zassert_equal(sys_get_le32(out), test_u32);
}

ZTEST(canopen_block, test_upload_errors)
{
	uint8_t resp[8];
	uint8_t req[8];

	init_req(req, 0xA4U, 0x2000);
	request(req, resp);
	expect_abort(resp, CANOPEN_SDO_ABORT_BLOCK_SIZE);

	req[4] = 128U;
	request(req, resp);
	expect_abort(resp, CANOPEN_SDO_ABORT_BLOCK_SIZE);

	req[4] = 2U;
	request(req, resp);
	zassert_equal(resp[0], 0xC6U);
	init_req(req, 0xA3U, 0U);
	send_sdo(req);
	recv_sdo(resp);
	recv_sdo(resp);
	init_req(req, 0xA2U, 0U);
	req[1] = 3U;
	req[2] = 2U;
	request(req, resp);
	expect_abort(resp, CANOPEN_SDO_ABORT_SEQUENCE);

	init_req(req, 0xA4U, 0x2000);
	req[4] = 2U;
	request(req, resp);
	init_req(req, 0xA3U, 0U);
	send_sdo(req);
	recv_sdo(resp);
	recv_sdo(resp);
	init_req(req, 0xA2U, 0U);
	req[1] = 2U;
	req[2] = 0U;
	request(req, resp);
	expect_abort(resp, CANOPEN_SDO_ABORT_BLOCK_SIZE);

	init_req(req, 0xA3U, 0U);
	request(req, resp);
	expect_abort(resp, CANOPEN_SDO_ABORT_COMMAND);
}

ZTEST_SUITE(canopen_block, NULL, block_setup, block_before, NULL, NULL);
