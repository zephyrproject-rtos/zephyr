/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Runtime lock dependency validator
 */

#include <stdarg.h>
#include <string.h>
#include <zephyr/debug/lockdep.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>
#include <kernel_internal.h>

struct lockdep_instance {
	const void *lock;
	enum z_lockdep_type type;
};

struct lockdep_dependency {
	uintptr_t acquire_ip;
	uint16_t from;
	uint16_t to;
};

struct lockdep_cpu_state {
	uint8_t depth;
	uint8_t overflow_depth;
	bool panic_pending;
	size_t report_len;
	char report[CONFIG_LOCKDEP_REPORT_BUFFER_SIZE];
	struct z_lockdep_held_lock held[CONFIG_LOCKDEP_MAX_HELD_LOCKS];
};

struct lockdep_guard {
	unsigned int irq_key;
	uint8_t cpu_id;
};

enum lockdep_overflow {
	LOCKDEP_OVERFLOW_INSTANCES = BIT(0),
	LOCKDEP_OVERFLOW_DEPENDENCIES = BIT(1),
	LOCKDEP_OVERFLOW_HELD = BIT(2),
};

static atomic_t graph_lock;
static uint8_t lockdep_recursion[CONFIG_MP_MAX_NUM_CPUS];
static struct lockdep_cpu_state cpu_states[CONFIG_MP_MAX_NUM_CPUS];

static struct lockdep_instance instances[CONFIG_LOCKDEP_MAX_LOCKS];
static struct lockdep_dependency dependencies[CONFIG_LOCKDEP_MAX_DEPENDENCIES];
static uint16_t instance_count;
static uint16_t dependency_count;
static uint8_t overflow_mask;
static struct z_lockdep_stats lockdep_stats;

/* Graph searches are serialized by the guard, so one static workspace is enough. */
static uint16_t search_queue[CONFIG_LOCKDEP_MAX_LOCKS];
static uint16_t search_parent[CONFIG_LOCKDEP_MAX_LOCKS];
static uint16_t search_path[CONFIG_LOCKDEP_MAX_LOCKS];
static bool search_seen[CONFIG_LOCKDEP_MAX_LOCKS];

static const char *type_name(enum z_lockdep_type type)
{
	return type == Z_LOCKDEP_MUTEX ? "mutex" : "spinlock";
}

static bool lockdep_guard_enter(struct lockdep_guard *guard)
{
	guard->irq_key = arch_irq_lock();
	guard->cpu_id = _current_cpu->id;

	if (lockdep_recursion[guard->cpu_id] != 0U) {
		arch_irq_unlock(guard->irq_key);
		return false;
	}

	lockdep_recursion[guard->cpu_id] = 1U;
	/* UP is serialized by the IRQ lock; C atomics would reenter spinlock validation. */
	if (IS_ENABLED(CONFIG_SMP)) {
		while (!atomic_cas(&graph_lock, 0, 1)) {
			arch_spin_relax();
		}
	}
	cpu_states[guard->cpu_id].report_len = 0U;
	cpu_states[guard->cpu_id].report[0] = '\0';
	cpu_states[guard->cpu_id].panic_pending = false;

	return true;
}

static void lockdep_guard_exit(const struct lockdep_guard *guard)
{
	struct lockdep_cpu_state *cpu_state = &cpu_states[guard->cpu_id];
	bool panic_pending = cpu_state->panic_pending;

	if (IS_ENABLED(CONFIG_SMP)) {
		(void)atomic_clear(&graph_lock);
	}
	if (cpu_state->report_len > 0U) {
		printk("%s", cpu_state->report);
	}
	lockdep_recursion[guard->cpu_id] = 0U;
	arch_irq_unlock(guard->irq_key);
	if (panic_pending) {
		k_panic();
	}
}

static void report_append(const char *format, ...)
{
	struct lockdep_cpu_state *cpu_state = &cpu_states[_current_cpu->id];
	size_t remaining = sizeof(cpu_state->report) - cpu_state->report_len;
	va_list args;
	int length;

	if (remaining <= 1U) {
		return;
	}

	va_start(args, format);
	length = vsnprintk(&cpu_state->report[cpu_state->report_len], remaining, format, args);
	va_end(args);

	if (length > 0) {
		cpu_state->report_len += MIN((size_t)length, remaining - 1U);
	}
}

