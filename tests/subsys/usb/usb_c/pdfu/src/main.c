/*
 * Copyright (c) 2026 Meta Platforms, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * The PDFU Responder driven request by request, and the Initiator pushing
 * images into it. Messages go through a fake usbc_send_ext_msg(), which
 * records Responses and, for the Initiator tests, delivers each message to
 * the other role from a work item, as the USB-C stack thread would.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb_c/pdfu.h>
#include <zephyr/usb_c/pdfu_initiator.h>
#include <zephyr/usb_c/usbc.h>
#include <zephyr/ztest.h>

#include "pdfu_proto.h"

#define MAX_MSG_LEN (2 + 2 + PDFU_DATA_BLOCK_LEN)

static const struct device responder_port = {.name = "responder"};
static const struct device initiator_port = {.name = "initiator"};

static uint8_t image[1024];
static uint32_t image_len;

/* Transport */

struct msg {
	enum pd_ext_msg_type type;
	uint32_t len;
	uint8_t data[MAX_MSG_LEN];
};

K_MSGQ_DEFINE(wire, sizeof(struct msg), 8, 4);

static bool loopback;
/* Lose the next PDFU_DATA Request, as a dropped chunked message would. */
static bool drop_next_data;
static uint8_t rsp[MAX_MSG_LEN];
static uint32_t rsp_len;
static int rsp_count;

static void wire_work_cb(struct k_work *work)
{
	static struct msg m;

	ARG_UNUSED(work);

	while (k_msgq_get(&wire, &m, K_NO_WAIT) == 0) {
		if (m.type == PD_EXT_FIRMWARE_UPDATE_REQUEST) {
			pdfu_responder_msg(&responder_port, m.type, m.data, m.len);
		} else {
			pdfu_initiator_msg(&initiator_port, m.type, m.data, m.len);
		}
	}
}

static K_WORK_DEFINE(wire_work, wire_work_cb);

int usbc_send_ext_msg(const struct device *dev, const enum pd_ext_msg_type type,
		      const uint8_t *data, const uint32_t len)
{
	static struct msg m;

	zassert_true(len <= MAX_MSG_LEN, "message of %u bytes", len);

	if (type == PD_EXT_FIRMWARE_UPDATE_RESPONSE) {
		zassert_equal_ptr(dev, &responder_port);
		memcpy(rsp, data, len);
		rsp_len = len;
		rsp_count++;
	} else {
		zassert_equal_ptr(dev, &initiator_port);
	}

	if (!loopback) {
		return 0;
	}

	if (drop_next_data && type == PD_EXT_FIRMWARE_UPDATE_REQUEST && data[1] == PDFU_REQ_DATA) {
		drop_next_data = false;
		return 0;
	}

	m.type = type;
	m.len = len;
	memcpy(m.data, data, len);
	zassert_ok(k_msgq_put(&wire, &m, K_NO_WAIT));
	k_work_submit(&wire_work);

	return 0;
}

/* Responder backend, into RAM */

static uint8_t store[sizeof(image)];
static uint32_t max_size;
static int begin_status;
static int writes;
static uint32_t validated_len;
static bool aborted;
static bool manifested;

static void get_fw_id(struct pdfu_fw_id *id)
{
	*id = (struct pdfu_fw_id){
		.vid = 0x1234,
		.pid = 0x5678,
		.hw_version = 0x21,
		.si_version = 0x30,
		.fw_version = {1, 2, 3, 4},
		.image_bank = 1,
		.flags = {0x01, 0x03, 0x00, 0x08},
	};
}

static int begin(const uint16_t fw_version[4], uint32_t *max_image_size)
{
	ARG_UNUSED(fw_version);

	*max_image_size = max_size;
	return begin_status;
}

static int write(uint32_t offset, const uint8_t *data, uint32_t len)
{
	zassert_true(offset + len <= sizeof(store));
	memcpy(&store[offset], data, len);
	writes++;
	return 0;
}

static int validate(uint32_t total_len)
{
	validated_len = total_len;
	return (total_len == image_len && memcmp(store, image, total_len) == 0) ? 0 : PDFU_ERR_FILE;
}

static void abort_update(void)
{
	aborted = true;
}

static void manifest(void)
{
	manifested = true;
}

static const struct pdfu_ops ops = {
	.get_fw_id = get_fw_id,
	.begin = begin,
	.write = write,
	.validate = validate,
	.abort = abort_update,
	.manifest = manifest,
};

