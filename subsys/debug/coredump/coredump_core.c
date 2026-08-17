/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <kernel_internal.h>
#include <zephyr/toolchain.h>
#include <zephyr/debug/coredump.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "coredump_internal.h"
#if defined(CONFIG_DEBUG_COREDUMP_BACKEND_LOGGING)
extern struct coredump_backend_api coredump_backend_logging;
static struct coredump_backend_api
	*backend_api = &coredump_backend_logging;
#elif defined(CONFIG_DEBUG_COREDUMP_BACKEND_LOGGING_UDP)
extern struct coredump_backend_api coredump_backend_logging_udp;
static struct coredump_backend_api *backend_api = &coredump_backend_logging_udp;
#elif defined(CONFIG_DEBUG_COREDUMP_BACKEND_FLASH_PARTITION)
extern struct coredump_backend_api coredump_backend_flash_partition;
static struct coredump_backend_api
	*backend_api = &coredump_backend_flash_partition;
#elif defined(CONFIG_DEBUG_COREDUMP_BACKEND_INTEL_ADSP_MEM_WINDOW)
extern struct coredump_backend_api coredump_backend_intel_adsp_mem_window;
static struct coredump_backend_api
	*backend_api = &coredump_backend_intel_adsp_mem_window;
#elif defined(CONFIG_DEBUG_COREDUMP_BACKEND_IN_MEMORY)
extern struct coredump_backend_api coredump_backend_in_memory;
static struct coredump_backend_api
	*backend_api = &coredump_backend_in_memory;
#elif defined(CONFIG_DEBUG_COREDUMP_BACKEND_OTHER)
extern struct coredump_backend_api coredump_backend_other;
static struct coredump_backend_api
	*backend_api = &coredump_backend_other;
#else
#error "Need to select a coredump backend"
#endif

#if defined(CONFIG_COREDUMP_DEVICE)
#include <zephyr/drivers/coredump.h>
#define DT_DRV_COMPAT zephyr_coredump
#endif

/*
 * Upper bound on the number of threads snapshotted (see the SMP path in
 * process_memory_region_list()) while z_thread_monitor_lock is held, before
 * releasing it and running the slow per-thread dump I/O lock-free. Exposed as
 * CONFIG_DEBUG_COREDUMP_THREAD_SNAPSHOT_MAX so a system with more monitored
 * threads than the default can capture all of them (at the cost of that many
 * pointers of fatal-path stack); systems that exceed it dump only the first N.
 */
#if defined(CONFIG_DEBUG_COREDUMP_THREAD_SNAPSHOT_MAX)
#define COREDUMP_THREAD_SNAPSHOT_MAX CONFIG_DEBUG_COREDUMP_THREAD_SNAPSHOT_MAX
#else
#define COREDUMP_THREAD_SNAPSHOT_MAX 64
#endif

#if defined(CONFIG_DEBUG_COREDUMP_THREAD_STACK_TOP_LIMIT_FOR_CURRENT) &&                           \
	CONFIG_DEBUG_COREDUMP_THREAD_STACK_TOP_LIMIT_FOR_CURRENT >= 0
#define STACK_TOP_LIMIT_FOR_CURRENT                                                                \
	((size_t)CONFIG_DEBUG_COREDUMP_THREAD_STACK_TOP_LIMIT_FOR_CURRENT)
#else
#define STACK_TOP_LIMIT_FOR_CURRENT SIZE_MAX
#endif

#if defined(CONFIG_DEBUG_COREDUMP_THREAD_STACK_TOP_LIMIT) &&                                       \
	CONFIG_DEBUG_COREDUMP_THREAD_STACK_TOP_LIMIT >= 0
#define STACK_TOP_LIMIT ((size_t)CONFIG_DEBUG_COREDUMP_THREAD_STACK_TOP_LIMIT)
#else
#define STACK_TOP_LIMIT SIZE_MAX
#endif

#if defined(CONFIG_DEBUG_COREDUMP_DUMP_THREAD_PRIV_STACK)
__weak void arch_coredump_priv_stack_dump(struct k_thread *thread)
{
	/* Stub if architecture has not implemented this. */
	ARG_UNUSED(thread);
}
#endif /* CONFIG_DEBUG_COREDUMP_DUMP_THREAD_PRIV_STACK */

