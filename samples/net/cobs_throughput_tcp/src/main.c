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

LOG_MODULE_REGISTER(slip_tcp_throughput, LOG_LEVEL_INF);

/* External reference to COBS Serial L2 */
extern const struct net_l2 _net_l2_COBS_SERIAL;

#define SERVER_PORT 5001
#define BUFFER_SIZE 4096

/* IPv6 addresses for the two SLIP interfaces */
#define SLIP0_IPV6_ADDR "fd00:1::1"  /* Client interface */
#define SLIP1_IPV6_ADDR "fd00:2::1"  /* Server interface */

/* Interface contexts */
static struct net_if *slip0_iface;  /* Client interface */
static struct net_if *slip1_iface;  /* Server interface */

/* Test parameters */
static struct {
	uint32_t duration_sec;
	uint32_t chunk_size;
	uint32_t delay_ms;
	uint32_t processing_delay_ms;
	bool running;
	bool stop_requested;
} test_params = {
	.duration_sec = 10,
	.chunk_size = 1024,
	.delay_ms = 0,
	.processing_delay_ms = 100,
	.running = false,
	.stop_requested = false,
};

/* Statistics - protected by mutex for thread safety */
static struct k_mutex stats_lock;
static struct {
	uint64_t bytes_sent;
	uint64_t bytes_received;
	uint64_t bytes_echoed;
	uint32_t errors;
	uint32_t start_time;
	uint32_t last_progress_time;
} stats;

/* Thread stacks and IDs */
#define SERVER_STACK_SIZE 6144
#define CLIENT_STACK_SIZE 10240
#define SERVER_PRIORITY 6
#define CLIENT_PRIORITY 5

static K_THREAD_STACK_DEFINE(server_stack, SERVER_STACK_SIZE);
static K_THREAD_STACK_DEFINE(client_stack, CLIENT_STACK_SIZE);
static struct k_thread server_thread_data;
static struct k_thread client_thread_data;
static k_tid_t server_tid;
static k_tid_t client_tid;

/* Sockets */
static int server_sock = -1;
static int accept_sock = -1;
static int client_sock = -1;

/* Thread completion flags */
static volatile bool client_finished = false;
static volatile bool server_finished = false;

/* Progress reporting work */
static struct k_work_delayable progress_work;

/* TCP recv with complete read - blocking mode */
static int tcp_recv_complete(int sock, void *buf, size_t len)
{
	size_t received = 0;
	int ret;

	while (received < len && !test_params.stop_requested) {
		ret = zsock_recv(sock, (uint8_t *)buf + received, len - received, 0);
		if (ret < 0) {
			/* Return error and let caller decide how to handle it */
			return ret;
		}
		if (ret == 0) {
			/* Connection closed - return what we've received so far */
			/* If nothing received yet, return 0 to indicate EOF */
			return (received > 0) ? (int)received : 0;
		}
		received += ret;
	}
	
	if (test_params.stop_requested && received < len) {
		return -EINTR;
	}
	
	return received;
}

/* TCP send with complete write - blocking mode */
static int tcp_send_complete(int sock, const void *buf, size_t len)
{
	size_t sent = 0;
	int ret;

	while (sent < len && !test_params.stop_requested) {
		ret = zsock_send(sock, (const uint8_t *)buf + sent, len - sent, 0);
		if (ret < 0) {
			/* Return error and let caller decide how to handle it */
			return ret;
		}
		sent += ret;
	}
	
	if (test_params.stop_requested && sent < len) {
		return -EINTR;
	}
	
	return sent;
}

/* Forward declaration */
static int throughput_stop(void);

