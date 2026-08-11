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
 * @}
 */

#include <zephyr/syscalls/process.h>

#endif /* defined(CONFIG_SYS_PROCESS) || defined(__DOXYGEN__) */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_PROCESS_H_ */
