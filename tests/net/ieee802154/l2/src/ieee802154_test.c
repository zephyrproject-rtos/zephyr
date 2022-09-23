/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ieee802154_test, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>


#include <zephyr/crypto/crypto.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/net/ieee802154_mgmt.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/socket.h>

#include "net_private.h"
#include <ieee802154_frame.h>
#include <ipv6.h>

struct ieee802154_pkt_test {
	char *name;
	struct in6_addr src;
	struct in6_addr dst;
	uint8_t *pkt;
	uint8_t length;
	struct {
		struct ieee802154_fcf_seq *fc_seq;
		struct ieee802154_address_field *dst_addr;
		struct ieee802154_address_field *src_addr;
	} mhr_check;
};

uint8_t ns_pkt[] = {
	0x41, 0xd8, 0x3e, 0xcd, 0xab, 0xff, 0xff, 0xc2, 0xa3, 0x9e, 0x00,
	0x00, 0x4b, 0x12, 0x00, 0x7b, 0x09, 0x3a, 0x20, 0x01, 0x0d, 0xb8,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x02, 0x01, 0xff, 0x00, 0x00, 0x01, 0x87, 0x00, 0x2e, 0xad,
	0x00, 0x00, 0x00, 0x00, 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02,
	0x00, 0x12, 0x4b, 0x00, 0x00, 0x9e, 0xa3, 0xc2, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x3d, 0x74
};

struct ieee802154_pkt_test test_ns_pkt = {
	.name = "NS frame",
	.src =  { { { 0x20, 0x01, 0xdb, 0x08, 0x00, 0x00, 0x00, 0x00,
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 } } },
	.dst =  { { { 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		      0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x01 } } },
	.pkt = ns_pkt,
	.length = sizeof(ns_pkt),
	.mhr_check.fc_seq = (struct ieee802154_fcf_seq *)ns_pkt,
	.mhr_check.dst_addr = (struct ieee802154_address_field *)(ns_pkt + 3),
	.mhr_check.src_addr = (struct ieee802154_address_field *)(ns_pkt + 7),
};

uint8_t ack_pkt[] = { 0x02, 0x10, 0x16 };

struct ieee802154_pkt_test test_ack_pkt = {
	.name = "ACK frame",
	.pkt = ack_pkt,
	.length = sizeof(ack_pkt),
	.mhr_check.fc_seq = (struct ieee802154_fcf_seq *)ack_pkt,
	.mhr_check.dst_addr = NULL,
	.mhr_check.src_addr = NULL,
};

