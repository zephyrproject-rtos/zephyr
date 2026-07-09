/*
 * Copyright (c) 2026 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/esmf.h>

#include <errno.h>

static bool transition_matches(const struct smf_state *current,
			       const struct esmf_transition *transition, uint32_t trigger)
{
	if (transition->trigger != ESMF_TRIGGER_ANY && transition->trigger != trigger) {
		return false;
	}

	if (transition->from == NULL || transition->from == current) {
		return true;
	}

#ifdef CONFIG_SMF_ANCESTOR_SUPPORT
	for (const struct smf_state *state = current->parent; state != NULL;
	     state = state->parent) {
		if (state == transition->from) {
			return true;
		}
	}
#endif

	return false;
}

void esmf_init(struct esmf_ctx *ctx, const struct esmf_transition *transitions,
	       size_t transition_count)
{
	ctx->transitions = transitions;
	ctx->transition_count = transition_count;
}

int esmf_handle_event(struct esmf_ctx *ctx, uint32_t trigger)
{
	bool guard_rejected = false;
	const struct smf_state *current;
	const struct esmf_transition *selected = NULL;

	if (ctx == NULL || ctx->transitions == NULL || ctx->transition_count == 0U) {
		return -EINVAL;
	}

	current = smf_get_current_leaf_state(&ctx->smf);

	if (current == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0U; i < ctx->transition_count; i++) {
		const struct esmf_transition *transition = &ctx->transitions[i];

		if (!transition_matches(current, transition, trigger)) {
			continue;
		}

		if (transition->guard != NULL && !transition->guard(ctx, trigger)) {
			guard_rejected = true;
			continue;
		}

		if (selected == NULL || transition->priority < selected->priority) {
			selected = transition;
		}
	}

	if (selected == NULL) {
		return guard_rejected ? -EACCES : -ENOENT;
	}

	if (selected->action != NULL) {
		selected->action(ctx, trigger);
	}

	if (selected->to != NULL) {
		smf_set_state(&ctx->smf, selected->to);
	}

	return 0;
}