/* Initiator image source */

static int src_open(uint32_t *size, uint16_t version[4])
{
	*size = image_len;
	version[0] = 2;
	version[1] = 0;
	version[2] = 0;
	version[3] = 0;
	return 0;
}

static int src_read(uint32_t offset, uint8_t *buf, uint32_t len)
{
	memcpy(buf, &image[offset], len);
	return 0;
}

static const struct pdfu_image_src src = {
	.open = src_open,
	.read = src_read,
};

static void reset(void *fixture)
{
	ARG_UNUSED(fixture);

	k_msgq_purge(&wire);
	loopback = false;
	drop_next_data = false;
	rsp_len = 0;
	rsp_count = 0;
	memset(store, 0, sizeof(store));
	max_size = sizeof(store);
	begin_status = 0;
	writes = 0;
	validated_len = 0;
	aborted = false;
	manifested = false;
	for (int i = 0; i < sizeof(image); i++) {
		image[i] = (uint8_t)(i * 7 + 3);
	}
	image_len = 612;

	zassert_ok(pdfu_responder_init(&responder_port, &ops));
	zassert_ok(pdfu_initiator_init(&initiator_port, &src));
}

/* Requests to the responder, and its answers */

static void request(uint8_t type, const uint8_t *payload, uint32_t len)
{
	uint8_t buf[MAX_MSG_LEN] = {PDFU_PROTOCOL_V1_0, type};

	memcpy(&buf[2], payload, len);
	zassert_true(
		pdfu_responder_msg(&responder_port, PD_EXT_FIRMWARE_UPDATE_REQUEST, buf, len + 2));
}

static void initiate(void)
{
	uint8_t p[8] = {2, 0, 0, 0, 0, 0, 0, 0};

	request(PDFU_REQ_INITIATE, p, sizeof(p));
	zassert_equal(rsp[1], PDFU_RSP_INITIATE);
	zassert_equal(rsp[2], PDFU_OK);
}

static void data_block(uint8_t type, uint16_t index, uint32_t len)
{
	uint8_t p[2 + PDFU_DATA_BLOCK_LEN];

	sys_put_le16(index, p);
	memcpy(&p[2], &image[index * PDFU_DATA_BLOCK_LEN], len);
	request(type, p, 2 + len);
}

/* Checks the last Response was a PDFU_DATA one with @p status. */
static void expect_data_rsp(uint8_t status, uint16_t next)
{
	zassert_equal(rsp_len, 7);
	zassert_equal(rsp[1], PDFU_RSP_DATA);
	zassert_equal(rsp[2], status, "status %02x", rsp[2]);
	if (status == PDFU_OK) {
		zassert_equal(sys_get_le16(&rsp[5]), next);
	} else {
		zassert_equal(rsp[3], PDFU_WAIT_STOP);
	}
}

ZTEST(pdfu_responder, test_get_fw_id)
{
	static const uint8_t flags[4] = {0x01, 0x03, 0x00, 0x08};

	request(PDFU_REQ_GET_FW_ID, NULL, 0);

	zassert_equal(rsp_len, 22);
	zassert_equal(rsp[0], PDFU_PROTOCOL_V1_0);
	zassert_equal(rsp[1], PDFU_RSP_GET_FW_ID);
	zassert_equal(rsp[2], PDFU_OK);
	zassert_equal(sys_get_le16(&rsp[3]), 0x1234);
	zassert_equal(sys_get_le16(&rsp[5]), 0x5678);
	zassert_equal(rsp[7], 0x21);
	zassert_equal(rsp[8], 0x30);
	for (int i = 0; i < 4; i++) {
		zassert_equal(sys_get_le16(&rsp[9 + i * 2]), i + 1);
	}
	zassert_equal(rsp[17], 1);
	zassert_mem_equal(&rsp[18], flags, sizeof(flags));
}

