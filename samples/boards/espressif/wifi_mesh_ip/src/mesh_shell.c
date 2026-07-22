/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Interactive "mesh" shell for the IP-over-mesh sample: report the node's
 * mesh role and dump the routing table. The IP-level checks (ping, interface
 * and route inspection) use the built-in net shell.
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/wifi/esp_wifi_mesh.h>

/* Bounded by the mesh tree size (max layers * children per node). */
#define MESH_TABLE_MAX                                                                             \
	(CONFIG_WIFI_ESP32_MESH_MAX_LAYER * CONFIG_WIFI_ESP32_MESH_MAX_CONNECTIONS + 1)

static int cmd_mesh_status(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	struct esp_wifi_mesh_status status;

	esp_wifi_mesh_get_status(&status);

	shell_print(sh, "layer:         %d", status.layer);
	shell_print(sh, "role:          %s", status.is_root ? "root" : "node");
	shell_print(sh, "routing table: %d node(s)", status.routing_table_size);
	shell_print(sh, "toDS:          %s", status.tods_reachable ? "reachable" : "unreachable");

	return 0;
}

static int cmd_mesh_table(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	static struct esp_wifi_mesh_addr macs[MESH_TABLE_MAX];
	size_t count = 0;

	if (esp_wifi_mesh_get_routing_table(macs, MESH_TABLE_MAX, &count) != 0) {
		shell_error(sh, "cannot read routing table");
		return -EIO;
	}

	shell_print(sh, "routing table: %d node(s)", (int)count);
	for (size_t i = 0; i < count; i++) {
		shell_print(sh, "%2d: %02x:%02x:%02x:%02x:%02x:%02x", (int)i, macs[i].addr[0],
			    macs[i].addr[1], macs[i].addr[2], macs[i].addr[3], macs[i].addr[4],
			    macs[i].addr[5]);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_mesh,
			       SHELL_CMD_ARG(status, NULL, "Show this node's mesh status",
					     cmd_mesh_status, 1, 0),
			       SHELL_CMD_ARG(table, NULL, "Dump the mesh routing table",
					     cmd_mesh_table, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(mesh, &sub_mesh, "Wi-Fi mesh commands", NULL);