/* Print progress every second and auto-stop when threads finish */
static void progress_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	
	if (!test_params.running) {
		return;
	}
	
	k_mutex_lock(&stats_lock, K_FOREVER);
	uint32_t elapsed = (k_uptime_get_32() - stats.start_time) / 1000;
	uint64_t bytes_sent = stats.bytes_sent;
	uint64_t bytes_received = stats.bytes_received;
	uint32_t errors = stats.errors;
	k_mutex_unlock(&stats_lock);
	
	if (elapsed > 0) {
		uint64_t tx_kbps = (bytes_sent * 8) / (elapsed * 1000);
		uint64_t rx_kbps = (bytes_received * 8) / (elapsed * 1000);
		
		LOG_INF("Progress: %us | TX: %llu KB (%llu kbps) | RX: %llu KB (%llu kbps) | Errors: %u",
			elapsed, bytes_sent / 1024, tx_kbps,
			bytes_received / 1024, rx_kbps, errors);
	}
	
	/* Check if both threads have finished */
	if (client_finished && server_finished) {
		LOG_INF("Both threads finished, stopping test");
		throughput_stop();
		return;
	}
	
	/* Reschedule for next second */
	k_work_schedule(&progress_work, K_SECONDS(1));
}

/* Print final statistics */
static void print_final_stats(void)
{
	k_mutex_lock(&stats_lock, K_FOREVER);
	uint32_t elapsed = (k_uptime_get_32() - stats.start_time) / 1000;
	uint64_t bytes_sent = stats.bytes_sent;
	uint64_t bytes_received = stats.bytes_received;
	uint64_t bytes_echoed = stats.bytes_echoed;
	uint32_t errors = stats.errors;
	k_mutex_unlock(&stats_lock);

	if (elapsed == 0) {
		elapsed = 1;
	}

	uint64_t tx_kbps = (bytes_sent * 8) / (elapsed * 1000);
	uint64_t rx_kbps = (bytes_received * 8) / (elapsed * 1000);

	LOG_INF("");
	LOG_INF("=== Final Statistics ===");
	LOG_INF("Test duration: %u seconds", elapsed);
	LOG_INF("Chunk size: %u bytes", test_params.chunk_size);
	LOG_INF("");
	LOG_INF("Client TX: %llu bytes (%llu KB) (%llu kbps)",
		bytes_sent, bytes_sent / 1024, tx_kbps);
	LOG_INF("Client RX: %llu bytes (%llu KB) (%llu kbps)",
		bytes_received, bytes_received / 1024, rx_kbps);
	LOG_INF("Server Echo: %llu bytes (%llu KB)",
		bytes_echoed, bytes_echoed / 1024);
	LOG_INF("");
	LOG_INF("Errors: %u", errors);
	LOG_INF("========================");
}

