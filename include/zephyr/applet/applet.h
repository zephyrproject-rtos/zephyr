/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_APPLET_APPLET_H_
#define ZEPHYR_INCLUDE_APPLET_APPLET_H_

/**
 * @file
 * @brief Zephyr Applet Model
 *
 * The applet model groups one or more Zephyr threads into a single logical
 * unit ("applet") with a shared lifecycle and (optionally) a shared
 * memory domain.
 *
 * An applet may be either:
 *
 *  - **LLEXT-backed** — code is loaded at runtime from an ELF binary via
 *    the @ref llext API. The extension's TEXT/DATA/RODATA/BSS regions are
 *    added to the applet's memory domain, so the applet is
 *    hardware-isolated from the rest of the system.
 *
 *  - **Native** — its threads run entry functions that are statically linked
 *    into the main Zephyr image. No ELF loading or linking through LLEXT is
 *    involved. When @kconfig{CONFIG_USERSPACE} is enabled, a dedicated
 *    @c k_mem_domain is still created so the applet's threads can share
 *    the partitions added via @ref applet_add_partition while remaining
 *    isolated from the rest of the system.
 *
 * The number of threads per applet is not statically bounded — threads
 * are tracked in a linked list with per-slot heap allocations. The size
 * of the slot heap is controlled by @kconfig{CONFIG_APPLET_HEAP_SIZE}.
 *
 * @section applet_thread_safety Thread safety
 *
 * All functions in this API may be called concurrently from any number of
 * threads, including on the same descriptor. They are serialised by an
 * internal mutex, so they must be called from thread context only.
 *
 * @ref applet_join releases that mutex while it waits, so a long join on one
 * applet does not block operations on another. @ref applet_unload waits for
 * any in-flight join on the same applet to return before it frees anything.
 *
 * Two exceptions remain the caller's responsibility:
 *
 *  - Concurrent @ref applet_init / @ref applet_load_llext calls on the *same*
 *    descriptor. There is nothing to serialise against until the descriptor
 *    exists.
 *  - The @kconfig{CONFIG_APPLET_FATAL_HANDLER} path runs in fault context and
 *    cannot take the mutex, so it is best-effort.
 *
 * @defgroup applet_apis Applet Model
 * @since 4.4
 * @version 0.3.0
 * @ingroup os_services
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

#ifdef CONFIG_APPLET_LLEXT
#include <zephyr/llext/llext.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Default name of the entry-point symbol inside an LLEXT extension */
#define APPLET_ENTRY_SYM "applet_main"

/**
 * @brief Applet backend kind
 */
enum applet_kind {
	/** Code is linked into the main image; no ELF loading. */
	APPLET_KIND_NATIVE = 0,
	/** Code is loaded from an ELF binary via LLEXT. */
	APPLET_KIND_LLEXT,
};

/**
 * @brief Applet lifecycle states
 */
enum applet_state {
	/** Descriptor is initialised but holds no resources */
	APPLET_STATE_UNLOADED = 0,
	/** Applet is ready to run (threads may be attached and started) */
	APPLET_STATE_LOADED,
	/** At least one applet thread is running */
	APPLET_STATE_RUNNING,
	/** All applet threads have exited */
	APPLET_STATE_DEAD,
};

enum applet_halt_on_fault {
	/** Abort the faulting thread only */
	APPLET_HALT_ON_FAULT_THREAD = 0,
	/** Abort all threads in the applet when any thread faults */
	APPLET_HALT_ON_FAULT_APPLET,
	/** Halt the whole system when any thread faults */
	APPLET_HALT_ON_FAULT_SYSTEM,
};

/**
 * @brief Configuration options for an applet
 *
 * Initialise with @ref APPLET_OPTS_DEFAULT and then override individual
 * fields as needed. These options apply to all threads of the applet.
 */
struct applet_opts {
	/** Default scheduling priority for threads of this applet. */
	int thread_priority;

	/** CPU to pin all threads of this applet to */
	int cpu;

	/**
	 * Run the applet threads in unprivileged (user) mode.
	 * Requires both @kconfig{CONFIG_USERSPACE} and an applet memory
	 * domain. For native applets the caller is responsible for
	 * ensuring the thread entry function only calls APIs permitted to
	 * user threads.
	 *
	 * Default: @c true when CONFIG_USERSPACE is enabled.
	 */
	bool user_mode;

	/**
	 * If @kconfig{CONFIG_APPLET_FATAL_HANDLER} is enabled,
	 * specifies behavior when a thread faults. See @ref applet_halt_on_fault.
	 */
	enum applet_halt_on_fault halt_on_fault;

	/**
	 * Name of the entry-point symbol used by @ref applet_spawn (and
	 * @ref applet_add_thread_sym when its @c entry_sym argument is
	 * NULL selects @ref APPLET_ENTRY_SYM.
	 */
	const char *entry_sym;

