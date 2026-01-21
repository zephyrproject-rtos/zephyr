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
#include <zephyr/shell/shell.h>
#include <string.h>

LOG_MODULE_REGISTER(slip_throughput, LOG_LEVEL_INF);

/* External reference to COBS Serial L2 */
extern const struct net_l2 _net_l2_COBS_SERIAL;

#define SERVER_PORT 5001
#define MAX_PACKET_SIZE 1472  /* MTU 1500 - IP header - UDP header */
#define RX_TIMEOUT_MS 2000

/* IPv6 addresses for the two SLIP interfaces */
#define SLIP0_IPV6_ADDR "fd00:1::1"  /* Client interface */
#define SLIP1_IPV6_ADDR "fd00:2::1"  /* Server interface */

/* Interface contexts */
static struct net_if *slip0_iface;  /* Client interface */
static struct net_if *slip1_iface;  /* Server interface */

/* Test parameters */
static struct {
	uint32_t packet_size;
	uint32_t duration_sec;
	uint32_t delay_ms;
	uint32_t processing_delay_ms;
	bool running;
	bool stop_requested;
} test_params = {
	.packet_size = 512,
	.duration_sec = 10,
	.delay_ms = 500,  /* Default 1ms delay to avoid overwhelming the system */
	.processing_delay_ms = 0,
	.running = false,
	.stop_requested = false,
};

/* Statistics - protected by mutex for thread safety */
static struct {
	uint32_t packets_sent;
	uint32_t packets_received;
	uint64_t bytes_sent;
	uint64_t bytes_received;
	uint32_t errors;
	uint32_t pattern_errors;
	uint32_t timeouts;
	uint32_t server_rx_count;
	uint32_t server_tx_count;
	uint32_t min_rtt_us;
	uint32_t max_rtt_us;
	uint64_t total_rtt_us;
	uint32_t start_time;
	uint32_t tx_finish_time;
} stats;

/* Thread synchronization */
static K_SEM_DEFINE(test_complete_sem, 0, 1);
static struct k_spinlock stats_lock;

/* Work queue for auto-stopping test */
static struct k_work_delayable auto_stop_work;

/* Thread completion signaling using semaphores */
static K_SEM_DEFINE(tx_thread_done_sem, 0, 1);
static K_SEM_DEFINE(rx_thread_done_sem, 0, 1);

/* Thread synchronization: RX ready signal */
static K_SEM_DEFINE(rx_ready_sem, 0, 1);

/* Thread stacks and IDs */
#define SERVER_STACK_SIZE 4096
#define CLIENT_TX_STACK_SIZE 4096
#define CLIENT_RX_STACK_SIZE 4096
#define SERVER_PRIORITY 6
#define CLIENT_TX_PRIORITY 5
#define CLIENT_RX_PRIORITY 5

static K_THREAD_STACK_DEFINE(server_stack, SERVER_STACK_SIZE);
static K_THREAD_STACK_DEFINE(client_tx_stack, CLIENT_TX_STACK_SIZE);
static K_THREAD_STACK_DEFINE(client_rx_stack, CLIENT_RX_STACK_SIZE);
static struct k_thread server_thread_data;
static struct k_thread client_tx_thread_data;
static struct k_thread client_rx_thread_data;
static k_tid_t server_tid;
static k_tid_t client_tx_tid;
static k_tid_t client_rx_tid;

/* Sockets */
static int server_sock = -1;
static int client_sock = -1;

/* Fill packet with predictable pattern */
static void fill_pattern(uint8_t *data, size_t len, uint32_t seq)
{
	/* Pattern: sequence number (4 bytes) + repeating byte pattern */
	memcpy(data, &seq, sizeof(uint32_t));
	
	for (size_t i = sizeof(uint32_t); i < len; i++) {
		data[i] = (uint8_t)((seq + i) & 0xFF);
	}
}

