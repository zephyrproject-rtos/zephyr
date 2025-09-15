/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* @file
 * @brief wpa_cli implementation for Zephyr OS
 */

#include <stdlib.h>
#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/init.h>


#include "supp_main.h"

#include "common.h"
#include "wpa_supplicant_i.h"
#include "wpa_cli_zephyr.h"
#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
#include "hostapd.h"
#include "hapd_main.h"
#include "hostapd_cli_zephyr.h"
#endif

static int cmd_wpa_cli(const struct shell *sh, size_t argc, const char *argv[])
{
	struct net_if *iface = NULL;
	char if_name[CONFIG_NET_INTERFACE_NAME_LEN + 1];
	struct wpa_supplicant *wpa_s = NULL;
	size_t arg_offset = 1;
	int idx = -1;
	bool iface_found = false;

	if (argc > 2 &&
	    ((strcmp(argv[1], "-i") == 0) ||
	     (strncmp(argv[1], "-i", 2) == 0 && argv[1][2] != '\0'))) {
		/* Handle both "-i 2" and "-i2" */
		if (strcmp(argv[1], "-i") == 0) {
			idx = strtol(argv[2], NULL, 10);
			arg_offset = 3;
		} else {
			idx = strtol(&argv[1][2], NULL, 10);
			arg_offset = 2;
		}
		iface = net_if_get_by_index(idx);
		if (!iface) {
			shell_error(sh, "Interface index %d not found", idx);
			return -ENODEV;
		}
		net_if_get_name(iface, if_name, sizeof(if_name));
		if_name[sizeof(if_name) - 1] = '\0';
		iface_found = true;
	} else {
		/* Default to first Wi-Fi interface */
		iface = net_if_get_first_wifi();
		if (!iface) {
			shell_error(sh, "No Wi-Fi interface found");
			return -ENOENT;
		}
		net_if_get_name(iface, if_name, sizeof(if_name));
		if_name[sizeof(if_name) - 1] = '\0';
		arg_offset = 1;
		iface_found = true;
	}

	if (!iface_found) {
		shell_error(sh, "No interface found");
		return -ENODEV;
	}

	wpa_s = zephyr_get_handle_by_ifname(if_name);
	if (!wpa_s) {
		shell_error(sh, "No wpa_supplicant context for interface '%s'", if_name);
		return -ENODEV;
	}

	if (argc <= arg_offset) {
		shell_error(sh, "Missing argument");
		return -EINVAL;
	}

	argv[argc] = "interactive";
	argc++;

	/* Remove wpa_cli from the argument list */
	return zephyr_wpa_ctrl_zephyr_cmd(wpa_s->ctrl_conn, argc - arg_offset, &argv[arg_offset]);
}

#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
static int cmd_hostapd_cli(const struct shell *sh, size_t argc, const char *argv[])
{
	struct net_if *iface = NULL;
	size_t arg_offset = 1;
	struct hostapd_iface *hapd_iface;
	int idx = -1;
	bool iface_found = false;
	char if_name[CONFIG_NET_INTERFACE_NAME_LEN + 1];
	int ret;

	if (argc > 2 &&
	    ((strcmp(argv[1], "-i") == 0) ||
	     (strncmp(argv[1], "-i", 2) == 0 && argv[1][2] != '\0'))) {
		/* Handle both "-i 2" and "-i2" */
		if (strcmp(argv[1], "-i") == 0) {
			idx = strtol(argv[2], NULL, 10);
			arg_offset = 3;
		} else {
			idx = strtol(&argv[1][2], NULL, 10);
			arg_offset = 2;
		}
		iface = net_if_get_by_index(idx);
		if (!iface) {
			shell_error(sh, "Interface index %d not found", idx);
			return -ENODEV;
		}
		iface_found = true;
	} else {
		iface = net_if_get_first_wifi();
		if (!iface) {
			shell_error(sh, "No Wi-Fi interface found");
			return -ENOENT;
		}
		arg_offset = 1;
		iface_found = true;
	}

	if (!iface_found) {
		shell_error(sh, "No interface found");
		return -ENODEV;
	}

	ret = net_if_get_name(iface, if_name, sizeof(if_name));
	if (!ret) {
		shell_error(sh, "Cannot get interface name (%d)", ret);
		return -ENODEV;
	}

	hapd_iface = zephyr_get_hapd_handle_by_ifname(if_name);
	if (!hapd_iface) {
		shell_error(sh, "No hostapd context for interface '%s'", if_name);
		return -ENODEV;
	}

	if (argc <= arg_offset) {
		shell_error(sh, "Missing argument");
		return -EINVAL;
	}

	argv[argc] = "interactive";
	argc++;

	/* Remove hostapd_cli from the argument list */
	return zephyr_hostapd_ctrl_zephyr_cmd(hapd_iface->ctrl_conn, argc - arg_offset,
					      &argv[arg_offset]);
}
#endif

/* Persisting with "wpa_cli" naming for compatibility with Wi-Fi
 * certification applications and scripts.
 */
SHELL_CMD_REGISTER(wpa_cli,
		   NULL,
		   "wpa_cli [-i idx] <command> (only for internal use)",
		   cmd_wpa_cli);
#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
SHELL_CMD_REGISTER(hostapd_cli, NULL,
		   "hostapd_cli [-i idx] <command> (only for internal use)",
		   cmd_hostapd_cli);
#endif