ZTEST(pdfu_responder, test_update)
{
	initiate();
	zassert_equal(pdfu_responder_phase(), PDFU_PHASE_RECONFIGURATION);
	zassert_equal(rsp[3], 0, "WaitTime");
	zassert_equal(rsp[4] | (rsp[5] << 8) | (rsp[6] << 16), sizeof(store));

	data_block(PDFU_REQ_DATA, 0, 256);
	expect_data_rsp(PDFU_OK, 1);
	zassert_equal(pdfu_responder_phase(), PDFU_PHASE_TRANSFER);
	data_block(PDFU_REQ_DATA, 1, 256);
	expect_data_rsp(PDFU_OK, 2);
	data_block(PDFU_REQ_DATA, 2, 100);
	expect_data_rsp(PDFU_OK, 3);
	zassert_equal(pdfu_responder_phase(), PDFU_PHASE_VALIDATION);
	zassert_equal(pdfu_responder_bytes(), 612);

	request(PDFU_REQ_VALIDATE, NULL, 0);
	zassert_equal(rsp[1], PDFU_RSP_VALIDATE);
	zassert_equal(rsp[2], PDFU_OK);
	zassert_equal(validated_len, 612);
	zassert_equal(pdfu_responder_phase(), PDFU_PHASE_MANIFESTATION);

	/* Manifesting waits for the Response to go out. */
	zassert_false(manifested);
	k_sleep(K_MSEC(200));
	zassert_true(manifested);
	zassert_mem_equal(store, image, 612);
}

ZTEST(pdfu_responder, test_whole_blocks_then_validate)
{
	image_len = 512;
	initiate();
	data_block(PDFU_REQ_DATA, 0, 256);
	data_block(PDFU_REQ_DATA, 1, 256);
	expect_data_rsp(PDFU_OK, 2);
	zassert_equal(pdfu_responder_phase(), PDFU_PHASE_TRANSFER);

	/* No short block: VALIDATE ends the transfer. */
	request(PDFU_REQ_VALIDATE, NULL, 0);
	zassert_equal(rsp[2], PDFU_OK);
	zassert_equal(validated_len, 512);
}

ZTEST(pdfu_responder, test_repeated_block_acked_once)
{
	initiate();
	data_block(PDFU_REQ_DATA, 0, 256);
	data_block(PDFU_REQ_DATA, 0, 256);

	expect_data_rsp(PDFU_OK, 1);
	zassert_equal(writes, 1);
	zassert_equal(pdfu_responder_bytes(), 256);
}

ZTEST(pdfu_responder, test_block_out_of_order)
{
	initiate();
	data_block(PDFU_REQ_DATA, 1, 256);

	expect_data_rsp(PDFU_ERR_ADDRESS, 0);
	zassert_equal(writes, 0);
}

ZTEST(pdfu_responder, test_overrun)
{
	max_size = 300;
	initiate();
	data_block(PDFU_REQ_DATA, 0, 256);
	expect_data_rsp(PDFU_OK, 1);
	data_block(PDFU_REQ_DATA, 1, 256);

	expect_data_rsp(PDFU_ERR_ADDRESS, 0);
	zassert_equal(pdfu_responder_bytes(), 256);
}

/* Refused out of phase: the Response carries the status alone. */
static void expect_unexpected(uint8_t rsp_type)
{
	zassert_equal(rsp_len, 3);
	zassert_equal(rsp[1], rsp_type);
	zassert_equal(rsp[2], PDFU_ERR_UNEXPECTED_REQUEST);
}

ZTEST(pdfu_responder, test_unexpected_requests)
{
	data_block(PDFU_REQ_DATA, 0, 256);
	expect_unexpected(PDFU_RSP_DATA);
	zassert_equal(pdfu_responder_phase(), PDFU_PHASE_ENUMERATION);

	initiate();
	data_block(PDFU_REQ_DATA, 0, 256);
	request(PDFU_REQ_GET_FW_ID, NULL, 0);
	expect_unexpected(PDFU_RSP_GET_FW_ID);
	zassert_equal(pdfu_responder_phase(), PDFU_PHASE_TRANSFER);
}

ZTEST(pdfu_responder, test_abort)
{
	int count;

	initiate();
	data_block(PDFU_REQ_DATA, 0, 256);
	count = rsp_count;

	request(PDFU_REQ_ABORT, NULL, 0);

	zassert_equal(rsp_count, count, "PDFU_ABORT is never answered");
	zassert_true(aborted);
	zassert_equal(pdfu_responder_phase(), PDFU_PHASE_ENUMERATION);
	zassert_equal(pdfu_responder_bytes(), 0);
}

ZTEST(pdfu_responder, test_data_nr_unanswered)
{
	int count;

	initiate();
	data_block(PDFU_REQ_DATA, 0, 256);
	count = rsp_count;

	data_block(PDFU_REQ_DATA_NR, 1, 256);

	zassert_equal(rsp_count, count);
	zassert_equal(pdfu_responder_bytes(), 512);
}