/* Server thread - accepts connection and echoes data back */
static void server_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct sockaddr_in6 server_addr, client_addr;
	socklen_t client_addr_len;
	uint8_t buffer[BUFFER_SIZE];
	int ret;
	struct in6_addr slip1_addr;

	LOG_INF("Server thread started");

	/* Wait for cobs1 interface to be up */
	if (!slip1_iface) {
		LOG_ERR("Server: cobs1 interface not found");
		k_mutex_lock(&stats_lock, K_FOREVER);
		stats.errors++;
		k_mutex_unlock(&stats_lock);
		return;
	}

	int wait_count = 50; /* 5 seconds */
	while (!net_if_is_up(slip1_iface) && wait_count > 0) {
		k_sleep(K_MSEC(100));
		wait_count--;
	}

	if (!net_if_is_up(slip1_iface)) {
		LOG_ERR("Server: Timeout waiting for cobs1 interface");
		k_mutex_lock(&stats_lock, K_FOREVER);
		stats.errors++;
		k_mutex_unlock(&stats_lock);
		return;
	}

	/* Create TCP socket */
	server_sock = zsock_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	if (server_sock < 0) {
		LOG_ERR("Server: Failed to create socket: %d", errno);
		k_mutex_lock(&stats_lock, K_FOREVER);
		stats.errors++;
		k_mutex_unlock(&stats_lock);
		return;
	}

	/* Set socket options */
	int optval = 1;
	zsock_setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

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
		k_mutex_lock(&stats_lock, K_FOREVER);
		stats.errors++;
		k_mutex_unlock(&stats_lock);
		return;
	}

	/* Listen for connections */
	ret = zsock_listen(server_sock, 1);
	if (ret < 0) {
		LOG_ERR("Server: Failed to listen: %d", errno);
		zsock_close(server_sock);
		server_sock = -1;
		k_mutex_lock(&stats_lock, K_FOREVER);
		stats.errors++;
		k_mutex_unlock(&stats_lock);
		return;
	}

	LOG_INF("Server listening on [%s]:%d", SLIP1_IPV6_ADDR, SERVER_PORT);

	/* Accept connection */
	client_addr_len = sizeof(client_addr);
	accept_sock = zsock_accept(server_sock, (struct sockaddr *)&client_addr,
				   &client_addr_len);
	if (accept_sock < 0) {
		LOG_ERR("Server: Failed to accept connection: %d", errno);
		zsock_close(server_sock);
		server_sock = -1;
		k_mutex_lock(&stats_lock, K_FOREVER);
		stats.errors++;
		k_mutex_unlock(&stats_lock);
		return;
	}

	LOG_INF("Server: Connection accepted");

	/* Set receive timeout on accepted socket to avoid hanging
	 * Use shorter timeout to detect client disconnection faster
	 */
	struct timeval timeout;
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	ret = zsock_setsockopt(accept_sock, SOL_SOCKET, SO_RCVTIMEO,
			       &timeout, sizeof(timeout));
	if (ret < 0) {
		LOG_WRN("Server: Failed to set socket timeout: %d", errno);
	}

	/* Disable Nagle's algorithm for lower latency */
	int nodelay = 1;
	ret = zsock_setsockopt(accept_sock, IPPROTO_TCP, TCP_NODELAY,
			       &nodelay, sizeof(nodelay));
	if (ret < 0) {
		LOG_WRN("Server: Failed to set TCP_NODELAY: %d", errno);
	}

	/* Echo loop */
	while (!test_params.stop_requested) {
		ret = tcp_recv_complete(accept_sock, buffer, test_params.chunk_size);
		if (ret <= 0) {
			if (ret == 0) {
				/* Connection closed by peer - this is normal */
				LOG_INF("Server: Connection closed by client (received FIN)");
			} else if (ret < 0) {
				/* Check if it's an expected condition */
				if (errno == ETIMEDOUT || errno == EINTR || 
				    errno == EAGAIN || errno == EWOULDBLOCK) {
					LOG_INF("Server: Timeout waiting for data (client likely disconnected)");
				} else {
					LOG_ERR("Server: recv failed: %d (errno: %d)", ret, errno);
					k_mutex_lock(&stats_lock, K_FOREVER);
					stats.errors++;
					k_mutex_unlock(&stats_lock);
				}
			}
			break;
		}

		/* Echo back */
		ret = tcp_send_complete(accept_sock, buffer, ret);
		if (ret < 0) {
			LOG_ERR("Server: send failed: %d (errno: %d)", ret, errno);
			k_mutex_lock(&stats_lock, K_FOREVER);
			stats.errors++;
			k_mutex_unlock(&stats_lock);
			break;
		}

		k_mutex_lock(&stats_lock, K_FOREVER);
		stats.bytes_echoed += ret;
		k_mutex_unlock(&stats_lock);
		
		/* Simulate server processing work if configured */
		if (test_params.processing_delay_ms > 0) {
			k_sleep(K_MSEC(test_params.processing_delay_ms));
		}
	}

	/* Close the accepted connection gracefully */
	LOG_INF("Server: Shutting down connection");
	ret = zsock_shutdown(accept_sock, ZSOCK_SHUT_RDWR);
	if (ret < 0 && errno != ENOTCONN) {
		LOG_DBG("Server: shutdown failed: %d (errno: %d)", ret, errno);
	}
	
	LOG_INF("Server: Closing accepted socket");
	zsock_close(accept_sock);
	accept_sock = -1;
	
	/* Close the listening socket */
	LOG_INF("Server: Closing listening socket");
	zsock_close(server_sock);
	server_sock = -1;
	
	LOG_INF("Server thread finished");
	server_finished = true;
}

