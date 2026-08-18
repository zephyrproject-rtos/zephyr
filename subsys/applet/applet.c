/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/applet/applet.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/libc-hooks.h>

#ifdef CONFIG_APPLET_LLEXT
#include <zephyr/llext/buf_loader.h>
#include <zephyr/llext/llext.h>
#endif

#ifdef CONFIG_APPLET_FATAL_HANDLER
#include <zephyr/fatal.h>
#endif

LOG_MODULE_REGISTER(applet, CONFIG_APPLET_LOG_LEVEL);

K_HEAP_DEFINE(applet_slot_heap, CONFIG_APPLET_HEAP_SIZE);

/*
 * Every public entry point runs under this mutex, so a descriptor is only ever
 * mutated by one thread at a time. applet_join() releases it while it blocks so
 * that a long wait on one applet does not stall operations on another.
 */
static K_MUTEX_DEFINE(applet_lock);

/* Broadcast when an applet's outstanding join count drops back to zero. */
static K_CONDVAR_DEFINE(applet_idle);

static void unload_locked(struct applet *applet_inst);

static struct applet_thread *slot_alloc(void)
{
	struct applet_thread *slot = k_heap_alloc(&applet_slot_heap, sizeof(*slot), K_NO_WAIT);

	if (slot == NULL) {
		return NULL;
	}

	memset(slot, 0, sizeof(*slot));

#ifdef CONFIG_USERSPACE
	slot->thread = k_object_alloc(K_OBJ_THREAD);
#else
	slot->thread = k_heap_alloc(&applet_slot_heap, sizeof(struct k_thread), K_NO_WAIT);
#endif

	if (slot->thread == NULL) {
		k_heap_free(&applet_slot_heap, slot);
		return NULL;
	}

	return slot;
}

static void slot_free(struct applet_thread *slot)
{
	if (slot->thread != NULL) {
#ifdef CONFIG_USERSPACE
		k_object_free(slot->thread);
#else
		k_heap_free(&applet_slot_heap, slot->thread);
#endif
		slot->thread = NULL;
	}
	k_heap_free(&applet_slot_heap, slot);
}

static sys_slist_t applet_list = SYS_SLIST_STATIC_INIT(&applet_list);

/*
 * List integrity only, held just for the pointer updates and traversals. The
 * fatal handler runs in fault context and cannot take applet_lock, so the lists
 * it walks must also be consistent under a spinlock.
 */
static struct k_spinlock applet_list_lock;

static void applet_register(struct applet *applet_inst)
{
	k_spinlock_key_t key = k_spin_lock(&applet_list_lock);

	sys_slist_append(&applet_list, &applet_inst->applet_node);
	k_spin_unlock(&applet_list_lock, key);
}

static void applet_unregister(struct applet *applet_inst)
{
	k_spinlock_key_t key = k_spin_lock(&applet_list_lock);

	(void)sys_slist_find_and_remove(&applet_list, &applet_inst->applet_node);
	k_spin_unlock(&applet_list_lock, key);
}

static int add_partition_locked(struct applet *applet_inst, struct k_mem_partition *part)
{
	if (applet_inst == NULL || part == NULL) {
		return -EINVAL;
	}
	if (applet_inst->state == APPLET_STATE_UNLOADED) {
		return -EINVAL;
	}

#ifdef CONFIG_USERSPACE
	if (!applet_inst->has_domain) {
		return -EINVAL;
	}
	return k_mem_domain_add_partition(&applet_inst->domain, part);
#else
	ARG_UNUSED(part);
	return 0;
#endif
}

static int init_descriptor(struct applet *applet_inst, const char *name,
			   const struct applet_opts *opts, enum applet_kind kind)
{
	memset(applet_inst, 0, sizeof(*applet_inst));

	strncpy(applet_inst->name, name, CONFIG_APPLET_NAME_MAX_LEN);
	applet_inst->name[CONFIG_APPLET_NAME_MAX_LEN] = '\0';

	if (opts != NULL) {
		applet_inst->opts = *opts;
	} else {
		struct applet_opts defaults = APPLET_OPTS_DEFAULT;

		applet_inst->opts = defaults;
	}

	if (applet_inst->opts.entry_sym == NULL) {
		applet_inst->opts.entry_sym = APPLET_ENTRY_SYM;
	}

	applet_inst->kind = kind;
	sys_slist_init(&applet_inst->threads);

#ifdef CONFIG_USERSPACE
	int ret = k_mem_domain_init(&applet_inst->domain, 0, NULL);

	if (ret != 0) {
		LOG_ERR("applet '%s': k_mem_domain_init failed (%d)", applet_inst->name, ret);
		return ret;
	}
	applet_inst->has_domain = true;
#endif

	applet_inst->state = APPLET_STATE_LOADED;
	applet_register(applet_inst);

#if defined(CONFIG_USERSPACE) && defined(Z_LIBC_PARTITION_EXISTS)
	/* User-mode threads of any kind need this for errno/TLS/malloc state. */
	add_partition_locked(applet_inst, &z_libc_partition);
#endif

	return 0;
}

