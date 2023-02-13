# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(
	shell.c
	utils.c
	)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_SHELL_HEALTH_CLI health.c)

zephyr_library_sources_ifdef(CONFIG_BT_MESH_SHELL_CFG_CLI cfg.c)
