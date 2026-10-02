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
static enum net_verdict smp_ethernet_rx(const uint8_t *payload, size_t payload_len)
{
	static uint8_t src_mac[NET_ETH_ADDR_LEN] = {0x02, 0xaa, 0xbb, 0xcc, 0xdd, 0xee};
	enum net_verdict (*handler)(struct net_if *iface, uint16_t ptype, struct net_pkt *pkt) =
		NULL;
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

	zassert_ok(net_linkaddr_set(net_pkt_lladdr_src(pkt), src_mac, sizeof(src_mac)),
		   "failed to set the source address");

	verdict = handler(NULL, CONFIG_MCUMGR_TRANSPORT_ETHERNET_ETHERTYPE, pkt);

	/* The handler releases the packet only when it consumes it. */
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
		.mac = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01},
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
		.mac = {0x02, 0x00, 0x00, 0x00, 0x00, 0x02},
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
		.mac = {0x02, 0xaa, 0xbb, 0xcc, 0xdd, 0xee},
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
	const uint8_t pad_fill[] = {0x00, 0xff, 0x5a};
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
	uint8_t payload[sizeof(struct smp_hdr) + 4] = {0};

	zassert_equal(0, smp_ethernet_open(), NULL);

	smp_req_hdr_build(payload, sizeof(payload) + 32);

	zassert_equal(NET_OK, smp_ethernet_rx(payload, sizeof(payload)), NULL);
	rx_req_check(payload, sizeof(payload));
}

/* A payload too short to hold a header is passed on for SMP to report. */
ZTEST(transport_ethernet, test_recv_short_payload)
{
	const uint8_t payload[] = {0x08, 0x00, 0x00, 0x04};

	zassert_equal(0, smp_ethernet_open(), NULL);

	zassert_equal(NET_OK, smp_ethernet_rx(payload, sizeof(payload)), NULL);
	rx_req_check(payload, sizeof(payload));
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

ZTEST_SUITE(transport_ethernet, NULL, NULL, NULL, NULL, NULL);
