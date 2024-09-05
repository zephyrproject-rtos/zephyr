/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L /* For strnlen() */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/init.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/ethernet.h>

#include <zephyr/net/wifi_credentials.h>

#define MACSTR "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx"

static void print_network_info(void *cb_arg, const char *ssid, size_t ssid_len)
{
	int ret = 0;
	struct wifi_credentials_personal creds = {0};
	const struct shell *sh = (const struct shell *)cb_arg;

	ret = wifi_credentials_get_by_ssid_personal_struct(ssid, ssid_len, &creds);
	if (ret) {
		shell_error(sh,
			    "An error occurred when trying to load credentials for network \"%.*s\""
			    ". err: %d",
			    ssid_len, ssid, ret);
		return;
	}

	shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT,
		      "  network ssid: \"%.*s\", ssid_len: %d, type: %s", ssid_len, ssid, ssid_len,
		      wifi_security_txt(creds.header.type));

	if (creds.header.type == WIFI_SECURITY_TYPE_PSK ||
	    creds.header.type == WIFI_SECURITY_TYPE_PSK_SHA256 ||
	    creds.header.type == WIFI_SECURITY_TYPE_SAE ||
	    creds.header.type == WIFI_SECURITY_TYPE_WPA_PSK) {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT,
			      ", password: \"%.*s\", password_len: %d", creds.password_len,
			      creds.password, creds.password_len);
	}

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_BSSID) {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, ", bssid: " MACSTR,
			      creds.header.bssid[0], creds.header.bssid[1], creds.header.bssid[2],
			      creds.header.bssid[3], creds.header.bssid[4], creds.header.bssid[5]);
	}

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_2_4GHz) {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, ", band: 2.4GHz");
	}

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_5GHz) {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, ", band: 5GHz");
	}

	if (creds.header.channel) {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, ", channel: %d", creds.header.channel);
	}

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_FAVORITE) {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, ", favorite");
	}

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_MFP_REQUIRED) {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, ", MFP_REQUIRED");
	} else if (creds.header.flags & WIFI_CREDENTIALS_FLAG_MFP_DISABLED) {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, ", MFP_DISABLED");
	} else {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, ", MFP_OPTIONAL");
	}

	if (creds.header.timeout) {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, ", timeout: %d", creds.header.timeout);
	}

	shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, "\n");
}

static enum wifi_security_type parse_sec_type(const char *s)
{
	if (strcmp("OPEN", s) == 0) {
		return WIFI_SECURITY_TYPE_NONE;
	}

	if (strcmp("WPA2-PSK", s) == 0) {
		return WIFI_SECURITY_TYPE_PSK;
	}

	if (strcmp("WPA2-PSK-SHA256", s) == 0) {
		return WIFI_SECURITY_TYPE_PSK_SHA256;
	}

	if (strcmp("WPA3-SAE", s) == 0) {
		return WIFI_SECURITY_TYPE_SAE;
	}

	if (strcmp("WPA-PSK", s) == 0) {
		return WIFI_SECURITY_TYPE_WPA_PSK;
	}

	return WIFI_SECURITY_TYPE_UNKNOWN;
}

static enum wifi_frequency_bands parse_band(const char *s)
{
	if (strcmp("2.4GHz", s) == 0) {
		return WIFI_FREQ_BAND_2_4_GHZ;
	}

	if (strcmp("5GHz", s) == 0) {
		return WIFI_FREQ_BAND_5_GHZ;
	}

	if (strcmp("6GHz", s) == 0) {
		return WIFI_FREQ_BAND_6_GHZ;
	}

	return WIFI_FREQ_BAND_UNKNOWN;
}

