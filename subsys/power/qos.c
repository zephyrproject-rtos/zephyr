/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <zephyr/cpu_freq/pstate.h>
#include <zephyr/kernel.h>
#include <zephyr/power/qos.h>

static struct k_spinlock pstate_lock;
static sys_slist_t min_pstate_reqs;
static sys_slist_t max_pstate_reqs;
static const struct pstate *effective_min_pstate;
static const struct pstate *effective_max_pstate;
static bool effective_pstate_conflict;

static bool pstate_has_more_capacity(const struct pstate *left, const struct pstate *right)
{
	return left->frequency_hz > right->frequency_hz;
}

static bool pstate_has_lower_capacity(const struct pstate *left, const struct pstate *right)
{
	return left->frequency_hz < right->frequency_hz;
}

static bool request_is_registered(sys_slist_t *requests, const struct power_qos_pstate_request *req)
{
	struct power_qos_pstate_request *iter;

	SYS_SLIST_FOR_EACH_CONTAINER(requests, iter, node) {
		if (iter == req) {
			return true;
		}
	}

	return false;
}

static void update_pstate_constraints(void)
{
	struct power_qos_pstate_request *req;

	effective_min_pstate = NULL;
	effective_max_pstate = NULL;
	effective_pstate_conflict = false;

	SYS_SLIST_FOR_EACH_CONTAINER(&min_pstate_reqs, req, node) {
		if ((effective_min_pstate == NULL) ||
		    pstate_has_more_capacity(req->state, effective_min_pstate)) {
			effective_min_pstate = req->state;
		}
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&max_pstate_reqs, req, node) {
		if ((effective_max_pstate == NULL) ||
		    pstate_has_lower_capacity(req->state, effective_max_pstate)) {
			effective_max_pstate = req->state;
		}
	}

	if ((effective_min_pstate != NULL) && (effective_max_pstate != NULL) &&
	    pstate_has_more_capacity(effective_min_pstate, effective_max_pstate)) {
		effective_pstate_conflict = true;
	}
}

static void pstate_request_add(sys_slist_t *requests, struct power_qos_pstate_request *req,
				       const struct pstate *state)
{
	__ASSERT_NO_MSG(req != NULL);
	__ASSERT_NO_MSG(state != NULL);

	K_SPINLOCK(&pstate_lock) {
		req->state = state;
		if (!request_is_registered(requests, req)) {
			sys_slist_append(requests, &req->node);
		}
		update_pstate_constraints();
	}
}

static void pstate_request_update(struct power_qos_pstate_request *req, const struct pstate *state)
{
	__ASSERT_NO_MSG(req != NULL);
	__ASSERT_NO_MSG(state != NULL);

	K_SPINLOCK(&pstate_lock) {
		req->state = state;
		update_pstate_constraints();
	}
}

static void pstate_request_remove(sys_slist_t *requests, struct power_qos_pstate_request *req)
{
	__ASSERT_NO_MSG(req != NULL);

	K_SPINLOCK(&pstate_lock) {
		(void)sys_slist_find_and_remove(requests, &req->node);
		update_pstate_constraints();
	}
}

void power_qos_min_pstate_request_add(struct power_qos_pstate_request *req,
					      const struct pstate *state)
{
	pstate_request_add(&min_pstate_reqs, req, state);
}

void power_qos_min_pstate_request_update(struct power_qos_pstate_request *req,
						 const struct pstate *state)
{
	pstate_request_update(req, state);
}

void power_qos_min_pstate_request_remove(struct power_qos_pstate_request *req)
{
	pstate_request_remove(&min_pstate_reqs, req);
}

void power_qos_max_pstate_request_add(struct power_qos_pstate_request *req,
					      const struct pstate *state)
{
	pstate_request_add(&max_pstate_reqs, req, state);
}

void power_qos_max_pstate_request_update(struct power_qos_pstate_request *req,
						 const struct pstate *state)
{
	pstate_request_update(req, state);
}

void power_qos_max_pstate_request_remove(struct power_qos_pstate_request *req)
{
	pstate_request_remove(&max_pstate_reqs, req);
}

void power_qos_pstate_constraints_get(struct power_qos_pstate_constraints *constraints)
{
	__ASSERT_NO_MSG(constraints != NULL);

	K_SPINLOCK(&pstate_lock) {
		constraints->min_pstate = effective_min_pstate;
		constraints->max_pstate = effective_max_pstate;
		constraints->conflict = effective_pstate_conflict;
	}
}

bool power_qos_pstate_is_allowed(const struct pstate *state,
					 const struct power_qos_pstate_constraints *constraints)
{
	struct power_qos_pstate_constraints current_constraints;

	__ASSERT_NO_MSG(state != NULL);

	if (constraints == NULL) {
		power_qos_pstate_constraints_get(&current_constraints);
		constraints = &current_constraints;
	}

	if (constraints->conflict) {
		return false;
	}

	if ((constraints->min_pstate != NULL) &&
	    pstate_has_lower_capacity(state, constraints->min_pstate)) {
		return false;
	}

	if ((constraints->max_pstate != NULL) &&
	    pstate_has_more_capacity(state, constraints->max_pstate)) {
		return false;
	}

	return true;
}