uint8_t beacon_pkt[] = {
	0x00, 0xd0, 0x11, 0xcd, 0xab, 0xc2, 0xa3, 0x9e, 0x00, 0x00, 0x4b,
	0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

struct ieee802154_pkt_test test_beacon_pkt = {
	.name = "Empty beacon frame",
	.pkt = beacon_pkt,
	.length = sizeof(beacon_pkt),
	.mhr_check.fc_seq = (struct ieee802154_fcf_seq *)beacon_pkt,
	.mhr_check.dst_addr = NULL,
	.mhr_check.src_addr =
	(struct ieee802154_address_field *) (beacon_pkt + 3),
};

uint8_t sec_data_pkt[] = {
	0x49, 0xd8, 0x03, 0xcd, 0xab, 0xff, 0xff, 0x02, 0x6d, 0xbb, 0xa7,
	0x00, 0x4b, 0x12, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xd3, 0x8e,
	0x49, 0xa7, 0xe2, 0x00, 0x67, 0xd4, 0x00, 0x42, 0x52, 0x6f, 0x01,
	0x02, 0x00, 0x12, 0x4b, 0x00, 0xa7, 0xbb, 0x6d, 0x02, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x19, 0x7f, 0x91, 0xcf, 0x73, 0xf0
};

struct ieee802154_pkt_test test_sec_data_pkt = {
	.name = "Secured data frame",
	.pkt = sec_data_pkt,
	.length = sizeof(sec_data_pkt),
	.mhr_check.fc_seq = (struct ieee802154_fcf_seq *)sec_data_pkt,
	.mhr_check.dst_addr =
	(struct ieee802154_address_field *)(sec_data_pkt + 3),
	.mhr_check.src_addr =
	(struct ieee802154_address_field *)(sec_data_pkt + 7),
};

struct net_pkt *current_pkt;
struct net_if *iface;
K_SEM_DEFINE(driver_lock, 0, UINT_MAX);

static void pkt_hexdump(uint8_t *pkt, uint8_t length)
{
	int i;

	printk(" -> Packet content:\n");

	for (i = 0; i < length;) {
		int j;

		printk("\t");

		for (j = 0; j < 10 && i < length; j++, i++) {
			printk("%02x ", *pkt);
			pkt++;
		}

		printk("\n");
	}
}

static void ieee_addr_hexdump(uint8_t *addr, uint8_t length)
{
	int i;

	printk(" -> IEEE 802.15.4 Address: ");

	for (i = 0; i < length-1; i++) {
		printk("%02x:", *addr);
		addr++;
	}

	printk("%02x\n", *addr);
}

static bool test_packet_parsing(struct ieee802154_pkt_test *t)
{
	struct ieee802154_mpdu mpdu;

	NET_INFO("- Parsing packet 0x%p of frame %s\n", t->pkt, t->name);

	if (!ieee802154_validate_frame(t->pkt, t->length, &mpdu)) {
		NET_ERR("*** Could not validate frame %s\n", t->name);
		return false;
	}

	if (mpdu.mhr.fs != t->mhr_check.fc_seq ||
	    mpdu.mhr.dst_addr != t->mhr_check.dst_addr ||
	    mpdu.mhr.src_addr != t->mhr_check.src_addr) {
		NET_INFO("d: %p vs %p -- s: %p vs %p\n",
			 mpdu.mhr.dst_addr, t->mhr_check.dst_addr,
			 mpdu.mhr.src_addr, t->mhr_check.src_addr);
		NET_ERR("*** Wrong MPDU information on frame %s\n",
			t->name);

		return false;
	}

	return true;
}

static bool test_ns_sending(struct ieee802154_pkt_test *t)
{
	struct ieee802154_mpdu mpdu;

	NET_INFO("- Sending NS packet\n");

	if (net_ipv6_send_ns(iface, NULL, &t->src, &t->dst, &t->dst, false)) {
		NET_ERR("*** Could not create IPv6 NS packet\n");
		return false;
	}

	k_yield();
	k_sem_take(&driver_lock, K_SECONDS(1));

	if (!current_pkt->frags) {
		NET_ERR("*** Could not send IPv6 NS packet\n");
		return false;
	}

	pkt_hexdump(net_pkt_data(current_pkt), net_pkt_get_len(current_pkt));

	if (!ieee802154_validate_frame(net_pkt_data(current_pkt),
				       net_pkt_get_len(current_pkt), &mpdu)) {
		NET_ERR("*** Sent packet is not valid\n");
		net_pkt_frag_unref(current_pkt->frags);

		return false;
	}

	net_pkt_frag_unref(current_pkt->frags);
	current_pkt->frags = NULL;

	return true;
}

static bool test_dgram_packet_sending(struct sockaddr_ll *pkt_sll, uint32_t security_level)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	int fd;
	struct sockaddr_ll socket_sll = {0};
	uint8_t payload[4] = {0x01, 0x02, 0x03, 0x04};
	struct ieee802154_mpdu mpdu;
	bool result = false;
	struct ieee802154_security_params params = {
		.key = {0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb,
			0xcc, 0xcd, 0xce, 0xcf},
		.key_len = 16U,
		.key_mode = IEEE802154_KEY_ID_MODE_IMPLICIT,
		.level = security_level,
	};

	if (net_mgmt(NET_REQUEST_IEEE802154_SET_SECURITY_SETTINGS, iface, &params,
		     sizeof(struct ieee802154_security_params))) {
		NET_ERR("*** Failed to set security settings\n");
		goto out;
	}

	NET_INFO("- Sending DGRAM packet via AF_PACKET socket\n");
	fd = socket(AF_PACKET, SOCK_DGRAM, ETH_P_IEEE802154);
	if (fd < 0) {
		NET_ERR("*** Failed to create DGRAM socket : %d\n", errno);
		goto release_sec_ctx;
	}

	socket_sll.sll_ifindex = net_if_get_by_iface(iface);
	socket_sll.sll_family = AF_PACKET;
	socket_sll.sll_protocol = ETH_P_IEEE802154;
	if (bind(fd, (const struct sockaddr *)&socket_sll, sizeof(struct sockaddr_ll))) {
		NET_ERR("*** Failed to bind packet socket : %d\n", errno);
		goto release_fd;
	}

	if (sendto(fd, payload, sizeof(payload), 0, (const struct sockaddr *)pkt_sll,
		   sizeof(struct sockaddr_ll)) != sizeof(payload)) {
		NET_ERR("*** Failed to send, errno %d\n", errno);
		goto release_fd;
	}

	k_yield();
	k_sem_take(&driver_lock, K_SECONDS(1));

	if (!current_pkt->frags) {
		NET_ERR("*** Could not send DGRAM packet\n");
		goto release_fd;
	}

	pkt_hexdump(net_pkt_data(current_pkt), net_pkt_get_len(current_pkt));

	if (!ieee802154_validate_frame(net_pkt_data(current_pkt),
				       net_pkt_get_len(current_pkt), &mpdu)) {
		NET_ERR("*** Sent packet is not valid\n");
		goto release_frag;
	}

	net_pkt_lladdr_src(current_pkt)->addr = ctx->ext_addr;
	net_pkt_lladdr_src(current_pkt)->len = IEEE802154_MAX_ADDR_LENGTH;
	if (!ieee802154_decipher_data_frame(iface, current_pkt, &mpdu)) {
		NET_ERR("*** Cannot decipher/authenticate packet\n");
		goto release_frag;
	}

	if (memcmp(mpdu.payload, payload, sizeof(payload)) != 0) {
		NET_ERR("*** Payload of sent packet is incorrect\n");
		goto release_frag;
	}

	result = true;

release_frag:
	net_pkt_frag_unref(current_pkt->frags);
	current_pkt->frags = NULL;
release_fd:
	close(fd);
release_sec_ctx:
	cipher_free_session(ctx->sec_ctx.enc.device, &ctx->sec_ctx.enc);
	cipher_free_session(ctx->sec_ctx.dec.device, &ctx->sec_ctx.dec);
out:
	ctx->sec_ctx.level = IEEE802154_SECURITY_LEVEL_NONE;
	return result;
}