static int cmd_add_network(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;

	if (argc < 3) {
		goto help;
	}

	if (strnlen(argv[1], WIFI_SSID_MAX_LEN + 1) > WIFI_SSID_MAX_LEN) {
		shell_error(sh, "SSID too long");
		goto help;
	}

	struct wifi_credentials_personal creds = {
		.header.ssid_len = strlen(argv[1]),
		.header.type = parse_sec_type(argv[2]),
	};

	memcpy(creds.header.ssid, argv[1], creds.header.ssid_len);

	if (creds.header.type == WIFI_SECURITY_TYPE_UNKNOWN) {
		shell_error(sh, "Cannot parse security type");
		goto help;
	}

	size_t arg_idx = 3;

	if (creds.header.type == WIFI_SECURITY_TYPE_PSK ||
	    creds.header.type == WIFI_SECURITY_TYPE_PSK_SHA256 ||
	    creds.header.type == WIFI_SECURITY_TYPE_SAE ||
	    creds.header.type == WIFI_SECURITY_TYPE_WPA_PSK) {
		/* parse passphrase */
		if (argc < 4) {
			shell_error(sh, "Missing password");
			goto help;
		}
		creds.password_len = strlen(argv[3]);
		if (creds.password_len < WIFI_PSK_MIN_LEN) {
			shell_error(sh, "Passphrase should be minimum %d characters",
				    WIFI_PSK_MIN_LEN);
			goto help;
		}
		if ((creds.password_len > CONFIG_WIFI_CREDENTIALS_SAE_PASSWORD_LENGTH &&
		     creds.header.type == WIFI_SECURITY_TYPE_SAE) ||
		    (creds.password_len > WIFI_PSK_MAX_LEN &&
		     creds.header.type != WIFI_SECURITY_TYPE_SAE)) {
			shell_error(sh, "Password is too long for this security type");
			goto help;
		}
		memcpy(creds.password, argv[3], creds.password_len);
		++arg_idx;
	}

	if (arg_idx < argc) {
		/* look for bssid */
		ret = sscanf(argv[arg_idx], MACSTR, &creds.header.bssid[0], &creds.header.bssid[1],
			     &creds.header.bssid[2], &creds.header.bssid[3], &creds.header.bssid[4],
			     &creds.header.bssid[5]);
		if (ret == 6) {
			creds.header.flags |= WIFI_CREDENTIALS_FLAG_BSSID;
			++arg_idx;
		}
	}

	if (arg_idx < argc) {
		/* look for band */
		enum wifi_frequency_bands band = parse_band(argv[arg_idx]);

		if (band == WIFI_FREQ_BAND_2_4_GHZ) {
			creds.header.flags |= WIFI_CREDENTIALS_FLAG_2_4GHz;
			++arg_idx;
		}
		if (band == WIFI_FREQ_BAND_5_GHZ) {
			creds.header.flags |= WIFI_CREDENTIALS_FLAG_5GHz;
			++arg_idx;
		}
	}

	if (arg_idx < argc) {
		/* look for channel */
		char *end;

		creds.header.channel = strtol(argv[arg_idx], &end, 10);
		if (*end == '\0') {
			++arg_idx;
		}
	}

	if (arg_idx < argc) {
		/* look for favorite flag */
		if (strncmp("favorite", argv[arg_idx], strlen("favorite")) == 0) {
			creds.header.flags |= WIFI_CREDENTIALS_FLAG_FAVORITE;
			++arg_idx;
		}
	}

	if (arg_idx < argc) {
		/* look for mfp_disabled flag */
		if (strncmp("mfp_disabled", argv[arg_idx], strlen("mfp_disabled")) == 0) {
			creds.header.flags |= WIFI_CREDENTIALS_FLAG_MFP_DISABLED;
			++arg_idx;
		} else if (strncmp("mfp_required", argv[arg_idx], strlen("mfp_required")) == 0) {
			creds.header.flags |= WIFI_CREDENTIALS_FLAG_MFP_REQUIRED;
			++arg_idx;
		}
	}

	if (arg_idx < argc) {
		/* look for timeout */
		char *end;

		creds.header.timeout = strtol(argv[arg_idx], &end, 10);
		if (*end == '\0') {
			++arg_idx;
		}
	}

	if (arg_idx != argc) {
		for (size_t i = arg_idx; i < argc; ++i) {
			shell_warn(sh, "Unparsed arg: [%s]", argv[i]);
		}
	}

	return wifi_credentials_set_personal_struct(&creds);
help:
	shell_print(sh, "Usage: wifi_cred add \"network name\""
			" {OPEN, WPA2-PSK, WPA2-PSK-SHA256, WPA3-SAE, WPA-PSK}"
			" [psk/password]"
			" [bssid]"
			" [{2.4GHz, 5GHz}]"
			" [channel]"
			" [favorite]"
			" [mfp_disabled|mfp_required]"
			" [timeout]");
	return -EINVAL;
}

static int cmd_delete_network(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc != 2) {
		shell_print(sh, "Usage: wifi_cred delete \"network name\"");
		return -EINVAL;
	}

	if (strnlen(argv[1], WIFI_SSID_MAX_LEN + 1) > WIFI_SSID_MAX_LEN) {
		shell_error(sh, "SSID too long");
		return -EINVAL;
	}

	shell_print(sh, "\tDeleting network ssid: \"%s\", ssid_len: %d", argv[1], strlen(argv[1]));
	return wifi_credentials_delete_by_ssid(argv[1], strlen(argv[1]));
}

static int cmd_list_networks(const struct shell *sh, size_t argc, char *argv[])
{
	wifi_credentials_for_each_ssid(print_network_info, (void *)sh);
	return 0;
}

#if CONFIG_WIFI_CREDENTIALS_CONNECT_STORED

static int cmd_auto_connect(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	int rc = net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, iface, NULL, 0);

	if (rc) {
		shell_error(sh,
			    "An error occurred when trying to auto-connect to a network. err: %d",
			    rc);
	}

	return 0;
}

#endif /* CONFIG_WIFI_CREDENTIALS_CONNECT_STORED */

SHELL_STATIC_SUBCMD_SET_CREATE(sub_wifi_cred,
	SHELL_CMD_ARG(add, NULL,
		      "Add network to storage.\n",
		      cmd_add_network, 0, 0),
	SHELL_CMD_ARG(delete, NULL,
		      "Delete network from storage.\n",
		      cmd_delete_network,
		      0, 0),
	SHELL_CMD_ARG(list, NULL,
		      "List stored networks.\n",
		      cmd_list_networks,
		      0, 0),

#if CONFIG_WIFI_CREDENTIALS_CONNECT_STORED
	SHELL_CMD_ARG(auto_connect, NULL,
		      "Connect to any stored network.\n",
		      cmd_auto_connect,
		      0, 0),
#endif /* CONFIG_WIFI_CREDENTIALS_CONNECT_STORED */

	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((wifi), cred, &sub_wifi_cred,
		 "Wifi credentials management.\n",
		 NULL,
		 0, 0);
