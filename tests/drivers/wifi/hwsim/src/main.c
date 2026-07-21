/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/ztest.h>

#include "wifi_hwsim.h"
/* Internal header (subsys/net/ip) for net_route_ipv4_add(); the include path is
 * added in CMakeLists.txt, as the net route tests do.
 */
#include "route_ipv4.h"

static struct net_if *iface_ap;
static struct net_if *iface_sta;

static struct wifi_connect_req_params default_ap_params = {
	.ssid = (uint8_t *)"ZephyrHwsim",
	.ssid_length = 10,
	.channel = 6,
	.security = WIFI_SECURITY_TYPE_NONE,
	.band = WIFI_FREQ_BAND_2_4_GHZ,
	.mfp = WIFI_MFP_DISABLE,
};

/*
 * AP-enable and STA-connect share these for the secured association tests.
 * WPA2-PSK and WPA3-SAE use distinct SSIDs so the supplicant cannot "fast
 * associate" a secured connect against a stale scan entry left by the other
 * test on the same (reused) AP BSSID.
 */
static uint8_t psk_ssid[] = "ZephyrPsk";
static uint8_t sae_ssid[] = "ZephyrSae";
static uint8_t secured_psk[] = "ZephyrPass123";

static struct wifi_connect_req_params wpa2_psk_params = {
	.ssid = psk_ssid,
	.ssid_length = sizeof(psk_ssid) - 1,
	.psk = secured_psk,
	.psk_length = sizeof(secured_psk) - 1,
	.channel = 6,
	.security = WIFI_SECURITY_TYPE_PSK,
	.band = WIFI_FREQ_BAND_2_4_GHZ,
	.mfp = WIFI_MFP_OPTIONAL,
};

/* WPA3-SAE uses a passphrase and mandates management-frame protection. */
static struct wifi_connect_req_params wpa3_sae_params = {
	.ssid = sae_ssid,
	.ssid_length = sizeof(sae_ssid) - 1,
	.sae_password = secured_psk,
	.sae_password_length = sizeof(secured_psk) - 1,
	.psk = secured_psk,
	.psk_length = sizeof(secured_psk) - 1,
	.channel = 6,
	.security = WIFI_SECURITY_TYPE_SAE,
	.band = WIFI_FREQ_BAND_2_4_GHZ,
	.mfp = WIFI_MFP_REQUIRED,
};

struct hwsim_waiter {
	struct net_mgmt_event_callback cb;
	struct k_sem sem;
	struct net_if *iface;
	uint64_t event_mask;
	uint64_t last_event;
	int status;
};

static void waiter_handler(struct net_mgmt_event_callback *cb, uint64_t event,
			   struct net_if *iface)
{
	struct hwsim_waiter *w = CONTAINER_OF(cb, struct hwsim_waiter, cb);

	/* Only react to events on the interface this waiter is bound to,
	 * otherwise an event from another radio could release the semaphore.
	 */
	if (w->iface != NULL && iface != w->iface) {
		return;
	}
	if ((event & w->event_mask) != 0) {
		w->last_event = event;
#ifdef CONFIG_NET_MGMT_EVENT_INFO
		if (cb->info != NULL && cb->info_length >= sizeof(int)) {
			w->status = *(int *)cb->info;
		}
#endif
		k_sem_give(&w->sem);
	}
}

static void hwsim_waiter_init(struct hwsim_waiter *w, struct net_if *iface,
			     uint64_t event_mask)
{
	k_sem_init(&w->sem, 0, 1);
	w->iface = iface;
	w->event_mask = event_mask;
	w->last_event = 0;
	w->status = -1;
	net_mgmt_init_event_callback(&w->cb, waiter_handler, event_mask);
	net_mgmt_add_event_callback(&w->cb);
}

static int hwsim_waiter_wait(struct hwsim_waiter *w, k_timeout_t timeout)
{
	return k_sem_take(&w->sem, timeout);
}

static void hwsim_waiter_cleanup(struct hwsim_waiter *w)
{
	net_mgmt_del_event_callback(&w->cb);
}

static void *suite_setup(void)
{
	iface_ap = net_if_get_by_index(1);
	iface_sta = net_if_get_by_index(2);
	zassert_not_null(iface_ap, "AP iface not found");
	zassert_not_null(iface_sta, "STA iface not found");

	default_ap_params.ssid_length = 10;
	return NULL;
}

static void suite_teardown(void *data)
{
	ARG_UNUSED(data);
	/* Tear down any active association so no supplicant/hostapd timers keep
	 * the (native_sim) process alive after the suite completes.
	 */
	net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface_sta, NULL, 0);
	k_sleep(K_MSEC(100));
	net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, iface_ap, NULL, 0);
	k_sleep(K_MSEC(100));
}