/* Verify received packet pattern */
static bool verify_pattern(const uint8_t *data, size_t len, uint32_t seq)
{
	if (len < sizeof(uint32_t)) {
		return false;
	}

	uint32_t pkt_seq;
	memcpy(&pkt_seq, data, sizeof(uint32_t));
	
	if (pkt_seq != seq) {
		LOG_WRN("Sequence mismatch: expected %u, got %u", seq, pkt_seq);
		return false;
	}

	/* Verify the pattern */
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

/* Helper for recvfrom with poll-based timeout */
static int recvfrom_with_timeout(int sock, void *buf, size_t len,
				 struct sockaddr *addr, socklen_t *addrlen,
				 int timeout_ms)
{
	struct zsock_pollfd pfd = {
		.fd = sock,
		.events = ZSOCK_POLLIN | ZSOCK_POLLERR | ZSOCK_POLLHUP,
	};

	int pret = zsock_poll(&pfd, 1, timeout_ms);
	if (pret == 0) {
		errno = EAGAIN;
		return -1;
	}
	if (pret < 0) {
		return -1;
	}

	if ((pfd.revents & ZSOCK_POLLIN) == 0) {
		/* Woke up for error/hup, not readable */
		errno = EIO;
		return -1;
	}

	return zsock_recvfrom(sock, buf, len, 0, addr, addrlen);
}

/* Print final statistics */
static void print_final_stats(void)
{
	/* Calculate elapsed time from test start (when threads actually started sending) */
	/* stats.start_time is set before threads start, so we need to account for that */
	uint32_t elapsed = k_uptime_get_32() - stats.start_time;
	uint32_t elapsed_sec = elapsed / 1000;

	if (elapsed_sec == 0) {
		elapsed_sec = 1;
	}

	/* Copy stats under spinlock, then release before logging */
	k_spinlock_key_t key = k_spin_lock(&stats_lock);
	uint32_t packets_sent = stats.packets_sent;
	uint32_t packets_received = stats.packets_received;
	uint64_t bytes_sent = stats.bytes_sent;
	uint64_t bytes_received = stats.bytes_received;
	uint32_t server_rx_count = stats.server_rx_count;
	uint32_t server_tx_count = stats.server_tx_count;
	uint32_t errors = stats.errors;
	uint32_t pattern_errors = stats.pattern_errors;
	uint32_t timeouts = stats.timeouts;
	uint32_t min_rtt_us = stats.min_rtt_us;
	uint32_t max_rtt_us = stats.max_rtt_us;
	uint64_t total_rtt_us = stats.total_rtt_us;
	k_spin_unlock(&stats_lock, key);

	/* Now calculate and log without holding the lock */
	uint64_t tx_kbps = (bytes_sent * 8) / (elapsed_sec * 1000);
	uint64_t rx_kbps = (bytes_received * 8) / (elapsed_sec * 1000);
	uint32_t avg_rtt_us = packets_received > 0 ?
			      (uint32_t)(total_rtt_us / packets_received) : 0;

	LOG_INF("");
	LOG_INF("=== Final Statistics ===");
	LOG_INF("Test duration: %u seconds", elapsed_sec);
	LOG_INF("Packet size: %u bytes", test_params.packet_size);
	LOG_INF("");
	LOG_INF("Client TX: %u packets, %llu bytes (%llu kbps)",
		packets_sent, bytes_sent, tx_kbps);
	LOG_INF("Client RX: %u packets, %llu bytes (%llu kbps)",
		packets_received, bytes_received, rx_kbps);
	LOG_INF("");
	LOG_INF("Server RX: %u packets", server_rx_count);
	LOG_INF("Server TX: %u packets", server_tx_count);
	LOG_INF("");
	LOG_INF("Packet loss: %u (%.2f%%)",
		packets_sent - packets_received,
		packets_sent > 0 ? 
		(double)(packets_sent - packets_received) * 100.0 / packets_sent : 0.0);
	LOG_INF("Errors: %u, Pattern errors: %u, Timeouts: %u",
		errors, pattern_errors, timeouts);
	LOG_INF("");
	LOG_INF("RTT (us): min=%u, avg=%u, max=%u",
		min_rtt_us, avg_rtt_us, max_rtt_us);
	LOG_INF("======================");
}

/* Server thread - echoes packets back */
static void server_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct sockaddr_in6 server_addr, client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	uint8_t buffer[MAX_PACKET_SIZE];
	int ret;
	struct in6_addr slip1_addr;

	LOG_DBG("Server thread started");

	/* Wait for cobs1 interface to be up */
	if (!slip1_iface) {
		LOG_ERR("Server: cobs1 interface not found");
		k_spinlock_key_t key = k_spin_lock(&stats_lock);
		stats.errors++;
		k_spin_unlock(&stats_lock, key);
		return;
	}

	int wait_count = 50; /* 5 seconds */
	while (!net_if_is_up(slip1_iface) && wait_count > 0) {
		k_sleep(K_MSEC(100));
		wait_count--;
	}

	if (!net_if_is_up(slip1_iface)) {
		LOG_ERR("Server: Timeout waiting for cobs1 interface");
		k_spinlock_key_t key = k_spin_lock(&stats_lock);
		stats.errors++;
		k_spin_unlock(&stats_lock, key);
		return;
	}

	/* Create UDP socket */
	server_sock = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (server_sock < 0) {
		LOG_ERR("Server: Failed to create socket: %d", errno);
		k_spinlock_key_t key = k_spin_lock(&stats_lock);
		stats.errors++;
		k_spin_unlock(&stats_lock, key);
		return;
	}

	/* Bind to cobs1's address and server port */
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin6_family = AF_INET6;
	zsock_inet_pton(AF_INET6, SLIP1_IPV6_ADDR, &slip1_addr);
	server_addr.sin6_addr = slip1_addr;
	server_addr.sin6_port = htons(SERVER_PORT);

	ret = zsock_bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (ret < 0) {
		LOG_ERR("Server: Failed to bind socket: %d", errno);
		zsock_close(server_sock);
		server_sock = -1;
		k_spinlock_key_t key = k_spin_lock(&stats_lock);
		stats.errors++;
		k_spin_unlock(&stats_lock, key);
		return;
	}

	/* Bind server socket to cobs1 interface to ensure correct routing */
	struct net_ifreq if1_req = { 0 };
	(void)net_if_get_name(slip1_iface, if1_req.ifr_name, sizeof(if1_req.ifr_name));
	ret = zsock_setsockopt(server_sock, SOL_SOCKET, SO_BINDTODEVICE,
			       &if1_req, sizeof(if1_req));
	if (ret < 0) {
		LOG_WRN("Server: Failed to bind to device %s: %d", if1_req.ifr_name, errno);
	} else {
		LOG_DBG("Server bound to device %s (cobs1)", if1_req.ifr_name);
	}

	LOG_INF("Server listening on [%s]:%d (cobs1)", SLIP1_IPV6_ADDR, SERVER_PORT);
	
	/* Echo loop - runs continuously until explicitly stopped */
	while (!test_params.stop_requested) {
		client_addr_len = sizeof(client_addr);
		ret = recvfrom_with_timeout(server_sock, buffer, sizeof(buffer),
					    (struct sockaddr *)&client_addr,
					    &client_addr_len, 100);
		
		if (ret < 0) {
			if (errno == EAGAIN || errno == ETIMEDOUT) {
				/* Timeout is normal, continue waiting */
				continue;
			}
			LOG_ERR("Server recvfrom failed: %d (errno: %d)", ret, errno);
			k_spinlock_key_t key = k_spin_lock(&stats_lock);
			stats.errors++;
			k_spin_unlock(&stats_lock, key);
			continue;
		}

		if (ret == 0) {
			LOG_WRN("Server: Received zero-length packet");
			continue;
		}

		/* Extract sequence number from packet */
		uint32_t pkt_seq;
		if (ret >= sizeof(uint32_t)) {
			memcpy(&pkt_seq, buffer, sizeof(uint32_t));
		} else {
			pkt_seq = 0; /* Invalid packet */
		}

		k_spinlock_key_t key = k_spin_lock(&stats_lock);
		stats.server_rx_count++;
		uint32_t rx_count = stats.server_rx_count;
		k_spin_unlock(&stats_lock, key);
		
		/* Log every 1000 packets to reduce log spam */
		if (rx_count % 1000 == 0) {
			LOG_INF("Server: Received %u packets", rx_count);
		}

		/* Echo the packet back */
		char addr_str[INET6_ADDRSTRLEN];
		zsock_inet_ntop(AF_INET6, &client_addr.sin6_addr, addr_str, sizeof(addr_str));
		
		k_sleep(K_MSEC(test_params.processing_delay_ms));

		LOG_INF("Server: Echoing packet seq=%u (rx_count=%u)", pkt_seq, rx_count);

		ret = zsock_sendto(server_sock, buffer, ret, 0,
			     (struct sockaddr *)&client_addr, client_addr_len);
		
		if (ret < 0) {
			LOG_ERR("Server sendto failed to [%s]:%d: %d (errno: %d)", 
				addr_str, ntohs(client_addr.sin6_port), ret, errno);
			key = k_spin_lock(&stats_lock);
			stats.errors++;
			k_spin_unlock(&stats_lock, key);
			continue;
		}

		key = k_spin_lock(&stats_lock);
		stats.server_tx_count++;
		uint32_t tx_count = stats.server_tx_count;
		k_spin_unlock(&stats_lock, key);

		/* Log every 1000 packets to reduce log spam */
		if (tx_count % 1000 == 0) {
			LOG_INF("Server: Sent %u packets", tx_count);
		}
	}

	zsock_close(server_sock);
	server_sock = -1;
	LOG_INF("Server thread finished");
}

