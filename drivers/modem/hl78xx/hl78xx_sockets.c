/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/modem/chat.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_monitor.h>

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS)
#include "tls_internal.h"
#include <zephyr/net/tls_credentials.h>
#endif

#include <string.h>
#include <stdlib.h>
#include "hl78xx.h"
#include "hl78xx_socket.h"
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
#include "mqtt_offload/mqtt_offload_socket.h"
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */

LOG_MODULE_REGISTER(hl78xx_socket, CONFIG_MODEM_LOG_LEVEL);

RING_BUF_DECLARE(mdm_recv_pool, CONFIG_MODEM_HL78XX_UART_BUFFER_SIZES);

/* clang-format off */
#define MQTT_TLS_OR_OFFLOAD_ENABLED                      \
	(IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) ||     \
	 IS_ENABLED(CONFIG_MODEM_HL78XX_MQTT_OFFLOADING))
/* clang-format on */

struct work_socket_data work_buf;
struct receive_socket_data receive_buf;

uint16_t match_len;

bool match_connect_found;
bool match_eof_ok_found;
bool match_found;
bool socket_data_received;

uint16_t start_index_eof;
uint16_t size_of_socketdata;
atomic_t state_leftover;
struct hl78xx_socket_data socket_data;

static int offload_socket(int family, int type, int protom);
static int socket_close(struct modem_socket *sock);
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS)
static int map_credentials(struct modem_socket *sock, const void *optval, socklen_t optlen);
#endif /* CONFIG_NET_SOCKETS_SOCKOPT_TLS */
void hl78xx_on_kstatev_parser(struct hl78xx_data *data, int state)
{
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("KSTATEV: socket %d state %d", socket_data.current_sock_fd, state);
#endif
	switch (state) {
	case 1: /* CLOSED */
		LOG_DBG("Socket fd: %d closed by modem (KSTATEV: 2 1), resetting socket",
			socket_data.current_sock_fd);
		/* Mark socket as closed to block future I/O */
		/* Free socket resources */
		/* Set a global flag to indicate reconnect is needed */
		break;
	case 3:
		LOG_DBG("Socket %d connected", socket_data.current_sock_fd);
		break;
	case 5:
		LOG_DBG("Data ready on socket %d", socket_data.current_sock_fd);
		break;
	default:
		LOG_DBG("Unhandled KSTATEV for socket %d state %d", socket_data.current_sock_fd,
			state);
		break;
	}
}

static bool parse_ip(bool is_ipv4, const char *ip_str, void *out_addr)
{
	int ret = net_addr_pton(is_ipv4 ? AF_INET : AF_INET6, ip_str, out_addr);

	LOG_DBG("Parsing %s address: %s -> %s", is_ipv4 ? "IPv4" : "IPv6", ip_str,
		(ret < 0) ? "FAIL" : "OK");
	if (ret < 0) {
		LOG_ERR("Invalid IP address: %s", ip_str);
		return false;
	}
	return true;
}

static bool update_dns(bool is_ipv4, const char *dns_str)
{
	int ret;

	LOG_DBG("Updating DNS (%s): %s", is_ipv4 ? "IPv4" : "IPv6", dns_str);

	if (is_ipv4) {
		ret = strncmp(dns_str, socket_data.dns.v4_string, strlen(dns_str));
		if (ret != 0) {
			LOG_DBG("New IPv4 DNS differs from current, marking dns_ready = false");
			socket_data.dns.ready = false;
		}
		strncpy(socket_data.dns.v4_string, dns_str, sizeof(socket_data.dns.v4_string));
		socket_data.dns.v4_string[sizeof(socket_data.dns.v4_string) - 1] = '\0';
		return parse_ip(true, socket_data.dns.v4_string, &socket_data.dns.v4);
	}
#ifdef CONFIG_NET_IPV6
	else {
		ret = strncmp(dns_str, socket_data.dns.v6_string, strlen(dns_str));
		if (ret != 0) {
			LOG_DBG("New IPv6 DNS differs from current, marking dns_ready = false");
			socket_data.dns.ready = false;
		}
		strncpy(socket_data.dns.v6_string, dns_str, sizeof(socket_data.dns.v6_string));
		socket_data.dns.v6_string[sizeof(socket_data.dns.v6_string) - 1] = '\0';

		if (!parse_ip(false, socket_data.dns.v6_string, &socket_data.dns.v6)) {
			return false;
		}

		net_addr_ntop(AF_INET6, &socket_data.dns.v6, socket_data.dns.v6_string,
			      sizeof(socket_data.dns.v6_string));
		LOG_DBG("Parsed IPv6 DNS: %s", socket_data.dns.v6_string);
	}
#endif
	return true;
}

bool is_valid_ipv4_addr(struct in_addr *addr)
{
	return addr && (addr->s_addr != 0);
}

static void set_iface(bool is_ipv4, struct in6_addr *new_ipv6_addr, struct in6_addr *ipv6Addr)
{
	if (!socket_data.net_iface) {
		LOG_DBG("No network interface set. Skipping iface config.");
		return;
	}

	LOG_DBG("Setting %s interface address...", is_ipv4 ? "IPv4" : "IPv6");

	if (is_ipv4) {
#ifdef CONFIG_NET_IPV4
		if (is_valid_ipv4_addr(&socket_data.ipv4.addr)) {
			net_if_ipv4_addr_rm(socket_data.net_iface, &socket_data.ipv4.addr);
		}

		if (!net_if_ipv4_addr_add(socket_data.net_iface, &socket_data.ipv4.new_addr,
					  NET_ADDR_DHCP, 0)) {
			LOG_ERR("Failed to set IPv4 interface address.");
		}

		net_if_ipv4_set_netmask_by_addr(socket_data.net_iface, &socket_data.ipv4.new_addr,
						&socket_data.ipv4.subnet);
		net_if_ipv4_set_gw(socket_data.net_iface, &socket_data.ipv4.gateway);

		net_ipaddr_copy(&socket_data.ipv4.addr, &socket_data.ipv4.new_addr);
		LOG_DBG("IPv4 interface configuration complete.");
#endif
	} else {
#ifdef CONFIG_NET_IPV6
		net_if_ipv6_addr_rm(socket_data.net_iface, ipv6Addr);

		if (!net_if_ipv6_addr_add(socket_data.net_iface, new_ipv6_addr, NET_ADDR_AUTOCONF,
					  0)) {
			LOG_ERR("Failed to set IPv6 interface address.");
		} else {
			LOG_DBG("IPv6 interface configuration complete.");
		}
#endif
	}
}

static bool split_ipv4_and_subnet(const char *combined, char *ip_out, size_t ip_out_len,
				  char *subnet_out, size_t subnet_out_len)
{
	int dot_count = 0;
	const char *ptr = combined;
	const char *split = NULL;

	while (*ptr && dot_count < 4) {
		if (*ptr == '.') {
			dot_count++;
			if (dot_count == 4) {
				split = ptr;
				break;
			}
		}
		ptr++;
	}

	if (!split) {
		LOG_ERR("Invalid IPv4 + subnet format: %s", combined);
		return false;
	}

	size_t ip_len = split - combined;

	if (ip_len >= ip_out_len) {
		ip_len = ip_out_len - 1;
	}

	strncpy(ip_out, combined, ip_len);
	ip_out[ip_len] = '\0';

	strncpy(subnet_out, split + 1, subnet_out_len);
	subnet_out[subnet_out_len - 1] = '\0';

	LOG_DBG("Extracted IP: %s, Subnet: %s", ip_out, subnet_out);
	return true;
}

void hl78xx_release_socket_comms(void)
{
	k_sem_give(&socket_data.psm_cntrl_sem);
}
static void hl78xx_send_wakeup_signal(void)
{
	hl78xx_delegate_event(socket_data.mdata_global, MODEM_HL78XX_EVENT_RESUME);
}

static int validate_socket(const struct modem_socket *sock)
{

	if (!sock) {
		return -EINVAL;
	}

	if (!sock->is_connected) {
		errno = ENOTCONN;
		return -1;
	}

	return 0;
}

static void hl78xx_on_ktcpind(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct modem_socket *sock = NULL;
	int socket_id = -1;
	int tcp_conn_stat = -1;

	if (argc < 3 || !argv[1] || !argv[2]) {
		LOG_ERR("TCP_IND: Incomplete response");
		goto exit;
	}

	socket_id = ATOI(argv[1], -1, "socket_id");

	if (socket_id == -1) {
		goto exit;
	}
	sock = modem_socket_from_id(&socket_data.socket_config, socket_id);
	tcp_conn_stat = ATOI(argv[2], -1, "tcp_status");
	if (tcp_conn_stat == TCP_SOCKET_CONNECTED) {
		socket_data.tcp_conn_status.err_code = tcp_conn_stat;
		socket_data.tcp_conn_status.is_connected = true;
		return;
	}
exit:
	socket_data.tcp_conn_status.err_code = tcp_conn_stat;
	socket_data.tcp_conn_status.is_connected = false;
	if (socket_id != -1) {
		modem_socket_put(&socket_data.socket_config, sock->sock_fd);
	}
}
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
static void hl78xx_on_mqttind(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct modem_socket *sock = NULL;
	int socket_id = -1;
	int mqtt_conn_stat = -1;

	if (argc < 3 || !argv[1] || !argv[2]) {
		LOG_ERR("MQTTCFG: Incomplete response");
		goto exit;
	}

	socket_id = ATOI(argv[1], -1, "socket_id");

	if (socket_id == -1) {
		goto exit;
	}
	sock = modem_socket_from_id(&socket_data.socket_config, socket_id);
	mqtt_conn_stat = ATOI(argv[2], -1, "mqtt_status");
	LOG_DBG("%d %d", __LINE__, mqtt_conn_stat);
	switch (mqtt_conn_stat) {
	case MQTT_CONNECTION_SUCCESSFUL:
		socket_data.mqtt_status.conn.err_code = mqtt_conn_stat;
		socket_data.mqtt_status.conn.is_connected = true;
		LOG_DBG("%d CONNECTION SUCCESSFUL", __LINE__);
		return;
	case MQTT_SUBSCRIBE_SUCCESSFUL:
		LOG_DBG("%d SUBSCRIBE SUCCESSFUL", __LINE__);
		socket_data.mqtt_status.subscribe.err_code = mqtt_conn_stat;
		socket_data.mqtt_status.subscribe.is_connected = true;
		return;
	case MQTT_UNSUBSCRIBE_SUCCESSFUL:
		LOG_DBG("%d UNSUBSCRIBE SUCCESSFUL", __LINE__);
		socket_data.mqtt_status.unsubscribe.err_code = mqtt_conn_stat;
		socket_data.mqtt_status.unsubscribe.is_connected = true;
		return;
	case MQTT_PUBLISH_SUCCESSFUL:
		LOG_DBG("%d PUBLISH SUCCESSFUL", __LINE__);
		socket_data.mqtt_status.publish.err_code = mqtt_conn_stat;
		socket_data.mqtt_status.publish.is_connected = true;
		return;
	case MQTT_GENERIC_ERROR:
		LOG_DBG("%d GENERIC ERROR", __LINE__);
		__fallthrough;
	default:
		break;
	}
exit:
	socket_data.mqtt_status.conn.err_code = mqtt_conn_stat;
	socket_data.mqtt_status.conn.is_connected = false;
	if (socket_id != -1) {
		modem_socket_put(&socket_data.socket_config, sock->sock_fd);
	}
}
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */

