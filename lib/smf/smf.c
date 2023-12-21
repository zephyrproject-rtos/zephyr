/*
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/smf.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smf);

/*
 * Private structure (to this file) used to track state machine context.
 * The structure is not used directly, but instead to cast the "internal"
 * member of the smf_ctx structure.
 */
struct internal_ctx {
	bool new_state : 1;
	bool terminate : 1;
	bool exit      : 1;
	bool handled   : 1;
};

static bool share_paren(const struct smf_state *test_state,
			const struct smf_state *target_state)
{
	for (const struct smf_state *state = test_state;
	     state != NULL;
	     state = state->parent) {
		if (target_state == state) {
			return true;
		}
	}

	return false;
}

static bool last_state_share_paren(struct smf_ctx *const ctx,
				   const struct smf_state *state)
{
	/* Get parent state of previous state */
	if (!ctx->previous) {
		return false;
	}

	return share_paren(ctx->previous->parent, state);
}

static const struct smf_state *get_child_of(const struct smf_state *states,
					    const struct smf_state *parent)
{
	for (const struct smf_state *tmp = states; ; tmp = tmp->parent) {
		if (tmp->parent == parent) {
			return tmp;
		}

		if (tmp->parent == NULL) {
			return NULL;
		}
	}

	return NULL;
}

static const struct smf_state *get_last_of(const struct smf_state *states)
{
	return get_child_of(states, NULL);
}

/**
 * @brief Execute all ancestor entry actions
 *
 * @param ctx State machine context
 * @param target The entry actions of this target's ancestors are executed
 * @return true if the state machine should terminate, else false
 */
__unused static bool smf_execute_ancestor_entry_actions(
		struct smf_ctx *const ctx, const struct smf_state *target)
{
	struct internal_ctx * const internal = (void *) &ctx->internal;

	for (const struct smf_state *to_execute = get_last_of(target);
	     to_execute != NULL && to_execute != target;
	     to_execute = get_child_of(target, to_execute)) {
		/* Execute parent state's entry */
		if (!last_state_share_paren(ctx, to_execute) && to_execute->entry) {
			to_execute->entry(ctx);

			/* No need to continue if terminate was set */
			if (internal->terminate) {
				return true;
			}
		}
	}

	return false;
}

/**
 * @brief Execute all ancestor run actions
 *
 * @param ctx State machine context
 * @param target The run actions of this target's ancestors are executed
 * @return true if the state machine should terminate, else false
 */
__unused static bool smf_execute_ancestor_run_actions(struct smf_ctx *ctx)
{
	struct internal_ctx * const internal = (void *) &ctx->internal;
	/* Execute all run actions in reverse order */

	/* Return if the current state switched states */
	if (internal->new_state) {
		internal->new_state = false;
		return false;
	}

	/* Return if the current state terminated */
	if (internal->terminate) {
		return true;
	}

	if (internal->handled) {
		/* Event was handled by this state. Stop propagating */
		internal->handled = false;
		return false;
	}

	/* Try to run parent run actions */
	for (const struct smf_state *tmp_state = ctx->current->parent;
	     tmp_state != NULL;
	     tmp_state = tmp_state->parent) {
		/* Execute parent run action */
		if (tmp_state->run) {
			tmp_state->run(ctx);
			/* No need to continue if terminate was set */
			if (internal->terminate) {
				return true;
			}

			if (internal->new_state) {
				break;
			}

			if (internal->handled) {
				/* Event was handled by this state. Stop propagating */
				internal->handled = false;
				break;
			}
		}
	}

	internal->new_state = false;
	/* All done executing the run actions */

	return false;
}

/**
 * @brief Execute all ancestor exit actions
 *
 * @param ctx State machine context
 * @param target The exit actions of this target's ancestors are executed
 * @return true if the state machine should terminate, else false
 */
__unused static bool smf_execute_ancestor_exit_actions(
		struct smf_ctx *const ctx, const struct smf_state *target)
{
	struct internal_ctx * const internal = (void *) &ctx->internal;

