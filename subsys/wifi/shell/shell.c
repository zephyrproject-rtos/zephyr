/*
 * @file
 * @brief The shell client to interact with WiFi manager
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_WIFIMGR_STA) || defined(CONFIG_WIFIMGR_AP)

#include <init.h>
#include <shell/shell.h>

#include "ctrl_iface.h"
#include "os_adapter.h"
#include "sm.h"

#if defined(CONFIG_WIFIMGR_STA) && defined(CONFIG_WIFIMGR_AP)
#define WIFIMGR_CMD_COMMON_HELP	"<iface: sta or ap>"
#elif defined(CONFIG_WIFIMGR_STA) && !defined(CONFIG_WIFIMGR_AP)
#define WIFIMGR_CMD_COMMON_HELP	"<iface: sta>"
#elif defined(CONFIG_WIFIMGR_STA) && !defined(CONFIG_WIFIMGR_AP)
#define WIFIMGR_CMD_COMMON_HELP	"<iface: ap>"
#else
#define WIFIMGR_CMD_COMMON_HELP	NULL
#endif

static void wifimgr_cli_show_conf(const struct shell *shell, char *iface_name,
				  struct wifi_config *conf)
{
	if (!memiszero(conf, sizeof(struct wifi_config))) {
		shell_print(shell, "No config found!");
		return;
	}

	if (conf->ssid && strlen(conf->ssid))
		shell_print(shell, "SSID:\t\t%s", conf->ssid);
	if (conf->bssid && !is_zero_ether_addr(conf->bssid))
		shell_print(shell, "BSSID:\t\t" MACSTR "",
			    MAC2STR(conf->bssid));

	if (conf->security)
		shell_print(shell, "Security:\t%s",
			    security2str(conf->security));
	if (conf->passphrase && strlen(conf->passphrase))
		shell_print(shell, "Passphrase:\t%s", conf->passphrase);

	if (conf->band)
		shell_print(shell, "Band:\t\t%u", conf->band);
	if (conf->channel)
		shell_print(shell, "Channel:\t%u", conf->channel);
	if (conf->ch_width)
		shell_print(shell, "Channel Width:\t%u", conf->ch_width);

	shell_print(shell, "----------------");
	if (!conf->autorun)
		shell_print(shell, "Autorun:\toff");
	else
		shell_print(shell, "Autorun:\t%ums", conf->autorun);
}

static void wifimgr_cli_show_capa(const struct shell *shell, char *iface_name,
				  union wifi_drv_capa *capa)
{
	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA)) {
		shell_print(shell, "STA Capability");
		if (capa->sta.max_rtt_peers)
			shell_print(shell, "Max RTT NR:\t%u",
				    capa->sta.max_rtt_peers);
	} else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP)) {
		shell_print(shell, "AP Capability");
		if (capa->ap.max_ap_assoc_sta)
			shell_print(shell, "Max STA NR:\t%u",
				    capa->ap.max_ap_assoc_sta);
		if (capa->ap.max_acl_mac_addrs)
			shell_print(shell, "Max ACL NR:\t%u",
				    capa->ap.max_acl_mac_addrs);
	}
}

static void wifimgr_cli_show_status(const struct shell *shell, char *iface_name,
				    struct wifi_status *status)
{
	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA)) {
		shell_print(shell, "STA Status:\t%s",
			    sta_sts2str(status->state));
		if (status->own_mac && !is_zero_ether_addr(status->own_mac))
			shell_print(shell, "own MAC:\t" MACSTR "",
				    MAC2STR(status->own_mac));

		if (status->state == WIFI_STATE_STA_CONNECTED) {
			shell_print(shell, "----------------");
			if (status->u.sta.host_bssid
			    && !is_zero_ether_addr(status->u.sta.host_bssid))
				shell_print(shell, "Host BSSID:\t" MACSTR "",
					    MAC2STR(status->u.sta.host_bssid));
			shell_print(shell, "Host RSSI:\t%d",
				    status->u.sta.host_rssi);
		}
	} else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP)) {
		shell_print(shell, "AP Status:\t%s", ap_sts2str(status->state));
		if (status->own_mac && !is_zero_ether_addr(status->own_mac))
			shell_print(shell, "BSSID:\t\t" MACSTR "",
				    MAC2STR(status->own_mac));

		if (status->state == WIFI_STATE_AP_STARTED) {
			int i;
			char (*mac_addrs)[WIFI_MAC_ADDR_LEN];

			shell_print(shell, "----------------");
			shell_print(shell, "STA NR:\t%u", status->u.ap.nr_sta);
			if (status->u.ap.nr_sta) {
				shell_print(shell, "STA:");
				mac_addrs = status->u.ap.sta_mac_addrs;
				for (i = 0; i < status->u.ap.nr_sta; i++)
					shell_print(shell, "\t\t" MACSTR "",
						    MAC2STR(mac_addrs[i]));
			}

			shell_print(shell, "----------------");
			shell_print(shell, "ACL NR:\t%u", status->u.ap.nr_acl);
			if (status->u.ap.nr_acl) {
				shell_print(shell, "ACL:");
				mac_addrs = status->u.ap.acl_mac_addrs;
				for (i = 0; i < status->u.ap.nr_acl; i++)
					shell_print(shell, "\t\t" MACSTR "",
						    MAC2STR(mac_addrs[i]));
			}
		}
	}
}

static void wifimgr_cli_show_scan_res(struct wifi_scan_result *res)
{
	if (res->ssid && strlen(res->ssid))
		printf("\t%-32s", res->ssid);
	else
		printf("\t\t\t\t\t");

	if (res->bssid && !is_zero_ether_addr(res->bssid))
		printf("\t" MACSTR, MAC2STR(res->bssid));
	else
		printf("\t\t\t");

	printf("\t%s", security2str(res->security));
	printf("\t%uG\t%u\t%d\n", res->band, res->channel, res->rssi);
}

#ifdef CONFIG_WIFIMGR_STA
static void wifimgr_cli_show_rtt_resp(struct wifi_rtt_response *rtt_resp)
{
	if (rtt_resp->bssid && !is_zero_ether_addr(rtt_resp->bssid))
		printf("\t" MACSTR, MAC2STR(rtt_resp->bssid));
	else
		printf("\t\t\t");

	printf("\t%d\n", rtt_resp->range);
}
#endif

static int strtomac(char *mac_str, char *mac_addr)
{
	char *mac;
	int i;

	mac = strtok(mac_str, ":");

	for (i = 0; i < WIFI_MAC_ADDR_LEN; i++) {
		char *tail;

		mac_addr[i] = strtol(mac, &tail, 16);
		mac = strtok(NULL, ":");

		if (!mac)
			break;
	}

	if (i != (WIFI_MAC_ADDR_LEN - 1))
		return -EINVAL;

	return 0;
}

/* WiFi Manager CLI client commands */
static int wifimgr_cli_cmd_set_config(const struct shell *shell, size_t argc,
				      char *argv[])
{
	struct wifi_config conf;
	char *iface_name;
	int choice;

	if (!argv[1])
		return -EINVAL;
	iface_name = argv[1];

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA))
		shell_print(shell, "Setting STA Config ...");
	else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP))
		shell_print(shell, "Setting AP Config ...");
	else
		return -EINVAL;

	memset(&conf, 0, sizeof(conf));
	/* Load previous config */
	wifimgr_ctrl_iface_get_conf(iface_name, &conf);

	optind = 0;
	while ((choice = getopt(argc, argv, "a:b:c:m:n:p:w:")) != -1) {
		switch (choice) {
		case 'a':
			conf.autorun = atoi(optarg);
			break;
		case 'b':
			conf.band = atoi(optarg);
			if (!conf.band) {
				shell_error(shell, "invalid band!");
				return -EINVAL;
			}
			break;
		case 'c':
			conf.channel = atoi(optarg);
			if (!conf.channel) {
				shell_error(shell, "invalid channel!");
				return -EINVAL;
			}
			break;
		case 'm':
			if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA)) {
				int ret = strtomac(optarg, conf.bssid);

				if (ret) {
					shell_error(shell, "invalid BSSID!");
					return ret;
				}
			} else {
				shell_error(shell,
					    "invalid option '-%c' for '%s'",
					    choice, iface_name);
				return -EINVAL;
			}
			break;
		case 'n':
			if (!optarg || !strlen(optarg)) {
				shell_error(shell, "invalid SSID!");
				return -EINVAL;
			}
			strcpy(conf.ssid, optarg);
			break;
		case 'p':
			if (!optarg || !strlen(optarg))
				conf.security = WIFI_SECURITY_OPEN;
			else
				conf.security = WIFI_SECURITY_PSK;
			strcpy(conf.passphrase, optarg);
			break;
		case 'w':
			if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP)) {
				conf.ch_width = atoi(optarg);
				if (!conf.ch_width) {
					shell_error(shell,
						    "invalid channel width!");
					return -EINVAL;
				}
			} else {
				shell_error(shell,
					    "invalid option '-%c' for '%s'",
					    choice, iface_name);
				return -EINVAL;
			}
			break;
		default:
			return -EINVAL;
		}
	}

	return wifimgr_ctrl_iface_set_conf(iface_name, &conf);
}

