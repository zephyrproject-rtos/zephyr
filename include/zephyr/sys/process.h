/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_PROCESS_H_
#define ZEPHYR_INCLUDE_SYS_PROCESS_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/thread.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_SYS_PROCESS) || defined(__DOXYGEN__)

/**
 * @brief System-layer process, process-group, and session numbering
 * @defgroup sys_process_apis System process numbering APIs
 *
 * The kernel identifies processes, process groups, and sessions purely by
 * handle; integer identities imply a namespace, and namespaces are an
 * allocation-domain concern, so the number tables live here. Numbers are
 * assigned lowest-free at creation (the reserved idle, init, and kthread
 * processes receive 0, 1, and 2 by boot order) and retired with their object;
 * a group or session is numbered after its creating process.
 *
 * @ingroup os_services
 * @{
 */

/**
 * @brief Obtain the numeric ID of a process
 *
 * @param pid process handle (or the handle of any live member thread)
 * @return the numeric process ID, or -ESRCH if @a pid names no process
 */
__syscall int sys_process_id(k_pid_t pid);

/**
 * @brief Look up a process handle by numeric ID
 *
 * @param id numeric process ID
 * @return the process handle, or NULL if no such process exists
 */
__syscall k_pid_t sys_process_find(int id);

/**
 * @brief Obtain the numeric ID of a process group
 *
 * @param pgrp process group handle
 * @return the numeric group ID, or -ESRCH if @a pgrp names no group
 */
__syscall int sys_pgrp_id(k_pgrp_t pgrp);

/**
 * @brief Look up a process group by numeric ID
 *
 * @param id numeric process group ID
 * @return the group handle, or NULL if no such group exists
 */
__syscall k_pgrp_t sys_pgrp_find(int id);

/**
 * @brief Obtain the numeric ID of a session
 *
 * @param session session handle
 * @return the numeric session ID, or -ESRCH if @a session names no session
 */
__syscall int sys_session_id(k_session_t session);

/**
 * @brief Look up a session by numeric ID
 *
 * @param id numeric session ID
 * @return the session handle, or NULL if no such session exists
 */
__syscall k_session_t sys_session_find(int id);

/**
 * @brief Create a new session with the calling process as leader
 *
 * Pool-backed k_session_init(): the session and group cells come from the
 * system pool and return to it when the last member leaves.
 *
 * @return the new session, or NULL if the calling process is already a
 *         process-group leader or no cells are available
 */
__syscall k_session_t sys_setsid(void);

/**
 * @brief Set the process group of a process
 *
 * Like k_setpgid(), but @a pgrp may be NULL: the process then becomes the
 * sole member of a new process group in its session, backed by a cell from
 * the system pool (pool-backed k_process_group_init()).
 *
 * @retval 0 on success
 * @retval -ESRCH if @a pid names no process
 * @retval -EPERM if the target is a session leader
 * @retval -EINVAL if @a pgrp is not a live group in the target's session
 * @retval -EAGAIN if @a pgrp is NULL and no cell is available
 */
__syscall int sys_setpgid(k_pid_t pid, k_pgrp_t pgrp);

/**
 * @brief Create a child process that resumes at the call site (fork)
 *
 * Equivalent to sys_clone() with SYS_CLONE_VM_COPY and no entry point: the
 * child is a copy of the calling process resuming from this call, which
 * returns twice - the child handle to the parent, NULL to the child.
 *
 * @param child parent-side storage for the child's handle
 *
 * @retval 0 on success, in both the parent and the child
 * @retval -ENOSYS until the address-space duplication substrate is available
 * @retval -EAGAIN if no process record is available
 */
__syscall int sys_fork(k_pid_t *child);

/** How a process created with staged arguments is entered */
struct sys_process_start {
	/** Entry given to sys_clone(), entered as entry(argv, envp, argc), or NULL */
	k_thread_entry_t entry;
	/** main() of an image loaded by sys_exec_load(), or NULL */
	int (*main)(int argc, char **argv, char **envp);
	/** Descriptor actions to apply before entering, relocated into the buffer */
	const struct sys_clone_fd_action *fd_actions;
	/** Number of entries in @a fd_actions */
	size_t fd_actions_len;
};

