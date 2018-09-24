/** @file
 @brief GATT shell functions

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __GATT_H
#define __GATT_H

void cmd_gatt_show_db(const struct shell *shell, size_t argc, char *argv[]);
void cmd_gatt_exchange_mtu(const struct shell *shell,
			   size_t argc, char *argv[]);
void cmd_gatt_discover(const struct shell *shell, size_t argc, char *argv[]);
void cmd_gatt_read(const struct shell *shell, size_t argc, char *argv[]);
void cmd_gatt_mread(const struct shell *shell, size_t argc, char *argv[]);
void cmd_gatt_write(const struct shell *shell, size_t argc, char *argv[]);
void cmd_gatt_write_without_rsp(const struct shell *shell,
			       size_t argc, char *argv[]);
void cmd_gatt_subscribe(const struct shell *shell, size_t argc, char *argv[]);
void cmd_gatt_unsubscribe(const struct shell *shell, size_t argc, char *argv[]);
void cmd_gatt_register_test_svc(const struct shell *shell,
			       size_t argc, char *argv[]);
void cmd_gatt_unregister_test_svc(const struct shell *shell,
				 size_t argc, char *argv[]);
void cmd_gatt_write_cmd_metrics(const struct shell *shell,
			       size_t argc, char *argv[]);

#endif /* __GATT_H */