static void hl78xx_on_kudpind(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct modem_socket *sock = NULL;
	int socket_id = -1;
	int udp_conn_stat = -1;

	if (argc < 3 || !argv[1] || !argv[2]) {
		LOG_ERR("UDP_IND: Incomplete response");
		goto exit;
	}

	socket_id = ATOI(argv[1], -1, "socket_id");

	if (socket_id == -1) {
		goto exit;
	}

	/* look up new socket by special id */
	sock = modem_socket_from_newid(&socket_data.socket_config);
	if (sock) {
		sock->is_connected = true;
		socket_id = ATOI(argv[1], -1, "socket_id");
		/* on error give up modem socket */
		if (modem_socket_id_assign(&socket_data.socket_config, sock, socket_id) < 0) {
			modem_socket_put(&socket_data.socket_config, sock->sock_fd);
			sock->is_connected = false;
		}
	}
	udp_conn_stat = ATOI(argv[2], -1, "udp_status");
	if (udp_conn_stat == UDP_SOCKET_CONNECTED) {
		socket_data.udp_conn_status.err_code = udp_conn_stat;
		socket_data.udp_conn_status.is_connected = true;
		return;
	}
exit:
	socket_data.udp_conn_status.err_code = udp_conn_stat;
	socket_data.udp_conn_status.is_connected = false;
	if (socket_id != -1) {
		modem_socket_put(&socket_data.socket_config, sock->sock_fd);
	}
}

/* Handler: +KTCP/KUDP/KMQTT: <socket_id>[0] */
static void hl78xx_on_cmd_kxxxsockcreate(struct modem_chat *chat, char **argv, uint16_t argc,
					 void *user_data)
{
	struct modem_socket *sock = NULL;
	int socket_id;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d", __LINE__);
#endif
	if (argc < 2 || !argv[1]) {
		LOG_ERR("%s: Incomplete response", __func__);
		return;
	}
	/* look up new socket by special id */
	sock = modem_socket_from_newid(&socket_data.socket_config);
	if (sock) {
		sock->is_connected = true;
		socket_id = ATOI(argv[1], -1, "socket_id");
		/* on error give up modem socket */
		if (modem_socket_id_assign(&socket_data.socket_config, sock, socket_id) < 0) {
			modem_socket_put(&socket_data.socket_config, sock->sock_fd);
			sock->is_connected = false;
		}
	}
	/* don't give back semaphore -- OK to follow */
}

static void hl78xx_on_cgdcontrdp(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data)
{
	if (argc < 8 || !argv[4] || !argv[5] || !argv[6]) {
		LOG_ERR("Incomplete CGCONTRDP response: argc = %d", argc);
		return;
	}

	LOG_INF("CGCONTRDP: apn=%s addr=%s gw=%s dns=%s", argv[3], argv[4], argv[5], argv[6]);

	bool is_ipv4 = (strchr(argv[4], ':') == NULL);

	char ip_addr[NET_IPV6_ADDR_LEN];
	char subnet_mask[NET_IPV6_ADDR_LEN];

#ifdef CONFIG_MODEM_HL78XX_APN_SOURCE_NETWORK
	bool is_apn_exists = (argv[3] && strlen(argv[3]) > 0);

	if (is_apn_exists) {
		hl78xx_extract_essential_part_apn(argv[3], socket_data.mdata_global->identity.apn,
						  sizeof(socket_data.mdata_global->identity.apn));
	}
#endif
	if (!split_ipv4_and_subnet(argv[4], ip_addr, sizeof(ip_addr), subnet_mask,
				   sizeof(subnet_mask))) {
		return;
	}

	LOG_INF("Extracted IP: %s, Subnet: %s", ip_addr, subnet_mask);

#ifdef CONFIG_NET_IPV6
	struct in6_addr new_ipv6_addr = {0};
	struct in6_addr ipv6Addr = {0};
#endif

	if (is_ipv4) {
		if (!parse_ip(true, ip_addr, &socket_data.ipv4.new_addr)) {
			return;
		}
		if (!parse_ip(true, subnet_mask, &socket_data.ipv4.subnet)) {
			return;
		}
		if (!parse_ip(true, argv[5], &socket_data.ipv4.gateway)) {
			return;
		}
	} else {
#ifdef CONFIG_NET_IPV6
		if (!parse_ip(false, ip_addr, &new_ipv6_addr)) {
			return;
		}
#endif
	}

	if (!update_dns(is_ipv4, argv[6])) {
		return;
	}

#ifdef CONFIG_NET_IPV6
	set_iface(is_ipv4, &new_ipv6_addr, &ipv6Addr);
#else
	set_iface(is_ipv4, NULL, NULL);
#endif
}

static void hl78xx_on_ktcpstate(struct modem_chat *chat, char **argv, uint16_t argc,
				void *user_data)
{
	if (argc < 4) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s %s %s %s %s", __LINE__, argv[1], argv[2], argv[3], argv[4], argv[5]);
#endif
	uint8_t tcp_session_id = ATOI(argv[1], 0, "tcp_session_id");
	uint8_t tcp_status = ATOI(argv[2], -1, "tcp_status");
	int8_t tcp_notif = ATOI(argv[3], -1, "tcp_notif");
	uint16_t rcv_data = ATOI(argv[5], 0, "tcp_rcv_data");

	if (tcp_status != 3 && tcp_notif != -1) {
		return;
	}

	socket_notify_data(tcp_session_id, rcv_data);
}

static void hl78xx_on_cme_error(struct modem_chat *chat, char **argv, uint16_t argc,
				void *user_data)
{
	if (argc < 2 || !argv[1]) {
		LOG_ERR("%d %s CME ERROR: Incomplete response", __LINE__, __func__);
		return;
	}

	int error_code = ATOI(argv[1], -1, "CME error code");

	if (error_code < 0) {
		LOG_ERR("Invalid CME error code: %d", error_code);
		return;
	}
	socket_data.socket_data_error = true;
	LOG_ERR("CME ERROR: %d", error_code);
}

MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", NULL);
MODEM_CHAT_MATCHES_DEFINE(connect_matches, MODEM_CHAT_MATCH(CONNECT_STRING, "", NULL),
			  MODEM_CHAT_MATCH("+CME ERROR: ", "", hl78xx_on_cme_error));
MODEM_CHAT_MATCHES_DEFINE(allow_matches, MODEM_CHAT_MATCH("OK", "", NULL),
			  MODEM_CHAT_MATCH("+CME ERROR: ", "", NULL));
MODEM_CHAT_MATCH_DEFINE(kudpind_match, "+KUDP_IND: ", ",", hl78xx_on_kudpind);
MODEM_CHAT_MATCH_DEFINE(ktcpind_match, "+KTCP_IND: ", ",", hl78xx_on_ktcpind);
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
MODEM_CHAT_MATCH_DEFINE(kmqttind_match, "+KMQTT_IND: ", ",", hl78xx_on_mqttind);
MODEM_CHAT_MATCH_DEFINE(mqttcfg_match, "+KMQTTCFG: ", "", hl78xx_on_cmd_kxxxsockcreate);
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */
MODEM_CHAT_MATCH_DEFINE(ktcpcfg_match, "+KTCPCFG: ", "", hl78xx_on_cmd_kxxxsockcreate);
MODEM_CHAT_MATCH_DEFINE(cgdcontrdp_match, "+CGCONTRDP: ", ",", hl78xx_on_cgdcontrdp);
MODEM_CHAT_MATCH_DEFINE(ktcp_state_match, "+KTCPSTAT: ", ",", hl78xx_on_ktcpstate);

static void parser_reset(void)
{
	memset(&receive_buf, 0, sizeof(receive_buf));
	match_found = false;
}

static void found_reset(void)
{
	match_connect_found = false;
	match_eof_ok_found = false;
	socket_data_received = false;
}

static bool modem_chat_parse_end_del_start(struct modem_chat *chat)
{
	if (receive_buf.len == 0) {
		return false;
	}

	for (uint8_t i = 0; i < chat->delimiter_size; i++) {
		if (receive_buf.buf[receive_buf.len - 1] == chat->delimiter[i]) {
			return true;
		}
	}
	return false;
}

static bool modem_chat_parse_end_del_complete(struct modem_chat *chat)
{
	if (receive_buf.len < chat->delimiter_size) {
		return false;
	}

	return memcmp(&receive_buf.buf[receive_buf.len - chat->delimiter_size], chat->delimiter,
		      chat->delimiter_size) == 0;
}

static bool modem_chat_match_matches_received(const char *match, uint16_t match_size)
{
	if (receive_buf.len < match_size) {
		return false;
	}

	for (uint16_t i = 0; i < match_size; i++) {
		if (match[i] != receive_buf.buf[i]) {
			return false;
		}
	}
	return true;
}

static bool is_receive_buffer_full(void)
{
	return receive_buf.len >= ARRAY_SIZE(receive_buf.buf);
}

static void handle_expected_length_decrement(void)
{
	if (match_connect_found && socket_data.expected_buf_len > 0) {
		socket_data.expected_buf_len--;
	}
}

static bool is_end_delimiter_only(void)
{
	return receive_buf.len == socket_data.mdata_global->chat.delimiter_size;
}

static bool is_valid_eof_index(uint8_t size_match)
{
	start_index_eof = receive_buf.len - size_match - 2;
	return start_index_eof < ARRAY_SIZE(receive_buf.buf);
}

static void try_handle_eof_pattern(void)
{
	uint8_t size_match = strlen(EOF_PATTERN);

	if (receive_buf.len < size_match + 2) {
		return;
	}
	if (!is_valid_eof_index(size_match)) {
		return;
	}

	if (strncmp(&receive_buf.buf[start_index_eof], EOF_PATTERN, size_match) == 0) {
		int ret = ring_buf_put(socket_data.buf_pool, receive_buf.buf, start_index_eof);

		if (ret <= 0) {
			LOG_ERR("ring_buf_put failed: %d", ret);
		}
		socket_data_received = true;
		socket_data.collected_buf_len += ret;
		LOG_DBG("%d %d bytes received, total collected: %d %d", __LINE__, ret,
			socket_data.collected_buf_len, start_index_eof);
	}
}

static bool is_cme_error_match(void)
{
	uint8_t size_match = strlen(CME_ERROR_STRING);

	return receive_buf.len >= size_match &&
	       modem_chat_match_matches_received(CME_ERROR_STRING, size_match);
}

static bool is_connect_match(void)
{
	uint8_t size_match = strlen(CONNECT_STRING);

	return receive_buf.len == size_match &&
	       modem_chat_match_matches_received(CONNECT_STRING, size_match);
}

