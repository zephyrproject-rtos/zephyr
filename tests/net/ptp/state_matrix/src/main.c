/*
 * Copyright (c) 2026 Philipp Steiner <philipp.steiner1987@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "state_machine.h"

static const enum ptp_port_state all_states[] = {
	PTP_PS_INITIALIZING,         PTP_PS_FAULTY,           PTP_PS_DISABLED,     PTP_PS_LISTENING,
	PTP_PS_PRE_TIME_TRANSMITTER, PTP_PS_TIME_TRANSMITTER, PTP_PS_GRAND_MASTER, PTP_PS_PASSIVE,
	PTP_PS_UNCALIBRATED,         PTP_PS_TIME_RECEIVER,
};

static const enum ptp_port_event all_events[] = {
	PTP_EVT_NONE,
	PTP_EVT_POWERUP,
	PTP_EVT_INITIALIZE,
	PTP_EVT_INIT_COMPLETE,
	PTP_EVT_FAULT_DETECTED,
	PTP_EVT_FAULT_CLEARED,
	PTP_EVT_STATE_DECISION,
	PTP_EVT_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES,
	PTP_EVT_QUALIFICATION_TIMEOUT_EXPIRES,
	PTP_EVT_DESIGNATED_ENABLED,
	PTP_EVT_DESIGNATED_DISABLED,
	PTP_EVT_TIME_TRANSMITTER_CLOCK_SELECTED,
	PTP_EVT_SYNCHRONIZATION_FAULT,
	PTP_EVT_RS_TIME_TRANSMITTER,
	PTP_EVT_RS_GRAND_MASTER,
	PTP_EVT_RS_TIME_RECEIVER,
	PTP_EVT_RS_PASSIVE,
};

static const char *state_name(enum ptp_port_state state)
{
	switch (state) {
	case PTP_PS_INITIALIZING:
		return "INITIALIZING";
	case PTP_PS_FAULTY:
		return "FAULTY";
	case PTP_PS_DISABLED:
		return "DISABLED";
	case PTP_PS_LISTENING:
		return "LISTENING";
	case PTP_PS_PRE_TIME_TRANSMITTER:
		return "PRE_TIME_TRANSMITTER";
	case PTP_PS_TIME_TRANSMITTER:
		return "TIME_TRANSMITTER";
	case PTP_PS_GRAND_MASTER:
		return "GRAND_MASTER";
	case PTP_PS_PASSIVE:
		return "PASSIVE";
	case PTP_PS_UNCALIBRATED:
		return "UNCALIBRATED";
	case PTP_PS_TIME_RECEIVER:
		return "TIME_RECEIVER";
	default:
		return "UNKNOWN_STATE";
	}
}

static const char *event_name(enum ptp_port_event event)
{
	switch (event) {
	case PTP_EVT_NONE:
		return "NONE";
	case PTP_EVT_POWERUP:
		return "POWERUP";
	case PTP_EVT_INITIALIZE:
		return "INITIALIZE";
	case PTP_EVT_INIT_COMPLETE:
		return "INIT_COMPLETE";
	case PTP_EVT_FAULT_DETECTED:
		return "FAULT_DETECTED";
	case PTP_EVT_FAULT_CLEARED:
		return "FAULT_CLEARED";
	case PTP_EVT_STATE_DECISION:
		return "STATE_DECISION";
	case PTP_EVT_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES:
		return "ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES";
	case PTP_EVT_QUALIFICATION_TIMEOUT_EXPIRES:
		return "QUALIFICATION_TIMEOUT_EXPIRES";
	case PTP_EVT_DESIGNATED_ENABLED:
		return "DESIGNATED_ENABLED";
	case PTP_EVT_DESIGNATED_DISABLED:
		return "DESIGNATED_DISABLED";
	case PTP_EVT_TIME_TRANSMITTER_CLOCK_SELECTED:
		return "TIME_TRANSMITTER_CLOCK_SELECTED";
	case PTP_EVT_SYNCHRONIZATION_FAULT:
		return "SYNCHRONIZATION_FAULT";
	case PTP_EVT_RS_TIME_TRANSMITTER:
		return "RS_TIME_TRANSMITTER";
	case PTP_EVT_RS_GRAND_MASTER:
		return "RS_GRAND_MASTER";
	case PTP_EVT_RS_TIME_RECEIVER:
		return "RS_TIME_RECEIVER";
	case PTP_EVT_RS_PASSIVE:
		return "RS_PASSIVE";
	default:
		return "UNKNOWN_EVENT";
	}
}

static enum ptp_port_state expected_ptp_state_machine_all_present(enum ptp_port_state state,
								  enum ptp_port_event event)
{
	enum ptp_port_state new_state = state;

	if (event == PTP_EVT_INITIALIZE || event == PTP_EVT_POWERUP) {
		return PTP_PS_INITIALIZING;
	}

	switch (state) {
	case PTP_PS_INITIALIZING:
		switch (event) {
		case PTP_EVT_FAULT_DETECTED:
			new_state = PTP_PS_FAULTY;
			break;
		case PTP_EVT_INIT_COMPLETE:
			new_state = PTP_PS_LISTENING;
			break;
		default:
			break;
		}
		break;
	case PTP_PS_FAULTY:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = PTP_PS_DISABLED;
			break;
		case PTP_EVT_FAULT_CLEARED:
			new_state = PTP_PS_INITIALIZING;
			break;
		default:
			break;
		}
		break;
	case PTP_PS_DISABLED:
		if (event == PTP_EVT_DESIGNATED_ENABLED) {
			new_state = PTP_PS_INITIALIZING;
		}
		break;
	case PTP_PS_LISTENING:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = PTP_PS_DISABLED;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = PTP_PS_FAULTY;
			break;
		case PTP_EVT_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES:
			new_state = PTP_PS_TIME_TRANSMITTER;
			break;
		case PTP_EVT_RS_TIME_TRANSMITTER:
			new_state = PTP_PS_PRE_TIME_TRANSMITTER;
			break;
		case PTP_EVT_RS_GRAND_MASTER:
			new_state = PTP_PS_GRAND_MASTER;
			break;
		case PTP_EVT_RS_PASSIVE:
			new_state = PTP_PS_PASSIVE;
			break;
		case PTP_EVT_RS_TIME_RECEIVER:
			new_state = PTP_PS_UNCALIBRATED;
			break;
		default:
			break;
		}
		break;
	case PTP_PS_PRE_TIME_TRANSMITTER:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = PTP_PS_DISABLED;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = PTP_PS_FAULTY;
			break;
		case PTP_EVT_QUALIFICATION_TIMEOUT_EXPIRES:
			new_state = PTP_PS_TIME_TRANSMITTER;
			break;
		case PTP_EVT_RS_PASSIVE:
			new_state = PTP_PS_PASSIVE;
			break;
		case PTP_EVT_RS_TIME_RECEIVER:
			new_state = PTP_PS_UNCALIBRATED;
			break;
		default:
			break;
		}
		break;
	case PTP_PS_TIME_TRANSMITTER:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = PTP_PS_DISABLED;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = PTP_PS_FAULTY;
			break;
		case PTP_EVT_RS_PASSIVE:
			new_state = PTP_PS_PASSIVE;
			break;
		case PTP_EVT_RS_TIME_RECEIVER:
			new_state = PTP_PS_UNCALIBRATED;
			break;
		default:
			break;
		}
		break;
	case PTP_PS_GRAND_MASTER:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = PTP_PS_DISABLED;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = PTP_PS_FAULTY;
			break;
		case PTP_EVT_RS_PASSIVE:
			new_state = PTP_PS_PASSIVE;
			break;
		case PTP_EVT_RS_TIME_RECEIVER:
			new_state = PTP_PS_UNCALIBRATED;
			break;
		default:
			break;
		}
		break;
	case PTP_PS_PASSIVE:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = PTP_PS_DISABLED;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = PTP_PS_FAULTY;
			break;
		case PTP_EVT_RS_TIME_TRANSMITTER:
			new_state = PTP_PS_PRE_TIME_TRANSMITTER;
			break;
		case PTP_EVT_RS_GRAND_MASTER:
			new_state = PTP_PS_GRAND_MASTER;
			break;
		case PTP_EVT_RS_TIME_RECEIVER:
			new_state = PTP_PS_UNCALIBRATED;
			break;
		default:
			break;
		}
		break;
	case PTP_PS_UNCALIBRATED:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = PTP_PS_DISABLED;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = PTP_PS_FAULTY;
			break;
		case PTP_EVT_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES:
			new_state = PTP_PS_TIME_TRANSMITTER;
			break;
		case PTP_EVT_TIME_TRANSMITTER_CLOCK_SELECTED:
			new_state = PTP_PS_TIME_RECEIVER;
			break;
		case PTP_EVT_RS_TIME_TRANSMITTER:
			new_state = PTP_PS_PRE_TIME_TRANSMITTER;
			break;
		case PTP_EVT_RS_GRAND_MASTER:
			new_state = PTP_PS_GRAND_MASTER;
			break;
		case PTP_EVT_RS_PASSIVE:
			new_state = PTP_PS_PASSIVE;
			break;
		default:
			break;
		}
		break;
	case PTP_PS_TIME_RECEIVER:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = PTP_PS_DISABLED;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = PTP_PS_FAULTY;
			break;
		case PTP_EVT_SYNCHRONIZATION_FAULT:
			new_state = PTP_PS_UNCALIBRATED;
			break;
		case PTP_EVT_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES:
			new_state = PTP_PS_TIME_TRANSMITTER;
			break;
		case PTP_EVT_RS_TIME_TRANSMITTER:
			new_state = PTP_PS_PRE_TIME_TRANSMITTER;
			break;
		case PTP_EVT_RS_GRAND_MASTER:
			new_state = PTP_PS_GRAND_MASTER;
			break;
		case PTP_EVT_RS_PASSIVE:
			new_state = PTP_PS_PASSIVE;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return new_state;
}

static enum ptp_port_state expected_ptp_tr_state_machine_all_present(enum ptp_port_state state,
								     enum ptp_port_event event)
{
	enum ptp_port_state new_state = state;

	if (event == PTP_EVT_INITIALIZE || event == PTP_EVT_POWERUP) {
		return PTP_PS_INITIALIZING;
	}

	switch (state) {
	case PTP_PS_INITIALIZING:
		switch (event) {
		case PTP_EVT_FAULT_DETECTED:
			new_state = PTP_PS_FAULTY;
			break;
		case PTP_EVT_INIT_COMPLETE:
			new_state = PTP_PS_LISTENING;
			break;
		default:
			break;
		}
		break;
	case PTP_PS_FAULTY:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = PTP_PS_DISABLED;
			break;
		case PTP_EVT_FAULT_CLEARED:
			new_state = PTP_PS_INITIALIZING;
			break;
		default:
			break;
		}
		break;
	case PTP_PS_DISABLED:
		if (event == PTP_EVT_DESIGNATED_ENABLED) {
			new_state = PTP_PS_INITIALIZING;
		}
		break;
	case PTP_PS_LISTENING:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = PTP_PS_DISABLED;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = PTP_PS_FAULTY;
			break;
		case PTP_EVT_RS_TIME_RECEIVER:
			new_state = PTP_PS_UNCALIBRATED;
			break;
		default:
			break;
		}
		break;
	case PTP_PS_UNCALIBRATED:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = PTP_PS_DISABLED;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = PTP_PS_FAULTY;
			break;
		case PTP_EVT_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES:
		case PTP_EVT_RS_TIME_TRANSMITTER:
		case PTP_EVT_RS_GRAND_MASTER:
		case PTP_EVT_RS_PASSIVE:
			new_state = PTP_PS_LISTENING;
			break;
		case PTP_EVT_TIME_TRANSMITTER_CLOCK_SELECTED:
			new_state = PTP_PS_TIME_RECEIVER;
			break;
		default:
			break;
		}
		break;
	case PTP_PS_TIME_RECEIVER:
		switch (event) {
		case PTP_EVT_DESIGNATED_DISABLED:
			new_state = PTP_PS_DISABLED;
			break;
		case PTP_EVT_FAULT_DETECTED:
			new_state = PTP_PS_FAULTY;
			break;
		case PTP_EVT_SYNCHRONIZATION_FAULT:
			new_state = PTP_PS_UNCALIBRATED;
			break;
		case PTP_EVT_ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES:
		case PTP_EVT_RS_TIME_TRANSMITTER:
		case PTP_EVT_RS_GRAND_MASTER:
		case PTP_EVT_RS_PASSIVE:
			new_state = PTP_PS_LISTENING;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return new_state;
}

ZTEST(ptp_state_matrix, test_ptp_state_machine_full_matrix_tt_diff_false)
{
	size_t checks = 0;
	const size_t expected_checks = ARRAY_SIZE(all_states) * ARRAY_SIZE(all_events);

	for (size_t i = 0; i < ARRAY_SIZE(all_states); i++) {
		for (size_t j = 0; j < ARRAY_SIZE(all_events); j++) {
			enum ptp_port_state state = all_states[i];
			enum ptp_port_event event = all_events[j];
			enum ptp_port_state expected =
				expected_ptp_state_machine_all_present(state, event);
			enum ptp_port_state actual = ptp_state_machine(state, event, false);

			checks++;
			zassert_equal(actual, expected,
				      "ptp_state_machine(%s,%s,false): actual=%s expected=%s",
				      state_name(state), event_name(event), state_name(actual),
				      state_name(expected));
		}
	}

	zassert_equal(checks, expected_checks, "unexpected matrix check count");
}

ZTEST(ptp_state_matrix, test_ptp_tr_state_machine_full_matrix_tt_diff_false)
{
	size_t checks = 0;
	const size_t expected_checks = ARRAY_SIZE(all_states) * ARRAY_SIZE(all_events);

	for (size_t i = 0; i < ARRAY_SIZE(all_states); i++) {
		for (size_t j = 0; j < ARRAY_SIZE(all_events); j++) {
			enum ptp_port_state state = all_states[i];
			enum ptp_port_event event = all_events[j];
			enum ptp_port_state expected =
				expected_ptp_tr_state_machine_all_present(state, event);
			enum ptp_port_state actual = ptp_tr_state_machine(state, event, false);

			checks++;
			zassert_equal(actual, expected,
				      "ptp_tr_state_machine(%s,%s,false): actual=%s expected=%s",
				      state_name(state), event_name(event), state_name(actual),
				      state_name(expected));
		}
	}

	zassert_equal(checks, expected_checks, "unexpected matrix check count");
}

ZTEST(ptp_state_matrix, test_ptp_state_machine_tt_diff_delta)
{
	zassert_equal(ptp_state_machine(PTP_PS_TIME_RECEIVER, PTP_EVT_RS_TIME_RECEIVER, false),
		      PTP_PS_TIME_RECEIVER, "tt_diff=false should keep TIME_RECEIVER");
	zassert_equal(ptp_state_machine(PTP_PS_TIME_RECEIVER, PTP_EVT_RS_TIME_RECEIVER, true),
		      PTP_PS_UNCALIBRATED, "tt_diff=true should transition to UNCALIBRATED");
}

ZTEST(ptp_state_matrix, test_ptp_tr_state_machine_tt_diff_delta)
{
	zassert_equal(ptp_tr_state_machine(PTP_PS_TIME_RECEIVER, PTP_EVT_RS_TIME_RECEIVER, false),
		      PTP_PS_TIME_RECEIVER, "tt_diff=false should keep TIME_RECEIVER");
	zassert_equal(ptp_tr_state_machine(PTP_PS_TIME_RECEIVER, PTP_EVT_RS_TIME_RECEIVER, true),
		      PTP_PS_UNCALIBRATED, "tt_diff=true should transition to UNCALIBRATED");
}

ZTEST_SUITE(ptp_state_matrix, NULL, NULL, NULL, NULL, NULL);
