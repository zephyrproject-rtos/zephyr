/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/process/process.h>
#include <zephyr/internal/process.h>
#include <zephyr/sys/bitarray.h>
#include <string.h>

#define PID_NONE UINT16_MAX
#define POOL_SIZE CONFIG_PROCESS_POOL_SIZE
#define ARGV_BUF_SIZE CONFIG_PROCESS_ARGV_BUF_SIZE
#define ARGV_BUF_ALIGN CONFIG_PROCESS_ARGV_BUF_ALIGN

static struct k_thread threads[POOL_SIZE];
static K_THREAD_STACK_ARRAY_DEFINE(
	stacks,
	POOL_SIZE,
	CONFIG_PROCESS_STACK_SIZE
);
SYS_BITARRAY_DEFINE_STATIC(pids, POOL_SIZE);
static __aligned(ARGV_BUF_ALIGN) uint8_t argv_bufs[POOL_SIZE][ARGV_BUF_SIZE];
static sys_slist_t list;
static K_SEM_DEFINE(lock, 1, 1);

#ifdef CONFIG_USERSPACE
static struct k_mem_partition partitions[POOL_SIZE];
static struct k_mem_domain domains[POOL_SIZE];
#endif

static struct k_process *find_process_by_name_locked(const char *name)
{
	struct k_process *it;

	SYS_SLIST_FOR_EACH_CONTAINER(&list, it, node) {
		if (strcmp(it->name, name)) {
			continue;
		}

		return it;
	}

	return NULL;
}

static bool process_is_registered_locked(struct k_process *process)
{
	struct k_process *it;

	SYS_SLIST_FOR_EACH_CONTAINER(&list, it, node) {
		if (it != process) {
			continue;
		}

		return true;
	}

	return false;
}

static bool process_is_started_locked(struct k_process *process)
{
	return process->pid != PID_NONE;
}

static bool process_has_exited_locked(struct k_process *process)
{
	return k_thread_join(k_process_get_thread(process), K_NO_WAIT) == 0;
}

static int load_process_locked(struct k_process *process, k_process_entry_t *entry)
{
	return process->load(process, entry);
}

static void unload_process_locked(struct k_process *process)
{
	if (process->unload == NULL) {
		return;
	}

	process->unload(process);
}

static int alloc_process_pid_locked(struct k_process *process)
{
	int ret;
	size_t allocated;

	ret = sys_bitarray_alloc(&pids, 1, &allocated);
	if (ret) {
		return ret;
	}

	process->pid = allocated;
	return 0;
}

static void free_process_pid_locked(struct k_process *process)
{
	sys_bitarray_free(&pids, 1, process->pid);
	process->pid = PID_NONE;
}

static void cleanup_process_locked(struct k_process *process)
{
	if (!process_is_started_locked(process)) {
		return;
	}

	if (!process_has_exited_locked(process)) {
		return;
	}

	unload_process_locked(process);
	free_process_pid_locked(process);
}

static void cleanup_processes_locked(void)
{
	struct k_process *it;

	SYS_SLIST_FOR_EACH_CONTAINER(&list, it, node) {
		cleanup_process_locked(it);
	}
}

static int register_process_locked(struct k_process *process)
{
	if (process_is_registered_locked(process)) {
		return -EALREADY;
	}

	if (find_process_by_name_locked(process->name) != NULL) {
		return -EALREADY;
	}

	sys_slist_append(&list, &process->node);
	return 0;
}

static int unregister_process_locked(struct k_process *process)
{
	if (!process_is_registered_locked(process)) {
		return -ENOENT;
	}

	cleanup_processes_locked();

	if (process_is_started_locked(process)) {
		return -EPERM;
	}

	sys_slist_find_and_remove(&list, &process->node);
	return 0;
}

#if CONFIG_USERSPACE
static void clear_process_args(struct k_process *process)
{
	uint8_t *buf;

	buf = argv_bufs[process->pid];
	memset(buf, 0, ARGV_BUF_SIZE);
}
#endif

static int copy_args_to_process(struct k_process *process, size_t argc, const char **argv)
{
	char *buf;
	const char **ptr;
	size_t pos;
	size_t size;

	buf = (char *)argv_bufs[process->pid];
	ptr = (const char **)buf;
	pos = sizeof(const char *) * argc;

	for (size_t i = 0; i < argc; i++) {
		size = strlen(argv[i]) + 1;
		if (pos + size > ARGV_BUF_SIZE) {
			return -ENOSPC;
		}

		memcpy(&buf[pos], argv[i], size);
		ptr[i] = &buf[pos];
		pos += size;
	}

	return 0;
}

static const char **get_process_argv(struct k_process *process)
{
	return (const char **)argv_bufs[process->pid];
}

