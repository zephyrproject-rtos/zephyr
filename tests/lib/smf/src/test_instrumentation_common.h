/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Common test instrumentation infrastructure for SMF instrumented tests.
 * Provides action/transition logging hooks used to verify state machine behavior.
 */

#ifndef TEST_INSTRUMENTATION_COMMON_H
#define TEST_INSTRUMENTATION_COMMON_H

#ifndef TEST_MAX_ACTION_RECORDS
#define TEST_MAX_ACTION_RECORDS 64
#endif

#ifndef TEST_MAX_TRANSITION_RECORDS
#define TEST_MAX_TRANSITION_RECORDS 16
#endif

struct action_record {
	const struct smf_state *state;
	enum smf_action_type action;
};

struct transition_record {
	const struct smf_state *source;
	const struct smf_state *dest;
};

static struct action_record action_log[TEST_MAX_ACTION_RECORDS];
static int action_log_count;

static struct transition_record transition_log[TEST_MAX_TRANSITION_RECORDS];
static int transition_log_count;

static int error_log_count;

static void reset_logs(void)
{
	action_log_count = 0;
	transition_log_count = 0;
	error_log_count = 0;
}

static void on_action(struct smf_ctx *ctx, const struct smf_state *state,
		      enum smf_action_type action_type)
{
	(void)ctx;
	if (action_log_count < TEST_MAX_ACTION_RECORDS) {
		action_log[action_log_count].state = state;
		action_log[action_log_count].action = action_type;
		action_log_count++;
	}
}

static void on_transition(struct smf_ctx *ctx, const struct smf_state *source,
			  const struct smf_state *dest)
{
	(void)ctx;
	if (transition_log_count < TEST_MAX_TRANSITION_RECORDS) {
		transition_log[transition_log_count].source = source;
		transition_log[transition_log_count].dest = dest;
		transition_log_count++;
	}
}

static void on_error(struct smf_ctx *ctx, int error_code)
{
	(void)ctx;
	(void)error_code;
	error_log_count++;
}

static const struct smf_hooks test_hooks = {
	.on_transition = on_transition,
	.on_action = on_action,
	.on_error = on_error,
};

#endif /* TEST_INSTRUMENTATION_COMMON_H */