int applet_init(struct applet *applet_inst, const char *name, const struct applet_opts *opts)
{
	if (applet_inst == NULL || name == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&applet_lock, K_FOREVER);

	int ret = init_descriptor(applet_inst, name, opts, APPLET_KIND_NATIVE);

	if (ret == 0) {
		/*
		 * Native applets default to supervisor mode: their entry functions
		 * are arbitrary C code linked into the main image and usually call
		 * APIs that are not user-callable. Callers who really want native
		 * user-mode threads must opt in explicitly via opts.user_mode.
		 */
		if (opts == NULL) {
			applet_inst->opts.user_mode = false;
		}

		LOG_INF("applet '%s': initialised (native)", applet_inst->name);
	}

	k_mutex_unlock(&applet_lock);
	return ret;
}

#ifdef CONFIG_APPLET_LLEXT
static int load_llext_locked(struct applet *applet_inst, const char *name, const void *elf_data,
			     size_t elf_size, const struct applet_opts *opts)
{
	if (applet_inst == NULL || name == NULL || elf_data == NULL || elf_size == 0) {
		return -EINVAL;
	}

	int ret = init_descriptor(applet_inst, name, opts, APPLET_KIND_LLEXT);

	if (ret != 0) {
		return ret;
	}

	struct llext_buf_loader buf_loader = LLEXT_BUF_LOADER((const uint8_t *)elf_data, elf_size);
	const struct llext_load_param ldr_parm = LLEXT_LOAD_PARAM_DEFAULT;

	ret = llext_load(&buf_loader.loader, applet_inst->name, &applet_inst->ext, &ldr_parm);
	if (ret != 0) {
		LOG_ERR("applet '%s': llext_load failed (%d)", applet_inst->name, ret);
		goto err;
	}

#ifdef CONFIG_USERSPACE
	ret = llext_add_domain(applet_inst->ext, &applet_inst->domain);
	if (ret != 0) {
		LOG_ERR("applet '%s': llext_add_domain failed (%d)", applet_inst->name, ret);
		llext_unload(&applet_inst->ext);
		goto err;
	}
#endif /* CONFIG_USERSPACE */

	LOG_INF("applet '%s': loaded (LLEXT)", applet_inst->name);
	return 0;

err:
#ifdef CONFIG_USERSPACE
	if (applet_inst->has_domain) {
		k_mem_domain_deinit(&applet_inst->domain);
		applet_inst->has_domain = false;
	}
#endif
	applet_unregister(applet_inst);
	applet_inst->state = APPLET_STATE_UNLOADED;
	return ret;
}

int applet_load_llext(struct applet *applet_inst, const char *name, const void *elf_data,
		      size_t elf_size, const struct applet_opts *opts)
{
	k_mutex_lock(&applet_lock, K_FOREVER);

	int ret = load_llext_locked(applet_inst, name, elf_data, elf_size, opts);

	k_mutex_unlock(&applet_lock);
	return ret;
}
#endif /* CONFIG_APPLET_LLEXT */

int applet_add_partition(struct applet *applet_inst, struct k_mem_partition *part)
{
	k_mutex_lock(&applet_lock, K_FOREVER);

	int ret = add_partition_locked(applet_inst, part);

	k_mutex_unlock(&applet_lock);
	return ret;
}

