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

struct ptp_tlv_container *ptp_tlv_alloc(void)
{
	zassert_unreachable("Unexpected TLV allocation");
	return NULL;
}

void ptp_tlv_free(struct ptp_tlv_container *tlv_container)
{
	ARG_UNUSED(tlv_container);
	zassert_unreachable("Unexpected TLV free");
}

int ptp_tlv_post_recv(struct ptp_tlv *tlv)
{
	ARG_UNUSED(tlv);
	zassert_unreachable("Unexpected TLV post-receive");
	return -EINVAL;
}

void ptp_tlv_pre_send(struct ptp_tlv *tlv)
{
	ARG_UNUSED(tlv);
	zassert_unreachable("Unexpected TLV pre-send");
}

const struct ptp_parent_ds *ptp_clock_parent_ds(void)
{
	return &test_parent_ds;
}

static void init_announce_msg(struct ptp_msg *msg)
{
	memset(msg, 0, sizeof(*msg));
	sys_slist_init(&msg->tlvs);

	msg->header.type_major_sdo_id = PTP_MSG_ANNOUNCE;
	msg->header.version = PTP_VERSION;
	msg->header.msg_length = net_htons(sizeof(struct ptp_announce_msg));

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

ZTEST_SUITE(ptp_msg_post_recv, NULL, NULL, NULL, NULL, NULL);
