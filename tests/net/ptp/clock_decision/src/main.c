/*
 * Copyright (c) 2026 Philipp Steiner <philipp.steiner1987@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "btca.h"
#include "clock.h"
#include "msg.h"
#include "port.h"

#define MAX_TRACKED_PORTS 16

struct port_stub_ctx {
	struct ptp_port *port;
	struct ptp_foreign_tt_clock *foreign;
	enum ptp_port_state decision;
	int event_calls;
	enum ptp_port_event last_event;
	bool last_tt_diff;
};

struct foreign_desc {
	uint8_t sender_seed;
	uint16_t sender_port_num;
	uint8_t priority1;
	uint8_t priority2;
	uint16_t steps_rm;
};

static struct port_stub_ctx ctx_map[MAX_TRACKED_PORTS];
static size_t ctx_map_count;
static int total_event_calls;

static void set_clk_id(ptp_clk_id *clk_id, uint8_t seed)
{
	for (size_t i = 0; i < ARRAY_SIZE(clk_id->id); i++) {
		clk_id->id[i] = (uint8_t)(seed + i);
	}
}

static struct port_stub_ctx *ctx_for_port(struct ptp_port *port, bool create)
{
	for (size_t i = 0; i < ctx_map_count; i++) {
		if (ctx_map[i].port == port) {
			return &ctx_map[i];
		}
	}

	if (!create || ctx_map_count >= ARRAY_SIZE(ctx_map)) {
		return NULL;
	}

	ctx_map[ctx_map_count].port = port;
	ctx_map[ctx_map_count].decision = PTP_PS_LISTENING;

	return &ctx_map[ctx_map_count++];
}

bool ptp_port_id_eq(const struct ptp_port_id *p1, const struct ptp_port_id *p2)
{
	return memcmp(&p1->clk_id, &p2->clk_id, sizeof(p1->clk_id)) == 0 &&
	       p1->port_number == p2->port_number;
}

struct ptp_foreign_tt_clock *ptp_port_best_foreign(struct ptp_port *port)
{
	struct port_stub_ctx *ctx = ctx_for_port(port, false);

	return ctx ? ctx->foreign : NULL;
}

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

enum ptp_port_state ptp_btca_state_decision(struct ptp_port *port)
{
	struct port_stub_ctx *ctx = ctx_for_port(port, false);

	return ctx ? ctx->decision : PTP_PS_LISTENING;
}

void ptp_port_event_handle(struct ptp_port *port, enum ptp_port_event event, bool tt_diff)
{
	struct port_stub_ctx *ctx = ctx_for_port(port, false);

	zassert_not_null(ctx, "missing context for port");

	ctx->event_calls++;
	ctx->last_event = event;
	ctx->last_tt_diff = tt_diff;
	total_event_calls++;
}

static struct ptp_default_ds *clock_default_ds_mut(void)
{
	return (struct ptp_default_ds *)ptp_clock_default_ds();
}

static struct ptp_parent_ds *clock_parent_ds_mut(void)
{
	return (struct ptp_parent_ds *)ptp_clock_parent_ds();
}

static struct ptp_current_ds *clock_current_ds_mut(void)
{
	return (struct ptp_current_ds *)ptp_clock_current_ds();
}

static struct ptp_time_prop_ds *clock_time_prop_ds_mut(void)
{
	return (struct ptp_time_prop_ds *)ptp_clock_time_prop_ds();
}

static void init_port(struct ptp_port *port, uint8_t clk_seed, uint16_t port_num)
{
	memset(port, 0, sizeof(*port));

	set_clk_id(&port->port_ds.id.clk_id, clk_seed);
	port->port_ds.id.port_number = port_num;

	sys_slist_append(ptp_clock_ports_list(), &port->node);
}

static void init_announce_msg(struct ptp_msg *msg, uint8_t gm_seed, uint8_t prio1, uint8_t prio2,
			      uint16_t steps_rm, uint16_t utc_offset, uint8_t flags1)
{
	memset(msg, 0, sizeof(*msg));

	msg->announce.gm_priority1 = prio1;
	msg->announce.gm_priority2 = prio2;
	msg->announce.steps_rm = steps_rm;
	msg->announce.current_utc_offset = utc_offset;
	msg->header.flags[1] = flags1;

	msg->announce.gm_clk_quality.cls = 128;
	msg->announce.gm_clk_quality.accuracy = 0x33;
	msg->announce.gm_clk_quality.offset_scaled_log_variance = 0x1234;

	set_clk_id(&msg->announce.gm_id, gm_seed);
}

static struct foreign_desc make_foreign_desc(uint8_t sender_seed, uint16_t sender_port_num,
					     uint8_t priority1, uint8_t priority2,
					     uint16_t steps_rm)
{
	struct foreign_desc desc;

	desc.sender_seed = sender_seed;
	desc.sender_port_num = sender_port_num;
	desc.priority1 = priority1;
	desc.priority2 = priority2;
	desc.steps_rm = steps_rm;

	return desc;
}

static void init_foreign(struct ptp_foreign_tt_clock *foreign, struct ptp_port *receiver,
			 struct foreign_desc desc, struct ptp_msg *announce_msg)
{
	memset(foreign, 0, sizeof(*foreign));

	foreign->port = receiver;
	foreign->dataset.priority1 = desc.priority1;
	foreign->dataset.priority2 = desc.priority2;
	foreign->dataset.steps_rm = desc.steps_rm;
	foreign->dataset.sender.port_number = desc.sender_port_num;

	set_clk_id(&foreign->dataset.clk_id, (uint8_t)(desc.sender_seed + 0x40));
	set_clk_id(&foreign->dataset.sender.clk_id, desc.sender_seed);

	if (receiver) {
		foreign->dataset.receiver = receiver->port_ds.id;
	}

	k_fifo_init(&foreign->messages);
	if (announce_msg) {
		foreign->messages_count = 1;
		k_fifo_put(&foreign->messages, announce_msg);
	}
}

static void map_port(struct ptp_port *port, enum ptp_port_state decision,
		     struct ptp_foreign_tt_clock *foreign)
{
	struct port_stub_ctx *ctx = ctx_for_port(port, true);

	zassert_not_null(ctx, "could not allocate port context");
	ctx->decision = decision;
	ctx->foreign = foreign;
	ctx->event_calls = 0;
	ctx->last_event = PTP_EVT_NONE;
	ctx->last_tt_diff = false;
}

static void assert_single_event(struct ptp_port *port, enum ptp_port_event expected_event)
{
	struct port_stub_ctx *ctx = ctx_for_port(port, false);

	zassert_not_null(ctx, "missing context for asserted port");
	zassert_equal(ctx->event_calls, 1, "unexpected event count");
	zassert_equal(ctx->last_event, expected_event, "unexpected event value");
	zassert_false(ctx->last_tt_diff, "tt_diff should remain false");
}

static void reset_clock_decision_state(void)
{
	struct ptp_default_ds *dds = clock_default_ds_mut();
	struct ptp_parent_ds *pds = clock_parent_ds_mut();
	struct ptp_current_ds *cds = clock_current_ds_mut();
	struct ptp_time_prop_ds *tpds = clock_time_prop_ds_mut();

	memset(dds, 0, sizeof(*dds));
	memset(pds, 0, sizeof(*pds));
	memset(cds, 0, sizeof(*cds));
	memset(tpds, 0, sizeof(*tpds));
	sys_slist_init(ptp_clock_ports_list());

	memset(ctx_map, 0, sizeof(ctx_map));
	ctx_map_count = 0;
	total_event_calls = 0;

	/* Force state-decision flag and best pointer into a known cleared state. */
	ptp_clock_state_decision_req();
	ptp_clock_handle_state_decision_evt();
}

