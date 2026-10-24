/*
 * Copyright (c) 2026 Aleksandr Senin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp_ethernet.h>
#include <smp_internal.h>

static uint8_t local_mac[NET_ETH_ADDR_LEN] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 };
static const uint8_t peer_mac[NET_ETH_ADDR_LEN] = { 0x02, 0xaa, 0xbb, 0xcc, 0xdd, 0xee };
static const uint8_t multicast_mac[NET_ETH_ADDR_LEN] = { 0x03, 0x00, 0x00, 0x00, 0x00, 0x01 };
static const uint8_t broadcast_mac[NET_ETH_ADDR_LEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
static struct {
	struct net_eth_hdr eth;
	struct smp_hdr smp;
	struct net_if *iface;
	int error;
} transmitted;
static K_SEM_DEFINE(tx_complete, 0, 1);

static enum ethernet_hw_caps test_get_capabilities(const struct device *dev, struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return IS_ENABLED(CONFIG_TEST_ETH_HW_FILTERING) ? ETHERNET_HW_FILTERING : 0;
}

static int test_set_config(const struct device *dev, struct net_if *iface,
			   enum ethernet_config_type type, const struct ethernet_config *config)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);
	ARG_UNUSED(config);

	if (type != ETHERNET_CONFIG_TYPE_FILTER) {
		return -ENOTSUP;
	}

	return 0;
}

static void test_iface_init(struct net_if *iface)
{
	net_if_set_link_addr(iface, local_mac, sizeof(local_mac), NET_LINK_ETHERNET);
	ethernet_init(iface);
}

static int test_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);

	if (net_pkt_ll_proto_type(pkt) != CONFIG_MCUMGR_TRANSPORT_ETHERNET_ETHERTYPE) {
		return 0;
	}

	transmitted.iface = net_pkt_iface(pkt);
	net_pkt_cursor_init(pkt);
	transmitted.error = net_pkt_read(pkt, &transmitted.eth, sizeof(transmitted.eth));
	if (transmitted.error == 0) {
		transmitted.error = net_pkt_read(pkt, &transmitted.smp, sizeof(transmitted.smp));
	}
	k_sem_give(&tx_complete);

	return 0;
}

static const struct ethernet_api test_eth_api = {
	.iface_api.init = test_iface_init,
	.get_capabilities = test_get_capabilities,
	.set_config = test_set_config,
	.send = test_send,
};

ETH_NET_DEVICE_INIT(test_eth, "test_eth", NULL, NULL, NULL, NULL,
		    CONFIG_ETH_INIT_PRIORITY, &test_eth_api, NET_ETH_MTU);
ETH_NET_DEVICE_INIT(test_eth2, "test_eth2", NULL, NULL, NULL, NULL,
		    CONFIG_ETH_INIT_PRIORITY, &test_eth_api, NET_ETH_MTU);

#if !defined(CONFIG_MCUMGR_TRANSPORT_ETHERNET)
#error "CONFIG_MCUMGR_TRANSPORT_ETHERNET must be enabled for this test"
#endif

BUILD_ASSERT(sizeof(struct smp_ethernet_addr) <= CONFIG_MCUMGR_TRANSPORT_NETBUF_USER_DATA_SIZE,
	     "net_buf user data area is too small for struct smp_ethernet_addr");

/* The transport hands a received frame to SMP through this function; capture
 * the buffer instead so that a test can inspect what was forwarded.
 */
static struct net_buf *rx_req;

void smp_rx_req(struct smp_transport *smpt, struct net_buf *nb)
{
	ARG_UNUSED(smpt);

	rx_req = nb;
}

static void smp_req_hdr_build(uint8_t *buf, uint16_t data_len)
{
	struct smp_hdr *hdr = (struct smp_hdr *)buf;

	memset(hdr, 0, sizeof(*hdr));
	hdr->nh_op = MGMT_OP_READ;
	hdr->nh_version = SMP_MCUMGR_VERSION_2;
	hdr->nh_len = sys_cpu_to_be16(data_len);
	hdr->nh_seq = 0x42;
}