	/* Execute all parent exit actions in reverse order */

	for (const struct smf_state *tmp_state = ctx->current->parent;
	     tmp_state != NULL;
	     tmp_state = tmp_state->parent) {
		if (!share_paren(target->parent, tmp_state) && tmp_state->exit) {
			tmp_state->exit(ctx);

			/* No need to continue if terminate was set */
			if (internal->terminate) {
				return true;
			}
		}
	}
	return false;
}

void smf_set_initial(struct smf_ctx *ctx, const struct smf_state *init_state)
{
	struct internal_ctx * const internal = (void *) &ctx->internal;


#ifdef CONFIG_SMF_INITIAL_TRANSITION
	/*
	 * The final target will be the deepest leaf state that
	 * the target contains. Set that as the real target.
	 */
	while (init_state->initial) {
		init_state = init_state->initial;
	}
#endif
	internal->exit = false;
	internal->terminate = false;
	ctx->current = init_state;
	ctx->previous = NULL;
	ctx->terminate_val = 0;

	if (IS_ENABLED(CONFIG_SMF_ANCESTOR_SUPPORT)) {
		internal->new_state = false;

		if (smf_execute_ancestor_entry_actions(ctx, init_state)) {
			return;
		}
	}

	/* Now execute the initial state's entry action */
	if (init_state->entry) {
		init_state->entry(ctx);
	}
}

void smf_set_state(struct smf_ctx *const ctx, const struct smf_state *target)
{
	struct internal_ctx * const internal = (void *) &ctx->internal;

	/*
	 * It does not make sense to call set_state in an exit phase of a state
	 * since we are already in a transition; we would always ignore the
	 * intended state to transition into.
	 */
	if (internal->exit) {
		LOG_WRN("Calling %s from exit action", __func__);
		return;
	}

	internal->exit = true;

	/* Execute the current states exit action */
	if (ctx->current->exit) {
		ctx->current->exit(ctx);

		/*
		 * No need to continue if terminate was set in the
		 * exit action
		 */
		if (internal->terminate) {
			return;
		}
	}

	if (IS_ENABLED(CONFIG_SMF_ANCESTOR_SUPPORT)) {
		internal->new_state = true;

		if (smf_execute_ancestor_exit_actions(ctx, target)) {
			return;
		}
	}

	internal->exit = false;

#ifdef CONFIG_SMF_INITIAL_TRANSITION
	/*
	 * The final target will be the deepest leaf state that
	 * the target contains. Set that as the real target.
	 */
	while (target->initial) {
		target = target->initial;
	}
#endif

	/* update the state variables */
	ctx->previous = ctx->current;
	ctx->current = target;

	if (IS_ENABLED(CONFIG_SMF_ANCESTOR_SUPPORT)) {
		if (smf_execute_ancestor_entry_actions(ctx, target)) {
			return;
		}
	}

	/* Now execute the target entry action */
	if (ctx->current->entry) {
		ctx->current->entry(ctx);
		/*
		 * If terminate was set, it will be handled in the
		 * smf_run_state function
		 */
	}
}

void smf_set_terminate(struct smf_ctx *ctx, int32_t val)
{
	struct internal_ctx * const internal = (void *) &ctx->internal;

	internal->terminate = true;
	ctx->terminate_val = val;
}

void smf_set_handled(struct smf_ctx *ctx)
{
	struct internal_ctx *const internal = (void *)&ctx->internal;

	internal->handled = true;
}

int32_t smf_run_state(struct smf_ctx *const ctx)
{
	struct internal_ctx * const internal = (void *) &ctx->internal;

	/* No need to continue if terminate was set */
	if (internal->terminate) {
		return ctx->terminate_val;
	}

	if (ctx->current->run) {
		ctx->current->run(ctx);
	}

	if (IS_ENABLED(CONFIG_SMF_ANCESTOR_SUPPORT)) {
		if (smf_execute_ancestor_run_actions(ctx)) {
			return ctx->terminate_val;
		}
	}

	return 0;
}