static bool is_ok_match(void)
{
	uint8_t size_match = strlen(OK_STRING);

	return receive_buf.len == size_match &&
	       modem_chat_match_matches_received(OK_STRING, size_match);
}

static void socket_process_bytes(char byte)
{
	if (is_receive_buffer_full()) {
		LOG_WRN("Receive buffer overrun");
		parser_reset();
		return;
	}

	receive_buf.buf[receive_buf.len++] = byte;
	handle_expected_length_decrement();

	if (modem_chat_parse_end_del_complete(&socket_data.mdata_global->chat)) {
		if (is_end_delimiter_only()) {
			parser_reset();
			return;
		}

		size_of_socketdata = receive_buf.len;

		if (match_connect_found && !match_eof_ok_found) {
			try_handle_eof_pattern();
		}

		parser_reset();
		return;
	}

	if (modem_chat_parse_end_del_start(&socket_data.mdata_global->chat)) {
		return;
	}

	if (!match_found && !match_connect_found) {
		if (is_connect_match()) {
			match_connect_found = true;
			LOG_DBG("CONNECT matched. Expecting %d more bytes.",
				socket_data.expected_buf_len);
			return;
		} else if (is_cme_error_match()) {
			match_found = true; /* mark this to prevent further parsing */
			LOG_ERR("CME ERROR received. Connection failed.");
			socket_data.expected_buf_len = 0;
			socket_data.collected_buf_len = 0;
			parser_reset();
			socket_data.socket_data_error = true;
			/*  Optionally notify script or raise an event here */
			k_sem_give(&socket_data.mdata_global->script_stopped_sem_rx_int);
			return;
		}
	}
	if (match_connect_found && !match_eof_ok_found && is_ok_match()) {
		match_eof_ok_found = true;
		LOG_DBG("OK matched.");
	}
}

static int modem_process_handler(void)
{
	if (socket_data.expected_buf_len == 0) {
		LOG_DBG("No more data expected");
		atomic_set_bit(&state_leftover, MODEM_SOCKET_DATA_LEFTOVER_STATE_BIT);
		return 0;
	}

	int recv_len = modem_pipe_receive(socket_data.mdata_global->uart_pipe, work_buf.buf,
					  MIN(sizeof(work_buf.buf), socket_data.expected_buf_len));
	if (recv_len <= 0) {
		LOG_WRN("modem_pipe_receive returned %d", recv_len);
		return recv_len;
	}

	work_buf.len = recv_len;
	LOG_HEXDUMP_DBG(work_buf.buf, recv_len, "Received bytes:");

	for (int i = 0; i < recv_len; i++) {
		socket_process_bytes(work_buf.buf[i]);
	}

	if (match_eof_ok_found && socket_data_received) {
		LOG_DBG("All data received: %d bytes", size_of_socketdata);
		socket_data.expected_buf_len = 0;
		found_reset();

		k_sem_give(&socket_data.mdata_global->script_stopped_sem_rx_int);
	}

	return 0;
}

static void modem_pipe_callback(struct modem_pipe *pipe, enum modem_pipe_event event,
				void *user_data)
{
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d Pipe event received: %d", __LINE__, event);
#endif
	switch (event) {
	case MODEM_PIPE_EVENT_RECEIVE_READY:
		modem_process_handler();
		break;

	case MODEM_PIPE_EVENT_TRANSMIT_IDLE:
		k_sem_give(&socket_data.mdata_global->script_stopped_sem_tx_int);
		break;

	default:
		LOG_DBG("Unhandled event: %d", event);
		break;
	}
}

void notif_carrier_off(void)
{
	net_if_carrier_off(socket_data.net_iface);
}

void notif_carrier_on(void)
{
	net_if_carrier_on(socket_data.net_iface);
}

void iface_status_work_cb(struct hl78xx_data *data, modem_chat_script_callback script_user_callback)
{

	const char *cmd = "AT+CGCONTRDP=1";
	int ret = 0;

	ret = modem_cmd_send_int(data, script_user_callback, cmd, strlen(cmd), &cgdcontrdp_match, 1,
				 false);
	if (ret < 0) {
		LOG_ERR("Failed to send AT+CGCONTRDP command: %d", ret);
		return;
	}
}

void dns_work_cb(bool reset)
{
#if defined(CONFIG_DNS_RESOLVER) && !defined(CONFIG_DNS_SERVER_IP_ADDRESSES)
	int ret;
	struct dns_resolve_context *dnsCtx;
	struct sockaddr temp_addr;
	bool valid_address = false;
	bool retry = false;
	const char *const dns_servers_str[DNS_SERVERS_COUNT] = {
#ifdef CONFIG_NET_IPV6
		socket_data.dns_v6_string,
#endif
#ifdef CONFIG_NET_IPV4
		socket_data.dns.v4_string,
#endif
		NULL};
	const char *dns_servers_wrapped[ARRAY_SIZE(dns_servers_str)];

	if (reset) {
		LOG_DBG("Resetting DNS resolver");
		dnsCtx = dns_resolve_get_default();
		if (dnsCtx->state != DNS_RESOLVE_CONTEXT_INACTIVE) {
			dns_resolve_close(dnsCtx);
		}
		socket_data.dns.ready = false;
	}

#ifdef CONFIG_NET_IPV6
	valid_address = net_ipaddr_parse(socket_data.dns_v6_string,
					 strlen(socket_data.dns_v6_string), &temp_addr);
	if (!valid_address && IS_ENABLED(CONFIG_NET_IPV4)) {
		/* IPv6 DNS string is not valid, replace it with IPv4 address and recheck */
		strncpy(socket_data.dns_v6_string, socket_data.dns_v4_string,
			sizeof(socket_data.dns_v4_string) - 1);
		valid_address = net_ipaddr_parse(socket_data.dns_v6_string,
						 strlen(socket_data.dns_v6_string), &temp_addr);
	}
#else
	valid_address = net_ipaddr_parse(socket_data.dns.v4_string,
					 strlen(socket_data.dns.v4_string), &temp_addr);
#endif
	if (!valid_address) {
		LOG_WRN("No valid DNS address!");
		return;
	}
	if (!socket_data.net_iface || !net_if_is_up(socket_data.net_iface) ||
	    socket_data.dns.ready) {
		return;
	}

	memcpy(dns_servers_wrapped, dns_servers_str, sizeof(dns_servers_wrapped));

	/* set new DNS addr in DNS resolver */
	LOG_DBG("Refresh DNS resolver");
	dnsCtx = dns_resolve_get_default();
	if (dnsCtx->state == DNS_RESOLVE_CONTEXT_INACTIVE) {
		LOG_DBG("Initializing DNS resolver");
		ret = dns_resolve_init(dnsCtx, dns_servers_wrapped, NULL);
	} else {
		LOG_DBG("Reconfiguring DNS resolver");
		ret = dns_resolve_reconfigure(dnsCtx, dns_servers_wrapped, NULL, DNS_SOURCE_MANUAL);
	}
	if (ret < 0) {
		LOG_ERR("dns_resolve_reconfigure fail (%d)", ret);
		retry = true;
	} else {
		LOG_DBG("DNS ready");
		socket_data.dns.ready = true;
	}
	if (retry) {
		LOG_WRN("DNS not ready, scheduling a retry");
	}
#endif
}

static int on_cmd_sockread_common(int socket_id, uint16_t socket_data_length, uint16_t len,
				  void *user_data)
{
	struct modem_socket *sock;
	struct socket_read_data *sock_data;
	int ret = 0;
	int packet_size = 0;

	sock = modem_socket_from_fd(&socket_data.socket_config, socket_id);
	if (!sock) {
		LOG_ERR("Socket not found! (%d)", socket_id);
		return -EINVAL;
	}

	sock_data = sock->data;
	if (!sock_data) {
		LOG_ERR("Socket data missing! Ignoring (%d)", socket_id);
		return -EINVAL;
	}
	if (socket_data.socket_data_error && socket_data.collected_buf_len == 0) {
		errno = ECONNABORTED;
		return -ECONNABORTED;
	}

	if ((len <= 0) || socket_data_length <= 0 || socket_data.collected_buf_len < (size_t)len) {
		LOG_ERR("%d Invalid data length: %d %d %d Aborting!", __LINE__, socket_data_length,
			(int)len, socket_data.collected_buf_len);
		return -EAGAIN;
	}

	if (len < socket_data_length) {
		LOG_DBG("Incomplete data received! Expected: %d, Received: %d", socket_data_length,
			len);
		return -EAGAIN;
	}

	ret = ring_buf_get(socket_data.buf_pool, sock_data->recv_buf, len);
	if (ret != len) {
		LOG_ERR("%d Data retrieval mismatch: expected %u, got %d", __LINE__, len, ret);
		return -EAGAIN;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_HEXDUMP_DBG(sock_data->recv_buf, ret, "Received Data:");
#endif

	if (sock_data->recv_buf_len < (size_t)len) {
		LOG_ERR("Buffer overflow! Received: %zu vs. Available: %zu", len,
			sock_data->recv_buf_len);
		return -EINVAL;
	}

	if ((size_t)len != (size_t)socket_data_length) {
		LOG_ERR("Data mismatch! Copied: %zu vs. Received: %d", len, socket_data_length);
		return -EINVAL;
	}

	sock_data->recv_read_len = len;

	/* Remove packet from list */
	packet_size = modem_socket_next_packet_size(&socket_data.socket_config, sock);
	modem_socket_packet_size_update(&socket_data.socket_config, sock, -socket_data_length);
	socket_data.collected_buf_len = 0;

	return len;
}

int modem_handle_data_capture(size_t target_len, struct hl78xx_data *data)
{
	return on_cmd_sockread_common(socket_data.current_sock_fd, socket_data.sizeof_socket_data,
				      target_len, data);
}

/*
 * generic socket creation function
 * which can be called in bind() or connect()
 */
static int extract_ip_family_and_port(const struct sockaddr *addr, int *af, uint16_t *port)
{
	if (addr->sa_family == AF_INET6) {
		*port = ntohs(net_sin6(addr)->sin6_port);
		*af = MDM_HL78XX_SOCKET_AF_IPV6;
	} else if (addr->sa_family == AF_INET) {
		*port = ntohs(net_sin(addr)->sin_port);
		*af = MDM_HL78XX_SOCKET_AF_IPV4;
	} else {
		errno = EAFNOSUPPORT;
		return -1;
	}
	return 0;
}

static int format_ip_and_setup_tls(const struct sockaddr *addr, char *ip_str, size_t ip_str_len,
				   struct modem_socket *sock)
{
	int ret = modem_context_sprint_ip_addr(addr, ip_str, ip_str_len);

	if (ret != 0) {
		LOG_ERR("Failed to format IP!");
		errno = ENOMEM;
		return -1;
	}

	if (sock->ip_proto == IPPROTO_TCP) {
		memcpy(socket_data.tls.hostname, ip_str, MIN(MDM_MAX_HOSTNAME_LEN, ip_str_len));
		socket_data.tls.hostname[ip_str_len] = '\0';
		socket_data.tls.hostname_set = false;
	}

	return 0;
}

static int send_tcp_or_tls_config(struct modem_socket *sock, uint16_t dst_port, int af, int mode,
				  struct hl78xx_data *data)
{
	char cmd_buf[sizeof("AT+KTCPCFG=#,#,\"" MODEM_HL78XX_ADDRESS_FAMILY_FORMAT
			    "\",#####,,,,#,,#") +
		     MDM_MAX_HOSTNAME_LEN + NET_IPV6_ADDR_LEN + MDM_MQTT_MAX_CLIENT_ID_LEN];
	const struct modem_chat_match *response_matches;

#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
	if ((socket_data.socketopt & SOL_MQTT) != 0) {
		snprintk(cmd_buf, sizeof(cmd_buf),
			 "AT+KMQTTCFG=1,%d,\"%s\",%u,4,\"%s\",%d,%d,\"\",\"\",,%d,\"\"",
			 socket_data.tls.hostname_set, socket_data.tls.hostname, dst_port,
			 socket_data.mqtt_client_id, socket_data.mqtt_config.keep_alive,
			 socket_data.mqtt_config.clean_session, socket_data.mqtt_config.qos);
		response_matches = &mqttcfg_match;
	} else {
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */

		snprintk(cmd_buf, sizeof(cmd_buf), "AT+KTCPCFG=1,%d,\"%s\",%u,,,,%d,%s,0", mode,
			 socket_data.tls.hostname, dst_port, af, mode == 3 ? "0" : "");
		response_matches = &ktcpcfg_match;
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
	}
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */

	int ret = modem_cmd_send_int(data, NULL, cmd_buf, strlen(cmd_buf), response_matches, 1,
				     false);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", cmd_buf, ret);
		modem_socket_put(&socket_data.socket_config, sock->sock_fd);
		errno = ret;
		return -1;
	}
	return 0;
}