/* Client thread - connects and sends/receives data */
static void client_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	uint8_t tx_buffer[BUFFER_SIZE];
	uint8_t rx_buffer[BUFFER_SIZE];
	struct sockaddr_in6 server_addr;
	struct in6_addr slip1_addr;
	int ret;
	uint8_t byte_counter = 0;
	uint32_t test_end_time;

	LOG_INF("Client thread started");

	/* Give server time to start */
	k_sleep(K_MSEC(500));

	/* Setup server address */
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin6_family = AF_INET6;
	server_addr.sin6_port = htons(SERVER_PORT);
	ret = zsock_inet_pton(AF_INET6, SLIP1_IPV6_ADDR, &slip1_addr);
	if (ret != 1) {
		LOG_ERR("Client: Invalid server IP address");
		k_mutex_lock(&stats_lock, K_FOREVER);
		stats.errors++;
		k_mutex_unlock(&stats_lock);
		return;
	}
	server_addr.sin6_addr = slip1_addr;

	/* Connect to server */
	ret = zsock_connect(client_sock, (struct sockaddr *)&server_addr,
			    sizeof(server_addr));
	if (ret < 0) {
		LOG_ERR("Client: Failed to connect: %d", errno);
		k_mutex_lock(&stats_lock, K_FOREVER);
		stats.errors++;
		k_mutex_unlock(&stats_lock);
		return;
	}

	LOG_INF("Client: Connected to server");

	/* Disable Nagle's algorithm for lower latency */
	int nodelay = 1;
	ret = zsock_setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY,
			       &nodelay, sizeof(nodelay));
	if (ret < 0) {
		LOG_WRN("Client: Failed to set TCP_NODELAY: %d", errno);
	}

	/* Fill tx buffer with incrementing byte pattern */
	for (size_t i = 0; i < test_params.chunk_size; i++) {
		tx_buffer[i] = byte_counter++;
	}

	test_end_time = k_uptime_get_32() + (test_params.duration_sec * 1000);

	/* Main transfer loop */
	while (!test_params.stop_requested && k_uptime_get_32() < test_end_time) {
		/* Send chunk */
		ret = tcp_send_complete(client_sock, tx_buffer, test_params.chunk_size);
		if (ret < 0) {
			LOG_ERR("Client: send failed: %d (errno: %d)", ret, errno);
			k_mutex_lock(&stats_lock, K_FOREVER);
			stats.errors++;
			k_mutex_unlock(&stats_lock);
			break;
		}

		k_mutex_lock(&stats_lock, K_FOREVER);
		stats.bytes_sent += ret;
		k_mutex_unlock(&stats_lock);

		/* Receive echo */
		ret = tcp_recv_complete(client_sock, rx_buffer, test_params.chunk_size);
		if (ret <= 0) {
			if (ret == 0) {
				LOG_INF("Client: Connection closed by server (received FIN)");
			} else if (ret < 0) {
				if (errno == ETIMEDOUT || errno == EINTR || 
				    errno == EAGAIN || errno == EWOULDBLOCK) {
					LOG_INF("Client: Timeout or connection closed");
				} else {
					LOG_ERR("Client: recv failed: %d (errno: %d)", ret, errno);
					k_mutex_lock(&stats_lock, K_FOREVER);
					stats.errors++;
					k_mutex_unlock(&stats_lock);
				}
			}
			break;
		}

		k_mutex_lock(&stats_lock, K_FOREVER);
		stats.bytes_received += ret;
		k_mutex_unlock(&stats_lock);

		/* Verify pattern */
		bool error_found = false;
		for (size_t i = 0; i < test_params.chunk_size; i++) {
			if (rx_buffer[i] != tx_buffer[i]) {
				if (!error_found) {
					LOG_ERR("Client: Pattern mismatch at offset %zu: "
						"expected 0x%02x, got 0x%02x",
						i, tx_buffer[i], rx_buffer[i]);
					k_mutex_lock(&stats_lock, K_FOREVER);
					stats.errors++;
					k_mutex_unlock(&stats_lock);
					error_found = true;
				}
			}
		}

		/* Update pattern for next iteration */
		for (size_t i = 0; i < test_params.chunk_size; i++) {
			tx_buffer[i] = byte_counter++;
		}

		/* Delay between chunks if configured */
		if (test_params.delay_ms > 0) {
			k_sleep(K_MSEC(test_params.delay_ms));
		}
	}

	/* Properly shutdown connection with graceful TCP teardown */
	LOG_INF("Client: Shutting down socket (sending FIN)");
	ret = zsock_shutdown(client_sock, ZSOCK_SHUT_WR);
	if (ret < 0) {
		/* ENOTCONN is not necessarily an error - socket might already be closing */
		if (errno == ENOTCONN) {
			LOG_INF("Client: Socket already disconnected");
		} else {
			LOG_ERR("Client: shutdown failed: %d (errno: %d)", ret, errno);
		}
	}
	
	/* Complete the TCP teardown by reading until EOF (server's FIN)
	 * This ensures proper connection closure and allows server to detect EOF
	 */
	LOG_INF("Client: Draining remaining data until EOF");
	uint8_t drain_buf[256];
	int drain_count = 0;
	struct timeval drain_timeout;
	drain_timeout.tv_sec = 2;
	drain_timeout.tv_usec = 0;
	zsock_setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO,
			 &drain_timeout, sizeof(drain_timeout));
	
	while (drain_count++ < 100) { /* Limit iterations to prevent infinite loop */
		ret = zsock_recv(client_sock, drain_buf, sizeof(drain_buf), 0);
		if (ret == 0) {
			LOG_INF("Client: Received EOF from server (proper close)");
			break;
		}
		if (ret < 0) {
			if (errno == ETIMEDOUT || errno == EAGAIN || errno == EWOULDBLOCK) {
				LOG_INF("Client: Timeout waiting for server EOF");
			} else {
				LOG_DBG("Client: Error draining socket: %d (errno: %d)", ret, errno);
			}
			break;
		}
		/* Server sent more data, discard it */
		LOG_DBG("Client: Drained %d bytes", ret);
	}
	
	LOG_INF("Client: Closing socket");
	zsock_close(client_sock);
	client_sock = -1;

	LOG_INF("Client thread finished");
	client_finished = true;
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

		if (if_count == 0) {
			slip0_iface = iface;
		} else if (if_count == 1) {
			slip1_iface = iface;
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

	zsock_inet_pton(AF_INET6, SLIP1_IPV6_ADDR, &addr1);
	ifaddr1 = net_if_ipv6_addr_add(slip1_iface, &addr1, NET_ADDR_MANUAL, 0);
	if (!ifaddr1) {
		LOG_ERR("Failed to add IPv6 address to cobs1");
		return -ENOMEM;
	}

	/* Add IPv6 prefixes for routing */
	struct in6_addr prefix0, prefix1;
	zsock_inet_pton(AF_INET6, "fd00:1::", &prefix0);
	struct net_if_ipv6_prefix *prefix0_ptr =
		net_if_ipv6_prefix_add(slip0_iface, &prefix0, 64, 0);
	if (!prefix0_ptr) {
		LOG_ERR("Failed to add IPv6 prefix to cobs0");
		return -ENOMEM;
	}
	net_if_ipv6_prefix_set_lf(prefix0_ptr, false);

	zsock_inet_pton(AF_INET6, "fd00:2::", &prefix1);
	struct net_if_ipv6_prefix *prefix1_ptr =
		net_if_ipv6_prefix_add(slip1_iface, &prefix1, 64, 0);
	if (!prefix1_ptr) {
		LOG_ERR("Failed to add IPv6 prefix to cobs1");
		return -ENOMEM;
	}
	net_if_ipv6_prefix_set_lf(prefix1_ptr, false);

	/* Wait for both interfaces to be up */
	int setup_timeout = 50; /* 5 seconds */
	while ((!net_if_is_up(slip0_iface) || !net_if_is_up(slip1_iface)) &&
	       setup_timeout > 0) {
		k_sleep(K_MSEC(100));
		setup_timeout--;
	}

	if (!net_if_is_up(slip0_iface) || !net_if_is_up(slip1_iface)) {
		LOG_ERR("Timeout waiting for SLIP interfaces to come up");
		return -ETIMEDOUT;
	}

	LOG_INF("SLIP interfaces configured: cobs0=%s, cobs1=%s",
		SLIP0_IPV6_ADDR, SLIP1_IPV6_ADDR);
	return 0;
}