/* Client TX thread - sends packets */
static void client_tx_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	uint8_t tx_buffer[MAX_PACKET_SIZE];
	struct sockaddr_in6 server_addr;
	int ret;
	uint32_t seq = 0;
	/* Calculate test end time based on when this thread actually starts */
	uint32_t test_end_time = k_uptime_get_32() + (test_params.duration_sec * 1000);

	LOG_DBG("Client TX thread started");

	/* Wait for RX thread to be ready before sending first packet */
	LOG_DBG("Client TX: Waiting for RX thread to be ready...");
	ret = k_sem_take(&rx_ready_sem, K_SECONDS(5));
	if (ret < 0) {
		LOG_ERR("Client TX: Timeout waiting for RX thread to be ready");
		k_spinlock_key_t key = k_spin_lock(&stats_lock);
		stats.errors++;
		k_spin_unlock(&stats_lock, key);
		return;
	}
	LOG_INF("Client TX: RX thread is ready, starting transmission");

	/* Setup server address - send to cobs1's IPv6 address */
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin6_family = AF_INET6;
	server_addr.sin6_port = htons(SERVER_PORT);
	ret = zsock_inet_pton(AF_INET6, SLIP1_IPV6_ADDR, &server_addr.sin6_addr);
	if (ret != 1) {
		LOG_ERR("Client TX: Invalid server IP address");
		k_spinlock_key_t key = k_spin_lock(&stats_lock);
		stats.errors++;
		k_spin_unlock(&stats_lock, key);
		return;
	}

	/* Main TX loop - controlled by stop condition and duration */
	while (!test_params.stop_requested && k_uptime_get_32() < test_end_time) {
		/* Fill packet with pattern */
		fill_pattern(tx_buffer, test_params.packet_size, seq);

		/* Send packet */
		ret = zsock_sendto(client_sock, tx_buffer, test_params.packet_size, 0,
			     (struct sockaddr *)&server_addr, sizeof(server_addr));
		
		if (ret < 0) {
			LOG_ERR("Client TX: sendto failed (seq %u): %d (errno: %d)", seq, ret, errno);
			k_spinlock_key_t key = k_spin_lock(&stats_lock);
			stats.errors++;
			k_spin_unlock(&stats_lock, key);
			seq++;
			continue;
		}

		/* Only log first packet and then every 1000 packets */
		if (seq == 0) {
			LOG_DBG("Client TX: Sent first packet (%d bytes)", ret);
		}
		k_spinlock_key_t key = k_spin_lock(&stats_lock);
		stats.packets_sent++;
		stats.bytes_sent += ret;
		k_spin_unlock(&stats_lock, key);

		LOG_INF("Client TX: Sent packet seq=%u", seq);

		seq++;

		/* Delay between packets to avoid overwhelming the system */
		if (test_params.delay_ms > 0) {
			k_sleep(K_MSEC(test_params.delay_ms));
		} else {
			/* Even with 0 delay, yield to allow other threads to run */
			k_yield();
		}
	}

	k_spinlock_key_t key = k_spin_lock(&stats_lock);
	uint32_t packets_sent = stats.packets_sent;
	stats.tx_finish_time = k_uptime_get_32();
	k_spin_unlock(&stats_lock, key);
	
	LOG_INF("Client TX thread finished: sent %u packets", packets_sent);
	
	/* Signal TX thread completion */
	k_sem_give(&tx_thread_done_sem);
}

