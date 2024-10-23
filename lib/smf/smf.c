/*
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/smf.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smf);

/**
 * @brief Private structure (to this file) used to track state machine context.
 *        The structure is not used directly, but instead to cast the "internal"
 *        member of the smf_ctx structure.
 */
struct internal_ctx {
	bool new_state: 1;
	bool terminate: 1;
	bool is_exit: 1;
	bool is_run: 1;
	bool handled: 1;
};

#ifdef CONFIG_SMF_ANCESTOR_SUPPORT
static bool share_paren(const struct smf_state *test_state, const struct smf_state *target_state)
{
	for (const struct smf_state *state = test_state; state != NULL; state = state->parent) {
		if (target_state == state) {
			return true;
		}
	}

	return false;
}

static const struct smf_state *get_child_of(const struct smf_state *states,
					    const struct smf_state *parent)
{
	const struct smf_state *tmp = states;

	while (true) {
		if (tmp->parent == parent) {
			return tmp;
		}

		if (tmp->parent == NULL) {
			return NULL;
		}

		tmp = tmp->parent;
	}
}

static const struct smf_state *get_last_of(const struct smf_state *states)
{
	return get_child_of(states, NULL);
}

/**
 * @brief Find the Least Common Ancestor (LCA) of two states
 *
 * @param source transition source
 * @param dest transition destination
 * @return LCA state, or NULL if states have no LCA.
 */
static const struct smf_state *get_lca_of(const struct smf_state *source,
					  const struct smf_state *dest)
{
	for (const struct smf_state *ancestor = source->parent; ancestor != NULL;
	     ancestor = ancestor->parent) {
		if (ancestor == dest) {
			return ancestor->parent;
		} else if (share_paren(dest, ancestor)) {
			return ancestor;
		}
	}

	return NULL;
}

/**
 * @brief Executes all entry actions from the direct child of topmost to the new state
 *
 * @param ctx State machine context
 * @param new_state State we are transitioning to
 * @param topmost State we are entering from. Its entry action is not executed
 * @return true if the state machine should terminate, else false
 */