static void print_instance(uint16_t instance_id)
{
	const struct lockdep_instance *instance = &instances[instance_id];

	report_append("%s %p", type_name(instance->type), instance->lock);
}

static bool report_error(const char *reason)
{
	lockdep_stats.errors++;
	cpu_states[_current_cpu->id].panic_pending = IS_ENABLED(CONFIG_LOCKDEP_PANIC_ON_ERROR);
	report_append("\n========================================\n");
	report_append("WARNING: lockdep: %s\n", reason);
	report_append("========================================\n");
	return IS_ENABLED(CONFIG_LOCKDEP_PANIC_ON_ERROR);
}

static void report_overflow(enum lockdep_overflow overflow, const char *table, uint32_t limit)
{
	if ((overflow_mask & overflow) != 0U) {
		return;
	}

	overflow_mask |= overflow;
	lockdep_stats.overflows++;
	report_append("lockdep: %s capacity exhausted (limit %u); validation is now incomplete\n",
		      table, limit);
}

static int find_instance(const void *lock)
{
	for (uint16_t i = 0U; i < instance_count; i++) {
		if (instances[i].lock == lock) {
			return i;
		}
	}

	return -1;
}

static struct lockdep_instance *register_instance(const void *lock, enum z_lockdep_type type)
{
	int instance_id = find_instance(lock);

	if (instance_id >= 0) {
		if (instances[instance_id].type != type) {
			(void)report_error("one lock instance was used as different lock types");
			return NULL;
		}
		return &instances[instance_id];
	}

	if (instance_count == ARRAY_SIZE(instances)) {
		report_overflow(LOCKDEP_OVERFLOW_INSTANCES, "lock instance table",
				CONFIG_LOCKDEP_MAX_LOCKS);
		return NULL;
	}

	instance_id = instance_count++;
	instances[instance_id] = (struct lockdep_instance){
		.lock = lock,
		.type = type,
	};
	lockdep_stats.lock_instances = instance_count;

	return &instances[instance_id];
}

static bool find_dependency_path(uint16_t from, uint16_t to, uint16_t *path_len)
{
	uint16_t head = 0U;
	uint16_t tail = 0U;

	memset(search_seen, 0, sizeof(search_seen));
	for (uint16_t i = 0U; i < instance_count; i++) {
		search_parent[i] = UINT16_MAX;
	}

	search_queue[tail++] = from;
	search_seen[from] = true;

	while (head < tail) {
		uint16_t current = search_queue[head++];

		if (current == to) {
			uint16_t length = 0U;

			for (uint16_t node = to; node != UINT16_MAX; node = search_parent[node]) {
				search_path[length++] = node;
			}
			for (uint16_t i = 0U; i < length / 2U; i++) {
				uint16_t tmp = search_path[i];

				search_path[i] = search_path[length - i - 1U];
				search_path[length - i - 1U] = tmp;
			}
			*path_len = length;
			return true;
		}

		for (uint16_t i = 0U; i < dependency_count; i++) {
			uint16_t next;

			if (dependencies[i].from != current) {
				continue;
			}
			next = dependencies[i].to;
			if (!search_seen[next]) {
				search_seen[next] = true;
				search_parent[next] = current;
				search_queue[tail++] = next;
			}
		}
	}

	return false;
}

static bool dependency_exists(uint16_t from, uint16_t to)
{
	for (uint16_t i = 0U; i < dependency_count; i++) {
		if ((dependencies[i].from == from) && (dependencies[i].to == to)) {
			return true;
		}
	}

	return false;
}

static uintptr_t dependency_ip(uint16_t from, uint16_t to)
{
	for (uint16_t i = 0U; i < dependency_count; i++) {
		if ((dependencies[i].from == from) && (dependencies[i].to == to)) {
			return dependencies[i].acquire_ip;
		}
	}

	return 0U;
}

