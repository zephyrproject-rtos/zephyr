/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LOG_BACKEND_OPEN_AMP_H__
#define LOG_BACKEND_OPEN_AMP_H__
typedef void (*log_backend_open_amp_clbk_t)(u8_t *data, size_t buf);

void log_backend_open_amp_init(log_backend_open_amp_clbk_t clbk);
void log_backend_open_amp_send(u8_t *data, size_t buf);
#endif /* LOG_BACKEND_OPEN_AMP_H__ */