/* Feed a payload to the registered L3 handler the way the Ethernet layer does. */
static enum net_verdict smp_ethernet_rx_to(const uint8_t *payload, size_t payload_len,
				    const uint8_t *dst, size_t dst_len)
{
	enum net_verdict (*handler)(struct net_if *iface, uint16_t ptype,
				    struct net_pkt *pkt) = NULL;
	struct net_pkt *pkt;
	enum net_verdict verdict;

	STRUCT_SECTION_FOREACH(net_l3_register, l3) {
		if (l3->ptype == CONFIG_MCUMGR_TRANSPORT_ETHERNET_ETHERTYPE &&
		    l3->l2 == &NET_L2_GET_NAME(ETHERNET)) {
			handler = l3->handler;
			break;
		}
	}

	zassert_not_null(handler, "raw Ethernet L3 handler is not registered");

	pkt = net_pkt_alloc_with_buffer(NULL, payload_len, NET_AF_UNSPEC, 0, K_NO_WAIT);
	zassert_not_null(pkt, "failed to allocate the packet");
	zassert_ok(net_pkt_write(pkt, payload, payload_len), "failed to fill the packet");

	zassert_ok(net_linkaddr_set(net_pkt_lladdr_src(pkt), peer_mac, sizeof(peer_mac)),
		   "failed to set the source address");
	if (dst != NULL) {
		zassert_ok(net_linkaddr_set(net_pkt_lladdr_dst(pkt), dst, dst_len), NULL);
	}

	verdict = handler(net_if_get_by_index(1), CONFIG_MCUMGR_TRANSPORT_ETHERNET_ETHERTYPE, pkt);

	/* The handler releases the packet only when it consumes it. */
	if (verdict != NET_OK) {
		net_pkt_unref(pkt);
	}

	return verdict;
}

static enum net_verdict smp_ethernet_rx(const uint8_t *payload, size_t payload_len)
{
	return smp_ethernet_rx_to(payload, payload_len, local_mac, sizeof(local_mac));
}

static enum net_verdict smp_ethernet_frame_rx(struct net_if *iface, const uint8_t *payload,
				       size_t payload_len, const uint8_t *destination)
{
	struct net_eth_hdr hdr = {
		.type = sys_cpu_to_be16(CONFIG_MCUMGR_TRANSPORT_ETHERNET_ETHERTYPE),
	};
	struct net_pkt *pkt = net_pkt_alloc_with_buffer(
		iface, sizeof(hdr) + payload_len, NET_AF_UNSPEC, 0, K_NO_WAIT);
	enum net_verdict verdict;

	zassert_not_null(pkt, NULL);
	memcpy(hdr.src.addr, peer_mac, sizeof(peer_mac));
	memcpy(hdr.dst.addr, destination, NET_ETH_ADDR_LEN);
	zassert_ok(net_pkt_write(pkt, &hdr, sizeof(hdr)), NULL);
	zassert_ok(net_pkt_write(pkt, payload, payload_len), NULL);
	verdict = net_if_recv_data(iface, pkt);
	if (verdict != NET_OK) {
		net_pkt_unref(pkt);
	}

	return verdict;
}

static void rx_req_check(const uint8_t *frame, size_t frame_len)
{
	zassert_not_null(rx_req, "no request was passed to SMP");
	zassert_equal(frame_len, (size_t)rx_req->len, "request is %zu bytes, expected %zu",
		      (size_t)rx_req->len, frame_len);
	zassert_mem_equal(rx_req->data, frame, frame_len, "request content differs");

	smp_packet_free(rx_req);
	rx_req = NULL;
}

/* The transport must register itself and wire up its callbacks at init. */
ZTEST(transport_ethernet, test_transport_registered)
{
	struct smp_transport *smpt = smp_client_transport_get(SMP_ETHERNET_TRANSPORT);

	zassert_not_null(smpt, "raw Ethernet transport was not registered");
	zassert_not_null(smpt->functions.output, "transport output callback is not set");
	zassert_not_null(smpt->functions.ud_copy, "transport ud_copy callback is not set");
	zassert_not_null(smpt->functions.ud_init, "transport ud_init callback is not set");
}

ZTEST(transport_ethernet, test_open_close)
{
	zassert_equal(0, smp_ethernet_close(), NULL);
	zassert_equal(0, smp_ethernet_open(), NULL);
	zassert_equal(0, smp_ethernet_open(), NULL);
	zassert_equal(0, smp_ethernet_close(), NULL);
	/* Leave the transport open, matching the automatic-init default. */
	zassert_equal(0, smp_ethernet_open(), NULL);
}

