/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POWER_QOS_H_
#define ZEPHYR_INCLUDE_POWER_QOS_H_

#include <stdbool.h>

#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

struct pstate;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Power QoS constraints.
 * @defgroup subsys_power_qos Power QoS
 * @since 4.4
 * @version 0.1.0
 * @ingroup os_services
 * @{
 */

/**
 * @brief P-state constraint request.
 *
 * @note All fields in this structure are meant for private usage.
 */
struct power_qos_pstate_request {
	/** @cond INTERNAL_HIDDEN */
	sys_snode_t node;
	const struct pstate *state;
	/** @endcond */
};

/** Effective P-state constraints after request aggregation. */
struct power_qos_pstate_constraints {
	/** Minimum allowed P-state, or NULL if no minimum was requested. */
	const struct pstate *min_pstate;

	/** Maximum allowed P-state, or NULL if no maximum was requested. */
	const struct pstate *max_pstate;

	/** True if the effective minimum is higher than the effective maximum. */
	bool conflict;
};

#ifdef CONFIG_POWER_QOS
/**
 * @brief Add a minimum P-state request.
 *
 * @param req Request object owned by the caller.
 * @param state Minimum allowed P-state.
 */
void power_qos_min_pstate_request_add(struct power_qos_pstate_request *req,
					      const struct pstate *state);

/**
 * @brief Update a minimum P-state request.
 *
 * @param req Request object.
 * @param state New minimum allowed P-state.
 */
void power_qos_min_pstate_request_update(struct power_qos_pstate_request *req,
						 const struct pstate *state);

/**
 * @brief Remove a minimum P-state request.
 *
 * @param req Request object.
 */
void power_qos_min_pstate_request_remove(struct power_qos_pstate_request *req);

/**
 * @brief Add a maximum P-state request.
 *
 * @param req Request object owned by the caller.
 * @param state Maximum allowed P-state.
 */
void power_qos_max_pstate_request_add(struct power_qos_pstate_request *req,
					      const struct pstate *state);

/**
 * @brief Update a maximum P-state request.
 *
 * @param req Request object.
 * @param state New maximum allowed P-state.
 */
void power_qos_max_pstate_request_update(struct power_qos_pstate_request *req,
						 const struct pstate *state);

/**
 * @brief Remove a maximum P-state request.
 *
 * @param req Request object.
 */
void power_qos_max_pstate_request_remove(struct power_qos_pstate_request *req);

/**
 * @brief Get effective P-state constraints.
 *
 * @param constraints Output constraints.
 */
void power_qos_pstate_constraints_get(struct power_qos_pstate_constraints *constraints);

/**
 * @brief Test whether a P-state satisfies effective constraints.
 *
 * If @p constraints is NULL, the current effective constraints are queried.
 * A conflicting constraint set allows no P-state.
 *
 * @param state Candidate P-state.
 * @param constraints Optional constraints to test against.
 *
 * @return True if @p state is allowed by the constraints.
 */
bool power_qos_pstate_is_allowed(const struct pstate *state,
					 const struct power_qos_pstate_constraints *constraints);
#else
static inline void power_qos_min_pstate_request_add(struct power_qos_pstate_request *req,
						    const struct pstate *state)
{
	ARG_UNUSED(req);
	ARG_UNUSED(state);
}

static inline void power_qos_min_pstate_request_update(struct power_qos_pstate_request *req,
						       const struct pstate *state)
{
	ARG_UNUSED(req);
	ARG_UNUSED(state);
}

static inline void power_qos_min_pstate_request_remove(struct power_qos_pstate_request *req)
{
	ARG_UNUSED(req);
}

static inline void power_qos_max_pstate_request_add(struct power_qos_pstate_request *req,
						    const struct pstate *state)
{
	ARG_UNUSED(req);
	ARG_UNUSED(state);
}

static inline void power_qos_max_pstate_request_update(struct power_qos_pstate_request *req,
						       const struct pstate *state)
{
	ARG_UNUSED(req);
	ARG_UNUSED(state);
}

static inline void power_qos_max_pstate_request_remove(struct power_qos_pstate_request *req)
{
	ARG_UNUSED(req);
}

static inline void power_qos_pstate_constraints_get(
	struct power_qos_pstate_constraints *constraints)
{
	constraints->min_pstate = NULL;
	constraints->max_pstate = NULL;
	constraints->conflict = false;
}

static inline bool power_qos_pstate_is_allowed(
	const struct pstate *state, const struct power_qos_pstate_constraints *constraints)
{
	ARG_UNUSED(state);
	ARG_UNUSED(constraints);

	return true;
}
#endif /* CONFIG_POWER_QOS */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POWER_QOS_H_ */
