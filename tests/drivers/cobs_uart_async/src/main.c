/*
 * Copyright (c) 2025 Martin Schröder
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/device.h>

LOG_MODULE_REGISTER(slip_sockets_test, LOG_LEVEL_DBG);

/* External reference to COBS Serial L2 */
extern const struct net_l2 _net_l2_COBS_SERIAL;

#define SERVER_PORT 5001
#define CLIENT_PORT 5002
#define TEST_PACKET_SIZE 512
#define TEST_PACKET_COUNT 100
#define RX_TIMEOUT_MS 2000
#define SERVER_IDLE_TIMEOUT_MS 10000

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

/* Test context for each interface
 * 
 * Using IPv6 with link-local addresses on different subnets.
 * IPv6 has proper routing support in Zephyr, unlike IPv4.
 * Using fd00::/64 unique local addresses to avoid loopback optimization.
 */
struct iface_context {
	struct net_if *iface;
	char ip_addr[48];
	const char *name;
};

static struct iface_context if0_ctx = {
	.ip_addr = "fd00:1::1",
	.name = "cobs0",
};

static struct iface_context if1_ctx = {
	.ip_addr = "fd00:2::1",
	.name = "cobs1",
};

/* Test statistics - using atomics for thread safety */
static struct {
	atomic_t packets_sent;
	atomic_t packets_received;
	atomic_t bytes_sent;
	atomic_t bytes_received;
	atomic_t errors;
	atomic_t rx_verified;  /* Successfully verified echo responses */
} test_stats;

/* Thread synchronization */
struct test_context {
	int sock;
	struct sockaddr_in6 server_addr;
	bool tx_done;
	bool rx_done;
	atomic_t active;
};

static struct test_context test_ctx;

/* Semaphore for flow control - RX signals TX when ready for more */
static K_SEM_DEFINE(tx_flow_control, 0, 10);

/* UART emulator cross-wiring for native_sim
 * 
 * On native_sim, we need to programmatically wire the two UART emulators
 * together: uart0 TX → uart1 RX and uart1 TX → uart0 RX.
 * We use TX data ready callbacks to immediately transfer data as it becomes available.
 */
static const struct device *uart0_dev;
static const struct device *uart1_dev;

#define WIRE_BUFFER_SIZE 4096

/* Callback when uart0 has TX data ready - transfer to uart1 RX */
static void uart0_tx_ready(const struct device *dev, size_t size, void *user_data)
{
	ARG_UNUSED(user_data);
	uint8_t buffer[WIRE_BUFFER_SIZE];

	/* Transfer all available data */
	uint32_t bytes = uart_emul_get_tx_data(dev, buffer, MIN(size, sizeof(buffer)));
	if (bytes > 0) {
		uint32_t written = uart_emul_put_rx_data(uart1_dev, buffer, bytes);
		if (written != bytes) {
			LOG_WRN("uart0→uart1: only wrote %u/%u bytes", written, bytes);
		} else {
			LOG_DBG("Wire: uart0→uart1: %u bytes", bytes);
		}
	}
}

/* Callback when uart1 has TX data ready - transfer to uart0 RX */
static void uart1_tx_ready(const struct device *dev, size_t size, void *user_data)
{
	ARG_UNUSED(user_data);
	uint8_t buffer[WIRE_BUFFER_SIZE];

	/* Transfer all available data */
	uint32_t bytes = uart_emul_get_tx_data(dev, buffer, MIN(size, sizeof(buffer)));
	if (bytes > 0) {
		uint32_t written = uart_emul_put_rx_data(uart0_dev, buffer, bytes);
		if (written != bytes) {
			LOG_WRN("uart1→uart0: only wrote %u/%u bytes", written, bytes);
		} else {
			LOG_DBG("Wire: uart1→uart0: %u bytes", bytes);
		}
	}
}