ZTEST(pdfu_responder, test_validation_fails)
{
	initiate();
	data_block(PDFU_REQ_DATA, 0, 256);
	data_block(PDFU_REQ_DATA, 1, 256);
	data_block(PDFU_REQ_DATA, 2, 100);
	/* What arrived no longer matches what the backend expects. */
	image[600] ^= 0xff;

	request(PDFU_REQ_VALIDATE, NULL, 0);

	zassert_equal(rsp[1], PDFU_RSP_VALIDATE);
	zassert_equal(rsp[2], PDFU_ERR_FILE);
	zassert_true(aborted);
	zassert_equal(pdfu_responder_phase(), PDFU_PHASE_ENUMERATION);
	k_sleep(K_MSEC(200));
	zassert_false(manifested);
}

ZTEST(pdfu_responder, test_initiate_refused)
{
	uint8_t p[8] = {0};

	begin_status = PDFU_ERR_TARGET;
	request(PDFU_REQ_INITIATE, p, sizeof(p));

	zassert_equal(rsp[2], PDFU_ERR_TARGET);
	zassert_equal(rsp[3], PDFU_WAIT_STOP);
	zassert_equal(pdfu_responder_phase(), PDFU_PHASE_ENUMERATION);
}

ZTEST_SUITE(pdfu_responder, NULL, NULL, reset, NULL, NULL);

/* The initiator, pushing into the responder */

static struct pdfu_initiator_status push(const struct pdfu_initiator_faults *faults)
{
	struct pdfu_initiator_status st;

	loopback = true;
	zassert_ok(pdfu_initiator_start(faults));

	for (int i = 0; i < 400; i++) {
		pdfu_initiator_status(&st);
		if (st.state > PDFU_INIT_VALIDATE) {
			return st;
		}
		k_sleep(K_MSEC(10));
	}

	zassert_unreachable("still in %s after 4 s", pdfu_initiator_state_name(st.state));
	return st;
}

ZTEST(pdfu_initiator, test_push)
{
	struct pdfu_initiator_status st = push(NULL);

	zassert_equal(st.state, PDFU_INIT_DONE, "%s: %s", pdfu_initiator_state_name(st.state),
		      st.error);
	zassert_equal(st.bytes_sent, 612);
	zassert_equal(st.responder_version[3], 4);
	zassert_mem_equal(store, image, 612);
	k_sleep(K_MSEC(200));
	zassert_true(manifested);
}

ZTEST(pdfu_initiator, test_push_whole_blocks)
{
	struct pdfu_initiator_status st;

	image_len = 512;
	st = push(NULL);

	zassert_equal(st.state, PDFU_INIT_DONE, "%s", st.error);
	zassert_equal(validated_len, 512);
}

ZTEST(pdfu_initiator, test_push_aborted)
{
	struct pdfu_initiator_faults faults = {.abort_after = 1};
	struct pdfu_initiator_status st = push(&faults);

	zassert_equal(st.state, PDFU_INIT_ABORTED);
	k_sleep(K_MSEC(50));
	zassert_true(aborted);
	zassert_equal(pdfu_responder_phase(), PDFU_PHASE_ENUMERATION);
}

ZTEST(pdfu_initiator, test_push_corrupt)
{
	struct pdfu_initiator_faults faults = {.corrupt_block = 2};
	struct pdfu_initiator_status st = push(&faults);

	zassert_equal(st.state, PDFU_INIT_FAILED);
	zassert_equal(st.last_status, PDFU_ERR_FILE);
	k_sleep(K_MSEC(200));
	zassert_false(manifested);
	zassert_equal(pdfu_responder_phase(), PDFU_PHASE_ENUMERATION);
}

ZTEST(pdfu_initiator, test_push_lost_request)
{
	struct pdfu_initiator_status st;

	drop_next_data = true;
	st = push(NULL);

	zassert_equal(st.state, PDFU_INIT_DONE, "%s", st.error);
	zassert_mem_equal(store, image, 612);
}

ZTEST(pdfu_initiator, test_busy)
{
	struct pdfu_initiator_status st;

	loopback = true;
	zassert_ok(pdfu_initiator_start(NULL));
	zassert_equal(pdfu_initiator_start(NULL), -EBUSY);
	pdfu_initiator_abort();
	pdfu_initiator_status(&st);
	zassert_equal(st.state, PDFU_INIT_ABORTED);
	k_sleep(K_MSEC(50));
}

ZTEST_SUITE(pdfu_initiator, NULL, NULL, reset, NULL, NULL);