#ifdef CONFIG_USERSPACE
struct k_mem_partition *init_and_get_partition_locked(struct k_process *process)
{
	struct k_mem_partition *partition = &partitions[process->pid];
	uint8_t *argv_buf = argv_bufs[process->pid];

	partition->start = (uintptr_t)argv_buf;
	partition->size = ARGV_BUF_SIZE;
	partition->attr = K_MEM_PARTITION_P_RO_U_RO;
	return partition;
}
#endif

static void entry_wrapper(void *p1, void *p2, void *p3)
{
	k_process_entry_t entry = p1;
	size_t argc = (size_t)p2;
	const char **argv = p3;

	entry(argc, argv);
}

const char *k_process_get_name(struct k_process *process)
{
	return process->name;
}

k_tid_t k_process_get_thread(struct k_process *process)
{
	return &threads[process->pid];
}

k_thread_stack_t *k_process_get_thread_stack(struct k_process *process)
{
	return stacks[process->pid];
}

#ifdef CONFIG_USERSPACE
struct k_mem_domain *k_process_get_domain(struct k_process *process)
{
	return &domains[process->pid];
}
#endif

void k_process_init(struct k_process *process,
		    const char *name,
		    k_process_load_t load,
		    k_process_unload_t unload)
{
	process->name = name;
	process->load = load;
	process->unload = unload;
	process->pid = PID_NONE;
	k_object_init(process);
}

int k_process_register(struct k_process *process)
{
	int ret;

	k_sem_take(&lock, K_FOREVER);
	ret = register_process_locked(process);
	k_sem_give(&lock);
	return ret;
}

int k_process_unregister(struct k_process *process)
{
	int ret;

	k_sem_take(&lock, K_FOREVER);
	ret = unregister_process_locked(process);
	k_sem_give(&lock);
	return ret;
}

struct k_process *z_impl_k_process_get(const char *name)
{
	struct k_process *process;

	k_sem_take(&lock, K_FOREVER);
	process = find_process_by_name_locked(name);
	k_sem_give(&lock);
	return process;
}

static int start_process_locked(struct k_process *process, size_t argc, const char **argv)
{
	int ret;
	k_process_entry_t entry;
	uint32_t options;

	if (!process_is_registered_locked(process)) {
		return -ENOENT;
	}

	cleanup_processes_locked();

	ret = alloc_process_pid_locked(process);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_USERSPACE
	clear_process_args(process);
#endif

	ret = copy_args_to_process(process, argc, argv);
	if (ret) {
		free_process_pid_locked(process);
		return ret;
	}

#ifdef CONFIG_USERSPACE
	k_mem_domain_init(k_process_get_domain(process), 0, NULL);
	ret = k_mem_domain_inherit_thread_partitions(k_process_get_domain(process),
						     k_current_get());
	if (ret) {
		free_process_pid_locked(process);
		return ret;
	}

	ret = k_mem_domain_add_partition(k_process_get_domain(process),
					 init_and_get_partition_locked(process));
	if (ret) {
		free_process_pid_locked(process);
		return ret;
	}
#endif

	ret = load_process_locked(process, &entry);
	if (ret) {
		free_process_pid_locked(process);
		return ret;
	}

	options = K_INHERIT_PERMS;

	if (k_thread_is_user_thread(k_current_get())) {
		options |= K_USER;
	}

	k_thread_create(k_process_get_thread(process),
			k_process_get_thread_stack(process),
			K_THREAD_STACK_LEN(CONFIG_PROCESS_STACK_SIZE),
			entry_wrapper,
			entry,
			(void *)argc,
			get_process_argv(process),
			k_thread_priority_get(k_current_get()),
			options,
			K_FOREVER);

#ifdef CONFIG_USERSPACE
	k_mem_domain_add_thread(k_process_get_domain(process),
				k_process_get_thread(process));
#endif

	k_thread_start(k_process_get_thread(process));
	return 0;
}

int z_impl_k_process_start(struct k_process *process, size_t argc, const char **argv)
{
	int ret;

	k_sem_take(&lock, K_FOREVER);
	ret = start_process_locked(process, argc, argv);
	k_sem_give(&lock);
	return ret;
}

static int stop_process_locked(struct k_process *process)
{
	if (!process_is_started_locked(process)) {
		return -EALREADY;
	}

	k_thread_abort(k_process_get_thread(process));
	cleanup_processes_locked();
	return 0;
}

int z_impl_k_process_stop(struct k_process *process)
{
	int ret;

	k_sem_take(&lock, K_FOREVER);
	ret = stop_process_locked(process);
	k_sem_give(&lock);
	return ret;
}
