/*
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "smf.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(smf);

/*
 * Private structure (to this file) used to track state machine context.
 * The structure is not used directly, but instead to cast the "internal"
 * member of the smf_ctx structure.
 */
struct internal_ctx {
	const struct smf_state *last_entered;
	uint32_t running : 1;
	uint32_t enter	 : 1;
	uint32_t exit	 : 1;
};

#ifndef CONFIG_SMF_FLAT
/* Gets the first shared parent state between a and b (inclusive) */
static const struct smf_state *shared_parent_state(const struct smf_state *a,
				const struct smf_state *b)
{
	const struct smf_state *orig_b = b;

	/* There are no common ancestors */
	if (b == NULL)
		return NULL;

	/* This assumes that both A and B are NULL terminated without cycles */
	while (a != NULL) {
		/* We found a match return */
		if (a == b)
			return a;

		/*
		 * Otherwise, increment b down the list for comparison until we
		 * run out, then increment a and start over on b for comparison
		 */
		if (b->parent == NULL) {
			a = a->parent;
			b = orig_b;
		} else {
			b = b->parent;
		}
	}

	return NULL;
}
#endif

/*
 * Call all entry functions of parents before children. If set_state is called
 * during one of the entry functions, then do not call any remaining entry
 * functions.
 */
static void call_entry_functions(struct smf_ctx *const ctx,
					struct internal_ctx *const internal,
			       const struct smf_state *stop,
			       const struct smf_state *current)
{
	if (current == stop) {
		return;
	}

#ifndef CONFIG_SMF_FLAT
	call_entry_functions(ctx, internal, stop, current->parent);
#else
	call_entry_functions(ctx, internal, stop, NULL);
#endif

	/*
	 * If the previous entry function called set_state, then don't enter
	 * remaining states.
	 */
	if (!internal->enter) {
		return;
	}

	/* Track the latest state that was entered, so we can exit properly. */
	internal->last_entered = current;
	if (current->entry) {
#ifdef CONFIG_SMF_PRINT_MSG
		if (current->entry_msg) {
			printk("%s\n", current->entry_msg);
		}
#endif
		current->entry((void *)ctx);
	}
}

/*
 * Call all exit functions of children before parents. Note set_state is ignored
 * during an exit function.
 */
static void call_exit_functions(struct smf_ctx *const ctx,
			const struct smf_state *stop,
			const struct smf_state *current)
{
	if (current == stop)
		return;

	if (current->exit) {
#ifdef CONFIG_SMF_PRINT_MSG
		if (current->exit_msg) {
			printk("%s\n", current->exit_msg);
		}
#endif
		current->exit((void *)ctx);
	}

#ifndef CONFIG_SMF_FLAT
	call_exit_functions(ctx, stop, current->parent);
#else
	call_exit_functions(ctx, stop, NULL);
#endif
}

void set_state(struct smf_ctx *const ctx,
	       const struct smf_state *new_state)
{
	struct internal_ctx * const internal = (void *) ctx->internal;
	const struct smf_state *last_state;
	const struct smf_state *shared_parent;

	/*
	 * It does not make sense to call set_state in an exit phase of a state
	 * since we are already in a transition; we would always ignore the
	 * intended state to transition into.
	 */
	if (internal->exit) {
		LOG_WRN("Calling %s from exit action", __func__);
		return;
	}

	/*
	 * Determine the last state that was entered. Normally it is current,
	 * but we could have called set_state within an entry phase, so we
	 * shouldn't exit any states that weren't fully entered.
	 */
	last_state = internal->enter ? internal->last_entered : ctx->current;
#ifndef CONFIG_SMF_FLAT
	/* We don't exit and re-enter shared parent states */
	shared_parent = shared_parent_state(last_state, new_state);
#else
	shared_parent = NULL;
#endif
	/*
	 * Exit all of the non-common states from the last state.
	 */
	internal->exit = true;
	call_exit_functions(ctx, shared_parent, last_state);
	internal->exit = false;

	ctx->previous = ctx->current;
	ctx->current = new_state;

	/*
	 * Enter all new non-common states. last_entered will contain the last
	 * state that successfully entered before another set_state was called.
	 */
	internal->last_entered = NULL;
	internal->enter = true;
	call_entry_functions(ctx, internal, shared_parent, ctx->current);
	/*
	 * Setting enter to false ensures that all pending entry calls will be
	 * skipped (in the case of a parent state calling set_state, which means
	 * we should not enter any child states)
	 */
	internal->enter = false;

	/*
	 * If we set_state while we are running a child state, then stop running
	 * any remaining parent states.
	 */
	internal->running = false;
}

/*
 * Call all run functions of children before parents. If set_state is called
 * during one of the entry functions, then do not call any remaining entry
 * functions.
 */
static void call_run_functions(const struct smf_ctx *const ctx,
							const struct internal_ctx *const internal,
							const struct smf_state *current)
{
	if (!current)
		return;

	/* If set_state is called during run, don't call remain functions. */
	if (!internal->running)
		return;

	if (current->run)
		current->run((void *)ctx);

#ifndef CONFIG_SMF_FLAT
	call_run_functions(ctx, internal, current->parent);
#else
	call_run_functions(ctx, internal, NULL);
#endif
}

void run_state(struct smf_ctx *const ctx)
{
	struct internal_ctx * const internal = (void *) ctx->internal;

	internal->running = true;
	call_run_functions(ctx, internal, ctx->current);
	internal->running = false;
}
