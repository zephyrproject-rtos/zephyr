/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_LOG_BACKEND_STD_H_
#define ZEPHYR_LOG_BACKEND_STD_H_

#include <zephyr/logging/log_msg.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Logger backend interface for forwarding to standard backend
 * @defgroup log_backend_std Logger backend standard interface
 * @ingroup logger
 * @{
 */

static inline uint32_t log_backend_std_get_flags(void)
{
	uint32_t flags = (LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_TIMESTAMP);

	if (IS_ENABLED(CONFIG_LOG_BACKEND_SHOW_COLOR)) {
		flags |= LOG_OUTPUT_FLAG_COLORS;
	}

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	return flags;
}

/** @brief Put a standard logger backend into panic mode.
 *
 * @param output	Log output instance.
 */
static inline void
log_backend_std_panic(const struct log_output *const output)
{
	log_output_flush(output);
}

/** @brief Report dropped messages to a standard logger backend.
 *
 * @param output	Log output instance.
 * @param cnt		Number of dropped messages.
 */
static inline void
log_backend_std_dropped(const struct log_output *const output, uint32_t cnt)
{
	log_output_dropped_process(output, cnt);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LOG_BACKEND_STD_H_ */
