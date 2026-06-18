/**
 * Copyright 2023-2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file nxp_wifi_shell.c
 * Wi-Fi driver shell adapter layer
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include "nxp_wifi_drv.h"

static struct nxp_wifi_dev *nxp_wifi;

void nxp_wifi_shell_register(struct nxp_wifi_dev *dev)
{
	/* only one instance supported */
	if (nxp_wifi) {
		return;
	}

	nxp_wifi = dev;
}

static int nxp_wifi_shell_cmd(const struct shell *sh, size_t argc, char **argv)
{
	if (nxp_wifi == NULL) {
		shell_print(sh, "no nxp_wifi device registered");
		return -ENOEXEC;
	}

	if (argc < 2) {
		shell_help(sh);
		return -ENOEXEC;
	}

	k_mutex_lock(&nxp_wifi->mutex, K_FOREVER);

	nxp_wifi_request(argc, argv);

	k_mutex_unlock(&nxp_wifi->mutex);

	return 0;
}

SHELL_CMD_ARG_REGISTER(nxp_wifi, NULL, "NXP Wi-Fi commands (Use help)", nxp_wifi_shell_cmd, 2,
		       CONFIG_SHELL_ARGC_MAX);
