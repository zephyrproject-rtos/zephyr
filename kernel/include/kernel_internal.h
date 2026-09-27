/*
 * Copyright (c) 2010-2012, 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Architecture-independent private kernel APIs
 *
 * This file contains private kernel APIs that are not architecture-specific.
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_KERNEL_INTERNAL_H_
#define ZEPHYR_KERNEL_INCLUDE_KERNEL_INTERNAL_H_

#include <errno.h>

#include <zephyr/kernel.h>
#include <kernel_arch_interface.h>
#include <kthread.h>
#include <string.h>

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize per-CPU kernel data */
void z_init_cpu(int id);

/* Initialize a thread */
void z_init_thread_base(struct _thread_base *thread_base, int priority,
			uint32_t initial_state, unsigned int options);


FUNC_NORETURN void z_cstart(void);

/*
 * Lean kernel-internal init model.
 *
 * A subsystem registers a parameterless init function that the boot path runs
 * at a fixed phase, without the SYS_INIT level/priority machinery (kernel init
 * order is fixed at compile time). Each entry is emitted into the subsystem's
 * own translation unit, so it -- and its init -- is linked only when the
 * subsystem itself is linked, preserving the pay-per-use linkage that SYS_INIT
 * provides. The entry holds a single function pointer (no device back-pointer
 * and no priority), so it is half the size of a SYS_INIT init_entry.
 */
struct k_kernel_init_pre_entry {
	void (*init_fn)(void);
};

struct k_kernel_init_post_entry {
	void (*init_fn)(void);
};

/* Register a kernel init function to run before multithreading starts (the
 * PRE_KERNEL phase), ahead of PRE_KERNEL device init.
 */
#define K_KERNEL_INIT_PRE(_fn)                                                 \
	static const STRUCT_SECTION_ITERABLE(k_kernel_init_pre_entry,          \
					     _kernel_init_pre_##_fn) = {       \
		.init_fn = (_fn),                                              \
	}

/* Register a kernel init function to run after the kernel is up (the
 * POST_KERNEL phase), before POST_KERNEL device init.
 */
#define K_KERNEL_INIT_POST(_fn)                                                \
	static const STRUCT_SECTION_ITERABLE(k_kernel_init_post_entry,         \
					     _kernel_init_post_##_fn) = {      \
		.init_fn = (_fn),                                              \
	}

extern FUNC_NORETURN void z_thread_entry(k_thread_entry_t entry,
			  void *p1, void *p2, void *p3);

extern char *z_setup_new_thread(struct k_thread *new_thread,
				k_thread_stack_t *stack, size_t stack_size,
				k_thread_entry_t entry,
				void *p1, void *p2, void *p3,
				int prio, uint32_t options, const char *name);

/**
 * @brief Allocate aligned memory from the current thread's resource pool
 *
 * Threads may be assigned a resource pool, which will be used to allocate
 * memory on behalf of certain kernel and driver APIs. Memory reserved
 * in this way should be freed with k_free().
 *
 * If called from an ISR, the k_malloc() system heap will be used if it exists.
 *
 * @param align Required memory alignment
 * @param size Memory allocation size
 * @return A pointer to the allocated memory, or NULL if there is insufficient
 * RAM in the pool or there is no pool to draw memory from
 */
void *z_thread_aligned_alloc(size_t align, size_t size);

/**
 * @brief Allocate some memory from the current thread's resource pool
 *
 * Threads may be assigned a resource pool, which will be used to allocate
 * memory on behalf of certain kernel and driver APIs. Memory reserved
 * in this way should be freed with k_free().
 *
 * If called from an ISR, the k_malloc() system heap will be used if it exists.
 *
 * @param size Memory allocation size
 * @return A pointer to the allocated memory, or NULL if there is insufficient
 * RAM in the pool or there is no pool to draw memory from
 */
void *z_thread_malloc(size_t size);


#ifdef CONFIG_USE_SWITCH
/* This is a arch function traditionally, but when the switch-based
 * z_swap() is in use it's a simple inline provided by the kernel.
 */
static ALWAYS_INLINE void
arch_thread_return_value_set(struct k_thread *thread, unsigned int value)
{
	thread->swap_retval = value;
}
#endif

static ALWAYS_INLINE void
z_thread_return_value_set_with_data(struct k_thread *thread,
				   unsigned int value,
				   void *data)
{
	arch_thread_return_value_set(thread, value);
	thread->base.swap_data = data;
}