static int wifimgr_cli_cmd_clear_config(const struct shell *shell, size_t argc,
					char *argv[])
{
	char *iface_name;
	struct wifi_config conf;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA))
		shell_print(shell, "Clearing STA Config ...");
	else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP))
		shell_print(shell, "Clearing AP Config ...");
	else
		return -EINVAL;

	memset(&conf, 0, sizeof(conf));

	return wifimgr_ctrl_iface_set_conf(iface_name, &conf);
}

static int wifimgr_cli_cmd_get_config(const struct shell *shell, size_t argc,
				      char *argv[])
{
	char *iface_name;
	struct wifi_config conf;
	int ret;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA))
		shell_print(shell, "STA Config");
	else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP))
		shell_print(shell, "AP Config");
	else
		return -EINVAL;

	memset(&conf, 0, sizeof(conf));
	ret = wifimgr_ctrl_iface_get_conf(iface_name, &conf);
	if (!ret)
		wifimgr_cli_show_conf(shell, iface_name, &conf);

	return ret;
}

static int wifimgr_cli_cmd_capa(const struct shell *shell, size_t argc,
				char *argv[])
{
	char *iface_name;
	union wifi_drv_capa capa;
	int ret;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	memset(&capa, 0, sizeof(capa));
	ret = wifimgr_ctrl_iface_get_capa(iface_name, &capa);
	if (!ret)
		wifimgr_cli_show_capa(shell, iface_name, &capa);

	return ret;
}

