/*
 * Copyright (c) 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(test, CONFIG_SAMPLE_MODULE_LOG_LEVEL);

/* Log calls with template-dependent arguments must build in C++, including
 * with CONFIG_LOG_USE_TAGGED_ARGUMENTS where the type of each argument is
 * computed with decltype().
 */
template <typename T> void log_template_dependent_args(T val)
{
	LOG_INF("%zu %d", sizeof(T), (int)val);
}

template void log_template_dependent_args<int>(int val);
