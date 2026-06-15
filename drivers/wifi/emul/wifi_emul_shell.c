/*
 * SPDX-FileCopyrightText: 2026 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/wifi/wifi_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#define WIFI_EMUL_DEV_ENTRY(node_id) DEVICE_DT_GET(node_id),

static const struct device *const wifi_emul_devs[] = {
	DT_FOREACH_STATUS_OKAY(zephyr_wifi_emul, WIFI_EMUL_DEV_ENTRY)};

/* CONFIG_WIFI_EMUL_SHELL depends on WIFI_EMUL, which requires at least one
 * enabled instance, so the array is never empty and indexing [0] is safe.
 */
BUILD_ASSERT(ARRAY_SIZE(wifi_emul_devs) > 0, "No emulated Wi-Fi devices");

/* Resolve the target device from an interface index, or default to the sole
 * instance when no index is given (iface_index < 0).
 */
static const struct device *wifi_emul_dev_resolve(const struct shell *sh, int iface_index)
{
	if (iface_index >= 0) {
		struct net_if *iface = net_if_get_by_index(iface_index);
		const struct device *dev;

		if (iface == NULL) {
			shell_error(sh, "No interface with index %d", iface_index);
			return NULL;
		}

		dev = net_if_get_device(iface);
		ARRAY_FOR_EACH(wifi_emul_devs, i) {
			if (wifi_emul_devs[i] == dev) {
				return dev;
			}
		}

		shell_error(sh, "Interface %d is not an emulated Wi-Fi device", iface_index);
		return NULL;
	}

	if (ARRAY_SIZE(wifi_emul_devs) > 1) {
		shell_error(sh, "Multiple emulated Wi-Fi devices, select one with -i <iface>");
		return NULL;
	}

	return wifi_emul_devs[0];
}

/* Resolve the target device by scanning argv for -i/--iface. Used by the
 * getopt-based "ap add" command, where the option is consumed by getopt too.
 */
static const struct device *wifi_emul_dev_get(const struct shell *sh, size_t argc, char *argv[])
{
	int iface_index = -1;

	for (size_t i = 1; i < argc; i++) {
		if ((strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--iface") == 0) &&
		    (i + 1 < argc)) {
			int err = 0;

			iface_index = shell_strtol(argv[i + 1], 10, &err);
			if (err || iface_index < 0) {
				shell_error(sh, "Invalid interface index: %s", argv[i + 1]);
				return NULL;
			}
			break;
		}
	}

	return wifi_emul_dev_resolve(sh, iface_index);
}

/* Resolve the target device and collect positional arguments, excluding the
 * -i/--iface option and its value. Avoids getopt so negative positional
 * values (RSSI, reset status) are not mistaken for options.
 */
static const struct device *wifi_emul_args_parse(const struct shell *sh, size_t argc,
						 char *argv[], const char *pos[],
						 size_t *pos_count, size_t max_pos)
{
	int iface_index = -1;
	size_t count = 0;

	for (size_t i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--iface") == 0) {
			int err = 0;

			if (i + 1 >= argc) {
				shell_error(sh, "Missing interface index after %s", argv[i]);
				return NULL;
			}

			iface_index = shell_strtol(argv[++i], 10, &err);
			if (err || iface_index < 0) {
				shell_error(sh, "Invalid interface index: %s", argv[i]);
				return NULL;
			}
			continue;
		}

		if (count >= max_pos) {
			shell_error(sh, "Too many arguments");
			return NULL;
		}

		pos[count++] = argv[i];
	}

	*pos_count = count;

	return wifi_emul_dev_resolve(sh, iface_index);
}