/**
 * @brief Claim the vectors staged for the calling process by sys_clone()
 *
 * Copies the argument and environment vectors given to sys_clone() into
 * @a buf, laid out as argv (argc + 1 pointers), envp (NULL-terminated), then
 * the strings, all relocated to @a buf, returns the entry point the process
 * was created with in @a entry, and releases the staged copy. The leader's
 * entry trampoline calls this once; the vectors then live as long as @a buf
 * does.
 *
 * @param start receives how to enter the image: @ref sys_process_start
 * @param buf destination, at least @kconfig{CONFIG_SYS_PROCESS_ARG_BYTES} bytes
 * @param size size of @a buf, in bytes
 * @retval argc (non-negative) on success
 * @retval -ENOENT if nothing is staged for the calling process, or no entry is set
 * @retval -E2BIG if @a buf is too small for the staged vectors
 */
__syscall int sys_process_args(struct sys_process_start *start, void *buf, size_t size);

/**
 * @brief Load a file system image as a paused child's process image
 *
 * Loads the extension at @a path, maps its regions into the child's memory
 * domain, and sets its exported main() as the entry the child starts with;
 * the child must be a not-yet-started child of the caller created by
 * sys_clone() with staged arguments and no entry point. The image's main()
 * receives the staged vectors as main(argc, argv, envp) and its return value
 * is the process's exit status. The extension is unloaded when the process
 * is reaped. Requires @kconfig{CONFIG_SYS_PROCESS_EXEC}.
 *
 * With @a child the calling thread, itself the leader of its process, the
 * image is loaded as the pending image of the caller's own process, to be
 * entered by sys_exec_start(); a pending image not started is unloaded at
 * reap or replaced by the next load.
 *
 * @retval 0 on success
 * @retval -ESRCH if @a child is neither a paused, argument-staged child of the
 *         caller nor the calling leader
 * @retval -ENOENT if @a path cannot be loaded
 * @retval -ENOEXEC if the image exports no main()
 * @retval -ENOMEM if memory, the domain, or the name space is exhausted
 * @retval -ENOTSUP without @kconfig{CONFIG_SYS_PROCESS_EXEC}
 */
__syscall int sys_exec_load(k_pid_t child, const char *path);

/**
 * @brief Replace the calling process's image
 *
 * Stages @a argv and @a envp for the caller's process, replaces its image
 * with the pending one loaded by sys_exec_load() (unloading the previous
 * image), and restarts the calling leader thread on a fresh stack in the
 * mode it runs in, entering @a entry as entry(argv, envp, argc) or, with
 * @a entry NULL, the pending image's main(argc, argv, envp). Does not return
 * on success. Descriptors marked close-on-exec are closed; every other member
 * thread must already be gone, and signal disposition semantics of exec are
 * the caller's business before this call. Requires @kconfig{CONFIG_SYS_PROCESS}.
 *
 * @retval -EINVAL if the caller is not its process's leader
 * @retval -ENOENT if @a entry is NULL and no image is pending
 * @retval -E2BIG if the vectors exceed @kconfig{CONFIG_SYS_PROCESS_ARG_BYTES}
 * @retval -ENOMEM if the vectors cannot be staged
 */
__syscall int sys_exec_start(k_thread_entry_t entry, char *const *argv, char *const *envp);

/**
 * @brief The calling process's private C library area
 *
 * An image process loaded with @kconfig{CONFIG_SYS_PROCESS_STATE_AREA} set
 * owns a writable area of that size inside its image's block, for the C
 * library's per-process state: the malloc arena and the standard streams.
 * The area is zeroed at load; the C library lays it out and initializes it
 * on first use.
 *
 * @param area receives the area's start
 * @param size receives its size, in bytes
 * @retval 0 on success
 * @retval -ENOENT if the calling process shares the boot image's C library state
 */
__syscall int sys_process_state_area(void **area, size_t *size);

/**
 * @}
 */

#include <zephyr/syscalls/process.h>

#endif /* defined(CONFIG_SYS_PROCESS) || defined(__DOXYGEN__) */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_PROCESS_H_ */