/* Client RX thread - receives and verifies echo responses */
static void client_rx_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	uint8_t rx_buffer[MAX_PACKET_SIZE];
	int ret;

	LOG_DBG("Client RX thread started");

	/* Signal that RX thread is ready to receive */
	k_sem_give(&rx_ready_sem);
	LOG_DBG("Client RX: Ready to receive");

	/* Main RX loop - runs continuously until explicitly stopped */
	while (!test_params.stop_requested) {
		ret = recvfrom_with_timeout(client_sock, rx_buffer, sizeof(rx_buffer),
					    NULL, NULL, RX_TIMEOUT_MS);

		if (ret < 0) {
			if (errno == EAGAIN || errno == ETIMEDOUT) {
				k_spinlock_key_t key = k_spin_lock(&stats_lock);
				stats.timeouts++;
				k_spin_unlock(&stats_lock, key);
				continue;
			}

			LOG_ERR("Client RX: recvfrom failed: %d", errno);
			k_spinlock_key_t key = k_spin_lock(&stats_lock);
			stats.errors++;
			k_spin_unlock(&stats_lock, key);
			continue;
		}

		/* Extract sequence number first */
		uint32_t seq;
		if (ret >= sizeof(uint32_t)) {
			memcpy(&seq, rx_buffer, sizeof(uint32_t));
		} else {
			seq = 0; /* Invalid packet */
		}

		k_spinlock_key_t key = k_spin_lock(&stats_lock);
		stats.packets_received++;
		stats.bytes_received += ret;
		uint32_t rx_count = stats.packets_received;
		k_spin_unlock(&stats_lock, key);

		LOG_INF("Client RX: Received packet seq=%u (rx_count=%u)", seq, rx_count);

		if (!verify_pattern(rx_buffer, ret, seq)) {
			key = k_spin_lock(&stats_lock);
			stats.pattern_errors++;
			k_spin_unlock(&stats_lock, key);
		}

		/* Calculate RTT (simplified - using packet count as proxy) */
		/* In a real implementation, we'd track per-packet timestamps */
	}

	k_spinlock_key_t key = k_spin_lock(&stats_lock);
	uint32_t packets_received = stats.packets_received;
	k_spin_unlock(&stats_lock, key);
	
	LOG_INF("Client RX thread finished: received %u packets", packets_received);
	
	/* Signal RX thread completion */
	k_sem_give(&rx_thread_done_sem);
}