static bool smf_execute_all_entry_actions(struct smf_ctx *const ctx,
					  const struct smf_state *new_state,
					  const struct smf_state *topmost)
{
	struct internal_ctx *const internal = (void *)&ctx->internal;

	if (new_state == topmost) {
		/* There are no child states, so do nothing */
		return false;
	}

	for (const struct smf_state *to_execute = get_child_of(new_state, topmost);
	     to_execute != NULL && to_execute != new_state;
	     to_execute = get_child_of(new_state, to_execute)) {
		/* Keep track of the executing entry action in case it calls
		 * smf_set_state()
		 */
		ctx->executing = to_execute;
		/* Execute every entry action EXCEPT that of the topmost state */
		if (to_execute->entry) {
			to_execute->entry(ctx);

			/* No need to continue if terminate was set */
			if (internal->terminate) {
				return true;
			}
		}
	}

	/* and execute the new state entry action */
	ctx->executing = new_state;
	if (new_state->entry) {
		new_state->entry(ctx);

		/* No need to continue if terminate was set */
		if (internal->terminate) {
			return true;
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
static bool smf_execute_ancestor_run_actions(struct smf_ctx *ctx)
{
	struct internal_ctx *const internal = (void *)&ctx->internal;
	/* Execute all run actions in reverse order */

	/* Return if the current state terminated */
	if (internal->terminate) {
		return true;
	}

	/* The child state either transitioned or handled it. Either way, stop propagating. */
	if (internal->new_state || internal->handled) {
		internal->new_state = false;
		internal->handled = false;
		return false;
	}

	/* Try to run parent run actions */
	for (const struct smf_state *tmp_state = ctx->current->parent; tmp_state != NULL;
	     tmp_state = tmp_state->parent) {
		/* Keep track of where we are in case an ancestor calls smf_set_state()  */
		ctx->executing = tmp_state;
		/* Execute parent run action */
		if (tmp_state->run) {
			tmp_state->run(ctx);
			/* No need to continue if terminate was set */
			if (internal->terminate) {
				return true;
			}

			/* This state dealt with it. Stop propagating. */
			if (internal->new_state || internal->handled) {
				break;
			}
		}
	}

	internal->new_state = false;
	internal->handled = false;

	/* All done executing the run actions */

	return false;
}

/**
 * @brief Executes all exit actions from ctx->current to the direct child of topmost
 *
 * @param ctx State machine context
 * @param topmost State we are exiting to. Its exit action is not executed
 * @return true if the state machine should terminate, else false
 */
static bool smf_execute_all_exit_actions(struct smf_ctx *const ctx, const struct smf_state *topmost)
{
	struct internal_ctx *const internal = (void *)&ctx->internal;

	for (const struct smf_state *to_execute = ctx->current;
	     to_execute != NULL && to_execute != topmost;
	     to_execute = to_execute->parent) {
		if (to_execute->exit) {
			to_execute->exit(ctx);

			/* No need to continue if terminate was set in the exit action */
			if (internal->terminate) {
				return true;
			}
		}
	}

	return false;
}
#endif /* CONFIG_SMF_ANCESTOR_SUPPORT */

void smf_set_initial(struct smf_ctx *ctx, const struct smf_state *init_state)
{
	struct internal_ctx *const internal = (void *)&ctx->internal;

#ifdef CONFIG_SMF_INITIAL_TRANSITION
	/*
	 * The final target will be the deepest leaf state that
	 * the target contains. Set that as the real target.
	 */
	while (init_state->initial) {
		init_state = init_state->initial;
	}
#endif

	internal->is_exit = false;
	internal->is_run = false;
	internal->terminate = false;
	internal->handled = false;
	internal->new_state = false;
	ctx->current = init_state;
	ctx->previous = NULL;
	ctx->terminate_val = 0;

#ifdef CONFIG_SMF_ANCESTOR_SUPPORT
	ctx->executing = init_state;
	const struct smf_state *topmost = get_last_of(init_state);

	/* Execute topmost state entry action, since smf_execute_all_entry_actions()
	 * doesn't
	 */
	if (topmost->entry) {
		topmost->entry(ctx);
		if (internal->terminate) {
			/* No need to continue if terminate was set */
			return;
		}
	}

	if (smf_execute_all_entry_actions(ctx, init_state, topmost)) {
		/* No need to continue if terminate was set */
		return;
	}
#else
	/* execute entry action if it exists */
	if (init_state->entry) {
		init_state->entry(ctx);
	}
#endif
}

void smf_set_state(struct smf_ctx *const ctx, const struct smf_state *new_state)
{
	struct internal_ctx *const internal = (void *)&ctx->internal;

	if (new_state == NULL) {
		LOG_ERR("new_state cannot be NULL");
		return;
	}

	/*
	 * This function should only be called during the entry or run phase of a state.
	 * Calls from outside the state machine are not allowed and will be ignored.
	 */
	if (internal->is_run) {
		LOG_ERR("Calling %s from outside the state machine", __func__);
		return;
	}
	/*
	 * It does not make sense to call smf_set_state in an exit phase of a state
	 * since we are already in a transition; we would always ignore the
	 * intended state to transition into.
	 */
	if (internal->is_exit) {
		LOG_ERR("Calling %s from exit action", __func__);
		return;
	}

#ifdef CONFIG_SMF_ANCESTOR_SUPPORT
	const struct smf_state *topmost;

	if (share_paren(ctx->executing, new_state)) {
		/* new state is a parent of where we are now*/
		topmost = new_state;
	} else if (share_paren(new_state, ctx->executing)) {
		/* we are a parent of the new state */
		topmost = ctx->executing;
	} else {
		/* not directly related, find LCA */
		topmost = get_lca_of(ctx->executing, new_state);
	}

	internal->is_exit = true;
	internal->new_state = true;

	/* call all exit actions up to (but not including) the topmost */
	if (smf_execute_all_exit_actions(ctx, topmost)) {
		/* No need to continue if terminate was set in the exit action */
		return;
	}

	/* if self-transition, call the exit action */
	if ((ctx->executing == new_state) && (new_state->exit)) {
		new_state->exit(ctx);

		/* No need to continue if terminate was set in the exit action */
		if (internal->terminate) {
			return;
		}
	}

	internal->is_exit = false;

	/* if self transition, call the entry action */
	if ((ctx->executing == new_state) && (new_state->entry)) {
		new_state->entry(ctx);

		/* No need to continue if terminate was set in the entry action */
		if (internal->terminate) {
			return;
		}
	}
#ifdef CONFIG_SMF_INITIAL_TRANSITION
	/*
	 * The final target will be the deepest leaf state that
	 * the target contains. Set that as the real target.
	 */
	while (new_state->initial) {
		new_state = new_state->initial;
	}
#endif

	/* update the state variables */
	ctx->previous = ctx->current;
	ctx->current = new_state;

	/* call all entry actions (except those of topmost) */
	if (smf_execute_all_entry_actions(ctx, new_state, topmost)) {
		/* No need to continue if terminate was set in the entry action */
		return;
	}
#else
	/* Flat state machines have a very simple transition: */
	if (ctx->current->exit) {
		internal->is_exit = true;
		ctx->current->exit(ctx);
		/* No need to continue if terminate was set in the exit action */
		if (internal->terminate) {
			return;
		}
		internal->is_exit = false;
	}
	/* update the state variables */
	ctx->previous = ctx->current;
	ctx->current = new_state;

	if (ctx->current->entry) {
		ctx->current->entry(ctx);
		/* No need to continue if terminate was set in the entry action */
		if (internal->terminate) {
			return;
		}
	}
#endif
}

void smf_set_terminate(struct smf_ctx *ctx, int32_t val)
{
	struct internal_ctx *const internal = (void *)&ctx->internal;

	/*
	 * This function should only be called during the entry or run phase of a state.
	 * Calls from outside the state machine are not allowed and will be ignored.
	 */
	if (internal->is_run) {
		LOG_ERR("Calling %s from outside the state machine", __func__);
		return;
	}

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
	struct internal_ctx *const internal = (void *)&ctx->internal;

	/* No need to continue if terminate was set */
	if (internal->terminate) {
		return ctx->terminate_val;
	}

#ifdef CONFIG_SMF_ANCESTOR_SUPPORT
	ctx->executing = ctx->current;
#endif
	internal->is_run = true;

	if (ctx->current->run) {
		ctx->current->run(ctx);
	}

#ifdef CONFIG_SMF_ANCESTOR_SUPPORT
	if (smf_execute_ancestor_run_actions(ctx)) {
		internal->is_run = false;
		return ctx->terminate_val;
	}
#endif
	internal->is_run = false;
	return 0;
}
