/*
 * Copyright (c) 2026 Philipp Steiner <philipp.steiner1987@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/ptp.h>
#include <zephyr/ztest.h>

#include "clock.h"
#include "msg.h"
#include "tlv.h"

static struct ptp_parent_ds test_parent_ds;
static struct ptp_tlv_container tlv_pool[4];
static int tlv_alloc_calls;
static int tlv_free_calls;
static int tlv_post_recv_calls;
static int tlv_pre_send_calls;
static int tlv_post_recv_ret;
static struct ptp_tlv *last_post_recv_tlv;
static struct ptp_tlv *last_pre_send_tlv;

struct ptp_tlv_container *ptp_tlv_alloc(void)
{
	if (tlv_alloc_calls >= ARRAY_SIZE(tlv_pool)) {
		return NULL;
	}

	return &tlv_pool[tlv_alloc_calls++];
}

void ptp_tlv_free(struct ptp_tlv_container *tlv_container)
{
	zassert_not_null(tlv_container, "Unexpected null TLV free");
	tlv_free_calls++;
}

int ptp_tlv_post_recv(struct ptp_tlv *tlv)
{
	last_post_recv_tlv = tlv;
	tlv_post_recv_calls++;

	return tlv_post_recv_ret;
}

void ptp_tlv_pre_send(struct ptp_tlv *tlv)
{
	last_pre_send_tlv = tlv;
	tlv_pre_send_calls++;
}

const struct ptp_parent_ds *ptp_clock_parent_ds(void)
{
	return &test_parent_ds;
}

static void reset_tlv_fakes(void)
{
	memset(tlv_pool, 0, sizeof(tlv_pool));
	tlv_alloc_calls = 0;
	tlv_free_calls = 0;
	tlv_post_recv_calls = 0;
	tlv_pre_send_calls = 0;
	tlv_post_recv_ret = 0;
	last_post_recv_tlv = NULL;
	last_pre_send_tlv = NULL;
	memset(&test_parent_ds, 0, sizeof(test_parent_ds));
}

static void msg_post_recv_before(void *fixture)
{
	ARG_UNUSED(fixture);

	reset_tlv_fakes();
}

static void init_msg_list(struct ptp_msg *msg)
{
	memset(msg, 0, sizeof(*msg));
	sys_slist_init(&msg->tlvs);
}

static void cleanup_msg_tlvs(struct ptp_msg *msg)
{
	sys_snode_t *node;

	while ((node = sys_slist_get(&msg->tlvs)) != NULL) {
		ptp_tlv_free(CONTAINER_OF(node, struct ptp_tlv_container, node));
	}
}

static void init_header(struct ptp_msg *msg, enum ptp_msg_type type, uint16_t length)
{
	msg->header.type_major_sdo_id = type;
	msg->header.version = PTP_VERSION;
	msg->header.msg_length = net_htons(length);
	msg->header.correction = net_htonll(0x0102030405060708LL);
	msg->header.sequence_id = net_htons(0xBEEF);
	msg->header.src_port_id.port_number = net_htons(0x1234);
}

static void init_announce_msg(struct ptp_msg *msg)
{
	init_msg_list(msg);

	init_header(msg, PTP_MSG_ANNOUNCE, sizeof(struct ptp_announce_msg));

	msg->announce.current_utc_offset = net_htons(37);
	msg->announce.gm_clk_quality.offset_scaled_log_variance = net_htons(0x1234);
	msg->announce.steps_rm = net_htons(2);
}

ZTEST(ptp_msg_post_recv, test_announce_preserves_transport_timestamp)
{
	struct ptp_port port = {0};
	struct ptp_msg msg;
	int ret;

	init_announce_msg(&msg);
	port.port_ds.id.port_number = 1;

	msg.timestamp.host.second = 1234;
	msg.timestamp.host.nanosecond = 567890123;

	ret = ptp_msg_post_recv(&port, &msg, sizeof(struct ptp_announce_msg));
	zassert_ok(ret, "announce post-receive failed");
	zassert_equal(msg.timestamp.host.second, 1234, "transport timestamp seconds changed");
	zassert_equal(msg.timestamp.host.nanosecond, 567890123,
		      "transport timestamp nanoseconds changed");
}

ZTEST(ptp_msg_post_recv, test_announce_synthesizes_fallback_timestamp)
{
	struct ptp_port port = {0};
	struct ptp_msg msg;
	int ret;

	k_sleep(K_MSEC(1));

	init_announce_msg(&msg);
	port.port_ds.id.port_number = 1;

	ret = ptp_msg_post_recv(&port, &msg, sizeof(struct ptp_announce_msg));
	zassert_ok(ret, "announce post-receive failed");
	zassert_true(msg.timestamp.host.second != 0 || msg.timestamp.host.nanosecond != 0,
		     "missing synthesized fallback timestamp");
}

ZTEST(ptp_msg_post_recv, test_short_packet_is_rejected)
{
	struct ptp_port port = {0};
	struct ptp_msg msg;

	init_msg_list(&msg);
	init_header(&msg, PTP_MSG_SYNC, sizeof(struct ptp_sync_msg));

	zassert_equal(ptp_msg_post_recv(&port, &msg, sizeof(struct ptp_header)), -EBADMSG,
		      "short sync packet should be rejected");
}

ZTEST(ptp_msg_post_recv, test_unsupported_major_version_is_rejected)
{
	struct ptp_port port = {0};
	struct ptp_msg msg;

	init_msg_list(&msg);
	init_header(&msg, PTP_MSG_SYNC, sizeof(struct ptp_sync_msg));
	msg.header.version = 1;

	zassert_equal(ptp_msg_post_recv(&port, &msg, sizeof(struct ptp_sync_msg)), -EBADMSG,
		      "unsupported PTP version should be rejected");
}

ZTEST(ptp_msg_post_recv, test_delay_resp_post_recv_converts_header_timestamp_and_port)
{
	struct ptp_port port = {0};
	struct ptp_msg msg;

	init_msg_list(&msg);
	init_header(&msg, PTP_MSG_DELAY_RESP, sizeof(struct ptp_delay_resp_msg));
	msg.delay_resp.receive_timestamp.seconds_high = net_htons(0x0001);
	msg.delay_resp.receive_timestamp.seconds_low = net_htonl(0x23456789);
	msg.delay_resp.receive_timestamp.nanoseconds = net_htonl(987654321);
	msg.delay_resp.req_port_id.port_number = net_htons(0x4567);

	zassert_ok(ptp_msg_post_recv(&port, &msg, sizeof(struct ptp_delay_resp_msg)),
		   "delay response post-receive failed");
	zassert_equal(msg.header.msg_length, sizeof(struct ptp_delay_resp_msg),
		      "message length not converted");
	zassert_equal(msg.header.correction, 0x0102030405060708LL, "correction not converted");
	zassert_equal(msg.header.sequence_id, 0xBEEF, "sequence ID not converted");
	zassert_equal(msg.header.src_port_id.port_number, 0x1234, "source port not converted");
	zassert_equal(msg.timestamp.protocol.second, 0x000123456789ULL,
		      "protocol timestamp seconds mismatch");
	zassert_equal(msg.timestamp.protocol.nanosecond, 987654321,
		      "protocol timestamp nanoseconds mismatch");
	zassert_equal(msg.delay_resp.req_port_id.port_number, 0x4567,
		      "requesting port ID not converted");
}

ZTEST(ptp_msg_post_recv, test_valid_tlv_is_parsed_and_linked)
{
	struct ptp_port port = {0};
	struct ptp_msg msg;
	struct ptp_tlv *tlv;
	uint16_t len = sizeof(struct ptp_announce_msg) + sizeof(struct ptp_tlv) + 2;

	init_announce_msg(&msg);
	msg.header.msg_length = net_htons(len);
	tlv = (struct ptp_tlv *)msg.announce.suffix;
	tlv->type = net_htons(PTP_TLV_TYPE_PAD);
	tlv->length = net_htons(2);
	msg.announce.suffix[sizeof(*tlv)] = 0xAA;
	msg.announce.suffix[sizeof(*tlv) + 1] = 0x55;

	zassert_ok(ptp_msg_post_recv(&port, &msg, len), "TLV post-receive failed");
	zassert_equal(tlv_alloc_calls, 1, "TLV container not allocated");
	zassert_equal(tlv_post_recv_calls, 1, "TLV post-receive not called");
	zassert_equal(last_post_recv_tlv, tlv, "unexpected TLV passed to post-receive");
	zassert_equal(tlv->type, PTP_TLV_TYPE_PAD, "TLV type not converted");
	zassert_equal(tlv->length, 2, "TLV length not converted");
	zassert_not_null(sys_slist_peek_head(&msg.tlvs), "TLV not linked to message");

	cleanup_msg_tlvs(&msg);
}

ZTEST(ptp_msg_post_recv, test_odd_tlv_length_is_rejected)
{
	struct ptp_port port = {0};
	struct ptp_msg msg;
	struct ptp_tlv *tlv;
	uint16_t len = sizeof(struct ptp_announce_msg) + sizeof(struct ptp_tlv) + 1;

	init_announce_msg(&msg);
	msg.header.msg_length = net_htons(len);
	tlv = (struct ptp_tlv *)msg.announce.suffix;
	tlv->type = net_htons(PTP_TLV_TYPE_PAD);
	tlv->length = net_htons(1);

	zassert_equal(ptp_msg_post_recv(&port, &msg, len), -EBADMSG,
		      "odd TLV length should be rejected");
	zassert_equal(tlv_free_calls, 1, "rejected TLV should be freed");
}

ZTEST(ptp_msg_post_recv, test_oversized_tlv_length_is_rejected)
{
	struct ptp_port port = {0};
	struct ptp_msg msg;
	struct ptp_tlv *tlv;
	uint16_t len = sizeof(struct ptp_announce_msg) + sizeof(struct ptp_tlv);

	init_announce_msg(&msg);
	msg.header.msg_length = net_htons(len);
	tlv = (struct ptp_tlv *)msg.announce.suffix;
	tlv->type = net_htons(PTP_TLV_TYPE_PAD);
	tlv->length = net_htons(2);

	zassert_equal(ptp_msg_post_recv(&port, &msg, len), -EBADMSG,
		      "oversized TLV should be rejected");
	zassert_equal(tlv_free_calls, 1, "rejected TLV should be freed");
}

ZTEST(ptp_msg_post_recv, test_header_tlv_length_mismatch_is_rejected)
{
	struct ptp_port port = {0};
	struct ptp_msg msg;
	struct ptp_tlv *tlv;
	uint16_t cnt = sizeof(struct ptp_announce_msg) + sizeof(struct ptp_tlv) + 2;

	init_announce_msg(&msg);
	msg.header.msg_length = net_htons(sizeof(struct ptp_announce_msg));
	tlv = (struct ptp_tlv *)msg.announce.suffix;
	tlv->type = net_htons(PTP_TLV_TYPE_PAD);
	tlv->length = net_htons(2);

	zassert_equal(ptp_msg_post_recv(&port, &msg, cnt), -EMSGSIZE,
		      "header/TLV length mismatch should be rejected");

	cleanup_msg_tlvs(&msg);
}

ZTEST(ptp_msg_post_recv, test_pre_send_converts_announce_and_header_fields)
{
	struct ptp_msg msg;

	init_msg_list(&msg);
	msg.header.type_major_sdo_id = PTP_MSG_ANNOUNCE;
	msg.header.version = PTP_VERSION;
	msg.header.msg_length = sizeof(struct ptp_announce_msg);
	msg.header.correction = 0x0102030405060708LL;
	msg.header.sequence_id = 0xBEEF;
	msg.header.src_port_id.port_number = 0x1234;
	msg.announce.current_utc_offset = 37;
	msg.announce.gm_clk_quality.offset_scaled_log_variance = 0x1234;
	msg.announce.steps_rm = 2;

	ptp_msg_pre_send(&msg);

	zassert_equal(msg.header.msg_length, net_htons(sizeof(struct ptp_announce_msg)),
		      "message length not converted");
	zassert_equal(msg.header.correction, net_htonll(0x0102030405060708LL),
		      "correction not converted");
	zassert_equal(msg.header.sequence_id, net_htons(0xBEEF), "sequence ID not converted");
	zassert_equal(msg.header.src_port_id.port_number, net_htons(0x1234),
		      "source port not converted");
	zassert_equal(msg.announce.current_utc_offset, net_htons(37), "UTC offset not converted");
	zassert_equal(msg.announce.gm_clk_quality.offset_scaled_log_variance, net_htons(0x1234),
		      "clock quality variance not converted");
	zassert_equal(msg.announce.steps_rm, net_htons(2), "steps removed not converted");
}

ZTEST(ptp_msg_post_recv, test_pre_send_converts_response_and_target_port_fields)
{
	struct ptp_msg msg;

	init_msg_list(&msg);
	msg.header.type_major_sdo_id = PTP_MSG_DELAY_RESP;
	msg.header.version = PTP_VERSION;
	msg.header.msg_length = sizeof(struct ptp_delay_resp_msg);
	msg.delay_resp.receive_timestamp.seconds_high = 0x0001;
	msg.delay_resp.receive_timestamp.seconds_low = 0x23456789;
	msg.delay_resp.receive_timestamp.nanoseconds = 987654321;
	msg.delay_resp.req_port_id.port_number = 0x4567;

	ptp_msg_pre_send(&msg);
	zassert_equal(msg.delay_resp.receive_timestamp.seconds_high, net_htons(0x0001),
		      "timestamp high seconds not converted");
	zassert_equal(msg.delay_resp.receive_timestamp.seconds_low, net_htonl(0x23456789),
		      "timestamp low seconds not converted");
	zassert_equal(msg.delay_resp.receive_timestamp.nanoseconds, net_htonl(987654321),
		      "timestamp nanoseconds not converted");
	zassert_equal(msg.delay_resp.req_port_id.port_number, net_htons(0x4567),
		      "delay response request port not converted");

	init_msg_list(&msg);
	msg.header.type_major_sdo_id = PTP_MSG_SIGNALING;
	msg.header.version = PTP_VERSION;
	msg.header.msg_length = sizeof(struct ptp_signaling_msg);
	msg.signaling.target_port_id.port_number = 0x7654;
	ptp_msg_pre_send(&msg);
	zassert_equal(msg.signaling.target_port_id.port_number, net_htons(0x7654),
		      "signaling target port not converted");

	init_msg_list(&msg);
	msg.header.type_major_sdo_id = PTP_MSG_MANAGEMENT;
	msg.header.version = PTP_VERSION;
	msg.header.msg_length = sizeof(struct ptp_management_msg);
	msg.management.target_port_id.port_number = 0x9876;
	ptp_msg_pre_send(&msg);
	zassert_equal(msg.management.target_port_id.port_number, net_htons(0x9876),
		      "management target port not converted");
}

ZTEST(ptp_msg_post_recv, test_pre_send_processes_and_frees_linked_tlvs)
{
	struct ptp_msg msg;
	struct ptp_tlv tlv = {
		.type = PTP_TLV_TYPE_PAD,
		.length = 0,
	};

	init_msg_list(&msg);
	msg.header.type_major_sdo_id = PTP_MSG_ANNOUNCE;
	msg.header.version = PTP_VERSION;
	msg.header.msg_length = sizeof(struct ptp_announce_msg);

	tlv_pool[0].tlv = &tlv;
	sys_slist_append(&msg.tlvs, &tlv_pool[0].node);

	ptp_msg_pre_send(&msg);

	zassert_equal(tlv_pre_send_calls, 1, "TLV pre-send not called");
	zassert_equal(last_pre_send_tlv, &tlv, "unexpected TLV passed to pre-send");
	zassert_equal(tlv_free_calls, 1, "TLV container not freed");
	zassert_is_null(sys_slist_peek_head(&msg.tlvs), "TLV list not cleared");
}

ZTEST(ptp_msg_post_recv, test_add_tlv_uses_suffix_and_existing_tail)
{
	struct ptp_msg msg;
	struct ptp_tlv *first;
	struct ptp_tlv *second;

	init_msg_list(&msg);
	msg.header.type_major_sdo_id = PTP_MSG_ANNOUNCE;
	msg.header.version = PTP_VERSION;
	msg.header.msg_length = sizeof(struct ptp_announce_msg);

	first = ptp_msg_add_tlv(&msg, sizeof(struct ptp_tlv) + 2);
	zassert_not_null(first, "first TLV allocation failed");
	first->type = PTP_TLV_TYPE_PAD;
	first->length = 2;
	sys_slist_append(&msg.tlvs, &tlv_pool[0].node);
	zassert_equal((uint8_t *)first, msg.announce.suffix, "first TLV placement mismatch");
	zassert_equal(msg.header.msg_length,
		      sizeof(struct ptp_announce_msg) + sizeof(struct ptp_tlv) + 2,
		      "message length not extended");

	second = ptp_msg_add_tlv(&msg, sizeof(struct ptp_tlv));
	zassert_not_null(second, "second TLV allocation failed");
	zassert_equal((uint8_t *)second,
		      msg.announce.suffix + sizeof(struct ptp_tlv) + first->length,
		      "second TLV placement mismatch");
}

ZTEST(ptp_msg_post_recv, test_add_tlv_rejects_unknown_type_and_bounds_failure)
{
	struct ptp_msg msg;

	init_msg_list(&msg);
	msg.header.type_major_sdo_id = 0xF;
	zassert_is_null(ptp_msg_add_tlv(&msg, sizeof(struct ptp_tlv)),
			"unknown message type should not accept TLV");

	init_msg_list(&msg);
	msg.header.type_major_sdo_id = PTP_MSG_ANNOUNCE;
	msg.header.msg_length = sizeof(struct ptp_announce_msg);
	zassert_is_null(ptp_msg_add_tlv(&msg, sizeof(msg)), "oversized TLV should be rejected");
}

ZTEST_SUITE(ptp_msg_post_recv, NULL, NULL, msg_post_recv_before, NULL, NULL);
