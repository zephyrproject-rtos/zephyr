/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/shell/shell.h>

#define BT_MESH_SHELL_MDL_INSTANCE_CMDS(cmd_set_name, mod_id, mod_ptr) \
	static int cmd_##cmd_set_name##_get_all(const struct shell *sh, size_t argc, char *argv[]) \
	{ \
		return bt_mesh_shell_mdl_print_all(sh, (mod_id)); \
	} \
	\
	static int cmd_##cmd_set_name##_set(const struct shell *sh, size_t argc, \
					    char *argv[]) \
	{ \
		int err = 0; \
		uint8_t elem_idx = shell_strtoul(argv[1], 0, &err); \
	\
		if (err) { \
			shell_warn(sh, "Unable to parse input string arg"); \
			return err; \
		} \
	\
		return bt_mesh_shell_mdl_instance_set(sh, &(mod_ptr), (mod_id), elem_idx); \
	} \
	\
	SHELL_STATIC_SUBCMD_SET_CREATE(cmd_set_name, \
			       SHELL_CMD_ARG(set, NULL, "<ElemIdx>", cmd_##cmd_set_name##_set, 2,\
					     0), \
			       SHELL_CMD_ARG(get-all, NULL, NULL, cmd_##cmd_set_name##_get_all, 1,\
					     0), \
			       SHELL_SUBCMD_SET_END);

bool bt_mesh_shell_mdl_first_get(uint16_t id, struct bt_mesh_model **mod);

int bt_mesh_shell_mdl_instance_set(const struct shell *sh, struct bt_mesh_model **mod,
			      uint16_t mod_id, uint8_t elem_idx);

int bt_mesh_shell_mdl_print_all(const struct shell *sh, uint16_t mod_id);

int bt_mesh_shell_mdl_cmds_help(const struct shell *sh, size_t argc, char **argv);