static void clock_decision_before(void *fixture)
{
	ARG_UNUSED(fixture);
	reset_clock_decision_state();
}

ZTEST(ptp_clock_decision, test_no_state_decision_request_noop)
{
	struct ptp_port port;
	struct ptp_foreign_tt_clock foreign;

	init_port(&port, 0x10, 1);
	init_foreign(&foreign, &port, make_foreign_desc(0x20, 2, 120, 100, 3), NULL);
	map_port(&port, PTP_PS_GRAND_MASTER, &foreign);

	ptp_clock_handle_state_decision_evt();

	zassert_equal(total_event_calls, 0, "handler should do nothing without request");
	zassert_is_null(ptp_clock_best_time_transmitter(), "best foreign should remain unchanged");
	zassert_is_null(ptp_clock_best_foreign_ds(), "best foreign dataset should remain NULL");
}

ZTEST(ptp_clock_decision, test_request_without_eligible_foreign_keeps_best_null)
{
	struct ptp_port port;

	init_port(&port, 0x11, 1);
	map_port(&port, PTP_PS_LISTENING, NULL);

	ptp_clock_state_decision_req();
	ptp_clock_handle_state_decision_evt();

	zassert_is_null(ptp_clock_best_time_transmitter(), "best foreign should be NULL");
	zassert_is_null(ptp_clock_best_foreign_ds(), "best foreign dataset should be NULL");
	assert_single_event(&port, PTP_EVT_NONE);
	zassert_equal(total_event_calls, 1, "unexpected total event calls");
}