static void dump_header(unsigned int reason)
{
	struct coredump_hdr_t hdr = {
		.id = {'Z', 'E'},
		.hdr_version = COREDUMP_HDR_VER,
		.reason = sys_cpu_to_le16(reason),
	};

	if (sizeof(uintptr_t) == 8) {
		hdr.ptr_size_bits = 6; /* 2^6 = 64 */
	} else if (sizeof(uintptr_t) == 4) {
		hdr.ptr_size_bits = 5; /* 2^5 = 32 */
	} else {
		hdr.ptr_size_bits = 0; /* Unknown */
	}

	hdr.tgt_code = sys_cpu_to_le16(arch_coredump_tgt_code_get());

	backend_api->buffer_output((uint8_t *)&hdr, sizeof(hdr));
}

#if defined(CONFIG_DEBUG_COREDUMP_MEMORY_DUMP_MIN) ||                                              \
	defined(CONFIG_DEBUG_COREDUMP_MEMORY_DUMP_THREADS)

static inline void select_stack_region(const struct k_thread *thread, bool is_current,
				       uintptr_t *start, uintptr_t *end)
{
	uintptr_t sp;
	size_t limit;

	*start = thread->stack_info.start;
	*end = thread->stack_info.start + thread->stack_info.size;

	if (!IS_ENABLED(CONFIG_DEBUG_COREDUMP_THREAD_STACK_TOP)) {
		return;
	}

	sp = arch_coredump_stack_ptr_get(thread);

	if (IN_RANGE(sp, *start, *end)) {
		/* Skip ahead to the stack pointer. */
		*start = sp;
	}

	/* Make sure no more than STACK_TOP_LIMIT[_FOR_CURRENT] bytes of the stack are dumped. */
	limit = (is_current ? STACK_TOP_LIMIT_FOR_CURRENT : STACK_TOP_LIMIT);
	*end = *start + MIN((size_t)(*end - *start), limit);
}

static void dump_thread(struct k_thread *thread, bool is_current)
{
	uintptr_t start_addr;
	uintptr_t end_addr;

	/*
	 * When dumping minimum information,
	 * the current thread struct and stack need to
	 * be dumped so debugger can examine them.
	 */

	if (thread == NULL) {
		return;
	}

	start_addr = POINTER_TO_UINT(thread);
	end_addr = start_addr + sizeof(*thread);
	coredump_memory_dump(start_addr, end_addr);

	select_stack_region(thread, is_current, &start_addr, &end_addr);
	coredump_memory_dump(start_addr, end_addr);

#if defined(CONFIG_DEBUG_COREDUMP_DUMP_THREAD_PRIV_STACK)
	if ((thread->base.user_options & K_USER) == K_USER) {
		arch_coredump_priv_stack_dump(thread);
	}
#endif /* CONFIG_DEBUG_COREDUMP_DUMP_THREAD_PRIV_STACK */
}
#endif

#if defined(CONFIG_COREDUMP_DEVICE)
static void process_coredump_dev_memory(const struct device *dev)
{
	DEVICE_API_GET(coredump, dev)->dump(dev);
}
#endif