static void test_before(void *data)
{
	ARG_UNUSED(data);
	net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface_sta, NULL, 0);
	k_sleep(K_MSEC(50));
	hwsim_set_loss(1, 0);
	hwsim_set_channel(1, 2437U);
	hwsim_set_signal(1, CONFIG_WIFI_HWSIM_DEFAULT_SIGNAL_DBM);
}

ZTEST_SUITE(hwsim, NULL, suite_setup, test_before, NULL, suite_teardown);

ZTEST(hwsim, test_01_ap_enable)
{
	struct hwsim_waiter w;

	hwsim_waiter_init(&w, iface_ap, NET_EVENT_WIFI_AP_ENABLE_RESULT);

	zassert_equal(net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface_ap,
			       &default_ap_params, sizeof(default_ap_params)),
		      0, "ap_enable request failed");

	zassert_equal(hwsim_waiter_wait(&w, K_SECONDS(1)), 0,
		      "AP enable event not received");
	hwsim_waiter_cleanup(&w);
}

static bool scan_found_ssid;

static void scan_result_cb(struct net_mgmt_event_callback *cb, uint64_t event,
			   struct net_if *iface)
{
	struct wifi_scan_result *res = (struct wifi_scan_result *)cb->info;

	ARG_UNUSED(iface);
	if (event == NET_EVENT_WIFI_SCAN_RESULT && res != NULL &&
	    res->ssid_length == 10 &&
	    memcmp(res->ssid, "ZephyrHwsim", 10) == 0) {
		scan_found_ssid = true;
	}
}

ZTEST(hwsim, test_02_sta_scan)
{
	struct hwsim_waiter w;
	struct net_mgmt_event_callback scan_cb;

	scan_found_ssid = false;
	hwsim_waiter_init(&w, iface_sta, NET_EVENT_WIFI_SCAN_DONE);
	net_mgmt_init_event_callback(&scan_cb, scan_result_cb,
				     NET_EVENT_WIFI_SCAN_RESULT);
	net_mgmt_add_event_callback(&scan_cb);

	zassert_equal(net_mgmt(NET_REQUEST_WIFI_SCAN, iface_sta, NULL, 0), 0,
		      "scan failed");
	zassert_equal(hwsim_waiter_wait(&w, K_SECONDS(2)), 0,
		      "scan done not received");

	net_mgmt_del_event_callback(&scan_cb);
	hwsim_waiter_cleanup(&w);
	zassert_true(scan_found_ssid, "AP SSID not found in scan results");
}

ZTEST(hwsim, test_03_connect)
{
	struct hwsim_waiter w;

	hwsim_waiter_init(&w, iface_sta, NET_EVENT_WIFI_CONNECT_RESULT);

	zassert_equal(net_mgmt(NET_REQUEST_WIFI_CONNECT, iface_sta,
			       &default_ap_params, sizeof(default_ap_params)),
		      0, "connect request failed");

	zassert_equal(hwsim_waiter_wait(&w,
		      K_SECONDS(CONFIG_WIFI_HWSIM_CONNECT_TIMEOUT_SEC + 2)), 0,
		      "connect result not received");
	zassert_equal(w.status, 0, "connect failed with status %d", w.status);
	hwsim_waiter_cleanup(&w);
}

#if defined(CONFIG_NET_IPV4)
#include <zephyr/net/icmp.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>

/* ICMPv4 echo reply payload (after ICMP header); avoid private icmpv4.h */
struct icmpv4_echo_payload {
	uint16_t identifier;
	uint16_t sequence;
} __packed;

static struct net_in_addr ap_addr = { { { 192, 168, 1, 1 } } };
static struct net_in_addr sta_addr = { { { 192, 168, 2, 1 } } };
static struct net_if_addr *ap_ifaddr;
static struct net_if_addr *sta_ifaddr;

static struct k_sem ping_reply_sem;
static uint16_t ping_id = 0x1234U;
static uint16_t ping_seq = 1U;

static enum net_verdict ping_echo_reply_handler(struct net_icmp_ctx *ctx,
					       struct net_pkt *pkt,
					       struct net_icmp_ip_hdr *hdr,
					       struct net_icmp_hdr *icmp_hdr,
					       void *user_data)
{
	struct icmpv4_echo_payload payload;

	ARG_UNUSED(ctx);
	ARG_UNUSED(hdr);
	ARG_UNUSED(user_data);

	if (icmp_hdr->code != 0 || pkt == NULL) {
		return NET_CONTINUE;
	}

	/* The ICMP header has already been consumed by the core ICMP input,
	 * so the cursor is positioned at the echo identifier/sequence.
	 */
	if (net_pkt_read(pkt, (void *)&payload, sizeof(payload)) < 0) {
		return NET_CONTINUE;
	}

	if (net_ntohs(payload.identifier) == ping_id &&
	    net_ntohs(payload.sequence) == ping_seq) {
		k_sem_give(&ping_reply_sem);
		return NET_OK;
	}

	return NET_CONTINUE;
}

