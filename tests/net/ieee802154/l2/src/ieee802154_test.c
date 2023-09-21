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
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/socket.h>

#include "net_private.h"
#include <ieee802154_frame.h>
#include <ieee802154_priv.h>
#include <ipv6.h>

struct ieee802154_pkt_test {
	char *name;
	struct in6_addr src;
	struct in6_addr dst;
	uint8_t *pkt;
	uint8_t sequence;
	uint8_t length;
	uint8_t payload_length;
	struct {
		struct ieee802154_fcf_seq *fc_seq;
		struct ieee802154_address_field *dst_addr;
		struct ieee802154_address_field *src_addr;
	} mhr_check;
};

/* Sample NS packet with extended address. */
uint8_t ns_pkt[] = {
	0x41, 0xd8, /* FCF */
	0x45, /* Sequence Number */
	0xcd, 0xab, /* PAN ID */
	0xff, 0xff, /* Destination Address (Broadcast) */
	0xc2, 0xa3, 0x9e, 0x00, 0x00, 0x4b, 0x12, 0x00, /* Source Address */
	0x7b, 0x09, /* IPHC Header */
	0x3a, /* Next Header: ICMPv6 */
	0x20, 0x01, 0xdb, 0x08, 0x00, 0x00, 0x00, 0x00, /* IPv6 Source Address */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x02, 0x01, 0xff, 0x00, 0x00, 0x01, /* IPv6 Destination Address */
	0x87, /* ICMPv6 Type: Neighbour Solicitation */
	0x00, /* Code */
	0x91, 0x11, /* Checksum */
	0x00, 0x00, 0x00, 0x00, /* Reserved */
	0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Target Address */
	0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x01,
	0x01, /* ICMPv6 Option: Source LL Address */
	0x02, /* Length */
	0x00, 0x12, 0x4b, 0x00, 0x00, 0x9e, 0xa3, 0xc2, /* LL Address */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00 /* Padding */
};

struct ieee802154_pkt_test test_ns_pkt = {
	.name = "NS frame",
	.src =  { { { 0x20, 0x01, 0xdb, 0x08, 0x00, 0x00, 0x00, 0x00,
		      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 } } },
	.dst =  { { { 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		      0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x01 } } },
	.pkt = ns_pkt,
	.sequence = 69U,
	.length = sizeof(ns_pkt),
	.payload_length = 65U,
	.mhr_check = {
		.fc_seq = (struct ieee802154_fcf_seq *)ns_pkt,
		.dst_addr = (struct ieee802154_address_field *)(ns_pkt + 3),
		.src_addr = (struct ieee802154_address_field *)(ns_pkt + 7),
	}};

uint8_t ack_pkt[] = {
	0x02, 0x10, /* FCF */
	0x16 /* Sequence Number */
};

struct ieee802154_pkt_test test_ack_pkt = {
	.name = "ACK frame",
	.sequence = 22U,
	.pkt = ack_pkt,
	.length = sizeof(ack_pkt),
	.payload_length = 0U,
	.mhr_check = {
		.fc_seq = (struct ieee802154_fcf_seq *)ack_pkt,
		.dst_addr = NULL,
		.src_addr = NULL,
	}};

uint8_t beacon_pkt[] = {
	0x00, 0xd0, /* FCF */
	0x11, /* Sequence Number */
	0xcd, 0xab, /* Source PAN ID */
	0xc2, 0xa3, 0x9e, 0x00, 0x00, 0x4b, 0x12, 0x00, /* Source Address */
	0x00, 0x00, /* Superframe Specification */
	0x00, /* GTS */
	0x00, /* Pending Addresses */
	0x00, 0x00 /* Payload */
};

struct ieee802154_pkt_test test_beacon_pkt = {
	.name = "Empty beacon frame",
	.sequence = 17U,
	.pkt = beacon_pkt,
	.length = sizeof(beacon_pkt),
	.payload_length = 6U,
	.mhr_check = {
		.fc_seq = (struct ieee802154_fcf_seq *)beacon_pkt,
		.dst_addr = NULL,
		.src_addr = (struct ieee802154_address_field *)(beacon_pkt + 3),
	}};

uint8_t sec_data_pkt[] = {
	0x49, 0xd8, /* FCF */
	0x45, /* Sequence Number */
	0xcd, 0xab, /* Destination PAN */
	0xff, 0xff, /* Destination Address */
	0xc2, 0xa3, 0x9e, 0x00, 0x00, 0x4b, 0x12, 0x00, /* Source Address */
	0x07,			/* Security Control Field */
	0x03, 0x00, 0x00, 0x00, /* Frame Counter */
	0x98, 0x49, 0x1f, 0x57, /* Encrypted Payload */
	0x99, 0xf9, 0x10, 0x5e, 0x5b, 0x8c, 0x31, 0x0b, /* MIC (128 bit) */
	0xa1, 0x70, 0x99, 0x14, 0x0a, 0x6a, 0x38, 0x47,
};

struct ieee802154_pkt_test test_sec_data_pkt = {
	.name = "Secured data frame",
	.sequence = 69U,
	.pkt = sec_data_pkt,
	.length = sizeof(sec_data_pkt),
	.payload_length = 4U /* encrypted payload */ + 16U /* MIC */,
	.mhr_check = {
		.fc_seq = (struct ieee802154_fcf_seq *)sec_data_pkt,
		.dst_addr = (struct ieee802154_address_field *)(sec_data_pkt + 3),
		.src_addr = (struct ieee802154_address_field *)(sec_data_pkt + 7),
	}};

/* Construct raw packet payload, length and FCS gets added in the radio driver,
 * see https://github.com/linux-wpan/wpan-tools/blob/master/examples/af_packet_tx.c
 */
