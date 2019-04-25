/*
 * @file
 * @brief Wi-Fi repeater demo
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <net/wifi_api.h>
#include <posix/posix_types.h>
#include <stdio.h>

#define WIFIMGR_AUTORUN_STA_RETRY	(3)
struct k_delayed_work sta_autowork;
struct wifi_config sta_config;
struct wifi_status sta_status;

struct k_delayed_work ap_autowork;
struct wifi_config ap_config;
struct wifi_status ap_status;

static void wifi_autorun_scan_result(struct wifi_scan_result *scan_res)
{
	/* Find specified AP */
	if (!strcmp(scan_res->ssid, sta_config.ssid)) {
		printf("SSID:\t\t%s\n", sta_config.ssid);
		/* Choose the first match when BSSID is not specified */
		if (is_zero_ether_addr(sta_config.bssid))
			sta_status.u.sta.host_found = 1;
		else if (!strncmp
			 (scan_res->bssid, sta_config.bssid, WIFI_MAC_ADDR_LEN))
			sta_status.u.sta.host_found = 1;
	}
}

static void wifi_autorun_notify_connect(union wifi_notifier_val val)
{
	int status = val.val_char;

	if (status) {
		printf("failed to connect!\n");
		return;
	}

	printf("connect successfully!\n");
	sta_status.state = WIFI_STATE_STA_CONNECTED;
}

static void wifi_autorun_notify_disconnect(union wifi_notifier_val val)
{
	int reason_code = val.val_char;

	printf("disconnect! reason: %d\n", reason_code);
	sta_status.state = WIFI_STATE_STA_READY;
	k_delayed_work_submit(&sta_autowork, sta_config.autorun);
}

static void wifi_autorun_notify_new_station(union wifi_notifier_val val)
{
	char *mac = val.val_ptr;

	printf("Station (" MACSTR ") connected!\n", MAC2STR(mac));
}

static void wifi_autorun_notify_station_leave(union wifi_notifier_val val)
{
	char *mac = val.val_ptr;

	printf("Station (" MACSTR ") disconnected!\n", MAC2STR(mac));
}

static void wifi_autorun_sta(struct k_work *work)
{
	int cnt;
	int ret;

	/* Get STA config */
	ret = wifi_sta_get_conf(&sta_config);
	if (ret) {
		printf("failed to get_conf! %d\n", ret);
		goto exit;
	}
	if (!sta_config.autorun) {
		printf("STA autorun disabled!\n");
		return;
	}
	if (!sta_config.ssid || !strlen(sta_config.ssid)) {
		printf("no STA config!\n");
		goto exit;
	}

	/* Get STA status */
	ret = wifi_sta_get_status(&sta_status);
	if (ret) {
		printf("failed to get_status! %d\n", ret);
		goto exit;
	}

	switch (sta_status.state) {
	case WIFI_STATE_STA_UNAVAIL:
		/* Open STA */
		ret = wifi_sta_open();
		if (ret) {
			printf("failed to open! %d\n", ret);
			goto exit;
		}
	case WIFI_STATE_STA_READY:
		/* Trigger STA scan */
		for (cnt = 0; cnt < WIFIMGR_AUTORUN_STA_RETRY; cnt++) {
			ret = wifi_sta_scan(NULL, wifi_autorun_scan_result);
			if (ret)
				printf("failed to scan! %d\n", ret);
			if (sta_status.u.sta.host_found)
				break;
		}
		if (cnt == WIFIMGR_AUTORUN_STA_RETRY) {
			printf("maximum scan retry reached!\n");
			goto exit;
		}

		/* Connect to the AP */
		ret = wifi_sta_connect();
		if (ret) {
			printf("failed to connect! %d\n", ret);
			goto exit;
		}
	case WIFI_STATE_STA_CONNECTED:
		return;
	default:
		printf("nothing else to do under %s!\n",
			    sta_sts2str(sta_status.state));
		break;
	}
exit:
	/* Prepare next run */
	k_delayed_work_submit(&sta_autowork, sta_config.autorun);
}

static void wifi_autorun_ap(struct k_work *work)
{
	int ret;

	/* Get AP config */
	ret = wifi_ap_get_conf(&ap_config);
	if (ret) {
		printf("failed to get_conf! %d\n", ret);
		goto exit;
	}
	if (!ap_config.autorun) {
		printf("AP autorun disabled!\n");
		return;
	}
	if (!ap_config.ssid || !strlen(ap_config.ssid)) {
		printf("no AP config!\n");
		goto exit;
	}

	/* Get AP status */
	ret = wifi_ap_get_status(&ap_status);
	if (ret) {
		printf("failed to get_status! %d\n", ret);
		goto exit;
	}

	switch (ap_status.state) {
	case WIFI_STATE_AP_UNAVAIL:
		/* Open AP */
		ret = wifi_ap_open();
		if (ret) {
			printf("failed to open! %d\n", ret);
			goto exit;
		}
	case WIFI_STATE_AP_READY:
		/* Start AP */
		ret = wifi_ap_start_ap();
		if (!ret) {
			printf("done!\n");
			return;
		}

		printf("failed to start AP! %d\n", ret);
		goto exit;

		break;
	default:
		printf("nothing else to do under %s!\n",
			    ap_sts2str(ap_status.state));
		goto exit;
	}

exit:
	/* Prepare next run */
	k_delayed_work_submit(&ap_autowork, ap_config.autorun);
}

void wifi_repeater(void)
{
	printf("starting Wi-Fi Repeater...\n");

	wifi_register_connection_notifier(wifi_autorun_notify_connect);
	wifi_register_disconnection_notifier(wifi_autorun_notify_disconnect);
	k_delayed_work_init(&sta_autowork, wifi_autorun_sta);
	/* Trigger first run */
	k_delayed_work_submit(&sta_autowork, 0);

	wifi_register_new_station_notifier
	    (wifi_autorun_notify_new_station);
	wifi_register_station_leave_notifier
	    (wifi_autorun_notify_station_leave);
	k_delayed_work_init(&ap_autowork, wifi_autorun_ap);
	/* Trigger first run */
	k_delayed_work_submit(&ap_autowork, 0);
}
