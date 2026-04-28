/*
 * Copyright (c) 2026 Philipp Steiner <philipp.steiner1987@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "btca.h"
#include "clock.h"
#include "msg.h"
#include "port.h"

/* Port foreign-master tests only need basic announce comparison and ref handling. */
int ptp_msg_announce_cmp(const struct ptp_announce_msg *m1, const struct ptp_announce_msg *m2)
{
	size_t offset = offsetof(struct ptp_announce_msg, gm_priority1);
	size_t len = offsetof(struct ptp_announce_msg, time_src) - offset;

	return memcmp((const uint8_t *)m1 + offset, (const uint8_t *)m2 + offset, len);
}

void ptp_msg_ref(struct ptp_msg *msg)
{
	ARG_UNUSED(msg);
}

void ptp_msg_unref(struct ptp_msg *msg)
{
	ARG_UNUSED(msg);
}

/* Signed comparator contract: > 0 means a is better, < 0 means b is better. */
int ptp_btca_ds_cmp(const struct ptp_dataset *a, const struct ptp_dataset *b)
{
	int clk_id_cmp;

	if (a == b) {
		return 0;
	}

	if (a && !b) {
		return 1;
	}

	if (!a && b) {
		return -1;
	}

	if (a->priority1 < b->priority1) {
		return 1;
	}

	if (a->priority1 > b->priority1) {
		return -1;
	}

	if (a->priority2 < b->priority2) {
		return 1;
	}

	if (a->priority2 > b->priority2) {
		return -1;
	}

	clk_id_cmp = memcmp(&a->clk_id, &b->clk_id, sizeof(a->clk_id));
	if (clk_id_cmp < 0) {
		return 1;
	}
	if (clk_id_cmp > 0) {
		return -1;
	}

	return 0;
}

static struct ptp_port port;
static struct ptp_msg msg1;
static struct ptp_msg msg2;
static struct ptp_msg msg3;
static struct ptp_msg msg4;

static void timer_dummy_cb(struct k_timer *timer)
{
	ARG_UNUSED(timer);
}

static void set_clk_id(ptp_clk_id *clk_id, uint8_t value)
{
	memset(clk_id->id, value, sizeof(clk_id->id));
}

static void set_host_timestamp_now(struct ptp_msg *msg)
{
	int64_t now_ns = k_uptime_get() * NSEC_PER_MSEC;

	msg->timestamp.host.second = now_ns / NSEC_PER_SEC;
	msg->timestamp.host.nanosecond = now_ns % NSEC_PER_SEC;
}

static void set_host_timestamp_ns(struct ptp_msg *msg, int64_t timestamp_ns)
{
	msg->timestamp.host.second = timestamp_ns / NSEC_PER_SEC;
	msg->timestamp.host.nanosecond = timestamp_ns % NSEC_PER_SEC;
}

static void init_announce_msg_ext(struct ptp_msg *msg, uint8_t sender_id, uint16_t sender_port,
				  uint8_t gm_priority1, uint8_t gm_priority2, uint16_t steps_rm,
				  int8_t log_msg_interval)
{
	memset(msg, 0, sizeof(*msg));

	set_clk_id(&msg->header.src_port_id.clk_id, sender_id);
	msg->header.src_port_id.port_number = sender_port;
	msg->header.log_msg_interval = log_msg_interval;

	msg->announce.gm_priority1 = gm_priority1;
	msg->announce.gm_priority2 = gm_priority2;
	msg->announce.steps_rm = steps_rm;
	msg->announce.gm_clk_quality.cls = 128;
	msg->announce.gm_clk_quality.accuracy = 0x31;
	msg->announce.gm_clk_quality.offset_scaled_log_variance = 0x200;
	set_clk_id(&msg->announce.gm_id, (uint8_t)(sender_id + 1));

	set_host_timestamp_now(msg);
}

static void init_announce_msg(struct ptp_msg *msg, uint8_t sender_id, uint16_t sender_port,
			      uint8_t gm_priority1)
{
	init_announce_msg_ext(msg, sender_id, sender_port, gm_priority1, 100, 1, 0);
}