uint8_t raw_payload[] = {
	0x01, 0xc8, /* FCF */
	0x8b,	    /* Sequence number */
	0xff, 0xff, /* Destination PAN ID 0xffff */
	0x02, 0x00, /* Destination short address 0x0002 */
	0x23, 0x00, /* Source PAN ID 0x0023 */
	0x60, 0xe2, 0x16, 0x21,
	0x1c, 0x4a, 0xc2, 0xae, /* Source extended address ae:c2:4a:1c:21:16:e2:60 */
	0xAA, 0xBB, 0xCC,	/* MAC Payload */
};
#define RAW_MAC_PAYLOAD_START_INDEX 17
#define RAW_MAC_PAYLOAD_LENGTH 3

#define MOCK_PAN_ID 0xabcd

extern struct net_pkt *current_pkt;
extern struct k_sem driver_lock;

static struct net_if *net_iface;

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

static int disassociate(struct net_if *iface, struct ieee802154_context *ctx)
{
	uint16_t short_addr_not_associated = IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED;
	int ret;

	if (ctx->short_addr == IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED) {
		return 0;
	}

	ret = net_mgmt(NET_REQUEST_IEEE802154_SET_SHORT_ADDR, iface,
		       &short_addr_not_associated,
		       sizeof(short_addr_not_associated));
	if (ret) {
		NET_ERR("*** Failed to %s.\n", __func__);
		return ret;
	}

	return 0;
}

static int associate(struct net_if *iface, struct ieee802154_context *ctx, uint16_t short_addr)
{
	uint16_t mock_pan_id = MOCK_PAN_ID;
	int ret;

	if (ctx->short_addr == short_addr) {
		return -EALREADY;
	}

	ret = net_mgmt(NET_REQUEST_IEEE802154_SET_PAN_ID, iface, &mock_pan_id,
		       sizeof(mock_pan_id));
	if (ret) {
		NET_ERR("*** Failed to set PAN ID in %s.\n", __func__);
		return ret;
	}

	ret = net_mgmt(NET_REQUEST_IEEE802154_SET_SHORT_ADDR, iface, &short_addr,
		       sizeof(short_addr));
	if (ret) {
		NET_ERR("*** Failed to set short addr in %s.\n", __func__);
		return ret;
	}

	return 0;
}

static int set_up_short_addr(struct net_if *iface, struct ieee802154_context *ctx)
{
	const uint16_t mock_short_addr = 0x5678;
	int ret;

	ret = disassociate(iface, ctx);
	if (ret) {
		return ret;
	}

	ret = associate(iface, ctx, mock_short_addr);
	if (ret) {
		return ret;
	}

	return 0;
}

static int tear_down_short_addr(struct net_if *iface, struct ieee802154_context *ctx)
{
	uint16_t no_short_addr_assigned = IEEE802154_NO_SHORT_ADDRESS_ASSIGNED;
	int ret;

	if (ctx->linkaddr.len != IEEE802154_SHORT_ADDR_LENGTH) {
		/* nothing to do */
		return 0;
	}

	ret = disassociate(iface, ctx);
	if (ret) {
		return ret;
	}

	ret = associate(iface, ctx, no_short_addr_assigned);
	if (ret) {
		return ret;
	}

	return 0;
}

static struct net_pkt *get_data_pkt_with_ar(void)
{
	/* Incoming IEEE 802.15.4 packet with payload header compression. */
	static uint8_t data_pkt_with_ar[] = {
		/* IEEE 802.15.4 MHR */
		0x61, 0xd8,					/* FCF with AR bit set */
		0x16,						/* Sequence */
		0xcd, 0xab,					/* Destination PAN */
		0x78, 0x56,					/* Destination Address */
		0xc2, 0xa3, 0x9e, 0x00, 0x00, 0x4b, 0x12, 0x00, /* Source Address */
		/* IEEE 802.15.4 MAC Payload */
		0x7b, 0x39, /* IPHC header, SAM: compressed, DAM: 48-bits inline */
		0x3a,	    /* Next header: ICMPv6 */
		0x02, 0x01, 0xff, 0x4b, 0x12, 0x00, /* IPv6 Destination */
		0x87,				    /* Type: NS */
		0x00,				    /* Code*/
		0xb7, 0x45,			    /* Checksum */
		0x00, 0x00, 0x00, 0x00,		    /* Reserved */
		0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x14, 0xa6, 0x1c, 0x00, 0x4b,
		0x12, 0x00, /* Target Address */
		0x01,	    /* ICMPv6 Option: Source LL address */
		0x02,	    /* Length */
		0xe5, 0xac, 0xa1, 0x1c, 0x00, 0x4b, 0x12, 0x00, /* LL address */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		/* Padding */
	};
	struct net_pkt *pkt;

	pkt = net_pkt_rx_alloc_with_buffer(net_iface, sizeof(data_pkt_with_ar), AF_UNSPEC, 0,
					   K_FOREVER);
	if (!pkt) {
		NET_ERR("*** No buffer to allocate\n");
		return NULL;
	}

	net_buf_add_mem(pkt->frags, data_pkt_with_ar, sizeof(data_pkt_with_ar));

	return pkt;
}

#ifdef CONFIG_NET_SOCKETS
static bool set_up_security(uint8_t security_level)
{
	struct ieee802154_context *ctx = net_if_l2_data(net_iface);
	uint16_t saved_short_addr = ctx->short_addr;
	struct ieee802154_security_params params;

	if (security_level == IEEE802154_SECURITY_LEVEL_NONE) {
		return true;
	}

	if (disassociate(net_iface, ctx) != 0) {
		return false;
	}

	params = (struct ieee802154_security_params){
		.key = {0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb,
			0xcc, 0xcd, 0xce, 0xcf},
		.key_len = 16U,
		.key_mode = IEEE802154_KEY_ID_MODE_IMPLICIT,
		.level = security_level,
	};

	if (net_mgmt(NET_REQUEST_IEEE802154_SET_SECURITY_SETTINGS, net_iface, &params,
		     sizeof(struct ieee802154_security_params))) {
		NET_ERR("*** Failed to set security settings\n");
		return false;
	}

	if (saved_short_addr != IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED &&
	    associate(net_iface, ctx, saved_short_addr) != 0) {
		return false;
	}

	return true;
}