static bool add_dependency(const struct z_lockdep_held_lock *held, uint16_t held_id,
			   uint16_t wanted_id, uintptr_t acquire_ip)
{
	uint16_t path_len;
	bool panic;

	if (dependency_exists(held_id, wanted_id)) {
		return false;
	}

	if (find_dependency_path(wanted_id, held_id, &path_len)) {
		panic = report_error("possible circular locking dependency detected");
		report_append("new dependency: ");
		print_instance(held_id);
		report_append(" -> ");
		print_instance(wanted_id);
		report_append(" at %p\n", (void *)acquire_ip);
		report_append("existing dependency path: ");
		for (uint16_t i = 0U; i < path_len; i++) {
			if (i != 0U) {
				report_append(" -> ");
			}
			print_instance(search_path[i]);
		}
		report_append("\n");
		for (uint16_t i = 0U; i + 1U < path_len; i++) {
			report_append("  existing edge %u -> %u was observed at %p\n",
				      search_path[i], search_path[i + 1U],
				      (void *)dependency_ip(search_path[i], search_path[i + 1U]));
		}
		report_append("held lock was acquired at %p\n", (void *)held->acquire_ip);
		return panic;
	}

	if (dependency_count == ARRAY_SIZE(dependencies)) {
		report_overflow(LOCKDEP_OVERFLOW_DEPENDENCIES, "dependency table",
				CONFIG_LOCKDEP_MAX_DEPENDENCIES);
		return false;
	}

	dependencies[dependency_count++] = (struct lockdep_dependency){
		.acquire_ip = acquire_ip,
		.from = held_id,
		.to = wanted_id,
	};
	lockdep_stats.dependencies = dependency_count;

	return false;
}

static bool validate_held_set(const struct z_lockdep_held_lock *held, uint8_t depth,
			      const struct lockdep_instance *wanted, uintptr_t acquire_ip,
			      bool add_edges)
{
	bool panic = false;
	uint16_t wanted_id = (uint16_t)(wanted - instances);

	for (uint8_t i = 0U; i < depth; i++) {
		int held_id;

		if (held[i].lock == wanted->lock) {
			if (wanted->type == Z_LOCKDEP_SPINLOCK) {
				panic |= report_error("recursive spinlock acquisition");
				report_append("trying to acquire spinlock %p again at %p\n",
					      wanted->lock, (void *)acquire_ip);
				return panic;
			}
			continue;
		}

		if (add_edges) {
			held_id = find_instance(held[i].lock);
			if (held_id >= 0) {
				panic |= add_dependency(&held[i], held_id, wanted_id, acquire_ip);
			}
		}
	}

	return panic;
}

static bool validate_dependencies(const struct lockdep_instance *wanted, uintptr_t acquire_ip)
{
	struct lockdep_cpu_state *cpu_state = &cpu_states[_current_cpu->id];
	bool panic = false;

	if (_current != NULL) {
		/* A recursive mutex acquisition cannot wait on another owner. */
		if (wanted->type == Z_LOCKDEP_MUTEX) {
			for (uint8_t i = 0U; i < _current->lockdep.depth; i++) {
				if (_current->lockdep.held[i].lock == wanted->lock) {
					return false;
				}
			}
		}
		panic |= validate_held_set(_current->lockdep.held, _current->lockdep.depth, wanted,
					   acquire_ip, true);
	}
	panic |= validate_held_set(cpu_state->held, cpu_state->depth, wanted, acquire_ip, true);

	return panic;
}

static struct z_lockdep_held_lock *find_held(struct z_lockdep_held_lock *held, uint8_t depth,
					     const void *lock, uint8_t *index)
{
	for (uint8_t i = depth; i > 0U; i--) {
		if (held[i - 1U].lock == lock) {
			*index = i - 1U;
			return &held[i - 1U];
		}
	}

	return NULL;
}