static struct ptp_foreign_tt_clock *first_foreign_clock(struct ptp_port *port_ctx)
{
	sys_snode_t *node = sys_slist_peek_head(&port_ctx->foreign_list);

	if (!node) {
		return NULL;
	}

	return CONTAINER_OF(node, struct ptp_foreign_tt_clock, node);
}

static int foreign_clock_count(struct ptp_port *port_ctx)
{
	struct ptp_foreign_tt_clock *foreign;
	int count = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&port_ctx->foreign_list, foreign, node) {
		count++;
	}

	return count;
}

static struct ptp_foreign_tt_clock *find_foreign_clock(struct ptp_port *port_ctx, uint8_t sender_id,
						       uint16_t sender_port)
{
	struct ptp_foreign_tt_clock *foreign;
	struct ptp_port_id sender = {0};

	set_clk_id(&sender.clk_id, sender_id);
	sender.port_number = sender_port;

	SYS_SLIST_FOR_EACH_CONTAINER(&port_ctx->foreign_list, foreign, node) {
		if (ptp_port_id_eq(&foreign->dataset.sender, &sender)) {
			return foreign;
		}
	}

	return NULL;
}

static void foreign_master_before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&port, 0, sizeof(port));
	sys_slist_init(&port.foreign_list);

	port.port_ds.id.port_number = 1;
	port.port_ds.announce_receipt_timeout = 3;
	port.port_ds.log_announce_interval = 0;

	k_timer_init(&port.timers.announce, timer_dummy_cb, NULL);
}

static void foreign_master_after(void *fixture)
{
	ARG_UNUSED(fixture);

	ptp_port_free_foreign_tts(&port);
}

ZTEST(ptp_foreign_master, test_first_foreign_announce_is_stored)
{
	struct ptp_foreign_tt_clock *foreign;
	int ret;

	init_announce_msg(&msg1, 0x11, 1, 120);

	ret = ptp_port_add_foreign_tt(&port, &msg1);
	zassert_equal(ret, 0, "first announce should not trigger threshold");

	foreign = first_foreign_clock(&port);
	zassert_not_null(foreign, "foreign clock was not created");
	zassert_equal(foreign->messages_count, 1, "first announce was not stored");
	zassert_equal(k_fifo_peek_tail(&foreign->messages), &msg1,
		      "first announce is not queue tail");
}

ZTEST(ptp_foreign_master, test_second_announce_reaches_threshold)
{
	struct ptp_foreign_tt_clock *foreign;
	int ret;

	init_announce_msg(&msg1, 0x22, 1, 120);
	init_announce_msg(&msg2, 0x22, 1, 120);

	zassert_equal(ptp_port_add_foreign_tt(&port, &msg1), 0, "first add failed");

	ret = ptp_port_add_foreign_tt(&port, &msg2);
	zassert_equal(ret, 1, "second announce should raise threshold event");

	foreign = first_foreign_clock(&port);
	zassert_not_null(foreign, "foreign clock missing");
	zassert_equal(foreign->messages_count, FOREIGN_TIME_TRANSMITTER_THRESHOLD,
		      "unexpected foreign message count");
	zassert_equal(k_fifo_peek_tail(&foreign->messages), &msg2,
		      "second announce is not queue tail");
}

ZTEST(ptp_foreign_master, test_repeated_announce_no_diff_after_threshold)
{
	struct ptp_foreign_tt_clock *foreign;
	int ret;

	init_announce_msg(&msg1, 0x26, 1, 120);
	init_announce_msg(&msg2, 0x26, 1, 120);
	init_announce_msg(&msg3, 0x26, 1, 120);

	zassert_equal(ptp_port_add_foreign_tt(&port, &msg1), 0, "first add failed");
	zassert_equal(ptp_port_add_foreign_tt(&port, &msg2), 1,
		      "second add should report threshold");

	ret = ptp_port_add_foreign_tt(&port, &msg3);
	zassert_equal(ret, 0, "repeated announce should not report dataset change");

	foreign = first_foreign_clock(&port);
	zassert_not_null(foreign, "foreign clock missing");
	zassert_equal(foreign_clock_count(&port), 1, "unexpected number of foreign clocks");
	zassert_equal(k_fifo_peek_tail(&foreign->messages), &msg3,
		      "latest repeated announce is not queue tail");
}

