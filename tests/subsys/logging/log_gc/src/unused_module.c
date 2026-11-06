/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright 2026 Mohammed El Amrani
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(log_gc_unused, LOG_LEVEL_INF);

void log_gc_unused_module_run(void)
{
	LOG_INF("Unused logging module");
}
