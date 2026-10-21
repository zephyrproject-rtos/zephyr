/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/ptp.h>
#include <zephyr/ztest.h>

#include "clock.h"
#include "msg.h"
#include "tlv.h"

/* The pools are private to msg.c and tlv.c, their sizes are not. */
#define MSG_POOL_SIZE CONFIG_PTP_MSG_POLL_SIZE
#define TLV_POOL_SIZE (2 * CONFIG_PTP_MSG_POLL_SIZE)

static struct ptp_parent_ds test_parent_ds;

const struct ptp_parent_ds *ptp_clock_parent_ds(void)
{
	return &test_parent_ds;
}

/* Take every TLV container that is free and return how many there were. */
static int take_tlv_containers(struct ptp_tlv_container **containers, int max)
{
	int count = 0;

	while (count < max) {
		containers[count] = ptp_tlv_alloc();
		if (containers[count] == NULL) {
			break;
		}
		count++;
	}

	return count;
}

static void free_tlv_containers(struct ptp_tlv_container **containers, int count)
{
	for (int i = 0; i < count; i++) {
		ptp_tlv_free(containers[i]);
	}
}

/* An allocation from an empty pool must fail. If it waited instead, the PTP
 * thread, which is the only one that releases messages, would never run again.
 */
ZTEST(ptp_msg_pool, test_msg_alloc_fails_when_pool_is_empty)
{
	struct ptp_msg *msgs[MSG_POOL_SIZE];

	for (int i = 0; i < MSG_POOL_SIZE; i++) {
		msgs[i] = ptp_msg_alloc();
		zassert_not_null(msgs[i], "message %d not allocated", i);
	}

	zassert_is_null(ptp_msg_alloc(), "allocation from an empty pool should fail");

	ptp_msg_unref(msgs[0]);
	msgs[0] = ptp_msg_alloc();
	zassert_not_null(msgs[0], "a released message should be available again");

	for (int i = 0; i < MSG_POOL_SIZE; i++) {
		ptp_msg_unref(msgs[i]);
	}
}

ZTEST(ptp_msg_pool, test_tlv_alloc_fails_when_pool_is_empty)
{
	struct ptp_tlv_container *containers[TLV_POOL_SIZE + 1];
	int count;

	count = take_tlv_containers(containers, ARRAY_SIZE(containers));
	zassert_equal(count, TLV_POOL_SIZE, "unexpected number of TLV containers");

	ptp_tlv_free(containers[0]);
	containers[0] = ptp_tlv_alloc();
	zassert_not_null(containers[0], "a released container should be available again");

	free_tlv_containers(containers, count);
}

/* A received message with more TLVs than there are containers is rejected, and
 * the containers it took are available again once the message is released.
 */
ZTEST(ptp_msg_pool, test_message_with_too_many_tlvs_is_rejected)
{
	struct ptp_tlv_container *containers[TLV_POOL_SIZE + 1];
	struct ptp_port port = {0};
	struct ptp_msg *msg;
	struct ptp_tlv *tlv;
	uint16_t len = sizeof(struct ptp_announce_msg) +
		       (TLV_POOL_SIZE + 1) * sizeof(struct ptp_tlv);

	msg = ptp_msg_alloc();
	zassert_not_null(msg, "message not allocated");

	msg->header.type_major_sdo_id = PTP_MSG_ANNOUNCE;
	msg->header.version = PTP_VERSION;
	msg->header.msg_length = net_htons(len);

	/* TLVs without a value */
	tlv = (struct ptp_tlv *)msg->announce.suffix;
	for (int i = 0; i <= TLV_POOL_SIZE; i++) {
		tlv[i].type = net_htons(PTP_TLV_TYPE_PAD);
		tlv[i].length = 0;
	}

	zassert_not_equal(ptp_msg_post_recv(&port, msg, len), 0,
			  "a message with too many TLVs should be rejected");

	ptp_msg_unref(msg);

	zassert_equal(take_tlv_containers(containers, ARRAY_SIZE(containers)), TLV_POOL_SIZE,
		      "the containers of the rejected message were not released");
	free_tlv_containers(containers, TLV_POOL_SIZE);
}

/* The same message with as many TLVs as there are containers is accepted. */
ZTEST(ptp_msg_pool, test_message_with_all_tlv_containers_is_accepted)
{
	struct ptp_port port = {0};
	struct ptp_msg *msg;
	struct ptp_tlv *tlv;
	uint16_t len = sizeof(struct ptp_announce_msg) + TLV_POOL_SIZE * sizeof(struct ptp_tlv);

	msg = ptp_msg_alloc();
	zassert_not_null(msg, "message not allocated");

	msg->header.type_major_sdo_id = PTP_MSG_ANNOUNCE;
	msg->header.version = PTP_VERSION;
	msg->header.msg_length = net_htons(len);

	tlv = (struct ptp_tlv *)msg->announce.suffix;
	for (int i = 0; i < TLV_POOL_SIZE; i++) {
		tlv[i].type = net_htons(PTP_TLV_TYPE_PAD);
		tlv[i].length = 0;
	}

	zassert_ok(ptp_msg_post_recv(&port, msg, len), "a message that fits should be accepted");

	ptp_msg_unref(msg);
}

ZTEST_SUITE(ptp_msg_pool, NULL, NULL, NULL, NULL, NULL);
