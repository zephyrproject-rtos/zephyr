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

/**
 * @brief Applet fault handling behavior
 */
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
	 * Name of the entry-point symbol used by @ref applet_spawn and
	 * @ref applet_add_thread_sym. When @c entry_sym is
	 * NULL, @ref APPLET_ENTRY_SYM becomes the entry point
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
 * @brief Define an array of stacks suitable for applet threads.
 */
#define APPLET_THREAD_STACK_ARRAY_DEFINE(_name, _nmemb, _size)                                     \
	K_THREAD_STACK_ARRAY_DEFINE(_name, _nmemb, _size)

/**
 * @brief Initialise a native applet descriptor.
 *
 * When @kconfig{CONFIG_USERSPACE} is enabled, a fresh (empty)
 * @c k_mem_domain is created so subsequently-added threads can access the
 * partitions added via @ref applet_add_partition.
 *
 * A descriptor must be zero-initialised before its first use (static storage
 * is). It may be re-initialised after @ref applet_unload; the memory domain
 * is then reused, because most architectures cannot deinitialise one.
 *
 * @param applet_inst  Descriptor to initialise; cleared by the call, except for
 *                     a memory domain it already owns
 * @param name         Human-readable name, truncated to
 *                     @kconfig{CONFIG_APPLET_NAME_MAX_LEN} characters
 * @param opts         Options; NULL selects the defaults with @c user_mode
 *                     forced off, since native entry functions are ordinary
 *                     in-image code
 *
 * @retval 0       Success; @p applet_inst is in @ref APPLET_STATE_LOADED
 * @retval -EINVAL @p applet_inst or @p name is NULL
 * @retval <0      Error from @c k_mem_domain_init
 */
int applet_init(struct applet *applet_inst, const char *name, const struct applet_opts *opts);

#if defined(CONFIG_APPLET_LLEXT) || defined(__DOXYGEN__)
/**
 * @brief Load an LLEXT-backed applet from an ELF image in memory.
 *
 * Sets up an LLEXT and (with @kconfig{CONFIG_USERSPACE}) a memory domain
 * containing the extension's regions. Does not create any threads.
 *
 * The ELF image is parsed in the caller's context, so the calling thread
 * needs roughly 1.5 kB of stack on top of its own usage. The default
 * @kconfig{CONFIG_MAIN_STACK_SIZE} is not enough on most 32-bit targets.
 *
 * @param applet_inst  Descriptor to initialise; cleared by the call, except for
 *                     a memory domain it already owns
 * @param name         Human-readable name, truncated to
 *                     @kconfig{CONFIG_APPLET_NAME_MAX_LEN} characters
 * @param elf_data     ELF image to load from
 * @param elf_size     Size of @p elf_data in bytes
 * @param opts         Options; NULL selects defaults
 *
 * @retval 0       Success; @p applet_inst is in @ref APPLET_STATE_LOADED
 * @retval -EINVAL @p applet_inst, @p name or @p elf_data is NULL, or
 *                 @p elf_size is zero
 * @retval <0      Error from @c k_mem_domain_init, @c llext_load or
 *                 @c llext_add_domain
 */
int applet_load_llext(struct applet *applet_inst, const char *name, const void *elf_data,
		      size_t elf_size, const struct applet_opts *opts);
#endif /* CONFIG_APPLET_LLEXT || __DOXYGEN__ */

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
 * @retval -EINVAL  A pointer argument is NULL, @p stack_size is zero, or the
 *                  applet is not in @ref APPLET_STATE_LOADED
 * @retval -ENOMEM  Slot heap exhausted; raise
 *                  @kconfig{CONFIG_APPLET_HEAP_SIZE}. Under
 *                  @kconfig{CONFIG_USERSPACE} the thread object is drawn from
 *                  the kernel heap instead, so
 *                  @kconfig{CONFIG_HEAP_MEM_POOL_SIZE} may be the binding limit
 */
int applet_add_thread(struct applet *applet_inst, k_thread_stack_t *stack, size_t stack_size,
		      k_thread_entry_t entry, void *arg, const char *thread_name);

