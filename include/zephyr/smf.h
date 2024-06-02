/*
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief State Machine Framework header file
 */

#ifndef ZEPHYR_INCLUDE_SMF_H_
#define ZEPHYR_INCLUDE_SMF_H_

#include <zephyr/sys/util.h>

/**
 * @brief State Machine Framework API
 * @defgroup smf State Machine Framework API
 * @version 0.1.0
 * @ingroup os_services
 * @{
 */

/**
 * @brief Macro to create a hierarchical state with initial transitions.
 *
 * @param _entry   State entry function or NULL
 * @param _run     State run function or NULL
 * @param _exit    State exit function or NULL
 * @param _parent  State parent object or NULL
 * @param _initial State initial transition object or NULL
 */
#define SMF_CREATE_STATE(_entry, _run, _exit, _parent, _initial)           \
{                                                                          \
	.entry   = _entry,                                                 \
	.run     = _run,                                                   \
	.exit    = _exit,                                                  \
	IF_ENABLED(CONFIG_SMF_ANCESTOR_SUPPORT, (.parent = _parent,))      \
	IF_ENABLED(CONFIG_SMF_INITIAL_TRANSITION, (.initial = _initial,))  \
}

/**
 * @brief Macro to cast user defined object to state machine
 *        context.
 *
 * @param o A pointer to the user defined object
 */
#define SMF_CTX(o) ((struct smf_ctx *)o)

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>

/**
 * @brief Function pointer that implements a portion of a state
 *
 * @param obj pointer user defined object
 */
typedef void (*state_execution)(void *obj);

/** General state that can be used in multiple state machines. */
struct smf_state {
	/** Optional method that will be run when this state is entered */
	const state_execution entry;
	/**
	 * Optional method that will be run repeatedly during state machine
	 * loop.
	 */
	const state_execution run;
	/** Optional method that will be run when this state exists */
	const state_execution exit;
#ifdef CONFIG_SMF_ANCESTOR_SUPPORT
	/**
	 * Optional parent state that contains common entry/run/exit
	 *	implementation among various child states.
	 *	entry: Parent function executes BEFORE child function.
	 *	run:   Parent function executes AFTER child function.
	 *	exit:  Parent function executes AFTER child function.
	 *
	 *	Note: When transitioning between two child states with a shared
	 *      parent,	that parent's exit and entry functions do not execute.
	 */
	const struct smf_state *parent;

#ifdef CONFIG_SMF_INITIAL_TRANSITION
	/**
	 * Optional initial transition state. NULL for leaf states.
	 */
	const struct smf_state *initial;
#endif /* CONFIG_SMF_INITIAL_TRANSITION */
#endif /* CONFIG_SMF_ANCESTOR_SUPPORT */
};

/** Defines the current context of the state machine. */
struct smf_ctx {
	/** Current state the state machine is executing. */
	const struct smf_state *current;
	/** Previous state the state machine executed */
	const struct smf_state *previous;

#ifdef CONFIG_SMF_ANCESTOR_SUPPORT
	/** Currently executing state (which may be a parent) */
	const struct smf_state *executing;
#endif /* CONFIG_SMF_ANCESTOR_SUPPORT */
	/**
	 * This value is set by the set_terminate function and
	 * should terminate the state machine when its set to a
	 * value other than zero when it's returned by the
	 * run_state function.
	 */
	int32_t terminate_val;
	/**
	 * The state machine casts this to a "struct internal_ctx" and it's
	 * used to track state machine context
	 */
	uint32_t internal;
};

/**
 * @brief Initializes the state machine and sets its initial state.
 *
 * @param ctx        State machine context
 * @param init_state Initial state the state machine starts in.
 */
void smf_set_initial(struct smf_ctx *ctx, const struct smf_state *init_state);

/**
 * @brief Changes a state machines state. This handles exiting the previous
 *        state and entering the target state. For HSMs the entry and exit
 *        actions of the Least Common Ancestor will not be run.
 *
 * @param ctx       State machine context
 * @param new_state State to transition to (NULL is valid and exits all states)
 */
void smf_set_state(struct smf_ctx *ctx, const struct smf_state *new_state);

/**
 * @brief Terminate a state machine
 *
 * @param ctx  State machine context
 * @param val  Non-Zero termination value that's returned by the smf_run_state
 *             function.
 */
void smf_set_terminate(struct smf_ctx *ctx, int32_t val);

/**
 * @brief Tell the SMF to stop propagating the event to ancestors. This allows
 *        HSMs to implement 'programming by difference' where substates can
 *        handle events on their own or propagate up to a common handler.
 *
 * @param ctx  State machine context
 */
void smf_set_handled(struct smf_ctx *ctx);

/**
 * @brief Runs one iteration of a state machine (including any parent states)
 *
 * @param ctx  State machine context
 * @return	   A non-zero value should terminate the state machine. This
 *			   non-zero value could represent a terminal state being reached
 *			   or the detection of an error that should result in the
 *			   termination of the state machine.
 */
int32_t smf_run_state(struct smf_ctx *ctx);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_SMF_H_ */
