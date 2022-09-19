# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(
	shell.c
	utils.c
	)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_HEALTH_CLI health_cli.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_CFG_CLI cfg_cli.c)