static int wifimgr_cli_cmd_status(const struct shell *shell, size_t argc,
				  char *argv[])
{
	char *iface_name;
	struct wifi_status sts;
	int ret;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	memset(&sts, 0, sizeof(sts));
	ret = wifimgr_ctrl_iface_get_status(iface_name, &sts);
	if (!ret)
		wifimgr_cli_show_status(shell, iface_name, &sts);

	return ret;
}

static int wifimgr_cli_cmd_open(const struct shell *shell, size_t argc,
				char *argv[])
{
	char *iface_name;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_ctrl_iface_open(iface_name);
}

static int wifimgr_cli_cmd_close(const struct shell *shell, size_t argc,
				 char *argv[])
{
	char *iface_name;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_ctrl_iface_close(iface_name);
}

static int wifimgr_cli_cmd_scan(const struct shell *shell, size_t argc,
				char *argv[])
{
	struct wifi_scan_params params;
	char *iface_name;
	int choice;

	if (!argv[1])
		return -EINVAL;
	iface_name = argv[1];

	memset(&params, 0, sizeof(params));

	optind = 0;
	while ((choice = getopt(argc, argv, "b:c:")) != -1) {
		switch (choice) {
		case 'b':
			params.band = atoi(optarg);
			if (!params.band) {
				shell_error(shell, "invalid band!");
				return -EINVAL;
			}
			break;
		case 'c':
			params.channel = atoi(optarg);
			if (!params.channel) {
				shell_error(shell, "invalid channel!");
				return -EINVAL;
			}
			break;
		default:
			return -EINVAL;
		}
	}

	return wifimgr_ctrl_iface_scan(iface_name, &params,
				       wifimgr_cli_show_scan_res);
}

