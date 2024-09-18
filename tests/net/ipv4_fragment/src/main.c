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
#include <zephyr/net/net_if.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/net/socket.h>
#include <net_private.h>
#include <ipv4.h>
#include <udp_internal.h>
#include <tcp_internal.h>

/* Packet size for tests, excluding headers */
#define IPV4_TEST_PACKET_SIZE 2048

/* Wait times for semaphores and buffers */
#define WAIT_TIME K_SECONDS(2)
#define ALLOC_TIMEOUT K_MSEC(500)

/* Dummy network addresses, 192.168.8.1 and 192.168.8.2 */
static struct in_addr my_addr1 = { { { 0xc0, 0xa8, 0x08, 0x01 } } };
static struct in_addr my_addr2 = { { { 0xc0, 0xa8, 0x08, 0x02 } } };

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

enum {
	TEST_UDP,
	TEST_TCP,
	TEST_SINGLE_FRAGMENT,
	TEST_NO_FRAGMENT,
};

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

static struct dummy_api net_iface_api = {
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

/* Checks all IPv4 headers against expected values */
static void check_ipv4_fragment_header(struct net_pkt *pkt, const uint8_t *orig_hdr, uint16_t id,
				       uint16_t current_length, bool final)
{
	uint16_t pkt_len;
	uint16_t pkt_offset;
	uint8_t pkt_flags;
	const struct net_ipv4_hdr *hdr = NET_IPV4_HDR(pkt);

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

	pkt_len = ntohs(hdr->len);
	pkt_offset = ntohs(*((uint16_t *)&hdr->offset));
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
	zassert_equal(net_calc_chksum_ipv4(pkt), 0, "IPv4 header checksum mismatch");
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
			if (active_test == TEST_UDP || active_test == TEST_TCP) {
				zassert_not_equal(pkt_id, 0, "IPv4 header ID should not be 0");
			} else if (active_test == TEST_SINGLE_FRAGMENT) {
				zassert_equal(pkt_id, 0, "IPv4 header ID should be 0");
			}
		}

		last_packet = ((pkt_recv_size + net_pkt_get_len(pkt)) >= pkt_recv_expected_size ?
			      true : false);
		check_ipv4_fragment_header(pkt, (active_test == TEST_UDP ? ipv4_udp :
					   (active_test == TEST_TCP ? ipv4_tcp :
					   ipv4_icmp_reassembly_time)), pkt_id, pkt_recv_size,
					   last_packet);
		pkt_recv_size += net_pkt_get_len(pkt) - NET_IPV4H_LEN;

		if (last_packet) {
			last_packet_received = true;
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
			net_pkt_skip(recv_pkt, NET_IPV4H_LEN);
			net_pkt_read_be16(recv_pkt, &buf_port_src);
			net_pkt_read_be16(recv_pkt, &buf_port_dst);

			net_pkt_cursor_init(recv_pkt);
			net_pkt_skip(recv_pkt, NET_IPV4H_LEN);
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

	pkt_len = ntohs(hdr->len);
	pkt_offset = ntohs(*((uint16_t *)&hdr->offset));
	pkt_flags = (pkt_offset & ~NET_IPV4_FRAGH_OFFSET_MASK) >> 13;
	pkt_offset &= NET_IPV4_FRAGH_OFFSET_MASK;
	pkt_offset *= 8;

	zassert_equal(pkt_flags, 0, "IPv4 header fragment flags mismatch");
	zassert_equal(net_pkt_get_len(pkt), pkt_len, "IPv4 header length mismatch");
	zassert_equal(pkt_offset, 0, "IPv4 header length mismatch");
	zassert_equal(net_calc_chksum_ipv4(pkt), 0, "IPv4 header checksum mismatch");

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

	pkt_len = ntohs(hdr->len);
	pkt_offset = ntohs(*((uint16_t *)&hdr->offset));
	pkt_flags = (pkt_offset & ~NET_IPV4_FRAGH_OFFSET_MASK) >> 13;
	pkt_offset &= NET_IPV4_FRAGH_OFFSET_MASK;
	pkt_offset *= 8;

	zassert_equal(pkt_flags, 0, "IPv4 header fragment flags mismatch");
	zassert_equal(net_pkt_get_len(pkt), pkt_len, "IPv4 header length mismatch");
	zassert_equal(pkt_offset, 0, "IPv4 header length mismatch");
	zassert_equal(net_calc_chksum_ipv4(pkt), 0, "IPv4 header checksum mismatch");

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

static void setup_udp_handler(const struct in_addr *raddr, const struct in_addr *laddr,
			      uint16_t remote_port, uint16_t local_port)
{
	static struct net_conn_handle *handle;
	struct sockaddr remote_addr = { 0 };
	struct sockaddr local_addr = { 0 };
	int ret;

	net_ipaddr_copy(&net_sin(&local_addr)->sin_addr, laddr);
	local_addr.sa_family = AF_INET;

	net_ipaddr_copy(&net_sin(&remote_addr)->sin_addr, raddr);
	remote_addr.sa_family = AF_INET;

	ret = net_udp_register(AF_INET, &local_addr, &remote_addr, local_port, remote_port, NULL,
			       udp_data_received, NULL, &handle);

	zassert_equal(ret, 0, "Cannot register UDP connection");
}

static void setup_tcp_handler(const struct in_addr *raddr, const struct in_addr *laddr,
			      uint16_t remote_port, uint16_t local_port)
{
	static struct net_conn_handle *handle;
	struct sockaddr remote_addr = { 0 };
	struct sockaddr local_addr = { 0 };
	int ret;

	net_ipaddr_copy(&net_sin(&local_addr)->sin_addr, laddr);
	local_addr.sa_family = AF_INET;

	net_ipaddr_copy(&net_sin(&remote_addr)->sin_addr, raddr);
	remote_addr.sa_family = AF_INET;

	ret = net_conn_register(IPPROTO_TCP, AF_INET, &local_addr, &remote_addr, local_port,
				remote_port, NULL, tcp_data_received, NULL, &handle);

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
	setup_udp_handler(&my_addr1, &my_addr2, 4352, 25348);
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

	/* Setup test variables */
	active_test = TEST_UDP;
	test_started = true;

	/* Create packet */
	pkt = net_pkt_alloc_with_buffer(iface1, sizeof(ipv4_udp) + IPV4_TEST_PACKET_SIZE, AF_INET,
					IPPROTO_UDP, ALLOC_TIMEOUT);
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
	net_pkt_set_family(pkt, AF_INET);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));

