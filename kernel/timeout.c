/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <timeout_q.h>
#include <drivers/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>
#include <ksched.h>
#include <syscall_handler.h>
#include <init.h>
#include <counter.h>

#define LOCKED(lck) for (k_spinlock_key_t __i = {},			\
					  __key = k_spin_lock(lck);	\
			__i.key == 0;					\
			k_spin_unlock(lck, __key), __i.key = 1)

static sys_dlist_t timeout_list = SYS_DLIST_STATIC_INIT(&timeout_list);
static struct k_spinlock timeout_lock;

struct device *counter;	  /* Counter device used as clock source for kernel. */
static u32_t alarm_cycle; /* Clock cycle on which alarm is scheduled. */

int z_clock_hw_cycles_per_sec = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(z_clock_hw_cycles_per_sec_runtime_get)
{
	return z_impl_z_clock_hw_cycles_per_sec_runtime_get();
}
#endif /* CONFIG_USERSPACE */

static void system_clock_set_alarm(s32_t cycle);

static struct _timeout *first(void)
{
	sys_dnode_t *t = sys_dlist_peek_head(&timeout_list);

	return t == NULL ? NULL : CONTAINER_OF(t, struct _timeout, node);
}

static struct _timeout *next(struct _timeout *t)
{
	sys_dnode_t *n = sys_dlist_peek_next(&timeout_list, &t->node);

	return n == NULL ? NULL : CONTAINER_OF(n, struct _timeout, node);
}

static void remove(struct _timeout *t)
{
	sys_dlist_remove(&t->node);
}

static s32_t next_timeout(void)
{
	if (first() != NULL)
		return first()->cycle;

	return k_cycle_get_32() + counter_get_max_relative_alarm(counter);
}

void z_add_timeout(struct _timeout *to, _timeout_func_t fn, s32_t cycle)
{
	__ASSERT(!sys_dnode_is_linked(&to->node), "");

	to->cycle = cycle;
	to->fn = fn;

	LOCKED(&timeout_lock) {
		struct _timeout *t;

		for (t = first(); t != NULL; t = next(t)) {
			if ((t->cycle - to->cycle) > 0) {
				sys_dlist_insert(&t->node, &to->node);
				break;
			}
		}

		if (t == NULL) {
			sys_dlist_append(&timeout_list, &to->node);
		}

		if (to == first()) {
			system_clock_set_alarm(next_timeout());
		}
	}
}

int z_abort_timeout(struct _timeout *to)
{
	int ret = -EINVAL;

	LOCKED(&timeout_lock) {
		if (sys_dnode_is_linked(&to->node)) {
			remove(to);
			ret = 0;
		}
	}

	return ret;
}

s32_t z_get_next_timeout_expiry(void)
{
	__ASSERT(false, "");
	return 0;
}

void z_set_timeout_expiry(s32_t ticks, bool idle)
{
	__ASSERT(false, "");
}

u32_t z_timer_cycle_get_32(void)
{
	return counter_read(counter);
}

static void system_clock_alarm(struct device *dev, u8_t chan_id,
                              u32_t cycle, void *user_data)
{
	u32_t now = cycle;

	k_spinlock_key_t key = k_spin_lock(&timeout_lock);

	while (first() != NULL && (s32_t)(now - first()->cycle) >= 0) {
		struct _timeout *t = first();

		remove(t);

		k_spin_unlock(&timeout_lock, key);
		t->fn(t);
		key = k_spin_lock(&timeout_lock);

		now = k_cycle_get_32();
	}

	k_spin_unlock(&timeout_lock, key);
}

static void system_clock_set_alarm(s32_t cycle)
{
	s32_t now = counter_read(counter);
	struct counter_alarm_cfg alarm_cfg = {
		.callback = system_clock_alarm,
		.absolute = true,
	};
	int atempt = 1;
	int status;

	if ((alarm_cycle - now) > 0 &&
	    (alarm_cycle - now) < z_us_to_cycles(100)) { // TODO: How select/eliminate this constant? */
		/* Just wait for alarm if it is scheduled in less than 100us. */
		return;
	}

	if ((cycle - now) > counter_get_max_relative_alarm(counter)) {
		cycle = now + counter_get_max_relative_alarm(counter);
	}

	do {
		/*
		 * If alarm is in the past or closer than 3us from
		 * now, delay it a bit.
		 */
		if ((cycle - now) < z_us_to_cycles(10)) { // TODO: How to select/eliminate this constant?
			cycle += (atempt * z_us_to_cycles(10));
		}

		alarm_cfg.ticks = cycle;
		counter_cancel_channel_alarm(counter, 0);
		status = counter_set_channel_alarm(counter, 0,
								&alarm_cfg);

		/*
		 * If we are still before alarm, the work is done.
		 * Otherwise there is a risk that we missed alarm,
		 * so in order to avoid uncertainty repeat counter
		 * configuration.
		 */
		now = counter_read(counter);
		atempt += 1;
	} while ((now - cycle) >= 0);

	__ASSERT(status == 0,
		 "Cannot set alarm. Error %d!", status);

	alarm_cycle = cycle;
}

static int system_clock_init(struct device *device)
{
	struct counter_top_cfg counter_top_cfg = { 0 };
	int status;
	u32_t max;

	counter = device_get_binding(DT_CLOCK_SOURCE_ON_DEV_NAME);

	__ASSERT(counter != NULL, "System clock device not found!");
	__ASSERT(counter_get_num_of_channels(counter) > 0,
		"System clock device must have at least one compare channel!");
	__ASSERT(counter_is_counting_up(counter),
		"System clock device must count up!");

	z_clock_hw_cycles_per_sec = counter_get_frequency(counter);

	max = counter_get_max_top_value(counter);
	__ASSERT(max == UINT32_MAX,
		"Maximum counter top value must be equal to UINT32_MAX!");

	/*
	 * Set counter top to the largest possible value. Note, that some
	 * counters might not support ceratin reset modes, so we might need
	 * more that one attempt.
	 */
	counter_top_cfg.ticks = max;
	status = counter_set_top_value(counter, &counter_top_cfg);
	if (status != 0) {
		counter_top_cfg.flags |= COUNTER_TOP_CFG_DONT_RESET;
		status = counter_set_top_value(counter, &counter_top_cfg);
	}
	__ASSERT(status == 0, "Could not configure system clock device!");

	status = counter_start(counter);
	__ASSERT(status == 0, "Could not start system clock!");

#if 1
	/* DEMO: Print information about current clock source. */
	printk("Clock Source: %s (frequency: %u Hz)\n",
		DT_CLOCK_SOURCE_ON_DEV_NAME,
		z_clock_hw_cycles_per_sec);
#endif

	return status;
}

SYS_INIT(system_clock_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(k_uptime_get, ret_p)
{
	u64_t *ret = (u64_t *)ret_p;

	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(ret, sizeof(*ret)));
	*ret = z_impl_k_uptime_get();
	return 0;
}
#endif
