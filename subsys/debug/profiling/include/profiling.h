/*
 * Copyright (c) 2019 Kevin Townsend
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DBG_PROFILING_H_
#define ZEPHYR_INCLUDE_DBG_PROFILING_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup dbg_profiling Debug profiling subsystem
 * @ingroup dbg_profiling
 * @{
 */

/**
 * @struct dbg_prof_task
 * Profiling task for a specific code segment.
 */
struct dbg_prof_task {
	char *name;
	/**< User-defined label for this profiling task handler. */

	u32_t count;
	/**< Total number of counter ticks tracked for this task. */
};

/**
 * @brief Initialisation of the debug profiling subsys and backend.
 *
 * Can be called at application startup.
 *
 * @return 0 on success, non-zero on failure.
 */
int dbg_prof_init(void);

/**
 * @brief Initiailisation of the specified task handler.
 *
 * @param task  Pointer to the dbg_prof_task struct to initialise.
 *
 * @return 0 on success, non-zero on failure.
 */
int dbg_prof_task_init(struct dbg_prof_task *task);

/**
 * @brief (Re)starts incrementing the counter for the specified instrument task.
 *
 * @param task  Pointer to the dbg_prof_task struct to (re)start.
 *
 * @return 0 on success, non-zero on failure.
 */
int dbg_prof_task_start(struct dbg_prof_task *task);

/**
 * @brief Stops incrementing the counter for the specified instrument task,
 *        and sends a log event with the current count value.
 *
 * @param task  Pointer to the dbg_prof_task struct to stop.
 *
 * @return 0 on success, non-zero on failure.
 */
int dbg_prof_task_stop(struct dbg_prof_task *task);

/**
 * @brief Sends a log event with the current count value for the specified
 *        instrument task.
 *
 * @param task  Pointer to the dbg_prof_task struct to initialise.
 *
 * @return 0 on success, non-zero on failure.
 */
int dbg_prof_task_update(struct dbg_prof_task *task);

/**
 * @brief Resets the count value for the specified instrument task.
 *
 * @param task  Pointer to the dbg_prof_task struct to reset.
 *
 * @return 0 on success, non-zero on failure.
 */
int dbg_prof_task_reset(struct dbg_prof_task *task);

/**
 * @} instrumentation
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DBG_PROFILING_H_ */