ZTEST(transport_ethernet, test_client_set_dst)
{
	struct smp_ethernet_addr addr = {
		.mac = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 },
		.iface_index = 1,
	};
	struct smp_client_object obj;
	int rc;

	rc = smp_client_object_init(&obj, SMP_ETHERNET_TRANSPORT);
	zassert_equal(MGMT_ERR_EOK, rc, "client object init failed: %d", rc);
	zassert_not_null(obj.smpt, "client object has no transport");

	rc = smp_client_ethernet_set_dst(&obj, &addr);
	zassert_equal(0, rc, "set_dst failed: %d", rc);
	zassert_equal_ptr(&addr, smp_client_object_get_data(&obj),
			  "destination address was not stored");
}

/* A client object bound to a different transport must be rejected. */
ZTEST(transport_ethernet, test_client_set_dst_wrong_transport)
{
	struct smp_ethernet_addr addr = {
		.mac = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x02 },
		.iface_index = 1,
	};
	struct smp_client_object obj;
	int rc;

	rc = smp_client_object_init(&obj, SMP_USER_DEFINED_TRANSPORT);
	zassert_equal(MGMT_ERR_EINVAL, rc, "expected init to fail for unknown transport: %d", rc);

	rc = smp_client_ethernet_set_dst(&obj, &addr);
	zassert_equal(-EINVAL, rc, "set_dst should reject a foreign transport: %d", rc);
}

/*
 * The transport stores the peer address in the net_buf user data so that the
 * reply leaves through the receiving interface back to the sender. Verify that
 * plumbing directly, without needing an Ethernet interface.
 */
ZTEST(transport_ethernet, test_user_data_address_plumbing)
{
	struct smp_transport *smpt = smp_client_transport_get(SMP_ETHERNET_TRANSPORT);
	struct smp_ethernet_addr src = {
		.mac = { 0x02, 0xaa, 0xbb, 0xcc, 0xdd, 0xee },
		.iface_index = 3,
	};
	struct smp_ethernet_addr *ud;
	struct net_buf *rx;
	struct net_buf *tx;

	zassert_not_null(smpt, NULL);
	zassert_not_null(smpt->functions.ud_init, NULL);
	zassert_not_null(smpt->functions.ud_copy, NULL);

	rx = smp_packet_alloc();
	tx = smp_packet_alloc();
	zassert_not_null(rx, NULL);
	zassert_not_null(tx, NULL);

	/* The receive path records the sender address on the request buffer. */
	smpt->functions.ud_init(rx, &src);
	ud = net_buf_user_data(rx);
	zassert_mem_equal(ud->mac, src.mac, sizeof(src.mac), NULL);
	zassert_equal(ud->iface_index, src.iface_index, NULL);

	/* Initialising a response buffer with no private data must be safe. */
	smpt->functions.ud_init(tx, NULL);

	/* The response buffer inherits the request's destination address. */
	zassert_equal(MGMT_ERR_EOK, smpt->functions.ud_copy(tx, rx), NULL);
	ud = net_buf_user_data(tx);
	zassert_mem_equal(ud->mac, src.mac, sizeof(src.mac), NULL);
	zassert_equal(ud->iface_index, src.iface_index, NULL);

	smp_packet_free(rx);
	smp_packet_free(tx);
}

/* A frame without padding is forwarded unchanged. */
ZTEST(transport_ethernet, test_recv_exact_frame)
{
	uint8_t payload[sizeof(struct smp_hdr) + 3];

	zassert_equal(0, smp_ethernet_open(), NULL);

	smp_req_hdr_build(payload, 3);
	payload[sizeof(struct smp_hdr)] = 0x01;
	payload[sizeof(struct smp_hdr) + 1] = 0x02;
	payload[sizeof(struct smp_hdr) + 2] = 0x03;

	zassert_equal(NET_OK, smp_ethernet_rx(payload, sizeof(payload)), NULL);
	rx_req_check(payload, sizeof(payload));
}

/* Short Ethernet frames are padded to the 60 byte minimum by the link; the
 * padding must be discarded whatever bytes it contains, including zeros.
 */
ZTEST(transport_ethernet, test_recv_discards_padding)
{
	const size_t frame_len = sizeof(struct smp_hdr) + 4;
	const uint8_t pad_fill[] = { 0x00, 0xff, 0x5a };
	uint8_t payload[46];

	zassert_equal(0, smp_ethernet_open(), NULL);

	for (size_t i = 0; i < ARRAY_SIZE(pad_fill); i++) {
		smp_req_hdr_build(payload, 4);
		payload[sizeof(struct smp_hdr)] = 0xde;
		payload[sizeof(struct smp_hdr) + 1] = 0xad;
		payload[sizeof(struct smp_hdr) + 2] = 0xbe;
		payload[sizeof(struct smp_hdr) + 3] = 0xef;
		memset(payload + frame_len, pad_fill[i], sizeof(payload) - frame_len);

		zassert_equal(NET_OK, smp_ethernet_rx(payload, sizeof(payload)), NULL);
		rx_req_check(payload, frame_len);
	}
}