ZTEST(ptp_foreign_master, test_changed_announce_returns_nonzero_on_add_path)
{
	int ret;

	init_announce_msg(&msg1, 0x27, 1, 120);
	init_announce_msg(&msg2, 0x27, 1, 120);
	init_announce_msg(&msg3, 0x27, 1, 110);

	zassert_equal(ptp_port_add_foreign_tt(&port, &msg1), 0, "first add failed");
	zassert_equal(ptp_port_add_foreign_tt(&port, &msg2), 1,
		      "second add should report threshold");

	ret = ptp_port_add_foreign_tt(&port, &msg3);
	zassert_not_equal(ret, 0, "changed announce dataset should be detected");
}

ZTEST(ptp_foreign_master, test_two_distinct_senders_are_tracked)
{
	init_announce_msg(&msg1, 0x28, 1, 120);
	init_announce_msg(&msg4, 0x38, 1, 120);

	zassert_equal(ptp_port_add_foreign_tt(&port, &msg1), 0, "first add failed");
	zassert_equal(ptp_port_add_foreign_tt(&port, &msg4), 0, "second sender add failed");
	zassert_equal(foreign_clock_count(&port), 2,
		      "two distinct source IDs should create two foreign entries");
}

ZTEST(ptp_foreign_master, test_update_current_tt_compares_against_previous_latest)
{
	struct ptp_foreign_tt_clock *foreign;
	int ret;

	init_announce_msg(&msg1, 0x33, 1, 110);
	init_announce_msg(&msg2, 0x33, 1, 110);
	init_announce_msg(&msg3, 0x33, 1, 111);

	zassert_equal(ptp_port_add_foreign_tt(&port, &msg1), 0, "first add failed");
	zassert_equal(ptp_port_add_foreign_tt(&port, &msg2), 1, "second add failed");

	foreign = first_foreign_clock(&port);
	zassert_not_null(foreign, "foreign clock missing");
	port.best = foreign;

	ret = ptp_port_update_current_time_transmitter(&port, &msg3);
	zassert_not_equal(ret, 0, "changed announce must differ from previous latest");
	zassert_equal(foreign->messages_count, FOREIGN_TIME_TRANSMITTER_THRESHOLD + 1,
		      "unexpected message count after update");
	zassert_equal(k_fifo_peek_tail(&foreign->messages), &msg3,
		      "updated announce is not queue tail");
}

ZTEST(ptp_foreign_master, test_update_current_tt_falls_back_to_add_for_new_sender)
{
	int ret;

	init_announce_msg(&msg1, 0x44, 1, 120);
	init_announce_msg(&msg4, 0x55, 1, 120);

	zassert_equal(ptp_port_add_foreign_tt(&port, &msg1), 0, "first add failed");
	port.best = first_foreign_clock(&port);
	zassert_not_null(port.best, "best foreign not set");

	ret = ptp_port_update_current_time_transmitter(&port, &msg4);
	zassert_equal(ret, 0, "new sender should behave as first add");
	zassert_equal(foreign_clock_count(&port), 2, "new sender should add new foreign clock");
}

ZTEST(ptp_foreign_master, test_best_foreign_returns_null_below_threshold)
{
	init_announce_msg(&msg1, 0x61, 1, 120);
	init_announce_msg(&msg2, 0x71, 1, 110);

	zassert_equal(ptp_port_add_foreign_tt(&port, &msg1), 0, "first sender add failed");
	zassert_equal(ptp_port_add_foreign_tt(&port, &msg2), 0, "second sender add failed");

	zassert_is_null(ptp_port_best_foreign(&port),
			"best foreign should be NULL below threshold");
}

