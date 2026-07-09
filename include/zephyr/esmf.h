/*
 * Copyright (c) 2026 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Event-driven State Machine Framework API built on top of SMF.
 */

#ifndef ZEPHYR_INCLUDE_ESMF_H_
#define ZEPHYR_INCLUDE_ESMF_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/smf.h>

/**
 * @brief Event-driven State Machine Framework (ESMF) APIs
 * @defgroup esmf Event-driven State Machine Framework (ESMF)
 * @ingroup os_services
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Macro to cast a user-defined object to ESMF context.
 *
 * The user-defined object must have :c:struct:`esmf_ctx` as its first member.
 *
 * @param o Pointer to user-defined object.
 */
#define ESMF_CTX(o) ((struct esmf_ctx *)o)

/**
 * @brief Macro to create an ESMF transition rule.
 *
 * @param _from     Source state or NULL for any state.
 * @param _trigger Event identifier or @ref ESMF_TRIGGER_ANY.
 * @param _guard    Guard callback or NULL.
 * @param _action   Action callback or NULL.
 * @param _to       Destination state or NULL for an internal transition.
 */
/* clang-format off */
#define ESMF_CREATE_TRANSITION(_from, _trigger, _guard, _action, _to) \
	ESMF_CREATE_TRANSITION_PRIO(_from, _trigger, _guard, _action, _to, 0)
/* clang-format on */

/**
 * @brief Macro to create an ESMF transition rule.
 *
 * @param _from     Source state or NULL for any state.
 * @param _trigger Event identifier or @ref ESMF_TRIGGER_ANY.
 * @param _guard    Guard callback or NULL.
 * @param _action   Action callback or NULL.
 * @param _to       Destination state or NULL for an internal transition.
 */
/* clang-format off */
#define ESMF_CREATE_TRANSITION_PRIO(_from, _trigger, _guard, _action, _to, _prio) \
{                                                                             \
	.from = _from,                                                            \
	.trigger = _trigger,                                                    \
	.guard = _guard,                                                          \
	.action = _action,                                                        \
	.to = _to,                                                                \
	.priority = _prio,                                                        \
}
/* clang-format on */

/**
 * @brief Wildcard event value that matches any event in a transition rule.
 */
#define ESMF_TRIGGER_ANY UINT32_MAX

struct esmf_ctx;

/**
 * @brief Guard callback signature for event transitions.
 *
 * @param ctx      ESMF context.
 * @param trigger Incoming event identifier.
 *
 * @retval true  Transition may be used.
 * @retval false Transition is rejected.
 */
typedef bool (*esmf_guard_t)(struct esmf_ctx *ctx, uint32_t trigger);

/**
 * @brief Action callback signature for event transitions.
 *
 * The action is executed before transitioning to @p to if @p to is non-NULL.
 *
 * @param ctx      ESMF context.
 * @param trigger Incoming event identifier.
 */
typedef void (*esmf_action_t)(struct esmf_ctx *ctx, uint32_t trigger);

/**
 * @brief A single transition rule in the event transition table.
 */
struct esmf_transition {
	/**
	 * Source state to match.
	 *
	 * If NULL, the rule matches from any current state.
	 */
	const struct smf_state *from;

	/**
	 * Event identifier to match, or @ref ESMF_TRIGGER_ANY.
	 */
	uint32_t trigger;

	/**
	 * Optional guard callback. If NULL, the transition is always allowed.
	 */
	esmf_guard_t guard;

	/**
	 * Optional action callback executed before the transition.
	 */
	esmf_action_t action;

	/**
	 * Destination state.
	 *
	 * If NULL, no state transition occurs (internal transition).
	 *
	 * If equal to the current state (self-transition), :c:func:`smf_set_state`
	 * is still called, so the state's exit and entry actions are re-executed,
	 * matching SMF self-transition semantics.
	 */
	const struct smf_state *to;

	/**
	 * Priority of the transition rule.
	 * Lower values have higher priority. If multiple rules match, the one with
	 * the highest priority is selected.
	 */
	int priority;
};

/**
 * @brief ESMF runtime context.
 */
struct esmf_ctx {
	/**
	 * SMF context controlled by this event table.
	 *
	 * This member must be first so :c:macro:`SMF_CTX` and :c:macro:`ESMF_CTX`
	 * can be used with the same user-defined object.
	 */
	struct smf_ctx smf;

	/** Transition table. */
	const struct esmf_transition *transitions;

	/** Number of entries in @ref transitions. */
	size_t transition_count;
};

/**
 * @brief Initialize an ESMF context.
 *
 * @param ctx              ESMF context.
 * @param transitions      Transition table.
 * @param transition_count Number of entries in @p transitions.
 */
void esmf_init(struct esmf_ctx *ctx, const struct esmf_transition *transitions,
	       size_t transition_count);

/**
 * @brief Handle one event against the transition table.
 *
 * Among matching and guard-approved rules, lower @ref esmf_transition.priority
 * values win. Rules with equal priority keep table order (first found wins).
 *
 * For external transitions (``to != NULL``), SMF entry/exit actions execute
 * according to :c:func:`smf_set_state` semantics.
 *
 * :c:func:`esmf_handle_event` does not execute SMF run actions. SMF run actions
 * should be ``NULL`` when using pure event-driven dispatch, or handled
 * separately outside ESMF event handling.
 *
 * @param ctx      ESMF context.
 * @param trigger Incoming event ID.
 *
 * @retval 0       Event handled (with or without state transition).
 * @retval -EINVAL Invalid arguments.
 * @retval -ENOENT No matching transition rule was found.
 * @retval -EACCES At least one rule matched, but all were blocked by guards.
 */
int esmf_handle_event(struct esmf_ctx *ctx, uint32_t trigger);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_ESMF_H_ */
