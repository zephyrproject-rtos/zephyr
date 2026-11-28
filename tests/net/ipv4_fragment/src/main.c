/*
 * Copyright (c) 2022 Jamie McCrae
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ipv4_test, CONFIG_NET_IPV4_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/linker/sections.h>
#include <zephyr/random/random.h>
#include <zephyr/ztest.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net_buf.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/byteorder.h>
#include <net_private.h>
#include <ipv4.h>
#include <udp_internal.h>
#include <tcp_internal.h>

/* Packet size for tests, excluding headers */
#define IPV4_TEST_PACKET_SIZE 2048

/* Wait times for semaphores and buffers */
#define WAIT_TIME K_MSEC(1100)
#define ALLOC_TIMEOUT K_MSEC(500)

/* Reassembly error-path test packet parameters */
#define REASS_ERR_FRAG_PAYLOAD  16
#define REASS_ERR_UDP_DGRAM_LEN (REASS_ERR_FRAG_PAYLOAD * 3)
#define REASS_ERR_FRAG_ID       0xA55A

/* Dummy network addresses, 192.168.8.1 and 192.168.8.2 */
static struct net_in_addr my_addr1 = { { { 0xc0, 0xa8, 0x08, 0x01 } } };
static struct net_in_addr my_addr2 = { { { 0xc0, 0xa8, 0x08, 0x02 } } };

/* Reassembly error-path test addresses, 192.0.2.1 and 192.0.2.2 */
static struct net_in_addr reass_err_src_addr = { { { 0xc0, 0x00, 0x02, 0x01 } } };
static struct net_in_addr reass_err_dst_addr = { { { 0xc0, 0x00, 0x02, 0x02 } } };

/* IPv4 TCP packet header */
static const unsigned char ipv4_tcp[] = {
	/* IPv4 header */
	0x45, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x80, 0x06, 0x00, 0x00,
	0xc0, 0xa8, 0x08, 0x01,
	0xc0, 0xa8, 0x08, 0x02,

	/* TCP header */
	0x0f, 0xfc, 0x4c, 0x5f,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x50, 0x10, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

/* IPv4 UDP packet header */
static const unsigned char ipv4_udp[] = {
	/* IPv4 header */
	0x45, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x80, 0x11, 0x00, 0x00,
	0xc0, 0xa8, 0x08, 0x01,
	0xc0, 0xa8, 0x08, 0x02,

	/* UDP header */
	0x11, 0x00, 0x63, 0x04,
	0x00, 0x00, 0x00, 0x00,
};

/* IPv4 UDP Single fragment packet (with more fragment bit set) */
static const unsigned char ipv4_udp_frag[] = {
	/* IPv4 header */
	0x45, 0x00, 0x00, 0x24,
	0x12, 0x34, 0x20, 0x00,
	0x80, 0x11, 0x00, 0x00,
	0xc0, 0xa8, 0x08, 0x02,
	0xc0, 0xa8, 0x08, 0x01,

	/* UDP header */
	0x11, 0x00, 0x63, 0x04,
	0x00, 0x80, 0x00, 0x00,

	/* UDP data */
	0xaa, 0xbb, 0xcc, 0xdd,
	0xee, 0xff, 0x94, 0x12,
};

/* IPv4 ICMP fragment assembly time exceeded packet (in response to ipv4_udp_frag) */
static const unsigned char ipv4_icmp_reassembly_time[] = {
	/* IPv4 Header */
	0x45, 0x00, 0x00, 0x38,
	0x00, 0x00, 0x00, 0x00,
	0x40, 0x01, 0xe9, 0x71,
	0xc0, 0xa8, 0x08, 0x01,
	0xc0, 0xa8, 0x08, 0x02,

	/* ICMPv4 fragment assembly time exceeded */
	0x0b, 0x01, 0x80, 0x7a,
	0x00, 0x00, 0x00, 0x00,

	/* Original IPv4 packet data */
	0x45, 0x00, 0x00, 0x24,
	0x12, 0x34, 0x20, 0x00,
	0x80, 0x11, 0x77, 0x41,
	0xc0, 0xa8, 0x08, 0x02,
	0xc0, 0xa8, 0x08, 0x01,
	0x11, 0x00, 0x63, 0x04,
	0x00, 0x80, 0x00, 0x00,
};

/* IPv4 UDP packet header with do not fragment flag set */
static const unsigned char ipv4_udp_do_not_frag[] = {
	/* IPv4 header */
	0x45, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x40, 0x00,
	0x80, 0x11, 0x00, 0x00,
	0xc0, 0xa8, 0x08, 0x01,
	0xc0, 0xa8, 0x08, 0x02,

	/* UDP header */
	0x11, 0x00, 0x63, 0x04,
	0x00, 0x00, 0x00, 0x00,
};

/* IPv4 header options: Router Alert, copied into every fragment, and
 * Record Route, only in the first one, padded with End of Option List.
 */
static const unsigned char ipv4_opts[] = {
	0x94, 0x04, 0x00, 0x00,
	0x07, 0x07, 0x04, 0x00, 0x00, 0x00, 0x00,
	0x00,
};

/* Options carried by the fragments after the first one */
static const unsigned char ipv4_opts_copied[] = {
	0x94, 0x04, 0x00, 0x00,
};

/* Record Route, only in the first fragment, and Loose Source and Record
 * Route with one address, copied into every fragment. LSRR is 7 bytes,
 * so the copied options need one byte of End of Option List padding.
 */
static const unsigned char ipv4_opts_lsrr[] = {
	0x07, 0x07, 0x04, 0x00, 0x00, 0x00, 0x00,
	0x83, 0x07, 0x04, 0xc0, 0xa8, 0x08, 0x02,
	0x00, 0x00,
};

static const unsigned char ipv4_opts_lsrr_copied[] = {
	0x83, 0x07, 0x04, 0xc0, 0xa8, 0x08, 0x02,
	0x00,
};

#define IPV4_OPTS_HDR_LEN (NET_IPV4H_LEN + sizeof(ipv4_opts))
#define IPV4_OPTS_SRC_PORT 4353
#define IPV4_OPTS_DST_PORT 25349

enum {
	TEST_UDP,
	TEST_TCP,
	TEST_SINGLE_FRAGMENT,
	TEST_NO_FRAGMENT,
	TEST_UDP_OPTIONS,
	TEST_OPTIONS_WIRE,
};

/* Options expected in the first fragment and in the later ones */
static const uint8_t *exp_opts;
static size_t exp_opts_len;
static const uint8_t *exp_opts_copied;
static size_t exp_opts_copied_len;

/* Payload of the fragments sent in a TEST_OPTIONS_WIRE test, at its
 * fragment offset.
 */
static uint8_t wire_payload[IPV4_TEST_PACKET_SIZE + 64];

/* The payload as it was before the fragmentation, with a zero L4 checksum */
static uint8_t wire_expected[IPV4_TEST_PACKET_SIZE + 64];

static struct net_if *iface1;

static struct k_sem wait_data;
static struct k_sem wait_received_data;

static bool test_started;
static uint16_t pkt_id;
static uint16_t pkt_recv_size;
static uint16_t pkt_recv_expected_size;
static bool last_packet_received;
static uint8_t active_test;
static uint8_t lower_layer_packet_count;
static uint8_t upper_layer_packet_count;
static uint16_t lower_layer_total_size;
static uint16_t upper_layer_total_size;

static uint8_t test_tmp_buf[256];
static uint8_t net_iface_dummy_data;

static void net_iface_init(struct net_if *iface);
static int sender_iface(const struct device *dev, struct net_pkt *pkt);
static int reass_err_rx_pkt_free_count(void);
static void reass_err_ipv4_hdr_init(struct net_ipv4_hdr *hdr, uint16_t total_len,
				    uint16_t offset_bytes, bool more);
static void reass_err_fault_inject_noncontiguous_ipv4_hdr(struct net_pkt *pkt);
static struct net_pkt *reass_err_make_udp_fragment(struct net_if *iface,
						   const uint8_t *udp_dgram,
						   uint16_t offset, uint16_t payload_len,
						   bool more);

static DEVICE_API(dummy, net_iface_api) = {
	.iface_api.init = net_iface_init,
	.send = sender_iface,
};

NET_DEVICE_INIT_INSTANCE(net_iface1_test,
			 "iface1",
			 iface1,
			 NULL,
			 NULL,
			 &net_iface_dummy_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 DUMMY_L2,
			 NET_L2_GET_CTX_TYPE(DUMMY_L2),
			 NET_IPV4_MTU);


static void net_iface_init(struct net_if *iface)
{
	static uint8_t mac[6] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 };

	net_if_set_link_addr(iface, mac, sizeof(mac), NET_LINK_DUMMY);
}

/* Generates dummy data to use in tests */
static void generate_dummy_data(uint8_t *buffer, uint16_t buffer_size)
{
	uint16_t i = 0;

	while (i < buffer_size) {
		buffer[i] = (uint8_t)(i & 0xff);
		++i;
	}
}

/* Callback function for processing all reassembly buffers */
static void reassembly_foreach_cb(struct net_ipv4_reassembly *reassembly, void *data)
{
	uint8_t *packets = (uint8_t *)data;
	++*packets;
}

/* Reassembly timeout-vs-completion race test state */
static K_SEM_DEFINE(race_reached, 0, 1);
static K_SEM_DEFINE(race_resume, 0, 1);
static K_THREAD_STACK_DEFINE(race_worker_stack, 2048);
static struct k_thread race_worker;
static struct net_pkt *race_final_frag;
static struct net_ipv4_hdr race_final_hdr;
static struct k_work_delayable *race_timer;
static bool race_armed;

