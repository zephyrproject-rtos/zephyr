/*
 * SPDX-FileCopyrightText: 2026 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/ztest.h>

#include <zephyr/drivers/wifi/wifi_emul.h>

#define TEST_ETH_TYPE      0x9000
#define TEST_EVENT_TIMEOUT K_MSEC(500)

static const struct device *wifi_dev = DEVICE_DT_GET(DT_NODELABEL(wlan0));
static struct net_if *wifi_iface;
static struct net_if *eth_fake_iface;

static const struct wifi_emul_ap ap_open = {
	.ssid = "OpenAP",
	.ssid_length = 6,
	.bssid = {0x00, 0x00, 0x5e, 0x00, 0x53, 0xa0},
	.channel = 6,
	.band = WIFI_FREQ_BAND_2_4_GHZ,
	.security = WIFI_SECURITY_TYPE_NONE,
	.rssi = -42,
};

static const struct wifi_emul_ap ap_psk = {
	.ssid = "SecureAP",
	.ssid_length = 8,
	.bssid = {0x00, 0x00, 0x5e, 0x00, 0x53, 0xa1},
	.channel = 36,
	.band = WIFI_FREQ_BAND_5_GHZ,
	.security = WIFI_SECURITY_TYPE_PSK,
	.psk = "password123",
	.rssi = -55,
};

static const struct wifi_emul_ap ap_hidden = {
	.ssid = "HiddenAP",
	.ssid_length = 8,
	.bssid = {0x00, 0x00, 0x5e, 0x00, 0x53, 0xa2},
	.channel = 11,
	.band = WIFI_FREQ_BAND_2_4_GHZ,
	.security = WIFI_SECURITY_TYPE_NONE,
	.rssi = -70,
	.hidden = true,
};

/* Fake Ethernet device providing the data plane */

struct eth_fake_context {
	struct net_if *iface;
	struct net_pkt *sent_pkt;
	uint8_t mac_address[6];
};

static struct eth_fake_context eth_fake_data;

static void eth_fake_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_fake_context *ctx = dev->data;

	ctx->iface = iface;
	eth_fake_iface = iface;

	ctx->mac_address[0] = 0xc2;
	ctx->mac_address[1] = 0xaa;
	ctx->mac_address[2] = 0xbb;
	ctx->mac_address[3] = 0xcc;
	ctx->mac_address[4] = 0xdd;
	ctx->mac_address[5] = 0xee;

	net_if_set_link_addr(iface, ctx->mac_address, sizeof(ctx->mac_address), NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static int eth_fake_send(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_fake_context *ctx = dev->data;

	if (ctx->sent_pkt != NULL) {
		net_pkt_unref(ctx->sent_pkt);
	}

	ctx->sent_pkt = net_pkt_shallow_clone(pkt, K_NO_WAIT);
	if (ctx->sent_pkt == NULL) {
		return -ENOMEM;
	}

	return 0;
}

static const struct ethernet_api eth_fake_api_funcs = {
	.iface_api.init = eth_fake_iface_init,
	.send = eth_fake_send,
};

ETH_NET_DEVICE_INIT(eth_fake, "eth_fake", NULL, NULL, &eth_fake_data, NULL,
		    CONFIG_ETH_INIT_PRIORITY, &eth_fake_api_funcs, NET_ETH_MTU);

static struct net_pkt *eth_fake_take_sent_pkt(void)
{
	struct net_pkt *pkt = eth_fake_data.sent_pkt;

	eth_fake_data.sent_pkt = NULL;

	return pkt;
}

static int eth_fake_inject(const uint8_t *frame, size_t len)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_rx_alloc_with_buffer(eth_fake_iface, len, NET_AF_UNSPEC, 0, K_FOREVER);
	zassert_not_null(pkt, "Failed to allocate RX packet");

	zassert_ok(net_pkt_write(pkt, frame, len));

	ret = net_recv_data(eth_fake_iface, pkt);
	if (ret < 0) {
		net_pkt_unref(pkt);
	}

	return ret;
}

/* Self-contained data plane: capture transmitted frames via the TX callback */

static struct net_pkt *captured_tx_pkt;

static void tx_capture_cb(const struct device *dev, struct net_pkt *pkt, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if (captured_tx_pkt != NULL) {
		net_pkt_unref(captured_tx_pkt);
	}

	captured_tx_pkt = net_pkt_shallow_clone(pkt, K_NO_WAIT);
}

static struct net_pkt *take_captured_tx_pkt(void)
{
	struct net_pkt *pkt = captured_tx_pkt;

