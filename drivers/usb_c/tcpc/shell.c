/*
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/usb_c/tcpci_priv.h>
#include <zephyr/usb_c/usbc.h>
#include <zephyr/usb_c/tcpci.h>
#include <zephyr/shell/shell.h>

/** Macro used to call the dump_std_reg function from USB-C connector node */
#define TCPC_DUMP_NODE(node) ret |= tcpc_dump_std_reg(DEVICE_DT_GET(DT_PROP(node, tcpc)));

/**
 * @brief Shell command that dumps standard registers of TCPCs for all available USB-C ports
 *
 * @param sh Shell structure
 * @param argc Unused arguments count
 * @param argv Unused arguments
 * @return int ORed return values of all the functions executed, 0 in case of success
 */
static int cmd_tcpc_dump(const struct shell *sh, size_t argc, char **argv)
{
	int ret = 0;

	DT_FOREACH_STATUS_OKAY(usb_c_connector, TCPC_DUMP_NODE);

	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_tcpc_cmds,
			       SHELL_CMD(dump, NULL, "Dump TCPC registers", cmd_tcpc_dump),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(tcpc, &sub_tcpc_cmds, "TCPC (USB-C PD) diagnostics", NULL);