	/** Argument passed to the implicit thread created by spawn. */
	void *arg;
};

/** @brief Default initialiser for @ref applet_opts */
#define APPLET_OPTS_DEFAULT                                                                        \
	{                                                                                          \
		.thread_priority = CONFIG_APPLET_THREAD_PRIORITY_DEFAULT,                          \
		.cpu = 0,                                                                          \
		.user_mode = IS_ENABLED(CONFIG_USERSPACE),                                         \
		.halt_on_fault = APPLET_HALT_ON_FAULT_APPLET,                                      \
		.entry_sym = APPLET_ENTRY_SYM,                                                     \
		.arg = NULL,                                                                       \
	}

/** @cond INTERNAL_HIDDEN */

/*
 * Per-thread bookkeeping slot. One instance is heap-allocated for every
 * call to applet_add_thread / applet_add_thread_sym and linked into
 * applet::threads.
 */
struct applet_thread {
	sys_snode_t node;
	struct k_thread *thread;
	k_thread_stack_t *stack;
	size_t stack_size;
	k_thread_entry_t entry_fn;
	void *arg;
	int priority;
	bool started;
	bool joined;
};

/** @endcond */

/**
 * @brief Applet descriptor. Treat as opaque; use the API functions to access.
 */
struct applet {
	/** @cond INTERNAL_HIDDEN */
	char name[CONFIG_APPLET_NAME_MAX_LEN + 1];
	enum applet_kind kind;

#ifdef CONFIG_APPLET_LLEXT
	struct llext *ext;
	bool bringup_done;
#endif

#ifdef CONFIG_USERSPACE
	struct k_mem_domain domain;
	bool has_domain;
#endif

	sys_slist_t threads;
	unsigned int thread_count;

	/* Threads currently blocked inside applet_join() on this applet. */
	unsigned int join_busy;

	volatile enum applet_state state;

	struct applet_opts opts;

	sys_snode_t applet_node;
	/** @endcond */
};

/**
 * @brief Define a stack suitable for an applet thread.
 */
#define APPLET_THREAD_STACK_DEFINE(_name, _size) K_THREAD_STACK_DEFINE(_name, _size)

/**
 * @brief Initialise a native applet descriptor.
 *
 * When @kconfig{CONFIG_USERSPACE} is enabled, a fresh (empty)
 * @c k_mem_domain is created so subsequently-added threads can access the
 * partitions added via @ref applet_add_partition.
 *
 * @param applet_inst  Descriptor to initialise (zeroed by the call)
 * @param name  Human-readable name of the applet_inst
 * @param opts  Options; NULL selects defaults
 *
 * @retval 0       Success; applet_inst is in @ref APPLET_STATE_LOADED
 * @retval -EINVAL Bad argument
 * @retval <0      Error from @c k_mem_domain_init
 */
int applet_init(struct applet *applet_inst, const char *name, const struct applet_opts *opts);

#ifdef CONFIG_APPLET_LLEXT
/**
 * @brief Load an LLEXT-backed applet from an ELF image in memory.
 *
 * Sets up an LLEXT and (with @kconfig{CONFIG_USERSPACE}) a memory domain
 * containing the extension's regions. Does not create any threads.
 *
 * The ELF image is parsed in the caller's context, so the calling thread
 * needs roughly 1.5 kB of stack on top of its own usage. The default
 * @kconfig{CONFIG_MAIN_STACK_SIZE} is not enough on most 32-bit targets.
 */
int applet_load_llext(struct applet *applet_inst, const char *name, const void *elf_data,
		      size_t elf_size, const struct applet_opts *opts);
#endif

/**
 * @brief Attach a native-function thread to an applet.
 *
 * The number of threads per applet is bounded only by the size of the
 * applet slot heap (@kconfig{CONFIG_APPLET_HEAP_SIZE}).
 *
 * @param applet_inst Applet in LOADED state
 * @param stack       Stack memory (e.g. via @ref APPLET_THREAD_STACK_DEFINE)
 * @param stack_size  Size of the stack in bytes
 * @param entry       Function to run in the new thread (Zephyr thread entry
 *                    signature: @c void(void*,void*,void*))
 * @param arg         Opaque argument forwarded as @c p1
 * @param thread_name Optional thread name (NULL = applet_inst name)
 *
 * @retval 0        Success
 * @retval -EINVAL  Bad argument or wrong state
 * @retval -ENOMEM  Applet slot heap exhausted; raise
 *                  @kconfig{CONFIG_APPLET_HEAP_SIZE}
 */
int applet_add_thread(struct applet *applet_inst, k_thread_stack_t *stack, size_t stack_size,
		      k_thread_entry_t entry, void *arg, const char *thread_name);