	captured_tx_pkt = NULL;

	return pkt;
}

/* Wi-Fi management events */

static K_SEM_DEFINE(scan_done_sem, 0, 1);
static K_SEM_DEFINE(connect_result_sem, 0, 1);
static K_SEM_DEFINE(disconnect_result_sem, 0, 1);
static int scan_result_count;
static struct wifi_scan_result last_scan_result;
static int last_connect_status;
static int last_disconnect_status;

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				    struct net_if *iface)
{
	zassert_equal(iface, wifi_iface, "Event raised on unexpected interface");

	if (mgmt_event == NET_EVENT_WIFI_SCAN_RESULT) {
		scan_result_count++;
		memcpy(&last_scan_result, cb->info, sizeof(last_scan_result));
	} else if (mgmt_event == NET_EVENT_WIFI_SCAN_DONE) {
		k_sem_give(&scan_done_sem);
	} else if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
		last_connect_status = ((const struct wifi_status *)cb->info)->status;
		k_sem_give(&connect_result_sem);
	} else if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
		last_disconnect_status = ((const struct wifi_status *)cb->info)->status;
		k_sem_give(&disconnect_result_sem);
	}
}

static struct net_mgmt_event_callback wifi_mgmt_cb;

/* Helpers */

static void do_scan(void)
{
	struct wifi_scan_params params = {0};

	scan_result_count = 0;
	zassert_ok(net_mgmt(NET_REQUEST_WIFI_SCAN, wifi_iface, &params, sizeof(params)));
	zassert_ok(k_sem_take(&scan_done_sem, TEST_EVENT_TIMEOUT), "Scan did not complete");
}

static void do_connect(const char *ssid, const char *psk, enum wifi_security_type security,
		       int expected_status)
{
	struct wifi_connect_req_params params = {
		.ssid = ssid,
		.ssid_length = strlen(ssid),
		.psk = psk,
		.psk_length = psk != NULL ? strlen(psk) : 0,
		.security = security,
		.channel = WIFI_CHANNEL_ANY,
		.band = WIFI_FREQ_BAND_UNKNOWN,
	};

	zassert_ok(net_mgmt(NET_REQUEST_WIFI_CONNECT, wifi_iface, &params, sizeof(params)));
	zassert_ok(k_sem_take(&connect_result_sem, TEST_EVENT_TIMEOUT), "No connect result event");
	zassert_equal(last_connect_status, expected_status,
		      "Unexpected connect status %d (expected %d)", last_connect_status,
		      expected_status);
}

static void do_disconnect(void)
{
	zassert_ok(net_mgmt(NET_REQUEST_WIFI_DISCONNECT, wifi_iface, NULL, 0));
	zassert_ok(k_sem_take(&disconnect_result_sem, TEST_EVENT_TIMEOUT),
		   "No disconnect result event");
}

static int packet_socket_open(void)
{
	struct net_sockaddr_ll addr = {
		.sll_family = NET_AF_PACKET,
		.sll_protocol = net_htons(TEST_ETH_TYPE),
		.sll_ifindex = net_if_get_by_iface(wifi_iface),
	};
	int sock;

	sock = zsock_socket(NET_AF_PACKET, NET_SOCK_DGRAM, net_htons(TEST_ETH_TYPE));
	zassert_true(sock >= 0, "Failed to create packet socket");
	zassert_ok(zsock_bind(sock, (struct net_sockaddr *)&addr, sizeof(addr)));

	return sock;
}

static int packet_socket_recv(int sock, uint8_t *buf, size_t len, k_timeout_t timeout)
{
	struct zsock_pollfd fds[] = {
		{.fd = sock, .events = ZSOCK_POLLIN},
	};
	int ret;

	ret = zsock_poll(fds, ARRAY_SIZE(fds), k_ticks_to_ms_floor32(timeout.ticks));
	if (ret <= 0) {
		return -EAGAIN;
	}

	return zsock_recv(sock, buf, len, 0);
}

/* Tests */

static void *wifi_emul_test_setup(void)
{
	wifi_iface = net_if_lookup_by_dev(wifi_dev);
	zassert_not_null(wifi_iface, "No Wi-Fi interface");

	net_mgmt_init_event_callback(&wifi_mgmt_cb, wifi_mgmt_event_handler,
				     NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_SCAN_DONE |
					     NET_EVENT_WIFI_CONNECT_RESULT |
					     NET_EVENT_WIFI_DISCONNECT_RESULT);
	net_mgmt_add_event_callback(&wifi_mgmt_cb);

	return NULL;
}

