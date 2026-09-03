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
 * @param entry receives the entry point to run
 * @param buf destination, at least @kconfig{CONFIG_SYS_PROCESS_ARG_BYTES} bytes
 * @param size size of @a buf, in bytes
 * @retval argc (non-negative) on success
 * @retval -ENOENT if nothing is staged for the calling process, or no entry is set
 * @retval -E2BIG if @a buf is too small for the staged vectors
 */
__syscall int sys_process_args(k_thread_entry_t *entry, void *buf, size_t size);
/**
 * @}
 */

#include <zephyr/syscalls/process.h>

#endif /* defined(CONFIG_SYS_PROCESS) || defined(__DOXYGEN__) */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_PROCESS_H_ */
