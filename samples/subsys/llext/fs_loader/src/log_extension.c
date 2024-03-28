/*
 * Copyright (c) 2024 Schneider Electric
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(extension, LOG_LEVEL);

#ifdef CONFIG_LLEXT
#include <zephyr/llext/symbol.h>
EXPORT_SYMBOL(log_dynamic_extension);
#endif