ZTEST(ptp_foreign_master, test_best_foreign_returns_null_when_time_transmitter_only)
{
	init_announce_msg(&msg1, 0x62, 1, 120);
	init_announce_msg(&msg2, 0x62, 1, 120);

	zassert_equal(ptp_port_add_foreign_tt(&port, &msg1), 0, "first add failed");
	zassert_equal(ptp_port_add_foreign_tt(&port, &msg2), 1, "second add failed");

	port.port_ds.time_transmitter_only = true;

	zassert_is_null(ptp_port_best_foreign(&port),
			"timeTransmitterOnly port should not select foreign best");
}

ZTEST(ptp_foreign_master, test_best_foreign_selects_better_sender)
{
	struct ptp_foreign_tt_clock *best;
	struct ptp_foreign_tt_clock *foreign_a;
	struct ptp_foreign_tt_clock *foreign_b;

	init_announce_msg(&msg1, 0x63, 1, 130);
	init_announce_msg(&msg2, 0x63, 1, 130);
	init_announce_msg(&msg3, 0x73, 1, 120);
	init_announce_msg(&msg4, 0x73, 1, 120);

	zassert_equal(ptp_port_add_foreign_tt(&port, &msg1), 0, "sender A add 1 failed");
	zassert_equal(ptp_port_add_foreign_tt(&port, &msg2), 1, "sender A add 2 failed");
	zassert_equal(ptp_port_add_foreign_tt(&port, &msg3), 0, "sender B add 1 failed");
	zassert_equal(ptp_port_add_foreign_tt(&port, &msg4), 1, "sender B add 2 failed");

	foreign_a = find_foreign_clock(&port, 0x63, 1);
	foreign_b = find_foreign_clock(&port, 0x73, 1);

	zassert_not_null(foreign_a, "sender A foreign clock missing");
	zassert_not_null(foreign_b, "sender B foreign clock missing");

	best = ptp_port_best_foreign(&port);
	zassert_equal(best, foreign_b, "lower priority sender should be selected as best");
}

ZTEST(ptp_foreign_master, test_best_foreign_negative_cmp_does_not_replace_best)
{
	struct ptp_foreign_tt_clock *best;
	struct ptp_foreign_tt_clock *foreign_a;
	struct ptp_foreign_tt_clock *foreign_b;

	init_announce_msg(&msg1, 0x64, 1, 110);
	init_announce_msg(&msg2, 0x64, 1, 110);
	init_announce_msg(&msg3, 0x74, 1, 140);
	init_announce_msg(&msg4, 0x74, 1, 140);

	zassert_equal(ptp_port_add_foreign_tt(&port, &msg1), 0, "sender A add 1 failed");
	zassert_equal(ptp_port_add_foreign_tt(&port, &msg2), 1, "sender A add 2 failed");
	zassert_equal(ptp_port_add_foreign_tt(&port, &msg3), 0, "sender B add 1 failed");
	zassert_equal(ptp_port_add_foreign_tt(&port, &msg4), 1, "sender B add 2 failed");

	foreign_a = find_foreign_clock(&port, 0x64, 1);
	foreign_b = find_foreign_clock(&port, 0x74, 1);

	zassert_not_null(foreign_a, "sender A foreign clock missing");
	zassert_not_null(foreign_b, "sender B foreign clock missing");

	best = ptp_port_best_foreign(&port);
	zassert_equal(best, foreign_a, "worse foreign should not replace the current best");
	zassert_equal(foreign_b->messages_count, 0,
		      "losing foreign should be cleared after comparison");
}