/* CONFIG_TRACING_USER hook run on entry to k_work_cancel_delayable(), before
 * the work lock is taken. Reassembly completion cancels the slot timer first;
 * when armed for that timer, signal the test and block so the slot timeout
 * can fire against the same slot. One-shot, so the timeout handler's own
 * cancel passes straight through.
 */
void sys_trace_k_work_cancel_delayable_enter_user(struct k_work_delayable *dwork)
{
	if (race_armed && dwork == race_timer) {
		race_armed = false;
		k_sem_give(&race_reached);
		k_sem_take(&race_resume, K_FOREVER);
	}
}

/* Records the timer of the only active reassembly slot. */
static void race_timer_cb(struct net_ipv4_reassembly *reassembly, void *data)
{
	ARG_UNUSED(data);

	race_timer = &reassembly->timer;
}

/* Delivers the final fragment from its own thread so the main thread stays free
 * to advance time while the completion path is paused in the hook.
 */
static void race_worker_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	(void)net_ipv4_handle_fragment_hdr(race_final_frag, &race_final_hdr);
}

/* Ones' complement sum of data, added to sum */
static uint16_t inet_sum(uint32_t sum, const uint8_t *data, size_t len)
{
	size_t i;

	for (i = 0; i + 1U < len; i += 2U) {
		sum += sys_get_be16(&data[i]);
	}

	if (i < len) {
		sum += (uint32_t)data[i] << 8;
	}

	while (sum > 0xffffU) {
		sum = (sum & 0xffffU) + (sum >> 16);
	}

	return sum;
}

static uint16_t ipv4_hdr_sum(const uint8_t *hdr, size_t len)
{
	return inet_sum(0U, hdr, len);
}

/* Checks a fragment of a datagram that carries exp_opts */
static void check_ipv4_options_fragment(struct net_pkt *pkt, uint16_t current_length,
					bool final)
{
	uint8_t hdr[NET_IPV4H_LEN + NET_IPV4_HDR_OPTNS_MAX_LEN];
	const struct net_ipv4_hdr *ipv4 = (const struct net_ipv4_hdr *)hdr;
	uint16_t offset;
	size_t hdr_len;

	net_pkt_cursor_init(pkt);
	zassert_ok(net_pkt_read(pkt, hdr, NET_IPV4H_LEN));

	hdr_len = (ipv4->vhl & NET_IPV4_IHL_MASK) * 4U;
	offset = sys_get_be16(ipv4->offset);

	zassert_equal((offset & NET_IPV4_FRAGH_OFFSET_MASK) * 8U, current_length,
		      "IPv4 fragment offset mismatch");
	zassert_equal((offset & NET_IPV4_MORE_FRAG_MASK) == 0U, final,
		      "IPv4 more fragments flag mismatch");
	zassert_equal(net_ntohs(ipv4->len), net_pkt_get_len(pkt),
		      "IPv4 header length mismatch");

	if (current_length == 0U) {
		zassert_equal(hdr_len, NET_IPV4H_LEN + exp_opts_len, "First fragment IHL mismatch");
		zassert_ok(net_pkt_read(pkt, hdr + NET_IPV4H_LEN, exp_opts_len));
		zassert_mem_equal(hdr + NET_IPV4H_LEN, exp_opts, exp_opts_len,
				  "First fragment options mismatch");
	} else {
		zassert_equal(hdr_len, NET_IPV4H_LEN + exp_opts_copied_len,
			      "Fragment IHL mismatch");
		zassert_ok(net_pkt_read(pkt, hdr + NET_IPV4H_LEN, exp_opts_copied_len));
		zassert_mem_equal(hdr + NET_IPV4H_LEN, exp_opts_copied, exp_opts_copied_len,
				  "Fragment options mismatch");
	}

	zassert_equal(ipv4_hdr_sum(hdr, hdr_len), 0xffffU, "IPv4 header checksum mismatch");

	net_pkt_cursor_init(pkt);
}

/* Checks all IPv4 headers against expected values */
static void check_ipv4_fragment_header(struct net_pkt *pkt, const uint8_t *orig_hdr, uint16_t id,
				       uint16_t current_length, bool final)
{
	uint16_t pkt_len;
	uint16_t pkt_offset;
	uint8_t pkt_flags;
	const struct net_ipv4_hdr *hdr = NET_IPV4_HDR(pkt);
	uint16_t chksum = 0;
	int ret;

	zassert_equal(hdr->vhl, orig_hdr[offsetof(struct net_ipv4_hdr, vhl)],
		      "IPv4 header vhl mismatch");
	zassert_equal(hdr->tos, orig_hdr[offsetof(struct net_ipv4_hdr, tos)],
		      "IPv4 header tos mismatch");
	zassert_equal(hdr->ttl, orig_hdr[offsetof(struct net_ipv4_hdr, ttl)],
		      "IPv4 header ttl mismatch");
	zassert_equal(hdr->proto, orig_hdr[offsetof(struct net_ipv4_hdr, proto)],
		      "IPv4 header protocol mismatch");
	zassert_equal(*((uint16_t *)&hdr->id), pkt_id, "IPv4 header ID mismatch");
	zassert_mem_equal(hdr->src, &orig_hdr[offsetof(struct net_ipv4_hdr, src)],
			  NET_IPV4_ADDR_SIZE, "IPv4 header source IP mismatch");
	zassert_mem_equal(hdr->dst, &orig_hdr[offsetof(struct net_ipv4_hdr, dst)],
			  NET_IPV4_ADDR_SIZE, "IPv4 header destination IP mismatch");

	pkt_len = net_ntohs(hdr->len);
	pkt_offset = net_ntohs(*((uint16_t *)&hdr->offset));
	pkt_flags = (pkt_offset & ~NET_IPV4_FRAGH_OFFSET_MASK) >> 13;
	pkt_offset &= NET_IPV4_FRAGH_OFFSET_MASK;
	pkt_offset *= 8;

	if (final) {
		zassert_equal(pkt_flags, 0, "IPv4 header fragment flags mismatch");
	} else {
		zassert_equal(pkt_flags, BIT(0), "IPv4 header fragment flags mismatch");
	}

	zassert_equal(net_pkt_get_len(pkt), pkt_len, "IPv4 header length mismatch");
	zassert_equal(pkt_offset, current_length, "IPv4 header length mismatch");

	ret = net_calc_chksum_ipv4(pkt, &chksum);
	zassert_equal(ret, 0, "Calculation failed");
	zassert_equal(chksum, 0, "IPv4 header checksum mismatch");
}

static int sender_iface(const struct device *dev, struct net_pkt *pkt)
{
	struct net_pkt *recv_pkt;
	int ret;
	uint8_t buf_ip_src[4];
	uint8_t buf_ip_dst[4];
	uint16_t buf_port_src;
	uint16_t buf_port_dst;
	uint16_t offset;
	uint16_t hdr_len;

	if (!pkt->buffer) {
		LOG_ERR("No data to send!");
		return -ENODATA;
	}

	if (net_pkt_get_len(pkt) > NET_IPV4_MTU) {
		LOG_DBG("Too large for test");
		pkt_recv_size = net_pkt_get_len(pkt);
		return -EMSGSIZE;
	}

	if (test_started) {
		++lower_layer_packet_count;
		lower_layer_total_size += net_pkt_get_len(pkt);

		/* Verify the fragments */
		bool last_packet;

		net_pkt_cursor_init(pkt);

		if (lower_layer_packet_count == 1 && pkt_id == 0) {
			/* Extract ID from first packet */
			pkt_id = *((uint16_t *)&NET_IPV4_HDR(pkt)->id);

			/* Check ID is 0 for non-fragmented packets and non-0 for fragmented
			 * packets
			 */
			if (active_test == TEST_UDP || active_test == TEST_TCP ||
			    active_test == TEST_UDP_OPTIONS || active_test == TEST_OPTIONS_WIRE) {
				zassert_not_equal(pkt_id, 0, "IPv4 header ID should not be 0");
			} else if (active_test == TEST_SINGLE_FRAGMENT) {
				zassert_equal(pkt_id, 0, "IPv4 header ID should be 0");
			}
		}

		hdr_len = (NET_IPV4_HDR(pkt)->vhl & NET_IPV4_IHL_MASK) * 4U;

		if (active_test == TEST_UDP_OPTIONS || active_test == TEST_OPTIONS_WIRE) {
			/* The expected size counts the payload only */
			last_packet = pkt_recv_size + net_pkt_get_len(pkt) - hdr_len >=
				      pkt_recv_expected_size;
			check_ipv4_options_fragment(pkt, pkt_recv_size, last_packet);
		} else {
			last_packet = ((pkt_recv_size + net_pkt_get_len(pkt)) >=
				       pkt_recv_expected_size ? true : false);
			check_ipv4_fragment_header(pkt, (active_test == TEST_UDP ? ipv4_udp :
						   (active_test == TEST_TCP ? ipv4_tcp :
						   ipv4_icmp_reassembly_time)), pkt_id,
						   pkt_recv_size, last_packet);
		}

		if (active_test == TEST_OPTIONS_WIRE) {
			size_t len = net_pkt_get_len(pkt) - hdr_len;

			/* Keep the payload to check the L4 checksum over the
			 * whole datagram, and do not loop the fragment back.
			 */
			zassert_true(pkt_recv_size + len <= sizeof(wire_payload),
				     "Fragment past the end of the datagram");
			net_pkt_cursor_init(pkt);
			net_pkt_set_overwrite(pkt, true);
			zassert_ok(net_pkt_skip(pkt, hdr_len));
			zassert_ok(net_pkt_read(pkt, &wire_payload[pkt_recv_size], len));
			net_pkt_cursor_init(pkt);
		}

		pkt_recv_size += net_pkt_get_len(pkt) - hdr_len;

		if (last_packet) {
			last_packet_received = true;
		}

		if (active_test == TEST_OPTIONS_WIRE) {
			goto no_duplicate;
		}

		if (active_test == TEST_SINGLE_FRAGMENT) {
			/* Verify that this is an ICMPv4 response */
			zassert_mem_equal(ipv4_icmp_reassembly_time, pkt->buffer->data,
					  sizeof(ipv4_icmp_reassembly_time), "Expected ICMP error");
			goto no_duplicate;
		}

		/* Duplicate the packet and flip the source/destination IPs and ports then
		 * re-insert the packets back into the same interface
		 */
		recv_pkt = net_pkt_rx_clone(pkt, K_NO_WAIT);
		net_pkt_set_overwrite(recv_pkt, true);

		/* Read IPs */
		net_pkt_cursor_init(recv_pkt);
		net_pkt_skip(recv_pkt, 12);
		net_pkt_read(recv_pkt, buf_ip_src, sizeof(buf_ip_src));
		net_pkt_read(recv_pkt, buf_ip_dst, sizeof(buf_ip_dst));

		/* Write opposite IPs */
		net_pkt_cursor_init(recv_pkt);
		net_pkt_skip(recv_pkt, 12);
		net_pkt_write(recv_pkt, buf_ip_dst, sizeof(buf_ip_dst));
		net_pkt_write(recv_pkt, buf_ip_src, sizeof(buf_ip_src));

		/* Get offset to check if this is the first packet */
		net_pkt_cursor_init(recv_pkt);
		net_pkt_skip(recv_pkt, 6);
		net_pkt_read_be16(recv_pkt, &offset);
		offset &= NET_IPV4_FRAGH_OFFSET_MASK;

		if (offset == 0) {
			/* Offset is 0, flip ports of packet */
			net_pkt_cursor_init(recv_pkt);
			net_pkt_skip(recv_pkt, hdr_len);
			net_pkt_read_be16(recv_pkt, &buf_port_src);
			net_pkt_read_be16(recv_pkt, &buf_port_dst);

			net_pkt_cursor_init(recv_pkt);
			net_pkt_skip(recv_pkt, hdr_len);
			net_pkt_write_be16(recv_pkt, buf_port_dst);
			net_pkt_write_be16(recv_pkt, buf_port_src);
		}

		/* Reset position to beginning and ready packet for insertion */
		net_pkt_cursor_init(recv_pkt);
		net_pkt_set_overwrite(recv_pkt, false);
		net_pkt_set_iface(recv_pkt, iface1);
		ret = net_recv_data(net_pkt_iface(recv_pkt), recv_pkt);
		zassert_equal(ret, 0, "Cannot receive data (%d)", ret);
		k_sleep(K_MSEC(10));

no_duplicate:
		k_sem_give(&wait_data);

	}

	return 0;
}


