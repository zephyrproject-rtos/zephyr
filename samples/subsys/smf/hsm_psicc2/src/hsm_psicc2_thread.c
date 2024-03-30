/*
 * Copyright (c) 2024 Glenn Andrews
 * State Machine example copyright (c) Miro Samek
 *
 * Implementation of the statechart in Figure 2.11 of
 * Practical UML Statecharts in C/C++, 2nd Edition by Miro Samek
 * https://www.state-machine.com/psicc2
 * Used with permission of the author.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include <stdbool.h>
#include "hsm_psicc2_thread.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hsm_psicc2_thread);

/* User defined object */
struct s_object {
	/* This must be first */
	struct smf_ctx ctx;

	/* Other state specific data add here */
	struct hsm_psicc2_event event;
	int foo;
} s_obj;

/* Declaration of possible states */
enum demo_states {
	STATE_INITIAL,
	STATE_S,
	STATE_S1,
	STATE_S2,
	STATE_S11,
	STATE_S21,
	STATE_S211,
};

/* Forward declaration of state table */
static const struct smf_state demo_states[];

/********* STATE_INITIAL *********/
static void initial_entry(void *o)
{
	LOG_INF("%s", __func__);
	struct s_object *obj = (struct s_object *)o;

	obj->foo = false;
}

static void initial_run(void *o)
{
	LOG_INF("%s", __func__);
}

static void initial_exit(void *o)
{
	LOG_INF("%s", __func__);
}

/********* STATE_S *********/
static void s_entry(void *o)
{
	LOG_INF("%s", __func__);
}

static void s_run(void *o)
{
	LOG_INF("%s", __func__);
	struct s_object *obj = (struct s_object *)o;

	switch (obj->event.event_id) {
	case EVENT_E:
		LOG_INF("%s received EVENT_E", __func__);
		smf_set_state(SMF_CTX(obj), &demo_states[STATE_S11]);
		break;
	case EVENT_I:
		if (obj->foo) {
			LOG_INF("%s received EVENT_I and set foo false", __func__);
			obj->foo = false;
		} else {
			LOG_INF("%s received EVENT_I and did nothing", __func__);
		}
		smf_set_handled(SMF_CTX(obj));
		break;
	case EVENT_TERMINATE:
		LOG_INF("%s received SMF_EVENT_TERMINATE. Terminating", __func__);
		smf_set_terminate(SMF_CTX(obj), -1);
	}
}

static void s_exit(void *o)
{
	LOG_INF("%s", __func__);
}

/********* STATE_S1 *********/
static void s1_entry(void *o)
{
	LOG_INF("%s", __func__);
}

static void s1_run(void *o)
{
	LOG_INF("%s", __func__);
	struct s_object *obj = (struct s_object *)o;

	switch (obj->event.event_id) {
	case EVENT_A:
		LOG_INF("%s received EVENT_A", __func__);
		smf_set_state(SMF_CTX(obj), &demo_states[STATE_S1]);
		break;
	case EVENT_B:
		LOG_INF("%s received EVENT_B", __func__);
		smf_set_state(SMF_CTX(obj), &demo_states[STATE_S11]);
		break;
	case EVENT_C:
		LOG_INF("%s received EVENT_C", __func__);
		smf_set_state(SMF_CTX(obj), &demo_states[STATE_S2]);
		break;
	case EVENT_D:
		if (!obj->foo) {
			LOG_INF("%s received EVENT_D and acted on it", __func__);
			obj->foo = true;
			smf_set_state(SMF_CTX(obj), &demo_states[STATE_S]);
		} else {
			LOG_INF("%s received EVENT_D and ignored it", __func__);
		}
		break;
	case EVENT_F:
		LOG_INF("%s received EVENT_F", __func__);
		smf_set_state(SMF_CTX(obj), &demo_states[STATE_S211]);
		break;
	case EVENT_I:
		LOG_INF("%s received EVENT_I", __func__);
		smf_set_handled(SMF_CTX(obj));
		break;
	}
}

static void s1_exit(void *o)
{
	LOG_INF("%s", __func__);
}

/********* STATE_S11 *********/
static void s11_entry(void *o)
{
	LOG_INF("%s", __func__);
}

static void s11_run(void *o)
{
	LOG_INF("%s", __func__);
	struct s_object *obj = (struct s_object *)o;

	switch (obj->event.event_id) {
	case EVENT_D:
		if (obj->foo) {
			LOG_INF("%s received EVENT_D and acted upon it", __func__);
			obj->foo = false;
			smf_set_state(SMF_CTX(obj), &demo_states[STATE_S1]);
		} else {
			LOG_INF("%s received EVENT_D and ignored it", __func__);
		}
		break;
	case EVENT_G:
		LOG_INF("%s received EVENT_G", __func__);
		smf_set_state(SMF_CTX(obj), &demo_states[STATE_S21]);
		break;
	case EVENT_H:
		LOG_INF("%s received EVENT_H", __func__);
		smf_set_state(SMF_CTX(obj), &demo_states[STATE_S]);
		break;
	}
}

static void s11_exit(void *o)
{
	LOG_INF("%s", __func__);
}

/********* STATE_S2 *********/
static void s2_entry(void *o)
{
	LOG_INF("%s", __func__);
}