static int send_udp_config(const struct sockaddr *addr, struct hl78xx_data *data,
			   struct modem_socket *sock)
{
	char cmd_buf[64];
	uint8_t display_data_urc = 2;

#if defined(CONFIG_MODEM_HL78XX_SOCKET_UDP_DISPLAY_DATA_URC)
	display_data_urc = CONFIG_MODEM_HL78XX_SOCKET_UDP_DISPLAY_DATA_URC;
#endif

	snprintk(cmd_buf, sizeof(cmd_buf), "AT+KUDPCFG=1,%u,,%d,,,%d,%d", 0, display_data_urc,
		 (addr->sa_family - 1), 0);

	int ret =
		modem_cmd_send_int(data, NULL, cmd_buf, strlen(cmd_buf), &kudpind_match, 1, false);
	if (ret < 0) {
		goto error;
	}

	if (!socket_data.udp_conn_status.is_connected) {
		ret = socket_data.udp_conn_status.err_code;
		goto error;
	}

	return 0;

error:
	LOG_ERR("%s ret:%d", cmd_buf, ret);
	modem_socket_put(&socket_data.socket_config, sock->sock_fd);
	errno = ret;
	return -1;
}

static int create_socket(struct modem_socket *sock, const struct sockaddr *addr,
			 struct hl78xx_data *data)
{
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d", __LINE__);
#endif

	int af;
	uint16_t dst_port;
	char ip_str[NET_IPV6_ADDR_LEN];

	memcpy(&sock->dst, addr, sizeof(*addr));

	if (extract_ip_family_and_port(addr, &af, &dst_port) < 0) {
		return -1;
	}

	if (format_ip_and_setup_tls(addr, ip_str, sizeof(ip_str), sock) < 0) {
		return -1;
	}

	bool is_udp = (sock->ip_proto == IPPROTO_UDP);

	if (is_udp) {
		return send_udp_config(addr, data, sock);
	}

	int mode = (sock->ip_proto == IPPROTO_TLS_1_2) ? 3 : 0;
	/* only TCP and TLS are supported */
	if (sock->ip_proto != IPPROTO_TCP && sock->ip_proto != IPPROTO_TLS_1_2) {
		LOG_ERR("Unsupported protocol: %d", sock->ip_proto);
		errno = EPROTONOSUPPORT;
		return -1;
	}

	return send_tcp_or_tls_config(sock, dst_port, af, mode, data);
}

static int socket_close(struct modem_socket *sock)
{
	char buf[sizeof("AT+KTCPCLOSE=##\r")];
	int ret = 0;

	if (sock->ip_proto == IPPROTO_UDP) {
		snprintk(buf, sizeof(buf), "AT+KUDPCLOSE=%d", sock->id);
	} else {
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
		if ((socket_data.socketopt & SOL_MQTT) != 0) {
			snprintk(buf, sizeof(buf), "AT+KMQTTCLOSE=%d", sock->id);
		} else {
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */
			snprintk(buf, sizeof(buf), "AT+KTCPCLOSE=%d", sock->id);
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
		}
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */
	}

	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, buf, strlen(buf), allow_matches, 2,
				 false);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", buf, ret);
	}

	return ret;
}

static int socket_delete(struct modem_socket *sock)
{
	char buf[sizeof("AT+KTCPDEL=##\r")];
	int ret = 0;

	if (sock->ip_proto == IPPROTO_UDP) {
		snprintk(buf, sizeof(buf), "AT+KUDPDEL=%d", sock->id);
	} else {
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
		if ((socket_data.socketopt & SOL_MQTT) != 0) {
			snprintk(buf, sizeof(buf), "AT+KMQTTDEL=%d", sock->id);
		} else {
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */
			snprintk(buf, sizeof(buf), "AT+KTCPDEL=%d", sock->id);
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
		}
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */
	}

	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, buf, strlen(buf), allow_matches, 2,
				 false);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", buf, ret);
	}

	return ret;
}

/*
 * Socket Offload OPS
 */

static int offload_socket(int family, int type, int proto)
{
	int ret;
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d %d %d", __LINE__, family, type, proto);
#endif
	/* defer modem's socket create call to bind() */
	ret = modem_socket_get(&socket_data.socket_config, family, type, proto);
	if (ret < 0) {
		errno = ret;
		return -1;
	}

	LOG_DBG("%d %d", __LINE__, ret);
	errno = 0;
	return ret;
}

static int offload_close(void *obj)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	/* make sure socket is allocated and assigned an id */
	if (modem_socket_id_is_assigned(&socket_data.socket_config, sock) == false) {
		return 0;
	}
	if (validate_socket(sock) == 0) {
		LOG_DBG("%d %d %d", __LINE__,
			socket_data.mdata_global->status.pmc_power_down.requested_previously,
			socket_data.mdata_global->status.pmc_power_down.status_previously);
		if (!socket_data.mdata_global->status.pmc_power_down.requested_previously &&
		    !socket_data.mdata_global->status.pmc_power_down.status_previously) {
			socket_close(sock);
			socket_delete(sock);
		}
		modem_socket_put(&socket_data.socket_config, sock->sock_fd);
		sock->is_connected = false;
	}
	/* Consider here successfully socket is closed */
	return 0;
}

static int offload_bind(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d", __LINE__);
#endif
	/*  Save bind address information */
	memcpy(&sock->src, addr, sizeof(*addr));
	/* Check if socket is allocated */
	if (modem_socket_is_allocated(&socket_data.socket_config, sock)) {
		/* Trigger socket creation */
		if (create_socket(sock, addr, socket_data.mdata_global) < 0) {
			LOG_ERR("%d %s SOCKET CREATION", __LINE__, __func__);
			return -1;
		}
	}
	return 0;
}

static int offload_connect(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	int ret = 0;
	int af;
	char cmd_buf[sizeof("AT+KTCPCFG=#\r")];
	char ip_str[NET_IPV6_ADDR_LEN];
	uint16_t dst_port = 0U;
	const struct modem_chat_match *response_matches;
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
	bool is_it_mqtt = false;
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d", __LINE__);
#endif
	if (!addr) {
		errno = EINVAL;
		return -1;
	}
	if (!hl78xx_is_registered(socket_data.mdata_global)) {
		errno = ENETUNREACH;
		return -1;
	}

	if (validate_socket(sock) == 0) {
		LOG_ERR("Socket is already connected! id: %d, fd: %d", sock->id, sock->sock_fd);
		errno = EISCONN;
		return -1;
	}

	/* make sure socket has been allocated */
	if (modem_socket_is_allocated(&socket_data.socket_config, sock) == false) {
		LOG_ERR("Invalid socket_id(%d) from fd:%d", sock->id, sock->sock_fd);
		errno = EINVAL;
		return -1;
	}

	/* make sure we've created the socket */
	if (modem_socket_id_is_assigned(&socket_data.socket_config, sock) == false) {
		LOG_DBG("%d no socket assigned", __LINE__);
		if (create_socket(sock, addr, socket_data.mdata_global) < 0) {
			return -1;
		}
	}

	memcpy(&sock->dst, addr, sizeof(*addr));
	if (addr->sa_family == AF_INET6) {
		af = MDM_HL78XX_SOCKET_AF_IPV6;
		dst_port = ntohs(net_sin6(addr)->sin6_port);
	} else if (addr->sa_family == AF_INET) {
		af = MDM_HL78XX_SOCKET_AF_IPV4;
		dst_port = ntohs(net_sin(addr)->sin_port);
	} else {
		errno = EAFNOSUPPORT;
		return -1;
	}
	/* skip socket connect if UDP */
	if (sock->ip_proto == IPPROTO_UDP) {
		errno = 0;
		return 0;
	}
	LOG_DBG("%d socket stat: %d", __LINE__, sock->is_connected);
	ret = modem_context_sprint_ip_addr(addr, ip_str, sizeof(ip_str));
	if (ret != 0) {
		errno = -ret;
		LOG_ERR("Error formatting IP string %d", ret);
		return -1;
	}
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
	if ((socket_data.socketopt & SOL_MQTT) != 0) {
		is_it_mqtt = true;
		snprintk(cmd_buf, sizeof(cmd_buf), "AT+KMQTTCNX=%d", sock->id);
		response_matches = &kmqttind_match;
	} else {
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */

		snprintk(cmd_buf, sizeof(cmd_buf), "AT+KTCPCNX=%d", sock->id);
		response_matches = &ktcpind_match;
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
	}
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */
	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, cmd_buf, strlen(cmd_buf),
				 response_matches, 1, false);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", cmd_buf, ret);
		errno = ret;
		return -1;
	}
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
	if (is_it_mqtt && socket_data.mqtt_status.conn.is_connected == false) {
		sock->is_connected = false;
		errno = socket_data.mqtt_status.conn.err_code;
		return -(int)socket_data.mqtt_status.conn.err_code;
	}
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */
	if (
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
		is_it_mqtt == false &&
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */
		socket_data.tcp_conn_status.is_connected == false) {
		sock->is_connected = false;
		errno = socket_data.tcp_conn_status.err_code;
		return -(int)socket_data.tcp_conn_status.err_code;
	}
	sock->is_connected = true;
	errno = 0;
	return 0;
}