static enum net_verdict udp_data_received(struct net_conn *conn, struct net_pkt *pkt,
					  union net_ip_header *ip_hdr,
					  union net_proto_header *proto_hdr, void *user_data)
{
	const struct net_ipv4_hdr *hdr = NET_IPV4_HDR(pkt);
	uint8_t tmp_buf[256];
	uint8_t verify_buf[256];
	uint16_t i;
	uint16_t pkt_len;
	uint16_t pkt_offset;
	uint8_t pkt_flags;
	uint16_t udp_src_port;
	uint16_t udp_dst_port;
	uint16_t udp_len;
	uint16_t udp_checksum;
	int ret_chksum;
	uint16_t chksum = 0;

	/* Update counts */
	++upper_layer_packet_count;
	upper_layer_total_size += net_pkt_get_len(pkt);

	/* Generate dummy data to verify */
	generate_dummy_data(tmp_buf, sizeof(tmp_buf));

	/* Verify IPv4 header is valid */
	zassert_equal(hdr->vhl, ipv4_udp[offsetof(struct net_ipv4_hdr, vhl)],
		      "IPv4 header vhl mismatch");
	zassert_equal(hdr->tos, ipv4_udp[offsetof(struct net_ipv4_hdr, tos)],
		      "IPv4 header tos mismatch");
	zassert_equal(hdr->ttl, ipv4_udp[offsetof(struct net_ipv4_hdr, ttl)],
		      "IPv4 header ttl mismatch");
	zassert_equal(hdr->proto, ipv4_udp[offsetof(struct net_ipv4_hdr, proto)],
		      "IPv4 header protocol mismatch");
	zassert_equal(*((uint16_t *)&hdr->id), pkt_id, "IPv4 header ID mismatch");
	zassert_mem_equal(hdr->src, &ipv4_udp[offsetof(struct net_ipv4_hdr, dst)],
			  NET_IPV4_ADDR_SIZE, "IPv4 header source IP mismatch");
	zassert_mem_equal(hdr->dst, &ipv4_udp[offsetof(struct net_ipv4_hdr, src)],
			  NET_IPV4_ADDR_SIZE, "IPv4 header destination IP mismatch");

	pkt_len = net_ntohs(hdr->len);
	pkt_offset = net_ntohs(*((uint16_t *)&hdr->offset));
	pkt_flags = (pkt_offset & ~NET_IPV4_FRAGH_OFFSET_MASK) >> 13;
	pkt_offset &= NET_IPV4_FRAGH_OFFSET_MASK;
	pkt_offset *= 8;

	zassert_equal(pkt_flags, 0, "IPv4 header fragment flags mismatch");
	zassert_equal(net_pkt_get_len(pkt), pkt_len, "IPv4 header length mismatch");
	zassert_equal(pkt_offset, 0, "IPv4 header length mismatch");

	ret_chksum = net_calc_chksum_ipv4(pkt, &chksum);
	zassert_equal(ret_chksum, 0, "Calculation failed");
	zassert_equal(chksum, 0, "IPv4 header checksum mismatch");

	/* Verify IPv4 UDP header is valid */
	net_pkt_cursor_init(pkt);
	net_pkt_read(pkt, verify_buf, NET_IPV4H_LEN);

	net_pkt_read_be16(pkt, &udp_src_port);
	net_pkt_read_be16(pkt, &udp_dst_port);
	net_pkt_read_be16(pkt, &udp_len);
	net_pkt_read_be16(pkt, &udp_checksum);

	/* Verify IPv4 UDP data is valid */
	i = NET_IPV4H_LEN;
	while (i < net_pkt_get_len(pkt)) {
		net_pkt_read(pkt, verify_buf, sizeof(verify_buf));

		zassert_mem_equal(tmp_buf, verify_buf, sizeof(verify_buf),
				  "IPv4 data verification failure");
		i += sizeof(verify_buf);
	}

	LOG_DBG("Data %p received", pkt);

	net_pkt_unref(pkt);

	k_sem_give(&wait_received_data);

	return NET_OK;
}

static enum net_verdict tcp_data_received(struct net_conn *conn, struct net_pkt *pkt,
					  union net_ip_header *ip_hdr,
					  union net_proto_header *proto_hdr, void *user_data)
{
	const struct net_ipv4_hdr *hdr = NET_IPV4_HDR(pkt);
	uint8_t tmp_buf[256];
	uint8_t verify_buf[256];
	uint16_t i;
	uint16_t pkt_len;
	uint16_t pkt_offset;
	uint8_t pkt_flags;
	uint16_t tcp_src_port;
	uint16_t tcp_dst_port;
	uint32_t tcp_sequence;
	uint32_t tcp_acknowledgment;
	uint16_t tcp_flags;
	uint16_t tcp_window_size;
	uint16_t tcp_checksum;
	uint16_t tcp_urgent;
	int ret_chksum;
	uint16_t chksum = 0;

	/* Update counts */
	++upper_layer_packet_count;
	upper_layer_total_size += net_pkt_get_len(pkt);

	/* Generate dummy data to verify */
	generate_dummy_data(tmp_buf, sizeof(tmp_buf));

	/* Verify IPv4 header is valid */
	zassert_equal(hdr->vhl, ipv4_tcp[offsetof(struct net_ipv4_hdr, vhl)],
		      "IPv4 header vhl mismatch");
	zassert_equal(hdr->tos, ipv4_tcp[offsetof(struct net_ipv4_hdr, tos)],
		      "IPv4 header tos mismatch");
	zassert_equal(hdr->ttl, ipv4_tcp[offsetof(struct net_ipv4_hdr, ttl)],
		      "IPv4 header ttl mismatch");
	zassert_equal(hdr->proto, ipv4_tcp[offsetof(struct net_ipv4_hdr, proto)],
		      "IPv4 header protocol mismatch");
	zassert_equal(*((uint16_t *)&hdr->id), pkt_id, "IPv4 header ID mismatch");
	zassert_mem_equal(hdr->src, &ipv4_tcp[offsetof(struct net_ipv4_hdr, dst)],
			  NET_IPV4_ADDR_SIZE, "IPv4 header source IP mismatch");
	zassert_mem_equal(hdr->dst, &ipv4_tcp[offsetof(struct net_ipv4_hdr, src)],
			  NET_IPV4_ADDR_SIZE, "IPv4 header destination IP mismatch");

	pkt_len = net_ntohs(hdr->len);
	pkt_offset = net_ntohs(*((uint16_t *)&hdr->offset));
	pkt_flags = (pkt_offset & ~NET_IPV4_FRAGH_OFFSET_MASK) >> 13;
	pkt_offset &= NET_IPV4_FRAGH_OFFSET_MASK;
	pkt_offset *= 8;

