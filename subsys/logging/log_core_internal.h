/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_LOGGING_LOG_CORE_INTERNAL_H_
#define ZEPHYR_SUBSYS_LOGGING_LOG_CORE_INTERNAL_H_

#include <stdbool.h>

static inline bool z_log_process_lock_required_for_clean_output(int owner_cpu,
								int curr_cpu)
{
	return owner_cpu != curr_cpu;
}

#endif /* ZEPHYR_SUBSYS_LOGGING_LOG_CORE_INTERNAL_H_ */