ZTEST(ptp_clock_decision, test_best_selection_chooses_better_foreign)
{
	struct ptp_port port_a;
	struct ptp_port port_b;
	struct ptp_foreign_tt_clock foreign_a;
	struct ptp_foreign_tt_clock foreign_b;

	init_port(&port_a, 0x12, 1);
	init_port(&port_b, 0x13, 2);

	init_foreign(&foreign_a, &port_a, make_foreign_desc(0x22, 2, 130, 100, 1), NULL);
	init_foreign(&foreign_b, &port_b, make_foreign_desc(0x23, 3, 120, 100, 1), NULL);

	map_port(&port_a, PTP_PS_LISTENING, &foreign_a);
	map_port(&port_b, PTP_PS_LISTENING, &foreign_b);

	ptp_clock_state_decision_req();
	ptp_clock_handle_state_decision_evt();

	zassert_equal(ptp_clock_best_time_transmitter(), &foreign_b,
		      "lower priority foreign should be selected");
	zassert_equal(ptp_clock_best_foreign_ds(), &foreign_b.dataset,
		      "best dataset getter should point to selected foreign");
	assert_single_event(&port_a, PTP_EVT_NONE);
	assert_single_event(&port_b, PTP_EVT_NONE);
}

ZTEST(ptp_clock_decision, test_negative_comparator_does_not_replace_best)
{
	struct ptp_port port_a;
	struct ptp_port port_b;
	struct ptp_foreign_tt_clock foreign_a;
	struct ptp_foreign_tt_clock foreign_b;

	init_port(&port_a, 0x14, 1);
	init_port(&port_b, 0x15, 2);

	init_foreign(&foreign_a, &port_a, make_foreign_desc(0x24, 2, 110, 100, 1), NULL);
	init_foreign(&foreign_b, &port_b, make_foreign_desc(0x25, 3, 140, 100, 1), NULL);

	map_port(&port_a, PTP_PS_LISTENING, &foreign_a);
	map_port(&port_b, PTP_PS_LISTENING, &foreign_b);

	ptp_clock_state_decision_req();
	ptp_clock_handle_state_decision_evt();

	zassert_equal(ptp_clock_best_time_transmitter(), &foreign_a,
		      "worse foreign must not replace current best");
}

ZTEST(ptp_clock_decision, test_state_to_event_mapping)
{
	struct ptp_port p_listening;
	struct ptp_port p_grand_master;
	struct ptp_port p_time_transmitter;
	struct ptp_port p_time_receiver;
	struct ptp_port p_passive;
	struct ptp_port p_unexpected;
	struct ptp_foreign_tt_clock foreign_best;
	struct ptp_msg best_msg;

	init_announce_msg(&best_msg, 0x70, 90, 80, 5, 41, 0xA5);

	init_port(&p_listening, 0x20, 1);
	init_port(&p_grand_master, 0x21, 2);
	init_port(&p_time_transmitter, 0x22, 3);
	init_port(&p_time_receiver, 0x23, 4);
	init_port(&p_passive, 0x24, 5);
	init_port(&p_unexpected, 0x25, 6);

	init_foreign(&foreign_best, &p_listening, make_foreign_desc(0x30, 9, 100, 90, 4),
		     &best_msg);

	map_port(&p_listening, PTP_PS_LISTENING, &foreign_best);
	map_port(&p_grand_master, PTP_PS_GRAND_MASTER, NULL);
	map_port(&p_time_transmitter, PTP_PS_TIME_TRANSMITTER, NULL);
	map_port(&p_time_receiver, PTP_PS_TIME_RECEIVER, NULL);
	map_port(&p_passive, PTP_PS_PASSIVE, NULL);
	map_port(&p_unexpected, PTP_PS_INITIALIZING, NULL);

	ptp_clock_state_decision_req();
	ptp_clock_handle_state_decision_evt();

	assert_single_event(&p_listening, PTP_EVT_NONE);
	assert_single_event(&p_grand_master, PTP_EVT_RS_GRAND_MASTER);
	assert_single_event(&p_time_transmitter, PTP_EVT_RS_TIME_TRANSMITTER);
	assert_single_event(&p_time_receiver, PTP_EVT_RS_TIME_RECEIVER);
	assert_single_event(&p_passive, PTP_EVT_RS_PASSIVE);
	assert_single_event(&p_unexpected, PTP_EVT_FAULT_DETECTED);
	zassert_equal(total_event_calls, 6, "unexpected mapped event call count");
}

