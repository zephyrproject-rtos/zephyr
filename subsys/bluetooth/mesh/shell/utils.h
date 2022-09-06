/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/shell/shell.h>

bool bt_mesh_shell_mdl_first_get(uint16_t id, struct bt_mesh_model **mod);

int bt_mesh_shell_mdl_instance_set(const struct shell *shell, struct bt_mesh_model **mod,
			      uint16_t mod_id, uint8_t elem_idx);

int bt_mesh_shell_mdl_print_all(const struct shell *shell, uint16_t mod_id);

int bt_mesh_shell_mdl_cmds_help(const struct shell *shell, size_t argc, char **argv);
