/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright 2026 Mohammed El Amrani
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "test_modules.h"

LOG_MODULE_REGISTER(log_gc_used, LOG_LEVEL_INF);

void log_gc_used_module_run(void)
{
	LOG_INF("Used logging module");
}