static void wifi_emul_test_before(void *fixture)
{
	ARG_UNUSED(fixture);

	wifi_emul_ap_clear(wifi_dev);
	wifi_emul_force_connect_status(wifi_dev, -1);

	/* Start each test self-contained with no transmit callback */
	wifi_emul_set_eth_iface(wifi_dev, NULL);
	wifi_emul_set_tx_cb(wifi_dev, NULL, NULL);

	zassert_ok(wifi_emul_ap_add(wifi_dev, &ap_open));
	zassert_ok(wifi_emul_ap_add(wifi_dev, &ap_psk));
	zassert_ok(wifi_emul_ap_add(wifi_dev, &ap_hidden));
}

static void wifi_emul_test_after(void *fixture)
{
	struct net_pkt *pkt;

	ARG_UNUSED(fixture);

	if (wifi_emul_is_connected(wifi_dev)) {
		(void)wifi_emul_trigger_disconnect(wifi_dev, WIFI_REASON_DISCONN_UNSPECIFIED);
		(void)k_sem_take(&disconnect_result_sem, TEST_EVENT_TIMEOUT);
	}

	k_sem_reset(&scan_done_sem);
	k_sem_reset(&connect_result_sem);
	k_sem_reset(&disconnect_result_sem);

	pkt = eth_fake_take_sent_pkt();
	if (pkt != NULL) {
		net_pkt_unref(pkt);
	}

	pkt = take_captured_tx_pkt();
	if (pkt != NULL) {
		net_pkt_unref(pkt);
	}
}

ZTEST(wifi_emul, test_iface_discovery)
{
	zassert_equal(net_if_get_first_wifi(), wifi_iface,
		      "net_if_get_first_wifi() did not return the emulated interface");
	zassert_true(net_if_is_dormant(wifi_iface), "Interface should be dormant when idle");
}

ZTEST(wifi_emul, test_mac_address)
{
	struct net_linkaddr *lladdr = net_if_get_link_addr(wifi_iface);

	/* The interface gets its own MAC, random by default */
	zassert_equal(lladdr->len, WIFI_MAC_ADDR_LEN);
	zassert_false((lladdr->addr[0] & 0x01), "MAC must be unicast");
	zassert_true((lladdr->addr[0] & 0x02), "Random MAC must be locally administered");
}

ZTEST(wifi_emul, test_scan)
{
	do_scan();

	/* The hidden AP is not reported */
	zassert_equal(scan_result_count, 2, "Expected 2 scan results, got %d", scan_result_count);
}

ZTEST(wifi_emul, test_scan_result_content)
{
	zassert_ok(wifi_emul_ap_remove(wifi_dev, ap_open.bssid));
	zassert_ok(wifi_emul_ap_remove(wifi_dev, ap_hidden.bssid));

	do_scan();

	zassert_equal(scan_result_count, 1);
	zassert_equal(last_scan_result.ssid_length, ap_psk.ssid_length);
	zassert_mem_equal(last_scan_result.ssid, ap_psk.ssid, ap_psk.ssid_length);
	zassert_mem_equal(last_scan_result.mac, ap_psk.bssid, WIFI_MAC_ADDR_LEN);
	zassert_equal(last_scan_result.channel, ap_psk.channel);
	zassert_equal(last_scan_result.band, ap_psk.band);
	zassert_equal(last_scan_result.security, ap_psk.security);
	zassert_equal(last_scan_result.rssi, ap_psk.rssi);
}

ZTEST(wifi_emul, test_connect_open)
{
	do_connect("OpenAP", NULL, WIFI_SECURITY_TYPE_NONE, WIFI_STATUS_CONN_SUCCESS);

	zassert_true(wifi_emul_is_connected(wifi_dev));
	zassert_false(net_if_is_dormant(wifi_iface));

	do_disconnect();

	zassert_false(wifi_emul_is_connected(wifi_dev));
	zassert_true(net_if_is_dormant(wifi_iface));
	zassert_equal(last_disconnect_status, WIFI_REASON_DISCONN_USER_REQUEST);
}

