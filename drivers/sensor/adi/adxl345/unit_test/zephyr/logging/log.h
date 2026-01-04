/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCK_ZEPHYR_INCLUDE_LOGGING_LOG_H_
#define MOCK_ZEPHYR_INCLUDE_LOGGING_LOG_H_

/* used by twister unit tests. */

#include <stdio.h>

#define LOG_MODULE_REGISTER(...)
#define LOG_MODULE_DECLARE(...)

#define LOG_ERR(...) (printf(__VA_ARGS__))
#define LOG_WRN(...) (printf(__VA_ARGS__))
#define LOG_INF(...) (printf(__VA_ARGS__))
#define LOG_DBG(...) (printf(__VA_ARGS__))

#endif /* MOCK_ZEPHYR_INCLUDE_LOGGING_LOG_H_ */