#if defined(CONFIG_APPLET_LLEXT) || defined(__DOXYGEN__)
/**
 * @brief Attach a thread whose entry is an exported LLEXT symbol.
 *
 * @param applet_inst Applet in @ref APPLET_STATE_LOADED, loaded via
 *                    @ref applet_load_llext
 * @param stack       Stack memory (e.g. via @ref APPLET_THREAD_STACK_DEFINE)
 * @param stack_size  Size of the stack in bytes
 * @param entry_sym   Symbol name; NULL selects @c opts.entry_sym, which itself
 *                    defaults to @ref APPLET_ENTRY_SYM
 * @param arg         Opaque argument forwarded as @c p1
 * @param thread_name Optional thread name (NULL = applet name)
 *
 * @retval 0        Success
 * @retval -EINVAL  @p applet_inst is NULL, the applet is not LLEXT-backed, or
 *                  it is not in @ref APPLET_STATE_LOADED
 * @retval -ENOENT  @p entry_sym is not exported by the extension
 * @retval -ENOMEM  Slot heap exhausted; see @ref applet_add_thread
 */
int applet_add_thread_sym(struct applet *applet_inst, k_thread_stack_t *stack, size_t stack_size,
			  const char *entry_sym, void *arg, const char *thread_name);
#endif /* CONFIG_APPLET_LLEXT || __DOXYGEN__ */

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
 * @param applet_inst  Applet in any state other than @ref APPLET_STATE_UNLOADED
 * @param part         Caller-allocated partition (must remain valid for the
 *                     lifetime of the applet)
 *
 * @retval 0        Success (or @kconfig{CONFIG_USERSPACE} disabled)
 * @retval -EINVAL  @p applet_inst or @p part is NULL, the applet is
 *                  @ref APPLET_STATE_UNLOADED, or it has no memory domain
 * @retval <0       Error from @c k_mem_domain_add_partition
 */
int applet_add_partition(struct applet *applet_inst, struct k_mem_partition *part);

/**
 * @brief Start every attached, not-yet-started thread of the applet.
 *
 * For LLEXT-backed applets the extension's .init_array runs in supervisor
 * mode on the first call.
 *
 * @param applet_inst  Applet in @ref APPLET_STATE_LOADED with at least one
 *                     attached thread
 *
 * @retval 0        Applet is now @ref APPLET_STATE_RUNNING
 * @retval -EINVAL  @p applet_inst is NULL, the applet is not in
 *                  @ref APPLET_STATE_LOADED, or no threads are attached
 * @retval <0       Error from @c llext_bringup; the applet moves to
 *                  @ref APPLET_STATE_DEAD
 */
int applet_start(struct applet *applet_inst);

#if defined(CONFIG_APPLET_LLEXT) || defined(__DOXYGEN__)
/**
 * @brief One-shot helper: load an LLEXT, attach one thread, start.
 *
 * Equivalent to @ref applet_load_llext followed by @ref applet_add_thread_sym
 * and @ref applet_start. The applet is unloaded again if the thread cannot be
 * attached.
 *
 * @param applet_inst  Descriptor to initialise
 * @param name         Human-readable name, truncated to
 *                     @kconfig{CONFIG_APPLET_NAME_MAX_LEN} characters
 * @param elf_data     ELF image to load from
 * @param elf_size     Size of @p elf_data in bytes
 * @param stack        Stack memory for the implicit thread
 * @param stack_size   Size of the stack in bytes
 * @param opts         Options; NULL selects defaults. @c opts.entry_sym names
 *                     the entry symbol and @c opts.arg is passed to it
 *
 * @retval 0        Success; applet is @ref APPLET_STATE_RUNNING
 * @retval -EINVAL  Bad argument or wrong state
 * @retval -ENOENT  The entry symbol is not exported by the extension
 * @retval -ENOMEM  Slot heap exhausted; see @ref applet_add_thread
 * @retval <0       Error from @c llext_load, @c llext_add_domain or
 *                  @c llext_bringup
 */