ZTEST(wifi_emul, test_connect_psk)
{
	struct wifi_iface_status status = {0};

	do_connect("SecureAP", "password123", WIFI_SECURITY_TYPE_PSK, WIFI_STATUS_CONN_SUCCESS);

	zassert_ok(net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, wifi_iface, &status, sizeof(status)));
	zassert_equal(status.state, WIFI_STATE_COMPLETED);
	zassert_equal(status.ssid_len, ap_psk.ssid_length);
	zassert_mem_equal(status.ssid, ap_psk.ssid, ap_psk.ssid_length);
	zassert_mem_equal(status.bssid, ap_psk.bssid, WIFI_MAC_ADDR_LEN);
	zassert_equal(status.band, ap_psk.band);
	zassert_equal(status.channel, ap_psk.channel);
	zassert_equal(status.security, ap_psk.security);
	zassert_equal(status.rssi, ap_psk.rssi);

	do_disconnect();
}

ZTEST(wifi_emul, test_connect_hidden)
{
	do_connect("HiddenAP", NULL, WIFI_SECURITY_TYPE_NONE, WIFI_STATUS_CONN_SUCCESS);

	do_disconnect();
}

ZTEST(wifi_emul, test_connect_wrong_password)
{
	do_connect("SecureAP", "hunter22", WIFI_SECURITY_TYPE_PSK, WIFI_STATUS_CONN_WRONG_PASSWORD);

	zassert_false(wifi_emul_is_connected(wifi_dev));
	zassert_true(net_if_is_dormant(wifi_iface));
}

ZTEST(wifi_emul, test_connect_wrong_security)
{
	do_connect("SecureAP", NULL, WIFI_SECURITY_TYPE_NONE, WIFI_STATUS_CONN_WRONG_PASSWORD);

	zassert_false(wifi_emul_is_connected(wifi_dev));
}

ZTEST(wifi_emul, test_connect_unknown_ssid)
{
	do_connect("DoesNotExist", NULL, WIFI_SECURITY_TYPE_NONE, WIFI_STATUS_CONN_AP_NOT_FOUND);

	zassert_false(wifi_emul_is_connected(wifi_dev));
}

ZTEST(wifi_emul, test_connect_forced_status)
{
	wifi_emul_force_connect_status(wifi_dev, WIFI_STATUS_CONN_TIMEOUT);

	do_connect("OpenAP", NULL, WIFI_SECURITY_TYPE_NONE, WIFI_STATUS_CONN_TIMEOUT);

	zassert_false(wifi_emul_is_connected(wifi_dev));

	wifi_emul_force_connect_status(wifi_dev, -1);

	do_connect("OpenAP", NULL, WIFI_SECURITY_TYPE_NONE, WIFI_STATUS_CONN_SUCCESS);

	do_disconnect();
}

ZTEST(wifi_emul, test_forced_success_is_ignored)
{
	struct wifi_emul_ap ap;

	/* Forcing a success status must not connect without a matching AP:
	 * it falls through to normal evaluation.
	 */
	wifi_emul_force_connect_status(wifi_dev, WIFI_STATUS_CONN_SUCCESS);

	do_connect("DoesNotExist", NULL, WIFI_SECURITY_TYPE_NONE, WIFI_STATUS_CONN_AP_NOT_FOUND);
	zassert_false(wifi_emul_is_connected(wifi_dev));

	/* A registered AP still connects normally and reports a valid AP */
	do_connect("OpenAP", NULL, WIFI_SECURITY_TYPE_NONE, WIFI_STATUS_CONN_SUCCESS);
	zassert_true(wifi_emul_is_connected(wifi_dev));
	zassert_ok(wifi_emul_get_current_ap(wifi_dev, &ap));
	zassert_mem_equal(ap.bssid, ap_open.bssid, WIFI_MAC_ADDR_LEN);

	do_disconnect();
}

ZTEST(wifi_emul, test_ap_initiated_disconnect)
{
	do_connect("OpenAP", NULL, WIFI_SECURITY_TYPE_NONE, WIFI_STATUS_CONN_SUCCESS);

	zassert_equal(wifi_emul_trigger_disconnect(wifi_dev, WIFI_REASON_DISCONN_AP_LEAVING), 0);
	zassert_ok(k_sem_take(&disconnect_result_sem, TEST_EVENT_TIMEOUT),
		   "No disconnect result event");
	zassert_equal(last_disconnect_status, WIFI_REASON_DISCONN_AP_LEAVING);
	zassert_false(wifi_emul_is_connected(wifi_dev));
	zassert_true(net_if_is_dormant(wifi_iface));

	zassert_equal(wifi_emul_trigger_disconnect(wifi_dev, WIFI_REASON_DISCONN_AP_LEAVING),
		      -ENOTCONN);
}