#ifdef CONFIG_SMP
extern void z_smp_init(void);
#ifdef CONFIG_SYS_CLOCK_EXISTS
extern void smp_timer_init(void);
#endif /* CONFIG_SYS_CLOCK_EXISTS */
#endif /* CONFIG_SMP */

extern void z_early_rand_get(uint8_t *buf, size_t length);

#if defined(CONFIG_STACK_POINTER_RANDOM) && (CONFIG_STACK_POINTER_RANDOM != 0)
extern int z_stack_adjust_initialized;
#endif /* CONFIG_STACK_POINTER_RANDOM */

#ifdef CONFIG_MULTITHREADING
extern struct k_thread z_main_thread;
#endif /* CONFIG_MULTITHREADING */

#ifdef CONFIG_ARCH_HAS_STACK_JUMP
/* reset SP and enter fn(arg), abandoning every frame below; never returns */
FUNC_NORETURN void arch_stack_jump(void *sp, void (*fn)(void *), void *arg);
#endif /* CONFIG_ARCH_HAS_STACK_JUMP */

#ifdef CONFIG_PROCESS
/* the three kernel-reserved processes: idle, init, and the kthread process */
extern struct k_process z_proc_idle;
extern struct k_process z_proc_init;
extern struct k_process z_proc_kth;

/* initialize a record and place it in the process tree (under the process lock) */
void z_kernel_process_init_one(struct k_process *proc, struct k_process *parent);

/* Add a new thread to its creator's process (thread group) */
void z_process_init_thread(struct k_thread *thread);
void z_process_user_mode_enter(struct k_thread *thread);

/* Remove a dying thread from its process; zombifies the process on last exit */
void z_process_exit_thread(struct k_thread *thread);

/* Terminate the calling thread's process with a "killed by signal" wait status */
FUNC_NORETURN void z_process_exit_signaled(int sig);

/*
 * Release-side providers and the number query, implemented by lib/os. The
 * kernel never calls an allocator: it hands memory back to the provider that
 * owns it, and -EINVAL identifies memory the provider does not own (static or
 * caller-provided; reusable where it lives). Called under the process lock;
 * may also run under the scheduler lock and must defer heap reclamation.
 */
#ifdef CONFIG_SYS_PROCESS
int z_process_record_free(struct k_process *proc);
int z_pgrp_cell_free(struct k_process_group *grp);
int z_session_cell_free(struct k_session *session);
#else
static inline int z_process_record_free(struct k_process *proc)
{
	ARG_UNUSED(proc);
	return -EINVAL;
}

static inline int z_pgrp_cell_free(struct k_process_group *grp)
{
	ARG_UNUSED(grp);
	return -EINVAL;
}

static inline int z_session_cell_free(struct k_session *session)
{
	ARG_UNUSED(session);
	return -EINVAL;
}
#endif /* CONFIG_SYS_PROCESS */

/*
 * Per-process descriptor-table providers (zvfs). The exit sweep runs in the
 * dying thread's own context and may block; the drop runs under the process
 * lock and only detaches, deferring closes and frees to the provider.
 */
#if defined(CONFIG_ZVFS_FDTABLE) && defined(CONFIG_PROCESS)
void z_process_fds_exit(void);
void z_process_fds_drop(struct k_process *proc);
#else
static inline void z_process_fds_exit(void)
{
}

static inline void z_process_fds_drop(struct k_process *proc)
{
	ARG_UNUSED(proc);
}
#endif /* CONFIG_ZVFS_FDTABLE && CONFIG_PROCESS */

/* Per-process memory-mapping providers (zvfs), with the same contract */
#if defined(CONFIG_ZVFS_MMAP) && defined(CONFIG_PROCESS)
void z_process_mmap_exit(void);
void z_process_mmap_drop(struct k_process *proc);
#else
static inline void z_process_mmap_exit(void)
{
}

static inline void z_process_mmap_drop(struct k_process *proc)
{
	ARG_UNUSED(proc);
}
#endif /* CONFIG_ZVFS_MMAP && CONFIG_PROCESS */

#ifdef CONFIG_USERSPACE
/* permission-principal index management and the reap-time sweep */
int16_t z_process_perm_index_alloc(void);
void z_process_perm_index_free(int16_t index);
void z_process_perms_all_clear(struct k_process *proc);
/* copy a parent process's kernel-object permissions to a child (fork, spawn) */
void z_process_perms_inherit(struct k_process *parent, struct k_process *child);
#endif /* CONFIG_USERSPACE */