	zassert_equal(pkt_flags, 0, "IPv4 header fragment flags mismatch");
	zassert_equal(net_pkt_get_len(pkt), pkt_len, "IPv4 header length mismatch");
	zassert_equal(pkt_offset, 0, "IPv4 header length mismatch");

	ret_chksum = net_calc_chksum_ipv4(pkt, &chksum);
	zassert_equal(ret_chksum, 0, "Calculation failed");
	zassert_equal(chksum, 0, "IPv4 header checksum mismatch");

	/* Verify IPv4 UDP header is valid */
	net_pkt_cursor_init(pkt);
	net_pkt_read(pkt, verify_buf, NET_IPV4H_LEN);

	net_pkt_read_be16(pkt, &tcp_src_port);
	net_pkt_read_be16(pkt, &tcp_dst_port);
	net_pkt_read_be32(pkt, &tcp_sequence);
	net_pkt_read_be32(pkt, &tcp_acknowledgment);
	net_pkt_read_be16(pkt, &tcp_flags);
	net_pkt_read_be16(pkt, &tcp_window_size);
	net_pkt_read_be16(pkt, &tcp_checksum);
	net_pkt_read_be16(pkt, &tcp_urgent);

	zassert_equal(tcp_src_port, 19551, "IPv4 TCP source port verification failure");
	zassert_equal(tcp_dst_port, 4092, "IPv4 TCP destination port verification failure");
	zassert_equal(tcp_sequence, 0, "IPv4 TCP sequence verification failure");
	zassert_equal(tcp_acknowledgment, 0, "IPv4 TCP acknowledgment verification failure");
	zassert_equal(tcp_flags, 0x5010, "IPv4 TCP flags verification failure");
	zassert_equal(tcp_window_size, 0, "IPv4 TCP window size verification failure");
	zassert_equal(tcp_urgent, 0, "IPv4 TCP urgent verification failure");

	/* Verify IPv4 TCP data is valid */
	i = NET_IPV4H_LEN;
	while (i < net_pkt_get_len(pkt)) {
		net_pkt_read(pkt, verify_buf, sizeof(verify_buf));

		zassert_mem_equal(tmp_buf, verify_buf, sizeof(verify_buf),
				  "IPv4 data verification failure");
		i += sizeof(verify_buf);
	}

	LOG_DBG("Data %p received", pkt);

	net_pkt_unref(pkt);

	k_sem_give(&wait_received_data);

	return NET_OK;
}

/* Receives the reassembled datagrams that carry ipv4_opts */
static enum net_verdict udp_opts_data_received(struct net_conn *conn, struct net_pkt *pkt,
					       union net_ip_header *ip_hdr,
					       union net_proto_header *proto_hdr,
					       void *user_data)
{
	const struct net_ipv4_hdr *hdr = NET_IPV4_HDR(pkt);
	uint8_t opts[sizeof(ipv4_opts)];
	uint8_t data[sizeof(test_tmp_buf)];
	struct net_udp_hdr udp;
	size_t data_len;
	size_t i;

	++upper_layer_packet_count;
	upper_layer_total_size += net_pkt_get_len(pkt);

	zassert_equal(hdr->vhl, 0x40 | (IPV4_OPTS_HDR_LEN / 4U), "IPv4 header vhl mismatch");
	zassert_equal(net_ntohs(hdr->len), net_pkt_get_len(pkt), "IPv4 header length mismatch");
	zassert_equal(sys_get_be16(hdr->offset), 0U, "IPv4 fragment offset mismatch");

	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);
	zassert_ok(net_pkt_skip(pkt, NET_IPV4H_LEN));
	zassert_ok(net_pkt_read(pkt, opts, sizeof(opts)));
	zassert_mem_equal(opts, ipv4_opts, sizeof(ipv4_opts), "IPv4 header options mismatch");

	/* A reassembled datagram had its UDP checksum verified, unless the
	 * checksum is missing.
	 */
	zassert_ok(net_pkt_read(pkt, &udp, sizeof(udp)));
	zassert_not_equal(udp.chksum, 0U, "UDP checksum missing");

	/* The UDP data repeats test_tmp_buf */
	data_len = net_pkt_get_len(pkt) - IPV4_OPTS_HDR_LEN - NET_UDPH_LEN;

	for (i = 0; i < data_len; i += sizeof(data)) {
		size_t len = MIN(sizeof(data), data_len - i);

		zassert_ok(net_pkt_read(pkt, data, len));
		zassert_mem_equal(data, test_tmp_buf, len, "UDP data mismatch at %zu", i);
	}

	net_pkt_unref(pkt);

	k_sem_give(&wait_received_data);

	return NET_OK;
}

static void setup_udp_handler(const struct net_in_addr *raddr, const struct net_in_addr *laddr,
			      uint16_t remote_port, uint16_t local_port, net_conn_cb_t cb)
{
	static struct net_conn_handle *handle;
	struct net_sockaddr remote_addr = { 0 };
	struct net_sockaddr local_addr = { 0 };
	int ret;

	net_ipaddr_copy(&net_sin(&local_addr)->sin_addr, laddr);
	local_addr.sa_family = NET_AF_INET;

	net_ipaddr_copy(&net_sin(&remote_addr)->sin_addr, raddr);
	remote_addr.sa_family = NET_AF_INET;

	ret = net_udp_register(NET_AF_INET, &local_addr, &remote_addr,
			       local_port, remote_port, NULL,
			       cb, NULL, &handle);

	zassert_equal(ret, 0, "Cannot register UDP connection");
}

static void setup_tcp_handler(const struct net_in_addr *raddr, const struct net_in_addr *laddr,
			      uint16_t remote_port, uint16_t local_port)
{
	static struct net_conn_handle *handle;
	struct net_sockaddr remote_addr = { 0 };
	struct net_sockaddr local_addr = { 0 };
	int ret;

	net_ipaddr_copy(&net_sin(&local_addr)->sin_addr, laddr);
	local_addr.sa_family = NET_AF_INET;

	net_ipaddr_copy(&net_sin(&remote_addr)->sin_addr, raddr);
	remote_addr.sa_family = NET_AF_INET;

	ret = net_conn_register(NET_IPPROTO_TCP, NET_SOCK_STREAM, NET_AF_INET, &local_addr,
				&remote_addr, local_port, remote_port, NULL,
				tcp_data_received, NULL, &handle);

	zassert_equal(ret, 0, "Cannot register TCP connection");
}

static void *test_setup(void)
{
	struct net_if_addr *ifaddr;

	/* The semaphore is there to wait the data to be received. */
	k_sem_init(&wait_data, 0, UINT_MAX);
	k_sem_init(&wait_received_data, 0, UINT_MAX);

	iface1 = net_if_get_by_index(1);
	zassert_not_null(iface1, "Network interface is null");

	ifaddr = net_if_ipv4_addr_add(iface1, &my_addr1, NET_ADDR_MANUAL, 0);
	LOG_DBG("Add IPv4 address %s", net_sprint_ipv4_addr(&my_addr1));

	if (!ifaddr) {
		LOG_DBG("Cannot add IPv4 address %s", net_sprint_ipv4_addr(&my_addr1));
		zassert_not_null(ifaddr, "addr1");
	}

	net_if_up(iface1);

	/* Setup TCP and UDP connections */
	setup_udp_handler(&my_addr1, &my_addr2, 4352, 25348, udp_data_received);
	setup_udp_handler(&my_addr1, &my_addr2, IPV4_OPTS_SRC_PORT, IPV4_OPTS_DST_PORT,
			  udp_opts_data_received);
	setup_tcp_handler(&my_addr1, &my_addr2, 4092, 19551);

	/* Generate test data */
	generate_dummy_data(test_tmp_buf, sizeof(test_tmp_buf));

	return NULL;
}

ZTEST(net_ipv4_fragment, test_udp)
{
	struct net_pkt *pkt;
	int ret;
	uint16_t i;
	uint16_t packet_len;
	uint16_t chksum = 0;

	/* Setup test variables */
	active_test = TEST_UDP;
	test_started = true;

	/* Create packet */
	pkt = net_pkt_alloc_with_buffer(iface1, sizeof(ipv4_udp) + IPV4_TEST_PACKET_SIZE,
					NET_AF_INET, NET_IPPROTO_UDP, ALLOC_TIMEOUT);
	zassert_not_null(pkt, "Packet creation failed");

	/* Add IPv4 and UDP headers */
	ret = net_pkt_write(pkt, ipv4_udp, sizeof(ipv4_udp));
	zassert_equal(ret, 0, "IPv4 header append failed");

	/* Add enough data until we have 4 packets */
	i = 0;
	while (i < IPV4_TEST_PACKET_SIZE) {
		ret = net_pkt_write(pkt, test_tmp_buf, sizeof(test_tmp_buf));
		zassert_equal(ret, 0, "IPv4 data append failed");
		i += sizeof(test_tmp_buf);
	}

	/* Setup packet for insertion */
	net_pkt_set_iface(pkt, iface1);
	net_pkt_set_family(pkt, NET_AF_INET);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));

	/* Update IPv4 headers */
	packet_len = net_pkt_get_len(pkt);
	NET_IPV4_HDR(pkt)->len = net_htons(packet_len);

	ret = net_calc_chksum_ipv4(pkt, &chksum);
	zassert_equal(ret, 0, "Calculation failed");
	NET_IPV4_HDR(pkt)->chksum = chksum;

	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);
	net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt));
	net_udp_finalize(pkt, false);

	pkt_recv_expected_size = net_pkt_get_len(pkt);

	ret = net_send_data(pkt);
	zassert_equal(ret, 0, "Packet send failure");

	zassert_equal(k_sem_take(&wait_data, WAIT_TIME), 0,
		      "Timeout waiting for packet to be sent");
	zassert_equal(k_sem_take(&wait_received_data, WAIT_TIME), 0,
		      "Timeout waiting for packet to be received");

	/* Check packet counts are valid */
	k_sleep(K_SECONDS(1));
	zassert_equal(lower_layer_packet_count, 4, "Expected 4 packets at lower layers");
	zassert_equal(upper_layer_packet_count, 1, "Expected 1 packet at upper layers");
	zassert_equal(last_packet_received, 1, "Expected last packet");
	zassert_equal(lower_layer_total_size, (NET_IPV4H_LEN * 3) + packet_len,
		      "Expected data send size mismatch at lower layers");
	zassert_equal(upper_layer_total_size, packet_len,
		      "Expected data received size mismatch at upper layers");
	zassert_equal(pkt_recv_expected_size, (pkt_recv_size + NET_IPV4H_LEN),
		      "Packet size mismatch");
}