static bool test_raw_packet_sending(void)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	int fd;
	struct sockaddr_ll socket_sll = {0};
	struct msghdr msg = {0};
	struct iovec io_vector;
	uint8_t payload[20];
	struct ieee802154_mpdu mpdu;
	bool result = false;

	NET_INFO("- Sending RAW packet via AF_PACKET socket\n");
	fd = socket(AF_PACKET, SOCK_RAW, ETH_P_IEEE802154);
	if (fd < 0) {
		NET_ERR("*** Failed to create RAW socket : %d\n", errno);
		goto out;
	}

	socket_sll.sll_ifindex = net_if_get_by_iface(iface);
	socket_sll.sll_family = AF_PACKET;
	socket_sll.sll_protocol = ETH_P_IEEE802154;
	if (bind(fd, (const struct sockaddr *)&socket_sll, sizeof(struct sockaddr_ll))) {
		NET_ERR("*** Failed to bind packet socket : %d\n", errno);
		goto release_fd;
	}

	/* Construct raw packet payload, length and FCS gets added in the radio driver,
	 * see https://github.com/linux-wpan/wpan-tools/blob/master/examples/af_packet_tx.c
	 */
	payload[0] = 0x01; /* Frame Control Field */
	payload[1] = 0xc8; /* Frame Control Field */
	payload[2] = 0x8b; /* Sequence number */
	payload[3] = 0xff; /* Destination PAN ID 0xffff */
	payload[4] = 0xff; /* Destination PAN ID */
	payload[5] = 0x02; /* Destination short address 0x0002 */
	payload[6] = 0x00; /* Destination short address */
	payload[7] = 0x23; /* Source PAN ID 0x0023 */
	payload[8] = 0x00; /* */
	payload[9] = 0x60; /* Source extended address ae:c2:4a:1c:21:16:e2:60 */
	payload[10] = 0xe2; /* */
	payload[11] = 0x16; /* */
	payload[12] = 0x21; /* */
	payload[13] = 0x1c; /* */
	payload[14] = 0x4a; /* */
	payload[15] = 0xc2; /* */
	payload[16] = 0xae; /* */
	payload[17] = 0xAA; /* Payload */
	payload[18] = 0xBB; /* */
	payload[19] = 0xCC; /* */
	io_vector.iov_base = payload;
	io_vector.iov_len = sizeof(payload);
	msg.msg_iov = &io_vector;
	msg.msg_iovlen = 1;

	if (sendmsg(fd, &msg, 0) != sizeof(payload)) {
		NET_ERR("*** Failed to send, errno %d\n", errno);
		goto release_fd;
	}

	k_yield();
	k_sem_take(&driver_lock, K_SECONDS(1));

	if (!current_pkt->frags) {
		NET_ERR("*** Could not send RAW packet\n");
		goto release_fd;
	}

	pkt_hexdump(net_pkt_data(current_pkt), net_pkt_get_len(current_pkt));

	if (!ieee802154_validate_frame(net_pkt_data(current_pkt),
				       net_pkt_get_len(current_pkt), &mpdu)) {
		NET_ERR("*** Sent packet is not valid\n");
		goto release_frag;
	}

	if (memcmp(mpdu.payload, &payload[17], 3) != 0) {
		NET_ERR("*** Payload of sent packet is incorrect\n");
		goto release_frag;
	}

	result = true;