ZTEST(wifi_emul, test_bridge_data_path)
{
	static const uint8_t peer_mac[] = {0x00, 0x00, 0x5e, 0x00, 0x53, 0xff};
	static const uint8_t tx_payload[] = "wifi-emul tx data";
	static const uint8_t rx_payload[] = "wifi-emul rx data";
	struct net_sockaddr_ll peer = {
		.sll_family = NET_AF_PACKET,
		.sll_protocol = net_htons(TEST_ETH_TYPE),
		.sll_ifindex = net_if_get_by_iface(wifi_iface),
		.sll_halen = sizeof(peer_mac),
	};
	uint8_t frame[sizeof(struct net_eth_hdr) + sizeof(rx_payload)];
	struct net_eth_hdr *hdr = (struct net_eth_hdr *)frame;
	uint8_t buf[64];
	struct net_pkt *pkt;
	int sock;
	int ret;

	memcpy(peer.sll_addr, peer_mac, sizeof(peer_mac));

	/* Bridge mode: attach the fake Ethernet device as the data plane */
	zassert_ok(wifi_emul_set_eth_iface(wifi_dev, eth_fake_iface));

	do_connect("OpenAP", NULL, WIFI_SECURITY_TYPE_NONE, WIFI_STATUS_CONN_SUCCESS);

	sock = packet_socket_open();

	/* TX: a frame sent on the Wi-Fi interface reaches the Ethernet driver */
	ret = zsock_sendto(sock, tx_payload, sizeof(tx_payload), 0, (struct net_sockaddr *)&peer,
			   sizeof(peer));
	zassert_equal(ret, sizeof(tx_payload), "Failed to send (%d)", -errno);

	pkt = eth_fake_take_sent_pkt();
	zassert_not_null(pkt, "No frame forwarded to the Ethernet driver");
	zassert_equal(net_pkt_get_len(pkt), sizeof(struct net_eth_hdr) + sizeof(tx_payload));

	net_pkt_cursor_init(pkt);
	zassert_ok(net_pkt_read(pkt, buf, sizeof(struct net_eth_hdr)));
	hdr = (struct net_eth_hdr *)buf;
	zassert_mem_equal(hdr->dst.addr, peer_mac, sizeof(peer_mac));
	zassert_mem_equal(hdr->src.addr, net_if_get_link_addr(wifi_iface)->addr, WIFI_MAC_ADDR_LEN);
	zassert_equal(hdr->type, net_htons(TEST_ETH_TYPE));
	zassert_ok(net_pkt_read(pkt, buf, sizeof(tx_payload)));
	zassert_mem_equal(buf, tx_payload, sizeof(tx_payload));
	net_pkt_unref(pkt);

	/* RX: a frame received on the Ethernet driver is delivered on the
	 * Wi-Fi interface
	 */
	hdr = (struct net_eth_hdr *)frame;
	memcpy(hdr->dst.addr, net_if_get_link_addr(wifi_iface)->addr, WIFI_MAC_ADDR_LEN);
	memcpy(hdr->src.addr, peer_mac, sizeof(peer_mac));
	hdr->type = net_htons(TEST_ETH_TYPE);
	memcpy(&frame[sizeof(struct net_eth_hdr)], rx_payload, sizeof(rx_payload));

	zassert_ok(eth_fake_inject(frame, sizeof(frame)));

	ret = packet_socket_recv(sock, buf, sizeof(buf), TEST_EVENT_TIMEOUT);
	zassert_equal(ret, sizeof(rx_payload), "No frame received on the Wi-Fi interface (%d)",
		      ret);
	zassert_mem_equal(buf, rx_payload, sizeof(rx_payload));

	/* Disconnected: TX is rejected and RX is dropped */
	do_disconnect();

	ret = zsock_sendto(sock, tx_payload, sizeof(tx_payload), 0, (struct net_sockaddr *)&peer,
			   sizeof(peer));
	zassert_true(ret < 0, "TX should fail when disconnected");
	zassert_is_null(eth_fake_take_sent_pkt(),
			"No frame should reach the Ethernet driver when disconnected");

	(void)eth_fake_inject(frame, sizeof(frame));
	ret = packet_socket_recv(sock, buf, sizeof(buf), K_MSEC(100));
	zassert_true(ret < 0, "RX should be dropped when disconnected");

	zsock_close(sock);
}