/* Find and configure SLIP interfaces */
static void setup_interfaces(void)
{
	int if_count = 0;

	LOG_INF("Searching for SLIP interfaces...");

	STRUCT_SECTION_FOREACH(net_if, iface) {
		if (net_if_l2(iface) != &_net_l2_COBS_SERIAL) {
			continue;
		}

		LOG_INF("Found SLIP interface %d: %s",
			net_if_get_by_iface(iface),
			net_if_get_device(iface)->name);

		if (if_count == 0) {
			if0_ctx.iface = iface;
			LOG_INF("Assigned to if0_ctx (%s)", if0_ctx.name);
		} else if (if_count == 1) {
			if1_ctx.iface = iface;
			LOG_INF("Assigned to if1_ctx (%s)", if1_ctx.name);
		} else {
			LOG_WRN("Extra SLIP interface found, ignoring");
		}

		if_count++;
	}

	zassert_equal(if_count, 2, "Expected 2 SLIP interfaces, found %d", if_count);
	zassert_not_null(if0_ctx.iface, "SLIP interface 0 not found");
	zassert_not_null(if1_ctx.iface, "SLIP interface 1 not found");

	/* Bring interfaces up first */
	net_if_up(if0_ctx.iface);
	net_if_up(if1_ctx.iface);

	/* Configure IPv6 addresses */
	struct in6_addr addr0, addr1;
	struct net_if_addr *ifaddr0, *ifaddr1;

	zsock_inet_pton(AF_INET6, if0_ctx.ip_addr, &addr0);
	ifaddr0 = net_if_ipv6_addr_add(if0_ctx.iface, &addr0, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr0, "Failed to add IPv6 address to if0");
	LOG_INF("Configured %s with IP: %s", if0_ctx.name, if0_ctx.ip_addr);

	zsock_inet_pton(AF_INET6, if1_ctx.ip_addr, &addr1);
	ifaddr1 = net_if_ipv6_addr_add(if1_ctx.iface, &addr1, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr1, "Failed to add IPv6 address to if1");
	LOG_INF("Configured %s with IP: %s", if1_ctx.name, if1_ctx.ip_addr);

	LOG_INF("Both interfaces are up and configured");
	LOG_INF("if0 ptr: %p", if0_ctx.iface);
	LOG_INF("if1 ptr: %p", if1_ctx.iface);
}

/* Fill packet with predictable pattern */
static void fill_pattern(uint8_t *data, size_t len, uint32_t seq)
{
	memcpy(data, &seq, sizeof(uint32_t));
	for (size_t i = sizeof(uint32_t); i < len; i++) {
		data[i] = (uint8_t)((seq + i) & 0xFF);
	}
}

/* Verify packet pattern */
static bool verify_pattern(const uint8_t *data, size_t len, uint32_t seq)
{
	if (len < sizeof(uint32_t)) {
		return false;
	}

	uint32_t pkt_seq;
	memcpy(&pkt_seq, data, sizeof(uint32_t));

	if (pkt_seq != seq) {
		LOG_ERR("Sequence mismatch: expected %u, got %u", seq, pkt_seq);
		return false;
	}

	for (size_t i = sizeof(uint32_t); i < len; i++) {
		uint8_t expected = (uint8_t)((seq + i) & 0xFF);
		if (data[i] != expected) {
			LOG_ERR("Pattern error at offset %zu", i);
			return false;
		}
	}

	return true;
}

static void log_sockaddr_in6(const char *prefix, const struct sockaddr_in6 *addr)
{
	char ip[INET6_ADDRSTRLEN];

	if (!addr) {
		LOG_INF("%s <null>", prefix);
		return;
	}

	(void)zsock_inet_ntop(AF_INET6, &addr->sin6_addr, ip, sizeof(ip));
	LOG_INF("%s [%s]:%u", prefix, ip, (unsigned int)ntohs(addr->sin6_port));
}

