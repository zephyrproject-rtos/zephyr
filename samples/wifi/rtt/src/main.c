/*
 * @file
 * @brief Wi-Fi RTT range demo.
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
#include <stdlib.h>

#define WIFIMGR_AUTORUN_STA_RETRY	(3)
struct k_delayed_work sta_autowork;
struct wifi_config sta_config;
union wifi_drv_capa sta_capa;
struct wifi_status sta_status;
struct wifi_rtt_request sta_rtt_req;
unsigned char max_nr_peers;

static void wifi_autorun_scan_result(struct wifi_scan_result *scan_res)
{
	if (sta_rtt_req.nr_peers >= max_nr_peers) {
		printf("\t%s\t\t" MACSTR " (dropped)\n", scan_res->ssid,
		       MAC2STR(scan_res->bssid));
		return;
	}

	/* Filter RTT routers */
	if (scan_res->rtt_supported && !is_zero_ether_addr(scan_res->bssid)) {
		struct wifi_rtt_peers *rtt_peers = sta_rtt_req.peers;
		int i;

		/* Drop the duplicate result */
		for (i = 0; i < sta_rtt_req.nr_peers; i++, rtt_peers++) {
			if (!memcmp
			    (scan_res->bssid, rtt_peers->bssid,
			     WIFI_MAC_ADDR_LEN)
			    && (scan_res->band == rtt_peers->band)
			    && (scan_res->channel == rtt_peers->channel))
				return;
		}

		/* Fill RTT peers */
		printf("\t%s\t\t" MACSTR "\t%d\n", scan_res->ssid,
		       MAC2STR(scan_res->bssid), scan_res->channel);
		memcpy(rtt_peers->bssid, scan_res->bssid, WIFI_MAC_ADDR_LEN);
		rtt_peers->band = scan_res->band;
		rtt_peers->channel = scan_res->channel;
		sta_rtt_req.nr_peers++;
	}
}

static void wifi_autorun_rtt_resp(struct wifi_rtt_response *rtt_resp)
{
	if (rtt_resp->bssid && !is_zero_ether_addr(rtt_resp->bssid))
		printf("\t" MACSTR, MAC2STR(rtt_resp->bssid));
	else
		printf("\t\t\t");

	printf("\t%d\n", rtt_resp->range);
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

static void wifi_autorun_sta(struct k_work *work)
{
	struct wifi_rtt_peers *rtt_peers;
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

	/* Get STA capability */
	ret = wifi_sta_get_capa(&sta_capa);
	if (ret) {
		printf("failed to get_conf! %d\n", ret);
		goto exit;
	}
	if (sta_capa.sta.max_rtt_peers) {
		max_nr_peers = sta_capa.sta.max_rtt_peers;
	} else {
		max_nr_peers = 5;
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
		/* Prepare RTT request */
		rtt_peers =
		    malloc(sizeof(struct wifi_rtt_peers) * max_nr_peers);
		memset(rtt_peers, 0,
		       sizeof(struct wifi_rtt_peers) * max_nr_peers);
		sta_rtt_req.peers = rtt_peers;
		sta_rtt_req.nr_peers = 0;

		/* Trigger STA scan */
		ret = wifi_sta_scan(NULL, wifi_autorun_scan_result);
		if (ret)
			printf("failed to scan! %d\n", ret);

		if (!sta_rtt_req.nr_peers) {
			printf("No RTT peers!\n");
			goto exit;
		}

		/* Request RTT range */
		ret = wifi_sta_rtt_request(&sta_rtt_req, wifi_autorun_rtt_resp);
		free(sta_rtt_req.peers);
		if (ret) {
			printf("failed to request RTT! %d\n", ret);
			goto exit;
		}

		break;
	default:
		printf("nothing else to do under %s!\n",
		       sta_sts2str(sta_status.state));
		break;
	}
exit:
	/* Prepare next run */
	k_delayed_work_submit(&sta_autowork, sta_config.autorun);
}

void wifi_auto_rtt(void)
{
	printf("start RTT range...\n");

	wifi_register_connection_notifier(wifi_autorun_notify_connect);
	wifi_register_disconnection_notifier(wifi_autorun_notify_disconnect);
	k_delayed_work_init(&sta_autowork, wifi_autorun_sta);
	/* Trigger first run */
	k_delayed_work_submit(&sta_autowork, 0);
}