ZTEST(net_ipv4_fragment, test_tcp)
{
	struct net_pkt *pkt;
	int ret;
	uint8_t tmp_buf[256];
	uint16_t i;
	uint16_t packet_len;
	uint16_t chksum = 0;

	/* Setup test variables */
	active_test = TEST_TCP;
	test_started = true;

	generate_dummy_data(tmp_buf, sizeof(tmp_buf));

	pkt = net_pkt_alloc_with_buffer(iface1, (sizeof(ipv4_tcp) + IPV4_TEST_PACKET_SIZE),
					NET_AF_INET, NET_IPPROTO_TCP, ALLOC_TIMEOUT);
	zassert_not_null(pkt, "Packet creation failure");

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));

	/* Add IPv4 and TCP headers */
	ret = net_pkt_write(pkt, ipv4_tcp, sizeof(ipv4_tcp));
	zassert_equal(ret, 0, "IPv4 header append failed");

	/* Add enough data until we have 4 packets */
	i = 0;
	while (i < IPV4_TEST_PACKET_SIZE) {
		ret = net_pkt_write(pkt, tmp_buf, sizeof(tmp_buf));
		zassert_equal(ret, 0, "IPv4 data append failed");
		i += sizeof(tmp_buf);
	}

	net_pkt_set_iface(pkt, iface1);
	net_pkt_set_family(pkt, NET_AF_INET);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));

	packet_len = net_pkt_get_len(pkt);

	NET_IPV4_HDR(pkt)->len = net_htons(packet_len);

	ret = net_calc_chksum_ipv4(pkt, &chksum);
	zassert_equal(ret, 0, "Calculation failed");
	NET_IPV4_HDR(pkt)->chksum = chksum;

	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);
	net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt));

	net_tcp_finalize(pkt, false);

	pkt_recv_expected_size = net_pkt_get_len(pkt);

	ret = net_send_data(pkt);
	zassert_equal(ret, 0, "Packet send failure");

	zassert_equal(k_sem_take(&wait_data, WAIT_TIME), 0,
		      "Timeout waiting for packet to be sent");
	zassert_equal(k_sem_take(&wait_received_data, WAIT_TIME), 0,
		      "Timeout waiting for packet to be received");

	/* Check packet counts are valid */
	k_sleep(K_SECONDS(1));
	zassert_equal(lower_layer_packet_count, 4, "Expected 4 packets at lower layers");
	zassert_equal(upper_layer_packet_count, 1, "Expected 1 packet at upper layers");
	zassert_equal(last_packet_received, 1, "Expected last packet");
	zassert_equal(lower_layer_total_size, (NET_IPV4H_LEN * 3) + packet_len,
		      "Expected data send size mismatch at lower layers");
	zassert_equal(upper_layer_total_size, packet_len,
		      "Expected data received size mismatch at upper layers");
	zassert_equal(pkt_recv_expected_size, (pkt_recv_size + NET_IPV4H_LEN),
		      "Packet size mismatch");
}

/* Build a UDP datagram that carries ipv4_opts and data_len bytes of
 * test_tmp_buf, with the UDP checksum calculated.
 */
static struct net_pkt *make_udp_options_pkt(const struct net_in_addr *src,
					    const struct net_in_addr *dst,
					    uint16_t src_port, uint16_t dst_port,
					    size_t data_len)
{
	struct net_ipv4_hdr hdr = { 0 };
	struct net_udp_hdr udp = { 0 };
	struct net_pkt *pkt;
	uint16_t chksum = 0;
	size_t i;

	pkt = net_pkt_alloc_with_buffer(iface1, IPV4_OPTS_HDR_LEN + NET_UDPH_LEN + data_len,
					NET_AF_INET, NET_IPPROTO_UDP, ALLOC_TIMEOUT);
	zassert_not_null(pkt, "Packet creation failed");

	hdr.vhl = 0x40 | (IPV4_OPTS_HDR_LEN / 4U);
	hdr.ttl = 64;
	hdr.proto = NET_IPPROTO_UDP;
	hdr.len = net_htons(IPV4_OPTS_HDR_LEN + NET_UDPH_LEN + data_len);
	net_ipv4_addr_copy_raw(hdr.src, src->s4_addr);
	net_ipv4_addr_copy_raw(hdr.dst, dst->s4_addr);
	udp.src_port = net_htons(src_port);
	udp.dst_port = net_htons(dst_port);

	zassert_ok(net_pkt_write(pkt, &hdr, sizeof(hdr)));
	zassert_ok(net_pkt_write(pkt, ipv4_opts, sizeof(ipv4_opts)));
	zassert_ok(net_pkt_write(pkt, &udp, sizeof(udp)));

	for (i = 0; i < data_len; i += sizeof(test_tmp_buf)) {
		zassert_ok(net_pkt_write(pkt, test_tmp_buf,
					 MIN(sizeof(test_tmp_buf), data_len - i)));
	}

	net_pkt_set_iface(pkt, iface1);
	net_pkt_set_family(pkt, NET_AF_INET);
	net_pkt_set_ip_hdr_len(pkt, NET_IPV4H_LEN);
	net_pkt_set_ipv4_opts_len(pkt, sizeof(ipv4_opts));

	zassert_ok(net_calc_chksum_ipv4(pkt, &chksum));
	NET_IPV4_HDR(pkt)->chksum = chksum;

	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);
	zassert_ok(net_pkt_skip(pkt, IPV4_OPTS_HDR_LEN));
	zassert_ok(net_udp_finalize(pkt, true));
	net_pkt_cursor_init(pkt);

	return pkt;
}

ZTEST(net_ipv4_fragment, test_udp_options)
{
	struct net_pkt *pkt;
	uint16_t packet_len;
	int ret;

	active_test = TEST_UDP_OPTIONS;
	test_started = true;

	pkt = make_udp_options_pkt(&my_addr1, &my_addr2, IPV4_OPTS_SRC_PORT,
				   IPV4_OPTS_DST_PORT, IPV4_TEST_PACKET_SIZE);
	packet_len = net_pkt_get_len(pkt);

	/* Leave the UDP checksum to the fragmentation, as when the interface
	 * would offload it.
	 */
	net_pkt_set_overwrite(pkt, true);
	zassert_ok(net_pkt_skip(pkt, IPV4_OPTS_HDR_LEN + offsetof(struct net_udp_hdr, chksum)));
	zassert_ok(net_pkt_write_be16(pkt, 0U));
	net_pkt_cursor_init(pkt);
	net_pkt_set_chksum_done(pkt, false);
	pkt_recv_expected_size = packet_len - IPV4_OPTS_HDR_LEN;

	ret = net_send_data(pkt);
	zassert_equal(ret, 0, "Packet send failure");

	zassert_equal(k_sem_take(&wait_data, WAIT_TIME), 0,
		      "Timeout waiting for packet to be sent");
	zassert_equal(k_sem_take(&wait_received_data, WAIT_TIME), 0,
		      "Timeout waiting for packet to be received");

	/* The first fragment carries all the options, the other three only
	 * Router Alert.
	 */
	k_sleep(K_SECONDS(1));
	zassert_equal(lower_layer_packet_count, 4, "Expected 4 packets at lower layers");
	zassert_equal(upper_layer_packet_count, 1, "Expected 1 packet at upper layers");
	zassert_equal(last_packet_received, 1, "Expected last packet");
	zassert_equal(lower_layer_total_size,
		      packet_len + 3 * (NET_IPV4H_LEN + sizeof(ipv4_opts_copied)),
		      "Expected data send size mismatch at lower layers");
	zassert_equal(upper_layer_total_size, packet_len,
		      "Expected data received size mismatch at upper layers");
	zassert_equal(pkt_recv_size, pkt_recv_expected_size, "Packet size mismatch");
}