/* Server thread - echoes packets back */
static void server_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int sock;
	struct sockaddr_in6 server_addr, client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	uint8_t buffer[TEST_PACKET_SIZE];
	int ret;

	LOG_INF("Server thread started");

	/* Create UDP socket */
	sock = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock >= 0, "Failed to create server socket");

	/* Bind to if1's address (fd00:2::1) - packets will arrive here via cross-wired UART */
	struct in6_addr if1_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin6_family = AF_INET6;
	zsock_inet_pton(AF_INET6, if1_ctx.ip_addr, &if1_addr);
	server_addr.sin6_addr = if1_addr;
	server_addr.sin6_port = htons(SERVER_PORT);

	ret = zsock_bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
	zassert_ok(ret, "Failed to bind server socket");

	/* Try to bind server to if1 interface to only receive packets on that interface */
	struct net_ifreq if1_req = { 0 };
	/* With CONFIG_NET_INTERFACE_NAME=y, Zephyr expects an interface name
	 * (resolved via net_if_get_by_name()). */
	(void)net_if_get_name(if1_ctx.iface, if1_req.ifr_name, sizeof(if1_req.ifr_name));

	ret = zsock_setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE,
			       &if1_req, sizeof(if1_req));
	if (ret < 0) {
		LOG_WRN("SO_BINDTODEVICE not supported for server (errno=%d)", errno);
	} else {
		LOG_INF("Server bound to device %s (if1)", if1_req.ifr_name);
	}

	LOG_INF("Server listening on [%s]:%d (if1)", if1_ctx.ip_addr, SERVER_PORT);

	/* Echo loop */
	int64_t last_rx = k_uptime_get();
	while (atomic_get(&test_stats.packets_received) < TEST_PACKET_COUNT) {
		client_addr_len = sizeof(client_addr);
		ret = recvfrom_with_timeout(sock, buffer, sizeof(buffer),
					    (struct sockaddr *)&client_addr,
					    &client_addr_len,
					    RX_TIMEOUT_MS);
	
		if (ret < 0) {
			if (errno == EAGAIN || errno == ETIMEDOUT) {
				if ((k_uptime_get() - last_rx) > SERVER_IDLE_TIMEOUT_MS) {
					LOG_ERR("Server timed out waiting for traffic");
					atomic_inc(&test_stats.errors);
					break;
				}
				continue;
			}

			LOG_ERR("Server recvfrom failed: %d", errno);
			atomic_inc(&test_stats.errors);
			break;
		}

		last_rx = k_uptime_get();
		atomic_inc(&test_stats.packets_received);
		atomic_add(&test_stats.bytes_received, ret);

		LOG_DBG("Server received packet %u, size %d",
			atomic_get(&test_stats.packets_received), ret);
		log_sockaddr_in6("Server RX from", &client_addr);

		/* Echo back */
		ret = zsock_sendto(sock, buffer, ret, 0,
				   (struct sockaddr *)&client_addr, client_addr_len);
		
		if (ret < 0) {
			LOG_ERR("Server sendto failed: %d", errno);
			atomic_inc(&test_stats.errors);
		} else {
			log_sockaddr_in6("Server TX to", &client_addr);
		}
	}

	zsock_close(sock);
	LOG_INF("Server thread finished");
}

/* Server thread with very large stack and high priority for preemption */
K_THREAD_DEFINE(server_tid, 32768, server_thread, NULL, NULL, NULL, 3, 0, 0);

/* TX thread - continuously sends packets with flow control */
static void client_tx_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	uint8_t tx_buffer[TEST_PACKET_SIZE];
	int ret;

	LOG_INF("TX thread started");

	/* Prime the pump - allow initial burst of packets */
	for (int i = 0; i < 10; i++) {
		k_sem_give(&tx_flow_control);
	}

	for (uint32_t seq = 0; seq < TEST_PACKET_COUNT; seq++) {
		if (!atomic_get(&test_ctx.active)) {
			break;
		}

		/* Wait for flow control signal from RX (or timeout to prevent deadlock) */
		ret = k_sem_take(&tx_flow_control, K_MSEC(100));
		if (ret != 0 && ret != -EAGAIN) {
			/* Timeout is OK - just means RX is slow, send anyway */
			LOG_DBG("TX flow control timeout on seq %u", seq);
		}

		/* Fill packet with pattern */
		fill_pattern(tx_buffer, TEST_PACKET_SIZE, seq);

		/* Send to server */
		ret = zsock_sendto(test_ctx.sock, tx_buffer, TEST_PACKET_SIZE, 0,
				   (struct sockaddr *)&test_ctx.server_addr,
				   sizeof(test_ctx.server_addr));

		if (ret < 0) {
			LOG_ERR("Client sendto failed: %d", errno);
			atomic_inc(&test_stats.errors);
			continue;
		}

		atomic_inc(&test_stats.packets_sent);
		atomic_add(&test_stats.bytes_sent, ret);

		if ((seq + 1) % 20 == 0) {
			LOG_INF("TX Progress: %u/%u packets", seq + 1, TEST_PACKET_COUNT);
		}

		/* Explicit sleep to allow system to process - prevents buffer overflow */
		k_sleep(K_USEC(100));
	}

	test_ctx.tx_done = true;
	LOG_INF("TX thread finished: sent %u packets", atomic_get(&test_stats.packets_sent));
}

