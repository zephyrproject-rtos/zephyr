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

LOG_MODULE_REGISTER(cobs_client, LOG_LEVEL_INF);

/* External reference to COBS Serial L2 */
extern const struct net_l2 _net_l2_COBS_SERIAL;

#define SERVER_PORT 5001
#define MAX_PACKET_SIZE 1472  /* MTU 1500 - IP header - UDP header */

/* Default configuration (can be overridden via Kconfig) */
#ifndef CONFIG_SLIP_THROUGHPUT_PACKET_SIZE
#define CONFIG_SLIP_THROUGHPUT_PACKET_SIZE 512
#endif

#ifndef CONFIG_SLIP_THROUGHPUT_SERVER_IP
#define CONFIG_SLIP_THROUGHPUT_SERVER_IP "192.168.7.1"
#endif

#ifndef CONFIG_SLIP_THROUGHPUT_DURATION_SEC
#define CONFIG_SLIP_THROUGHPUT_DURATION_SEC 60
#endif

/* Test parameters */
static struct {
	char server_ip[16];
	uint16_t server_port;
	uint32_t packet_size;
	uint32_t duration_sec;
	uint32_t delay_ms;
	bool running;
} test_params = {
	.server_ip = CONFIG_SLIP_THROUGHPUT_SERVER_IP,
	.server_port = SERVER_PORT,
	.packet_size = CONFIG_SLIP_THROUGHPUT_PACKET_SIZE,
	.duration_sec = CONFIG_SLIP_THROUGHPUT_DURATION_SEC,
	.delay_ms = 0,
	.running = false,
};

/* Statistics */
static struct {
	uint64_t packets_sent;
	uint64_t packets_received;
	uint64_t bytes_sent;
	uint64_t bytes_received;
	uint64_t errors;
	uint64_t pattern_errors;
	uint64_t timeouts;
	uint32_t min_rtt_us;
	uint32_t max_rtt_us;
	uint64_t total_rtt_us;
	uint32_t start_time;
} stats;

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

/* Print final statistics */
static void print_final_stats(void)
{
	uint32_t elapsed = k_uptime_get_32() - stats.start_time;
	uint32_t elapsed_sec = elapsed / 1000;

	if (elapsed_sec == 0) {
		elapsed_sec = 1;
	}

	uint64_t tx_kbps = (stats.bytes_sent * 8) / (elapsed_sec * 1000);
	uint64_t rx_kbps = (stats.bytes_received * 8) / (elapsed_sec * 1000);
	uint32_t avg_rtt_us = stats.packets_received > 0 ?
			      (uint32_t)(stats.total_rtt_us / stats.packets_received) : 0;

	LOG_INF("");
	LOG_INF("=== Final Statistics ===");
	LOG_INF("Test duration: %u seconds", elapsed_sec);
	LOG_INF("Packet size: %u bytes", test_params.packet_size);
	LOG_INF("");
	LOG_INF("TX: %llu packets, %llu bytes (%llu kbps)",
		stats.packets_sent, stats.bytes_sent, tx_kbps);
	LOG_INF("RX: %llu packets, %llu bytes (%llu kbps)",
		stats.packets_received, stats.bytes_received, rx_kbps);
	LOG_INF("");
	LOG_INF("Packet loss: %llu (%.2f%%)",
		stats.packets_sent - stats.packets_received,
		stats.packets_sent > 0 ? 
		(double)(stats.packets_sent - stats.packets_received) * 100.0 / stats.packets_sent : 0.0);
	LOG_INF("Errors: %llu, Pattern errors: %llu, Timeouts: %llu",
		stats.errors, stats.pattern_errors, stats.timeouts);
	LOG_INF("");
	LOG_INF("RTT (us): min=%u, avg=%u, max=%u",
		stats.min_rtt_us, avg_rtt_us, stats.max_rtt_us);
	LOG_INF("======================");
}