static int wifi_emul_parse_band(const char *str, enum wifi_frequency_bands *band)
{
	if (strcmp(str, "2") == 0) {
		*band = WIFI_FREQ_BAND_2_4_GHZ;
	} else if (strcmp(str, "5") == 0) {
		*band = WIFI_FREQ_BAND_5_GHZ;
	} else if (strcmp(str, "6") == 0) {
		*band = WIFI_FREQ_BAND_6_GHZ;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int cmd_wifi_emul_ap_add(const struct shell *sh, size_t argc, char *argv[])
{
	static const struct sys_getopt_option long_options[] = {
		{"ssid", sys_getopt_required_argument, 0, 's'},
		{"bssid", sys_getopt_required_argument, 0, 'b'},
		{"channel", sys_getopt_required_argument, 0, 'c'},
		{"band", sys_getopt_required_argument, 0, 'B'},
		{"security", sys_getopt_required_argument, 0, 'k'},
		{"psk", sys_getopt_required_argument, 0, 'p'},
		{"rssi", sys_getopt_required_argument, 0, 'r'},
		{"hidden", sys_getopt_no_argument, 0, 'H'},
		{"iface", sys_getopt_required_argument, 0, 'i'},
		{0, 0, 0, 0}};
	struct wifi_emul_ap ap = {0};
	const struct device *dev;
	bool have_ssid = false;
	bool have_bssid = false;
	bool have_channel = false;
	bool have_band = false;
	bool have_security = false;
	int opt;
	int opt_index = 0;
	int err = 0;
	int ret;
	long val;

	dev = wifi_emul_dev_get(sh, argc, argv);
	if (dev == NULL) {
		return -ENODEV;
	}

	while ((opt = sys_getopt_long(argc, argv, "s:b:c:B:k:p:r:Hi:", long_options,
				      &opt_index)) != -1) {
		struct sys_getopt_state *state = sys_getopt_state_get();
		size_t len;

		switch (opt) {
		case 's':
			len = strlen(state->optarg);
			if (len == 0 || len > WIFI_SSID_MAX_LEN) {
				shell_error(sh, "SSID must be 1..%d characters", WIFI_SSID_MAX_LEN);
				return -EINVAL;
			}
			memcpy(ap.ssid, state->optarg, len);
			ap.ssid_length = (uint8_t)len;
			have_ssid = true;
			break;
		case 'b':
			if (net_bytes_from_str(ap.bssid, sizeof(ap.bssid), state->optarg) < 0) {
				shell_error(sh, "Invalid BSSID: %s", state->optarg);
				return -EINVAL;
			}
			have_bssid = true;
			break;
		case 'c':
			val = shell_strtol(state->optarg, 10, &err);
			if (err || val <= 0 || val > UINT8_MAX) {
				shell_error(sh, "Invalid channel: %s", state->optarg);
				return -EINVAL;
			}
			ap.channel = (uint8_t)val;
			have_channel = true;
			break;
		case 'B':
			if (wifi_emul_parse_band(state->optarg, &ap.band) < 0) {
				shell_error(sh, "Invalid band: %s (2, 5 or 6)", state->optarg);
				return -EINVAL;
			}
			have_band = true;
			break;
		case 'k':
			val = shell_strtol(state->optarg, 10, &err);
			if (err || val < 0 || val >= __WIFI_SECURITY_TYPE_AFTER_LAST) {
				shell_error(sh, "Invalid security type: %s", state->optarg);
				return -EINVAL;
			}
			ap.security = (enum wifi_security_type)val;
			have_security = true;
			break;
		case 'p':
			len = strlen(state->optarg);
			if (len > WIFI_PSK_MAX_LEN) {
				shell_error(sh, "PSK must be at most %d characters",
					    WIFI_PSK_MAX_LEN);
				return -EINVAL;
			}
			memcpy(ap.psk, state->optarg, len);
			break;
		case 'r':
			val = shell_strtol(state->optarg, 10, &err);
			if (err || val < INT8_MIN || val > INT8_MAX) {
				shell_error(sh, "Invalid RSSI: %s", state->optarg);
				return -EINVAL;
			}
			ap.rssi = (int8_t)val;
			break;
		case 'H':
			ap.hidden = true;
			break;
		case 'i':
			/* Handled by wifi_emul_dev_get() */
			break;
		default:
			shell_error(sh, "Unknown option");
			return -EINVAL;
		}
	}

	if (!have_ssid || !have_bssid || !have_channel || !have_band || !have_security) {
		shell_error(sh, "Missing required option, need -s, -b, -c, -B and -k");
		return -EINVAL;
	}

	ret = wifi_emul_ap_add(dev, &ap);
	if (ret < 0) {
		shell_error(sh, "Failed to add AP (%d)", ret);
		return ret;
	}

	shell_print(sh, "Added AP \"%.*s\"", ap.ssid_length, ap.ssid);

	return 0;
}

static int cmd_wifi_emul_ap_remove(const struct shell *sh, size_t argc, char *argv[])
{
	const char *pos[1];
	size_t pos_count;
	const struct device *dev;
	uint8_t bssid[WIFI_MAC_ADDR_LEN];
	int ret;

	dev = wifi_emul_args_parse(sh, argc, argv, pos, &pos_count, ARRAY_SIZE(pos));
	if (dev == NULL) {
		return -ENODEV;
	}

	if (pos_count != 1) {
		shell_error(sh, "Usage: wifi emul ap remove <bssid> [-i <iface>]");
		return -EINVAL;
	}

	if (net_bytes_from_str(bssid, sizeof(bssid), pos[0]) < 0) {
		shell_error(sh, "Invalid BSSID: %s", pos[0]);
		return -EINVAL;
	}

	ret = wifi_emul_ap_remove(dev, bssid);
	if (ret < 0) {
		shell_error(sh, "Failed to remove AP (%d)", ret);
		return ret;
	}

	shell_print(sh, "Removed AP");

	return 0;
}

static int cmd_wifi_emul_ap_clear(const struct shell *sh, size_t argc, char *argv[])
{
	size_t pos_count;
	const struct device *dev;

	dev = wifi_emul_args_parse(sh, argc, argv, NULL, &pos_count, 0);
	if (dev == NULL) {
		return -ENODEV;
	}

	wifi_emul_ap_clear(dev);
	shell_print(sh, "Cleared all APs");

	return 0;
}

static int cmd_wifi_emul_ap_rssi(const struct shell *sh, size_t argc, char *argv[])
{
	const char *pos[2];
	size_t pos_count;
	const struct device *dev;
	uint8_t bssid[WIFI_MAC_ADDR_LEN];
	int ret;
	int err = 0;
	long val;

	dev = wifi_emul_args_parse(sh, argc, argv, pos, &pos_count, ARRAY_SIZE(pos));
	if (dev == NULL) {
		return -ENODEV;
	}

	if (pos_count != 2) {
		shell_error(sh, "Usage: wifi emul ap rssi <bssid> <rssi> [-i <iface>]");
		return -EINVAL;
	}

	if (net_bytes_from_str(bssid, sizeof(bssid), pos[0]) < 0) {
		shell_error(sh, "Invalid BSSID: %s", pos[0]);
		return -EINVAL;
	}

	val = shell_strtol(pos[1], 10, &err);
	if (err || val < INT8_MIN || val > INT8_MAX) {
		shell_error(sh, "Invalid RSSI: %s", pos[1]);
		return -EINVAL;
	}

	ret = wifi_emul_ap_update_rssi(dev, bssid, (int8_t)val);
	if (ret < 0) {
		shell_error(sh, "Failed to update RSSI (%d)", ret);
		return ret;
	}

	shell_print(sh, "Updated RSSI");

	return 0;
}

static void wifi_emul_ap_list_cb(const struct wifi_emul_ap *ap, void *user_data)
{
	const struct shell *sh = user_data;

	shell_print(sh,
		    "SSID: %-32.*s  BSSID: %02X:%02X:%02X:%02X:%02X:%02X  band: %-6s  "
		    "channel: %3u  security: %-12s  RSSI: %4d dBm%s",
		    ap->ssid_length, ap->ssid, ap->bssid[0], ap->bssid[1], ap->bssid[2],
		    ap->bssid[3], ap->bssid[4], ap->bssid[5], wifi_band_txt(ap->band),
		    ap->channel, wifi_security_txt(ap->security), ap->rssi,
		    ap->hidden ? "  (hidden)" : "");
}

static int cmd_wifi_emul_ap_list(const struct shell *sh, size_t argc, char *argv[])
{
	size_t pos_count;
	const struct device *dev;

	dev = wifi_emul_args_parse(sh, argc, argv, NULL, &pos_count, 0);
	if (dev == NULL) {
		return -ENODEV;
	}

	wifi_emul_ap_foreach(dev, wifi_emul_ap_list_cb, (void *)sh);

	return 0;
}

static int cmd_wifi_emul_scan_delay(const struct shell *sh, size_t argc, char *argv[])
{
	const char *pos[1];
	size_t pos_count;
	const struct device *dev;
	int err = 0;
	long val;

	dev = wifi_emul_args_parse(sh, argc, argv, pos, &pos_count, ARRAY_SIZE(pos));
	if (dev == NULL) {
		return -ENODEV;
	}

	if (pos_count != 1) {
		shell_error(sh, "Usage: wifi emul scan_delay <ms> [-i <iface>]");
		return -EINVAL;
	}

	val = shell_strtol(pos[0], 10, &err);
	if (err || val < 0) {
		shell_error(sh, "Delay must be a non-negative number of milliseconds");
		return -EINVAL;
	}

	wifi_emul_set_scan_delay(dev, K_MSEC(val));
	shell_print(sh, "Scan delay set to %ld ms", val);

	return 0;
}

static int cmd_wifi_emul_connect_delay(const struct shell *sh, size_t argc, char *argv[])
{
	const char *pos[1];
	size_t pos_count;
	const struct device *dev;
	int err = 0;
	long val;

	dev = wifi_emul_args_parse(sh, argc, argv, pos, &pos_count, ARRAY_SIZE(pos));
	if (dev == NULL) {
		return -ENODEV;
	}

	if (pos_count != 1) {
		shell_error(sh, "Usage: wifi emul connect_delay <ms> [-i <iface>]");
		return -EINVAL;
	}

	val = shell_strtol(pos[0], 10, &err);
	if (err || val < 0) {
		shell_error(sh, "Delay must be a non-negative number of milliseconds");
		return -EINVAL;
	}

	wifi_emul_set_connect_delay(dev, K_MSEC(val));
	shell_print(sh, "Connect delay set to %ld ms", val);

	return 0;
}

static int cmd_wifi_emul_force_status(const struct shell *sh, size_t argc, char *argv[])
{
	const char *pos[1];
	size_t pos_count;
	const struct device *dev;
	int err = 0;
	long val;

	dev = wifi_emul_args_parse(sh, argc, argv, pos, &pos_count, ARRAY_SIZE(pos));
	if (dev == NULL) {
		return -ENODEV;
	}

	if (pos_count != 1) {
		shell_error(sh, "Usage: wifi emul force_status <status|-1> [-i <iface>]");
		return -EINVAL;
	}

	val = shell_strtol(pos[0], 10, &err);
	if (err) {
		shell_error(sh, "Invalid status: %s", pos[0]);
		return -EINVAL;
	}

	wifi_emul_force_connect_status(dev, (int)val);

	if (val < 0) {
		shell_print(sh, "Forced connect status cleared");
	} else {
		shell_print(sh, "Forced connect status set to %ld", val);
	}

	return 0;
}

static int cmd_wifi_emul_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	const char *pos[1];
	size_t pos_count;
	const struct device *dev;
	int reason = WIFI_REASON_DISCONN_UNSPECIFIED;
	int ret;
	int err = 0;
	long val;

	dev = wifi_emul_args_parse(sh, argc, argv, pos, &pos_count, ARRAY_SIZE(pos));
	if (dev == NULL) {
		return -ENODEV;
	}

	if (pos_count > 1) {
		shell_error(sh, "Usage: wifi emul disconnect [reason] [-i <iface>]");
		return -EINVAL;
	}

	if (pos_count == 1) {
		val = shell_strtol(pos[0], 10, &err);
		if (err || val < 0) {
			shell_error(sh, "Invalid reason: %s", pos[0]);
			return -EINVAL;
		}
		reason = (int)val;
	}

	ret = wifi_emul_trigger_disconnect(dev, reason);
	if (ret < 0) {
		shell_error(sh, "Failed to trigger disconnect (%d)", ret);
		return ret;
	}

	shell_print(sh, "Disconnect triggered");

	return 0;
}

static int cmd_wifi_emul_status(const struct shell *sh, size_t argc, char *argv[])
{
	size_t pos_count;
	const struct device *dev;
	struct wifi_emul_ap ap;
	int ret;

	dev = wifi_emul_args_parse(sh, argc, argv, NULL, &pos_count, 0);
	if (dev == NULL) {
		return -ENODEV;
	}

	if (!wifi_emul_is_connected(dev)) {
		shell_print(sh, "Disconnected");
		return 0;
	}

	ret = wifi_emul_get_current_ap(dev, &ap);
	if (ret < 0) {
		shell_error(sh, "Failed to get current AP (%d)", ret);
		return ret;
	}

	shell_print(sh,
		    "Connected to \"%.*s\" (%02X:%02X:%02X:%02X:%02X:%02X), band %s, "
		    "channel %u, security %s, RSSI %d dBm",
		    ap.ssid_length, ap.ssid, ap.bssid[0], ap.bssid[1], ap.bssid[2], ap.bssid[3],
		    ap.bssid[4], ap.bssid[5], wifi_band_txt(ap.band), ap.channel,
		    wifi_security_txt(ap.security), ap.rssi);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_wifi_emul_ap,
	SHELL_CMD_ARG(add, NULL,
		      SHELL_HELP("Register a virtual access point",
				 "-s <ssid> -b <bssid> -c <channel> -B <band 2|5|6> "
				 "-k <security type> [-p <psk>] [-r <rssi>] [-H] "
				 "[-i <iface>]"),
		      cmd_wifi_emul_ap_add, 1, 20),
	SHELL_CMD_ARG(remove, NULL,
		      SHELL_HELP("Remove a virtual access point", "<bssid> [-i <iface>]"),
		      cmd_wifi_emul_ap_remove, 2, 2),
	SHELL_CMD_ARG(clear, NULL,
		      SHELL_HELP("Remove all virtual access points", "[-i <iface>]"),
		      cmd_wifi_emul_ap_clear, 1, 2),
	SHELL_CMD_ARG(rssi, NULL,
		      SHELL_HELP("Update the RSSI of a virtual access point",
				 "<bssid> <rssi> [-i <iface>]"),
		      cmd_wifi_emul_ap_rssi, 3, 2),
	SHELL_CMD_ARG(list, NULL,
		      SHELL_HELP("List registered virtual access points", "[-i <iface>]"),
		      cmd_wifi_emul_ap_list, 1, 2),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_wifi_emul,
	SHELL_CMD(ap, &sub_wifi_emul_ap, "Virtual access point management.", NULL),
	SHELL_CMD_ARG(scan_delay, NULL,
		      SHELL_HELP("Set the delay between a scan request and its results",
				 "<ms> [-i <iface>]"),
		      cmd_wifi_emul_scan_delay, 2, 2),
	SHELL_CMD_ARG(connect_delay, NULL,
		      SHELL_HELP("Set the delay between a connect request and its result",
				 "<ms> [-i <iface>]"),
		      cmd_wifi_emul_connect_delay, 2, 2),
	SHELL_CMD_ARG(force_status, NULL,
		      SHELL_HELP("Force the status of subsequent connect requests",
				 "<status|-1 to clear> [-i <iface>]"),
		      cmd_wifi_emul_force_status, 2, 2),
	SHELL_CMD_ARG(disconnect, NULL,
		      SHELL_HELP("Emulate an AP-initiated disconnect", "[reason] [-i <iface>]"),
		      cmd_wifi_emul_disconnect, 1, 3),
	SHELL_CMD_ARG(status, NULL,
		      SHELL_HELP("Show the emulated connection status", "[-i <iface>]"),
		      cmd_wifi_emul_status, 1, 2),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((wifi), emul, &sub_wifi_emul, "Emulated Wi-Fi driver control.", NULL, 0, 0);