static void recv_options_fragment(const uint8_t *dgram, uint16_t offset, uint16_t payload_len,
				  bool more)
{
	struct net_ipv4_hdr hdr;
	struct net_pkt *pkt;
	const uint8_t *opts;
	size_t opts_len;
	uint16_t chksum = 0;
	uint16_t flags;

	if (offset == 0U) {
		opts = ipv4_opts;
		opts_len = sizeof(ipv4_opts);
	} else {
		opts = ipv4_opts_copied;
		opts_len = sizeof(ipv4_opts_copied);
	}

	pkt = net_pkt_rx_alloc_with_buffer(iface1, NET_IPV4H_LEN + opts_len + payload_len,
					   NET_AF_INET, NET_IPPROTO_UDP, ALLOC_TIMEOUT);
	zassert_not_null(pkt, "Packet creation failed");

	memcpy(&hdr, dgram, sizeof(hdr));
	hdr.vhl = 0x40 | ((NET_IPV4H_LEN + opts_len) / 4U);
	hdr.len = net_htons(NET_IPV4H_LEN + opts_len + payload_len);
	hdr.id[0] = 0x5a;
	hdr.id[1] = 0xa5;
	flags = offset / 8U;
	if (more) {
		flags |= NET_IPV4_MORE_FRAG_MASK;
	}
	sys_put_be16(flags, hdr.offset);
	hdr.chksum = 0U;

	zassert_ok(net_pkt_write(pkt, &hdr, sizeof(hdr)));
	zassert_ok(net_pkt_write(pkt, opts, opts_len));
	zassert_ok(net_pkt_write(pkt, dgram + IPV4_OPTS_HDR_LEN + offset, payload_len));

	net_pkt_set_family(pkt, NET_AF_INET);
	net_pkt_set_ip_hdr_len(pkt, NET_IPV4H_LEN);
	net_pkt_set_ipv4_opts_len(pkt, opts_len);
	zassert_ok(net_calc_chksum_ipv4(pkt, &chksum));
	NET_IPV4_HDR(pkt)->chksum = chksum;

	net_pkt_cursor_init(pkt);
	zassert_equal(net_recv_data(iface1, pkt), 0, "Cannot receive data");
}

/* Fragments that carry IPv4 header options are reassembled into the
 * original datagram.
 */
ZTEST(net_ipv4_fragment, test_reassembly_options)
{
	uint8_t dgram[IPV4_OPTS_HDR_LEN + NET_UDPH_LEN + 200];
	struct net_pkt *pkt;

	/* The datagram as the remote end sends it */
	pkt = make_udp_options_pkt(&my_addr2, &my_addr1, IPV4_OPTS_DST_PORT,
				   IPV4_OPTS_SRC_PORT, sizeof(dgram) - IPV4_OPTS_HDR_LEN -
				   NET_UDPH_LEN);
	zassert_equal(net_pkt_get_len(pkt), sizeof(dgram));
	zassert_ok(net_pkt_read(pkt, dgram, sizeof(dgram)));
	net_pkt_unref(pkt);

	recv_options_fragment(dgram, 0, 64, true);
	recv_options_fragment(dgram, 64, 64, true);
	recv_options_fragment(dgram, 128, sizeof(dgram) - IPV4_OPTS_HDR_LEN - 128, false);

	zassert_equal(k_sem_take(&wait_received_data, WAIT_TIME), 0,
		      "Datagram with options not reassembled");
	zassert_equal(upper_layer_packet_count, 1, "Expected 1 packet at upper layers");
	zassert_equal(upper_layer_total_size, sizeof(dgram),
		      "Expected data received size mismatch at upper layers");
}

/* Build a datagram from my_addr1 to my_addr2 that carries the given header
 * options and an ICMP, TCP or UDP header followed by data_len bytes of
 * test_tmp_buf. The L4 checksum is left to the fragmentation.
 */
static struct net_pkt *make_l4_options_pkt(uint8_t proto, const uint8_t *opts,
					   size_t opts_len, size_t data_len)
{
	size_t hdr_len = NET_IPV4H_LEN + opts_len;
	struct net_ipv4_hdr hdr = { 0 };
	struct net_icmp_hdr icmp = { 0 };
	struct net_tcp_hdr tcp = { 0 };
	struct net_udp_hdr udp = { 0 };
	const void *l4_hdr;
	size_t l4_hdr_len;
	struct net_pkt *pkt;
	uint16_t chksum = 0;
	size_t i;

	switch (proto) {
	case NET_IPPROTO_ICMP:
		icmp.type = NET_ICMPV4_ECHO_REQUEST;
		l4_hdr = &icmp;
		l4_hdr_len = sizeof(icmp);
		break;
	case NET_IPPROTO_TCP:
		tcp.src_port = net_htons(IPV4_OPTS_SRC_PORT);
		tcp.dst_port = net_htons(IPV4_OPTS_DST_PORT);
		tcp.offset = (NET_TCPH_LEN / 4U) << 4;
		tcp.flags = 0x18; /* PSH and ACK */
		sys_put_be16(1024, tcp.wnd);
		l4_hdr = &tcp;
		l4_hdr_len = sizeof(tcp);
		break;
	default:
		udp.src_port = net_htons(IPV4_OPTS_SRC_PORT);
		udp.dst_port = net_htons(IPV4_OPTS_DST_PORT);
		udp.len = net_htons(NET_UDPH_LEN + data_len);
		l4_hdr = &udp;
		l4_hdr_len = sizeof(udp);
		break;
	}

	pkt = net_pkt_alloc_with_buffer(iface1, hdr_len + l4_hdr_len + data_len,
					NET_AF_INET, proto, ALLOC_TIMEOUT);
	zassert_not_null(pkt, "Packet creation failed");

	hdr.vhl = 0x40 | (hdr_len / 4U);
	hdr.ttl = 64;
	hdr.proto = proto;
	hdr.len = net_htons(hdr_len + l4_hdr_len + data_len);
	net_ipv4_addr_copy_raw(hdr.src, my_addr1.s4_addr);
	net_ipv4_addr_copy_raw(hdr.dst, my_addr2.s4_addr);

	zassert_ok(net_pkt_write(pkt, &hdr, sizeof(hdr)));
	zassert_ok(net_pkt_write(pkt, opts, opts_len));
	zassert_ok(net_pkt_write(pkt, l4_hdr, l4_hdr_len));

	for (i = 0; i < data_len; i += sizeof(test_tmp_buf)) {
		zassert_ok(net_pkt_write(pkt, test_tmp_buf,
					 MIN(sizeof(test_tmp_buf), data_len - i)));
	}

	net_pkt_set_iface(pkt, iface1);
	net_pkt_set_family(pkt, NET_AF_INET);
	net_pkt_set_ip_hdr_len(pkt, NET_IPV4H_LEN);
	net_pkt_set_ipv4_opts_len(pkt, opts_len);

	zassert_ok(net_calc_chksum_ipv4(pkt, &chksum));
	NET_IPV4_HDR(pkt)->chksum = chksum;

	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);
	zassert_true(l4_hdr_len + data_len <= sizeof(wire_expected), "Datagram too long");
	zassert_ok(net_pkt_skip(pkt, hdr_len));
	zassert_ok(net_pkt_read(pkt, wire_expected, l4_hdr_len + data_len));

	net_pkt_cursor_init(pkt);
	net_pkt_set_chksum_done(pkt, false);

	return pkt;
}

/* Send a datagram with header options whose L4 checksum is left to the
 * fragmentation. Check the options of every fragment, and that the payload
 * of all the fragments is the original one with a valid L4 checksum in the
 * L4 header.
 */
static void send_options_datagram(uint8_t proto, const uint8_t *opts, size_t opts_len,
				  const uint8_t *copied, size_t copied_len)
{
	size_t fit_len = ROUND_DOWN(NET_IPV4_MTU - NET_IPV4H_LEN - opts_len, 8U);
	struct net_pkt *pkt;
	uint32_t sum = 0U;
	size_t chksum_offset;
	size_t len;

	exp_opts = opts;
	exp_opts_len = opts_len;
	exp_opts_copied = copied;
	exp_opts_copied_len = copied_len;

	active_test = TEST_OPTIONS_WIRE;
	test_started = true;

	pkt = make_l4_options_pkt(proto, opts, opts_len, IPV4_TEST_PACKET_SIZE);
	len = net_pkt_get_len(pkt) - NET_IPV4H_LEN - opts_len;
	pkt_recv_expected_size = len;

	zassert_ok(net_send_data(pkt), "Packet send failure");
	zassert_equal(k_sem_take(&wait_data, WAIT_TIME), 0,
		      "Timeout waiting for packet to be sent");
	k_sleep(K_MSEC(100));

	zexpect_true(last_packet_received, "Expected last packet");
	zexpect_equal(lower_layer_packet_count, DIV_ROUND_UP(len, fit_len),
		      "Fragment count mismatch");
	zassert_equal(pkt_recv_size, len, "Payload size mismatch");

	/* ICMP has no pseudo header */
	if (proto != NET_IPPROTO_ICMP) {
		sum = inet_sum(0U, my_addr1.s4_addr, sizeof(my_addr1));
		sum = inet_sum(sum, my_addr2.s4_addr, sizeof(my_addr2));
		sum += proto + len;
	}

	zexpect_equal(inet_sum(sum, wire_payload, len), 0xffffU, "L4 checksum mismatch");

	/* The checksum went into the L4 header and nowhere else */
	switch (proto) {
	case NET_IPPROTO_ICMP:
		chksum_offset = offsetof(struct net_icmp_hdr, chksum);
		break;
	case NET_IPPROTO_TCP:
		chksum_offset = offsetof(struct net_tcp_hdr, chksum);
		break;
	default:
		chksum_offset = offsetof(struct net_udp_hdr, chksum);
		break;
	}

	zexpect_not_equal(sys_get_be16(&wire_payload[chksum_offset]), 0U, "L4 checksum missing");
	sys_put_be16(0U, &wire_payload[chksum_offset]);
	zexpect_mem_equal(wire_payload, wire_expected, len, "L4 payload mismatch");
}

ZTEST(net_ipv4_fragment, test_icmp_options_chksum)
{
	send_options_datagram(NET_IPPROTO_ICMP, ipv4_opts, sizeof(ipv4_opts),
			      ipv4_opts_copied, sizeof(ipv4_opts_copied));
}

ZTEST(net_ipv4_fragment, test_tcp_options_chksum)
{
	send_options_datagram(NET_IPPROTO_TCP, ipv4_opts, sizeof(ipv4_opts),
			      ipv4_opts_copied, sizeof(ipv4_opts_copied));
}