ZTEST(ptp_foreign_master, test_best_foreign_dataset_fields_populated)
{
	struct ptp_foreign_tt_clock *best;
	struct ptp_port_id expected_sender = {0};

	init_announce_msg_ext(&msg1, 0x65, 7, 101, 88, 9, 0);
	init_announce_msg_ext(&msg2, 0x65, 7, 101, 88, 9, 0);

	msg1.announce.gm_clk_quality.cls = 6;
	msg1.announce.gm_clk_quality.accuracy = 0x20;
	msg1.announce.gm_clk_quality.offset_scaled_log_variance = 0x1234;
	set_clk_id(&msg1.announce.gm_id, 0xAB);

	msg2.announce.gm_clk_quality = msg1.announce.gm_clk_quality;
	msg2.announce.gm_id = msg1.announce.gm_id;

	zassert_equal(ptp_port_add_foreign_tt(&port, &msg1), 0, "first add failed");
	zassert_equal(ptp_port_add_foreign_tt(&port, &msg2), 1, "second add failed");

	best = ptp_port_best_foreign(&port);
	zassert_not_null(best, "best foreign missing");

	set_clk_id(&expected_sender.clk_id, 0x65);
	expected_sender.port_number = 7;

	zassert_equal(best->dataset.priority1, msg1.announce.gm_priority1, "priority1 mismatch");
	zassert_equal(best->dataset.priority2, msg1.announce.gm_priority2, "priority2 mismatch");
	zassert_equal(best->dataset.steps_rm, msg1.announce.steps_rm, "steps_rm mismatch");
	zassert_mem_equal(&best->dataset.clk_quality, &msg1.announce.gm_clk_quality,
			  sizeof(msg1.announce.gm_clk_quality), "clock quality mismatch");
	zassert_mem_equal(&best->dataset.clk_id, &msg1.announce.gm_id, sizeof(msg1.announce.gm_id),
			  "gm clock id mismatch");
	zassert_true(ptp_port_id_eq(&best->dataset.receiver, &port.port_ds.id),
		     "receiver port ID mismatch");
	zassert_true(ptp_port_id_eq(&best->dataset.sender, &expected_sender),
		     "sender port ID mismatch");
}

ZTEST(ptp_foreign_master, test_best_foreign_cleanup_trims_to_threshold)
{
	struct ptp_foreign_tt_clock *foreign;

	init_announce_msg(&msg1, 0x66, 1, 120);
	init_announce_msg(&msg2, 0x66, 1, 120);
	init_announce_msg(&msg3, 0x66, 1, 120);

	zassert_equal(ptp_port_add_foreign_tt(&port, &msg1), 0, "first add failed");
	zassert_equal(ptp_port_add_foreign_tt(&port, &msg2), 1, "second add failed");
	zassert_equal(ptp_port_add_foreign_tt(&port, &msg3), 0, "third add failed");

	foreign = find_foreign_clock(&port, 0x66, 1);
	zassert_not_null(foreign, "foreign clock missing");
	zassert_equal(foreign->messages_count, FOREIGN_TIME_TRANSMITTER_THRESHOLD + 1,
		      "test setup did not exceed threshold");

	zassert_not_null(ptp_port_best_foreign(&port), "best foreign should exist");
	zassert_equal(foreign->messages_count, FOREIGN_TIME_TRANSMITTER_THRESHOLD,
		      "cleanup should trim message count to threshold");
	zassert_equal(k_fifo_peek_head(&foreign->messages), &msg2,
		      "oldest message after trim should be msg2");
	zassert_equal(k_fifo_peek_tail(&foreign->messages), &msg3,
		      "newest message after trim should be msg3");
}

ZTEST(ptp_foreign_master, test_best_foreign_cleanup_drops_expired_records)
{
	struct ptp_foreign_tt_clock *foreign;

	init_announce_msg(&msg1, 0x67, 1, 120);
	init_announce_msg(&msg2, 0x67, 1, 120);

	zassert_equal(ptp_port_add_foreign_tt(&port, &msg1), 0, "first add failed");
	zassert_equal(ptp_port_add_foreign_tt(&port, &msg2), 1, "second add failed");

	msg1.header.log_msg_interval = -31;
	msg2.header.log_msg_interval = -31;
	set_host_timestamp_ns(&msg1, 0);
	set_host_timestamp_ns(&msg2, 0);

	zassert_is_null(ptp_port_best_foreign(&port), "expired records should not produce best");

	foreign = find_foreign_clock(&port, 0x67, 1);
	zassert_not_null(foreign, "foreign clock missing");
	zassert_equal(foreign->messages_count, 0, "expired records should be removed");
	zassert_true(k_fifo_is_empty(&foreign->messages), "foreign message queue should be empty");
}

ZTEST_SUITE(ptp_foreign_master, NULL, NULL, foreign_master_before, foreign_master_after, NULL);