#ifdef CONFIG_APPLET_LLEXT
static void create_applet_thread(struct applet *applet_inst, struct applet_thread *slot,
				 uint32_t opts_flags)
{
	if (applet_inst->kind == APPLET_KIND_LLEXT) {
		/* Wrap the entry so the extension's init/fini tables run around it. */
		k_thread_create(slot->thread, slot->stack, slot->stack_size,
				(k_thread_entry_t)&llext_bootstrap, applet_inst->ext,
				slot->entry_fn, slot->arg, slot->priority, opts_flags, K_FOREVER);
		return;
	}

	k_thread_create(slot->thread, slot->stack, slot->stack_size, slot->entry_fn, slot->arg,
			NULL, NULL, slot->priority, opts_flags, K_FOREVER);
}
#else
static void create_applet_thread(struct applet *applet_inst, struct applet_thread *slot,
				 uint32_t opts_flags)
{
	ARG_UNUSED(applet_inst);
	k_thread_create(slot->thread, slot->stack, slot->stack_size, slot->entry_fn, slot->arg,
			NULL, NULL, slot->priority, opts_flags, K_FOREVER);
}
#endif

static int add_thread_internal(struct applet *applet_inst, k_thread_stack_t *stack,
			       size_t stack_size, k_thread_entry_t entry, void *arg,
			       const char *thread_name)
{
	if (applet_inst == NULL || stack == NULL || stack_size == 0 || entry == NULL) {
		return -EINVAL;
	}
	if (applet_inst->state != APPLET_STATE_LOADED) {
		return -EINVAL;
	}

	struct applet_thread *slot = slot_alloc();

	if (slot == NULL) {
		LOG_ERR("applet '%s': unable to allocate slot from heaps"
			"(CONFIG_APPLET_HEAP_SIZE=%d, CONFIG_HEAP_MEM_POOL_SIZE=%d)",
			applet_inst->name, CONFIG_APPLET_HEAP_SIZE, CONFIG_HEAP_MEM_POOL_SIZE);
		return -ENOMEM;
	}

	slot->stack = stack;
	slot->stack_size = stack_size;
	slot->entry_fn = entry;
	slot->arg = arg;
	slot->priority = applet_inst->opts.thread_priority;

	uint32_t opts_flags = 0;

#ifdef CONFIG_USERSPACE
	if (applet_inst->opts.user_mode && applet_inst->has_domain) {
		opts_flags |= K_USER;
	}
#endif

	create_applet_thread(applet_inst, slot, opts_flags);

	LOG_DBG("applet '%s': added thread %p (entry=%p, arg=%p, stack=%p, size=%zu)",
		applet_inst->name, slot->thread, slot->entry_fn, slot->arg, slot->stack,
		slot->stack_size);

#ifdef CONFIG_SMP
	k_thread_cpu_pin(slot->thread, applet_inst->opts.cpu);
	LOG_DBG("applet '%s': thread %p pinned to CPU %d", applet_inst->name, slot->thread,
		applet_inst->opts.cpu);
#endif

	k_thread_name_set(slot->thread, thread_name != NULL ? thread_name : applet_inst->name);

	k_spinlock_key_t key = k_spin_lock(&applet_list_lock);

	sys_slist_append(&applet_inst->threads, &slot->node);
	k_spin_unlock(&applet_list_lock, key);

	applet_inst->thread_count++;
	return 0;
}

int applet_add_thread(struct applet *applet_inst, k_thread_stack_t *stack, size_t stack_size,
		      k_thread_entry_t entry, void *arg, const char *thread_name)
{
	k_mutex_lock(&applet_lock, K_FOREVER);

	int ret = add_thread_internal(applet_inst, stack, stack_size, entry, arg, thread_name);

	k_mutex_unlock(&applet_lock);
	return ret;
}

#ifdef CONFIG_APPLET_LLEXT
static int add_thread_sym_locked(struct applet *applet_inst, k_thread_stack_t *stack,
				 size_t stack_size, const char *entry_sym, void *arg,
				 const char *thread_name)
{
	if (applet_inst == NULL || applet_inst->kind != APPLET_KIND_LLEXT) {
		return -EINVAL;
	}

	if (entry_sym == NULL) {
		entry_sym = applet_inst->opts.entry_sym;
	}
	if (entry_sym == NULL) {
		entry_sym = APPLET_ENTRY_SYM;
	}

	llext_entry_fn_t fn =
		(llext_entry_fn_t)llext_find_sym(&applet_inst->ext->exp_tab, entry_sym);

	if (fn == NULL) {
		LOG_ERR("applet '%s': entry symbol '%s' not found", applet_inst->name, entry_sym);
		return -ENOENT;
	}

	return add_thread_internal(applet_inst, stack, stack_size, (k_thread_entry_t)fn, arg,
				   thread_name);
}

