/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_KERNEL_PROCESS_H_
#define ZEPHYR_INCLUDE_ZEPHYR_KERNEL_PROCESS_H_

/*
 * Not a standalone header; include <zephyr/kernel.h> instead. Types referenced
 * here (k_thread, k_pid_t, k_timeout_t, _wait_q_t) are provided by
 * <zephyr/kernel.h> prior to its inclusion of this file.
 */

#include <zephyr/sys/dlist.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/* handle types stay visible in every configuration: the generated syscall
 * prototypes in <zephyr/syscalls/kernel.h> reference them unconditionally
 */
struct k_process;
struct k_process_group;
struct k_session;

/** @brief Process group handle */
typedef struct k_process_group *k_pgrp_t;
/** @brief Session handle */
typedef struct k_session *k_session_t;

#if defined(CONFIG_PROCESS) || defined(__DOXYGEN__)

/**
 * @brief Kernel Process APIs
 * @defgroup k_process_apis Kernel Process APIs
 *
 * A process is a thread group: its identity is its thread
 * group leader, so a process handle is a thread handle (@ref k_pid_t). The
 * kernel-owned @ref k_process record carries the group state and remains the
 * anchor of the process's identity from creation until the process is reaped
 * with k_waitpid() - including after the leader thread itself has exited.
 *
 * A process handle held after its leader thread exited is an identity value:
 * it may be passed to the process APIs (which never dereference it), but it
 * must not be passed to thread APIs such as k_thread_join().
 *
 * Process groups and sessions are first-class kernel objects (@ref k_pgrp_t,
 * @ref k_session_t): a group is a list of member processes, a session a list
 * of member groups, each created on demand and existing exactly as long as it
 * has members. The kernel deals only in these handles; numeric process and
 * group IDs are not a kernel concern.
 *
 * @ingroup kernel_apis
 * @{
 */

struct k_thread;

/**
 * @name Process states
 * @{
 */
/** @brief Slot is not associated with any process */
#define K_PROCESS_UNUSED 0
/** @brief Process has at least one live thread */
#define K_PROCESS_ACTIVE 1
/** @brief Process is terminating; threads are being aborted */
#define K_PROCESS_EXITING 2
/** @brief Process has terminated but has not yet been reaped via k_waitpid() */
#define K_PROCESS_ZOMBIE 3
/** @} */

/** @cond INTERNAL_HIDDEN */
/** A K_SIG_CHLD notification for this zombie has not yet been emitted */
#define K_PROCESS_FLAG_SIGCHLD BIT(0)
/** @endcond */

/**
 * @name Wait status encoding
 *
 * The kernel-native wait status layout. Normal exit stores the low 8 bits of
 * the exit code in bits [15:8]; termination by signal stores the (kernel)
 * signal number in bits [6:0].
 *
 * @{
 */
/** @brief Encode a normal-exit wait status from @a code */
#define K_WSTATUS_EXITED(code) (((code) & 0xff) << 8)
/** @brief Encode a killed-by-signal wait status from kernel signal @a signo */
#define K_WSTATUS_SIGNALED(signo) ((signo) & 0x7f)
/** @brief True if the process exited normally */
#define K_WIFEXITED(wstatus) (((wstatus) & 0x7f) == 0)
/** @brief Exit code of a normally exited process */
#define K_WEXITSTATUS(wstatus) (((wstatus) >> 8) & 0xff)
/** @brief True if the process was terminated by a signal */
#define K_WIFSIGNALED(wstatus) ((((signed char)(((wstatus) & 0x7f) + 1)) >> 1) > 0)
/** @brief Kernel signal number that terminated the process */
#define K_WTERMSIG(wstatus) ((wstatus) & 0x7f)
/** @} */

/**
 * @name Wait option flags
 * @{
 */
/** @brief Do not block if no child has changed state */
#define K_PROCESS_WNOHANG BIT(0)
/** @brief Report status without reaping the child */
#define K_PROCESS_WNOWAIT BIT(24)
/** @brief Report stopped children (reserved; stop/continue is unsupported) */
#define K_PROCESS_WUNTRACED BIT(1)
/** @brief Report continued children (reserved; stop/continue is unsupported) */
#define K_PROCESS_WCONTINUED BIT(3)
/** @brief Report exited children (the default; accepted for waitid() compatibility) */
#define K_PROCESS_WEXITED BIT(2)
/** @} */


/**
 * @brief Session
 *
 * A list of member process groups. Created by k_setsid(); exists exactly as
 * long as it has member groups. Public only for build-time cell emission;
 * fields are kernel-internal.
 */