#ifdef CONFIG_WIFIMGR_STA
static int wifimgr_cli_cmd_rtt_req(const struct shell *shell, size_t argc,
				   char *argv[])
{
	struct wifi_rtt_request *rtt_req;
	struct wifi_rtt_peers *peer;
	int size;
	int choice;
	int ret;

	size = sizeof(struct wifi_rtt_request) + sizeof(struct wifi_rtt_peers);
	rtt_req = malloc(size);
	if (!rtt_req)
		return -ENOMEM;
	memset(rtt_req, 0, size);
	rtt_req->nr_peers = 1;
	rtt_req->peers =
	    (struct wifi_rtt_peers *)((char *)rtt_req +
				      rtt_req->nr_peers *
				      sizeof(struct wifi_rtt_request));
	peer = rtt_req->peers;

	optind = 0;
	while ((choice = getopt(argc, argv, "b:c:m:")) != -1) {
		switch (choice) {
		case 'b':
			peer->band = atoi(optarg);
			if (!peer->band) {
				shell_error(shell, "invalid band!");
				return -EINVAL;
			}
			break;
		case 'c':
			peer->channel = atoi(optarg);
			if (!peer->channel) {
				shell_error(shell, "invalid channel!");
				return -EINVAL;
			}
			break;
		case 'm':
			ret = strtomac(optarg, peer->bssid);
			if (ret) {
				shell_error(shell, "invalid BSSID!");
				return ret;
			}
			break;
		default:
			return -EINVAL;
		}
	}

	ret =
	    wifimgr_ctrl_iface_rtt_request(rtt_req, wifimgr_cli_show_rtt_resp);
	free(rtt_req);

	return ret;
}

static int wifimgr_cli_cmd_connect(const struct shell *shell, size_t argc,
				   char *argv[])
{
	return wifimgr_ctrl_iface_connect();
}

static int wifimgr_cli_cmd_disconnect(const struct shell *shell, size_t argc,
				      char *argv[])
{
	return wifimgr_ctrl_iface_disconnect();
}
#endif

#ifdef CONFIG_WIFIMGR_AP
static int wifimgr_cli_cmd_start_ap(const struct shell *shell, size_t argc,
				    char *argv[])
{
	return wifimgr_ctrl_iface_start_ap();
}

static int wifimgr_cli_cmd_stop_ap(const struct shell *shell, size_t argc,
				   char *argv[])
{
	return wifimgr_ctrl_iface_stop_ap();
}

static int wifimgr_cli_cmd_del_sta(const struct shell *shell, size_t argc,
				   char *argv[])
{
	char mac_addr[WIFI_MAC_ADDR_LEN];
	int ret;

	if (argc != 2 || !argv[1])
		return -EINVAL;

	ret = strtomac(argv[1], mac_addr);
	if (ret) {
		shell_error(shell, "invalid MAC address!");
		return ret;
	}

	if (is_broadcast_ether_addr(mac_addr))
		shell_print(shell, "Deauth all stations!");
	else
		shell_print(shell, "Deauth station (" MACSTR ")",
			    MAC2STR(mac_addr));

	ret = wifimgr_ctrl_iface_del_station(mac_addr);

	return ret;
}