ZTEST(wifi_emul, test_self_contained_tx)
{
	static const uint8_t peer_mac[] = {0x00, 0x00, 0x5e, 0x00, 0x53, 0xff};
	static const uint8_t tx_payload[] = "self-contained tx";
	struct net_sockaddr_ll peer = {
		.sll_family = NET_AF_PACKET,
		.sll_protocol = net_htons(TEST_ETH_TYPE),
		.sll_ifindex = net_if_get_by_iface(wifi_iface),
		.sll_halen = sizeof(peer_mac),
	};
	struct net_eth_hdr *hdr;
	uint8_t buf[64];
	struct net_pkt *pkt;
	int sock;
	int ret;

	memcpy(peer.sll_addr, peer_mac, sizeof(peer_mac));

	/* No data plane attached: transmitted frames reach the TX callback */
	wifi_emul_set_tx_cb(wifi_dev, tx_capture_cb, NULL);

	do_connect("OpenAP", NULL, WIFI_SECURITY_TYPE_NONE, WIFI_STATUS_CONN_SUCCESS);

	sock = packet_socket_open();

	ret = zsock_sendto(sock, tx_payload, sizeof(tx_payload), 0, (struct net_sockaddr *)&peer,
			   sizeof(peer));
	zassert_equal(ret, sizeof(tx_payload), "Failed to send (%d)", -errno);

	pkt = take_captured_tx_pkt();
	zassert_not_null(pkt, "No frame captured by the TX callback");
	zassert_equal(net_pkt_get_len(pkt), sizeof(struct net_eth_hdr) + sizeof(tx_payload));

	net_pkt_cursor_init(pkt);
	zassert_ok(net_pkt_read(pkt, buf, sizeof(struct net_eth_hdr)));
	hdr = (struct net_eth_hdr *)buf;
	zassert_mem_equal(hdr->dst.addr, peer_mac, sizeof(peer_mac));
	zassert_mem_equal(hdr->src.addr, net_if_get_link_addr(wifi_iface)->addr, WIFI_MAC_ADDR_LEN);
	zassert_equal(hdr->type, net_htons(TEST_ETH_TYPE));
	zassert_ok(net_pkt_read(pkt, buf, sizeof(tx_payload)));
	zassert_mem_equal(buf, tx_payload, sizeof(tx_payload));
	net_pkt_unref(pkt);

	/* Disconnected: TX is rejected and nothing is captured */
	do_disconnect();

	ret = zsock_sendto(sock, tx_payload, sizeof(tx_payload), 0, (struct net_sockaddr *)&peer,
			   sizeof(peer));
	zassert_true(ret < 0, "TX should fail when disconnected");
	zassert_is_null(take_captured_tx_pkt(), "No frame should be captured when disconnected");

	zsock_close(sock);
}

ZTEST(wifi_emul, test_self_contained_rx)
{
	static const uint8_t peer_mac[] = {0x00, 0x00, 0x5e, 0x00, 0x53, 0xff};
	static const uint8_t rx_payload[] = "self-contained rx";
	uint8_t frame[sizeof(struct net_eth_hdr) + sizeof(rx_payload)];
	struct net_eth_hdr *hdr = (struct net_eth_hdr *)frame;
	uint8_t buf[64];
	int sock;
	int ret;

	do_connect("OpenAP", NULL, WIFI_SECURITY_TYPE_NONE, WIFI_STATUS_CONN_SUCCESS);

	sock = packet_socket_open();

	memcpy(hdr->dst.addr, net_if_get_link_addr(wifi_iface)->addr, WIFI_MAC_ADDR_LEN);
	memcpy(hdr->src.addr, peer_mac, sizeof(peer_mac));
	hdr->type = net_htons(TEST_ETH_TYPE);
	memcpy(&frame[sizeof(struct net_eth_hdr)], rx_payload, sizeof(rx_payload));

	/* No data plane attached: frames are injected directly via the API */
	zassert_ok(wifi_emul_rx_inject(wifi_dev, frame, sizeof(frame)));

	ret = packet_socket_recv(sock, buf, sizeof(buf), TEST_EVENT_TIMEOUT);
	zassert_equal(ret, sizeof(rx_payload), "No frame received on the Wi-Fi interface (%d)",
		      ret);
	zassert_mem_equal(buf, rx_payload, sizeof(rx_payload));

	/* Disconnected: RX injection is rejected */
	do_disconnect();
	zassert_equal(wifi_emul_rx_inject(wifi_dev, frame, sizeof(frame)), -ENETDOWN);

	zsock_close(sock);
}

ZTEST_SUITE(wifi_emul, NULL, wifi_emul_test_setup, wifi_emul_test_before, wifi_emul_test_after,
	    NULL);