release_frag:
	net_pkt_frag_unref(current_pkt->frags);
	current_pkt->frags = NULL;
release_fd:
	close(fd);
out:
	ctx->sec_ctx.level = IEEE802154_SECURITY_LEVEL_NONE;
	return result;
}

static bool test_ack_reply(struct ieee802154_pkt_test *t)
{
	static uint8_t data_pkt[] = {
		0x61, 0xdc, 0x16, 0xcd, 0xab, 0xc2, 0xa3, 0x9e, 0x00, 0x00, 0x4b,
		0x12, 0x00, 0x26, 0x18, 0x32, 0x00, 0x00, 0x4b, 0x12, 0x00, 0x7b,
		0x00, 0x3a, 0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x20, 0x01, 0x0d, 0xb8,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x02, 0x87, 0x00, 0x8b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x01,
		0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
		0x16, 0xf0, 0x02, 0xff, 0x16, 0xf0, 0x12, 0xff, 0x16, 0xf0, 0x32,
		0xff, 0x16, 0xf0, 0x00, 0xff, 0x16, 0xf0, 0x00, 0xff, 0x16
	};
	struct ieee802154_mpdu mpdu;
	struct net_pkt *pkt;
	struct net_buf *frag;

	NET_INFO("- Sending ACK reply to a data packet\n");

	pkt = net_pkt_rx_alloc(K_FOREVER);
	frag = net_pkt_get_frag(pkt, K_FOREVER);

	memcpy(frag->data, data_pkt, sizeof(data_pkt));
	frag->len = sizeof(data_pkt);

	net_pkt_frag_add(pkt, frag);

	if (net_recv_data(iface, pkt) < 0) {
		NET_ERR("Recv data failed");
		return false;
	}

	k_yield();
	k_sem_take(&driver_lock, K_SECONDS(1));

	/* an ACK packet should be in current_pkt */
	if (!current_pkt->frags) {
		NET_ERR("*** No ACK reply sent\n");
		return false;
	}

	pkt_hexdump(net_pkt_data(current_pkt), net_pkt_get_len(current_pkt));

	if (!ieee802154_validate_frame(net_pkt_data(current_pkt),
				       net_pkt_get_len(current_pkt), &mpdu)) {
		NET_ERR("*** ACK Reply is invalid\n");
		return false;
	}

	if (memcmp(mpdu.mhr.fs, t->mhr_check.fc_seq,
		   sizeof(struct ieee802154_fcf_seq))) {
		NET_ERR("*** ACK Reply does not compare\n");
		return false;
	}

	net_pkt_frag_unref(current_pkt->frags);
	current_pkt->frags = NULL;

	return true;
}

