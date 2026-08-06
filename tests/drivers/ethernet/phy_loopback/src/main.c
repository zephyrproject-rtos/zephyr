/*
 * Copyright (c) 2026, Ylhyra ehf.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/mii.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define TEST_ETHERTYPE 0x88B5U
#define TEST_MAGIC     0x5048594CU

#define HEADER_SIZE     sizeof(struct net_eth_hdr)
#define STAMP_SIZE      (2U * sizeof(uint32_t))
#define MIN_FRAME_SIZE  60U
#define MAX_FRAME_SIZE  (NET_ETH_MTU + HEADER_SIZE)
#define TEST_FRAME_SIZE CLAMP(2U * CONFIG_NET_BUF_DATA_SIZE + 1U, MIN_FRAME_SIZE, MAX_FRAME_SIZE)

#define FRAME_TIMEOUT       K_SECONDS(1)
#define RECEIVE_SLICE_US    (50U * USEC_PER_MSEC)
#define PHY_UPDATE_TIMEOUT  K_SECONDS(2)
#define SIZE_SWEEP_PASSES   3U
#define ORDERED_FRAMES      64U
#define BURST_FRAMES        24U
#define BURST_QUIET_TIMEOUT K_MSEC(250)

BUILD_ASSERT(MIN_FRAME_SIZE >= HEADER_SIZE + STAMP_SIZE);

static struct net_if *iface;
static const struct device *phy;
static uint32_t original_bmcr;
static uint32_t sequence;
static bool bmcr_saved;
static bool initially_up;
static int sock = -1;

static uint8_t tx_frame[MAX_FRAME_SIZE];
static uint8_t rx_frame[MAX_FRAME_SIZE];
static size_t burst_lengths[BURST_FRAMES];
static bool burst_seen[BURST_FRAMES];

static void select_ethernet_interface(struct net_if *candidate, void *user_data)
{
	struct net_if **selected = user_data;

	if (*selected == NULL && net_if_l2(candidate) == &NET_L2_GET_NAME(ETHERNET) &&
	    net_eth_get_phy(candidate) != NULL) {
		*selected = candidate;
	}
}

static int wait_for_carrier(void)
{
	k_timepoint_t deadline = sys_timepoint_calc(PHY_UPDATE_TIMEOUT);

	do {
		if (net_if_is_carrier_ok(iface)) {
			return 0;
		}

		k_msleep(10);
	} while (!sys_timepoint_expired(deadline));

	return -ENETDOWN;
}

static int socket_open(void)
{
	struct net_sockaddr_ll address = {
		.sll_family = NET_AF_PACKET,
		.sll_ifindex = net_if_get_by_iface(iface),
	};
	struct zsock_timeval timeout = {
		.tv_usec = RECEIVE_SLICE_US,
	};
	int fd;

	fd = zsock_socket(NET_AF_PACKET, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	if (fd < 0) {
		return -errno;
	}

	if (zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &timeout, sizeof(timeout)) <
		    0 ||
	    zsock_bind(fd, (struct net_sockaddr *)&address, sizeof(address)) < 0) {
		int ret = -errno;

		zsock_close(fd);
		return ret;
	}

	return fd;
}

static void prepare_frame(uint8_t *frame, size_t length, uint32_t frame_sequence)
{
	const struct net_linkaddr *address = net_if_get_link_addr(iface);
	struct net_eth_hdr *header = (struct net_eth_hdr *)frame;

	memcpy(header->dst.addr, address->addr, NET_ETH_ADDR_LEN);
	memcpy(header->src.addr, address->addr, NET_ETH_ADDR_LEN);
	header->type = net_htons(TEST_ETHERTYPE);

	for (size_t i = HEADER_SIZE; i < length; i++) {
		frame[i] = (uint8_t)(i ^ length ^ frame_sequence);
	}

	sys_put_be32(TEST_MAGIC, frame + HEADER_SIZE);
	sys_put_be32(frame_sequence, frame + HEADER_SIZE + sizeof(uint32_t));
}

static bool frame_is_ours(const uint8_t *frame, size_t length, uint32_t *frame_sequence)
{
	const struct net_eth_hdr *header = (const struct net_eth_hdr *)frame;

	if (length < HEADER_SIZE + STAMP_SIZE || length > MAX_FRAME_SIZE ||
	    net_ntohs(header->type) != TEST_ETHERTYPE ||
	    sys_get_be32(frame + HEADER_SIZE) != TEST_MAGIC) {
		return false;
	}

	*frame_sequence = sys_get_be32(frame + HEADER_SIZE + sizeof(uint32_t));
	return true;
}

static int send_frame(const uint8_t *frame, size_t length)
{
	struct net_sockaddr_ll destination = {
		.sll_family = NET_AF_PACKET,
		.sll_ifindex = net_if_get_by_iface(iface),
	};
	int ret;

	ret = zsock_sendto(sock, frame, length, 0, (struct net_sockaddr *)&destination,
			   sizeof(destination));

	return ret < 0 ? -errno : ret;
}

static int receive_frame(k_timeout_t timeout, size_t *length, uint32_t *frame_sequence)
{
	k_timepoint_t deadline = sys_timepoint_calc(timeout);

	do {
		int ret = zsock_recv(sock, rx_frame, sizeof(rx_frame), 0);

		if (ret < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			}

			return -errno;
		}

		if (frame_is_ours(rx_frame, (size_t)ret, frame_sequence)) {
			*length = (size_t)ret;
			return 0;
		}
	} while (!sys_timepoint_expired(deadline));

	return -ETIMEDOUT;
}

static void assert_frame(size_t expected_length, uint32_t expected_sequence)
{
	prepare_frame(tx_frame, expected_length, expected_sequence);
	zassert_mem_equal(rx_frame, tx_frame, expected_length, "%zu-byte frame %u was corrupted",
			  expected_length, expected_sequence);
}

static void expect_frame(size_t length)
{
	uint32_t sent_sequence = ++sequence;
	uint32_t received_sequence;
	size_t received_length;
	int ret;

	prepare_frame(tx_frame, length, sent_sequence);
	ret = send_frame(tx_frame, length);
	zassert_equal(ret, length, "send of %zu bytes failed: %d", length, ret);

	ret = receive_frame(FRAME_TIMEOUT, &received_length, &received_sequence);
	zassert_ok(ret, "no reply for %zu-byte frame %u: %d", length, sent_sequence, ret);
	zassert_equal(received_sequence, sent_sequence, "expected sequence %u, got %u",
		      sent_sequence, received_sequence);
	zassert_equal(received_length, length, "sent %zu bytes, received %zu", length,
		      received_length);
	assert_frame(received_length, received_sequence);
}

static size_t burst_frame_length(size_t index)
{
	size_t boundary = ((index / 3U) % 2U + 1U) * CONFIG_NET_BUF_DATA_SIZE;
	size_t length = boundary + index % 3U - 1U;

	if (index % 11U == 0U) {
		length = MIN_FRAME_SIZE;
	}

	return CLAMP(length, MIN_FRAME_SIZE, MAX_FRAME_SIZE);
}

static void *ethernet_phy_loopback_setup(void)
{
	uint32_t loopback_bmcr;
	int ret;

	net_if_foreach(select_ethernet_interface, &iface);
	zassert_not_null(iface, "no Ethernet interface with a PHY was found");
	zassert_true(device_is_ready(net_if_get_device(iface)), "Ethernet device is not ready");

	phy = net_eth_get_phy(iface);
	zassert_true(device_is_ready(phy), "Ethernet PHY is not ready");

	zassert_ok(phy_read(phy, MII_BMCR, &original_bmcr), "failed to read PHY BMCR");
	bmcr_saved = true;
	ret = phy_configure_link(phy, LINK_FULL_100BASE, PHY_FLAG_AUTO_NEGOTIATION_DISABLED);
	zassert_true(ret == 0 || ret == -EALREADY, "failed to configure PHY link (%d)", ret);
	zassert_ok(phy_read(phy, MII_BMCR, &loopback_bmcr), "failed to read configured BMCR");
	loopback_bmcr |= MII_BMCR_LOOPBACK;
	zassert_ok(phy_write(phy, MII_BMCR, loopback_bmcr), "failed to enable PHY loopback");

	initially_up = net_if_is_admin_up(iface);
	if (!initially_up) {
		ret = net_if_up(iface);
		zassert_true(ret == 0 || ret == -EALREADY, "failed to start interface (%d)", ret);
	}

	zassert_ok(wait_for_carrier(), "interface did not report carrier in PHY loopback");

	sock = socket_open();
	zassert_true(sock >= 0, "failed to open packet socket (%d)", sock);

	return NULL;
}

static void ethernet_phy_loopback_before(void *fixture)
{
	ARG_UNUSED(fixture);

	while (zsock_recv(sock, rx_frame, sizeof(rx_frame), ZSOCK_MSG_DONTWAIT) > 0) {
	}
}

static void ethernet_phy_loopback_teardown(void *fixture)
{
	int ret;

	ARG_UNUSED(fixture);

	if (sock >= 0) {
		zsock_close(sock);
		sock = -1;
	}

	if (bmcr_saved) {
		uint32_t restore_bmcr = original_bmcr;

		if ((restore_bmcr & MII_BMCR_AUTONEG_ENABLE) != 0U) {
			restore_bmcr |= MII_BMCR_AUTONEG_RESTART;
		}

		ret = phy_write(phy, MII_BMCR, restore_bmcr);
		bmcr_saved = false;
		zassert_ok(ret, "failed to restore PHY BMCR");
	}

	if (!initially_up && net_if_is_admin_up(iface)) {
		ret = net_if_down(iface);
		zassert_true(ret == 0 || ret == -EALREADY, "failed to restore interface (%d)", ret);
	}
}

ZTEST(ethernet_phy_loopback, test_frame_boundaries)
{
	for (size_t pass = 0U; pass < SIZE_SWEEP_PASSES; pass++) {
		expect_frame(MIN_FRAME_SIZE);

		for (size_t boundary = CONFIG_NET_BUF_DATA_SIZE; boundary < MAX_FRAME_SIZE;
		     boundary += CONFIG_NET_BUF_DATA_SIZE) {
			for (int offset = -1; offset <= 1; offset++) {
				size_t length = boundary + offset;

				if (length > MIN_FRAME_SIZE && length < MAX_FRAME_SIZE) {
					expect_frame(length);
				}
			}
		}

		expect_frame(MAX_FRAME_SIZE);
	}
}

ZTEST(ethernet_phy_loopback, test_ordered_delivery)
{
	for (size_t i = 0U; i < ORDERED_FRAMES; i++) {
		expect_frame(TEST_FRAME_SIZE);
	}
}

ZTEST(ethernet_phy_loopback, test_restart_first_frame)
{
	zassert_ok(net_if_down(iface), "failed to stop interface");
	zassert_ok(net_if_up(iface), "failed to restart interface");
	zassert_ok(wait_for_carrier(), "interface did not restore carrier");
	expect_frame(TEST_FRAME_SIZE);
}

ZTEST(ethernet_phy_loopback, test_burst_and_recovery)
{
	uint32_t first_sequence = sequence + 1U;
	size_t sent = 0U;
	size_t received = 0U;

	memset(burst_seen, 0, sizeof(burst_seen));
	for (size_t i = 0U; i < BURST_FRAMES; i++) {
		uint32_t frame_sequence = ++sequence;
		size_t length = burst_frame_length(i);
		int ret;

		burst_lengths[i] = length;
		prepare_frame(tx_frame, length, frame_sequence);
		ret = send_frame(tx_frame, length);
		if (ret < 0 && IS_ENABLED(CONFIG_TEST_ETH_PHY_LOOPBACK_REQUIRE_PRESSURE)) {
			zassert_true(ret == -ENOMEM || ret == -ENOBUFS || ret == -EAGAIN,
				     "unexpected send failure: %d", ret);
			break;
		}

		zassert_equal(ret, length, "send of burst frame %zu failed: %d", i, ret);
		sent++;
	}

	zassert_true(sent > 0U, "the burst could not send a frame");
	while (true) {
		uint32_t frame_sequence;
		size_t length;
		size_t index;
		int ret = receive_frame(BURST_QUIET_TIMEOUT, &length, &frame_sequence);

		if (ret == -ETIMEDOUT) {
			break;
		}

		zassert_ok(ret, "burst receive failed: %d", ret);
		zassert_between_inclusive(frame_sequence, first_sequence,
					  first_sequence + sent - 1U,
					  "received a frame outside the burst");
		index = frame_sequence - first_sequence;
		zassert_equal(length, burst_lengths[index], "burst frame changed length");
		zassert_false(burst_seen[index], "frame %u was delivered twice", frame_sequence);
		assert_frame(length, frame_sequence);
		burst_seen[index] = true;
		received++;
	}

	if (IS_ENABLED(CONFIG_TEST_ETH_PHY_LOOPBACK_REQUIRE_PRESSURE)) {
		zassert_true(received < sent, "the constrained RX pools did not create pressure");
	} else {
		zassert_equal(received, sent, "received %zu of %zu accepted frames", received,
			      sent);
	}

	TC_PRINT("burst: sent %zu, received %zu\n", sent, received);
	expect_frame(TEST_FRAME_SIZE);
}

ZTEST_SUITE(ethernet_phy_loopback, NULL, ethernet_phy_loopback_setup, ethernet_phy_loopback_before,
	    NULL, ethernet_phy_loopback_teardown);
