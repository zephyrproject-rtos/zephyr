# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(
	CONFIG_EC_HOST_CMD_SIMULATOR
	ec_host_cmd_simulator.c)

zephyr_library_sources_ifdef(
	CONFIG_EC_HOST_CMD_PERIPH_ESPI
	ec_host_cmd_periph_espi.c)

zephyr_library_sources_ifdef(
	CONFIG_EC_HOST_CMD_PERIPH_SHI_NPCX
	ec_host_cmd_periph_shi_npcx.c)

zephyr_library_sources_ifdef(
	CONFIG_EC_HOST_CMD_PERIPH_SHI_ITE
	ec_host_cmd_periph_shi_ite.c)