/* RX thread - receives and verifies echo responses */
static void client_rx_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	uint8_t rx_buffer[TEST_PACKET_SIZE];
	struct sockaddr_in6 peer;
	socklen_t peer_len;
	int ret;
	uint32_t consecutive_timeouts = 0;

	LOG_INF("RX thread started");

	while (atomic_get(&test_ctx.active)) {
		peer_len = sizeof(peer);
		memset(&peer, 0, sizeof(peer));

		ret = recvfrom_with_timeout(test_ctx.sock, rx_buffer, sizeof(rx_buffer),
					    (struct sockaddr *)&peer, &peer_len,
					    RX_TIMEOUT_MS);

		if (ret < 0) {
			if (errno == EAGAIN || errno == ETIMEDOUT) {
				consecutive_timeouts++;
				
				/* If TX is done and we've had multiple timeouts, we're probably done */
				if (test_ctx.tx_done && consecutive_timeouts > 5) {
					LOG_INF("RX thread: TX done and multiple timeouts, finishing");
					break;
				}
				
				/* Continue waiting for more packets */
				continue;
			}

			LOG_ERR("Client recvfrom failed: %d", errno);
			atomic_inc(&test_stats.errors);
			continue;
		}

		consecutive_timeouts = 0;

		/* We received a packet */
		if (ret != TEST_PACKET_SIZE) {
			LOG_ERR("Echo size mismatch: got %d, expected %d", ret, TEST_PACKET_SIZE);
			atomic_inc(&test_stats.errors);
			continue;
		}

		/* Extract sequence number from packet */
		uint32_t seq;
		memcpy(&seq, rx_buffer, sizeof(uint32_t));

		/* Verify pattern */
		if (!verify_pattern(rx_buffer, ret, seq)) {
			LOG_ERR("Pattern verification failed for seq %u", seq);
			atomic_inc(&test_stats.errors);
			continue;
		}

		atomic_inc(&test_stats.rx_verified);

		/* Signal TX that we've successfully processed a packet (flow control) */
		k_sem_give(&tx_flow_control);

		if (atomic_get(&test_stats.rx_verified) % 20 == 0) {
			LOG_INF("RX Progress: verified %u packets",
				atomic_get(&test_stats.rx_verified));
		}

		/* Check if we've received all expected packets */
		if (atomic_get(&test_stats.rx_verified) >= TEST_PACKET_COUNT) {
			LOG_INF("RX thread: received all expected packets");
			break;
		}
	}

	test_ctx.rx_done = true;
	LOG_INF("RX thread finished: verified %u packets", atomic_get(&test_stats.rx_verified));
}

/* Thread stacks and IDs
 *
 * Threading Model for Parallel TX/RX:
 * =====================================
 * 
 * Priority Levels (lower number = higher priority):
 * - Server: Priority 3 (highest) - needs to respond to packets immediately
 * - Client RX: Priority 4 - preempts TX when data arrives
 * - Client TX: Priority 6 - lower priority, allows RX to preempt
 *
 * Flow Control:
 * - TX thread waits on semaphore before sending each packet
 * - RX thread signals semaphore after successfully receiving packet
 * - This creates backpressure to prevent overwhelming UART buffers
 * - TX also has explicit sleep to allow system processing time
 *
 * This model ensures:
 * 1. RX can always preempt TX when data arrives (higher priority)
 * 2. TX doesn't overwhelm buffers (flow control + sleep)
 * 3. Server can always respond immediately (highest priority)
 * 4. Both TX and RX loops can make progress concurrently
 */
#define CLIENT_TX_STACK_SIZE 4096
#define CLIENT_RX_STACK_SIZE 4096
#define CLIENT_TX_PRIORITY 6
#define CLIENT_RX_PRIORITY 4

static K_THREAD_STACK_DEFINE(client_tx_stack, CLIENT_TX_STACK_SIZE);
static K_THREAD_STACK_DEFINE(client_rx_stack, CLIENT_RX_STACK_SIZE);
static struct k_thread client_tx_thread_data;
static struct k_thread client_rx_thread_data;
static k_tid_t client_tx_tid;
static k_tid_t client_rx_tid;

