/** @file
 *  @brief Bluetooth shell functions
 *
 *  This is not to be included by the application.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_H
#define __BT_H

extern const struct shell *ctx_shell;

#define print(_sh, _ft, ...) \
	shell_fprintf(_sh ? _sh : ctx_shell, SHELL_NORMAL, _ft, ##__VA_ARGS__)
#define error(_sh, _ft, ...) \
	shell_fprintf(_sh ? _sh : ctx_shell, SHELL_ERROR, _ft, ##__VA_ARGS__)

extern struct bt_conn *default_conn;

#endif /* __BT_H */