static int wifimgr_cli_cmd_set_mac_acl(const struct shell *shell, size_t argc,
				       char *argv[])
{
	int choice;
	char subcmd = 0;
	char *mac = NULL;
	int ret;

	optind = 0;
	while ((choice = getopt(argc, argv, "ab:cu:")) != -1) {
		switch (choice) {
		case 'a':
		case 'b':
		case 'c':
		case 'u':
			switch (choice) {
			case 'a':
				subcmd = WIFI_MAC_ACL_BLOCK_ALL;
				break;
			case 'b':
				subcmd = WIFI_MAC_ACL_BLOCK;
				break;
			case 'c':
				subcmd = WIFI_MAC_ACL_UNBLOCK_ALL;
				break;
			case 'u':
				subcmd = WIFI_MAC_ACL_UNBLOCK;
				break;
			}
			if (!optarg) {
				mac = NULL;
			} else {
				char mac_addr[WIFI_MAC_ADDR_LEN];

				ret = strtomac(optarg, mac_addr);
				if (ret) {
					shell_error(shell,
						    "invalid MAC address!");
					return ret;
				}
				mac = mac_addr;
			}
			break;
		default:
			return -EINVAL;
		}
	}

	ret = wifimgr_ctrl_iface_set_mac_acl(subcmd, mac);

	return ret;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(wifimgr_commands,
	SHELL_CMD(get_config, NULL, WIFIMGR_CMD_COMMON_HELP,
	 wifimgr_cli_cmd_get_config),
	SHELL_CMD(set_config, NULL,
#ifdef CONFIG_WIFIMGR_STA
	 "<sta> -n <SSID> -m <BSSID> -c <channel>"
	 "\n<sta> -p <passphrase (\"\" for OPEN)>"
	 "\n<sta> -a <autorun interval (in milliseconds) (<0: disable)>"
#endif
#ifdef CONFIG_WIFIMGR_AP
	 "\n<ap> -n <SSID> -c <channel> -w <channel_width>"
	 "\n<ap> -p <passphrase (\"\" for OPEN)>"
	 "\n<ap> -a <autorun interval (in milliseconds) (<0: disable)>"
#endif
	 ,
	 wifimgr_cli_cmd_set_config),
	SHELL_CMD(clear_config, NULL, WIFIMGR_CMD_COMMON_HELP,
	 wifimgr_cli_cmd_clear_config),
	SHELL_CMD(capa, NULL, WIFIMGR_CMD_COMMON_HELP,
	 wifimgr_cli_cmd_capa),
	SHELL_CMD(status, NULL, WIFIMGR_CMD_COMMON_HELP,
	 wifimgr_cli_cmd_status),
	SHELL_CMD(open, NULL, WIFIMGR_CMD_COMMON_HELP,
	 wifimgr_cli_cmd_open),
	SHELL_CMD(close, NULL, WIFIMGR_CMD_COMMON_HELP,
	 wifimgr_cli_cmd_close),
	SHELL_CMD(scan, NULL,
	 WIFIMGR_CMD_COMMON_HELP" -b <band (optional)> -c <channel (optional)>",
	 wifimgr_cli_cmd_scan),
#ifdef CONFIG_WIFIMGR_STA
	SHELL_CMD(rtt_req, NULL, "-m <BSSID> -c <channel>",
	 wifimgr_cli_cmd_rtt_req),
	SHELL_CMD(connect, NULL, NULL,
	 wifimgr_cli_cmd_connect),
	SHELL_CMD(disconnect, NULL, NULL,
	 wifimgr_cli_cmd_disconnect),
#endif
#ifdef CONFIG_WIFIMGR_AP
	SHELL_CMD(start_ap, NULL, NULL,
	 wifimgr_cli_cmd_start_ap),
	SHELL_CMD(stop_ap, NULL, NULL,
	 wifimgr_cli_cmd_stop_ap),
	SHELL_CMD(del_sta, NULL, "<MAC address (to be deleted)>",
	 wifimgr_cli_cmd_del_sta),
	SHELL_CMD(mac_acl, NULL,
	 "-a (block all connected stations)"
	 "\n-b <MAC address (to be unblocked)>"
	 "\n-c (clear all blocked stations)"
	 "\n-u <MAC address (to be unblocked)>",
	 wifimgr_cli_cmd_set_mac_acl),
#endif
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(wifimgr, &wifimgr_commands, "WiFi Manager commands", NULL);
#endif