/* A frame claiming more bytes than were received is passed on as is, so that
 * SMP can report it as corrupt instead of it being dropped silently.
 */
ZTEST(transport_ethernet, test_recv_truncated_frame)
{
	uint8_t payload[sizeof(struct smp_hdr) + 4] = { 0 };

	zassert_equal(0, smp_ethernet_open(), NULL);

	smp_req_hdr_build(payload, sizeof(payload) + 32);

	zassert_equal(NET_OK, smp_ethernet_rx(payload, sizeof(payload)), NULL);
	rx_req_check(payload, sizeof(payload));
}

/* A payload too short to hold a header is passed on for SMP to report. */
ZTEST(transport_ethernet, test_recv_short_payload)
{
	const uint8_t payload[] = { 0x08, 0x00, 0x00, 0x04 };

	zassert_equal(0, smp_ethernet_open(), NULL);

	zassert_equal(NET_OK, smp_ethernet_rx(payload, sizeof(payload)), NULL);
	rx_req_check(payload, sizeof(payload));
}

static void check_request_address(void)
{
	const struct smp_ethernet_addr *addr = net_buf_user_data(rx_req);

	zassert_mem_equal(addr->mac, peer_mac, sizeof(peer_mac), NULL);
	zassert_equal(addr->iface_index, 1, NULL);
}

ZTEST(transport_ethernet, test_recv_destination_classes)
{
	const uint8_t *destinations[] = { local_mac, multicast_mac, broadcast_mac, local_mac };
	const bool accepted[] = { true, false, false, true };
	uint8_t payload[sizeof(struct smp_hdr)];

	zassert_ok(smp_ethernet_open(), NULL);
	smp_req_hdr_build(payload, 0U);
	for (size_t i = 0U; i < ARRAY_SIZE(destinations); i++) {
		enum net_verdict verdict = smp_ethernet_rx_to(
			payload, sizeof(payload), destinations[i], NET_ETH_ADDR_LEN);

		zassert_equal(verdict, accepted[i] ? NET_OK : NET_DROP, "destination %zu", i);
		if (accepted[i]) {
			zassert_not_null(rx_req, NULL);
			check_request_address();
			rx_req_check(payload, sizeof(payload));
		} else {
			zassert_is_null(rx_req, NULL);
		}
	}
}

ZTEST(transport_ethernet, test_recv_other_multicast)
{
	const uint8_t other_group[NET_ETH_ADDR_LEN] = { 0x03, 0x00, 0x00, 0x00, 0x00, 0x02 };
	uint8_t payload[sizeof(struct smp_hdr)];

	zassert_ok(smp_ethernet_open(), NULL);
	smp_req_hdr_build(payload, 0U);
	zassert_equal(NET_DROP, smp_ethernet_rx_to(payload, sizeof(payload), other_group,
					    sizeof(other_group)), NULL);
	zassert_is_null(rx_req, NULL);
}

ZTEST(transport_ethernet, test_recv_destination_from_ethernet_header)
{
	const uint8_t *destinations[] = { local_mac, multicast_mac, broadcast_mac };
	const bool accepted[] = { true, false, false };
	struct net_if *iface = net_if_get_by_index(1);
	uint8_t payload[sizeof(struct smp_hdr)];

	zassert_ok(smp_ethernet_open(), NULL);
	smp_req_hdr_build(payload, 0U);
	for (size_t i = 0U; i < ARRAY_SIZE(destinations); i++) {
		enum net_verdict verdict = smp_ethernet_frame_rx(iface, payload, sizeof(payload),
							  destinations[i]);
		zassert_equal(verdict, accepted[i] ? NET_OK : NET_DROP, NULL);
		if (accepted[i]) {
			zassert_not_null(rx_req, NULL);
			check_request_address();
			rx_req_check(payload, sizeof(payload));
		} else {
			zassert_is_null(rx_req, NULL);
		}
	}
}

static uint32_t command_calls;

static int test_command(struct smp_streamer *streamer)
{
	ARG_UNUSED(streamer);

	command_calls++;

	return MGMT_ERR_EOK;
}