static void record_acquire(struct z_lockdep_held_lock *held, uint8_t *depth,
			   uint8_t *overflow_depth, const struct lockdep_instance *instance,
			   uintptr_t acquire_ip)
{
	uint8_t index;
	struct z_lockdep_held_lock *record = find_held(held, *depth, instance->lock, &index);

	if (record != NULL) {
		record->recursion++;
		return;
	}

	if (*depth == CONFIG_LOCKDEP_MAX_HELD_LOCKS) {
		report_overflow(LOCKDEP_OVERFLOW_HELD, "held lock stack",
				CONFIG_LOCKDEP_MAX_HELD_LOCKS);
		if (*overflow_depth != UINT8_MAX) {
			(*overflow_depth)++;
		}
		return;
	}

	held[*depth] = (struct z_lockdep_held_lock){
		.lock = instance->lock,
		.acquire_ip = acquire_ip,
		.recursion = 1U,
	};
	(*depth)++;
}

static bool record_release(struct z_lockdep_held_lock *held, uint8_t *depth, const void *lock,
			   uint8_t *overflow_depth, bool require_lifo)
{
	uint8_t index;
	struct z_lockdep_held_lock *record = find_held(held, *depth, lock, &index);
	bool panic = false;

	if (record == NULL) {
		if (*overflow_depth > 0U) {
			(*overflow_depth)--;
			return false;
		}
		panic = report_error("lock release has no matching acquisition");
		report_append("released lock instance %p\n", lock);
		return panic;
	}

	if (record->recursion > 1U) {
		record->recursion--;
		return false;
	}

	if (require_lifo && (index != (*depth - 1U))) {
		panic = report_error("spinlocks were released out of order");
		report_append("released spinlock %p while %p was the innermost lock\n", lock,
			      held[*depth - 1U].lock);
	}

	for (uint8_t i = index; i + 1U < *depth; i++) {
		held[i] = held[i + 1U];
	}
	(*depth)--;

	return panic;
}

void z_lockdep_before_acquire(const void *lock, enum z_lockdep_type type, bool trylock,
			      uintptr_t acquire_ip)
{
	struct lockdep_guard guard;
	struct lockdep_instance *instance;
	bool panic = false;
	bool recursive_spin = false;

	if (!lockdep_guard_enter(&guard)) {
		return;
	}

	instance = register_instance(lock, type);
	if (instance != NULL) {
		struct lockdep_cpu_state *cpu_state = &cpu_states[guard.cpu_id];
		uint8_t unused;

		recursive_spin =
			type == Z_LOCKDEP_SPINLOCK &&
			find_held(cpu_state->held, cpu_state->depth, lock, &unused) != NULL;
		if (!trylock) {
			panic = validate_dependencies(instance, acquire_ip);
		} else if (recursive_spin) {
			panic = validate_held_set(cpu_state->held, cpu_state->depth, instance,
						  acquire_ip, false);
		}
	}

	lockdep_guard_exit(&guard);
	if (panic || recursive_spin) {
		k_panic();
	}
}

void z_lockdep_after_acquire(const void *lock, enum z_lockdep_type type, bool trylock,
			     uintptr_t acquire_ip)
{
	struct lockdep_guard guard;
	struct lockdep_instance *instance;
	bool panic = false;

	if (!lockdep_guard_enter(&guard)) {
		return;
	}

	instance = register_instance(lock, type);
	if (instance != NULL) {
		if (trylock) {
			panic = validate_dependencies(instance, acquire_ip);
		}

		if (type == Z_LOCKDEP_MUTEX) {
			if (_current != NULL) {
				record_acquire(_current->lockdep.held, &_current->lockdep.depth,
					       &_current->lockdep.overflow_depth, instance,
					       acquire_ip);
			}
		} else {
			struct lockdep_cpu_state *cpu_state = &cpu_states[guard.cpu_id];

			record_acquire(cpu_state->held, &cpu_state->depth,
				       &cpu_state->overflow_depth, instance, acquire_ip);
		}
	}

	lockdep_guard_exit(&guard);
	if (panic) {
		k_panic();
	}
}