int applet_add_thread_sym(struct applet *applet_inst, k_thread_stack_t *stack, size_t stack_size,
			  const char *entry_sym, void *arg, const char *thread_name)
{
	k_mutex_lock(&applet_lock, K_FOREVER);

	int ret =
		add_thread_sym_locked(applet_inst, stack, stack_size, entry_sym, arg, thread_name);

	k_mutex_unlock(&applet_lock);
	return ret;
}
#endif /* CONFIG_APPLET_LLEXT */

static int start_locked(struct applet *applet_inst)
{
	if (applet_inst == NULL || applet_inst->state != APPLET_STATE_LOADED) {
		return -EINVAL;
	}
	if (applet_inst->thread_count == 0) {
		return -EINVAL;
	}

#ifdef CONFIG_APPLET_LLEXT
	if (applet_inst->kind == APPLET_KIND_LLEXT && !applet_inst->bringup_done) {
		int ret = llext_bringup(applet_inst->ext);

		if (ret != 0) {
			LOG_ERR("applet '%s': llext_bringup failed (%d)", applet_inst->name, ret);
			applet_inst->state = APPLET_STATE_DEAD;
			return ret;
		}
		applet_inst->bringup_done = true;
	}
#endif

	struct applet_thread *slot;

	SYS_SLIST_FOR_EACH_CONTAINER(&applet_inst->threads, slot, node) {
		if (slot->started) {
			continue;
		}

#ifdef CONFIG_USERSPACE
		if (applet_inst->has_domain) {
			k_mem_domain_add_thread(&applet_inst->domain, slot->thread);
		}
#endif

		slot->started = true;
		k_thread_start(slot->thread);
	}

	applet_inst->state = APPLET_STATE_RUNNING;
	LOG_DBG("applet '%s': started (%u thread%s)", applet_inst->name, applet_inst->thread_count,
		applet_inst->thread_count == 1 ? "" : "s");
	return 0;
}

int applet_start(struct applet *applet_inst)
{
	k_mutex_lock(&applet_lock, K_FOREVER);

	int ret = start_locked(applet_inst);

	k_mutex_unlock(&applet_lock);
	return ret;
}

#ifdef CONFIG_APPLET_LLEXT
int applet_load(struct applet *applet_inst, const char *name, const void *elf_data, size_t elf_size,
		k_thread_stack_t *stack, size_t stack_size, const struct applet_opts *opts)
{
	k_mutex_lock(&applet_lock, K_FOREVER);

	int ret = load_llext_locked(applet_inst, name, elf_data, elf_size, opts);

	if (ret == 0) {
		ret = add_thread_sym_locked(applet_inst, stack, stack_size,
					    applet_inst->opts.entry_sym, applet_inst->opts.arg,
					    NULL);
		if (ret != 0) {
			unload_locked(applet_inst);
		}
	}

	k_mutex_unlock(&applet_lock);
	return ret;
}

int applet_spawn(struct applet *applet_inst, const char *name, const void *elf_data,
		 size_t elf_size, k_thread_stack_t *stack, size_t stack_size,
		 const struct applet_opts *opts)
{
	k_mutex_lock(&applet_lock, K_FOREVER);

	int ret = load_llext_locked(applet_inst, name, elf_data, elf_size, opts);

	if (ret == 0) {
		ret = add_thread_sym_locked(applet_inst, stack, stack_size,
					    applet_inst->opts.entry_sym, applet_inst->opts.arg,
					    NULL);
		if (ret != 0) {
			unload_locked(applet_inst);
		} else {
			ret = start_locked(applet_inst);
		}
	}

	k_mutex_unlock(&applet_lock);
	return ret;
}
#endif /* CONFIG_APPLET_LLEXT */

