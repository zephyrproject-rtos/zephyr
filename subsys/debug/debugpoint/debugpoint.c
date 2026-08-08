/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/debugpoint.h>
#include <zephyr/debug/debugpoint_internal.h>
#include <zephyr/kernel.h>
#if defined(CONFIG_SMP)
#include <ipi.h>
#endif
#if defined(CONFIG_PM)
#include <zephyr/pm/pm.h>
#endif
#include <zephyr/sys/atomic.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum debugpoint_slot_state {
	DEBUGPOINT_SLOT_FREE,
	DEBUGPOINT_SLOT_LIVE,
	DEBUGPOINT_SLOT_RETIRING,
};

/*
 * state, generation, and hit_count are protected by state_lock. config is
 * immutable while the slot is LIVE or RETIRING. hit_count pins a generation
 * while callbacks execute without the lock.
 */
struct z_debugpoint_slot {
	enum debugpoint_slot_state state;
	uint32_t generation;
	uint32_t hit_count;
	struct z_debugpoint_config config;
};

static struct z_debugpoint_slot slots[CONFIG_DEBUGPOINT_MAX_SLOTS];
/* Per-CPU recursion marker used to reject lifecycle calls from callbacks. */
static atomic_t callback_depth[CONFIG_MP_MAX_NUM_CPUS];
static struct k_spinlock state_lock;
/*
 * Thread-context add, remove, and cleanup are serialized separately from the
 * exception-safe slot state above.
 */
K_MUTEX_DEFINE(debugpoint_lock);

#if defined(CONFIG_PM)
static void debugpoint_pm_exit(enum pm_state state)
{
	ARG_UNUSED(state);

	(void)arch_debugpoint_cpu_sync();
}

static struct pm_notifier debugpoint_pm_notifier = {
	.state_exit = debugpoint_pm_exit,
};

static int debugpoint_pm_init(void)
{
	pm_notifier_register(&debugpoint_pm_notifier);
	return 0;
}