static const struct mgmt_handler test_handlers[] = {
	{ .mh_read = test_command, .mh_write = test_command },
};

static struct mgmt_group test_group = {
	.mg_handlers = test_handlers,
	.mg_handlers_count = ARRAY_SIZE(test_handlers),
	.mg_group_id = MGMT_GROUP_ID_PERUSER,
};

static void check_smp_command_and_reply(uint8_t operation)
{
	const uint8_t *destinations[] = { local_mac, multicast_mac, broadcast_mac };
	const bool accepted[] = { true, false, false };
	struct cbor_nb_reader reader;
	struct cbor_nb_writer writer;
	struct smp_streamer streamer = {
		.smpt = smp_client_transport_get(SMP_ETHERNET_TRANSPORT),
		.reader = &reader,
		.writer = &writer,
	};
	uint8_t payload[sizeof(struct smp_hdr) + 1U];
	struct smp_hdr *hdr = (struct smp_hdr *)payload;

	zassert_ok(smp_ethernet_open(), NULL);
	smp_req_hdr_build(payload, 1U);
	hdr->nh_op = operation;
	hdr->nh_group = sys_cpu_to_be16(MGMT_GROUP_ID_PERUSER);
	payload[sizeof(struct smp_hdr)] = 0xa0U;
	mgmt_register_group(&test_group);

	for (int index = 1; index <= 2; index++) {
		struct net_if *iface = net_if_get_by_index(index);
		int ret = net_if_up(iface);

		zassert_true(ret == 0 || ret == -EALREADY, "interface %d: %d", index, ret);
		net_if_carrier_on(iface);
		for (size_t i = 0U; i < ARRAY_SIZE(destinations); i++) {
			enum net_verdict verdict;

			command_calls = 0U;
			k_sem_reset(&tx_complete);
			verdict = smp_ethernet_frame_rx(iface, payload, sizeof(payload),
						 destinations[i]);
			zassert_equal(verdict, accepted[i] ? NET_OK : NET_DROP, NULL);
			if (accepted[i]) {
				struct net_buf *request = rx_req;

				zassert_not_null(request, NULL);
				rx_req = NULL;
				zassert_equal(smp_process_request_packet(&streamer, request),
					      MGMT_ERR_EOK, NULL);
				zassert_ok(k_sem_take(&tx_complete, K_SECONDS(1)), "no SMP reply");
				zassert_equal(command_calls, 1U, NULL);
				zassert_ok(transmitted.error, NULL);
				zassert_equal(transmitted.iface, iface, NULL);
				zassert_mem_equal(transmitted.eth.dst.addr, peer_mac,
						  sizeof(peer_mac), NULL);
				zassert_mem_equal(transmitted.eth.src.addr, local_mac,
						  sizeof(local_mac), NULL);
				zassert_equal(sys_be16_to_cpu(transmitted.eth.type),
					      CONFIG_MCUMGR_TRANSPORT_ETHERNET_ETHERTYPE, NULL);
				zassert_equal(transmitted.smp.nh_op, operation + 1U, NULL);
				zassert_equal(sys_be16_to_cpu(transmitted.smp.nh_group),
					      MGMT_GROUP_ID_PERUSER, NULL);
				zassert_equal(transmitted.smp.nh_id, hdr->nh_id, NULL);
				zassert_equal(transmitted.smp.nh_seq, hdr->nh_seq, NULL);
			} else {
				zassert_is_null(rx_req, NULL);
				zassert_equal(command_calls, 0U, NULL);
				zassert_equal(k_sem_count_get(&tx_complete), 0U, NULL);
			}
		}
	}
	mgmt_unregister_group(&test_group);
}

ZTEST(transport_ethernet, test_recv_invalid_destination)
{
	const uint8_t zero_mac[NET_ETH_ADDR_LEN] = { 0 };
	uint8_t payload[sizeof(struct smp_hdr)];

	zassert_ok(smp_ethernet_open(), NULL);
	smp_req_hdr_build(payload, 0U);
	zassert_equal(NET_DROP, smp_ethernet_rx_to(payload, sizeof(payload), NULL, 0U), NULL);
	zassert_equal(NET_DROP, smp_ethernet_rx_to(payload, sizeof(payload), local_mac, 5U), NULL);
	zassert_equal(NET_DROP, smp_ethernet_rx_to(payload, sizeof(payload), zero_mac,
					    sizeof(zero_mac)), NULL);
	zassert_is_null(rx_req, NULL);
}