int applet_join(struct applet *applet_inst, k_timeout_t timeout)
{
	if (applet_inst == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&applet_lock, K_FOREVER);

	int ret = 0;

	if (applet_inst->state != APPLET_STATE_RUNNING && applet_inst->state != APPLET_STATE_DEAD) {
		ret = -EINVAL;
		goto out;
	}

	/*
	 * Restart the walk after every wait: the lock is dropped while blocking,
	 * so slots may have been marked joined by someone else meanwhile. The
	 * busy count keeps applet_unload() from freeing the thread object we are
	 * parked on.
	 */
	for (;;) {
		struct applet_thread *slot;
		struct k_thread *target = NULL;

		SYS_SLIST_FOR_EACH_CONTAINER(&applet_inst->threads, slot, node) {
			if (slot->started && !slot->joined) {
				target = slot->thread;
				break;
			}
		}

		if (target == NULL) {
			break;
		}

		applet_inst->join_busy++;
		k_mutex_unlock(&applet_lock);

		ret = k_thread_join(target, timeout);

		k_mutex_lock(&applet_lock, K_FOREVER);
		applet_inst->join_busy--;
		if (applet_inst->join_busy == 0U) {
			k_condvar_broadcast(&applet_idle);
		}

		if (ret != 0) {
			goto out;
		}

		SYS_SLIST_FOR_EACH_CONTAINER(&applet_inst->threads, slot, node) {
			if (slot->thread == target) {
				slot->joined = true;
				break;
			}
		}
	}

	applet_inst->state = APPLET_STATE_DEAD;
out:
	k_mutex_unlock(&applet_lock);
	return ret;
}

/*
 * Threads cannot flag their own exit: a user-mode thread has no access to the
 * descriptor, and an aborted one never runs cleanup code. Sample them instead;
 * k_thread_join() with K_NO_WAIT reports termination from any context and
 * returns -EDEADLK when an applet thread asks about its own applet, which
 * correctly reads as "still running".
 */
static enum applet_state refresh_state(struct applet *applet_inst)
{
	if (applet_inst->state != APPLET_STATE_RUNNING) {
		return applet_inst->state;
	}

	struct applet_thread *slot;

	SYS_SLIST_FOR_EACH_CONTAINER(&applet_inst->threads, slot, node) {
		if (!slot->started || slot->joined) {
			continue;
		}
		if (k_thread_join(slot->thread, K_NO_WAIT) != 0) {
			return APPLET_STATE_RUNNING;
		}
		slot->joined = true;
	}

	applet_inst->state = APPLET_STATE_DEAD;
	return APPLET_STATE_DEAD;
}

enum applet_state applet_get_state(struct applet *applet_inst)
{
	if (applet_inst == NULL) {
		return APPLET_STATE_UNLOADED;
	}

	k_mutex_lock(&applet_lock, K_FOREVER);

	enum applet_state state = refresh_state(applet_inst);

	k_mutex_unlock(&applet_lock);
	return state;
}

static int kill_locked(struct applet *applet_inst)
{
	if (applet_inst == NULL || refresh_state(applet_inst) != APPLET_STATE_RUNNING) {
		return -EINVAL;
	}

	struct applet_thread *slot;

	SYS_SLIST_FOR_EACH_CONTAINER(&applet_inst->threads, slot, node) {
		if (!slot->started || slot->joined) {
			continue;
		}
		k_thread_abort(slot->thread);
		slot->joined = true;
	}

	applet_inst->state = APPLET_STATE_DEAD;

	LOG_INF("applet '%s': killed", applet_inst->name);
	return 0;
}

int applet_kill(struct applet *applet_inst)
{
	k_mutex_lock(&applet_lock, K_FOREVER);

	int ret = kill_locked(applet_inst);

	k_mutex_unlock(&applet_lock);
	return ret;
}

static void unload_locked(struct applet *applet_inst)
{
	if (applet_inst == NULL || applet_inst->state == APPLET_STATE_UNLOADED) {
		return;
	}

	if (refresh_state(applet_inst) == APPLET_STATE_RUNNING) {
		LOG_WRN("applet '%s': unloading while still running; "
			"calling applet_kill() first",
			applet_inst->name);
		kill_locked(applet_inst);
	}

	/*
	 * Aborting the threads above releases any blocked joiner, but a joiner
	 * still holds a pointer to the k_thread we are about to free until it
	 * gets scheduled again.
	 */
	while (applet_inst->join_busy != 0U) {
		k_condvar_wait(&applet_idle, &applet_lock, K_FOREVER);
	}

#ifdef CONFIG_APPLET_LLEXT
	if (applet_inst->kind == APPLET_KIND_LLEXT && applet_inst->ext != NULL) {
		int ret = llext_teardown(applet_inst->ext);

		if (ret != 0) {
			LOG_WRN("applet '%s': llext_teardown failed (%d)", applet_inst->name, ret);
		}
	}
#endif

#ifdef CONFIG_USERSPACE
	if (applet_inst->has_domain) {
		k_mem_domain_deinit(&applet_inst->domain);
	}
#endif

#ifdef CONFIG_APPLET_LLEXT
	if (applet_inst->kind == APPLET_KIND_LLEXT && applet_inst->ext != NULL) {
		llext_unload(&applet_inst->ext);
	}
#endif

	/* Free every per-thread slot */
	for (;;) {
		k_spinlock_key_t key = k_spin_lock(&applet_list_lock);
		sys_snode_t *node = sys_slist_get(&applet_inst->threads);

		k_spin_unlock(&applet_list_lock, key);

		if (node == NULL) {
			break;
		}
		slot_free(CONTAINER_OF(node, struct applet_thread, node));
	}

	applet_unregister(applet_inst);
	LOG_INF("applet '%s': unloaded", applet_inst->name);

	memset(applet_inst, 0, sizeof(*applet_inst));
	applet_inst->state = APPLET_STATE_UNLOADED;
}

