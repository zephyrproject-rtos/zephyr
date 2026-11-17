/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_nm.h>
#include <zephyr/ztest.h>

#define SUPPLICANT_WAIT_MS 10000
#define SUPPLICANT_POLL_MS 100

static struct net_if *iface_sta;

static void report_neighbor(int index, int channel)
{
	char buf[128];

	snprintk(buf, sizeof(buf),
		 "RRM-NEIGHBOR-REP-RECEIVED bssid=02:00:5e:00:53:%02x info=0x0000008f "
		 "op_class=81 chan=%d phy_type=7",
		 index, channel);

	wifi_mgmt_raise_neighbor_rep_recv_event(iface_sta, buf, strlen(buf));
}

static void *suite_setup(void)
{
	int waited = 0;

	iface_sta = net_if_get_first_wifi();
	zassert_not_null(iface_sta, "No Wi-Fi interface");

	while (wifi_nm_get_instance_iface(iface_sta) == NULL) {
		zassert_true(waited < SUPPLICANT_WAIT_MS, "Supplicant did not take the interface");
		k_msleep(SUPPLICANT_POLL_MS);
		waited += SUPPLICANT_POLL_MS;
	}

	if (!net_if_is_admin_up(iface_sta)) {
		zassert_ok(net_if_up(iface_sta), "Cannot bring the interface up");
	}

	return NULL;
}

/*
 * The neighbor report list keeps every distinct channel the AP reports, up to
 * six, while the candidate scan request carries at most
 * CONFIG_WIFI_MGMT_SCAN_CHAN_MAX_MANUAL channels (three by default). Run the
 * candidate scan once with exactly as many neighbors as the request can hold,
 * and once with more. The suite runs with ASAN, which reports any access
 * outside the channel array of the scan request.
 */
ZTEST(wifi_roaming, test_neighbor_rep_complete_channel_limit)
{
	static const int channels[] = {1, 6, 11, 2, 3, 4};
	const int scan_channels = CONFIG_WIFI_MGMT_SCAN_CHAN_MAX_MANUAL;
	int i;

	zassert_true(scan_channels < ARRAY_SIZE(channels),
		     "Test needs more neighbors than scan channels");

	for (i = 0; i < scan_channels; i++) {
		report_neighbor(i, channels[i]);
	}

	zexpect_ok(net_mgmt(NET_REQUEST_WIFI_NEIGHBOR_REP_COMPLETE, iface_sta, NULL, 0),
		   "Candidate scan with %d neighbors failed", scan_channels);

	/* Let the supplicant start the scan before the next request. */
	k_msleep(500);

	for (; i < ARRAY_SIZE(channels); i++) {
		report_neighbor(i, channels[i]);
	}

	zexpect_ok(net_mgmt(NET_REQUEST_WIFI_NEIGHBOR_REP_COMPLETE, iface_sta, NULL, 0),
		   "Candidate scan with %d neighbors failed", (int)ARRAY_SIZE(channels));
}

ZTEST_SUITE(wifi_roaming, NULL, suite_setup, NULL, NULL, NULL);
