/*
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* State Machine Framework */

#ifndef _SMF_H__
#define _SMF_H__

#ifndef CONFIG_SMF_FLAT
/**
 * @brief Macro to create a hierarchical state that prints a message
 *        on entry to and exit from the state.
 *
 * @param _entry_msg Message printed on state entry
 * @param _exit_msg  Message printed on state exit
 * @param _entry     State entry function
 * @param _run       State run function
 * @param _exit      State exit function
 * @param _parent    State parent object or NULL
 */
#define HSM_CREATE_MSG_STATE( \
	_entry_msg, _exit_msg, _entry, _run, _exit, _parent) \
{ \
	.entry_msg = _entry_msg, \
	.exit_msg  = _exit_msg,  \
	.entry     = _entry,     \
	.run       = _run,       \
	.exit      = _exit,      \
	.parent    = _parent     \
}

/**
 * @brief Macro to create a hierarchical state.
 *
 * @param _entry  State entry function
 * @param _run    State run function
 * @param _exit   State exit function
 * @param _parent State parent object or NULL
 */
#define HSM_CREATE_STATE(_entry, _run, _exit, _parent) \
{ \
	.entry  = _entry, \
	.run    = _run,   \
	.exit   = _exit,  \
	.parent = _parent \
}

#else

/**
 * @brief Macro to create a flat state that prints a message
 *        on entry to and exit from the state.
 *
 * @param _entry_msg Message printed on state entry
 * @param _exit_msg  Message printed on state exit
 * @param _entry     State entry function
 * @param _run       State run function
 * @param _exit      State exit function
 */
#define FSM_CREATE_MSG_STATE(_entry_msg, _exit_msg, _entry, _run, _exit) \
{ \
	.entry_msg = _entry_msg, \
	.exit_msg  = _exit_msg,  \
	.entry     = _entry,     \
	.run       = _run,       \
	.exit      = _exit       \
}

/**
 * @brief Macro to create a flat state.
 *
 * @param _entry  State entry function
 * @param _run  State run function
 * @param _exit  State exit function
 */
#define FSM_CREATE_STATE(_entry, _run, _exit) \
{ \
	.entry = _entry, \
	.run   = _run,   \
	.exit  = _exit   \
}

#endif /* CONFIG_SMF_FLAT */

#define SMF_CTX(o) ((struct smf_ctx *)&o)

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr.h>

/**
 * @brief Function pointer that implements a portion of a state
 *
 * @param obj pointer user defined object
 */
typedef void (*state_execution)(void *obj);

/*
 * General state that can be used in multiple state machines.
 *
 * entry - Optional method that will be run when this state is entered
 * run   - Optional method that will be run repeatedly during state machine
 *			loop
 * exit  - Optional method that will be run when this state exists
 * parent- Optional parent state that contains common entry/run/exit
 *	implementation among various child states.
 *	entry: Parent function executes BEFORE child function.
 *	run:   Parent function executes AFTER child function.
 *	exit:  Parent function executes AFTER child function.
 *
 *	Note: When transitioning between two child states with a shared parent,
 *	that parent's exit and entry functions do not execute.
 */
struct smf_state {
#ifdef CONFIG_SMF_PRINT_MSG
	const char *entry_msg;
	const char *exit_msg;
#endif
	const state_execution entry;
	const state_execution run;
	const state_execution exit;
#ifndef CONFIG_SMF_FLAT
	const struct smf_state *parent;
#endif
};

/* Defines the current context of the state machine. */
struct smf_ctx {
	const struct smf_state *current;
	const struct smf_state *previous;
	intptr_t internal[2];
};

/**
 * @brief Changes a state machines state. This handles exiting the previous
 *        state and entering the target state. A common parent state will not
 *        exited nor be re-entered.
 *
 * @param ctx       State machine context
 * @param new_state State to transition to (NULL is valid and exits all states)
 */
void set_state(struct smf_ctx *ctx, const struct smf_state *new_state);

/**
 * @brief Runs one iteration of a state machine (including any parent states)
 *
 * @param ctx  State machine context
 */
void run_state(struct smf_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif /* _SMF_H_ */