static bool validate_recv_args(void *buf, size_t len, int flags)
{
	if (!buf || len == 0) {
		errno = EINVAL;
		return false;
	}
	if (flags & ZSOCK_MSG_PEEK) {
		errno = ENOTSUP;
		return false;
	}
	return true;
}

static int lock_socket_mutex(void)
{
	int ret = k_mutex_lock(&socket_data.mdata_global->tx_lock, K_SECONDS(1));

	if (ret < 0) {
		LOG_ERR("Failed to acquire TX lock: %d", ret);
		errno = ret;
	}
	return ret;
}

static int wait_for_data_if_needed(struct modem_socket *sock, int flags)
{
	int size = modem_socket_next_packet_size(&socket_data.socket_config, sock);

	if (size > 0) {
		return size;
	}

	if (flags & ZSOCK_MSG_DONTWAIT) {
		errno = EAGAIN;
		return -1;
	}

	if (validate_socket(sock) == -1) {
		errno = 0;
		return 0;
	}

	modem_socket_wait_data(&socket_data.socket_config, sock);
	return modem_socket_next_packet_size(&socket_data.socket_config, sock);
}

static void prepare_read_command(char *sendbuf, size_t bufsize, struct modem_socket *sock,
				 size_t read_size)
{
	snprintk(sendbuf, bufsize, "AT+K%sRCV=%d,%zd%s",
		 sock->ip_proto == IPPROTO_UDP ? "UDP" : "TCP", sock->id, read_size,
		 socket_data.mdata_global->chat.delimiter);
}

static void setup_socket_data(struct modem_socket *sock, struct socket_read_data *sock_data,
			      void *buf, size_t len, struct sockaddr *from, uint16_t read_size)
{
	memset(sock_data, 0, sizeof(*sock_data));
	sock_data->recv_buf = buf;
	sock_data->recv_buf_len = len;
	sock_data->recv_addr = from;
	sock->data = sock_data;

	socket_data.sizeof_socket_data = read_size;
	socket_data.requested_socket_id = sock->id;
	socket_data.current_sock_fd = sock->sock_fd;
	socket_data.expected_buf_len = read_size + sizeof("\r\n") - 1 +
				       socket_data.mdata_global->buffers.eof_pattern_size +
				       sizeof(MODEM_STREAM_END_WORD) - 1;
	socket_data.collected_buf_len = 0;
	socket_data.socket_data_error = false;
}

static void restore_socket_state(void)
{
	k_mutex_unlock(&socket_data.mdata_global->tx_lock);
	modem_chat_attach(&socket_data.mdata_global->chat, socket_data.mdata_global->uart_pipe);
	socket_data.expected_buf_len = 0;
}

static void check_tcp_state_if_needed(struct modem_socket *sock)
{
	if (atomic_test_and_clear_bit(&state_leftover, MODEM_SOCKET_DATA_LEFTOVER_STATE_BIT) &&
	    sock->ip_proto == IPPROTO_TCP) {
		const char *check_ktcp_stat = "AT+KTCPSTAT";

		modem_cmd_send_int(socket_data.mdata_global, NULL, check_ktcp_stat,
				   strlen(check_ktcp_stat), &ktcp_state_match, 1, true);
	}
}
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
static int read_mqtt_data(struct modem_socket *sock, uint16_t expected_size)
{
	int ret = 0;
	int packet_size = 0;
	struct socket_read_data *sock_data;

	if (!sock) {
		errno = EINVAL;
		return -1;
	}
	sock_data = sock->data;
	if (!sock_data) {
		errno = EINVAL;
		return -1;
	}
	LOG_DBG("%d expected_size: %d", __LINE__, expected_size);
	ret = ring_buf_get(socket_data.buf_pool, sock_data->recv_buf, expected_size);
	if (ret != expected_size) {
		LOG_ERR("%d Data retrieval mismatch: expected %u, got %d", __LINE__, expected_size,
			ret);
		return -EAGAIN;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_HEXDUMP_DBG(sock_data->recv_buf, ret, "Received Data:");
#endif
	sock_data->recv_read_len = ret;

	/* Remove packet from list */
	packet_size = modem_socket_next_packet_size(&socket_data.socket_config, sock);
	modem_socket_packet_size_update(&socket_data.socket_config, sock, -expected_size);
	socket_data.collected_buf_len = 0;
	return ret;
}
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */
static ssize_t offload_recvfrom(void *obj, void *buf, size_t len, int flags, struct sockaddr *from,
				socklen_t *fromlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	int ret;
	char sendbuf[sizeof("AT+KUDPRCV=#,##########\r\n")];
	struct socket_read_data sock_data;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d", __LINE__);
#endif
	if (!hl78xx_is_registered(socket_data.mdata_global) || validate_socket(sock) == -1) {
		errno = EAGAIN;
		return -1;
	}
	if (!validate_recv_args(buf, len, flags)) {
		return -1;
	}

	ret = lock_socket_mutex();
	if (ret < 0) {
		return -1;
	}

	int next_packet_size = wait_for_data_if_needed(sock, flags);

	if (next_packet_size <= 0) {
		ret = next_packet_size;
		goto exit;
	}
	uint32_t max_data_length =
		MDM_MAX_DATA_LENGTH - (socket_data.mdata_global->buffers.eof_pattern_size +
				       sizeof(MODEM_STREAM_STARTER_WORD) - 1);

	next_packet_size = MIN(next_packet_size, max_data_length);

	uint16_t read_size = MIN(next_packet_size, len);

	setup_socket_data(sock, &sock_data, buf, len, from, read_size);
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
	if ((socket_data.socketopt & SOL_MQTT) != 0) {
		/* MQTT-SN handling */
		LOG_DBG("%d %d", __LINE__, sock->id);
		if (read_mqtt_data(sock, read_size) > 0) {
			goto processdone;
		} else {
			goto exit;
		}
	}
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */
	prepare_read_command(sendbuf, sizeof(sendbuf), sock, read_size);

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d socket_fd: %d, socket_id: %d, expected_data_len: %d", __LINE__,
		socket_data.current_sock_fd, socket_data.requested_socket_id,
		socket_data.expected_buf_len);
	LOG_HEXDUMP_DBG(sendbuf, strlen(sendbuf), "sending");
#endif

	modem_chat_release(&socket_data.mdata_global->chat);
	modem_pipe_attach(socket_data.mdata_global->uart_pipe, modem_pipe_callback,
			  socket_data.mdata_global);

	if (k_sem_take(&socket_data.mdata_global->script_stopped_sem_tx_int, K_FOREVER) < 0 ||
	    modem_pipe_transmit(socket_data.mdata_global->uart_pipe, sendbuf, strlen(sendbuf)) <
		    0 ||
	    k_sem_take(&socket_data.mdata_global->script_stopped_sem_rx_int, K_FOREVER) < 0 ||
	    modem_handle_data_capture(read_size, socket_data.mdata_global) < 0) {
		ret = -1;
		goto exit;
	}
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
processdone:
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */

	if (from && fromlen) {
		*fromlen = sizeof(sock->dst);
		memcpy(from, &sock->dst, *fromlen);
	}

	errno = 0;
	ret = sock_data.recv_read_len;

exit:
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d %d", __LINE__, errno, ret);
#endif
	restore_socket_state();
	check_tcp_state_if_needed(sock);
	return ret;
}

int check_if_any_socket_connected(void)
{
	struct modem_socket_config *cfg = &socket_data.socket_config;

	k_sem_take(&cfg->sem_lock, K_FOREVER);

	for (int i = 0; i < cfg->sockets_len; i++) {
		if (cfg->sockets[i].is_connected) {
			/* if there is any socket  connected */
			return true;
		}
	}

	k_sem_give(&cfg->sem_lock);
	return false;
}

static int prepare_udp_send_cmd(const struct modem_socket *sock, const struct sockaddr *dst_addr,
				size_t buf_len, char *cmd_buf, size_t cmd_buf_size)
{
	char ip_str[NET_IPV6_ADDR_LEN];
	uint16_t dst_port = 0;
	int ret = 0;

	ret = modem_context_sprint_ip_addr(dst_addr, ip_str, sizeof(ip_str));
	if (ret < 0) {
		LOG_ERR("Error formatting IP string %d", ret);
		return ret;
	}

	ret = modem_context_get_addr_port(dst_addr, &dst_port);
	if (ret < 0) {
		LOG_ERR("Error getting port from IP address %d", ret);
		return ret;
	}

	snprintk(cmd_buf, cmd_buf_size, "AT+KUDPSND=%d,\"%s\",%u,%zu", sock->id, ip_str, dst_port,
		 buf_len);
	return 0;
}
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
static ssize_t decode_payload_length(struct net_buf_simple *buf)
{
	size_t length;
	size_t length_field_s = 1;
	size_t buflen = buf->len;

	/*
	 * Size must not be larger than an uint16_t can fit,
	 * minus 3 bytes for the length field itself
	 */
	if (buflen > UINT16_MAX) {
		LOG_ERR("Message too large");
		return -EFBIG;
	}

	length = net_buf_simple_pull_u8(buf);
	if (length == HL78XX_MQTT_OFFLOAD_LENGTH_FIELD_EXTENDED_PREFIX) {
		length = net_buf_simple_pull_be16(buf);
		length_field_s = 3;
	}

	if (length != buflen) {
		LOG_ERR("Message length %zu != buffer size %zu", length, buflen);
		return -EPROTO;
	}

	if (length <= length_field_s) {
		LOG_ERR("Message length %zu - contains no data?", length);
		return -ENODATA;
	}

	/* subtract the size of the length field to get message length */
	return length - length_field_s;
}

