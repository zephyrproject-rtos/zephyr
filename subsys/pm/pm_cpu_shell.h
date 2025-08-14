/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_PM_PM_CPU_SHELL_H_
#define ZEPHYR_SUBSYS_PM_PM_CPU_SHELL_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * @brief thread_node.
 *
 * @note All fields in this structure are meant for private usage.
 */
struct thread_event {
	sys_snode_t node;
	const struct k_thread *thread;
};

/**
 * @brief Resume all threads previously suspended by the shell low power entry.
 *
 * This function resumes all application threads that were suspended
 * during entry to shell-triggered low power mode. It should be called
 * after the system wakes up from shell-invoked low power state to
 * restore normal thread execution.
 */
void pm_resume_threads(void);
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_PM_PM_CPU_SHELL_H_ */
