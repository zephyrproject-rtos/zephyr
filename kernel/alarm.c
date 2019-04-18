/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <logging/log.h>

extern struct k_alarm _k_alarm_list_start[];
extern struct k_alarm _k_alarm_list_end[];

static struct k_spinlock lock;
static sys_dlist_t scheduled_list = SYS_DLIST_STATIC_INIT(&scheduled_list);
static sys_dlist_t ready_list = SYS_DLIST_STATIC_INIT(&ready_list);

/** Return a pointer to the k_alarm that contains the node if it has a
 * deadline at or before the provided when.  Return a null pointer if
 * the k_alarm is scheduled strictly after the provided when.
 */
static ALWAYS_INLINE struct k_alarm *alarm_due_by(sys_dnode_t *node,
						  uint32_t when)
{
	struct k_alarm *cap = CONTAINER_OF(node, struct k_alarm, node);

	if ((s32_t)(cap->deadline - when) > 0) {
		return 0;
	}
	return cap;
}

/** Add alarm to the provided list before the first alarm that has a
 * later deadline, or at the end if necessary.
 *
 * Return @true if the head of list changed as a result of this
 * addition.
 */
static bool link_alarm(sys_dlist_t *list,
		       struct k_alarm *alarm,
		       u32_t now)
{
	sys_dnode_t *node = &alarm->node;
	sys_dnode_t *cp = sys_dlist_peek_head(list);

	while (cp) {
		if (!alarm_due_by(cp, alarm->deadline)) {
			break;
		}
		cp = sys_dlist_peek_next_no_check(list, cp);
	}
	if (cp) {
		sys_dlist_insert(cp, node);
	} else {
		sys_dlist_append(list, node);
	}
	return sys_dlist_is_head(list, node);
}

void k_alarm_init(struct k_alarm *alarm,
		  k_alarm_handler_t handler,
		  k_alarm_cancel_t cancel,
		  void *user_data)
{
	*alarm = Z_ALARM_INITIALIZER(handler, cancel, user_data);
}

int k_alarm_next_deadline_(u32_t *deadline)
{
	int rv = -1;
	k_spinlock_key_t key = k_spin_lock(&lock);

	if (!sys_dlist_is_empty(&ready_list)) {
		rv = 0;
		goto unlock;
	}

	sys_dnode_t *hp = sys_dlist_peek_head(&scheduled_list);

	if (hp) {
		struct k_alarm *ap = CONTAINER_OF(hp, struct k_alarm, node);
		*deadline = ap->deadline;
		rv = 1;
	}
unlock:
	k_spin_unlock(&lock, key);
	return rv;
}

int k_alarm_split_(u32_t now)
{
	int rv = 0;
	k_spinlock_key_t key = k_spin_lock(&lock);
	sys_dnode_t *cp = sys_dlist_peek_head(&scheduled_list);

	while (cp) {
		struct k_alarm *cap = alarm_due_by(cp, now);

		if (!cap) {
			break;
		}
		cap->state = K_ALARM_READY;
		++rv;
		cp = sys_dlist_peek_next_no_check(&scheduled_list, cp);
	}
	if ((rv == 0)
	    && !sys_dlist_is_empty(&ready_list)) {
		rv = -1;
	}

	if (cp) {
		sys_dlist_split(&ready_list, &scheduled_list, cp);
	} else {
		sys_dlist_join(&ready_list, &scheduled_list);
	}

	k_spin_unlock(&lock, key);
	return rv;
}

void k_alarm_process_ready_(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	do {
		sys_dnode_t *cp = sys_dlist_get(&ready_list);

		if (!cp) {
			break;
		}

		struct k_alarm *cap = CONTAINER_OF(cp, struct k_alarm, node);

		cap->state = K_ALARM_ACTIVE;
		if (cap->handler_fn) {
			k_spin_unlock(&lock, key);
			cap->handler_fn(cap, cap->user_data);
			key = k_spin_lock(&lock);
		}
		if (cap->state == K_ALARM_ACTIVE) {
			cap->state = K_ALARM_UNSCHEDULED;
		}
	} while (true);
	k_spin_unlock(&lock, key);
}

int z_impl_k_alarm_schedule(struct k_alarm *alarm,
			    u32_t deadline,
			    u32_t flags)
{
	int rv = -EINVAL;
	bool update_deadline = false;

	if (!alarm) {
		goto out;
	}
	k_spinlock_key_t key = k_spin_lock(&lock);

	bool may_resched = true
		&& (alarm->state == K_ALARM_SCHEDULED)
		&& (flags & (K_ALARM_FLAG_REPLACE | K_ALARM_FLAG_IF_SOONER));

	if (sys_dnode_is_linked(&alarm->node)
	    && !may_resched) {
		rv = -EINVAL;
		goto unlock;
	}
	if ((alarm->state != K_ALARM_UNSCHEDULED)
	    && (alarm->state != K_ALARM_ACTIVE)
	    && (alarm->state != K_ALARM_CANCELLED)
	    && !may_resched) {
		rv = -EBUSY;
		goto unlock;
	}

	u32_t now = k_cycle_get_32();
	s32_t delay = deadline - now;

	if (may_resched) {
		if (K_ALARM_FLAG_REPLACE & flags) {
			/* Unconditional reschedule */
		} else if ((K_ALARM_FLAG_IF_SOONER & flags)
			   && (delay >= (s32_t)(alarm->deadline - now))) {
			/* Not sooner; keep unchanged */
			rv = 1;
			goto unlock;
		}
		sys_dlist_remove(&alarm->node);
	}

	alarm->deadline = deadline;
	if (delay > 0) {
		alarm->state = K_ALARM_SCHEDULED;
		update_deadline = link_alarm(&scheduled_list, alarm, now);
		rv = 1;
	} else if (K_ALARM_FLAG_ERROR_IF_LATE & flags) {
		rv = -EINVAL;
	} else {
		alarm->state = K_ALARM_READY;
		update_deadline = link_alarm(&ready_list, alarm, now);
		rv = 0;
	}

unlock:
	k_spin_unlock(&lock, key);
out:
	if (update_deadline) {
		z_alarm_update_deadline();
	}

	return rv;
}

int z_impl_k_alarm_cancel(struct k_alarm *alarm)
{
	int rv = -EINVAL;
	bool update_deadline = false;

	if (!alarm) {
		goto out;
	}
	k_spinlock_key_t key = k_spin_lock(&lock);

	if (alarm->state == K_ALARM_SCHEDULED) {
		update_deadline = (&alarm->node
				   == sys_dlist_peek_head(&scheduled_list));
		rv = 1;
		sys_dlist_remove(&alarm->node);
	} else if (alarm->state == K_ALARM_READY) {
		sys_dlist_remove(&alarm->node);
		rv = 0;
	} else {
		goto unlock;
	}
	alarm->state = K_ALARM_CANCELLED;
	if (alarm->cancel_fn) {
		k_spin_unlock(&lock, key);
		alarm->cancel_fn(alarm, alarm->user_data);
		key = k_spin_lock(&lock);
	}
	if (alarm->state == K_ALARM_CANCELLED) {
		alarm->state = K_ALARM_UNSCHEDULED;
	}
unlock:
	k_spin_unlock(&lock, key);
out:
	if (update_deadline) {
		z_alarm_update_deadline();
	}

	return rv;
}