ZTEST(transport_ethernet, test_smp_read_and_unicast_reply)
{
	check_smp_command_and_reply(MGMT_OP_READ);
}

ZTEST(transport_ethernet, test_smp_write_and_unicast_reply)
{
	check_smp_command_and_reply(MGMT_OP_WRITE);
}

ZTEST(transport_ethernet, test_recv_closed)
{
	uint8_t payload[sizeof(struct smp_hdr)];

	smp_req_hdr_build(payload, 0U);
	zassert_ok(smp_ethernet_close(), NULL);
	zassert_equal(NET_DROP, smp_ethernet_rx(payload, sizeof(payload)), NULL);
	zassert_is_null(rx_req, NULL);
	zassert_ok(smp_ethernet_open(), NULL);
}

ZTEST(transport_ethernet, test_tx_no_memory)
{
	struct smp_transport *smpt = smp_client_transport_get(SMP_ETHERNET_TRANSPORT);
	struct smp_ethernet_addr addr = { .iface_index = 1 };
	struct net_pkt *packets[CONFIG_NET_PKT_TX_COUNT];
	struct net_buf *nb = smp_packet_alloc();
	size_t count = 0U;
	uint8_t remaining_refs;
	int ret;

	zassert_not_null(nb, NULL);
	memcpy(addr.mac, peer_mac, sizeof(peer_mac));
	smpt->functions.ud_init(nb, &addr);
	net_buf_add_u8(nb, 0xa0U);
	/* Keep one reference to check that output consumes the supplied reference. */
	nb = net_buf_ref(nb);
	while (count < ARRAY_SIZE(packets)) {
		struct net_pkt *pkt = net_pkt_alloc(K_NO_WAIT);

		if (pkt == NULL) {
			break;
		}
		packets[count++] = pkt;
	}

	ret = smpt->functions.output(nb);
	remaining_refs = nb->ref;
	smp_packet_free(nb);
	for (size_t i = 0U; i < count; i++) {
		net_pkt_unref(packets[i]);
	}
	zassert_equal(ret, MGMT_ERR_ENOMEM, NULL);
	zassert_equal(remaining_refs, 1U, "output did not consume its buffer reference");
}

/* With the transport buffer pool exhausted a frame is dropped, not consumed,
 * so that the network stack releases the received packet.
 */
ZTEST(transport_ethernet, test_recv_no_buffer)
{
	struct net_buf *held[CONFIG_MCUMGR_TRANSPORT_NETBUF_COUNT];
	uint8_t payload[sizeof(struct smp_hdr) + 1];
	size_t count;

	zassert_equal(0, smp_ethernet_open(), NULL);

	smp_req_hdr_build(payload, 1);
	payload[sizeof(struct smp_hdr)] = 0x01;

	for (count = 0; count < ARRAY_SIZE(held); count++) {
		held[count] = smp_packet_alloc();
		zassert_not_null(held[count], "pool ran out after %zu buffers", count);
	}

	/* The whole pool is taken, so the next receive cannot get a buffer. */
	rx_req = NULL;
	zassert_equal(NET_DROP, smp_ethernet_rx(payload, sizeof(payload)), NULL);
	zassert_is_null(rx_req, "a frame must not reach SMP without a buffer");

	while (count > 0) {
		smp_packet_free(held[--count]);
	}
}

ZTEST(transport_ethernet, test_tx_exceeds_interface_mtu)
{
	struct smp_transport *smpt = smp_client_transport_get(SMP_ETHERNET_TRANSPORT);
	struct net_if *iface = net_if_get_by_index(1);
	struct smp_ethernet_addr addr = { .iface_index = 1 };
	struct net_buf *nb = smp_packet_alloc();
	uint16_t mtu = net_if_get_mtu(iface);
	int ret;

	zassert_not_null(nb, NULL);
	memcpy(addr.mac, peer_mac, sizeof(peer_mac));
	smpt->functions.ud_init(nb, &addr);
	net_buf_add(nb, sizeof(struct smp_hdr) + 1U);
	net_if_set_mtu(iface, sizeof(struct smp_hdr));
	ret = smpt->functions.output(nb);
	net_if_set_mtu(iface, mtu);
	zassert_equal(ret, MGMT_ERR_EMSGSIZE, NULL);
}

ZTEST_SUITE(transport_ethernet, NULL, NULL, NULL, NULL, NULL);