/* Find and configure SLIP interfaces */
static int setup_interfaces(void)
{
	int if_count = 0;
	struct in6_addr addr0, addr1;
	struct net_if_addr *ifaddr0, *ifaddr1;

	LOG_INF("Setting up SLIP interfaces...");

	/* Find both SLIP interfaces */
	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (net_if_l2(iface) != &_net_l2_COBS_SERIAL) {
			continue;
		}

		LOG_INF("Found SLIP interface %d: %s",
			net_if_get_by_iface(iface),
			net_if_get_device(iface)->name);

		if (if_count == 0) {
			slip0_iface = iface;
			LOG_INF("Assigned to cobs0 (client)");
		} else if (if_count == 1) {
			slip1_iface = iface;
			LOG_INF("Assigned to cobs1 (server)");
		}

		if_count++;
	}

	if (if_count < 2) {
		LOG_ERR("Expected 2 SLIP interfaces, found %d", if_count);
		return -ENODEV;
	}

	if (!slip0_iface || !slip1_iface) {
		LOG_ERR("Failed to assign SLIP interfaces");
		return -ENODEV;
	}

	/* Bring interfaces up */
	net_if_up(slip0_iface);
	net_if_up(slip1_iface);

	/* Configure IPv6 addresses */
	zsock_inet_pton(AF_INET6, SLIP0_IPV6_ADDR, &addr0);
	ifaddr0 = net_if_ipv6_addr_add(slip0_iface, &addr0, NET_ADDR_MANUAL, 0);
	if (!ifaddr0) {
		LOG_ERR("Failed to add IPv6 address to cobs0");
		return -ENOMEM;
	}
	LOG_INF("Configured cobs0 with IP: %s", SLIP0_IPV6_ADDR);

	zsock_inet_pton(AF_INET6, SLIP1_IPV6_ADDR, &addr1);
	ifaddr1 = net_if_ipv6_addr_add(slip1_iface, &addr1, NET_ADDR_MANUAL, 0);
	if (!ifaddr1) {
		LOG_ERR("Failed to add IPv6 address to cobs1");
		return -ENOMEM;
	}
	LOG_INF("Configured cobs1 with IP: %s", SLIP1_IPV6_ADDR);

	/* Add IPv6 prefixes for routing */
	/* Prefix for cobs0 subnet: fd00:1::/64 */
	struct in6_addr prefix0;
	zsock_inet_pton(AF_INET6, "fd00:1::", &prefix0);
	struct net_if_ipv6_prefix *prefix0_ptr = net_if_ipv6_prefix_add(slip0_iface, &prefix0, 64, 0);
	if (!prefix0_ptr) {
		LOG_ERR("Failed to add IPv6 prefix to cobs0");
		return -ENOMEM;
	}
	net_if_ipv6_prefix_set_lf(prefix0_ptr, false);
	LOG_INF("Added prefix fd00:1::/64 to cobs0");

	/* Prefix for cobs1 subnet: fd00:2::/64 */
	struct in6_addr prefix1;
	zsock_inet_pton(AF_INET6, "fd00:2::", &prefix1);
	struct net_if_ipv6_prefix *prefix1_ptr = net_if_ipv6_prefix_add(slip1_iface, &prefix1, 64, 0);
	if (!prefix1_ptr) {
		LOG_ERR("Failed to add IPv6 prefix to cobs1");
		return -ENOMEM;
	}
	net_if_ipv6_prefix_set_lf(prefix1_ptr, false);
	LOG_INF("Added prefix fd00:2::/64 to cobs1");

	/* Wait for both interfaces to be up (with timeout) */
	int setup_timeout = 50; /* 5 seconds */
	while ((!net_if_is_up(slip0_iface) || !net_if_is_up(slip1_iface)) && setup_timeout > 0) {
		k_sleep(K_MSEC(100));
		setup_timeout--;
	}

	if (!net_if_is_up(slip0_iface) || !net_if_is_up(slip1_iface)) {
		LOG_ERR("Timeout waiting for SLIP interfaces to come up");
		LOG_ERR("cobs0 up: %d, cobs1 up: %d", 
			net_if_is_up(slip0_iface), net_if_is_up(slip1_iface));
		return -ETIMEDOUT;
	}

	LOG_INF("Both SLIP interfaces are up and configured");
	return 0;
}