SYS_INIT(debugpoint_pm_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif

static struct k_work cleanup_work;
static bool work_initialized;
static void cleanup_work_handler(struct k_work *work);

BUILD_ASSERT(CONFIG_DEBUGPOINT_MAX_SLOTS <= 32);

#ifdef CONFIG_SMP
static struct k_ipi_work sync_work;
static atomic_t sync_result;

static uint32_t debugpoint_cpu_mask(void)
{
	uint32_t mask = 0U;

	for (unsigned int i = 0; i < arch_num_cpus(); i++) {
		mask |= BIT(i);
	}

	return mask;
}
#endif

bool z_debugpoint_in_callback(void)
{
	return atomic_get(&callback_depth[arch_curr_cpu()->id]) != 0;
}

static bool invalid_call_context(void)
{
	return k_is_in_isr() || !arch_cpu_irqs_are_enabled() ||
	       z_debugpoint_in_callback();
}

static int validate(const struct z_debugpoint_config *config,
		    z_debugpoint_handle_t *handle)
{
	if (config == NULL || handle == NULL || config->callback == NULL) {
		return -EINVAL;
	}
	if (config->type < Z_DEBUGPOINT_WATCH_READ ||
	    config->type >= Z_DEBUGPOINT_TYPE_COUNT || config->size == 0U) {
		return -EINVAL;
	}

	uintptr_t addr = (uintptr_t)config->addr;

	return UINTPTR_MAX - addr < config->size - 1U ? -EINVAL : 0;
}

static int find_free_slot(void)
{
	for (int i = 0; i < ARRAY_SIZE(slots); i++) {
		if (slots[i].state == DEBUGPOINT_SLOT_FREE) {
			return i;
		}
	}

	return -ENOSPC;
}

/* Slot + 1 reserves zero as invalid; generation rejects stale hit reports. */
static z_debugpoint_handle_t make_handle(int slot, uint32_t generation)
{
	return ((z_debugpoint_handle_t)generation << 32) |
	       (uint32_t)(slot + 1);
}

static int handle_slot(z_debugpoint_handle_t handle)
{
	uint32_t encoded_slot = (uint32_t)handle;

	if (encoded_slot == 0U || encoded_slot > ARRAY_SIZE(slots)) {
		return -EINVAL;
	}

	return (int)encoded_slot - 1;
}

static uint32_t handle_generation(z_debugpoint_handle_t handle)
{
	return (uint32_t)(handle >> 32);
}

static bool handle_matches(const struct z_debugpoint_slot *slot,
			   z_debugpoint_handle_t handle)
{
	return slot->generation == handle_generation(handle);
}

static void wait_for_hit_callbacks(int slot, uint32_t generation)
{
	for (;;) {
		k_spinlock_key_t key = k_spin_lock(&state_lock);
		bool done = slots[slot].generation != generation ||
			    slots[slot].hit_count == 0U;

		k_spin_unlock(&state_lock, key);
		if (done) {
			return;
		}
		k_yield();
	}
}

static void release_slot(int slot, uint32_t generation)
{
	k_spinlock_key_t key = k_spin_lock(&state_lock);

	if (slots[slot].generation == generation &&
	    slots[slot].state == DEBUGPOINT_SLOT_RETIRING &&
	    slots[slot].hit_count == 0U) {
		slots[slot].state = DEBUGPOINT_SLOT_FREE;
	}
	k_spin_unlock(&state_lock, key);
}

static void debugpoint_work_init(void)
{
	if (work_initialized) {
		return;
	}

	k_work_init(&cleanup_work, cleanup_work_handler);
#ifdef CONFIG_SMP
	k_ipi_work_init(&sync_work);
#endif
	work_initialized = true;
}

#ifdef CONFIG_SMP
static void sync_ipi_handler(struct k_ipi_work *work)
{
	int ret = arch_debugpoint_cpu_sync();

	ARG_UNUSED(work);
	if (ret != 0) {
		(void)atomic_cas(&sync_result, 0, ret);
	}
}

/* Reconcile locally, submit remote IPI work, and wait for every target CPU. */
static int sync_cpus(void)
{
	unsigned int key = arch_irq_lock();
	unsigned int cpu = arch_curr_cpu()->id;
	uint32_t cpu_mask = debugpoint_cpu_mask() & ~BIT(cpu);

	atomic_set(&sync_result, arch_debugpoint_cpu_sync());

	if (cpu_mask == 0U) {
		arch_irq_unlock(key);
		return (int)atomic_get(&sync_result);
	}

	int ret = z_ipi_work_submit(&sync_work, cpu_mask, sync_ipi_handler);

	arch_irq_unlock(key);

	if (ret == 0) {
		ret = k_ipi_work_wait(&sync_work, K_FOREVER);
	}

	int sync_ret = (int)atomic_get(&sync_result);

	return sync_ret != 0 ? sync_ret : ret;
}
#endif

/*
 * Stop local hardware, drain callbacks, reconcile remote CPUs, then release
 * each slot. A failure leaves slots RETIRING so a later lifecycle call can
 * retry the same cleanup.
 */
static int cleanup_retiring_slots(void)
{
	uint32_t cleanup_mask = 0U;
	k_spinlock_key_t key = k_spin_lock(&state_lock);

	/* The caller holds debugpoint_lock, so retiring slots cannot be reused. */
	for (int i = 0; i < ARRAY_SIZE(slots); i++) {
		if (slots[i].state != DEBUGPOINT_SLOT_RETIRING) {
			continue;
		}

		cleanup_mask |= BIT(i);
	}
	k_spin_unlock(&state_lock, key);

	if (cleanup_mask == 0U) {
		return 0;
	}

	int ret = 0;

	for (int i = 0; i < ARRAY_SIZE(slots); i++) {
		if ((cleanup_mask & BIT(i)) == 0U) {
			continue;
		}

		int uninstall_ret = arch_debugpoint_uninstall_local(
			make_handle(i, slots[i].generation));

		if (ret == 0 && uninstall_ret != 0 && uninstall_ret != -ENOENT) {
			ret = uninstall_ret;
		}
	}

	for (int i = 0; i < ARRAY_SIZE(slots); i++) {
		if ((cleanup_mask & BIT(i)) != 0U) {
			wait_for_hit_callbacks(i, slots[i].generation);
		}
	}

#ifdef CONFIG_SMP
	int sync_ret = sync_cpus();

	if (ret == 0) {
		ret = sync_ret;
	}
#endif
	if (ret != 0) {
		return ret;
	}

	for (int i = 0; i < ARRAY_SIZE(slots); i++) {
		if ((cleanup_mask & BIT(i)) != 0U) {
			release_slot(i, slots[i].generation);
		}
	}
	return 0;
}

static void cleanup_work_handler(struct k_work *work)
{
	/* One-shot hits defer hardware cleanup to thread context. */
	ARG_UNUSED(work);

	k_mutex_lock(&debugpoint_lock, K_FOREVER);
	(void)cleanup_retiring_slots();
	k_mutex_unlock(&debugpoint_lock);
}

int z_debugpoint_add(const struct z_debugpoint_config *config,
		     z_debugpoint_handle_t *handle)
{
	int ret = validate(config, handle);

	if (ret != 0) {
		return ret;
	}
	if (invalid_call_context()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&debugpoint_lock, K_FOREVER);

	debugpoint_work_init();

	ret = arch_debugpoint_validate(config);
	if (ret != 0) {
		goto out;
	}

	ret = cleanup_retiring_slots();
	if (ret != 0) {
		goto out;
	}

	k_spinlock_key_t key = k_spin_lock(&state_lock);

	int slot = find_free_slot();

	if (slot < 0) {
		k_spin_unlock(&state_lock, key);
		ret = slot;
		goto out;
	}

	uint32_t generation = ++slots[slot].generation;

	if (generation == 0U) {
		generation = ++slots[slot].generation;
	}
	z_debugpoint_handle_t new_handle = make_handle(slot, generation);

	slots[slot].config = *config;
	slots[slot].hit_count = 0U;
	slots[slot].state = DEBUGPOINT_SLOT_LIVE;
	k_spin_unlock(&state_lock, key);

	ret = arch_debugpoint_install_local(&slots[slot].config, new_handle);
	if (ret != 0) {
		key = k_spin_lock(&state_lock);
		slots[slot].state = DEBUGPOINT_SLOT_FREE;
		k_spin_unlock(&state_lock, key);
		goto out;
	}

#ifdef CONFIG_SMP
	ret = sync_cpus();
	if (ret != 0) {
		int add_ret = ret;

		key = k_spin_lock(&state_lock);
		slots[slot].state = DEBUGPOINT_SLOT_RETIRING;
		k_spin_unlock(&state_lock, key);

		(void)cleanup_retiring_slots();
		ret = add_ret;
		goto out;
	}
#endif

	ret = 0;
	*handle = new_handle;

out:
	k_mutex_unlock(&debugpoint_lock);
	return ret;
}

int z_debugpoint_remove(z_debugpoint_handle_t handle)
{
	if (handle == Z_DEBUGPOINT_HANDLE_INVALID) {
		return 0;
	}

	int slot = handle_slot(handle);

	if (slot < 0) {
		return -EINVAL;
	}
	if (invalid_call_context()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&debugpoint_lock, K_FOREVER);
	debugpoint_work_init();

	k_spinlock_key_t key = k_spin_lock(&state_lock);

	if (!handle_matches(&slots[slot], handle) ||
	    slots[slot].state == DEBUGPOINT_SLOT_FREE) {
		k_spin_unlock(&state_lock, key);
		k_mutex_unlock(&debugpoint_lock);
		return 0;
	}

	slots[slot].state = DEBUGPOINT_SLOT_RETIRING;
	k_spin_unlock(&state_lock, key);

	int ret = cleanup_retiring_slots();

	k_mutex_unlock(&debugpoint_lock);
	return ret;
}

void z_debugpoint_hit(z_debugpoint_handle_t handle,
		      const struct z_debugpoint_event *event)
{
	int slot = handle_slot(handle);

	if (event == NULL || slot < 0) {
		return;
	}

	bool rearm_required = event->rearm_required;

	struct z_debugpoint_slot *entry = &slots[slot];
	k_spinlock_key_t key = k_spin_lock(&state_lock);

	if (!handle_matches(entry, handle) || entry->state != DEBUGPOINT_SLOT_LIVE) {
		k_spin_unlock(&state_lock, key);
		return;
	}

	/*
	 * Retire one-shot points before the callback blocks later hits.
	 * hit_count pins this generation after state_lock is dropped.
	 */
	if (rearm_required) {
		entry->state = DEBUGPOINT_SLOT_RETIRING;
	}
	entry->hit_count++;
	struct z_debugpoint_config config = entry->config;

	k_spin_unlock(&state_lock, key);

	unsigned int cpu = arch_curr_cpu()->id;

	atomic_inc(&callback_depth[cpu]);
	config.callback(&config, event, config.user_data);

	key = k_spin_lock(&state_lock);
	if (handle_matches(entry, handle)) {
		entry->hit_count--;
	}
	bool cleanup = rearm_required && handle_matches(entry, handle);

	k_spin_unlock(&state_lock, key);
	atomic_dec(&callback_depth[cpu]);

	if (cleanup) {
		(void)k_work_submit(&cleanup_work);
	}
}