static void release_lock(const void *lock, enum z_lockdep_type type, bool require_lifo)
{
	struct lockdep_guard guard;
	int instance_id;
	bool panic = false;

	if (!lockdep_guard_enter(&guard)) {
		return;
	}
	instance_id = find_instance(lock);
	if (instance_id < 0) {
		goto out;
	}
	if (instances[instance_id].type != type) {
		panic = report_error("one lock instance was released as the wrong lock type");
		goto out;
	}

	if (type == Z_LOCKDEP_MUTEX) {
		if (_current != NULL) {
			panic = record_release(_current->lockdep.held, &_current->lockdep.depth,
					       lock, &_current->lockdep.overflow_depth, false);
		}
	} else {
		struct lockdep_cpu_state *cpu_state = &cpu_states[guard.cpu_id];

		panic = record_release(cpu_state->held, &cpu_state->depth, lock,
				       &cpu_state->overflow_depth, require_lifo);
	}

out:
	lockdep_guard_exit(&guard);
	if (panic) {
		k_panic();
	}
}

void z_lockdep_release(const void *lock, enum z_lockdep_type type)
{
	release_lock(lock, type, type == Z_LOCKDEP_SPINLOCK);
}

void z_lockdep_release_unordered(const void *lock, enum z_lockdep_type type)
{
	release_lock(lock, type, false);
}

void z_lockdep_init_lock(const void *lock)
{
	struct lockdep_guard guard;
	int id;

	if (!lockdep_guard_enter(&guard)) {
		return;
	}

	id = find_instance(lock);
	if (id >= 0) {
		uint16_t last_id = instance_count - 1U;

		/* Reinitialization starts a new lifetime for this lock address. */
		for (uint16_t i = 0U; i < dependency_count;) {
			if ((dependencies[i].from == id) || (dependencies[i].to == id)) {
				dependencies[i] = dependencies[--dependency_count];
			} else {
				i++;
			}
		}
		if (id != last_id) {
			/* Held-lock records store addresses, not instance IDs. */
			instances[id] = instances[last_id];
			for (uint16_t i = 0U; i < dependency_count; i++) {
				if (dependencies[i].from == last_id) {
					dependencies[i].from = id;
				}
				if (dependencies[i].to == last_id) {
					dependencies[i].to = id;
				}
			}
		}
		instance_count--;
		lockdep_stats.lock_instances = instance_count;
		lockdep_stats.dependencies = dependency_count;
	}

	lockdep_guard_exit(&guard);
}

void z_lockdep_init_thread(struct k_thread *thread)
{
	thread->lockdep = (struct z_lockdep_thread_state){};
}

bool z_lockdep_is_held(const void *lock)
{
	struct lockdep_guard guard;
	bool found = false;
	uint8_t unused;

	if ((lock == NULL) || !lockdep_guard_enter(&guard)) {
		return false;
	}

	if (_current != NULL) {
		found = find_held(_current->lockdep.held, _current->lockdep.depth, lock, &unused) !=
			NULL;
	}
	if (!found) {
		struct lockdep_cpu_state *cpu_state = &cpu_states[guard.cpu_id];

		found = find_held(cpu_state->held, cpu_state->depth, lock, &unused) != NULL;
	}

	lockdep_guard_exit(&guard);
	return found;
}

void z_lockdep_stats_get(struct z_lockdep_stats *stats)
{
	struct lockdep_guard guard;

	if ((stats == NULL) || !lockdep_guard_enter(&guard)) {
		return;
	}
	*stats = lockdep_stats;
	lockdep_guard_exit(&guard);
}

void z_lockdep_reset(void)
{
	struct lockdep_guard guard;

	if (!lockdep_guard_enter(&guard)) {
		return;
	}

	instance_count = 0U;
	dependency_count = 0U;
	overflow_mask = 0U;
	lockdep_stats = (struct z_lockdep_stats){};
	/*
	 * Only the calling CPU's state may be cleared. Other CPUs can be inside
	 * a spinlock critical section (scheduler, timer ISR) right now; wiping
	 * their held stacks would make the matching release look unbalanced.
	 */
	cpu_states[guard.cpu_id] = (struct lockdep_cpu_state){};
	if (_current != NULL) {
		_current->lockdep = (struct z_lockdep_thread_state){};
	}

	lockdep_guard_exit(&guard);
}