/* The copied options that are not a multiple of 4 bytes are padded with
 * End of Option List.
 */
ZTEST(net_ipv4_fragment, test_copied_options_padding)
{
	send_options_datagram(NET_IPPROTO_UDP, ipv4_opts_lsrr, sizeof(ipv4_opts_lsrr),
			      ipv4_opts_lsrr_copied, sizeof(ipv4_opts_lsrr_copied));
}

/* A datagram whose header options cannot be parsed is not fragmented */
ZTEST(net_ipv4_fragment, test_malformed_options_not_fragmented)
{
	/* Option type in the last byte, without a length */
	static const uint8_t truncated[] = { 0x01, 0x01, 0x01, 0x83 };
	/* Option length below the minimum of 2 */
	static const uint8_t too_short[] = { 0x83, 0x00, 0x00, 0x00 };
	/* Option length past the end of the header */
	static const uint8_t too_long[] = {
		0x83, 0xff, 0x04, 0xc0, 0xa8, 0x08, 0x02, 0x00,
	};
	static const struct {
		const uint8_t *opts;
		size_t len;
	} cases[] = {
		{ truncated, sizeof(truncated) },
		{ too_short, sizeof(too_short) },
		{ too_long, sizeof(too_long) },
	};

	active_test = TEST_OPTIONS_WIRE;
	test_started = true;

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		struct net_pkt *pkt;
		int ret;

		pkt = make_l4_options_pkt(NET_IPPROTO_UDP, cases[i].opts, cases[i].len,
					  IPV4_TEST_PACKET_SIZE);

		ret = net_ipv4_send_fragmented_pkt(iface1, pkt, net_pkt_get_len(pkt),
						   NET_IPV4_MTU);
		zexpect_equal(ret, -EINVAL, "Case %zu fragmented (%d)", i, ret);

		net_pkt_unref(pkt);
	}

	k_sleep(K_MSEC(100));
	zexpect_equal(lower_layer_packet_count, 0, "Fragments sent");
}

/* Test inserting only 1 fragment and ensuring that it is removed after the timeout elapses */
ZTEST(net_ipv4_fragment, test_fragment_timeout)
{
	struct net_pkt *pkt;
	int ret;
	uint8_t packets;
	int sem_count;
	uint16_t chksum = 0;

	/* Setup test variables */
	active_test = TEST_SINGLE_FRAGMENT;
	test_started = true;

	/* Create a packet for the test */
	pkt = net_pkt_alloc_with_buffer(iface1, sizeof(ipv4_udp_frag), NET_AF_INET,
					NET_IPPROTO_UDP, ALLOC_TIMEOUT);
	zassert_not_null(pkt, "Packet creation failure");

	net_pkt_set_family(pkt, NET_AF_INET);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));

	/* Create packet from base data */
	net_pkt_cursor_init(pkt);
	ret = net_pkt_write(pkt, ipv4_udp_frag, sizeof(ipv4_udp_frag));
	zassert_equal(ret, 0, "IPv4 fragmented frame append failed");

	/* Generate valid checksum for frame */
	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);

	ret = net_calc_chksum_ipv4(pkt, &chksum);
	zassert_equal(ret, 0, "Calculation failed");

	NET_IPV4_HDR(pkt)->chksum = chksum;
	net_pkt_set_overwrite(pkt, false);

	pkt_recv_expected_size = sizeof(ipv4_icmp_reassembly_time);

	/* Directly put the packet into the interface */
	net_pkt_set_iface(pkt, iface1);
	ret = net_recv_data(net_pkt_iface(pkt), pkt);
	zassert_equal(ret, 0, "Cannot receive data (%d)", ret);

	/* Check number of pending reassembly packets */
	k_sleep(K_MSEC(10));
	packets = 0;
	net_ipv4_frag_foreach(reassembly_foreach_cb, &packets);
	zassert_equal(packets, 1, "Expected fragment to be present in buffer");

	/* Delay briefly and re-check number of pending reassembly packets */
	k_sleep(K_MSEC(1100));
	packets = 0;
	net_ipv4_frag_foreach(reassembly_foreach_cb, &packets);
	zassert_equal(packets, 0, "Expected fragment to be dropped after timeout");

	/* Ensure a lower-layer frame was received */
	sem_count = k_sem_count_get(&wait_data);
	zassert_equal(sem_count, 1, "Expected one lower-layer frame");

	/* Ensure no complete upper-layer packets were received */
	sem_count = k_sem_count_get(&wait_received_data);
	zassert_equal(sem_count, 0, "Expected no complete upper-layer packets");

	/* Check packet counts are valid */
	k_sleep(K_MSEC(500));
	zassert_equal(lower_layer_packet_count, 1, "Expected 1 packet at lower layers");
	zassert_equal(upper_layer_packet_count, 0, "Expected no packets at upper layers");
	zassert_equal(last_packet_received, 1, "Expected last packet");
	zassert_equal(lower_layer_total_size, sizeof(ipv4_icmp_reassembly_time),
		      "Expected 56 total bytes sent at lower layers");
	zassert_equal(upper_layer_total_size, 0,
		      "Expected 0 total bytes received at upper layers");
	zassert_equal(pkt_recv_expected_size, (pkt_recv_size + NET_IPV4H_LEN),
		      "Packet size mismatch");
}

/* Test inserting large packet with do not fragment bit set */
ZTEST(net_ipv4_fragment, test_do_not_fragment)
{
	struct net_pkt *pkt;
	int ret;
	uint8_t tmp_buf[256];
	uint16_t i;
	uint16_t packet_len;
	uint16_t chksum = 0;

	/* Setup test variables */
	active_test = TEST_NO_FRAGMENT;
	test_started = true;

	/* Generate test data */
	generate_dummy_data(tmp_buf, sizeof(tmp_buf));

	/* Create packet */
	pkt = net_pkt_alloc_with_buffer(iface1,
					(sizeof(ipv4_udp_do_not_frag) + IPV4_TEST_PACKET_SIZE),
					NET_AF_INET, NET_IPPROTO_UDP, ALLOC_TIMEOUT);
	zassert_not_null(pkt, "Packet creation failed");

	/* Add IPv4 and UDP headers */
	ret = net_pkt_write(pkt, ipv4_udp_do_not_frag, sizeof(ipv4_udp_do_not_frag));
	zassert_equal(ret, 0, "IPv4 header append failed");

	/* Add enough data until we have 4 packets */
	i = 0;
	while (i < IPV4_TEST_PACKET_SIZE) {
		ret = net_pkt_write(pkt, tmp_buf, sizeof(tmp_buf));
		zassert_equal(ret, 0, "IPv4 data append failed");
		i += sizeof(tmp_buf);
	}

	/* Setup packet for insertion */
	net_pkt_set_iface(pkt, iface1);
	net_pkt_set_family(pkt, NET_AF_INET);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));

	/* Update IPv4 headers */
	packet_len = net_pkt_get_len(pkt);
	NET_IPV4_HDR(pkt)->len = net_htons(packet_len);

	ret = net_calc_chksum_ipv4(pkt, &chksum);
	zassert_equal(ret, 0, "Calculation failed");
	NET_IPV4_HDR(pkt)->chksum = chksum;

	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);
	net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt));
	net_udp_finalize(pkt, false);

	pkt_recv_expected_size = net_pkt_get_len(pkt);

	ret = net_send_data(pkt);
	zassert_equal(ret, 0, "Packet send failure");

	zassert_equal(k_sem_take(&wait_data, WAIT_TIME), -EAGAIN,
		      "Expected timeout waiting for packet to be sent");
	zassert_equal(k_sem_take(&wait_received_data, WAIT_TIME), -EAGAIN,
		      "Expected timeout waiting for packet to be received");

	/* Check packet counts are valid */
	k_sleep(K_MSEC(100));
	zassert_equal(lower_layer_packet_count, 0, "Expected no packets at lower layers");
	zassert_equal(upper_layer_packet_count, 0, "Expected no packets at upper layers");
	zassert_equal(last_packet_received, 0, "Did not expect last packet");
	zassert_equal(lower_layer_total_size, 0,
		      "Expected data send size mismatch at lower layers");
	zassert_equal(upper_layer_total_size, 0,
		      "Expected data received size mismatch at upper layers");
	zassert_equal(pkt_recv_size, pkt_recv_expected_size, "Packet size mismatch");
}

static int reass_err_rx_pkt_free_count(void)
{
	struct k_mem_slab *rx = NULL, *tx = NULL;
	struct net_buf_pool *rx_data = NULL, *tx_data = NULL;

	net_pkt_get_info(&rx, &tx, &rx_data, &tx_data);
	zassert_not_null(rx);
	return k_mem_slab_num_free_get(rx);
}

static void reass_err_ipv4_hdr_init(struct net_ipv4_hdr *hdr, uint16_t total_len,
				    uint16_t offset_bytes, bool more)
{
	uint16_t off;

	memset(hdr, 0, sizeof(*hdr));
	hdr->vhl = 0x45;
	hdr->ttl = 64;
	hdr->proto = NET_IPPROTO_UDP;
	sys_put_be16(total_len, (uint8_t *)&hdr->len);
	sys_put_be16(REASS_ERR_FRAG_ID, hdr->id);
	off = (offset_bytes / 8);
	if (more) {
		off |= NET_IPV4_MORE_FRAG_MASK;
	}
	sys_put_be16(off, hdr->offset);
	memcpy(hdr->src, &reass_err_src_addr, sizeof(reass_err_src_addr));
	memcpy(hdr->dst, &reass_err_dst_addr, sizeof(reass_err_dst_addr));
}