/* validate a user-supplied group handle and every member record (under lock) */
int z_pgrp_validate(k_pgrp_t pgrp);
#else
static inline FUNC_NORETURN void z_process_exit_signaled(int sig)
{
	ARG_UNUSED(sig);
	k_thread_abort(k_current_get());
	CODE_UNREACHABLE;
}
#endif /* CONFIG_PROCESS */

#ifdef CONFIG_PROCESS
/* a dead thread's TCB still owned by the process layer: an unreaped process's
 * pid identity
 */
bool z_process_leader_pinned(struct k_thread *thread);
#else
static inline bool z_process_leader_pinned(struct k_thread *thread)
{
	ARG_UNUSED(thread);
	return false;
}
#endif /* CONFIG_PROCESS */

#ifdef CONFIG_USERSPACE
/* thread and stack objects under the kernel-object collector (kernel/userspace.c) */
/* an OWNED object is about to be disposed: its pool drops the bookkeeping (weak in
 * the kernel, provided by lib/os); may run under the scheduler lock
 */
struct k_object;
void z_kobj_owner_release(struct k_object *ko, bool sched_locked);
void z_kobj_halt_begin(const void *obj);
void z_kobj_halt_clear(const void *obj);
void z_kobj_collect(const void *obj, bool sched_locked);
void z_kobj_graveyard_flush(void);
void z_kobj_halt_end(const void *obj, bool sched_locked);
void z_kobj_bind_stack(const void *stack);
void z_kobj_reap(struct k_thread *thread, bool sched_locked);
#else
static inline void z_kobj_halt_begin(const void *obj)
{
	ARG_UNUSED(obj);
}

static inline void z_kobj_halt_clear(const void *obj)
{
	ARG_UNUSED(obj);
}

static inline void z_kobj_collect(const void *obj, bool sched_locked)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(sched_locked);
}

static inline void z_kobj_graveyard_flush(void)
{
}

static inline void z_kobj_halt_end(const void *obj, bool sched_locked)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(sched_locked);
}

static inline void z_kobj_bind_stack(const void *stack)
{
	ARG_UNUSED(stack);
}

static inline void z_kobj_reap(struct k_thread *thread, bool sched_locked)
{
	ARG_UNUSED(thread);
	ARG_UNUSED(sched_locked);
}
#endif /* CONFIG_USERSPACE */

/* Explicit reclaim of a pooled thread (and its pooled stack) when there are no
 * permission bits to count: provided by lib/os, -EINVAL for a thread the pool
 * does not own. Only reached from the graveyard drain and the join epilogue.
 */
int z_sys_thread_reclaim(struct k_thread *thread, bool sched_locked);
/* whether the pool owns this k_thread (weak false in the kernel) */
bool z_sys_thread_owned(const struct k_thread *thread);

#ifdef CONFIG_THREAD_ABORT_NEED_CLEANUP
/* end of halt_thread(): park the thread in the graveyard; nothing is disposed
 * under the scheduler lock. self: it halted on its own stack.
 */
void z_thread_retire(struct k_thread *thread, bool self, bool reap);
/* a reap event (join, detach of a dead thread, process reap): finish any deferred
 * disposal of the thread, then end every reference to it
 */
void z_thread_reap(struct k_thread *thread, bool sched_locked);
/* dispose every graveyard entry, waiting for threads still switching out */
void z_thread_graveyard_flush(void);
#else
static inline void z_thread_retire(struct k_thread *thread, bool self, bool reap)
{
	ARG_UNUSED(thread);
	ARG_UNUSED(self);
	ARG_UNUSED(reap);
}

static inline void z_thread_reap(struct k_thread *thread, bool sched_locked)
{
	z_kobj_reap(thread, sched_locked);
}

static inline void z_thread_graveyard_flush(void)
{
}
#endif /* CONFIG_THREAD_ABORT_NEED_CLEANUP */



K_KERNEL_STACK_ARRAY_DECLARE(z_interrupt_stacks, CONFIG_MP_MAX_NUM_CPUS,
				    CONFIG_ISR_STACK_SIZE);
K_THREAD_STACK_DECLARE(z_main_stack, CONFIG_MAIN_STACK_SIZE);

#ifdef CONFIG_GEN_PRIV_STACKS
extern uint8_t *z_priv_stack_find(k_thread_stack_t *stack);
#endif /* CONFIG_GEN_PRIV_STACKS */

/* Calculate stack usage. */
int z_stack_space_get(const uint8_t *stack_start, size_t size, size_t *unused_ptr);

