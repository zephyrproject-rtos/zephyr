/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Logging macros of the CPC core, on top of the Zephyr logging subsystem.
 * The core expects this header from its port: SLI_CPC_LOG_*() are the
 * macros of the upstream sli_cpc_log.h. The messages go to the silabs_cpc
 * module and are filtered on CONFIG_SILABS_CPC_LOG_LEVEL. Only the core
 * sources include this header, they get their module declaration from
 * here.
 */

#ifndef SLI_CPC_LOG_H
#define SLI_CPC_LOG_H

#include <zephyr/logging/log.h>
#include <zephyr/toolchain.h>

LOG_MODULE_DECLARE(silabs_cpc, CONFIG_SILABS_CPC_LOG_LEVEL);

#define SLI_CPC_LOG_ZEPHYR_ERR(...)  LOG_ERR(__VA_ARGS__)
#define SLI_CPC_LOG_ZEPHYR_WRN(...)  LOG_WRN(__VA_ARGS__)
#define SLI_CPC_LOG_ZEPHYR_INFO(...) LOG_INF(__VA_ARGS__)
#define SLI_CPC_LOG_ZEPHYR_DBG(...)  LOG_DBG(__VA_ARGS__)

#define __SLI_CPC_LOG(level, level_config, fmt, filename, line, ...)                               \
	SLI_CPC_LOG_ZEPHYR_##level("[" filename ":" STRINGIFY(line) "] " fmt, ##__VA_ARGS__)

#define SLI_CPC_LOG_ERROR(fmt, ...)                                                                \
	__SLI_CPC_LOG(ERR, ERROR, fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__)
#define SLI_CPC_LOG_WARN(fmt, ...)                                                                 \
	__SLI_CPC_LOG(WRN, WARN, fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__)
#define SLI_CPC_LOG_INFO(fmt, ...)                                                                 \
	__SLI_CPC_LOG(INFO, INFO, fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__)
#define SLI_CPC_LOG_DEBUG(fmt, ...)                                                                \
	__SLI_CPC_LOG(DBG, DEBUG, fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__)

#endif /* SLI_CPC_LOG_H */
