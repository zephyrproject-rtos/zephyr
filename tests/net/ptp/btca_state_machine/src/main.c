/*
 * Copyright (c) 2026 Philipp Steiner <philipp.steiner1987@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>

#include "btca.h"
#include "clock.h"
#include "port.h"
#include "state_machine.h"

static const struct ptp_dataset *stub_clock_default_ds;
static const struct ptp_dataset *stub_clock_best_ds;
static struct ptp_dataset *stub_port_best_ds;
static enum ptp_port_state stub_port_state_value;
static const struct ptp_foreign_tt_clock *stub_clock_best_foreign;

const struct ptp_dataset *ptp_clock_ds(void)
{
	return stub_clock_default_ds;
}

const struct ptp_dataset *ptp_clock_best_foreign_ds(void)
{
	return stub_clock_best_ds;
}

struct ptp_dataset *ptp_port_best_foreign_ds(struct ptp_port *port)
{
	(void)port;

	return stub_port_best_ds;
}

enum ptp_port_state ptp_port_state(struct ptp_port *port)
{
	(void)port;

	return stub_port_state_value;
}

const struct ptp_foreign_tt_clock *ptp_clock_best_time_transmitter(void)
{
	return stub_clock_best_foreign;
}

static void set_clk_id(ptp_clk_id *clk_id, uint8_t value)
{
	memset(clk_id->id, value, sizeof(clk_id->id));
}

static struct ptp_port_id make_port_id(uint8_t clk_id_value, uint16_t port_number)
{
	struct ptp_port_id id = {0};

	set_clk_id(&id.clk_id, clk_id_value);
	id.port_number = port_number;

	return id;
}

static struct ptp_dataset make_dataset(uint8_t clk_id_value)
{
	struct ptp_dataset ds = {
		.priority1 = 128,
		.clk_quality.cls = 128,
		.clk_quality.accuracy = 0x31,
		.clk_quality.offset_scaled_log_variance = 0x8100,
		.priority2 = 128,
		.steps_rm = 2,
	};

	set_clk_id(&ds.clk_id, clk_id_value);
	ds.sender = make_port_id((uint8_t)(clk_id_value + 1), 10);
	ds.receiver = make_port_id((uint8_t)(clk_id_value + 2), 20);

	return ds;
}

static void reset_btca_stubs(void)
{
	stub_clock_default_ds = NULL;
	stub_clock_best_ds = NULL;
	stub_port_best_ds = NULL;
	stub_port_state_value = PTP_PS_INITIALIZING;
	stub_clock_best_foreign = NULL;
}

static void btca_state_machine_before(void *fixture)
{
	(void)fixture;
	reset_btca_stubs();
}

ZTEST(ptp_btca_state_machine, test_btca_ds_cmp_ranking_order_lower_is_better)
{
	struct ptp_dataset a;
	struct ptp_dataset b;
	int ret;

	a = make_dataset(0x10);
	b = make_dataset(0x20);
	a.priority1 = 100;
	b.priority1 = 101;
	ret = ptp_btca_ds_cmp(&a, &b);
	zassert_true(ret > 0, "priority1 compare failed: %d", ret);
	ret = ptp_btca_ds_cmp(&b, &a);
	zassert_true(ret < 0, "priority1 reverse compare failed: %d", ret);

	a = make_dataset(0x10);
	b = make_dataset(0x20);
	a.priority1 = b.priority1;
	a.clk_quality.cls = 100;
	b.clk_quality.cls = 101;
	ret = ptp_btca_ds_cmp(&a, &b);
	zassert_true(ret > 0, "clock class compare failed: %d", ret);
	ret = ptp_btca_ds_cmp(&b, &a);
	zassert_true(ret < 0, "clock class reverse compare failed: %d", ret);

	a = make_dataset(0x10);
	b = make_dataset(0x20);
	a.priority1 = b.priority1;
	a.clk_quality.cls = b.clk_quality.cls;
	a.clk_quality.accuracy = 0x20;
	b.clk_quality.accuracy = 0x21;
	ret = ptp_btca_ds_cmp(&a, &b);
	zassert_true(ret > 0, "accuracy compare failed: %d", ret);
	ret = ptp_btca_ds_cmp(&b, &a);
	zassert_true(ret < 0, "accuracy reverse compare failed: %d", ret);

	a = make_dataset(0x10);
	b = make_dataset(0x20);
	a.priority1 = b.priority1;
	a.clk_quality.cls = b.clk_quality.cls;
	a.clk_quality.accuracy = b.clk_quality.accuracy;
	a.clk_quality.offset_scaled_log_variance = 1000;
	b.clk_quality.offset_scaled_log_variance = 1001;
	ret = ptp_btca_ds_cmp(&a, &b);
	zassert_true(ret > 0, "variance compare failed: %d", ret);
	ret = ptp_btca_ds_cmp(&b, &a);
	zassert_true(ret < 0, "variance reverse compare failed: %d", ret);

	a = make_dataset(0x10);
	b = make_dataset(0x20);
	a.priority1 = b.priority1;
	a.clk_quality.cls = b.clk_quality.cls;
	a.clk_quality.accuracy = b.clk_quality.accuracy;
	a.clk_quality.offset_scaled_log_variance = b.clk_quality.offset_scaled_log_variance;
	a.priority2 = 100;
	b.priority2 = 101;
	ret = ptp_btca_ds_cmp(&a, &b);
	zassert_true(ret > 0, "priority2 compare failed: %d", ret);
	ret = ptp_btca_ds_cmp(&b, &a);
	zassert_true(ret < 0, "priority2 reverse compare failed: %d", ret);
}

ZTEST(ptp_btca_state_machine, test_btca_ds_cmp_same_pointer_is_equal)
{
	struct ptp_dataset a = make_dataset(0x0A);

	zassert_equal(ptp_btca_ds_cmp(&a, &a), 0, "same object must compare equal");
}

ZTEST(ptp_btca_state_machine, test_btca_ds_cmp_null_handling)
{
	struct ptp_dataset a = make_dataset(0x0B);
	int ret;

	ret = ptp_btca_ds_cmp(&a, NULL);
	zassert_true(ret > 0, "dataset should beat NULL: %d", ret);

	ret = ptp_btca_ds_cmp(NULL, &a);
	zassert_true(ret < 0, "NULL should lose to dataset: %d", ret);
}

ZTEST(ptp_btca_state_machine, test_btca_ds_cmp_steps_removed_preference)
{
	struct ptp_dataset a = make_dataset(0x0C);
	struct ptp_dataset b = a;
	int ret;

	/* Keep same clock identity to force the ds_cmp2() path. */
	a.steps_rm = 1;
	b.steps_rm = 3;

	ret = ptp_btca_ds_cmp(&a, &b);
	zassert_true(ret > 0, "fewer steps removed should win: %d", ret);

	ret = ptp_btca_ds_cmp(&b, &a);
	zassert_true(ret < 0, "more steps removed should lose: %d", ret);
}