static bool initialize_test_environment(void)
{
	const struct device *dev;

	k_sem_reset(&driver_lock);

	current_pkt = net_pkt_rx_alloc(K_FOREVER);
	if (!current_pkt) {
		NET_ERR("*** No buffer to allocate\n");
		return false;
	}

	dev = device_get_binding("fake_ieee802154");
	if (!dev) {
		NET_ERR("*** Could not get fake device\n");
		return false;
	}

	iface = net_if_lookup_by_dev(dev);
	if (!iface) {
		NET_ERR("*** Could not get fake iface\n");
		return false;
	}

	NET_INFO("Fake IEEE 802.15.4 network interface ready\n");

	ieee_addr_hexdump(net_if_get_link_addr(iface)->addr, 8);

	return true;
}

static void *test_init(void)
{
	bool ret;

	ret = initialize_test_environment();

	zassert_true(ret, "Test initialization");

	return NULL;
}


ZTEST(ieee802154_l2, test_parsing_ns_pkt)
{
	bool ret;

	ret = test_packet_parsing(&test_ns_pkt);

	zassert_true(ret, "NS parsed");
}

ZTEST(ieee802154_l2, test_sending_ns_pkt)
{
	bool ret;

	ret = test_ns_sending(&test_ns_pkt);

	zassert_true(ret, "NS sent");
}

ZTEST(ieee802154_l2, test_parsing_ack_pkt)
{
	bool ret;

	ret = test_packet_parsing(&test_ack_pkt);

	zassert_true(ret, "ACK parsed");
}

ZTEST(ieee802154_l2, test_replying_ack_pkt)
{
	bool ret;

	ret = test_ack_reply(&test_ack_pkt);

	zassert_true(ret, "ACK replied");
}

ZTEST(ieee802154_l2, test_parsing_beacon_pkt)
{
	bool ret;

	ret = test_packet_parsing(&test_beacon_pkt);

	zassert_true(ret, "Beacon parsed");
}

ZTEST(ieee802154_l2, test_parsing_sec_data_pkt)
{
	bool ret;

	ret = test_packet_parsing(&test_sec_data_pkt);

	zassert_true(ret, "Secured data frame parsed");
}

ZTEST(ieee802154_l2, test_sending_broadcast_dgram_pkt)
{
	bool ret;
	uint16_t dst_short_addr = htons(IEEE802154_BROADCAST_ADDRESS);
	struct sockaddr_ll pkt_sll = {
		.sll_halen = sizeof(dst_short_addr),
		.sll_protocol = htons(ETH_P_IEEE802154),
	};
	memcpy(pkt_sll.sll_addr, &dst_short_addr, sizeof(dst_short_addr));

	ret = test_dgram_packet_sending(&pkt_sll, IEEE802154_SECURITY_LEVEL_NONE);

	zassert_true(ret, "Broadcast DGRAM packet sent");
}

ZTEST(ieee802154_l2, test_sending_authenticated_dgram_pkt)
{
	bool ret;
	uint16_t dst_short_addr = htons(0x1234);
	struct sockaddr_ll pkt_sll = {
		.sll_halen = sizeof(dst_short_addr),
		.sll_protocol = htons(ETH_P_IEEE802154),
	};
	memcpy(pkt_sll.sll_addr, &dst_short_addr, sizeof(dst_short_addr));

	ret = test_dgram_packet_sending(&pkt_sll, IEEE802154_SECURITY_LEVEL_MIC_128);

	zassert_true(ret, "Authenticated DGRAM packet sent");
}

ZTEST(ieee802154_l2, test_sending_encrypted_and_authenticated_dgram_pkt)
{
	bool ret;
	uint8_t dst_ext_addr[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	struct sockaddr_ll pkt_sll = {
		.sll_halen = sizeof(dst_ext_addr),
		.sll_protocol = htons(ETH_P_IEEE802154),
	};
	memcpy(pkt_sll.sll_addr, dst_ext_addr, sizeof(dst_ext_addr));

	ret = test_dgram_packet_sending(&pkt_sll, IEEE802154_SECURITY_LEVEL_ENC_MIC_128);

	zassert_true(ret, "Encrypted and authenticated DGRAM packet sent");
}

ZTEST(ieee802154_l2, test_sending_raw_pkt)
{
	bool ret;

	ret = test_raw_packet_sending();

	zassert_true(ret, "RAW packet sent");
}

ZTEST_SUITE(ieee802154_l2, NULL, test_init, NULL, NULL, NULL);