/*
 * With the STA associated to the AP, assign static IPs on the two radios,
 * add on-link routes so traffic traverses the simulated medium (rather than
 * short-circuiting on local delivery), and verify a STA->AP ICMP echo.
 */
static void sta_ping_ap(void)
{
	struct net_icmp_ctx icmp_ctx;
	struct net_sockaddr_in dst = { 0 };
	struct net_icmp_ping_params params = { 0 };
	int ret;

	/* Assign static IPs: AP 192.168.1.1, STA 192.168.2.1 */
	ap_ifaddr = net_if_ipv4_addr_add(iface_ap, &ap_addr, NET_ADDR_MANUAL, 0);
	zassert_not_null(ap_ifaddr, "AP IPv4 add failed");
	sta_ifaddr = net_if_ipv4_addr_add(iface_sta, &sta_addr, NET_ADDR_MANUAL, 0);
	zassert_not_null(sta_ifaddr, "STA IPv4 add failed");

	net_if_up(iface_ap);
	net_if_up(iface_sta);

	/* STA (192.168.2.0/24) needs an on-link route to reach the AP
	 * (192.168.1.0/24); AP likewise in reverse. On-link routes
	 * (nexthop=NULL) force the stack to ARP directly and send via
	 * the medium instead of short-circuiting on local delivery.
	 */
	{
		struct net_in_addr ap_net = { { { 192, 168, 1, 0 } } };
		struct net_in_addr sta_net = { { { 192, 168, 2, 0 } } };

		zassert_not_null(net_route_ipv4_add(iface_sta, &ap_net, 24,
						     NULL, 3600, 0),
				 "route STA->AP add failed");
		zassert_not_null(net_route_ipv4_add(iface_ap, &sta_net, 24,
						     NULL, 3600, 0),
				 "route AP->STA add failed");
	}

	/* Allow ARP and link to settle before sending ping */
	k_sleep(K_MSEC(500));

	k_sem_init(&ping_reply_sem, 0, 1);
	ret = net_icmp_init_ctx(&icmp_ctx, NET_AF_INET, NET_ICMPV4_ECHO_REPLY,
				0, ping_echo_reply_handler);
	zassert_equal(ret, 0, "ICMP init failed %d", ret);

	dst.sin_family = NET_AF_INET;
	net_ipaddr_copy(&dst.sin_addr, &ap_addr);
	params.identifier = ping_id;
	params.sequence = ping_seq;

	ret = net_icmp_send_echo_request(&icmp_ctx, iface_sta,
					 (struct net_sockaddr *)&dst,
					 &params, NULL);
	zassert_equal(ret, 0, "ICMP echo request failed %d", ret);

	ret = k_sem_take(&ping_reply_sem, K_SECONDS(5));
	zassert_equal(ret, 0, "Ping reply not received (timeout)");

	ret = net_icmp_cleanup_ctx(&icmp_ctx);
	zassert_equal(ret, 0, "ICMP cleanup failed %d", ret);

	net_if_ipv4_addr_rm(iface_ap, &ap_addr);
	net_if_ipv4_addr_rm(iface_sta, &sta_addr);
}

ZTEST(hwsim, test_04_connect_and_ping)
{
	struct hwsim_waiter w;

	/* Ensure AP is up and STA is disconnected from test_before. */
	hwsim_waiter_init(&w, iface_sta, NET_EVENT_WIFI_CONNECT_RESULT);

	zassert_equal(net_mgmt(NET_REQUEST_WIFI_CONNECT, iface_sta,
			       &default_ap_params, sizeof(default_ap_params)),
		      0, "connect request failed");

	zassert_equal(hwsim_waiter_wait(&w,
		      K_SECONDS(CONFIG_WIFI_HWSIM_CONNECT_TIMEOUT_SEC + 2)), 0,
		      "connect result not received");
	zassert_equal(w.status, 0, "connect failed with status %d", w.status);
	hwsim_waiter_cleanup(&w);

	sta_ping_ap();
}