/* Run throughput test */
static int run_test(void)
{
	int ret;
	struct timeval timeout;
	struct sockaddr_in6 client_bind_addr;
	struct in6_addr slip0_addr;

	if (test_params.running) {
		LOG_WRN("Test already running");
		return -EBUSY;
	}

	if (test_params.packet_size > MAX_PACKET_SIZE) {
		LOG_ERR("Packet size too large: %u (max: %u)",
			test_params.packet_size, MAX_PACKET_SIZE);
		return -EINVAL;
	}

	test_params.running = true;
	test_params.stop_requested = false;
	k_sem_reset(&tx_thread_done_sem);
	k_sem_reset(&rx_thread_done_sem);
	k_sem_reset(&rx_ready_sem);

	LOG_INF("=== Starting Throughput Test ===");
	LOG_INF("Packet size: %u bytes", test_params.packet_size);
	LOG_INF("Duration: %u seconds", test_params.duration_sec);
	LOG_INF("Delay: %u ms", test_params.delay_ms);
	LOG_INF("Route: cobs0 (%s) -> cobs1 (%s)", SLIP0_IPV6_ADDR, SLIP1_IPV6_ADDR);

	/* Check if interfaces are configured */
	if (!slip0_iface || !slip1_iface) {
		LOG_ERR("SLIP interfaces not configured");
		test_params.running = false;
		return -ENODEV;
	}

	/* Quick check if interfaces are up (non-blocking) */
	if (!net_if_is_up(slip0_iface) || !net_if_is_up(slip1_iface)) {
		LOG_WRN("Interfaces not up yet, threads will wait for them");
	}

	/* Create client UDP socket */
	client_sock = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (client_sock < 0) {
		LOG_ERR("Failed to create client socket: %d", errno);
		test_params.running = false;
		return -errno;
	}

	/* Bind client to cobs0's address */
	memset(&client_bind_addr, 0, sizeof(client_bind_addr));
	client_bind_addr.sin6_family = AF_INET6;
	zsock_inet_pton(AF_INET6, SLIP0_IPV6_ADDR, &slip0_addr);
	client_bind_addr.sin6_addr = slip0_addr;
	client_bind_addr.sin6_port = 0; /* Let system assign port */
	
	ret = zsock_bind(client_sock, (struct sockaddr *)&client_bind_addr,
			 sizeof(client_bind_addr));
	if (ret < 0) {
		LOG_ERR("Failed to bind client to cobs0 address: %d", errno);
		zsock_close(client_sock);
		client_sock = -1;
		test_params.running = false;
		return -errno;
	}

	/* Bind client socket to cobs0 interface to ensure correct routing */
	struct net_ifreq if0_req = { 0 };
	(void)net_if_get_name(slip0_iface, if0_req.ifr_name, sizeof(if0_req.ifr_name));
	ret = zsock_setsockopt(client_sock, SOL_SOCKET, SO_BINDTODEVICE,
			       &if0_req, sizeof(if0_req));
	if (ret < 0) {
		LOG_WRN("Client: Failed to bind to device %s: %d", if0_req.ifr_name, errno);
	} else {
		LOG_DBG("Client bound to device %s (cobs0)", if0_req.ifr_name);
	}

	LOG_DBG("Client bound to cobs0 (%s)", SLIP0_IPV6_ADDR);

	/* Set receive timeout for client socket */
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	ret = zsock_setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	if (ret < 0) {
		LOG_WRN("Failed to set client socket timeout: %d", errno);
	}

	/* Initialize statistics */
	k_spinlock_key_t key = k_spin_lock(&stats_lock);
	memset(&stats, 0, sizeof(stats));
	stats.min_rtt_us = UINT32_MAX;
	stats.start_time = k_uptime_get_32();
	stats.tx_finish_time = 0;
	k_spin_unlock(&stats_lock, key);

	LOG_DBG("Test started...");

	/* Start server thread */
	server_tid = k_thread_create(&server_thread_data, server_stack,
				     K_THREAD_STACK_SIZEOF(server_stack),
				     server_thread_fn, NULL, NULL, NULL,
				     SERVER_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(server_tid, "server");

	/* Start client TX thread */
	client_tx_tid = k_thread_create(&client_tx_thread_data, client_tx_stack,
				       K_THREAD_STACK_SIZEOF(client_tx_stack),
				       client_tx_thread_fn, NULL, NULL, NULL,
				       CLIENT_TX_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(client_tx_tid, "client_tx");

	/* Start client RX thread */
	client_rx_tid = k_thread_create(&client_rx_thread_data, client_rx_stack,
				       K_THREAD_STACK_SIZEOF(client_rx_stack),
				       client_rx_thread_fn, NULL, NULL, NULL,
				       CLIENT_RX_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(client_rx_tid, "client_rx");

	/* Schedule auto-stop work to check for test completion */
	k_work_schedule(&auto_stop_work, K_SECONDS(1));
	LOG_DBG("Auto-stop monitoring scheduled");

	return 0;
}

/* Forward declaration */
static int stop_test(void);

/* Work handler to auto-stop test when threads finish */
static void auto_stop_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	
	if (!test_params.running) {
		return;
	}
	
	/* Check if TX thread is done */
	if (k_sem_count_get(&tx_thread_done_sem) > 0) {
		/* TX is done, check how long we've been waiting */
		k_spinlock_key_t key = k_spin_lock(&stats_lock);
		uint32_t tx_finish_time = stats.tx_finish_time;
		uint32_t packets_sent = stats.packets_sent;
		uint32_t packets_received = stats.packets_received;
		k_spin_unlock(&stats_lock, key);
		
		uint32_t elapsed_since_tx_finish = k_uptime_get_32() - tx_finish_time;
		
		/* Calculate expected wait time: give enough time for all packets to be echoed back
		 * Use RX timeout (2s) plus some buffer (3s), total 5 seconds */
		#define AUTO_STOP_TIMEOUT_MS 5000
		
		if (elapsed_since_tx_finish >= AUTO_STOP_TIMEOUT_MS) {
			LOG_INF("Auto-stopping test (TX done, waited %u ms for remaining packets)",
				elapsed_since_tx_finish);
			LOG_INF("Final: sent=%u, received=%u", packets_sent, packets_received);
			stop_test();
			return;
		}
		
		/* If all packets have been received, stop immediately */
		if (packets_received >= packets_sent) {
			LOG_INF("Auto-stopping test (all packets received)");
			stop_test();
			return;
		}
	}
	
	/* Reschedule to check again in 1 second */
	k_work_schedule(&auto_stop_work, K_SECONDS(1));
}

/* Stop the test and wait for completion */
static int stop_test(void)
{
	if (!test_params.running) {
		LOG_WRN("No test is running");
		return -EINVAL;
	}

	LOG_INF("Stopping test...");
	
	/* Cancel auto-stop work */
	k_work_cancel_delayable(&auto_stop_work);
	
	test_params.stop_requested = true;

	/* Wait for threads to finish */
	k_thread_join(client_tx_tid, K_FOREVER);
	LOG_DBG("Client TX thread joined");
	
	k_thread_join(client_rx_tid, K_FOREVER);
	LOG_DBG("Client RX thread joined");
	
	k_thread_join(server_tid, K_FOREVER);
	LOG_DBG("Server thread joined");

	/* Close sockets */
	if (client_sock >= 0) {
		zsock_close(client_sock);
		client_sock = -1;
	}

	test_params.running = false;

	/* Print final statistics */
	print_final_stats();

	return 0;
}

/* Shell command: start test */
static int cmd_start(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Starting throughput test...");
	int ret = run_test();
	if (ret < 0) {
		shell_error(sh, "Failed to start test: %d", ret);
	}
	return ret;
}

/* Shell command: stop test */
static int cmd_stop(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Stopping throughput test...");
	int ret = stop_test();
	if (ret < 0) {
		shell_error(sh, "Failed to stop test: %d", ret);
	}
	return ret;
}

/* Shell command: set packet size */
static int cmd_set_size(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_error(sh, "Usage: set_size <bytes>");
		return -EINVAL;
	}

	if (test_params.running) {
		shell_error(sh, "Cannot change settings while test is running");
		return -EBUSY;
	}

	uint32_t size = strtoul(argv[1], NULL, 10);
	if (size < 16 || size > MAX_PACKET_SIZE) {
		shell_error(sh, "Invalid size. Must be between 16 and %u", MAX_PACKET_SIZE);
		return -EINVAL;
	}

	test_params.packet_size = size;
	shell_print(sh, "Packet size set to %u bytes", size);
	return 0;
}

/* Shell command: set duration */
static int cmd_set_duration(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_error(sh, "Usage: set_duration <seconds>");
		return -EINVAL;
	}

	if (test_params.running) {
		shell_error(sh, "Cannot change settings while test is running");
		return -EBUSY;
	}

	uint32_t duration = strtoul(argv[1], NULL, 10);
	if (duration == 0 || duration > 3600) {
		shell_error(sh, "Invalid duration. Must be between 1 and 3600 seconds");
		return -EINVAL;
	}

	test_params.duration_sec = duration;
	shell_print(sh, "Test duration set to %u seconds", duration);
	return 0;
}