	/* Update IPv4 headers */
	packet_len = net_pkt_get_len(pkt);
	NET_IPV4_HDR(pkt)->len = htons(packet_len);
	NET_IPV4_HDR(pkt)->chksum = net_calc_chksum_ipv4(pkt);

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

	/* Setup test variables */
	active_test = TEST_TCP;
	test_started = true;

	generate_dummy_data(tmp_buf, sizeof(tmp_buf));

	pkt = net_pkt_alloc_with_buffer(iface1, (sizeof(ipv4_tcp) + IPV4_TEST_PACKET_SIZE),
					AF_INET, IPPROTO_TCP, ALLOC_TIMEOUT);
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
	net_pkt_set_family(pkt, AF_INET);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));

	packet_len = net_pkt_get_len(pkt);

	NET_IPV4_HDR(pkt)->len = htons(packet_len);
	NET_IPV4_HDR(pkt)->chksum = net_calc_chksum_ipv4(pkt);

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

/* Test inserting only 1 fragment and ensuring that it is removed after the timeout elapses */
ZTEST(net_ipv4_fragment, test_fragment_timeout)
{
	struct net_pkt *pkt;
	int ret;
	uint8_t packets;
	int sem_count;

	/* Setup test variables */
	active_test = TEST_SINGLE_FRAGMENT;
	test_started = true;

	/* Create a packet for the test */
	pkt = net_pkt_alloc_with_buffer(iface1, sizeof(ipv4_udp_frag), AF_INET,
					IPPROTO_UDP, ALLOC_TIMEOUT);
	zassert_not_null(pkt, "Packet creation failure");

	net_pkt_set_family(pkt, AF_INET);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));

	/* Create packet from base data */
	net_pkt_cursor_init(pkt);
	ret = net_pkt_write(pkt, ipv4_udp_frag, sizeof(ipv4_udp_frag));
	zassert_equal(ret, 0, "IPv4 fragmented frame append failed");

	/* Generate valid checksum for frame */
	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);
	NET_IPV4_HDR(pkt)->chksum = net_calc_chksum_ipv4(pkt);
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
	k_sleep(K_SECONDS(6));
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
	k_sleep(K_SECONDS(1));
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

	/* Setup test variables */
	active_test = TEST_NO_FRAGMENT;
	test_started = true;

	/* Generate test data */
	generate_dummy_data(tmp_buf, sizeof(tmp_buf));

	/* Create packet */
	pkt = net_pkt_alloc_with_buffer(iface1,
					(sizeof(ipv4_udp_do_not_frag) + IPV4_TEST_PACKET_SIZE),
					AF_INET, IPPROTO_UDP, ALLOC_TIMEOUT);
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
	net_pkt_set_family(pkt, AF_INET);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));

	/* Update IPv4 headers */
	packet_len = net_pkt_get_len(pkt);
	NET_IPV4_HDR(pkt)->len = htons(packet_len);
	NET_IPV4_HDR(pkt)->chksum = net_calc_chksum_ipv4(pkt);

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
	k_sleep(K_SECONDS(1));
	zassert_equal(lower_layer_packet_count, 0, "Expected no packets at lower layers");
	zassert_equal(upper_layer_packet_count, 0, "Expected no packets at upper layers");
	zassert_equal(last_packet_received, 0, "Did not expect last packet");
	zassert_equal(lower_layer_total_size, 0,
		      "Expected data send size mismatch at lower layers");
	zassert_equal(upper_layer_total_size, 0,
		      "Expected data received size mismatch at upper layers");
	zassert_equal(pkt_recv_size, pkt_recv_expected_size, "Packet size mismatch");
}

static void test_pre(void *ptr)
{
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
}

ZTEST_SUITE(net_ipv4_fragment, NULL, test_setup, test_pre, NULL, NULL);