#if defined(CONFIG_ZVFS_MMAP) && !defined(CONFIG_PROCESS)
/* Release the memory mappings an exiting thread owns (lib/os/zvfs) */
void z_zvfs_mmap_thread_exit(struct k_thread *thread);
#endif /* CONFIG_ZVFS_MMAP && !CONFIG_PROCESS */

#ifdef CONFIG_MEM_WRITE_FAULT_HOOK
/*
 * Offered a write fault on a present page before it is treated as fatal.
 * Returns true when the fault has been resolved and the access may retry.
 * Runs in exception context: it must not block.
 */
bool z_mem_write_fault(void *addr);
#endif /* CONFIG_MEM_WRITE_FAULT_HOOK */

#ifdef CONFIG_USERSPACE
bool z_stack_is_user_capable(k_thread_stack_t *stack);

/* Memory domain setup hook, called from z_setup_new_thread() */
void z_mem_domain_init_thread(struct k_thread *thread);

/* Memory domain teardown hook, called from z_thread_abort() */
void z_mem_domain_exit_thread(struct k_thread *thread);

/* This spinlock:
 *
 * - Protects the full set of active k_mem_domain objects and their contents
 * - Serializes calls to arch_mem_domain_* APIs
 *
 * If architecture code needs to access k_mem_domain structures or the
 * partitions they contain at any other point, this spinlock should be held.
 * Uniprocessor systems can get away with just locking interrupts but this is
 * not recommended.
 */
extern struct k_spinlock z_mem_domain_lock;
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_GDBSTUB
struct gdb_ctx;

/* Should be called by the arch layer. This is the gdbstub main loop
 * and synchronously communicate with gdb on host.
 */
extern int z_gdb_main_loop(struct gdb_ctx *ctx);
#endif /* CONFIG_GDBSTUB */

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
void z_thread_mark_switched_in(void);
void z_thread_mark_switched_out(void);
#else

/**
 * @brief Called after a thread has been selected to run
 */
#define z_thread_mark_switched_in()

/**
 * @brief Called before a thread has been selected to run
 */

#define z_thread_mark_switched_out()

#endif /* CONFIG_INSTRUMENT_THREAD_SWITCHING */

/* Init hook for page frame management, invoked immediately upon entry of
 * main thread, before POST_KERNEL tasks
 */
void z_mem_manage_init(void);

/**
 * @brief Finalize page frame management at the end of boot process.
 */
void z_mem_manage_boot_finish(void);


bool z_handle_obj_poll_events(sys_dlist_t *events, uint32_t state);

#ifdef CONFIG_PM

/* When the kernel is about to go idle, it calls this function to notify the
 * power management subsystem, that the kernel is ready to enter the idle state.
 *
 * At this point, the kernel has disabled interrupts and computed the maximum
 * time the system can remain idle. The function passes the time that the system
 * can remain idle. The SOC interface performs power operations that can be done
 * in the available time. The power management operations must halt execution of
 * the CPU.
 *
 * This function assumes that a wake up event has already been set up by the
 * application.
 *
 * This function is entered with the idle thread's interrupt lock held. If it
 * enters a power state, it returns before the idle thread restores the saved
 * interrupt key. SoC PM code may use architecture helpers around the
 * low-power instruction, but normal IRQ dispatch must remain blocked until
 * that final idle-thread restore.
 *
 * @return True if the system suspended, otherwise return false
 */
bool pm_system_suspend(int32_t ticks);

#endif /* CONFIG_PM */

#ifdef CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM
/**
 * Initialize the timing histograms for demand paging.
 */
void z_paging_histogram_init(void);

/**
 * Increment the counter in the timing histogram.
 *
 * @param hist The timing histogram to be updated.
 * @param cycles Time spent in measured operation.
 */
void z_paging_histogram_inc(struct k_mem_paging_histogram_t *hist,
			    uint32_t cycles);
#endif /* CONFIG_DEMAND_PAGING_TIMING_HISTOGRAM */

#ifdef CONFIG_OBJ_CORE_STATS_THREAD
int z_thread_stats_raw(struct k_obj_core *obj_core, void *stats);
int z_thread_stats_query(struct k_obj_core *obj_core, void *stats);
int z_thread_stats_reset(struct k_obj_core *obj_core);
int z_thread_stats_disable(struct k_obj_core *obj_core);
int z_thread_stats_enable(struct k_obj_core *obj_core);
#endif /* CONFIG_OBJ_CORE_STATS_THREAD */