static int prepare_mqtt_send_cmd(const struct modem_socket *sock, size_t buf_len, const char *buf,
				 char *cmd_buf, size_t cmd_buf_size, char *payload_buf,
				 uint16_t *size_payload)
{
	const struct modem_chat_match *response_matches = NULL;

	struct net_buf_simple net_buf;
	uint8_t temp_buf[buf_len + 1];
	uint8_t msg_type = 0;
	uint16_t matches_size = 0;
	int ret = 0;

	memcpy(temp_buf, buf, buf_len);
	net_buf.data = temp_buf;
	net_buf.len = buf_len;
	net_buf.size = buf_len;

	ret = decode_payload_length(&net_buf);
	if (ret < 0) {
		LOG_ERR("Failed to decode payload length");
		return ret;
	}
	msg_type = net_buf_simple_pull_u8(&net_buf);
	LOG_DBG("buf_len=%d type=0x%02x", buf_len, msg_type);

	switch (msg_type) {
		/* MQTT-OFFLOAD SUBSCRIBE */
	case HL78XX_MQTT_OFFLOAD_MSG_TYPE_SUBSCRIBE: {
		if (buf_len < 5) {
			return -EINVAL;
		}

		uint8_t flags = net_buf_simple_pull_u8(&net_buf);
		uint8_t qos = ((flags & HL78XX_MQTT_OFFLOAD_FLAGS_MASK_QOS) >>
			       HL78XX_MQTT_OFFLOAD_FLAGS_SHIFT_QOS);

		/* topic is a string starting at ptr[3] */
		const char *topic = net_buf_simple_pull_mem(&net_buf, ret - 2);
		size_t topic_len = strnlen(topic, ret - 2);

		if (3 + topic_len > buf_len) {
			LOG_ERR("SUBSCRIBE topic parse error");
			return -EINVAL;
		}

		snprintk(cmd_buf, cmd_buf_size, "AT+KMQTTSUB=%d,\"%.*s\",%d", sock->id,
			 (int)topic_len, topic, qos);
		response_matches = &kmqttind_match;
		matches_size = 1;
		break;
	}
	/* MQTT-OFFLOAD UNSUBSCRIBE */
	case HL78XX_MQTT_OFFLOAD_MSG_TYPE_UNSUBSCRIBE: {
		if (buf_len < 5) {
			return -EINVAL;
		}

		uint8_t flags = net_buf_simple_pull_u8(&net_buf);
		const char *topic = net_buf_simple_pull_mem(&net_buf, ret - 2);
		size_t topic_len = strnlen(topic, ret - 2);

		ARG_UNUSED(flags);

		if (3 + topic_len > buf_len) {
			LOG_ERR("UNSUBSCRIBE topic parse error");
			return -EINVAL;
		}

		snprintk(cmd_buf, cmd_buf_size, "AT+KMQTTUNSUB=%d,\"%.*s\"", sock->id,
			 (int)topic_len, topic);
		response_matches = &kmqttind_match;
		matches_size = 1;
		break;
	}
		/* MQTT-OFFLOAD PUBLISH */
	case HL78XX_MQTT_OFFLOAD_MSG_TYPE_PUBLISH: {
		uint8_t flags = net_buf_simple_pull_u8(&net_buf);
		uint8_t qos = ((flags & HL78XX_MQTT_OFFLOAD_FLAGS_MASK_QOS) >>
			       HL78XX_MQTT_OFFLOAD_FLAGS_SHIFT_QOS);
		uint8_t retain = (bool)(flags & HL78XX_MQTT_OFFLOAD_FLAGS_RETAIN);

		const char *topic = NULL;
		uint16_t topic_len = 0;
		const char *payload = NULL;
		uint16_t payload_len = 0;
		/* 0 == HL78XX_MQTT_OFFLOAD_TOPIC_TYPE_NORMAL */
		topic_len = net_buf_simple_pull_be16(&net_buf);
		topic = net_buf_simple_pull_mem(&net_buf, topic_len);
		LOG_DBG("topic_len=%d qos=%d retain=%d", topic_len, qos, retain);
		if (5 + topic_len + 2 > buf_len) {
			LOG_ERR("PUBLISH topic parse error");
			return -EINVAL;
		}

		if (!topic) {
			return -EINVAL;
		}
		payload_len = ret - 4 - topic_len;
		payload = net_buf_simple_pull_mem(&net_buf, payload_len);
		LOG_DBG("payload_len=%d", payload_len);
		if (!payload) {
			return -EINVAL;
		}
		memcpy(payload_buf, payload, payload_len);
		*size_payload = payload_len;

		snprintk(cmd_buf, cmd_buf_size, "AT+KMQTTPUBSTART=%d,\"%.*s\",%d,%d", sock->id,
			 (int)topic_len, topic, qos, retain);

		return buf_len;
	}

	default:
		LOG_DBG("Unhandled msg_type 0x%02x", msg_type);
		return -ENOTSUP;
	}

	LOG_DBG("cmd=%s", cmd_buf);

	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, cmd_buf, strlen(cmd_buf),
				 response_matches, matches_size, false);

	return ret;
}
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */

static int prepare_tcp_send_cmd(const struct modem_socket *sock, size_t buf_len, char *cmd_buf,
				size_t cmd_buf_size)
{
	snprintk(cmd_buf, cmd_buf_size, "AT+KTCPSND=%d,%zu", sock->id, buf_len);
	return 0;
}

static int send_data_buffer(const char *buf, const size_t buf_len, int *sock_written)
{
	uint32_t offset = 0;
	int len = buf_len;
	int ret = 0;

	if (len == 0) {
		LOG_DBG("%d No data to send", __LINE__);
		return 0;
	}

	while (len > 0) {
		if (k_sem_take(&socket_data.mdata_global->script_stopped_sem_tx_int, K_FOREVER) <
		    0) {
			return -1;
		}

		ret = modem_pipe_transmit(socket_data.mdata_global->uart_pipe,
					  ((const uint8_t *)buf) + offset, len);
		if (ret <= 0) {
			LOG_ERR("Transmit error %d", ret);
			return -1;
		}

		offset += ret;
		len -= ret;
		*sock_written += ret;
	}
	return 0;
}

static int validate_and_prepare(struct modem_socket *sock, const struct sockaddr **dst_addr,
				size_t *buf_len, char *cmd_buf, size_t cmd_buf_len)
{
	int ret;

	ret = validate_socket(sock);
	if (ret < 0) {
		return ret;
	}

	if (!*dst_addr && sock->ip_proto == IPPROTO_UDP) {
		*dst_addr = &sock->dst;
	}

	if (*buf_len > MDM_MAX_DATA_LENGTH) {
		if (sock->type == SOCK_DGRAM) {
			errno = EMSGSIZE;
			return -1;
		}
		*buf_len = MDM_MAX_DATA_LENGTH;
	}

	if (sock->ip_proto == IPPROTO_UDP) {
		return prepare_udp_send_cmd(sock, *dst_addr, *buf_len, cmd_buf, cmd_buf_len);
	}

#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
	if ((socket_data.socketopt & SOL_MQTT) != 0) {
		return 1; /* signal MQTT path */
	}
#endif
	return prepare_tcp_send_cmd(sock, *buf_len, cmd_buf, cmd_buf_len);
}

#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
static int handle_mqtt_send(struct mqtt_send_ctx *ctx)
{
	int ret = prepare_mqtt_send_cmd(ctx->sock, ctx->buf_len, ctx->buf, ctx->cmd_buf,
					ctx->cmd_buf_len, ctx->payload, ctx->size_payload);
	if (ret < 0) {
		LOG_ERR("Error sending AT command %d", ret);
		return ret;
	}

	if (ret == ctx->buf_len) {
		/* MQTT PUBLISH split case */
		if (*ctx->size_payload == 0) {
			return -EINVAL;
		}
		return 0;
	}

	*ctx->sock_written = ctx->buf_len;
	return 1;
}

static int transmit_mqtt_payload(struct modem_socket *sock, char *payload, uint16_t size_payload,
				 size_t buf_len, int *sock_written)
{
	int ret;

	ret = send_data_buffer(payload, size_payload, sock_written);
	if (ret < 0) {
		return ret;
	}

	ret = k_sem_take(&socket_data.mdata_global->script_stopped_sem_tx_int, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	k_msleep(100);
	ret = modem_pipe_transmit(
		socket_data.mdata_global->uart_pipe,
		(uint8_t *)(socket_data.mdata_global->buffers.termination_pattern),
		socket_data.mdata_global->buffers.termination_pattern_size);
	if (ret < 0) {
		return ret;
	}

	*sock_written = buf_len;
	return 0;
}
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */

static int transmit_regular_data(const char *buf, size_t buf_len, int *sock_written)
{
	int ret;

	ret = send_data_buffer(buf, buf_len, sock_written);
	if (ret < 0) {
		return ret;
	}

	ret = k_sem_take(&socket_data.mdata_global->script_stopped_sem_tx_int, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	return modem_pipe_transmit(socket_data.mdata_global->uart_pipe,
				   (uint8_t *)socket_data.mdata_global->buffers.eof_pattern,
				   socket_data.mdata_global->buffers.eof_pattern_size);
}

static ssize_t send_socket_data(void *obj, const struct sockaddr *dst_addr, const char *buf,
				size_t buf_len, int flags, k_timeout_t timeout)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	char cmd_buf[82] = {0};
	int ret;
	int sock_written = 0;

#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
	char payload[MDM_MAX_DATA_LENGTH] = {0};
	uint16_t size_payload = 0;
#endif

	ret = validate_and_prepare(sock, &dst_addr, &buf_len, cmd_buf, sizeof(cmd_buf));
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
	if (ret == 1 && (socket_data.socketopt & SOL_MQTT)) {
		struct mqtt_send_ctx ctx = {
			.sock = sock,
			.buf = buf,
			.buf_len = buf_len,
			.cmd_buf = cmd_buf,
			.cmd_buf_len = sizeof(cmd_buf),
			.payload = payload,
			.size_payload = &size_payload,
			.sock_written = &sock_written,
		};

		ret = handle_mqtt_send(&ctx);
		if (ret != 0) {
			return (ret > 0) ? sock_written : -1;
		}
	}
#endif

	socket_data.socket_data_error = false;

	if (k_mutex_lock(&socket_data.mdata_global->tx_lock, K_SECONDS(1)) < 0) {
		return -1;
	}

	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, cmd_buf, strlen(cmd_buf),
				 connect_matches, ARRAY_SIZE(connect_matches), false);
	if (ret < 0 || socket_data.socket_data_error) {
		errno = (ret < 0) ? ret : EIO;
		ret = -1;
		goto cleanup;
	}

	modem_pipe_attach(socket_data.mdata_global->chat.pipe, modem_pipe_callback,
			  &socket_data.mdata_global->chat);

#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
	if ((socket_data.socketopt & SOL_MQTT) != 0) {
		ret = transmit_mqtt_payload(sock, payload, size_payload, buf_len, &sock_written);
	} else {
#endif

		ret = transmit_regular_data(buf, buf_len, &sock_written);
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
	}
#endif

	if (ret < 0) {
		goto cleanup;
	}

	modem_chat_attach(&socket_data.mdata_global->chat, socket_data.mdata_global->uart_pipe);

	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, "", 0, &ok_match, 1, false);
	if (ret < 0) {
		goto cleanup;
	}

cleanup:
	k_mutex_unlock(&socket_data.mdata_global->tx_lock);
	return (ret < 0) ? -1 : sock_written;
}

#ifdef CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS
static int handle_tls_sockopts(struct modem_socket *sock, int optname, const void *optval,
			       socklen_t optlen)
{
	int ret;

	switch (optname) {
	case TLS_SEC_TAG_LIST:
		ret = map_credentials(sock, optval, optlen);
		LOG_DBG("%d %d", __LINE__, ret);
		return ret;

	case TLS_HOSTNAME:
		if (optlen >= MDM_MAX_HOSTNAME_LEN) {
			return -EINVAL;
		}

		memset(socket_data.tls.hostname, 0, MDM_MAX_HOSTNAME_LEN);
		memcpy(socket_data.tls.hostname, optval, optlen);
		socket_data.tls.hostname[optlen] = '\0';
		socket_data.tls.hostname_set = true;

		ret = hl78xx_configure_chipper_suit();
		if (ret < 0) {
			LOG_ERR("Failed to configure chipper suit: %d", ret);
			errno = ret;
			return -1;
		}

		LOG_DBG("TLS hostname set to: %s", socket_data.tls.hostname);
		return 0;

	case TLS_PEER_VERIFY:
		if (*(const uint32_t *)optval != TLS_PEER_VERIFY_REQUIRED) {
			LOG_WRN("Disabling peer verification is not supported");
		}
		return 0;

	case TLS_CERT_NOCOPY:
		return 0; /* No-op, success */

	default:
		LOG_DBG("Unsupported TLS option: %d", optname);
		return -EINVAL;
	}
}
#endif /* CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS */

