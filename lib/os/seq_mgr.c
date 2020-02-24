/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/seq_mgr.h>
#include <sys/atomic.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(seq_mgr, CONFIG_SYS_SEQ_MGR_LOG_LEVEL);

/* Function called when sequence is completed. */
static void finalize(struct sys_seq_mgr *mgr, int res)
{
	struct sys_seq *seq = mgr->current_seq;
	sys_notify_generic_callback clbk;

	res = ((res == 0) && (mgr->abort)) ? -ECANCELED : res;
	mgr->current_idx = 0;
	mgr->in_teardown = 0;
	mgr->abort = 0;
	mgr->current_seq = NULL;

	LOG_DBG("Finalizing sequence (res: %d)", res);

	clbk = sys_notify_finalize(mgr->current_notify, res);
	if (mgr->vtable->callback && clbk) {
		mgr->vtable->callback(mgr, seq, res, clbk);
	}
}

static const union sys_seq_action *sys_seq_get_current_action(
							struct sys_seq_mgr *mgr)
{
	return &mgr->current_seq->actions[mgr->current_idx];
}

void *sys_seq_get_current_data(struct sys_seq_mgr *mgr)
{
	const union sys_seq_action *action = sys_seq_get_current_action(mgr);

	if (mgr->vtable->action_process) {
		return action->generic;
	}

	return (void *)action->custom->data;
}

static int execute_action(struct sys_seq_mgr *mgr)
{
	const union sys_seq_action *action = sys_seq_get_current_action(mgr);

	if (mgr->vtable->action_process) {
		return mgr->vtable->action_process(mgr, action->generic);
	}

	return action->custom->func(mgr, (void *)action->custom->data);
}

static void finish_seq(struct sys_seq_mgr *mgr, int res)
{
	/* close the sequence, attempt to do teardown or directly go to
	 * completion.
	 */
	if (mgr->vtable->teardown && (mgr->in_teardown == 0)) {
		int idx = mgr->current_idx;

		mgr->in_teardown = 1;
		res = mgr->vtable->teardown(mgr, mgr->current_seq,
					    idx, res);
		if (res == 0) {
			return;
		}
	}

	finalize(mgr, res);
}

void sys_seq_finalize(struct sys_seq_mgr *mgr, int res, int incr_offset)
{
	int idx;


	LOG_DBG("Action compleded (res: %d)", res);

	if (mgr->in_teardown) {
		/* finalize if in teardown. */
		finalize(mgr, res);
		return;
	}

	idx = (int)mgr->current_idx + incr_offset + ((mgr->in_setup) ? 0 : 1);
	mgr->in_setup = 0;
	if (idx < 0 || (idx > mgr->current_seq->num_actions)) {
		res = -EINVAL;
	} else {
		mgr->current_idx = idx;
	}

	/* finish (teardown then finalize) if error, abort or end of sequence.*/
	if ((res < 0) || mgr->abort ||
		(mgr->current_idx == mgr->current_seq->num_actions)) {
		finish_seq(mgr, res);
		return;
	}

	/* process next action */
	__ASSERT_NO_MSG(mgr->current_idx < mgr->current_seq->num_actions);
	res = execute_action(mgr);
	if (res < 0) {
		finish_seq(mgr, res);
	}
}

int sys_seq_process(struct sys_seq_mgr *mgr,
		    const struct sys_seq *seq,
		    struct sys_notify *notify)
{
	int err;

	if (seq->num_actions > SYS_SEQ_MAX_CHUNKS) {
		return -EINVAL;
	}

	/* Use current_seq pointer to track business of the manager */
	if (atomic_ptr_cas((atomic_ptr_t *)&mgr->current_seq,
			   NULL, (void *)seq) == false) {
		return -EBUSY;
	}

	LOG_DBG("Starting sequence process (%d actions)", seq->num_actions);
	mgr->current_notify = notify;
	mgr->in_setup = (mgr->vtable->setup) ? 1 : 0;
	err = (mgr->vtable->setup) ?
		mgr->vtable->setup(mgr, mgr->current_seq) : execute_action(mgr);
	if (err < 0) {
		mgr->current_seq = NULL;
		mgr->current_idx = 0;
	}

	return err;
}

int sys_seq_abort(struct sys_seq_mgr *mgr)
{
	if (mgr->current_seq == NULL) {
		return -EINVAL;
	}

	mgr->abort = 1;

	return 0;
}

void timeout_handler(struct k_timer *timer)
{
	struct sys_seq_mgr *mgr = k_timer_user_data_get(timer);

	sys_seq_finalize(mgr, 0, 0);
}

int sys_seq_delay(struct sys_seq_mgr *mgr, void *action)
{
	uint32_t *timeout = action;

	if (mgr->timer == NULL) {
		return -ENODEV;
	}

	k_timer_start(mgr->timer, K_USEC(*timeout), K_NO_WAIT);

	return 0;
}

int sys_seq_mgr_init(struct sys_seq_mgr *mgr,
		     const struct sys_seq_functions *vtable,
		     struct k_timer *timer)
{
	mgr->vtable = vtable;
	mgr->timer = timer;

	if (timer) {
		k_timer_init(mgr->timer, timeout_handler, NULL);
		k_timer_user_data_set(mgr->timer, mgr);
	}

	return 0;
}