/* Start throughput test */
static int throughput_start(void)
{
	int ret;
	struct sockaddr_in6 client_bind_addr;
	struct in6_addr slip0_addr;

	if (test_params.running) {
		LOG_WRN("Test already running");
		return -EBUSY;
	}

	test_params.running = true;
	test_params.stop_requested = false;
	client_finished = false;
	server_finished = false;

	LOG_INF("=== Starting TCP Throughput Test ===");
	LOG_INF("Chunk size: %u bytes", test_params.chunk_size);
	LOG_INF("Duration: %u seconds", test_params.duration_sec);
	LOG_INF("Client delay: %u ms", test_params.delay_ms);
	LOG_INF("Server processing delay: %u ms", test_params.processing_delay_ms);

	/* Check if interfaces are configured */
	if (!slip0_iface || !slip1_iface) {
		LOG_ERR("SLIP interfaces not configured");
		test_params.running = false;
		return -ENODEV;
	}

	/* Create client TCP socket */
	client_sock = zsock_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
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

	/* Initialize statistics */
	k_mutex_lock(&stats_lock, K_FOREVER);
	memset(&stats, 0, sizeof(stats));
	stats.start_time = k_uptime_get_32();
	stats.last_progress_time = stats.start_time;
	k_mutex_unlock(&stats_lock);

	/* Start server thread */
	server_tid = k_thread_create(&server_thread_data, server_stack,
				     SERVER_STACK_SIZE,
				     server_thread_fn, NULL, NULL, NULL,
				     SERVER_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(server_tid, "tcp_server");

	/* Start client thread */
	client_tid = k_thread_create(&client_thread_data, client_stack,
				    CLIENT_STACK_SIZE,
				    client_thread_fn, NULL, NULL, NULL,
				    CLIENT_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(client_tid, "tcp_client");

	/* Schedule progress reporting */
	k_work_schedule(&progress_work, K_SECONDS(1));

	return 0;
}

/* Stop the test and wait for completion */
static int throughput_stop(void)
{
	if (!test_params.running) {
		LOG_WRN("No test is running");
		return -EINVAL;
	}

	LOG_INF("Stopping test...");

	/* Cancel progress work */
	k_work_cancel_delayable(&progress_work);

	test_params.stop_requested = true;

	/* Wait for threads to finish */
	k_thread_join(client_tid, K_FOREVER);
	k_thread_join(server_tid, K_FOREVER);

	/* Close sockets if they're still open (shouldn't be needed if threads closed them) */
	if (client_sock >= 0) {
		LOG_DBG("Cleanup: Closing client socket");
		zsock_close(client_sock);
		client_sock = -1;
	}
	if (accept_sock >= 0) {
		LOG_DBG("Cleanup: Closing accept socket");
		zsock_close(accept_sock);
		accept_sock = -1;
	}
	if (server_sock >= 0) {
		LOG_DBG("Cleanup: Closing server socket");
		zsock_close(server_sock);
		server_sock = -1;
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

	int ret = throughput_start();
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

	int ret = throughput_stop();
	if (ret < 0) {
		shell_error(sh, "Failed to stop test: %d", ret);
	}
	return ret;
}

/* Shell command: set chunk size */
static int cmd_set_chunk_size(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_error(sh, "Usage: set_chunk_size <bytes>");
		return -EINVAL;
	}

	if (test_params.running) {
		shell_error(sh, "Cannot change settings while test is running");
		return -EBUSY;
	}

	uint32_t size = strtoul(argv[1], NULL, 10);
	if (size < 16 || size > BUFFER_SIZE) {
		shell_error(sh, "Invalid size. Must be between 16 and %u", BUFFER_SIZE);
		return -EINVAL;
	}

	test_params.chunk_size = size;
	shell_print(sh, "Chunk size set to %u bytes", size);
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
	shell_print(sh, "Client delay between chunks set to %u ms", delay);
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
	shell_print(sh, "  Chunk size: %u bytes", test_params.chunk_size);
	shell_print(sh, "  Duration: %u seconds", test_params.duration_sec);
	shell_print(sh, "  Client delay: %u ms", test_params.delay_ms);
	shell_print(sh, "  Server processing delay: %u ms", test_params.processing_delay_ms);
	shell_print(sh, "  Status: %s", test_params.running ? "RUNNING" : "IDLE");

	if (test_params.running) {
		k_mutex_lock(&stats_lock, K_FOREVER);
		uint32_t start_time = stats.start_time;
		uint64_t bytes_sent = stats.bytes_sent;
		uint64_t bytes_received = stats.bytes_received;
		uint32_t errors = stats.errors;
		k_mutex_unlock(&stats_lock);

		uint32_t elapsed = (k_uptime_get_32() - start_time) / 1000;
		shell_print(sh, "");
		shell_print(sh, "Current statistics:");
		shell_print(sh, "  Elapsed: %u seconds", elapsed);
		shell_print(sh, "  TX: %llu bytes (%llu KB)", bytes_sent, bytes_sent / 1024);
		shell_print(sh, "  RX: %llu bytes (%llu KB)", bytes_received,
			    bytes_received / 1024);
		shell_print(sh, "  Errors: %u", errors);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(throughput_cmds,
	SHELL_CMD(start, NULL, "Start throughput test", cmd_start),
	SHELL_CMD(stop, NULL, "Stop throughput test", cmd_stop),
	SHELL_CMD(set_chunk_size, NULL, "Set chunk size (bytes)", cmd_set_chunk_size),
	SHELL_CMD(set_duration, NULL, "Set test duration (seconds)", cmd_set_duration),
	SHELL_CMD(set_delay, NULL, "Set client delay between chunks (ms)", cmd_set_delay),
	SHELL_CMD(set_processing_delay, NULL, "Set server processing delay (ms)",
		  cmd_set_processing_delay),
	SHELL_CMD(status, NULL, "Show current status and statistics", cmd_status),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(throughput, &throughput_cmds, "TCP throughput test commands", NULL);

int main(void)
{
	int ret;

	/* Initialize mutex */
	k_mutex_init(&stats_lock);

	/* Initialize work queue for progress reporting */
	k_work_init_delayable(&progress_work, progress_work_handler);

	LOG_INF("SLIP TCP Throughput Test");
	LOG_INF("");

	/* Setup both SLIP interfaces with IPv6 addresses */
	ret = setup_interfaces();
	if (ret < 0) {
		LOG_ERR("Failed to setup SLIP interfaces: %d", ret);
		return -1;
	}

	LOG_INF("");
	LOG_INF("=== SLIP Interfaces Ready ===");
	LOG_INF("cobs0 (client): %s", SLIP0_IPV6_ADDR);
	LOG_INF("cobs1 (server): %s", SLIP1_IPV6_ADDR);
	LOG_INF("");
	LOG_INF("This sample tests TCP throughput over SLIP/UART");
	LOG_INF("Data is sent in a continuous stream and echoed back");
	LOG_INF("");
	LOG_INF("Available commands:");
	LOG_INF("  throughput status                    - Show current status");
	LOG_INF("  throughput set_chunk_size 1024       - Set chunk size");
	LOG_INF("  throughput set_duration 30           - Set test duration");
	LOG_INF("  throughput set_delay 10              - Set client delay (ms)");
	LOG_INF("  throughput set_processing_delay 100  - Set server processing delay (ms)");
	LOG_INF("  throughput start                     - Start test");
	LOG_INF("  throughput stop                      - Stop test and show results");

	return 0;
}