static bool tear_down_security(void)
{
	struct ieee802154_context *ctx = net_if_l2_data(net_iface);
	uint16_t saved_short_addr = ctx->short_addr;
	struct ieee802154_security_params params = {
		.level = IEEE802154_SECURITY_LEVEL_NONE,
	};

	if (disassociate(net_iface, ctx) != 0) {
		return false;
	}

	if (net_mgmt(NET_REQUEST_IEEE802154_SET_SECURITY_SETTINGS, net_iface, &params,
		     sizeof(struct ieee802154_security_params))) {
		NET_ERR("*** Failed to tear down security settings\n");
		return false;
	}

	if (saved_short_addr != IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED &&
	    associate(net_iface, ctx, saved_short_addr) != 0) {
		return false;
	}

	return true;
}

static int set_up_recv_socket(enum net_sock_type socket_type)
{
	struct sockaddr_ll socket_sll = {
		.sll_ifindex = net_if_get_by_iface(net_iface),
		.sll_family = AF_PACKET,
		.sll_protocol = ETH_P_IEEE802154,
	};
	struct timeval timeo_optval = {
		.tv_sec = 1,
		.tv_usec = 0,
	};
	int fd;

	fd = socket(AF_PACKET, socket_type, ETH_P_IEEE802154);
	if (fd < 0) {
		NET_ERR("*** Failed to create recv socket : %d\n", errno);
		return fd;
	}

	if (bind(fd, (const struct sockaddr *)&socket_sll, sizeof(struct sockaddr_ll))) {
		NET_ERR("*** Failed to bind packet socket : %d\n", errno);
		goto release_fd;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeo_optval, sizeof(timeo_optval))) {
		NET_ERR("*** Failed to set reception timeout on packet socket : %d\n", errno);
		goto release_fd;
	}

	return fd;

release_fd:
	close(fd);
	return -EFAULT;
}
#endif /* CONFIG_NET_SOCKETS */

static bool test_packet_parsing(struct ieee802154_pkt_test *t)
{
	struct ieee802154_mpdu mpdu = {0};

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

	if (mpdu.mhr.fs->sequence != t->sequence) {
		NET_ERR("*** Invalid sequence number\n", t->name);
		return false;
	}

	if (mpdu.payload_length != t->payload_length) {
		NET_ERR("*** Invalid payload length\n", t->name);
		return false;
	}

	return true;
}

static bool test_ns_sending(struct ieee802154_pkt_test *t, bool with_short_addr)
{
	struct ieee802154_context *ctx = net_if_l2_data(net_iface);
	struct ieee802154_mpdu mpdu;
	bool result = false;

	NET_INFO("- Sending NS packet\n");

	/* ensure reproducible results */
	ctx->sequence = t->sequence;

	if (with_short_addr && set_up_short_addr(net_iface, ctx)) {
		goto out;
	}

	if (net_ipv6_send_ns(net_iface, NULL, &t->src, &t->dst, &t->dst, false)) {
		NET_ERR("*** Could not create IPv6 NS packet\n");
		tear_down_short_addr(net_iface, ctx);
		goto out;
	}

	tear_down_short_addr(net_iface, ctx);

	k_yield();
	k_sem_take(&driver_lock, K_SECONDS(1));

	if (!current_pkt->frags) {
		NET_ERR("*** Could not send IPv6 NS packet\n");
		goto out;
	}

	pkt_hexdump(net_pkt_data(current_pkt), net_pkt_get_len(current_pkt));

	if (!with_short_addr) {
		if (net_pkt_get_len(current_pkt) != t->length ||
		    memcmp(net_pkt_data(current_pkt), t->pkt, t->length)) {
			NET_ERR("*** Sent packet deviates from expected packet\n");
			goto release_frag;
		}
	}

	if (!ieee802154_validate_frame(net_pkt_data(current_pkt),
				       net_pkt_get_len(current_pkt), &mpdu)) {
		NET_ERR("*** Sent packet is not valid\n");
		goto release_frag;
	}

	result = true;

release_frag:
	net_pkt_frag_unref(current_pkt->frags);
	current_pkt->frags = NULL;
out:
	return result;
}

static bool test_wait_for_ack(struct ieee802154_pkt_test *t)
{
	struct ieee802154_mpdu mpdu;
	struct net_pkt *one_ack_pkt;
	struct net_pkt *tx_pkt;
	bool result = false;
	bool ack_required;

	NET_INFO("- Waiting for ACK reply when sending a data packet\n");

	tx_pkt = get_data_pkt_with_ar();
	if (!tx_pkt) {
		goto out;
	}

	ack_required = ieee802154_prepare_for_ack(net_iface, tx_pkt, tx_pkt->frags);
	if (!ack_required) {
		NET_ERR("*** Expected AR flag to be set\n");
		goto release_tx_pkt;
	}

	if (!ieee802154_validate_frame(net_pkt_data(tx_pkt), net_pkt_get_len(tx_pkt), &mpdu)) {
		NET_ERR("*** Could not parse data pkt.\n");
		goto release_tx_pkt;
	}

	one_ack_pkt = net_pkt_rx_alloc_with_buffer(net_iface, IEEE802154_ACK_PKT_LENGTH,
						   AF_UNSPEC, 0, K_FOREVER);
	if (!one_ack_pkt) {
		NET_ERR("*** Could not allocate ack pkt.\n");
		goto release_tx_pkt;
	}

	if (!ieee802154_create_ack_frame(net_iface, one_ack_pkt, mpdu.mhr.fs->sequence)) {
		NET_ERR("*** Could not create ack frame.\n");
		goto release_tx_pkt;
	}

	pkt_hexdump(net_pkt_data(one_ack_pkt), net_pkt_get_len(one_ack_pkt));

	if (ieee802154_handle_ack(net_iface, one_ack_pkt) != NET_OK) {
		NET_ERR("*** Ack frame was not handled.\n");
		goto release_ack_pkt;
	}

	if (ieee802154_wait_for_ack(net_iface, ack_required) != 0) {
		NET_ERR("*** Ack frame was not recorded.\n");
		goto release_ack_pkt;
	}

	result = true;

release_ack_pkt:
	net_pkt_unref(one_ack_pkt);
release_tx_pkt:
	net_pkt_unref(tx_pkt);
out:
	return result;
}

