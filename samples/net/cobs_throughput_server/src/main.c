/*
 * Copyright (c) 2025 Martin Schröder
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cobs_server, LOG_LEVEL_INF);

/* External reference to COBS Serial L2 */
extern const struct net_l2 _net_l2_COBS_SERIAL;

#define SERVER_PORT 5001
#define MAX_PACKET_SIZE 1472  /* MTU 1500 - IP header - UDP header */

/* Statistics */
static struct {
	uint64_t packets_received;
	uint64_t packets_sent;
	uint64_t bytes_received;
	uint64_t bytes_sent;
	uint64_t errors;
	uint64_t pattern_errors;
	uint32_t start_time;
} stats;

/* Verify packet pattern */
static bool verify_pattern(const uint8_t *data, size_t len, uint32_t seq)
{
	/* Pattern: sequence number (4 bytes) + repeating byte pattern */
	if (len < sizeof(uint32_t)) {
		return false;
	}

	uint32_t pkt_seq;
	memcpy(&pkt_seq, data, sizeof(uint32_t));
	
	if (pkt_seq != seq) {
		LOG_WRN("Sequence mismatch: expected %u, got %u", seq, pkt_seq);
		return false;
	}

	/* Verify the pattern (incremental bytes) */
	for (size_t i = sizeof(uint32_t); i < len; i++) {
		uint8_t expected = (uint8_t)((seq + i) & 0xFF);
		if (data[i] != expected) {
			LOG_WRN("Pattern error at offset %zu: expected 0x%02x, got 0x%02x",
				i, expected, data[i]);
			return false;
		}
	}

	return true;
}

/* Print statistics periodically */
static void print_stats(void)
{
	uint32_t elapsed = k_uptime_get_32() - stats.start_time;
	uint32_t elapsed_sec = elapsed / 1000;

	if (elapsed_sec == 0) {
		return;
	}

	uint64_t rx_kbps = (stats.bytes_received * 8) / (elapsed_sec * 1000);
	uint64_t tx_kbps = (stats.bytes_sent * 8) / (elapsed_sec * 1000);

	LOG_INF("=== Statistics (elapsed: %u s) ===", elapsed_sec);
	LOG_INF("  RX: %llu pkts, %llu bytes (%llu kbps)",
		stats.packets_received, stats.bytes_received, rx_kbps);
	LOG_INF("  TX: %llu pkts, %llu bytes (%llu kbps)",
		stats.packets_sent, stats.bytes_sent, tx_kbps);
	LOG_INF("  Errors: %llu, Pattern errors: %llu",
		stats.errors, stats.pattern_errors);
}

/* Statistics printing thread */
static void stats_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		k_sleep(K_SECONDS(10));
		print_stats();
	}
}

K_THREAD_DEFINE(stats_tid, 1024, stats_thread, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
	int sock;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	uint8_t buffer[MAX_PACKET_SIZE];
	int ret;
	uint32_t expected_seq = 0;
	struct net_if *iface;

	LOG_INF("SLIP Throughput Server");

	/* Find SLIP interface */
	iface = net_if_get_first_by_type(&_net_l2_COBS_SERIAL);
	if (!iface) {
		LOG_ERR("No SLIP interface found");
		return -1;
	}

	LOG_INF("Using SLIP interface: %d", net_if_get_by_iface(iface));

	/* Wait for interface to be up */
	while (!net_if_is_up(iface)) {
		LOG_INF("Waiting for SLIP interface to come up...");
		k_sleep(K_MSEC(100));
	}

	LOG_INF("SLIP interface is up");

	/* Create UDP socket */
	sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create socket: %d", errno);
		return -1;
	}

	/* Bind to server port */
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERVER_PORT);

	ret = zsock_bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (ret < 0) {
		LOG_ERR("Failed to bind socket: %d", errno);
		zsock_close(sock);
		return -1;
	}

	LOG_INF("Server listening on port %d", SERVER_PORT);
	LOG_INF("Max packet size: %d bytes", MAX_PACKET_SIZE);

	/* Initialize statistics */
	stats.start_time = k_uptime_get_32();

	/* Main echo loop */
	while (1) {
		ret = zsock_recvfrom(sock, buffer, sizeof(buffer), 0,
			       (struct sockaddr *)&client_addr, &client_addr_len);
		
		if (ret < 0) {
			LOG_ERR("recvfrom failed: %d", errno);
			stats.errors++;
			continue;
		}

		if (ret == 0) {
			continue;
		}

		stats.packets_received++;
		stats.bytes_received += ret;

		/* Verify pattern */
		if (!verify_pattern(buffer, ret, expected_seq)) {
			stats.pattern_errors++;
		}
		expected_seq++;

		/* Echo the packet back */
		ret = zsock_sendto(sock, buffer, ret, 0,
			     (struct sockaddr *)&client_addr, client_addr_len);
		
		if (ret < 0) {
			LOG_ERR("sendto failed: %d", errno);
			stats.errors++;
			continue;
		}

		stats.packets_sent++;
		stats.bytes_sent += ret;
	}

	zsock_close(sock);
	return 0;
}