#ifdef CONFIG_APPLET_LLEXT
/**
 * @brief Attach a thread whose entry is an exported LLEXT symbol.
 *
 * @param entry_sym  Symbol name (NULL = @c opts.entry_sym)
 */
int applet_add_thread_sym(struct applet *applet_inst, k_thread_stack_t *stack, size_t stack_size,
			  const char *entry_sym, void *arg, const char *thread_name);
#endif

/**
 * @brief Add a memory partition to the applet's domain.
 *
 * After this call, every thread of @p applet_inst (current and future) can access
 * @p part with the access mode the caller set on it. Useful to expose a
 * shared buffer between threads of a native or LLEXT applet, or to grant
 * an applet access to a kernel-resident region that the caller controls.
 *
 * Requires @kconfig{CONFIG_USERSPACE}; on builds without it this function
 * is a successful no-op.
 *
 * @param applet_inst  Applet in LOADED state
 * @param part  Caller-allocated partition (must remain valid for the
 *              lifetime of the applet)
 *
 * @retval 0        Success (or USERSPACE disabled)
 * @retval -EINVAL  Bad argument or applet has no domain
 * @retval <0       Error from @c k_mem_domain_add_partition
 */
int applet_add_partition(struct applet *applet_inst, struct k_mem_partition *part);

/**
 * @brief Start every attached, not-yet-started thread of the applet.
 *
 * For LLEXT-backed applets the extension's .init_array runs in supervisor
 * mode on the first call.
 *
 * @retval 0        Applet is now @ref APPLET_STATE_RUNNING
 * @retval -EINVAL  No threads attached or wrong state
 * @retval <0       LLEXT bringup error
 */
int applet_start(struct applet *applet_inst);

#ifdef CONFIG_APPLET_LLEXT
/**
 * @brief One-shot helper: load an LLEXT, attach one thread, start.
 */
int applet_spawn(struct applet *applet_inst, const char *name, const void *elf_data,
		 size_t elf_size, k_thread_stack_t *stack, size_t stack_size,
		 const struct applet_opts *opts);

/**
 * @brief Legacy wrapper: load LLEXT and attach a single thread, but do not
 *        start the applet.
 */
int applet_load(struct applet *applet_inst, const char *name, const void *elf_data, size_t elf_size,
		k_thread_stack_t *stack, size_t stack_size, const struct applet_opts *opts);
#endif /* CONFIG_APPLET_LLEXT */

/**
 * @brief Wait for all applet threads to finish.
 *
 * The internal lock is released while waiting, so other threads can keep
 * operating on this and other applets. A concurrent @ref applet_unload on the
 * same applet aborts the threads and then waits for this call to return.
 *
 * @param timeout  Maximum time to wait per thread. If it expires on any
 *                 thread, @c -EAGAIN is returned immediately.
 */
int applet_join(struct applet *applet_inst, k_timeout_t timeout);

/**
 * @brief Abort every running thread of the applet.
 */
int applet_kill(struct applet *applet_inst);

/**
 * @brief Release all resources and reset the descriptor.
 *
 * Kills any thread still running, then waits for concurrent joins on this
 * applet to return before freeing the thread objects they refer to.
 */
void applet_unload(struct applet *applet_inst);

/**
 * @brief Get the current lifecycle state of an applet.
 *
 * Applet threads never report their own exit, so an applet in
 * @ref APPLET_STATE_RUNNING is resolved by sampling its threads: once all of
 * them have terminated the applet moves to @ref APPLET_STATE_DEAD. This holds
 * for threads that returned normally, were aborted, or died on a fatal error,
 * and it requires nothing from the applet's own code -- which is what makes it
 * work for unprivileged threads.
 *
 * The transition only happens when the state is observed, so call this rather
 * than reading the descriptor.
 *
 * Must not be called concurrently with the other applet APIs on the same
 * descriptor.
 *
 * @param applet_inst Applet descriptor, or NULL
 *
 * @return Current state, or @ref APPLET_STATE_UNLOADED if @p applet_inst is NULL
 */
enum applet_state applet_get_state(struct applet *applet_inst);

/**
 * @brief Number of threads currently attached to the applet.
 *
 * @return Snapshot of the count; 0 if @p applet_inst is NULL.
 */
unsigned int applet_thread_count(struct applet *applet_inst);

/**
 * @brief Get the @c k_thread for the @p idx attached thread.
 *
 * Threads are tracked in attachment order. Returns NULL if @p idx is out
 * of range (i.e. @p idx >= applet_thread_count(applet)). Because the
 * underlying slot is heap-allocated, the returned pointer is only valid
 * until the applet is unloaded.
 */
struct k_thread *applet_thread_get(struct applet *applet_inst, unsigned int idx);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_APPLET_APPLET_H_ */