/* Test: UDP echo over SLIP interfaces with separate TX/RX threads */
ZTEST(slip_sockets, test_udp_echo)
{
	int ret;
	struct timeval timeout;

	LOG_INF("=== UDP Echo Test (Parallel TX/RX) ===");
	LOG_INF("Sending %d packets of %d bytes", TEST_PACKET_COUNT, TEST_PACKET_SIZE);

	/* Reset stats and context */
	memset(&test_stats, 0, sizeof(test_stats));
	memset(&test_ctx, 0, sizeof(test_ctx));
	atomic_set(&test_ctx.active, 1);

	/* Give server time to start */
	k_sleep(K_MSEC(500));

	/* Create client socket */
	test_ctx.sock = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(test_ctx.sock >= 0, "Failed to create client socket");

	/* Bind client to if0's address (fd00:1::1) */
	struct sockaddr_in6 client_bind_addr;
	memset(&client_bind_addr, 0, sizeof(client_bind_addr));
	client_bind_addr.sin6_family = AF_INET6;
	zsock_inet_pton(AF_INET6, if0_ctx.ip_addr, &client_bind_addr.sin6_addr);
	client_bind_addr.sin6_port = 0; /* Let system assign port */
	ret = zsock_bind(test_ctx.sock, (struct sockaddr *)&client_bind_addr,
			 sizeof(client_bind_addr));
	zassert_ok(ret, "Failed to bind client to if0 address");

	/* Log the actual local port chosen */
	struct sockaddr_in6 client_actual;
	socklen_t client_actual_len = sizeof(client_actual);
	memset(&client_actual, 0, sizeof(client_actual));
	ret = zsock_getsockname(test_ctx.sock, (struct sockaddr *)&client_actual,
				&client_actual_len);
	if (ret == 0) {
		log_sockaddr_in6("Client bound local", &client_actual);
	}

	/* Try to force socket to use if0 for transmission via SO_BINDTODEVICE */
	struct net_ifreq if0_req = { 0 };
	(void)net_if_get_name(if0_ctx.iface, if0_req.ifr_name, sizeof(if0_req.ifr_name));

	ret = zsock_setsockopt(test_ctx.sock, SOL_SOCKET, SO_BINDTODEVICE,
			       &if0_req, sizeof(if0_req));
	if (ret < 0) {
		LOG_WRN("SO_BINDTODEVICE not supported for client (errno=%d), "
			"relying on source address binding", errno);
	} else {
		LOG_INF("Client bound to device %s (if0)", if0_req.ifr_name);
	}
	LOG_INF("Client: %s (if0) → %s (if1)", if0_ctx.ip_addr, if1_ctx.ip_addr);

	/* Set receive timeout */
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	ret = zsock_setsockopt(test_ctx.sock, SOL_SOCKET, SO_RCVTIMEO,
			       &timeout, sizeof(timeout));
	if (ret < 0) {
		LOG_WRN("Socket timeout not supported, continuing without timeout");
	}

	/* Setup server address - send to if1's address (fd00::2) */
	memset(&test_ctx.server_addr, 0, sizeof(test_ctx.server_addr));
	test_ctx.server_addr.sin6_family = AF_INET6;
	test_ctx.server_addr.sin6_port = htons(SERVER_PORT);
	ret = zsock_inet_pton(AF_INET6, if1_ctx.ip_addr, &test_ctx.server_addr.sin6_addr);
	zassert_equal(ret, 1, "Invalid server IP address");

	LOG_INF("Client sending to server at [%s]:%d", if1_ctx.ip_addr, SERVER_PORT);

	/* Start measurement */
	int64_t start_time = k_uptime_get();

	/* Create and start TX and RX threads */
	client_tx_tid = k_thread_create(&client_tx_thread_data, client_tx_stack,
					K_THREAD_STACK_SIZEOF(client_tx_stack),
					client_tx_thread, NULL, NULL, NULL,
					CLIENT_TX_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(client_tx_tid, "client_tx");

	client_rx_tid = k_thread_create(&client_rx_thread_data, client_rx_stack,
					K_THREAD_STACK_SIZEOF(client_rx_stack),
					client_rx_thread, NULL, NULL, NULL,
					CLIENT_RX_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(client_rx_tid, "client_rx");

	/* Wait for both threads to complete */
	k_thread_join(client_tx_tid, K_FOREVER);
	LOG_INF("TX thread joined");
	
	k_thread_join(client_rx_tid, K_FOREVER);
	LOG_INF("RX thread joined");

	int64_t elapsed_ms = k_uptime_delta(&start_time);

	/* Stop any remaining activity */
	atomic_set(&test_ctx.active, 0);

	zsock_close(test_ctx.sock);

	/* Wait for server to finish */
	k_sleep(K_MSEC(500));

	/* Print statistics */
	LOG_INF("=== Test Results ===");
	LOG_INF("Packets sent: %u", atomic_get(&test_stats.packets_sent));
	LOG_INF("Packets verified: %u", atomic_get(&test_stats.rx_verified));
	LOG_INF("Server packets received: %u", test_stats.packets_received);
	LOG_INF("Bytes sent: %u", atomic_get(&test_stats.bytes_sent));
	LOG_INF("Bytes received: %u", test_stats.bytes_received);
	LOG_INF("Errors: %u", atomic_get(&test_stats.errors));
	LOG_INF("Elapsed time: %lld ms", elapsed_ms);
	
	if (elapsed_ms > 0) {
		uint32_t throughput_kbps = (atomic_get(&test_stats.bytes_sent) * 8) / elapsed_ms;
		LOG_INF("TX Throughput: %u kbps", throughput_kbps);
	}

	/* Verify results */
	zassert_equal(atomic_get(&test_stats.packets_sent), TEST_PACKET_COUNT,
		      "Not all packets were sent");
	zassert_true(atomic_get(&test_stats.rx_verified) >= (TEST_PACKET_COUNT * 9 / 10),
		     "Too few packets verified: %u", atomic_get(&test_stats.rx_verified));
	zassert_true(atomic_get(&test_stats.errors) < (TEST_PACKET_COUNT / 10),
		     "Too many errors: %u", atomic_get(&test_stats.errors));

	LOG_INF("Test passed!");
}

/* Test: Verify interfaces exist and are configured */
ZTEST(slip_sockets, test_interfaces_configured)
{
	zassert_not_null(if0_ctx.iface, "Interface 0 not configured");
	zassert_not_null(if1_ctx.iface, "Interface 1 not configured");
	
	zassert_true(net_if_is_up(if0_ctx.iface), "Interface 0 is not up");
	zassert_true(net_if_is_up(if1_ctx.iface), "Interface 1 is not up");

	/* Verify L2 type */
	zassert_equal(net_if_l2(if0_ctx.iface), &_net_l2_COBS_SERIAL,
		      "Interface 0 has wrong L2");
	zassert_equal(net_if_l2(if1_ctx.iface), &_net_l2_COBS_SERIAL,
		      "Interface 1 has wrong L2");

	LOG_INF("Both interfaces configured correctly");
}

/* Test: Verify IP addresses */
ZTEST(slip_sockets, test_ip_addresses)
{
	struct net_if_ipv6 *ipv6_0, *ipv6_1;
	char addr_str[INET6_ADDRSTRLEN];

	ipv6_0 = if0_ctx.iface->config.ip.ipv6;
	zassert_not_null(ipv6_0, "Interface 0 has no IPv6 config");
	zassert_false(net_ipv6_is_addr_unspecified(&ipv6_0->unicast[0].address.in6_addr),
		      "Interface 0 has no IPv6 address");

	zsock_inet_ntop(AF_INET6, &ipv6_0->unicast[0].address.in6_addr, addr_str, sizeof(addr_str));
	LOG_INF("Interface 0 IP: %s", addr_str);
	zassert_equal(strcmp(addr_str, if0_ctx.ip_addr), 0,
		      "Interface 0 IP mismatch");

	ipv6_1 = if1_ctx.iface->config.ip.ipv6;
	zassert_not_null(ipv6_1, "Interface 1 has no IPv6 config");
	zassert_false(net_ipv6_is_addr_unspecified(&ipv6_1->unicast[0].address.in6_addr),
		      "Interface 1 has no IPv6 address");

	zsock_inet_ntop(AF_INET6, &ipv6_1->unicast[0].address.in6_addr, addr_str, sizeof(addr_str));
	LOG_INF("Interface 1 IP: %s", addr_str);
	zassert_equal(strcmp(addr_str, if1_ctx.ip_addr), 0,
		      "Interface 1 IP mismatch");

	LOG_INF("IP addresses configured correctly");
}

/* Test suite setup */
static void *slip_sockets_setup(void)
{
	LOG_INF("Setting up SLIP sockets test...");
	
	/* Initialize UART emulator devices for cross-wiring */
	uart0_dev = DEVICE_DT_GET(DT_NODELABEL(uart_emul0));
	uart1_dev = DEVICE_DT_GET(DT_NODELABEL(uart_emul1));
	
	zassert_true(device_is_ready(uart0_dev), "uart_emul0 not ready");
	zassert_true(device_is_ready(uart1_dev), "uart_emul1 not ready");
	
	/* Register TX callbacks to cross-wire the UARTs */
	uart_emul_callback_tx_data_ready_set(uart0_dev, uart0_tx_ready, NULL);
	uart_emul_callback_tx_data_ready_set(uart1_dev, uart1_tx_ready, NULL);
	
	LOG_INF("UART emulators cross-wired: uart0↔uart1");
	
	setup_interfaces();
	LOG_INF("Setup complete");
	return NULL;
}

ZTEST_SUITE(slip_sockets, NULL, slip_sockets_setup, NULL, NULL, NULL);