struct k_session {
	/** @cond INTERNAL_HIDDEN */
	/** Member process groups (k_process_group.session_node) */
	sys_dlist_t pgrps;
	/** Creating (leader) process's handle: an identity value, never dereferenced */
	k_pid_t leader;
	/** Node in the system layer's free list while unused */
	sys_dnode_t cell_node;
	/** K_SESSION_FLAG_* bits */
	uint8_t flags;
	/** @endcond */
};

/** @cond INTERNAL_HIDDEN */
/** Set by K_SESSION_DEFINE(): the session persists even when empty */
#define K_SESSION_FLAG_STATIC BIT(0)
/** @endcond */

/**
 * @brief Process group
 *
 * A list of member processes within one session. Created by k_setsid() or
 * k_setpgid(); exists exactly as long as it has member processes, so a group
 * (and its identity) naturally survives its creator's exit and reap. Public
 * only for build-time cell emission; fields are kernel-internal.
 */
struct k_process_group {
	/** @cond INTERNAL_HIDDEN */
	/** Member processes (k_process.pgrp_node) */
	sys_dlist_t members;
	/** Creating (leader) process's handle: an identity value, never dereferenced */
	k_pid_t leader;
	/** Session this group belongs to; preset (non-NULL) marks a static group */
	struct k_session *session;
	/** Node in the session's group list */
	sys_dnode_t session_node;
	/** @endcond */
};

/**
 * @brief Process (thread group) record
 *
 * Kernel-owned state of one thread group, anchored by the group leader's
 * handle. The definition is public only so that K_PROCESS_DEFINE() can
 * instantiate records at build time; fields are kernel-internal - use the
 * k_*() process APIs.
 */
struct k_process {
	/** @cond INTERNAL_HIDDEN */
	/**
	 * Thread group leader handle: the process's identity from creation to
	 * reap. After the leader thread exits this is a value compared and
	 * returned by the process APIs, never dereferenced.
	 */
	k_tid_t leader;
	/** Parent process record; NULL only for the idle process */
	struct k_process *parent;
	/** Node in the parent's list of children (reused for deferred frees) */
	sys_dnode_t sibling_node;
	/** Child process records */
	sys_dlist_t children;
	/** Live member threads (the thread group) */
	sys_dlist_t threads;
	/** Process group this process belongs to */
	struct k_process_group *pgrp;
	/** Node in the process group's member list */
	sys_dnode_t pgrp_node;
	/** One of K_PROCESS_UNUSED / ACTIVE / EXITING / ZOMBIE */
	uint8_t state;
	/** Internal K_PROCESS_FLAG_* state bits */
	uint8_t flags;
	/** Latched wait status (K_WSTATUS_*), valid in K_PROCESS_ZOMBIE */
	int wstatus;
	/** Threads pending in k_waitpid() on this process's children */
	_wait_q_t waitq;
	/** Node in the kernel's registry of live process records */
	sys_dnode_t active_node;
	/** @endcond */
};

/**
 * @brief Statically define and initialize a process
 *
 * Defines the process's thread group leader with K_THREAD_DEFINE() semantics
 * (autostarted after @a delay; K_TICKS_FOREVER defines it without starting)
 * plus the process record binding it. @a name is the leader's k_tid_t and
 * therefore directly the process's @ref k_pid_t handle.
 *
 * @param name name of the process; also the leader thread's k_tid_t symbol
 * @param stack_size stack size of the leader thread, in bytes
 * @param entry entry point of the leader thread
 * @param p1 first entry parameter
 * @param p2 second entry parameter
 * @param p3 third entry parameter
 * @param prio priority of the leader thread
 * @param options options of the leader thread (e.g. K_USER)
 * @param delay scheduling delay of the leader thread in milliseconds, zero for
 *              no delay, or K_TICKS_FOREVER to define it without starting it
 */