int applet_spawn(struct applet *applet_inst, const char *name, const void *elf_data,
		 size_t elf_size, k_thread_stack_t *stack, size_t stack_size,
		 const struct applet_opts *opts);

/**
 * @brief Legacy wrapper: load LLEXT and attach a single thread, but do not
 *        start the applet.
 *
 * Identical to @ref applet_spawn except that @ref applet_start is left to the
 * caller.
 *
 * @param applet_inst  Descriptor to initialise
 * @param name         Human-readable name, truncated to
 *                     @kconfig{CONFIG_APPLET_NAME_MAX_LEN} characters
 * @param elf_data     ELF image to load from
 * @param elf_size     Size of @p elf_data in bytes
 * @param stack        Stack memory for the implicit thread
 * @param stack_size   Size of the stack in bytes
 * @param opts         Options; NULL selects defaults
 *
 * @retval 0        Success; applet is @ref APPLET_STATE_LOADED
 * @retval -EINVAL  Bad argument or wrong state
 * @retval -ENOENT  The entry symbol is not exported by the extension
 * @retval -ENOMEM  Slot heap exhausted; see @ref applet_add_thread
 * @retval <0       Error from @c llext_load or @c llext_add_domain
 */
int applet_load(struct applet *applet_inst, const char *name, const void *elf_data, size_t elf_size,
		k_thread_stack_t *stack, size_t stack_size, const struct applet_opts *opts);
#endif /* CONFIG_APPLET_LLEXT || __DOXYGEN__ */

/**
 * @brief Wait for all applet threads to finish.
 *
 * The internal lock is released while waiting, so other threads can keep
 * operating on this and other applets. A concurrent @ref applet_unload on the
 * same applet aborts the threads and then waits for this call to return.
 *
 * @param applet_inst  Applet in @ref APPLET_STATE_RUNNING or @ref APPLET_STATE_DEAD
 * @param timeout  Maximum time to wait per thread. If it expires on any
 *                 thread, @c -EAGAIN is returned immediately.
 *
 * @retval 0        All threads joined; applet is now @ref APPLET_STATE_DEAD
 * @retval -EINVAL  @p applet_inst is NULL, or the applet is neither
 *                  @ref APPLET_STATE_RUNNING nor @ref APPLET_STATE_DEAD
 * @retval -EAGAIN  @p timeout expired before a thread terminated
 * @retval -EBUSY   @p timeout was @c K_NO_WAIT and a thread is still running
 * @retval -EDEADLK An applet thread tried to join its own applet
 */
int applet_join(struct applet *applet_inst, k_timeout_t timeout);

/**
 * @brief Abort every running thread of the applet.
 *
 * If the calling thread belongs to @p applet_inst it is aborted last, so this
 * function does not return in that case.
 *
 * @param applet_inst  Applet in @ref APPLET_STATE_RUNNING
 *
 * @retval 0        All threads aborted; applet is now @ref APPLET_STATE_DEAD
 * @retval -EINVAL  @p applet_inst is NULL, or no thread of the applet is
 *                  running
 */
int applet_kill(struct applet *applet_inst);

/**
 * @brief Release all resources and reset the descriptor.
 *
 * Kills any thread still running, then waits for concurrent joins on this
 * applet to return before freeing the thread objects they refer to. The
 * descriptor returns to @ref APPLET_STATE_UNLOADED and may be re-initialised.
 *
 * @param applet_inst  Applet in any state
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
 * @param applet_inst Applet descriptor, or NULL
 *
 * @return Current state, or @ref APPLET_STATE_UNLOADED if @p applet_inst is NULL
 */
enum applet_state applet_get_state(struct applet *applet_inst);

/**
 * @brief Number of threads currently attached to the applet.
 *
 * @param applet_inst Applet descriptor, or NULL
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
 *
 * @param applet_inst Applet descriptor
 * @param idx Index of the thread to retrieve
 *
 * @return Pointer to the @c k_thread, or NULL if @p idx is out of range
 */
struct k_thread *applet_thread_get(struct applet *applet_inst, unsigned int idx);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_APPLET_APPLET_H_ */
