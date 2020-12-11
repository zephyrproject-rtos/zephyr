# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(
	CONFIG_EC_HOST_CMD_SIMULATOR
	ec_host_cmd_simulator.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE ec_host_cmd_periph_handlers.c)