static void reass_err_fault_inject_noncontiguous_ipv4_hdr(struct net_pkt *pkt)
{
	size_t len;
	uint8_t copy[NET_IPV4H_LEN + REASS_ERR_FRAG_PAYLOAD];
	struct net_buf *frag;
	struct net_buf *b0;
	struct net_buf *b1;

	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(access, struct net_ipv4_hdr);

	len = net_pkt_get_len(pkt);
	zassert_true(len >= NET_IPV4H_LEN && len <= sizeof(copy));
	net_pkt_cursor_init(pkt);
	zassert_ok(net_pkt_read(pkt, copy, len));

	while (pkt->buffer) {
		frag = pkt->buffer;
		pkt->buffer = frag->frags;
		frag->frags = NULL;
		net_buf_unref(frag);
	}

	b0 = net_pkt_get_frag(pkt, 10, K_NO_WAIT);
	zassert_not_null(b0);
	net_buf_add_mem(b0, copy, 10);
	net_pkt_frag_add(pkt, b0);

	b1 = net_pkt_get_frag(pkt, len - 10, K_NO_WAIT);
	zassert_not_null(b1);
	net_buf_add_mem(b1, copy + 10, len - 10);
	net_pkt_frag_add(pkt, b1);

	net_pkt_set_overwrite(pkt, true);
	net_pkt_set_ip_hdr_len(pkt, NET_IPV4H_LEN);
	net_pkt_cursor_init(pkt);

	zassert_false(net_pkt_is_contiguous(pkt, NET_IPV4H_LEN));
	zassert_is_null(net_pkt_get_data(pkt, &access));
}

static struct net_pkt *reass_err_make_udp_fragment(struct net_if *iface,
						   const uint8_t *udp_dgram,
						   uint16_t offset, uint16_t payload_len,
						   bool more)
{
	struct net_pkt *pkt;
	struct net_ipv4_hdr hdr;
	uint16_t total_len = NET_IPV4H_LEN + payload_len;

	pkt = net_pkt_rx_alloc_with_buffer(iface, total_len, NET_AF_INET, NET_IPPROTO_UDP,
					   K_NO_WAIT);
	zassert_not_null(pkt);

	reass_err_ipv4_hdr_init(&hdr, total_len, offset, more);
	net_pkt_set_ip_hdr_len(pkt, NET_IPV4H_LEN);
	net_pkt_set_ipv4_opts_len(pkt, 0);
	net_pkt_set_iface(pkt, iface);
	net_pkt_set_family(pkt, NET_AF_INET);

	zassert_ok(net_pkt_write(pkt, &hdr, sizeof(hdr)));
	zassert_ok(net_pkt_write(pkt, udp_dgram + offset, payload_len));
	net_pkt_cursor_init(pkt);

	return pkt;
}

ZTEST(net_ipv4_fragment, test_reassembly_error_path_cleanup)
{
	struct net_if *iface = net_if_get_default();
	uint8_t udp_dgram[REASS_ERR_UDP_DGRAM_LEN];
	struct net_pkt *frag0, *frag1, *frag2;
	struct net_ipv4_hdr hdr0, hdr1, hdr2;
	int free_before, free_after;
	const int allocated = 3;

	zassert_not_null(iface);

	for (int i = 0; i < REASS_ERR_UDP_DGRAM_LEN; i++) {
		udp_dgram[i] = (uint8_t)i;
	}

	frag0 = reass_err_make_udp_fragment(iface, udp_dgram, 0, REASS_ERR_FRAG_PAYLOAD, true);
	frag1 = reass_err_make_udp_fragment(iface, udp_dgram, REASS_ERR_FRAG_PAYLOAD,
					    REASS_ERR_FRAG_PAYLOAD, true);
	frag2 = reass_err_make_udp_fragment(iface, udp_dgram, REASS_ERR_FRAG_PAYLOAD * 2,
					    REASS_ERR_FRAG_PAYLOAD, false);

	zassert_ok(net_pkt_read(frag0, &hdr0, sizeof(hdr0)));
	net_pkt_cursor_init(frag0);
	zassert_ok(net_pkt_read(frag1, &hdr1, sizeof(hdr1)));
	net_pkt_cursor_init(frag1);
	zassert_ok(net_pkt_read(frag2, &hdr2, sizeof(hdr2)));
	net_pkt_cursor_init(frag2);

	reass_err_fault_inject_noncontiguous_ipv4_hdr(frag1);
	free_before = reass_err_rx_pkt_free_count();

	zassert_equal(net_ipv4_handle_fragment_hdr(frag0, &hdr0), NET_OK);
	zassert_equal(net_ipv4_handle_fragment_hdr(frag1, &hdr1), NET_OK);
	zassert_equal(net_ipv4_handle_fragment_hdr(frag2, &hdr2), NET_OK);

	free_after = reass_err_rx_pkt_free_count();

	zassert_equal(free_after, free_before + allocated,
		      "leaked %d of %d fragments (free count %d -> %d)",
		      free_before + allocated - free_after, allocated,
		      free_before, free_after);
}

/* Exercises the reassembly timeout against completion of the same slot.
 *
 * The final fragment drives reassembly completion while the per-slot timeout
 * handler runs concurrently against the same slot. The timer cancel hook
 * pins the interleaving: completion is paused at its cancel, the slot timeout
 * is allowed to fire, then completion resumes. The timeout handler must wait
 * for completion to finish rather than release the fragments under it, so
 * reassembly completes and the slot is released.
 */
ZTEST(net_ipv4_fragment, test_reassembly_timeout_race)
{
	struct net_if *iface = net_if_get_default();
	uint8_t udp_dgram[REASS_ERR_FRAG_PAYLOAD * 2];
	struct net_pkt *frag0;
	struct net_ipv4_hdr hdr0;
	uint8_t packets;

	zassert_not_null(iface);

	for (unsigned int i = 0; i < sizeof(udp_dgram); i++) {
		udp_dgram[i] = (uint8_t)i;
	}

	/* First fragment arms the slot timeout. */
	frag0 = reass_err_make_udp_fragment(iface, udp_dgram, 0, REASS_ERR_FRAG_PAYLOAD, true);
	zassert_ok(net_pkt_read(frag0, &hdr0, sizeof(hdr0)));
	net_pkt_cursor_init(frag0);
	zassert_equal(net_ipv4_handle_fragment_hdr(frag0, &hdr0), NET_OK);

	packets = 0;
	net_ipv4_frag_foreach(reassembly_foreach_cb, &packets);
	zassert_equal(packets, 1, "Expected one pending reassembly");

	race_timer = NULL;
	net_ipv4_frag_foreach(race_timer_cb, NULL);
	zassert_not_null(race_timer, "Reassembly slot timer not found");

	/* Prepare the final fragment for delivery from the worker thread. */
	race_final_frag = reass_err_make_udp_fragment(iface, udp_dgram, REASS_ERR_FRAG_PAYLOAD,
						      REASS_ERR_FRAG_PAYLOAD, false);
	zassert_ok(net_pkt_read(race_final_frag, &race_final_hdr, sizeof(race_final_hdr)));
	net_pkt_cursor_init(race_final_frag);

	race_armed = true;

	k_thread_create(&race_worker, race_worker_stack,
			K_THREAD_STACK_SIZEOF(race_worker_stack),
			race_worker_fn, NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	/* Wait until completion is inside reassembly and paused. */
	zassert_equal(k_sem_take(&race_reached, K_SECONDS(1)), 0,
		      "Reassembly completion path was not reached");

	/* Let the slot timeout fire while completion is paused. */
	k_sleep(K_MSEC(CONFIG_NET_IPV4_FRAGMENT_TIMEOUT * 1000 + 200));

	/* Resume completion. */
	k_sem_give(&race_resume);

	zassert_equal(k_thread_join(&race_worker, K_SECONDS(2)), 0,
		      "Reassembly completion did not finish");

	/* The slot must be released after completion. */
	k_sleep(K_MSEC(100));
	packets = 0;
	net_ipv4_frag_foreach(reassembly_foreach_cb, &packets);
	zassert_equal(packets, 0, "Reassembly slot was not released");
}

ZTEST(net_ipv4_fragment, test_clone_keeps_reassembled_flag)
{
	struct net_pkt *pkt;
	struct net_pkt *clone;

	pkt = net_pkt_alloc_with_buffer(iface1, sizeof(ipv4_udp), NET_AF_INET,
					NET_IPPROTO_UDP, K_NO_WAIT);
	zassert_not_null(pkt, "Packet creation failed");

	net_pkt_set_ip_reassembled(pkt, true);

	clone = net_pkt_clone(pkt, K_NO_WAIT);
	zassert_not_null(clone, "Packet clone failed");

	zexpect_true(net_pkt_is_ip_reassembled(clone), "Clone lost the reassembled flag");

	net_pkt_unref(clone);
	net_pkt_unref(pkt);
}

static void test_pre(void *ptr)
{
	race_armed = false;
	k_sem_reset(&race_reached);
	k_sem_reset(&race_resume);

	k_sem_reset(&wait_data);
	k_sem_reset(&wait_received_data);

	lower_layer_packet_count = 0;
	upper_layer_packet_count = 0;
	lower_layer_total_size = 0;
	upper_layer_total_size = 0;
	last_packet_received = false;
	test_started = false;
	pkt_id = 0;
	pkt_recv_size = 0;
	pkt_recv_expected_size = 0;

	exp_opts = ipv4_opts;
	exp_opts_len = sizeof(ipv4_opts);
	exp_opts_copied = ipv4_opts_copied;
	exp_opts_copied_len = sizeof(ipv4_opts_copied);
}

ZTEST_SUITE(net_ipv4_fragment, NULL, test_setup, test_pre, NULL, NULL);
