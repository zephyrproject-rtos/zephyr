/** @file
 * @brief Bluetooth Link Layer shell functions
 *
 * This is not to be included by the application.
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LL_H
#define __LL_H

int cmd_ll_addr_read(const struct shell *sh, size_t argc, char *argv[]);

int cmd_advx(const struct shell *sh, size_t  argc, char *argv[]);
int cmd_scanx(const struct shell *sh, size_t  argc, char *argv[]);

int cmd_test_tx(const struct shell *sh, size_t  argc, char *argv[]);
int cmd_test_rx(const struct shell *sh, size_t  argc, char *argv[]);
int cmd_test_end(const struct shell *sh, size_t  argc, char *argv[]);
#endif /* __LL_H */
