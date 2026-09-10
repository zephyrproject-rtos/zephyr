/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/mesh.h>
#include <zephyr/shell/shell.h>
#include <ctype.h>
#include <string.h>

#include "mesh/net.h"
#include "mesh/access.h"
#include "utils.h"

bool bt_mesh_shell_mdl_first_get(uint16_t id, const struct bt_mesh_model **mod)
{
	const struct bt_mesh_comp *comp = bt_mesh_comp_get();

	for (int i = 0; i < comp->elem_count; i++) {
		*mod = bt_mesh_model_find(&comp->elem[i], id);
		if (*mod) {
			return true;
		}
	}

	return false;
}

int bt_mesh_shell_mdl_instance_set(const struct shell *sh, const struct bt_mesh_model **mod,
				 uint16_t mod_id, uint8_t elem_idx)
{
	const struct bt_mesh_model *mod_temp;
	const struct bt_mesh_comp *comp = bt_mesh_comp_get();

	if (elem_idx >= comp->elem_count) {
		shell_error(sh, "Invalid element index");
		return -EINVAL;
	}

	mod_temp = bt_mesh_model_find(&comp->elem[elem_idx], mod_id);

	if (mod_temp) {
		*mod = mod_temp;
	} else {
		shell_error(sh, "Unable to find model instance for element index %d", elem_idx);
		return -ENODEV;
	}

	return 0;
}

int bt_mesh_shell_mdl_print_all(const struct shell *sh, uint16_t mod_id)
{
	const struct bt_mesh_comp *comp = bt_mesh_comp_get();
	const struct bt_mesh_model *mod;

	for (int i = 0; i < comp->elem_count; i++) {
		mod = bt_mesh_model_find(&comp->elem[i], mod_id);
		if (mod) {
			shell_print(sh,
				    "Client model instance found at addr 0x%.4X. Element index: %d",
				    comp->elem[i].rt->addr, mod->rt->elem_idx);
		}
	}

	return 0;
}

int bt_mesh_shell_mdl_cmds_help(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(
		sh,
		"\nFor a detailed description of the commands and arguments in this shell module,\n"
		"please refer to the Zephyr Project documentation online.\n");

	if (argc == 1) {
		shell_help(sh);
		return 0;
	}

	shell_error(sh, "%s unknown command: %s", argv[0], argv[1]);
	return -EINVAL;
}
