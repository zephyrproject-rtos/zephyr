/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LOG_BACKEND_ADSP_MTRACE_H_
#define ZEPHYR_LOG_BACKEND_ADSP_MTRACE_H_

#include <stdint.h>
#include <stddef.h>

/**
 *@brief mtracelogger requires a hook for IPC messages
 *
 * When new log data is added to the SRAM buffer, a IPC message
 * should be sent to the host. This hook function pointer allows
 * for that.
 */
typedef void(*adsp_mtrace_log_hook_t)(size_t written, size_t space_left);

/**
 * @brief Initialize the Intel ADSP mtrace logger
 *
 * @param hook Function is called after each write to the SRAM buffer
 *             It is up to the author of the hook to serialize if needed.
 */
void adsp_mtrace_log_init(adsp_mtrace_log_hook_t hook);

const struct log_backend *log_backend_adsp_mtrace_get(void);

#endif /* ZEPHYR_LOG_BACKEND_ADSP_MTRACE_H_ */