ZTEST(ptp_btca_state_machine, test_btca_ds_cmp_receiver_port_tie_break)
{
	struct ptp_dataset a = make_dataset(0x33);
	struct ptp_dataset b = a;
	int ret;

	a.sender = make_port_id(0x44, 8);
	b.sender = a.sender;

	a.receiver = make_port_id(0x55, 1);
	b.receiver = make_port_id(0x55, 2);

	ret = ptp_btca_ds_cmp(&a, &b);
	zassert_true(ret > 0, "receiver tie-break failed: %d", ret);
	ret = ptp_btca_ds_cmp(&b, &a);
	zassert_true(ret < 0, "receiver tie-break reverse failed: %d", ret);
}

ZTEST(ptp_btca_state_machine, test_state_machine_time_transmitter_ignores_rs_grand_master)
{
	enum ptp_port_state new_state;

	new_state = ptp_state_machine(PTP_PS_TIME_TRANSMITTER, PTP_EVT_RS_GRAND_MASTER, false);
	zassert_equal(new_state, PTP_PS_TIME_TRANSMITTER, "unexpected state: %d", new_state);
}

ZTEST(ptp_btca_state_machine, test_btca_state_decision_passive_on_topology_tie_break)
{
	struct ptp_port local_port = {0};
	struct ptp_port remote_port = {0};
	struct ptp_foreign_tt_clock best_foreign = {
		.port = &remote_port,
	};
	struct ptp_dataset clk_default = make_dataset(0x70);
	struct ptp_dataset clk_best = make_dataset(0x22);
	struct ptp_dataset port_best;
	enum ptp_port_state decision;

	clk_default.priority1 = 200;
	clk_default.clk_quality.cls = 248;

	clk_best.priority1 = 100;
	clk_best.clk_quality.cls = 248;
	clk_best.steps_rm = 4;
	clk_best.sender = make_port_id(0x23, 20);
	clk_best.receiver = make_port_id(0x24, 1);

	port_best = clk_best;
	port_best.receiver.port_number = 2;

	stub_clock_default_ds = &clk_default;
	stub_clock_best_ds = &clk_best;
	stub_port_best_ds = &port_best;
	stub_port_state_value = PTP_PS_TIME_TRANSMITTER;
	stub_clock_best_foreign = &best_foreign;

	decision = ptp_btca_state_decision(&local_port);
	zassert_equal(decision, PTP_PS_PASSIVE, "expected PASSIVE, got %d", decision);
}

ZTEST(ptp_btca_state_machine, test_btca_state_decision_listening_without_foreign)
{
	struct ptp_port local_port = {0};
	struct ptp_foreign_tt_clock best_foreign = {
		.port = &local_port,
	};
	struct ptp_dataset ds = make_dataset(0x90);
	enum ptp_port_state decision;

	stub_clock_default_ds = &ds;
	stub_clock_best_ds = &ds;
	stub_port_best_ds = NULL;
	stub_port_state_value = PTP_PS_LISTENING;
	stub_clock_best_foreign = &best_foreign;

	decision = ptp_btca_state_decision(&local_port);
	zassert_equal(decision, PTP_PS_LISTENING, "expected LISTENING, got %d", decision);
}

ZTEST_SUITE(ptp_btca_state_machine, NULL, NULL, btca_state_machine_before, NULL, NULL);
