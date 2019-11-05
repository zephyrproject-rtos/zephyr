/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LOG_LINK_OPEN_AMP_H__
#define LOG_LINK_OPEN_AMP_H__

typedef void (*log_link_open_amp_clbk_t)(u8_t *data, size_t buf);

int log_link_open_amp_init(log_link_open_amp_clbk_t clbk);
void log_link_open_amp_send(u8_t *data, size_t buf);
#endif /* LOG_LINK_OPEN_AMP_H__ */