static void s2_run(void *o)
{
	LOG_INF("%s", __func__);
	struct s_object *obj = (struct s_object *)o;

	switch (obj->event.event_id) {
	case EVENT_C:
		LOG_INF("%s received EVENT_C", __func__);
		smf_set_state(SMF_CTX(obj), &demo_states[STATE_S1]);
		break;
	case EVENT_F:
		LOG_INF("%s received EVENT_F", __func__);
		smf_set_state(SMF_CTX(obj), &demo_states[STATE_S11]);
		break;
	case EVENT_I:
		if (!obj->foo) {
			LOG_INF("%s received EVENT_I and set foo true", __func__);
			obj->foo = true;
			smf_set_handled(SMF_CTX(obj));
		} else {
			LOG_INF("%s received EVENT_I and did nothing", __func__);
		}
		break;
	}
}

static void s2_exit(void *o)
{
	LOG_INF("%s", __func__);
}

/********* STATE_S21 *********/
static void s21_entry(void *o)
{
	LOG_INF("%s", __func__);
}

static void s21_run(void *o)
{
	LOG_INF("%s", __func__);
	struct s_object *obj = (struct s_object *)o;

	switch (obj->event.event_id) {
	case EVENT_A:
		LOG_INF("%s received EVENT_A", __func__);
		smf_set_state(SMF_CTX(obj), &demo_states[STATE_S21]);
		break;
	case EVENT_B:
		LOG_INF("%s received EVENT_B", __func__);
		smf_set_state(SMF_CTX(obj), &demo_states[STATE_S211]);
		break;
	case EVENT_G:
		LOG_INF("%s received EVENT_G", __func__);
		smf_set_state(SMF_CTX(obj), &demo_states[STATE_S1]);
		break;
	}
}

static void s21_exit(void *o)
{
	LOG_INF("%s", __func__);
}

/********* STATE_S211 *********/
static void s211_entry(void *o)
{
	LOG_INF("%s", __func__);
}

static void s211_run(void *o)
{
	LOG_INF("%s", __func__);
	struct s_object *obj = (struct s_object *)o;

	switch (obj->event.event_id) {
	case EVENT_D:
		LOG_INF("%s received EVENT_D", __func__);
		smf_set_state(SMF_CTX(obj), &demo_states[STATE_S21]);
		break;
	case EVENT_H:
		LOG_INF("%s received EVENT_H", __func__);
		smf_set_state(SMF_CTX(obj), &demo_states[STATE_S]);
		break;
	}
}

static void s211_exit(void *o)
{
	LOG_INF("%s", __func__);
}

/* State storage: handler functions, parent states and initial transition states */
static const struct smf_state demo_states[] = {
	[STATE_INITIAL] = SMF_CREATE_STATE(initial_entry, initial_run, initial_exit, NULL,
					   &demo_states[STATE_S2]),
	[STATE_S] = SMF_CREATE_STATE(s_entry, s_run, s_exit, &demo_states[STATE_INITIAL],
				     &demo_states[STATE_S11]),
	[STATE_S1] = SMF_CREATE_STATE(s1_entry, s1_run, s1_exit, &demo_states[STATE_S],
				      &demo_states[STATE_S11]),
	[STATE_S2] = SMF_CREATE_STATE(s2_entry, s2_run, s2_exit, &demo_states[STATE_S],
				      &demo_states[STATE_S211]),
	[STATE_S11] = SMF_CREATE_STATE(s11_entry, s11_run, s11_exit, &demo_states[STATE_S1], NULL),
	[STATE_S21] = SMF_CREATE_STATE(s21_entry, s21_run, s21_exit, &demo_states[STATE_S2],
				       &demo_states[STATE_S211]),
	[STATE_S211] =
		SMF_CREATE_STATE(s211_entry, s211_run, s211_exit, &demo_states[STATE_S21], NULL),
};

K_THREAD_STACK_DEFINE(hsm_psicc2_thread_stack, HSM_PSICC2_THREAD_STACK_SIZE);
K_MSGQ_DEFINE(hsm_psicc2_msgq, sizeof(struct hsm_psicc2_event), HSM_PSICC2_THREAD_EVENT_QUEUE_SIZE,
	      1);

static struct k_thread hsm_psicc2_thread_data;
static k_tid_t hsm_psicc2_thread_tid;

static void hsm_psicc2_thread(void *arg1, void *arg2, void *arg3)
{
	smf_set_initial(SMF_CTX(&s_obj), &demo_states[STATE_INITIAL]);
	while (1) {
		int rc = k_msgq_get(&hsm_psicc2_msgq, &s_obj.event, K_FOREVER);

		if (rc == 0) {
			/* Run state machine with given message */
			rc = smf_run_state(SMF_CTX(&s_obj));

			if (rc) {
				/* State machine terminates if a non-zero value is returned */
				LOG_INF("%s terminating state machine thread", __func__);
				break;
			}
		} else {
			LOG_ERR("Waiting for event failed, code %d", rc);
		}
	}
}

void hsm_psicc2_thread_run(void)
{
	hsm_psicc2_thread_tid =
		k_thread_create(&hsm_psicc2_thread_data, hsm_psicc2_thread_stack,
				K_THREAD_STACK_SIZEOF(hsm_psicc2_thread_stack), hsm_psicc2_thread,
				NULL, NULL, NULL, HSM_PSICC2_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(hsm_psicc2_thread_tid, "psicc2_thread");
}
