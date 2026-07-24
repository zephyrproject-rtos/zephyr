/*
 * Copyright (c) 2026 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/esmf.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "traffic_events.h"

LOG_MODULE_REGISTER(esmf_traffic_light, LOG_LEVEL_INF);

#define GREEN_MIN_TICKS  2
#define EVENT_QUEUE_SIZE 16

enum traffic_state_id {
	STATE_OPERATIONAL,
	STATE_RED,
	STATE_GREEN,
	STATE_YELLOW,
	STATE_FLASHING_RED,
	STATE_COUNT,
};

struct traffic_ctx {
	struct esmf_ctx esmf;
	atomic_t ticks_in_green;
	atomic_t emergency_active;
};

struct traffic_event {
	uint32_t trigger;
	const char *name;
};

K_MSGQ_DEFINE(event_msgq, sizeof(struct traffic_event), EVENT_QUEUE_SIZE, sizeof(void *));

static struct traffic_ctx traffic;
static const struct smf_state traffic_states[STATE_COUNT];

static const char *state_name(const struct smf_state *state)
{
	if (state == &traffic_states[STATE_OPERATIONAL]) {
		return "OPERATIONAL";
	}
	if (state == &traffic_states[STATE_RED]) {
		return "RED";
	}
	if (state == &traffic_states[STATE_GREEN]) {
		return "GREEN";
	}
	if (state == &traffic_states[STATE_YELLOW]) {
		return "YELLOW";
	}
	if (state == &traffic_states[STATE_FLASHING_RED]) {
		return "FLASHING_RED";
	}

	return "UNKNOWN";
}

static void log_current_state(struct traffic_ctx *ctx, const char *reason)
{
	const struct smf_state *current = smf_get_current_leaf_state(&ctx->esmf.smf);

	LOG_INF("state=%s (%s)", state_name(current), reason);
}

static void red_entry(void *obj)
{
	struct traffic_ctx *ctx = obj;

	ARG_UNUSED(ctx);
	LOG_INF("ENTER RED");
}

static void operational_entry(void *obj)
{
	struct traffic_ctx *ctx = obj;

	ARG_UNUSED(ctx);
	LOG_INF("ENTER OPERATIONAL");
}

static void green_entry(void *obj)
{
	struct traffic_ctx *ctx = obj;

	atomic_set(&ctx->ticks_in_green, 0);
	LOG_INF("ENTER GREEN");
}

static void yellow_entry(void *obj)
{
	struct traffic_ctx *ctx = obj;

	ARG_UNUSED(ctx);
	LOG_INF("ENTER YELLOW");
}

static void flashing_red_entry(void *obj)
{
	struct traffic_ctx *ctx = obj;

	ARG_UNUSED(ctx);
	LOG_INF("ENTER FLASHING_RED");
}

static bool ped_guard(struct esmf_ctx *esmf, uint32_t trigger)
{
	struct traffic_ctx *ctx = (struct traffic_ctx *)esmf;

	ARG_UNUSED(trigger);
	return atomic_get(&ctx->ticks_in_green) >= GREEN_MIN_TICKS;
}

static void diag_action(struct esmf_ctx *esmf, uint32_t trigger)
{
	struct traffic_ctx *ctx = (struct traffic_ctx *)esmf;

	ARG_UNUSED(trigger);
	LOG_INF("diag: state=%s, green_ticks=%d, emergency=%d",
		state_name(smf_get_current_leaf_state(&ctx->esmf.smf)),
		(int)atomic_get(&ctx->ticks_in_green), (int)atomic_get(&ctx->emergency_active));
}

static void emergency_action(struct esmf_ctx *esmf, uint32_t trigger)
{
	struct traffic_ctx *ctx = (struct traffic_ctx *)esmf;

	ARG_UNUSED(trigger);
	atomic_set(&ctx->emergency_active, 1);
	LOG_INF("EMERGENCY override accepted");
}

static void clear_emergency_action(struct esmf_ctx *esmf, uint32_t trigger)
{
	struct traffic_ctx *ctx = (struct traffic_ctx *)esmf;

	ARG_UNUSED(trigger);
	atomic_set(&ctx->emergency_active, 0);
	LOG_INF("Emergency cleared");
}

static void tick_action(struct esmf_ctx *esmf, uint32_t trigger)
{
	struct traffic_ctx *ctx = (struct traffic_ctx *)esmf;

	ARG_UNUSED(trigger);
	atomic_inc(&ctx->ticks_in_green);
}

static const struct esmf_transition transitions[] = {
	/*
	 * Emergency transitions have lowest numeric priority and therefore win
	 * over state-specific rules even though they appear later in the table.
	 */
	ESMF_CREATE_TRANSITION(&traffic_states[STATE_RED], TRIG_TIMER, NULL, NULL,
			       &traffic_states[STATE_GREEN]),
	ESMF_CREATE_TRANSITION(&traffic_states[STATE_GREEN], TRIG_TIMER, NULL, NULL,
			       &traffic_states[STATE_YELLOW]),
	ESMF_CREATE_TRANSITION(&traffic_states[STATE_YELLOW], TRIG_TIMER, NULL, NULL,
			       &traffic_states[STATE_RED]),
	ESMF_CREATE_TRANSITION(&traffic_states[STATE_GREEN], TRIG_TICK, NULL, tick_action, NULL),
	ESMF_CREATE_TRANSITION(&traffic_states[STATE_GREEN], TRIG_PED_BUTTON, ped_guard, NULL,
			       &traffic_states[STATE_YELLOW]),
	ESMF_CREATE_TRANSITION(&traffic_states[STATE_OPERATIONAL], TRIG_DIAG_TICK, NULL,
			       diag_action, NULL),
	ESMF_CREATE_TRANSITION(&traffic_states[STATE_FLASHING_RED], TRIG_DIAG_TICK, NULL,
			       diag_action, NULL),
	ESMF_CREATE_TRANSITION_PRIO(&traffic_states[STATE_OPERATIONAL], TRIG_EMERGENCY, NULL,
				    emergency_action, &traffic_states[STATE_FLASHING_RED], -10),
	ESMF_CREATE_TRANSITION_PRIO(&traffic_states[STATE_FLASHING_RED], TRIG_EMERGENCY, NULL,
				    emergency_action, &traffic_states[STATE_FLASHING_RED], -10),
	ESMF_CREATE_TRANSITION_PRIO(&traffic_states[STATE_FLASHING_RED], TRIG_RESET, NULL,
				    clear_emergency_action, &traffic_states[STATE_RED], 0),
	/* Equal-priority tie demo: first matching transition wins. */
	ESMF_CREATE_TRANSITION_PRIO(&traffic_states[STATE_RED], TRIG_RESET, NULL, diag_action,
				    &traffic_states[STATE_RED], 20),
	ESMF_CREATE_TRANSITION_PRIO(&traffic_states[STATE_RED], TRIG_RESET, NULL, NULL,
				    &traffic_states[STATE_GREEN], 20),
};