static bool test_packet_cloning_with_cb(void)
{
	struct net_pkt *pkt;
	struct net_pkt *cloned_pkt;

	NET_INFO("- Cloning packet\n");

	pkt = net_pkt_rx_alloc_with_buffer(net_iface, 64, AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		NET_ERR("*** No buffer to allocate\n");
		return false;
	}

	/* Set some arbitrary flags and data */
	net_pkt_set_ieee802154_ack_fpb(pkt, true);
	net_pkt_set_ieee802154_lqi(pkt, 50U);
	net_pkt_set_ieee802154_frame_secured(pkt, true);

	cloned_pkt = net_pkt_clone(pkt, K_NO_WAIT);
	zassert_not_equal(net_pkt_cb(cloned_pkt), net_pkt_cb(pkt));

	zassert_true(net_pkt_ieee802154_ack_fpb(cloned_pkt));
	zassert_true(net_pkt_ieee802154_frame_secured(cloned_pkt));
	zassert_false(net_pkt_ieee802154_arb(cloned_pkt));
	zassert_false(net_pkt_ieee802154_mac_hdr_rdy(cloned_pkt));
	zassert_equal(net_pkt_ieee802154_lqi(cloned_pkt), 50U);
	zassert_equal(net_pkt_ieee802154_rssi(cloned_pkt), 0U);

	net_pkt_unref(pkt);
	net_pkt_unref(cloned_pkt);

	return true;
}

static bool test_packet_rssi_conversion(void)
{
	uint8_t raw_signed_rssi_dbm;
	int8_t signed_rssi_dbm;
	struct net_pkt *pkt;

	NET_INFO("- RSSI conversion between unsigned and signed representation\n");

	pkt = net_pkt_rx_alloc_on_iface(net_iface, K_NO_WAIT);
	if (!pkt) {
		NET_ERR("*** No pkt to allocate\n");
		return false;
	}

	/* Test setting/getting of unsigned RSSI. */
	net_pkt_set_ieee802154_rssi(pkt, 50U);
	zassert_equal(net_pkt_ieee802154_rssi(pkt), 50U);

	/* Test setting/getting of signed RSSI (in range). */
	net_pkt_set_ieee802154_rssi_dbm(pkt, IEEE802154_MAC_RSSI_DBM_MIN);
	zassert_equal(net_pkt_ieee802154_rssi(pkt), IEEE802154_MAC_RSSI_MIN);
	zassert_equal(net_pkt_ieee802154_rssi_dbm(pkt), IEEE802154_MAC_RSSI_DBM_MIN);
	net_pkt_set_ieee802154_rssi_dbm(pkt, IEEE802154_MAC_RSSI_DBM_MAX);
	zassert_equal(net_pkt_ieee802154_rssi(pkt), IEEE802154_MAC_RSSI_MAX);
	zassert_equal(net_pkt_ieee802154_rssi_dbm(pkt), IEEE802154_MAC_RSSI_DBM_MAX);
	net_pkt_set_ieee802154_rssi_dbm(pkt, 0);
	zassert_equal(net_pkt_ieee802154_rssi(pkt), 174U);
	zassert_equal(net_pkt_ieee802154_rssi_dbm(pkt), 0);

	/* Test setting/getting of signed RSSI (outside range). */
	net_pkt_set_ieee802154_rssi_dbm(pkt, INT16_MIN + 1);
	zassert_equal(net_pkt_ieee802154_rssi(pkt), IEEE802154_MAC_RSSI_MIN);
	zassert_equal(net_pkt_ieee802154_rssi_dbm(pkt), IEEE802154_MAC_RSSI_DBM_MIN);
	net_pkt_set_ieee802154_rssi_dbm(pkt, INT16_MAX);
	zassert_equal(net_pkt_ieee802154_rssi(pkt), IEEE802154_MAC_RSSI_MAX);
	zassert_equal(net_pkt_ieee802154_rssi_dbm(pkt), IEEE802154_MAC_RSSI_DBM_MAX);

	/* Test setting/getting of signed RSSI (special value - "no RSSI available"). */
	net_pkt_set_ieee802154_rssi_dbm(pkt, IEEE802154_MAC_RSSI_DBM_UNDEFINED);
	zassert_equal(net_pkt_ieee802154_rssi(pkt), IEEE802154_MAC_RSSI_UNDEFINED);
	zassert_equal(net_pkt_ieee802154_rssi_dbm(pkt), IEEE802154_MAC_RSSI_DBM_UNDEFINED);

	/* Demonstrate setting/getting of signed RSSI represented as a raw
	 * two-complements value in uint8_t (explicit cast required).
	 */
	raw_signed_rssi_dbm = (uint8_t)-2;
	net_pkt_set_ieee802154_rssi_dbm(pkt, (int8_t) raw_signed_rssi_dbm);
	zassert_equal(net_pkt_ieee802154_rssi(pkt), 172U);
	zassert_equal(net_pkt_ieee802154_rssi_dbm(pkt), -2);

	/* Demonstrate setting/getting of signed RSSI represented as int8_t
	 * (no explicit cast required)
	 */
	signed_rssi_dbm = -2;
	net_pkt_set_ieee802154_rssi_dbm(pkt, signed_rssi_dbm);
	zassert_equal(net_pkt_ieee802154_rssi(pkt), 172U);
	zassert_equal(net_pkt_ieee802154_rssi_dbm(pkt), -2);

	net_pkt_unref(pkt);
	return true;
}