#if defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS) || defined(CONFIG_MODEM_HL78XX_MQTT_OFFLOADING)
static int offload_setsockopt(void *obj, int level, int optname, const void *optval,
			      socklen_t optlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d level: %d optname: %d", __LINE__, level, optname);
#endif

	if (!MQTT_TLS_OR_OFFLOAD_ENABLED) {
		return -EINVAL;
	}

	switch (level) {
#if defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS)
	case SOL_TLS:
		return handle_tls_sockopts(sock, optname, optval, optlen);
#endif
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING
	case SOL_MQTT:
		switch (optname) {
		case MQTT_CLIENT_ID:
			socket_data.socketopt |= SOL_MQTT;
			memset(socket_data.mqtt_client_id, 0, MDM_MQTT_MAX_CLIENT_ID_LEN);
			memcpy(socket_data.mqtt_client_id, optval, optlen);
			socket_data.mqtt_client_id[optlen] = '\0';
			return 0;
		case MQTT_PACKAGE_TYPE:
			break;
		case MQTT_KEEP_ALIVE:
			socket_data.mqtt_config.keep_alive = *(const int *)optval;
			return 0;
		case MQTT_CLEAN_SESSION:
			socket_data.mqtt_config.clean_session = *(const int *)optval;
			return 0;
		case MQTT_QOS:
			socket_data.mqtt_config.qos = *(const int *)optval;
			return 0;
		default:
#if defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS)
			return handle_tls_sockopts(sock, optname, optval, optlen);
#else
			return -EINVAL;
#endif /* CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS */
		}
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */
	default:
		LOG_DBG("Unsupported socket option: %d", optname);
		return -EINVAL;
	}
}
#endif
/* CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS  or CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */

static ssize_t offload_sendto(void *obj, const void *buf, size_t len, int flags,
			      const struct sockaddr *to, socklen_t tolen)
{
	int ret = 0;
	struct modem_socket *sock = (struct modem_socket *)obj;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d", __LINE__, len);
#endif
	if (!hl78xx_is_registered(socket_data.mdata_global)) {
		LOG_ERR("Modem currently not attached to the network!");
		if (hl78xx_is_in_psm(socket_data.mdata_global) ||
		    hl78xx_is_in_pwr_dwn(socket_data.mdata_global)) {
			hl78xx_send_wakeup_signal();
			ret = k_sem_take(&socket_data.psm_cntrl_sem, K_SECONDS(55));
			if (ret == 0) {
				LOG_DBG("Woke up modem from PSM");
				goto goon;
			} else {
				LOG_DBG("%d timeout waiting for modem wakeup", __LINE__);
			}
		}
		errno = EAGAIN;
		return -1;
	}
goon:
	/* Do some sanity checks. */
	if (!buf || len == 0) {
		errno = EINVAL;
		return -1;
	}
	/* Socket has to be connected. */
	if (!sock->is_connected) {
		errno = ENOTCONN;
		return -1;
	}
	/* Only send up to MTU bytes. */
	if (len > MDM_MAX_DATA_LENGTH) {
		len = MDM_MAX_DATA_LENGTH;
	}
	ret = send_socket_data(obj, to, buf, len, flags, K_SECONDS(MDM_CMD_TIMEOUT));
	if (ret < 0) {
		errno = ret;
		return -1;
	}
	errno = 0;
	return ret;
}

static int offload_ioctl(void *obj, unsigned int request, va_list args)
{
	int ret = 0;
	struct modem_socket *sock = (struct modem_socket *)obj;
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d", __LINE__, request);
#endif
	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;
		struct k_poll_event *pev_end;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
		LOG_DBG("poll_prepare: fd=%d, events=0x%x", pfd->fd, pfd->events);
#endif
		ret = modem_socket_poll_prepare(&socket_data.socket_config, obj, pfd, pev, pev_end);

		if (ret == -1 && errno == ENOTSUP && (pfd->events & ZSOCK_POLLOUT) &&
		    sock->ip_proto == IPPROTO_UDP) {
			/* Not Implemented */
			/*
			 *	You can implement this later when needed
			 *	For now, just ignore it
			 */
			errno = ENOTSUP;
			ret = 0;
		}
		return ret;
	}
	case ZFD_IOCTL_POLL_UPDATE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		return modem_socket_poll_update(obj, pfd, pev);
	}

	case F_GETFL:
		return 0;
	case F_SETFL: {
		int flags = va_arg(args, int);

		LOG_DBG("F_SETFL called with flags=0x%x", flags);
		/* You can store flags if you want, but it's safe to just ignore them. */
		return 0;
	}
	default:
		errno = EINVAL;
		return -1;
	}
}

static ssize_t offload_read(void *obj, void *buffer, size_t count)
{
	return offload_recvfrom(obj, buffer, count, 0, NULL, 0);
}

static ssize_t offload_write(void *obj, const void *buffer, size_t count)
{
	return offload_sendto(obj, buffer, count, 0, NULL, 0);
}

static ssize_t offload_sendmsg(void *obj, const struct msghdr *msg, int flags)
{
	ssize_t sent = 0;
	struct iovec bkp_iovec = {0};
	struct msghdr crafted_msg = {.msg_name = msg->msg_name, .msg_namelen = msg->msg_namelen};
	size_t full_len = 0;
	int ret;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d", __LINE__);
#endif
	/* Compute the full length to send and validate input */
	for (int i = 0; i < msg->msg_iovlen; i++) {
		if (!msg->msg_iov[i].iov_base || msg->msg_iov[i].iov_len == 0) {
			errno = EINVAL;
			return -1;
		}
		full_len += msg->msg_iov[i].iov_len;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("msg_iovlen:%zd flags:%d, full_len:%zd", msg->msg_iovlen, flags, full_len);
#endif
	while (full_len > sent) {
		int removed = 0;
		int i = 0;
		int bkp_iovec_idx = -1;

		crafted_msg.msg_iovlen = msg->msg_iovlen;
		crafted_msg.msg_iov = &msg->msg_iov[0];

		/* Adjust iovec to remove already sent bytes */
		while (removed < sent) {
			int to_remove = sent - removed;

			if (to_remove >= msg->msg_iov[i].iov_len) {
				crafted_msg.msg_iovlen -= 1;
				crafted_msg.msg_iov = &msg->msg_iov[i + 1];
				removed += msg->msg_iov[i].iov_len;
			} else {
				bkp_iovec_idx = i;
				bkp_iovec = msg->msg_iov[i];

				msg->msg_iov[i].iov_len -= to_remove;
				msg->msg_iov[i].iov_base =
					((uint8_t *)msg->msg_iov[i].iov_base) + to_remove;

				removed += to_remove;
			}
			i++;
		}

		ret = send_socket_data(obj, crafted_msg.msg_name, crafted_msg.msg_iov->iov_base,
				       crafted_msg.msg_iovlen, flags, K_SECONDS(MDM_CMD_TIMEOUT));

		if (bkp_iovec_idx != -1) {
			msg->msg_iov[bkp_iovec_idx] = bkp_iovec;
		}

		if (ret < 0) {
			errno = ret;
			return -1;
		}

		sent += ret;
	}

	return sent;
}
/* clang-format off */
static const struct socket_op_vtable offload_socket_fd_op_vtable = {
	.fd_vtable = {
			.read = offload_read,
			.write = offload_write,
			.close = offload_close,
			.ioctl = offload_ioctl,
		},
	.bind = offload_bind,
	.connect = offload_connect,
	.sendto = offload_sendto,
	.recvfrom = offload_recvfrom,
	.listen = NULL,
	.accept = NULL,
	.sendmsg = offload_sendmsg,
	.getsockopt = NULL,
#if defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS) \
	|| defined(CONFIG_MODEM_HL78XX_MQTT_OFFLOADING)
	.setsockopt = offload_setsockopt,
#else
	.setsockopt = NULL,
#endif
};
/* clang-format on */
static int hl78xx_init_sockets(const struct device *dev)
{
	int ret;

	socket_data.buf_pool = &mdm_recv_pool;
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d", __LINE__);
#endif
	k_sem_init(&socket_data.psm_cntrl_sem, 0, 1);

	/* socket config */
	ret = modem_socket_init(&socket_data.socket_config, &socket_data.sockets[0],
				ARRAY_SIZE(socket_data.sockets), MDM_BASE_SOCKET_NUM, false,
				&offload_socket_fd_op_vtable);
	if (ret) {
		goto error;
	}
	return 0;
error:
	return ret;
}

void socket_notify_data(int socket_id, int new_total)
{
	struct modem_socket *sock;
	int ret = 0;

	sock = modem_socket_from_id(&socket_data.socket_config, socket_id);
	if (!sock) {
		return;
	}
	LOG_DBG("%d new total: %d sckid: %d", __LINE__, new_total, socket_id);
	ret = modem_socket_packet_size_update(&socket_data.socket_config, sock, new_total);
	if (ret < 0) {
		LOG_ERR("socket_id:%d left_bytes:%d err: %d", socket_id, new_total, ret);
	}
	if (new_total > 0) {
		LOG_DBG("%d", new_total);
		modem_socket_data_ready(&socket_data.socket_config, sock);
	}
}

void tcp_notify_data(int socket_id, int tcp_notif)
{
	enum hl78xx_tcp_notif tcp_notif_received = tcp_notif;

	socket_data.requested_socket_id = socket_id;

	switch (tcp_notif_received) {
	case TCP_NOTIF_REMOTE_DISCONNECTION:
		socket_notify_data(socket_id, 1);
		break;
	case TCP_NOTIF_NETWORK_ERROR:
		/* Handle network error */
		break;
	default:
		break;
	}
}
#ifdef CONFIG_MODEM_HL78XX_MQTT_OFFLOADING

static size_t mqtt_encode_publish(struct net_buf_simple *buf, const char *topic, size_t topic_len,
				  const char *payload, size_t payload_len, uint8_t qos,
				  uint8_t retain)
{
	size_t length = 1 + 1 + 1 + 2 + topic_len + payload_len;
	uint8_t flags = 0;

	if (length >= 256) {
		if (net_buf_simple_tailroom(buf) < length + 2) {
			LOG_DBG("%d", __LINE__);
			return 0;
		}
		length += 2;
		net_buf_simple_add_u8(buf, 0x01);
		net_buf_simple_add_be16(buf, length);
	} else {
		if (net_buf_simple_tailroom(buf) < length) {
			LOG_DBG("%d", __LINE__);
			return 0;
		}
		net_buf_simple_add_u8(buf, (uint8_t)length);
	}

	net_buf_simple_add_u8(buf, 0x0C);  /*  PUBLISH */
	net_buf_simple_add_u8(buf, flags); /*  Flags */

	net_buf_simple_add_be16(buf, topic_len);

	net_buf_simple_add_mem(buf, topic, topic_len);
	net_buf_simple_add_mem(buf, payload, payload_len);

	return buf->len;
}

void mqtt_notify_data(int socket_id, const char *mqtt_topic, const char *mqtt_payload)
{
	/* Build a recv buffer "topic\0payload" */
	LOG_DBG("MQTT topic: %s payload: %s", mqtt_topic, mqtt_payload);
	size_t topic_len = strlen(mqtt_topic);
	size_t payload_len = strlen(mqtt_payload);
	/* len + type + flags + topiclen + payload */
	size_t total_needed_len = 3 + 1 + 1 + 2 + topic_len + payload_len;
	struct net_buf_simple buf_simple;
	uint8_t *buf = k_malloc(total_needed_len);

	if (!buf) {
		LOG_ERR("OOM for MQTT incoming");
		return;
	}

	net_buf_simple_init_with_data(&buf_simple, (void *)buf, total_needed_len);

	net_buf_simple_init(&buf_simple, 0);

	size_t mqtt_len = mqtt_encode_publish(&buf_simple, mqtt_topic, topic_len, mqtt_payload,
					      payload_len, 0, 0);
	if (mqtt_len == 0) {
		LOG_ERR("Failed to encode MQTT publish");
		return;
	}
	LOG_HEXDUMP_DBG(buf, mqtt_len, "MQTT publish");
	int written = ring_buf_put(socket_data.buf_pool, buf, mqtt_len);

	if (written < mqtt_len) {
		LOG_WRN("MQTT RX buffer overflow, dropping message %d < %d", written, mqtt_len);
		k_free(buf);
		return;
	}
	LOG_DBG("%d %d %d", __LINE__, socket_id, written);
	socket_notify_data(socket_id, written);
	k_free(buf);
}
#endif /* CONFIG_MODEM_HL78XX_MQTT_OFFLOADING */

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS)
int hl78xx_configure_chipper_suit(void)
{
	const char *cmd_chipper_suit = "AT+KSSLCRYPTO=0,8,1,8192,4,4,3,0";

	return modem_cmd_send_int(socket_data.mdata_global, NULL, cmd_chipper_suit,
				  strlen(cmd_chipper_suit), &ok_match, 1, false);
}
/* send binary data via the K....STORE commands */
static ssize_t hl78xx_send_cert(struct modem_socket *sock, const char *cert_data, size_t cert_len,
				enum tls_credential_type cert_type)
{
	int ret;
	char send_buf[sizeof("AT+KPRIVKSTORE=#,####\r\n")];
	int sock_written = 0;