/* Shell command: set delay */
static int cmd_set_delay(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_error(sh, "Usage: set_delay <milliseconds>");
		return -EINVAL;
	}

	if (test_params.running) {
		shell_error(sh, "Cannot change settings while test is running");
		return -EBUSY;
	}

	uint32_t delay = strtoul(argv[1], NULL, 10);
	if (delay > 10000) {
		shell_error(sh, "Delay too large. Maximum is 10000 ms");
		return -EINVAL;
	}
	test_params.delay_ms = delay;
	shell_print(sh, "Packet delay set to %u ms", delay);
	if (delay == 0) {
		shell_print(sh, "Warning: 0ms delay may overwhelm the system at high rates");
	}
	return 0;
}

/* Shell command: set processing delay */
static int cmd_set_processing_delay(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_error(sh, "Usage: set_processing_delay <milliseconds>");
		return -EINVAL;
	}

	if (test_params.running) {
		shell_error(sh, "Cannot change settings while test is running");
		return -EBUSY;
	}

	uint32_t delay = strtoul(argv[1], NULL, 10);
	if (delay > 10000) {
		shell_error(sh, "Delay too large. Maximum is 10000 ms");
		return -EINVAL;
	}
	test_params.processing_delay_ms = delay;
	shell_print(sh, "Server processing delay set to %u ms", delay);
	return 0;
}

