/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* @file
 * @brief wpa_cli implementation for Zephyr OS
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/net/net_if.h>
#include <zephyr/init.h>


#include "supp_main.h"

#include "common.h"
#include "wpa_supplicant_i.h"
#include "wpa_cli_zephyr.h"
#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
#include "hostapd_cli_zephyr.h"
#endif

static int cmd_wpa_cli(const struct shell *sh,
			  size_t argc,
			  const char *argv[])
{
	struct net_if *iface = net_if_get_first_wifi();
	char if_name[CONFIG_NET_INTERFACE_NAME_LEN + 1];
	struct wpa_supplicant *wpa_s = NULL;

	ARG_UNUSED(sh);

	if (iface == NULL) {
		shell_error(sh, "No Wifi interface found");
		return -ENOENT;
	}

	net_if_get_name(iface, if_name, sizeof(if_name));
	wpa_s = zephyr_get_handle_by_ifname(if_name);

	if (argc == 1) {
		shell_error(sh, "Missing argument");
		return -EINVAL;
	}

	argv[argc] = "interactive";
	argc++;

	/* Remove wpa_cli from the argument list */
	return zephyr_wpa_ctrl_zephyr_cmd(wpa_s->ctrl_conn, argc - 1, &argv[1]);
}

#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
static int cmd_hostapd_cli(const struct shell *sh, size_t argc, const char *argv[])
{
	ARG_UNUSED(sh);

	if (argc == 1) {
		shell_error(sh, "Missing argument");
		return -EINVAL;
	}

	argv[argc] = "interactive";
	argc++;

	/* Remove hostapd_cli from the argument list */
	return zephyr_hostapd_ctrl_zephyr_cmd(argc - 1, &argv[1]);
}
#endif

/* Persisting with "wpa_cli" naming for compatibility with Wi-Fi
 * certification applications and scripts.
 */
SHELL_CMD_REGISTER(wpa_cli,
		   NULL,
		   "wpa_cli commands (only for internal use)",
		   cmd_wpa_cli);
#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
SHELL_CMD_REGISTER(hostapd_cli, NULL, "hostapd_cli commands (only for internal use)",
		   cmd_hostapd_cli);
#endif