	if (!sock) {
		return -EINVAL;
	}

	if (cert_len == 0 || !cert_data) {
		LOG_ERR("Invalid certificate data or length");
		return -EINVAL;
	}

	__ASSERT_NO_MSG(cert_len <= MDM_MAX_CERT_LENGTH);
	if (cert_type == TLS_CREDENTIAL_CA_CERTIFICATE ||
	    cert_type == TLS_CREDENTIAL_SERVER_CERTIFICATE) {
		snprintk(send_buf, sizeof(send_buf), "AT+KCERTSTORE=%d,%d", (cert_type - 1),
			 cert_len);

	} else if (cert_type == TLS_CREDENTIAL_PRIVATE_KEY) {
		snprintk(send_buf, sizeof(send_buf), "AT+KPRIVKSTORE=0,%d", cert_len);

	} else {
		LOG_ERR("Unsupported certificate type: %d", cert_type);
		return -EINVAL;
	}

	socket_data.socket_data_error = false;

	if (k_mutex_lock(&socket_data.mdata_global->tx_lock, K_SECONDS(1)) < 0) {
		errno = EBUSY;
		return -1;
	}
	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, send_buf, strlen(send_buf),
				 connect_matches, ARRAY_SIZE(connect_matches), false);
	if (ret < 0) {
		LOG_ERR("Error sending AT command %d", ret);
	}
	if (socket_data.socket_data_error) {
		ret = -ENODEV;
		errno = ENODEV;
		goto cleanup;
	}
	modem_pipe_attach(socket_data.mdata_global->chat.pipe, modem_pipe_callback,
			  &socket_data.mdata_global->chat);
	ret = send_data_buffer(cert_data, cert_len, &sock_written);
	if (ret < 0) {
		goto cleanup;
	}
	ret = k_sem_take(&socket_data.mdata_global->script_stopped_sem_tx_int, K_FOREVER);
	if (ret < 0) {
		goto cleanup;
	}
	ret = modem_pipe_transmit(socket_data.mdata_global->uart_pipe,
				  (uint8_t *)socket_data.mdata_global->buffers.eof_pattern,
				  socket_data.mdata_global->buffers.eof_pattern_size);
	if (ret < 0) {
		LOG_ERR("Error sending EOF pattern: %d", ret);
	}
	modem_chat_attach(&socket_data.mdata_global->chat, socket_data.mdata_global->uart_pipe);
	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, "", 0, &ok_match, 1, false);
	if (ret < 0) {
		LOG_ERR("Final confirmation failed: %d", ret);
		goto cleanup;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %d %d", __LINE__, sock_written, ret);
#endif
cleanup:
	k_mutex_unlock(&socket_data.mdata_global->tx_lock);
	return (ret < 0) ? -1 : sock_written;

	return ret;
}

static int map_credentials(struct modem_socket *sock, const void *optval, socklen_t optlen)
{
	const sec_tag_t *sec_tags = (const sec_tag_t *)optval;
	int ret = 0;
	int tags_len;
	sec_tag_t tag;
	int i;
	struct tls_credential *cert;

	if ((optlen % sizeof(sec_tag_t)) != 0 || (optlen == 0)) {
		return -EINVAL;
	}

	tags_len = optlen / sizeof(sec_tag_t);
	LOG_DBG("%d %d", __LINE__, tags_len);

	/* For each tag, retrieve the credentials value and type: */
	for (i = 0; i < tags_len; i++) {
		tag = sec_tags[i];
		cert = credential_next_get(tag, NULL);
		LOG_DBG("Processing tag: %d %d", tag, cert != NULL);
		while (cert != NULL) {
			switch (cert->type) {
			case TLS_CREDENTIAL_CA_CERTIFICATE:
				LOG_DBG("TLS_CREDENTIAL_CA_CERTIFICATE tag: %d", tag);
				break;
			case TLS_CREDENTIAL_SERVER_CERTIFICATE:
				LOG_DBG("TLS_CREDENTIAL_SERVER_CERTIFICATE tag: %d", tag);
				break;
			case TLS_CREDENTIAL_PRIVATE_KEY:
				LOG_DBG("TLS_CREDENTIAL_PRIVATE_KEY tag: %d", tag);
				break;
			case TLS_CREDENTIAL_NONE:
			case TLS_CREDENTIAL_PSK:
			case TLS_CREDENTIAL_PSK_ID:
			default:
				/* Not handled */
				return -EINVAL;
			}
			LOG_DBG("%d", __LINE__);

			ret = hl78xx_send_cert(sock, cert->buf, cert->len, cert->type);
			if (ret < 0) {
				return ret;
			}
			cert = credential_next_get(tag, cert);
		}
	}

	return 0;
}
#endif

static void l4_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			     struct net_if *iface)
{
	struct hl78xx_socket_data *data = CONTAINER_OF(cb, struct hl78xx_socket_data, l4_cb);

	ARG_UNUSED(data);

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d mgmt_event: %lld", __LINE__, mgmt_event);
#endif

	switch (mgmt_event) {
	case NET_EVENT_DNS_SERVER_ADD:
		/* Do something */
		LOG_INF("Network connectivity established and IP address assigned IPv4: %s IPv6: "
			"%s",
			data->dns.v4_string, data->dns.v6_string);
		break;
	case NET_EVENT_L4_CONNECTED:
		/* Do something */
		break;
	case NET_EVENT_L4_DISCONNECTED:
		/* Do something */
		break;
	default:
		break;
	}
}
static void connectivity_event_handler(struct net_mgmt_event_callback *cb, uint64_t event,
				       struct net_if *iface)
{
	if (event == CONN_LAYER_EVENT_MASK) {
		LOG_ERR("Fatal error received from the connectivity layer");
		/* Do something */
		return;
	}
}

void hl78xx_socket_init(struct hl78xx_data *data)
{
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d", __LINE__);
#endif
	socket_data.mdata_global = data;
	atomic_set(&state_leftover, 0);
	{
		/* Setup handler for Zephyr NET Connection Manager events. */
		net_mgmt_init_event_callback(&socket_data.l4_cb, l4_event_handler, L4_EVENT_MASK);
		net_mgmt_add_event_callback(&socket_data.l4_cb);

		/* Setup handler for Zephyr NET Connection Manager Connectivity layer. */
		net_mgmt_init_event_callback(&socket_data.conn_cb, connectivity_event_handler,
					     CONN_LAYER_EVENT_MASK);
		net_mgmt_add_event_callback(&socket_data.conn_cb);
	}
}

static void modem_net_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct hl78xx_socket_data *data = dev->data;
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d", __LINE__);
#endif
	net_if_set_link_addr(
		iface, modem_get_mac(socket_data.mac_addr, socket_data.mdata_global->identity.imei),
		sizeof(data->mac_addr), NET_LINK_ETHERNET);
	data->net_iface = iface;

	hl78xx_init_sockets(dev);
	net_if_socket_offload_set(iface, offload_socket);
}

static struct offloaded_if_api api_funcs = {
	.iface_api.init = modem_net_iface_init,
};

static bool offload_is_supported(int family, int type, int proto)
{
	return (family == AF_INET || family == AF_INET6) &&
	       (type == SOCK_DGRAM || type == SOCK_STREAM) &&
	       (proto == IPPROTO_TCP || proto == IPPROTO_UDP
#if defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS)
		|| proto == IPPROTO_TLS_1_2
#endif
	       );
}

#define MODEM_HL78XX_DEFINE_INSTANCE(inst)                                                         \
	NET_DEVICE_OFFLOAD_INIT(inst, "hl78xx_dev", NULL, NULL, &socket_data, NULL,                \
				CONFIG_MODEM_HL78XX_OFFLOAD_INIT_PRIORITY, &api_funcs,             \
				MDM_MAX_DATA_LENGTH);                                              \
	NET_SOCKET_OFFLOAD_REGISTER(inst, CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY, AF_UNSPEC,          \
				    offload_is_supported, offload_socket);

#define MODEM_DEVICE_SWIR_HL78XX(inst) MODEM_HL78XX_DEFINE_INSTANCE(inst)

#define DT_DRV_COMPAT swir_hl7812
DT_INST_FOREACH_STATUS_OKAY(MODEM_DEVICE_SWIR_HL78XX)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT swir_hl7800
DT_INST_FOREACH_STATUS_OKAY(MODEM_DEVICE_SWIR_HL78XX)
#undef DT_DRV_COMPAT