void process_memory_region_list(struct k_thread *current)
{
#ifdef CONFIG_DEBUG_COREDUMP_MEMORY_DUMP_LINKER_RAM
	unsigned int idx = 0;

	while (true) {
		struct z_coredump_memory_region_t *r =
			&z_coredump_memory_regions[idx];

		if (r->end == POINTER_TO_UINT(NULL)) {
			break;
		}

		coredump_memory_dump(r->start, r->end);

		idx++;
	}
#endif

#ifdef CONFIG_DEBUG_COREDUMP_MEMORY_DUMP_THREADS
	{
		struct k_thread *thread;
		unsigned int n;
		/*
		 * Defensive bound on the walk below: not a real thread-count
		 * limit, just insurance against an infinite loop if the list
		 * is ever seen mid-corruption (e.g. a concurrent unlink on
		 * another CPU leaves next_thread pointing back into itself).
		 */
		const unsigned int max_threads = 4096U;
#ifdef CONFIG_SMP
		k_spinlock_key_t key = {0};
		bool locked = false;
		/*
		 * Snapshot the thread pointers while holding
		 * z_thread_monitor_lock, then release it before calling
		 * dump_thread() for each snapshotted thread below.
		 * dump_thread() can be slow (e.g. the UDP backend transmits
		 * every thread's stack over the network), and holding this
		 * lock across that I/O starves any other CPU whose thread
		 * naturally exits while the dump is in progress: its
		 * k_thread_abort() -> halt_thread() -> z_thread_monitor_exit()
		 * path needs this same lock, so that CPU would spin for the
		 * entire dump duration instead of completing a normal exit.
		 *
		 * Like k_thread_foreach_unlocked(), the snapshot stabilizes the
		 * list for traversal but does not pin each thread object against
		 * concurrent teardown once the lock is dropped. This is safe here
		 * because it runs only from the fatal path: the panicking CPU has
		 * IRQs locked and does not reschedule, so it cannot itself free a
		 * thread mid-dump, and a peer CPU that aborts a thread blocks in
		 * halt_thread() -> z_thread_monitor_exit() waiting on the very
		 * lock this loop just released and re-derefs nothing until the
		 * dump completes. (The freeze/thaw follow-up additionally holds
		 * the peers frozen for the duration.)
		 */
		struct k_thread *snapshot[COREDUMP_THREAD_SNAPSHOT_MAX];
		unsigned int snapshot_count = 0;

		/*
		 * Try to take z_thread_monitor_lock so the list is stable
		 * against concurrent k_thread_create()/k_thread_abort() on
		 * another CPU. Use trylock rather than a blocking lock: if
		 * the crashing CPU itself already holds this lock (e.g. it
		 * faulted inside k_thread_create()/k_thread_abort()),
		 * blocking here would deadlock the dump on the same CPU and
		 * lose the entire capture. If the lock is contended or held,
		 * fall back to a bounded lock-free walk -- a racy read of a
		 * single thread entry is preferable to no dump at all.
		 *
		 * When CONFIG_SPIN_VALIDATE is enabled, k_spin_trylock()
		 * asserts (via z_spinlock_validate_pre()) that the current
		 * CPU does not already hold the lock. In the fatal path the
		 * crashing CPU may indeed already hold it, so calling trylock
		 * would trip the assert and recurse into the fault handler.
		 * Probe with z_spin_lock_valid() first and skip locking in
		 * that case, falling back to the bounded lock-free walk.
		 */
#ifdef CONFIG_SPIN_VALIDATE
		if (z_spin_lock_valid(&z_thread_monitor_lock)) {
			locked = k_spin_trylock(&z_thread_monitor_lock, &key) == 0;
		}
#else
		locked = k_spin_trylock(&z_thread_monitor_lock, &key) == 0;
#endif /* CONFIG_SPIN_VALIDATE */

		for (thread = _kernel.threads, n = 0;
		     (thread != NULL) && (n < max_threads);
		     thread = thread->next_thread, n++) {
			if (snapshot_count < COREDUMP_THREAD_SNAPSHOT_MAX) {
				snapshot[snapshot_count++] = thread;
			}
			/*
			 * If the list is longer than the snapshot capacity the
			 * dump is truncated to the first N threads rather than
			 * risking unbounded fatal-path stack; raise
			 * CONFIG_DEBUG_COREDUMP_THREAD_SNAPSHOT_MAX to capture
			 * more. A mid-dump diagnostic is deliberately not
			 * emitted here: it would inject stray bytes into the
			 * backend's coredump stream and break host-side parsing.
			 */
		}

		if (locked) {
			k_spin_unlock(&z_thread_monitor_lock, key);
		}

		for (n = 0; n < snapshot_count; n++) {
			dump_thread(snapshot[n], snapshot[n] == current);
		}
#else
		/*
		 * On uniprocessor, IRQs are already locked by the exception
		 * entry path so no other context can modify the list.
		 */
		for (thread = _kernel.threads, n = 0;
		     (thread != NULL) && (n < max_threads);
		     thread = thread->next_thread, n++) {
			dump_thread(thread, thread == current);
		}
#endif /* CONFIG_SMP */

		/*
		 * Dump the ISR (interrupt) stack for every CPU so that crashes
		 * occurring inside an interrupt handler on any core are captured.
		 * On uniprocessor this is identical to the original single-CPU
		 * dump; on SMP it covers all CONFIG_MP_MAX_NUM_CPUS cores.
		 */
		for (int cpu = 0; cpu < CONFIG_MP_MAX_NUM_CPUS; cpu++) {
			char *irq_sp = _kernel.cpus[cpu].irq_stack;
			uintptr_t isr_start =
				POINTER_TO_UINT(irq_sp) - CONFIG_ISR_STACK_SIZE;

			coredump_memory_dump(isr_start, POINTER_TO_UINT(irq_sp));
		}
	}
#endif /* CONFIG_DEBUG_COREDUMP_MEMORY_DUMP_THREADS */

#if defined(CONFIG_COREDUMP_DEVICE)
#define MY_FN(inst) process_coredump_dev_memory(DEVICE_DT_INST_GET(inst));
	DT_INST_FOREACH_STATUS_OKAY(MY_FN)
#endif
}

