/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* @file
 * @brief NRF Wi-Fi debug shell module
 */
#include <stdlib.h>
#include <zephyr/shell/shell.h>

SHELL_SUBCMD_SET_CREATE(nrf70_commands, (nrf70));

SHELL_CMD_REGISTER(nrf70, &nrf70_commands, "nRF70 specific commands", NULL);