ZTEST(ptp_clock_decision, test_grand_master_update_path)
{
	struct ptp_port port;
	struct ptp_default_ds *dds = clock_default_ds_mut();
	struct ptp_current_ds *cds = clock_current_ds_mut();
	struct ptp_parent_ds *pds = clock_parent_ds_mut();
	struct ptp_time_prop_ds *tpds = clock_time_prop_ds_mut();

	init_port(&port, 0x31, 1);
	map_port(&port, PTP_PS_GRAND_MASTER, NULL);

	set_clk_id(&dds->clk_id, 0x50);
	dds->clk_quality.cls = 7;
	dds->clk_quality.accuracy = 0x20;
	dds->clk_quality.offset_scaled_log_variance = 0x2200;
	dds->priority1 = 100;
	dds->priority2 = 101;

	cds->steps_rm = 55;
	cds->offset_from_tt = 0xAAAA;
	cds->mean_delay = 0xBBBB;
	tpds->current_utc_offset = 1;
	tpds->flags = 0xFF;

	ptp_clock_state_decision_req();
	ptp_clock_handle_state_decision_evt();

	zassert_equal(cds->steps_rm, 0, "current_ds.steps_rm should reset");
	zassert_equal(cds->offset_from_tt, 0, "current_ds.offset should reset");
	zassert_equal(cds->mean_delay, 0, "current_ds.mean_delay should reset");

	zassert_mem_equal(&pds->port_id.clk_id, &dds->clk_id, sizeof(dds->clk_id),
			  "parent port clock id mismatch");
	zassert_mem_equal(&pds->gm_id, &dds->clk_id, sizeof(dds->clk_id),
			  "grandmaster id mismatch");
	zassert_equal(pds->port_id.port_number, 0, "parent port number mismatch");
	zassert_mem_equal(&pds->gm_clk_quality, &dds->clk_quality, sizeof(dds->clk_quality),
			  "clock quality mismatch");
	zassert_equal(pds->gm_priority1, dds->priority1, "priority1 mismatch");
	zassert_equal(pds->gm_priority2, dds->priority2, "priority2 mismatch");

	zassert_equal(tpds->current_utc_offset, 37, "UTC offset should be forced to 37");
	zassert_equal(tpds->flags, 0, "time properties flags should reset");
	assert_single_event(&port, PTP_EVT_RS_GRAND_MASTER);
}

ZTEST(ptp_clock_decision, test_time_receiver_update_path)
{
	struct ptp_port port;
	struct ptp_foreign_tt_clock foreign;
	struct ptp_msg msg;
	struct ptp_current_ds *cds = clock_current_ds_mut();
	struct ptp_parent_ds *pds = clock_parent_ds_mut();
	struct ptp_time_prop_ds *tpds = clock_time_prop_ds_mut();

	init_announce_msg(&msg, 0x80, 111, 112, 6, 44, 0x5A);
	init_port(&port, 0x41, 1);
	init_foreign(&foreign, &port, make_foreign_desc(0x55, 7, 120, 100, 6), &msg);
	map_port(&port, PTP_PS_TIME_RECEIVER, &foreign);

	ptp_clock_state_decision_req();
	ptp_clock_handle_state_decision_evt();

	zassert_equal(cds->steps_rm, 7, "steps_rm should be 1 + best dataset steps");
	zassert_mem_equal(&pds->gm_id, &msg.announce.gm_id, sizeof(msg.announce.gm_id),
			  "parent gm_id mismatch");
	zassert_true(ptp_port_id_eq(&pds->port_id, &foreign.dataset.sender),
		     "parent sender port mismatch");
	zassert_mem_equal(&pds->gm_clk_quality, &msg.announce.gm_clk_quality,
			  sizeof(msg.announce.gm_clk_quality), "parent gm clock quality mismatch");
	zassert_equal(pds->gm_priority1, msg.announce.gm_priority1, "priority1 mismatch");
	zassert_equal(pds->gm_priority2, msg.announce.gm_priority2, "priority2 mismatch");

	zassert_equal(tpds->current_utc_offset, msg.announce.current_utc_offset,
		      "UTC offset mismatch");
	zassert_equal(tpds->flags, msg.header.flags[1], "time_prop flags mismatch");
	assert_single_event(&port, PTP_EVT_RS_TIME_RECEIVER);
}

ZTEST(ptp_clock_decision, test_getters_reflect_selected_best)
{
	struct ptp_port port;
	struct ptp_foreign_tt_clock foreign;

	init_port(&port, 0x42, 1);
	init_foreign(&foreign, &port, make_foreign_desc(0x56, 8, 90, 80, 2), NULL);
	map_port(&port, PTP_PS_LISTENING, &foreign);

	ptp_clock_state_decision_req();
	ptp_clock_handle_state_decision_evt();

	zassert_equal(ptp_clock_best_time_transmitter(), &foreign,
		      "best time transmitter getter mismatch");
	zassert_equal(ptp_clock_best_foreign_ds(), &foreign.dataset,
		      "best foreign dataset getter mismatch");
}

ZTEST_SUITE(ptp_clock_decision, NULL, NULL, clock_decision_before, NULL, NULL);
