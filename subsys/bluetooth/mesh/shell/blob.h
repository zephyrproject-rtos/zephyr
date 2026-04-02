/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>

extern const struct bt_mesh_blob_io *bt_mesh_shell_blob_io;
extern bool bt_mesh_shell_blob_valid;

void bt_mesh_shell_blob_cmds_init(void);