/* Shell command: show status */
static int cmd_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Current configuration:");
	shell_print(sh, "  Packet size: %u bytes", test_params.packet_size);
	shell_print(sh, "  Duration: %u seconds", test_params.duration_sec);
	shell_print(sh, "  TX Delay: %u ms", test_params.delay_ms);
	shell_print(sh, "  Server processing delay: %u ms", test_params.processing_delay_ms);
	shell_print(sh, "  Status: %s", test_params.running ? "RUNNING" : "IDLE");
	
	if (test_params.running) {
		/* Read all stats atomically under spinlock */
		k_spinlock_key_t key = k_spin_lock(&stats_lock);
		uint32_t start_time = stats.start_time;
		uint32_t packets_sent = stats.packets_sent;
		uint32_t packets_received = stats.packets_received;
		uint32_t server_rx_count = stats.server_rx_count;
		uint32_t server_tx_count = stats.server_tx_count;
		uint32_t errors = stats.errors;
		k_spin_unlock(&stats_lock, key);

		uint32_t elapsed = (k_uptime_get_32() - start_time) / 1000;
		shell_print(sh, "");
		shell_print(sh, "Current statistics:");
		shell_print(sh, "  Elapsed: %u seconds", elapsed);
		shell_print(sh, "  Client TX: %u packets", packets_sent);
		shell_print(sh, "  Client RX: %u packets", packets_received);
		shell_print(sh, "  Server RX: %u packets", server_rx_count);
		shell_print(sh, "  Server TX: %u packets", server_tx_count);
		shell_print(sh, "  Errors: %u", errors);
	}
	
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(throughput_cmds,
	SHELL_CMD(start, NULL, "Start throughput test", cmd_start),
	SHELL_CMD(stop, NULL, "Stop throughput test", cmd_stop),
	SHELL_CMD(set_size, NULL, "Set packet size (bytes)", cmd_set_size),
	SHELL_CMD(set_duration, NULL, "Set test duration (seconds)", cmd_set_duration),
	SHELL_CMD(set_delay, NULL, "Set delay between TX packets (ms)", cmd_set_delay),
	SHELL_CMD(set_processing_delay, NULL, "Set server processing delay (ms)", cmd_set_processing_delay),
	SHELL_CMD(status, NULL, "Show current status and statistics", cmd_status),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(throughput, &throughput_cmds, "SLIP throughput test commands", NULL);

int main(void)
{
	int ret;

	/* Initialize work queue for auto-stop */
	k_work_init_delayable(&auto_stop_work, auto_stop_work_handler);

	LOG_INF("SLIP Throughput Test (Combined Client/Server)");
	LOG_INF("");

	/* Setup both SLIP interfaces with IPv6 addresses */
	ret = setup_interfaces();
	if (ret < 0) {
		LOG_ERR("Failed to setup SLIP interfaces: %d", ret);
		return -1;
	}

	LOG_INF("");
	LOG_INF("=== SLIP Interfaces Ready ===");
	LOG_INF("cobs0 (client): %s on UART2", SLIP0_IPV6_ADDR);
	LOG_INF("cobs1 (server): %s on UART3", SLIP1_IPV6_ADDR);
	LOG_INF("");
	LOG_INF("Hardware setup required:");
	LOG_INF("  Connect P1.09 (UART2 TX) -> P0.28 (UART3 RX)");
	LOG_INF("  Connect P1.08 (UART2 RX) -> P0.29 (UART3 TX)");
	LOG_INF("");
	LOG_INF("This sample tests SLIP/UART throughput via cross-wired UARTs");
	LOG_INF("Use 'throughput' shell commands to configure and run tests");
	LOG_INF("");
	LOG_INF("Available commands:");
	LOG_INF("  throughput status                    - Show current status");
	LOG_INF("  throughput set_size 1024             - Set packet size");
	LOG_INF("  throughput set_duration 30           - Set test duration");
	LOG_INF("  throughput set_delay 10              - Set TX delay between packets");
	LOG_INF("  throughput set_processing_delay 100  - Set server processing delay");
	LOG_INF("  throughput start                     - Start test");
	LOG_INF("  throughput stop                      - Stop test and show results");

	return 0;
}