#define K_PROCESS_DEFINE(name, stack_size, entry, p1, p2, p3, prio, options, delay)                \
	K_THREAD_DEFINE(name, stack_size, entry, p1, p2, p3, prio, options, delay);                \
	static STRUCT_SECTION_ITERABLE(k_process_group, _k_pgrp_cell_##name);                               \
	static STRUCT_SECTION_ITERABLE(k_session, _k_session_cell_##name);                         \
	static STRUCT_SECTION_ITERABLE(k_process, _k_process_ctx_##name) = {                       \
		.leader = &_k_thread_obj_##name,                                                   \
	}

/**
 * @brief Grant the leader thread of a statically defined process access to kernel objects
 *
 * @param name name of the process passed to K_PROCESS_DEFINE()
 */
#define K_PROCESS_ACCESS_GRANT(name, ...) K_THREAD_ACCESS_GRANT(name, __VA_ARGS__)

/**
 * @brief Statically define a session
 *
 * Statically defined sessions persist even when empty. Process groups are
 * placed in the session with K_PROCESS_GROUP_DEFINE(); the membership lists
 * are assembled at link time.
 *
 * @param name symbol name of the session (a struct k_session)
 */
#define K_SESSION_DEFINE(name)                                                                     \
	STRUCT_SECTION_ITERABLE(k_session, name) = {                                               \
		.flags = K_SESSION_FLAG_STATIC,                                                    \
	}

/**
 * @brief Statically define a process group within a session
 *
 * Statically defined groups persist even when empty. Processes are placed
 * in the group with K_PROCESS_DEFINE_EXT(); the membership lists are
 * assembled at link time.
 *
 * @param name symbol name of the process group (a struct k_process_group)
 * @param session session the group belongs to (a struct k_session *)
 */
#define K_PROCESS_GROUP_DEFINE(name, session_)                                                     \
	STRUCT_SECTION_ITERABLE(k_process_group, name) = {                                         \
		.session = (session_),                                                             \
	}

/**
 * @brief Statically define and initialize a process in a specific process group
 *
 * As K_PROCESS_DEFINE(), with the process placed in @a pgrp (a group defined
 * with K_PROCESS_GROUP_DEFINE()) instead of the boot-default group; the
 * group's membership list is assembled at link time.
 *
 * @param name name of the process; also the leader thread's k_tid_t symbol
 * @param pgrp process group (a struct k_process_group *)
 * @param stack_size stack size of the leader thread, in bytes
 * @param entry entry point of the leader thread
 * @param p1 first entry parameter
 * @param p2 second entry parameter
 * @param p3 third entry parameter
 * @param prio priority of the leader thread
 * @param options options of the leader thread (e.g. K_USER)
 * @param delay scheduling delay of the leader thread in milliseconds, zero for
 *              no delay, or K_TICKS_FOREVER to define it without starting it
 */
#define K_PROCESS_DEFINE_EXT(name, pgrp_, stack_size, entry, p1, p2, p3, prio, options, delay)     \
	K_THREAD_DEFINE(name, stack_size, entry, p1, p2, p3, prio, options, delay);                \
	static STRUCT_SECTION_ITERABLE(k_process, _k_process_ctx_##name) = {                       \
		.leader = &_k_thread_obj_##name,                                                   \
		.pgrp = (pgrp_),                                                                   \
	}

/**
 * @brief Build-time thread-to-process membership preset
 *
 * Emitted by K_THREAD_DEFINE_EXT(); consumed once at thread start to place the
 * thread in a statically defined process instead of inheriting its creator's.
 */
struct k_process_thread_preset {
	/** @cond INTERNAL_HIDDEN */
	/** The statically defined thread */
	struct k_thread *thread;
	/** Handle (leader) of the statically defined process to join */
	k_tid_t pid;
	/** @endcond */
};

/**
 * @brief Statically define a thread as a member of a statically defined process
 *
 * As K_THREAD_DEFINE(), with the thread placed in the process defined by
 * K_PROCESS_DEFINE()/K_PROCESS_DEFINE_EXT() as @a process instead of the
 * process of the thread that starts it.
 *
 * @param name name of the thread
 * @param process name of the process passed to K_PROCESS_DEFINE()
 * @param stack_size stack size of the thread, in bytes
 * @param entry thread entry point
 * @param p1 first entry parameter
 * @param p2 second entry parameter
 * @param p3 third entry parameter
 * @param prio thread priority
 * @param options thread options (e.g. K_USER)
 * @param delay scheduling delay in milliseconds, or K_TICKS_FOREVER
 */
#define K_THREAD_DEFINE_EXT(name, process, stack_size, entry, p1, p2, p3, prio, options, delay)    \
	K_THREAD_DEFINE(name, stack_size, entry, p1, p2, p3, prio, options, delay);                \
	static STRUCT_SECTION_ITERABLE(k_process_thread_preset, _k_thread_preset_##name) = {       \
		.thread = &_k_thread_obj_##name,                                                   \
		.pid = (process),                                                                  \
	}

#if defined(CONFIG_PROCESS_INIT) || defined(__DOXYGEN__)
/**
 * @brief Entry point of the init process
 *
 * With @kconfig{CONFIG_PROCESS_INIT}, the init process gains an initial member
 * thread at boot running this function. It is a weak symbol whose default
 * implementation sleeps forever; define it in the application to act as the
 * system's init process. It must not return.
 *
 * @param p1 unused
 * @param p2 unused
 * @param p3 unused
 */
void z_process_init_entry(void *p1, void *p2, void *p3);
#endif /* CONFIG_PROCESS_INIT || __DOXYGEN__ */


/**
 * @}
 */

#endif /* defined(CONFIG_PROCESS) || defined(__DOXYGEN__) */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_KERNEL_PROCESS_H_ */