#ifdef CONFIG_NET_SOCKETS
static bool test_dgram_packet_sending(void *dst_sll, uint8_t dst_sll_halen, uint32_t security_level)
{
	/* tests should be run sequentially, so no need for context locking */
	struct ieee802154_context *ctx = net_if_l2_data(net_iface);
	struct sockaddr_ll socket_sll = {.sll_ifindex = net_if_get_by_iface(net_iface),
					 .sll_family = AF_PACKET,
					 .sll_protocol = ETH_P_IEEE802154};
	struct sockaddr_ll pkt_dst_sll = {
		.sll_halen = dst_sll_halen,
		.sll_protocol = htons(ETH_P_IEEE802154),
	};
	uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};
	struct ieee802154_mpdu mpdu;
	bool result = false;
	int fd;

	if (!set_up_security(security_level)) {
		goto out;
	}

	NET_INFO("- Sending DGRAM packet via AF_PACKET socket\n");
	fd = socket(AF_PACKET, SOCK_DGRAM, ETH_P_IEEE802154);
	if (fd < 0) {
		NET_ERR("*** Failed to create DGRAM socket : %d\n", errno);
		goto reset_security;
	}

	/* In case we have a short destination address
	 * we simulate an associated device.
	 */
	/* TODO: support short addresses with encryption (requires neighbour cache) */
	memcpy(pkt_dst_sll.sll_addr, dst_sll, dst_sll_halen);
	bool bind_short_address = pkt_dst_sll.sll_halen == IEEE802154_SHORT_ADDR_LENGTH &&
				  security_level == IEEE802154_SECURITY_LEVEL_NONE;

	if (bind_short_address && set_up_short_addr(net_iface, ctx)) {
		goto release_fd;
	}

	if (bind(fd, (const struct sockaddr *)&socket_sll, sizeof(struct sockaddr_ll))) {
		NET_ERR("*** Failed to bind packet socket : %d\n", errno);
		goto release_fd;
	}

	if (sendto(fd, payload, sizeof(payload), 0, (const struct sockaddr *)&pkt_dst_sll,
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

	net_pkt_lladdr_src(current_pkt)->addr = net_if_get_link_addr(net_iface)->addr;
	net_pkt_lladdr_src(current_pkt)->len = net_if_get_link_addr(net_iface)->len;

	if (!ieee802154_decipher_data_frame(net_iface, current_pkt, &mpdu)) {
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
	tear_down_short_addr(net_iface, ctx);
	close(fd);
reset_security:
	tear_down_security();
out:
	return result;
}

/* src_ll_addr is always big endian */
static bool test_dgram_packet_reception(void *src_ll_addr, uint8_t src_ll_addr_len,
					uint32_t security_level, bool is_broadcast)
{
	struct ieee802154_context *ctx = net_if_l2_data(net_iface);
	uint8_t our_ext_addr[IEEE802154_EXT_ADDR_LENGTH]; /* big endian */
	uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};
	uint16_t our_short_addr = ctx->short_addr; /* CPU byte order */
	uint8_t ll_hdr_len = 0, authtag_len = 0;
	struct sockaddr_ll recv_src_sll = {0};
	uint8_t received_payload[4] = {0};
	socklen_t recv_src_sll_len;
	struct net_buf *frame_buf;
	struct net_pkt *pkt;
	bool frame_result;
	int received_len;
	bool result;
	int fd;

	result = false;

	sys_memcpy_swap(our_ext_addr, ctx->ext_addr, sizeof(our_ext_addr));

	if (!set_up_security(security_level)) {
		goto out;
	}

	NET_INFO("- Receiving DGRAM packet via AF_PACKET socket\n");

	fd = set_up_recv_socket(SOCK_DGRAM);
	if (fd < 0) {
		goto reset_security;
	}

	pkt = net_pkt_rx_alloc(K_FOREVER);
	if (!pkt) {
		NET_ERR("*** Failed to allocate net pkt.\n");
		goto release_fd;
	}

	pkt->lladdr_dst.type = NET_LINK_IEEE802154;
	pkt->lladdr_dst.addr = is_broadcast ? NULL : our_ext_addr;
	pkt->lladdr_dst.len = is_broadcast ? 0 : sizeof(ctx->ext_addr);

	if (src_ll_addr_len == IEEE802154_SHORT_ADDR_LENGTH ||
	    src_ll_addr_len == IEEE802154_EXT_ADDR_LENGTH) {
		pkt->lladdr_src.addr = src_ll_addr;
	} else {
		NET_ERR("*** Illegal L2 source address length.\n");
		goto release_pkt;
	}
	pkt->lladdr_src.len = src_ll_addr_len;

	frame_buf = net_pkt_get_frag(pkt, IEEE802154_MTU, K_FOREVER);
	if (!frame_buf) {
		NET_ERR("*** Failed to allocate net pkt frag.\n");
		goto release_pkt;
	}

	ieee802154_compute_header_and_authtag_len(
		net_iface, net_pkt_lladdr_dst(pkt), net_pkt_lladdr_src(pkt),
		&ll_hdr_len, &authtag_len);

	net_buf_add(frame_buf, ll_hdr_len);
	net_buf_add_mem(frame_buf, payload, sizeof(payload));
	net_buf_add(frame_buf, authtag_len);

	/* Temporarily set the ctx address to the given source address so
	 * we can use ieee802154_create_data_frame().
	 */
	if (src_ll_addr_len == IEEE802154_SHORT_ADDR_LENGTH) {
		ctx->short_addr = ntohs(*(uint16_t *)src_ll_addr);
	} else if (src_ll_addr_len == IEEE802154_EXT_ADDR_LENGTH) {
		sys_memcpy_swap(ctx->ext_addr, src_ll_addr, sizeof(ctx->ext_addr));
	} else {
		NET_ERR("*** Illegal L2 source address length.\n");
		goto release_pkt;
	}

	frame_result = ieee802154_create_data_frame(ctx, net_pkt_lladdr_dst(pkt),
						    net_pkt_lladdr_src(pkt), frame_buf, ll_hdr_len);

	if (src_ll_addr_len == IEEE802154_SHORT_ADDR_LENGTH) {
		ctx->short_addr = our_short_addr;
	} else {
		sys_memcpy_swap(ctx->ext_addr, our_ext_addr, sizeof(ctx->ext_addr));
	}

	if (!frame_result) {
		NET_ERR("*** Error while creating data frame.\n");
		goto release_pkt;
	};

	net_pkt_frag_add(pkt, frame_buf);

	if (net_recv_data(net_iface, pkt)) {
		NET_ERR("*** Error while processing packet.\n");
		goto release_pkt;
	}

	if (current_pkt->frags) {
		NET_ERR("*** Generated unexpected (ACK?) packet when processing packet.\n");
		net_pkt_frag_unref(current_pkt->frags);
		current_pkt->frags = NULL;
		goto release_pkt;
	}

	recv_src_sll_len = sizeof(recv_src_sll);
	received_len = recvfrom(fd, received_payload, sizeof(received_payload), 0,
				(struct sockaddr *)&recv_src_sll, &recv_src_sll_len);
	if (received_len < 0) {
		NET_ERR("*** Failed to receive packet, errno %d\n", errno);
		goto release_pkt;
	}

	pkt_hexdump(received_payload, received_len);

	if (received_len != sizeof(payload) || memcmp(received_payload, payload, sizeof(payload))) {
		NET_ERR("*** Payload of received packet is incorrect\n");
		goto release_pkt;
	}

	if (recv_src_sll_len != sizeof(struct sockaddr_ll) ||
	    recv_src_sll.sll_family != AF_PACKET || recv_src_sll.sll_protocol != ETH_P_IEEE802154 ||
	    recv_src_sll.sll_ifindex != net_if_get_by_iface(net_iface) ||
	    recv_src_sll.sll_halen != src_ll_addr_len ||
	    memcmp(recv_src_sll.sll_addr, src_ll_addr, src_ll_addr_len)) {
		NET_ERR("*** Source L2 address of received packet is incorrect\n");
		goto release_pkt;
	}

	result = true;

release_pkt:
	net_pkt_unref(pkt);
release_fd:
	close(fd);
reset_security:
	tear_down_security();
out:
	return result;
}

static bool test_raw_packet_sending(void)
{
	/* tests should be run sequentially, so no need for context locking */
	struct sockaddr_ll socket_sll = {0};
	struct ieee802154_mpdu mpdu;
	struct msghdr msg = {0};
	struct iovec io_vector;
	bool result = false;
	int fd;

	NET_INFO("- Sending RAW packet via AF_PACKET socket\n");

	fd = socket(AF_PACKET, SOCK_RAW, ETH_P_IEEE802154);
	if (fd < 0) {
		NET_ERR("*** Failed to create RAW socket : %d\n", errno);
		goto out;
	}

	socket_sll.sll_ifindex = net_if_get_by_iface(net_iface);
	socket_sll.sll_family = AF_PACKET;
	socket_sll.sll_protocol = ETH_P_IEEE802154;

	if (bind(fd, (const struct sockaddr *)&socket_sll, sizeof(struct sockaddr_ll))) {
		NET_ERR("*** Failed to bind packet socket : %d\n", errno);
		goto release_fd;
	}

	io_vector.iov_base = raw_payload;
	io_vector.iov_len = sizeof(raw_payload);
	msg.msg_iov = &io_vector;
	msg.msg_iovlen = 1;

	if (sendmsg(fd, &msg, 0) != sizeof(raw_payload)) {
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

	if (memcmp(mpdu.payload, &raw_payload[RAW_MAC_PAYLOAD_START_INDEX],
		   RAW_MAC_PAYLOAD_LENGTH) != 0) {
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
	return result;
}

static bool test_raw_packet_reception(void)
{
	uint8_t received_payload[sizeof(raw_payload)] = {0};
	struct net_buf *frame_buf;
	struct net_pkt *pkt;
	int received_len;
	bool result;
	int fd;

	result = false;

	NET_INFO("- Receiving RAW packet via AF_PACKET socket\n");

	fd = set_up_recv_socket(SOCK_RAW);
	if (fd < 0) {
		goto out;
	}

	pkt = net_pkt_rx_alloc(K_FOREVER);
	if (!pkt) {
		NET_ERR("*** Failed to allocate net pkt.\n");
		goto release_fd;
	}

	frame_buf = net_pkt_get_frag(pkt, sizeof(raw_payload), K_FOREVER);
	if (!frame_buf) {
		NET_ERR("*** Failed to allocate net pkt frag.\n");
		goto release_pkt;
	}

	net_buf_add_mem(frame_buf, raw_payload, sizeof(raw_payload));
	net_pkt_frag_add(pkt, frame_buf);

	if (net_recv_data(net_iface, pkt)) {
		NET_ERR("*** Error while processing packet.\n");
		goto release_pkt;
	}

	if (current_pkt->frags) {
		NET_ERR("*** Generated unexpected packet when processing packet.\n");
		net_pkt_frag_unref(current_pkt->frags);
		current_pkt->frags = NULL;
		goto release_pkt;
	}

	/* TODO: For POSIX compliance raw packets should be parsed and a LL header be
	 *       extracted. We'll only be able to do so when Zephyr provides hooks to
	 *       call out to L2 from raw socket contexts.
	 */
	received_len = recv(fd, received_payload, sizeof(received_payload), 0);
	if (received_len < 0) {
		NET_ERR("*** Failed to receive packet, errno %d\n", errno);
		goto release_pkt;
	}

	pkt_hexdump(received_payload, received_len);

	/* TODO: The received raw packet should actually contain an FCS
	 *       for full compatibility with Linux's raw socket implementation.
	 *       This will only be possible once we
	 *         1) let HW drivers include FCS if they have it and
	 *         2) provide a hook for mangling raw packets that allows us
	 *            to include a synthetic FCS if the HW driver does not
	 *            provide one.
	 */
	if (received_len != sizeof(raw_payload) ||
	    memcmp(received_payload, raw_payload, sizeof(raw_payload))) {
		NET_ERR("*** Payload of received packet is incorrect\n");
		goto release_pkt;
	}

	result = true;

release_pkt:
	net_pkt_unref(pkt);
release_fd:
	close(fd);
out:
	return result;
}

static bool test_recv_and_send_ack_reply(struct ieee802154_pkt_test *t)
{
	/* Expected uncompressed IPv6 payload. */
	static uint8_t expected_rx_pkt[] = {
		0x60, 0x00, 0x00, 0x00, /* IPv6, Traffic Class, Flow Label */
		0x00, 0x28,		/* Payload Length */
		0x3a,			/* Next header: ICMPv6 */
		0xff,			/* Hop Limit */
		0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x02, 0x12, 0x4b, 0x00, 0x00, 0x9e, 0xa3, 0xc2, /* Source */
		0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x01, 0xff, 0x4b, 0x12, 0x00, /* Destination */
		0x87,						/* Type: NS */
		0x00,						/* Code*/
		0xb7, 0x45,					/* Checksum */
		0x00, 0x00, 0x00, 0x00,				/* Reserved */
		0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x70, 0x14, 0xa6, 0x1c, 0x00, 0x4b, 0x12, 0x00, /* Target Address */
		0x01, /* ICMPv6 Option: Source LL address */
		0x02, /* Length */
		0xe5, 0xac, 0xa1, 0x1c, 0x00, 0x4b, 0x12, 0x00, /* LL address */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		/* Padding */
	};
	struct ieee802154_context *ctx = net_if_l2_data(net_iface);
	struct sockaddr_ll recv_src_sll = {0};
	struct sockaddr_ll socket_sll = {
		.sll_ifindex = net_if_get_by_iface(net_iface),
		.sll_family = AF_PACKET,
		.sll_protocol = ETH_P_IEEE802154,
	};
	uint8_t received_payload[80] = {0};
	struct timeval timeo_optval = {
		.tv_sec = 1,
		.tv_usec = 0,
	};
	struct ieee802154_mpdu mpdu;
	socklen_t recv_src_sll_len;
	struct net_pkt *rx_pkt;
	bool result = false;
	uint8_t mac_be[8];
	int received_len;
	int fd;

	NET_INFO("- Sending ACK reply to a data packet\n");

	fd = socket(AF_PACKET, SOCK_DGRAM, ETH_P_IEEE802154);
	if (fd < 0) {
		NET_ERR("*** Failed to create DGRAM socket : %d\n", errno);
		goto out;
	}

	if (bind(fd, (const struct sockaddr *)&socket_sll, sizeof(struct sockaddr_ll))) {
		NET_ERR("*** Failed to bind packet socket : %d\n", errno);
		goto release_fd;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeo_optval, sizeof(timeo_optval))) {
		NET_ERR("*** Failed to set reception timeout on packet socket : %d\n", errno);
		goto release_fd;
	}

	if (set_up_short_addr(net_iface, ctx)) {
		goto release_fd;
	}

	rx_pkt = get_data_pkt_with_ar();
	if (!rx_pkt) {
		goto reset_short_addr;
	}

	if (net_recv_data(net_iface, rx_pkt) < 0) {
		NET_ERR("Recv data failed");
		goto release_rx_pkt;
	}

	recv_src_sll_len = sizeof(recv_src_sll);
	received_len = recvfrom(fd, received_payload, sizeof(received_payload), 0,
				(struct sockaddr *)&recv_src_sll, &recv_src_sll_len);
	if (received_len < 0) {
		NET_ERR("*** Failed to receive packet, errno %d\n", errno);
		goto release_rx_pkt;
	}

	sys_memcpy_swap(mac_be, ctx->ext_addr, IEEE802154_EXT_ADDR_LENGTH);
	if (recv_src_sll_len != sizeof(struct sockaddr_ll) ||
	    recv_src_sll.sll_ifindex != net_if_get_by_iface(net_iface) ||
	    recv_src_sll.sll_family != AF_PACKET || recv_src_sll.sll_protocol != ETH_P_IEEE802154 ||
	    recv_src_sll.sll_halen != IEEE802154_EXT_ADDR_LENGTH ||
	    memcmp(recv_src_sll.sll_addr, mac_be, IEEE802154_EXT_ADDR_LENGTH)) {
		NET_ERR("*** Received socket address does not compare\n", errno);
		goto release_rx_pkt;
	}

	pkt_hexdump(received_payload, received_len);

	if (memcmp(expected_rx_pkt, received_payload,
		   sizeof(expected_rx_pkt))) {
		NET_ERR("*** Received uncompressed IPv6 payload does not compare\n");
		goto release_rx_pkt;
	}

	k_yield();
	k_sem_take(&driver_lock, K_SECONDS(1));

	/* an ACK packet should be in current_pkt */
	if (!current_pkt->frags) {
		NET_ERR("*** No ACK reply sent\n");
		goto release_rx_pkt;
	}

	pkt_hexdump(net_pkt_data(current_pkt), net_pkt_get_len(current_pkt));

	if (!ieee802154_validate_frame(net_pkt_data(current_pkt),
				       net_pkt_get_len(current_pkt), &mpdu)) {
		NET_ERR("*** ACK Reply is invalid\n");
		goto release_tx_frag;
	}

	if (memcmp(mpdu.mhr.fs, t->mhr_check.fc_seq,
		   sizeof(struct ieee802154_fcf_seq))) {
		NET_ERR("*** ACK Reply does not compare\n");
		goto release_tx_frag;
	}

	if (mpdu.mhr.fs->sequence != t->sequence) {
		NET_ERR("*** Sequence number invalid\n");
		goto release_tx_frag;
	}

	result = true;

release_tx_frag:
	net_pkt_frag_unref(current_pkt->frags);
	current_pkt->frags = NULL;
release_rx_pkt:
	net_pkt_unref(rx_pkt);
reset_short_addr:
	tear_down_short_addr(net_iface, ctx);
release_fd:
	close(fd);
out:
	return result;
}
#endif /* CONFIG_NET_SOCKETS */

static bool initialize_test_environment(void)
{
	uint16_t mock_pan_id = MOCK_PAN_ID;
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
		goto release_pkt;
	}

	net_iface = net_if_lookup_by_dev(dev);
	if (!net_iface) {
		NET_ERR("*** Could not get fake iface\n");
		goto release_pkt;
	}

	if (net_mgmt(NET_REQUEST_IEEE802154_SET_PAN_ID, net_iface,
		     &mock_pan_id, sizeof(mock_pan_id))) {
		NET_ERR("*** Failed to set PAN ID in %s.\n", __func__);
		goto release_pkt;
	}

	NET_INFO("Fake IEEE 802.15.4 network interface ready\n");

	ieee_addr_hexdump(net_if_get_link_addr(net_iface)->addr, 8);

	return true;

release_pkt:
	net_pkt_unref(current_pkt);
	return false;
}

static void *test_setup(void)
{
	bool ret;

	ret = initialize_test_environment();

	zassert_true(ret, "Test initialization");

	return NULL;
}

static void test_teardown(void *test_fixture)
{
	ARG_UNUSED(test_fixture);

	net_pkt_unref(current_pkt);
	current_pkt = NULL;
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

	ret = test_ns_sending(&test_ns_pkt, false);

	zassert_true(ret, "NS sent");
}

ZTEST(ieee802154_l2, test_sending_ns_pkt_with_short_addr)
{
	bool ret;

	ret = test_ns_sending(&test_ns_pkt, true);

	zassert_true(ret, "NS sent");
}

ZTEST(ieee802154_l2, test_parsing_ack_pkt)
{
	bool ret;

	ret = test_packet_parsing(&test_ack_pkt);

	zassert_true(ret, "ACK parsed");
}

ZTEST(ieee802154_l2, test_waiting_for_ack_pkt)
{
	bool ret;

	ret = test_wait_for_ack(&test_ack_pkt);

	zassert_true(ret, "ACK received");
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

ZTEST(ieee802154_l2, test_clone_cb)
{
	bool ret;

	ret = test_packet_cloning_with_cb();

	zassert_true(ret, "IEEE 802.15.4 net_pkt control block correctly cloned.");
}

ZTEST(ieee802154_l2, test_convert_rssi)
{
	bool ret;

	ret = test_packet_rssi_conversion();

	zassert_true(ret, "IEEE 802.15.4 net_pkt RSSI value correctly converted between dBm and "
			  "normalized value.");
}

ZTEST_SUITE(ieee802154_l2, NULL, test_setup, NULL, NULL, test_teardown);

#ifdef CONFIG_NET_SOCKETS
ZTEST(ieee802154_l2_sockets, test_receiving_pkt_and_replying_ack_pkt)
{
	bool ret;

	ret = test_recv_and_send_ack_reply(&test_ack_pkt);

	zassert_true(ret, "ACK sent");
}

ZTEST(ieee802154_l2_sockets, test_sending_broadcast_dgram_pkt)
{
	uint16_t dst_short_addr = htons(IEEE802154_BROADCAST_ADDRESS);
	bool ret;

	ret = test_dgram_packet_sending(&dst_short_addr, sizeof(dst_short_addr),
					IEEE802154_SECURITY_LEVEL_NONE);

	zassert_true(ret, "Broadcast DGRAM packet sent");
}

ZTEST(ieee802154_l2_sockets, test_receiving_broadcast_dgram_pkt)
{
	uint16_t src_short_addr = htons(0x1234);
	bool ret;

	ret = test_dgram_packet_reception(&src_short_addr, sizeof(src_short_addr),
					  IEEE802154_SECURITY_LEVEL_NONE, true);

	zassert_true(ret, "Broadcast DGRAM packet received");
}

ZTEST(ieee802154_l2_sockets, test_sending_authenticated_dgram_pkt)
{
	uint16_t dst_short_addr = htons(0x1234);
	bool ret;

	ret = test_dgram_packet_sending(&dst_short_addr, sizeof(dst_short_addr),
					IEEE802154_SECURITY_LEVEL_MIC_128);

	zassert_true(ret, "Authenticated DGRAM packet sent");
}

ZTEST(ieee802154_l2_sockets, test_receiving_authenticated_dgram_pkt)
{
	/* TODO: Receiving authenticated packages with short addresses is not
	 * yet supported (requires neighbour cache).
	 */
	uint8_t src_ext_addr[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	bool ret;

	ret = test_dgram_packet_reception(src_ext_addr, sizeof(src_ext_addr),
					  IEEE802154_SECURITY_LEVEL_MIC_128, false);

	zassert_true(ret, "Authenticated DGRAM packet received");
}

ZTEST(ieee802154_l2_sockets, test_sending_encrypted_and_authenticated_dgram_pkt)
{
	uint8_t dst_ext_addr[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	bool ret;

	ret = test_dgram_packet_sending(dst_ext_addr, sizeof(dst_ext_addr),
					IEEE802154_SECURITY_LEVEL_ENC_MIC_128);

	zassert_true(ret, "Encrypted and authenticated DGRAM packet sent");
}

ZTEST(ieee802154_l2_sockets, test_receiving_encrypted_and_authenticated_dgram_pkt)
{
	uint8_t src_ext_addr[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	bool ret;

	ret = test_dgram_packet_reception(src_ext_addr, sizeof(src_ext_addr),
					  IEEE802154_SECURITY_LEVEL_ENC_MIC_128, false);

	zassert_true(ret, "Encrypted and authenticated DGRAM packet received");
}

ZTEST(ieee802154_l2_sockets, test_sending_raw_pkt)
{
	bool ret;

	ret = test_raw_packet_sending();

	zassert_true(ret, "RAW packet sent");
}

ZTEST(ieee802154_l2_sockets, test_receiving_raw_pkt)
{
	bool ret;

	ret = test_raw_packet_reception();

	zassert_true(ret, "RAW packet received");
}

ZTEST_SUITE(ieee802154_l2_sockets, NULL, test_setup, NULL, NULL, test_teardown);
#endif /* CONFIG_NET_SOCKETS */