#ifdef CONFIG_OBJ_CORE_STATS_SYSTEM
int z_cpu_stats_raw(struct k_obj_core *obj_core, void *stats);
int z_cpu_stats_query(struct k_obj_core *obj_core, void *stats);

int z_kernel_stats_raw(struct k_obj_core *obj_core, void *stats);
int z_kernel_stats_query(struct k_obj_core *obj_core, void *stats);
#endif /* CONFIG_OBJ_CORE_STATS_SYSTEM */

#if defined(CONFIG_THREAD_ABORT_NEED_CLEANUP)
/**
 * Finish any deferred disposal of a thread object about to be reused.
 *
 * A thread that halted on its own stack is disposed from the graveyard later;
 * reusing its k_thread for a new thread must complete that first. Should
 * mainly be used during thread creation.
 *
 * @param thread Pointer to thread to be checked.
 */
void k_thread_abort_cleanup_check_reuse(struct k_thread *thread);
#endif /* CONFIG_THREAD_ABORT_NEED_CLEANUP */

#ifdef CONFIG_THREAD_RESULT
/* cleanup handlers, thread-specific storage destructors, then abort; runs in the caller's mode */
FUNC_NORETURN void z_thread_teardown(void);
#endif /* CONFIG_THREAD_RESULT */

#ifdef CONFIG_THREAD_CANCEL
FUNC_NORETURN void z_thread_cancel_actuate(void);
bool z_thread_cancel_try_actuate(void);
/* the trampoline-side action that actuates a cancellation */
void z_thread_cancel_handler(int sig, struct k_sig_info *info, void *ucontext);

static inline bool z_thread_cancel_is_async(struct k_thread *thread)
{
	return (thread->base.user_options & K_CANCEL_ASYNC) != 0U;
}

static inline bool z_thread_cancel_is_enabled(struct k_thread *thread)
{
	return (thread->base.user_options & K_CANCEL_DISABLED) == 0U;
}

/* actuate the pending request when thread next returns from a system call */
static inline void z_thread_cancel_arm(struct k_thread *thread)
{
	atomic_set(&thread->base.cancel_armed, 1);
}
#endif /* CONFIG_THREAD_CANCEL */

#ifdef CONFIG_SYSCALL_RETURN_HOOK
/* the trampoline: fetch the pending action with k_sig_current(), run it, k_sig_return() */
void z_sig_trampoline(void);
#endif /* CONFIG_SYSCALL_RETURN_HOOK */

#ifdef CONFIG_SIGNAL
/* claim the next deliverable signal for the current thread into its context; 1 when claimed */
int z_sig_claim(struct k_thread *tid);
/* a signal may be deliverable to tid (its own or its process's) */
bool z_sig_pending_any(struct k_thread *tid);
#endif /* CONFIG_SIGNAL */

#ifdef CONFIG_TIMER_REALTIME
/* re-target armed absolute-realtime timers after a realtime clock change (internal) */
void z_timer_realtime_settime(void);
#endif /* CONFIG_TIMER_REALTIME */

#ifdef CONFIG_TIMER_SIGNAL
/* attach/detach signal-based expiry notification (internal; used by sys_timer) */
void z_timer_sig_attach(struct k_timer *timer, k_pid_t target, int signo, union k_sig_val value);
void z_timer_sig_detach(struct k_timer *timer);

/* generation-side plumbing between kernel/timer.c and kernel/signal.c (internal) */
int z_sig_queue_timer(struct k_timer *timer, k_pid_t pid, int signo, union k_sig_val value);
void z_sig_purge_timer(struct k_timer *timer);

/* delivery-side hooks: atomics only - must never take the timer lock (internal) */
void z_timer_sig_delivered(struct k_timer *timer);
void z_timer_sig_discarded(struct k_timer *timer);
#endif /* CONFIG_TIMER_SIGNAL */

#if defined(CONFIG_PROCESS) && defined(CONFIG_SIGNAL)
/* generation-side plumbing between kernel/process.c and kernel/signal.c (internal) */
int z_sig_queue_chld(k_tid_t tid, struct k_process *proc, k_pid_t child_pid, int child_status,
		     bool exited);
int z_sig_queue_thread(k_tid_t tid, int signo, union k_sig_val value);
int z_sig_queue_process(struct k_process *proc, int signo, union k_sig_val value);
int z_sig_actions_clone(struct k_process *parent, struct k_process *child, bool all);
void z_sig_process_drop(struct k_process *proc);
#endif /* CONFIG_PROCESS && CONFIG_SIGNAL */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_KERNEL_INCLUDE_KERNEL_INTERNAL_H_ */