ZTEST(hwsim, test_05_ap_start_connect_ping_full)
{
	struct hwsim_waiter w;

	/* 0. Ensure clean state: STA disconnected, AP disabled */
	hwsim_waiter_init(&w, iface_sta, NET_EVENT_WIFI_DISCONNECT_RESULT);
	net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface_sta, NULL, 0);
	hwsim_waiter_wait(&w, K_SECONDS(2));
	hwsim_waiter_cleanup(&w);
	k_sleep(K_MSEC(100));

	hwsim_waiter_init(&w, iface_ap, NET_EVENT_WIFI_AP_DISABLE_RESULT);
	net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, iface_ap, NULL, 0);
	hwsim_waiter_wait(&w, K_SECONDS(1));
	hwsim_waiter_cleanup(&w);
	k_sleep(K_MSEC(100));

	/* 1. AP enable */
	hwsim_waiter_init(&w, iface_ap, NET_EVENT_WIFI_AP_ENABLE_RESULT);
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface_ap,
			       &default_ap_params, sizeof(default_ap_params)),
		      0, "ap_enable failed");
	zassert_equal(hwsim_waiter_wait(&w, K_SECONDS(1)), 0,
		      "AP enable event not received");
	hwsim_waiter_cleanup(&w);

	/* 2. STA connect */
	hwsim_waiter_init(&w, iface_sta, NET_EVENT_WIFI_CONNECT_RESULT);
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_CONNECT, iface_sta,
			       &default_ap_params, sizeof(default_ap_params)),
		      0, "connect request failed");
	zassert_equal(hwsim_waiter_wait(&w,
		      K_SECONDS(CONFIG_WIFI_HWSIM_CONNECT_TIMEOUT_SEC + 2)), 0,
		      "connect result not received");
	zassert_equal(w.status, 0, "connect status %d", w.status);
	hwsim_waiter_cleanup(&w);

	/* 3. Assign IPs and ping */
	sta_ping_ap();
}

/*
 * Bring AP and STA to a clean state, start the AP with the given secured
 * profile, connect the STA with the same profile, and assert the connect
 * succeeds. For a secured network a successful connect result means the real
 * 4-way handshake (EAPOL over the medium) completed.
 */
static void secured_connect(struct wifi_connect_req_params *params)
{
	struct hwsim_waiter w;

	/* Clean state: STA disconnected, AP disabled. */
	hwsim_waiter_init(&w, iface_sta, NET_EVENT_WIFI_DISCONNECT_RESULT);
	net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface_sta, NULL, 0);
	hwsim_waiter_wait(&w, K_SECONDS(2));
	hwsim_waiter_cleanup(&w);

	hwsim_waiter_init(&w, iface_ap, NET_EVENT_WIFI_AP_DISABLE_RESULT);
	net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, iface_ap, NULL, 0);
	hwsim_waiter_wait(&w, K_SECONDS(1));
	hwsim_waiter_cleanup(&w);
	k_sleep(K_MSEC(100));

	/* Start the secured AP. */
	hwsim_waiter_init(&w, iface_ap, NET_EVENT_WIFI_AP_ENABLE_RESULT);
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface_ap,
			       params, sizeof(*params)), 0,
		      "secured ap_enable request failed");
	zassert_equal(hwsim_waiter_wait(&w, K_SECONDS(2)), 0,
		      "AP enable event not received");
	hwsim_waiter_cleanup(&w);

	/*
	 * Bring both interfaces administratively up before connecting: the
	 * 4-way handshake exchanges real EAPOL data frames over the medium,
	 * which requires the TX path (unlike an open connect).
	 */
	net_if_up(iface_ap);
	net_if_up(iface_sta);

	/* Connect the STA; success implies the 4-way handshake completed. */
	hwsim_waiter_init(&w, iface_sta, NET_EVENT_WIFI_CONNECT_RESULT);
	zassert_equal(net_mgmt(NET_REQUEST_WIFI_CONNECT, iface_sta,
			       params, sizeof(*params)), 0,
		      "secured connect request failed");
	zassert_equal(hwsim_waiter_wait(&w,
		      K_SECONDS(CONFIG_WIFI_HWSIM_CONNECT_TIMEOUT_SEC + 5)), 0,
		      "secured connect result (handshake) not received");
	zassert_equal(w.status, 0, "secured connect failed with status %d",
		      w.status);
	hwsim_waiter_cleanup(&w);
}

ZTEST(hwsim, test_06_connect_wpa2_psk)
{
	secured_connect(&wpa2_psk_params);
	sta_ping_ap();
}

ZTEST(hwsim, test_07_connect_wpa3_sae)
{
	secured_connect(&wpa3_sae_params);
	sta_ping_ap();
}
#endif /* CONFIG_NET_IPV4 */