void applet_unload(struct applet *applet_inst)
{
	k_mutex_lock(&applet_lock, K_FOREVER);
	unload_locked(applet_inst);
	k_mutex_unlock(&applet_lock);
}

unsigned int applet_thread_count(struct applet *applet_inst)
{
	if (applet_inst == NULL) {
		return 0;
	}

	k_mutex_lock(&applet_lock, K_FOREVER);

	unsigned int count = applet_inst->thread_count;

	k_mutex_unlock(&applet_lock);
	return count;
}

struct k_thread *applet_thread_get(struct applet *applet_inst, unsigned int idx)
{
	if (applet_inst == NULL) {
		return NULL;
	}

	k_mutex_lock(&applet_lock, K_FOREVER);

	struct applet_thread *slot;
	struct k_thread *thread = NULL;
	unsigned int i = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&applet_inst->threads, slot, node) {
		if (i == idx) {
			thread = slot->thread;
			break;
		}
		i++;
	}

	k_mutex_unlock(&applet_lock);
	return thread;
}

#ifdef CONFIG_APPLET_FATAL_HANDLER

/*
 * Runs in fault context, where applet_lock cannot be taken. The spinlock keeps
 * the traversal consistent against concurrent list updates; faulting in an
 * applet that another thread is unloading at the same time is not covered.
 */
static struct applet *find_applet_of_thread(struct k_thread *thread)
{
	k_spinlock_key_t key = k_spin_lock(&applet_list_lock);
	struct applet *applet_inst;
	struct applet *found = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(&applet_list, applet_inst, applet_node) {
		struct applet_thread *slot;

		SYS_SLIST_FOR_EACH_CONTAINER(&applet_inst->threads, slot, node) {
			if (slot->thread == thread) {
				found = applet_inst;
				goto out;
			}
		}
	}
out:
	k_spin_unlock(&applet_list_lock, key);
	return found;
}

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	struct k_thread *cur = k_current_get();
	struct applet *applet_inst = find_applet_of_thread(cur);

	if (applet_inst != NULL) {
		if (applet_inst->opts.halt_on_fault == APPLET_HALT_ON_FAULT_SYSTEM) {
			LOG_PANIC();
			LOG_ERR("applet '%s': fatal error %u in thread %p; "
				"halting system",
				applet_inst->name, reason, (void *)cur);
			k_fatal_halt(reason);
			CODE_UNREACHABLE;
		} else if (applet_inst->opts.halt_on_fault == APPLET_HALT_ON_FAULT_APPLET) {
			LOG_PANIC();
			LOG_ERR("applet '%s': fatal error %u in thread %p; "
				"aborting all threads in applet",
				applet_inst->name, reason, (void *)cur);
			/* Lock-free on purpose: fault context cannot block. */
			kill_locked(applet_inst);
			CODE_UNREACHABLE;
		} else if (applet_inst->opts.halt_on_fault == APPLET_HALT_ON_FAULT_THREAD) {
			LOG_PANIC();
			LOG_ERR("applet '%s': fatal error %u in thread %p; "
				"aborting thread",
				applet_inst->name, reason, (void *)cur);
			k_thread_abort(cur);
			CODE_UNREACHABLE;
		}

		return;
	}

	LOG_PANIC();
	LOG_ERR("Halting system");
	k_fatal_halt(reason);
	CODE_UNREACHABLE;
}

#endif /* CONFIG_APPLET_FATAL_HANDLER */