/* Run throughput test */
static int run_test(void)
{
	int sock;
	struct sockaddr_in server_addr;
	uint8_t tx_buffer[MAX_PACKET_SIZE];
	uint8_t rx_buffer[MAX_PACKET_SIZE];
	int ret;
	uint32_t seq = 0;
	struct timeval timeout;
	uint32_t test_end_time;
	struct net_if *iface;

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

	LOG_INF("=== Starting Throughput Test ===");
	LOG_INF("Server: %s:%u", test_params.server_ip, test_params.server_port);
	LOG_INF("Packet size: %u bytes", test_params.packet_size);
	LOG_INF("Duration: %u seconds", test_params.duration_sec);
	LOG_INF("Delay: %u ms", test_params.delay_ms);

	/* Find SLIP interface */
	iface = net_if_get_first_by_type(&_net_l2_COBS_SERIAL);
	if (!iface) {
		LOG_ERR("No SLIP interface found");
		test_params.running = false;
		return -ENODEV;
	}

	/* Wait for interface to be up */
	while (!net_if_is_up(iface)) {
		LOG_INF("Waiting for SLIP interface to come up...");
		k_sleep(K_MSEC(100));
	}

	/* Create UDP socket */
	sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create socket: %d", errno);
		test_params.running = false;
		return -errno;
	}

	/* Set receive timeout */
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	ret = zsock_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	if (ret < 0) {
		LOG_WRN("Failed to set socket timeout: %d", errno);
	}

	/* Setup server address */
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(test_params.server_port);
	ret = zsock_inet_pton(AF_INET, test_params.server_ip, &server_addr.sin_addr);
	if (ret != 1) {
		LOG_ERR("Invalid server IP address: %s", test_params.server_ip);
		zsock_close(sock);
		test_params.running = false;
		return -EINVAL;
	}

	/* Initialize statistics */
	memset(&stats, 0, sizeof(stats));
	stats.min_rtt_us = UINT32_MAX;
	stats.start_time = k_uptime_get_32();
	test_end_time = stats.start_time + (test_params.duration_sec * 1000);

	LOG_INF("Test started...");

	/* Main test loop */
	while (k_uptime_get_32() < test_end_time) {
		uint32_t send_time, recv_time, rtt_us;

		/* Fill packet with pattern */
		fill_pattern(tx_buffer, test_params.packet_size, seq);

		/* Send packet */
		send_time = k_cycle_get_32();
		ret = zsock_sendto(sock, tx_buffer, test_params.packet_size, 0,
			     (struct sockaddr *)&server_addr, sizeof(server_addr));
		
		if (ret < 0) {
			LOG_ERR("sendto failed: %d", errno);
			stats.errors++;
			seq++;
			continue;
		}

		stats.packets_sent++;
		stats.bytes_sent += ret;

		/* Receive echo response */
		ret = zsock_recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0, NULL, NULL);
		recv_time = k_cycle_get_32();

		if (ret < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				stats.timeouts++;
			} else {
				LOG_ERR("recvfrom failed: %d", errno);
				stats.errors++;
			}
			seq++;
			continue;
		}

		stats.packets_received++;
		stats.bytes_received += ret;

		/* Calculate RTT */
		rtt_us = k_cyc_to_us_floor32(recv_time - send_time);
		stats.total_rtt_us += rtt_us;
		if (rtt_us < stats.min_rtt_us) {
			stats.min_rtt_us = rtt_us;
		}
		if (rtt_us > stats.max_rtt_us) {
			stats.max_rtt_us = rtt_us;
		}

		/* Verify pattern */
		if (!verify_pattern(rx_buffer, ret, seq)) {
			stats.pattern_errors++;
		}

		seq++;

		/* Optional delay between packets */
		if (test_params.delay_ms > 0) {
			k_sleep(K_MSEC(test_params.delay_ms));
		}
	}

	zsock_close(sock);
	test_params.running = false;

	print_final_stats();

	return 0;
}

/* Shell command: start test */
static int cmd_start(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Starting throughput test...");
	return run_test();
}

/* Shell command: set packet size */
static int cmd_set_size(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_error(sh, "Usage: set_size <bytes>");
		return -EINVAL;
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

/* Shell command: set server IP */
static int cmd_set_server(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_error(sh, "Usage: set_server <ip_address>");
		return -EINVAL;
	}

	strncpy(test_params.server_ip, argv[1], sizeof(test_params.server_ip) - 1);
	shell_print(sh, "Server IP set to %s", test_params.server_ip);
	return 0;
}

/* Shell command: set duration */
static int cmd_set_duration(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_error(sh, "Usage: set_duration <seconds>");
		return -EINVAL;
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

	uint32_t delay = strtoul(argv[1], NULL, 10);
	test_params.delay_ms = delay;
	shell_print(sh, "Packet delay set to %u ms", delay);
	return 0;
}

/* Shell command: show config */
static int cmd_show_config(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Current configuration:");
	shell_print(sh, "  Server: %s:%u", test_params.server_ip, test_params.server_port);
	shell_print(sh, "  Packet size: %u bytes", test_params.packet_size);
	shell_print(sh, "  Duration: %u seconds", test_params.duration_sec);
	shell_print(sh, "  Delay: %u ms", test_params.delay_ms);
	shell_print(sh, "  Status: %s", test_params.running ? "RUNNING" : "IDLE");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(throughput_cmds,
	SHELL_CMD(start, NULL, "Start throughput test", cmd_start),
	SHELL_CMD(set_size, NULL, "Set packet size (bytes)", cmd_set_size),
	SHELL_CMD(set_server, NULL, "Set server IP address", cmd_set_server),
	SHELL_CMD(set_duration, NULL, "Set test duration (seconds)", cmd_set_duration),
	SHELL_CMD(set_delay, NULL, "Set delay between packets (ms)", cmd_set_delay),
	SHELL_CMD(show, NULL, "Show current configuration", cmd_show_config),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(throughput, &throughput_cmds, "SLIP throughput test commands", NULL);

int main(void)
{
	struct net_if *iface;

	LOG_INF("SLIP Throughput Client");

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
	LOG_INF("");
	LOG_INF("Use 'throughput' shell commands to configure and start test");
	LOG_INF("Example:");
	LOG_INF("  throughput show                    - Show current config");
	LOG_INF("  throughput set_size 1024           - Set packet size");
	LOG_INF("  throughput set_server 192.168.7.1  - Set server IP");
	LOG_INF("  throughput set_duration 30         - Set test duration");
	LOG_INF("  throughput start                   - Start test");

	/* Auto-start test if configured */
#ifdef CONFIG_SLIP_THROUGHPUT_AUTO_START
	LOG_INF("Auto-starting test in 5 seconds...");
	k_sleep(K_SECONDS(5));
	run_test();
#endif

	return 0;
}