static const struct smf_state traffic_states[STATE_COUNT] = {
	[STATE_OPERATIONAL] = SMF_CREATE_STATE(operational_entry, NULL, NULL, NULL, NULL),
	[STATE_RED] =
		SMF_CREATE_STATE(red_entry, NULL, NULL, &traffic_states[STATE_OPERATIONAL], NULL),
	[STATE_GREEN] =
		SMF_CREATE_STATE(green_entry, NULL, NULL, &traffic_states[STATE_OPERATIONAL], NULL),
	[STATE_YELLOW] = SMF_CREATE_STATE(yellow_entry, NULL, NULL,
					  &traffic_states[STATE_OPERATIONAL], NULL),
	[STATE_FLASHING_RED] = SMF_CREATE_STATE(flashing_red_entry, NULL, NULL, NULL, NULL),
};

static int dispatch(struct traffic_ctx *ctx, uint32_t trigger, const char *name)
{
	int rc = esmf_handle_event(&ctx->esmf, trigger);

	if (rc == 0) {
		log_current_state(ctx, name);
		return 0;
	}

	if (rc == -EACCES) {
		LOG_INF("trigger=%s rejected by guard", name);
		return rc;
	}

	LOG_INF("trigger=%s not handled (rc=%d)", name, rc);
	return rc;
}

int traffic_post_event(uint32_t trigger, const char *name, k_timeout_t timeout)
{
	struct traffic_event event = {
		.trigger = trigger,
		.name = name,
	};

	return k_msgq_put(&event_msgq, &event, timeout);
}

int main(void)
{
	struct traffic_event event;

	LOG_INF("ESMF Traffic Light Demo");
	LOG_INF("Use shell: traffic event <timer|ped|emergency|diag|reset>");

	atomic_set(&traffic.ticks_in_green, 0);
	atomic_set(&traffic.emergency_active, 0);

	esmf_init(&traffic.esmf, transitions, ARRAY_SIZE(transitions));
	smf_set_initial(SMF_CTX(&traffic), &traffic_states[STATE_RED]);
	log_current_state(&traffic, "initial");

	while (k_msgq_get(&event_msgq, &event, K_FOREVER) == 0) {
		dispatch(&traffic, event.trigger, event.name);
	}

	return 0;
}