#ifdef CONFIG_DEBUG_COREDUMP_THREADS_METADATA
static void dump_threads_metadata(void)
{
	struct coredump_threads_meta_hdr_t hdr = {
		.id = THREADS_META_HDR_ID,
		.hdr_version = THREADS_META_HDR_VER,
		.num_bytes = 0,
	};

	hdr.num_bytes += sizeof(_kernel);

	coredump_buffer_output((uint8_t *)&hdr, sizeof(hdr));
	coredump_buffer_output((uint8_t *)&_kernel, sizeof(_kernel));
}
#endif /* CONFIG_DEBUG_COREDUMP_THREADS_METADATA */

void coredump(unsigned int reason, const struct arch_esf *esf,
	      struct k_thread *thread)
{
	z_coredump_start();

	dump_header(reason);

	if (esf != NULL) {
		arch_coredump_info_dump(esf);
	}

#ifdef CONFIG_DEBUG_COREDUMP_THREADS_METADATA
	dump_threads_metadata();
#endif

	if (thread != NULL) {
#ifdef CONFIG_DEBUG_COREDUMP_MEMORY_DUMP_MIN
		dump_thread(thread, /* is_current */ true);
#endif
	}

	process_memory_region_list(thread);

	z_coredump_end();
}

void z_coredump_start(void)
{
	backend_api->start();
}

void z_coredump_end(void)
{
	backend_api->end();
}

void coredump_buffer_output(uint8_t *buf, size_t buflen)
{
	if ((buf == NULL) || (buflen == 0)) {
		/* Invalid buffer, skip */
		return;
	}

	backend_api->buffer_output(buf, buflen);
}

void coredump_memory_dump(uintptr_t start_addr, uintptr_t end_addr)
{
	struct coredump_mem_hdr_t m;
	size_t len;

	if ((start_addr == POINTER_TO_UINT(NULL)) ||
	    (end_addr == POINTER_TO_UINT(NULL))) {
		return;
	}

	if (start_addr >= end_addr) {
		return;
	}

	len = end_addr - start_addr;

	m.id = COREDUMP_MEM_HDR_ID;
	m.hdr_version = COREDUMP_MEM_HDR_VER;

	if (sizeof(uintptr_t) == 8) {
		m.start	= sys_cpu_to_le64(start_addr);
		m.end = sys_cpu_to_le64(end_addr);
	} else if (sizeof(uintptr_t) == 4) {
		m.start	= sys_cpu_to_le32(start_addr);
		m.end = sys_cpu_to_le32(end_addr);
	}

	coredump_buffer_output((uint8_t *)&m, sizeof(m));

	coredump_buffer_output((uint8_t *)start_addr, len);
}

int coredump_query(enum coredump_query_id query_id, void *arg)
{
	int ret;

	if (backend_api->query == NULL) {
		ret = -ENOTSUP;
	} else {
		ret = backend_api->query(query_id, arg);
	}

	return ret;
}

int coredump_cmd(enum coredump_cmd_id cmd_id, void *arg)
{
	int ret;

	if (backend_api->cmd == NULL) {
		ret = -ENOTSUP;
	} else {
		ret = backend_api->cmd(cmd_id, arg);
	}

	return ret;
}
