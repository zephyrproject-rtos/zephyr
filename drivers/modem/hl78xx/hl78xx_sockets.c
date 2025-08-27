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

LOG_MODULE_REGISTER(hl78xx_socket, CONFIG_MODEM_LOG_LEVEL);

/* Helper macros and constants */
#define MODEM_STREAM_STARTER_WORD "\r\n" CONNECT_STRING "\r\n"
#define MODEM_STREAM_END_WORD     "\r\n" OK_STRING "\r\n"

#define MODEM_SOCKET_DATA_LEFTOVER_STATE_BIT (0)

#define DNS_SERVERS_COUNT                                                                          \
	(0 + (IS_ENABLED(CONFIG_NET_IPV6) ? 1 : 0) + (IS_ENABLED(CONFIG_NET_IPV4) ? 1 : 0) +       \
	 1 /* for NULL terminator */                                                               \
	)
#define L4_EVENT_MASK         (NET_EVENT_DNS_SERVER_ADD | NET_EVENT_L4_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

#define HL78XX_UART_PIPE_WORK_SOCKET_BUFFER_SIZE 32
#define HL78XX_MAC_ADDR_SIZE                     6
#define MODEM_MAX_HOSTNAME_LEN                   128

RING_BUF_DECLARE(mdm_recv_pool, CONFIG_MODEM_HL78XX_UART_BUFFER_SIZES);

/* ---------------- Global Data Structures ---------------- */
struct hl78xx_socket_data {
	struct net_if *net_iface;
	uint8_t mac_addr[HL78XX_MAC_ADDR_SIZE];

	/* socket data */
	struct modem_socket_config socket_config;
	struct modem_socket sockets[MDM_MAX_SOCKETS];
	int current_sock_fd;
	int sizeof_socket_data;
	int requested_socket_id;

	char dns_v4_string[NET_IPV4_ADDR_LEN];
	char dns_v6_string[NET_IPV6_ADDR_LEN];
	bool dns_ready;
	struct in_addr ipv4Addr;
	struct in_addr subnet;
	struct in_addr gateway;
	struct in_addr dns_v4;
	struct in_addr new_ipv4_addr;
	struct in6_addr dns_v6;
	/* rx net buffer */
	struct ring_buf *buf_pool;
	uint32_t expected_buf_len;
	uint32_t collected_buf_len;

	struct hl78xx_data *mdata_global;

	struct net_mgmt_event_callback l4_cb;
	struct net_mgmt_event_callback conn_cb;

	struct k_sem psm_cntrl_sem;

	bool socket_data_error;

	char tls_hostname[MODEM_MAX_HOSTNAME_LEN];
	bool tls_hostname_set;
};
struct work_socket_data {
	char buf[HL78XX_UART_PIPE_WORK_SOCKET_BUFFER_SIZE];
	uint16_t len;
};

struct receive_socket_data {
	char buf[MDM_MAX_DATA_LENGTH + ARRAY_SIZE(MODEM_STREAM_STARTER_WORD) +
		 ARRAY_SIZE(MODEM_STREAM_END_WORD)];
	uint16_t len;
};

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
static int hl78xx_configure_chipper_suit(void);
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
		ret = strncmp(dns_str, socket_data.dns_v4_string, strlen(dns_str));
		if (ret != 0) {
			LOG_DBG("New IPv4 DNS differs from current, marking dns_ready = false");
			socket_data.dns_ready = false;
		}
		strncpy(socket_data.dns_v4_string, dns_str, sizeof(socket_data.dns_v4_string));
		socket_data.dns_v4_string[sizeof(socket_data.dns_v4_string) - 1] = '\0';
		return parse_ip(true, socket_data.dns_v4_string, &socket_data.dns_v4);
	}
#ifdef CONFIG_NET_IPV6
	else {
		ret = strncmp(dns_str, socket_data.dns_v6_string, strlen(dns_str));
		if (ret != 0) {
			LOG_DBG("New IPv6 DNS differs from current, marking dns_ready = false");
			socket_data.dns_ready = false;
		}
		strncpy(socket_data.dns_v6_string, dns_str, sizeof(socket_data.dns_v6_string));
		socket_data.dns_v6_string[sizeof(socket_data.dns_v6_string) - 1] = '\0';

		if (!parse_ip(false, socket_data.dns_v6_string, &socket_data.dns_v6)) {
			return false;
		}

		net_addr_ntop(AF_INET6, &socket_data.dns_v6, socket_data.dns_v6_string,
			      sizeof(socket_data.dns_v6_string));
		LOG_DBG("Parsed IPv6 DNS: %s", socket_data.dns_v6_string);
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
		if (is_valid_ipv4_addr(&socket_data.ipv4Addr)) {
			net_if_ipv4_addr_rm(socket_data.net_iface, &socket_data.ipv4Addr);
		}

		if (!net_if_ipv4_addr_add(socket_data.net_iface, &socket_data.new_ipv4_addr,
					  NET_ADDR_DHCP, 0)) {
			LOG_ERR("Failed to set IPv4 interface address.");
		}

		net_if_ipv4_set_netmask_by_addr(socket_data.net_iface, &socket_data.new_ipv4_addr,
						&socket_data.subnet);
		net_if_ipv4_set_gw(socket_data.net_iface, &socket_data.gateway);

		net_ipaddr_copy(&socket_data.ipv4Addr, &socket_data.new_ipv4_addr);
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

	if (!sock->is_connected && sock->ip_proto != IPPROTO_UDP) {
		errno = ENOTCONN;
		return -1;
	}

	return 0;
}

static void hl78xx_on_kudpsnd(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct modem_socket *sock = NULL;
	int id;

	/* look up new socket by special id */
	sock = modem_socket_from_newid(&socket_data.socket_config);
	if (sock) {
		id = ATOI(argv[1], -1, "socket_id");
		sock->is_connected = true;

		/* on error give up modem socket */
		if (modem_socket_id_assign(&socket_data.socket_config, sock, id) < 0) {

			modem_socket_put(&socket_data.socket_config, sock->sock_fd);
		}
	}
}

/* Handler: +USOCR: <socket_id>[0] */
static void hl78xx_on_cmd_sockcreate(struct modem_chat *chat, char **argv, uint16_t argc,
				     void *user_data)
{
	struct modem_socket *sock = NULL;
	int socket_id;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d", __LINE__);
#endif
	/* look up new socket by special id */
	sock = modem_socket_from_newid(&socket_data.socket_config);
	if (sock) {
		socket_id = ATOI(argv[1], -1, "socket_id");
		/* on error give up modem socket */
		if (modem_socket_id_assign(&socket_data.socket_config, sock, socket_id) < 0) {
			modem_socket_put(&socket_data.socket_config, sock->sock_fd);
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
		if (!parse_ip(true, ip_addr, &socket_data.new_ipv4_addr)) {
			return;
		}
		if (!parse_ip(true, subnet_mask, &socket_data.subnet)) {
			return;
		}
		if (!parse_ip(true, argv[5], &socket_data.gateway)) {
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
		LOG_ERR("CME ERROR: Incomplete response");
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
MODEM_CHAT_MATCH_DEFINE(kudpind_match, "+KUDP_IND: ", ",", hl78xx_on_kudpsnd);
MODEM_CHAT_MATCH_DEFINE(ktcpind_match, "+KTCP_IND: ", ",", hl78xx_on_kudpsnd);
MODEM_CHAT_MATCH_DEFINE(ktcp_match, "+KTCPCFG: ", "", hl78xx_on_cmd_sockcreate);
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

	char *cmd = "AT+CGCONTRDP=1";
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
		socket_data.dns_v4_string,
#endif
		NULL};
	const char *dns_servers_wrapped[ARRAY_SIZE(dns_servers_str)];

	if (reset) {
		LOG_DBG("Resetting DNS resolver");
		dnsCtx = dns_resolve_get_default();
		if (dnsCtx->state != DNS_RESOLVE_CONTEXT_INACTIVE) {
			dns_resolve_close(dnsCtx);
		}
		socket_data.dns_ready = false;
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
	valid_address = net_ipaddr_parse(socket_data.dns_v4_string,
					 strlen(socket_data.dns_v4_string), &temp_addr);
#endif
	if (!valid_address) {
		LOG_WRN("No valid DNS address!");
		return;
	}
	if (!socket_data.net_iface || !net_if_is_up(socket_data.net_iface) ||
	    socket_data.dns_ready) {
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
		socket_data.dns_ready = true;
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
		LOG_ERR("Data retrieval mismatch: expected %u, got %d", len, ret);
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
static int create_socket(struct modem_socket *sock, const struct sockaddr *addr,
			 struct hl78xx_data *data)
{
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d", __LINE__);
#endif
	int ret;
	int af;
	char cmd_buf[sizeof("AT+KTCPCFG=#,#,\"" MODEM_HL78XX_ADDRESS_FAMILY_FORMAT
			    "\",#####,,,,#,,#") +
		     MDM_MAX_HOSTNAME_LEN + NET_IPV6_ADDR_LEN + MDM_MQTT_MAX_CLIENT_ID_LEN];
	char ip_str[NET_IPV6_ADDR_LEN];
	uint16_t dst_port = 0U;
	bool udp_configproto = false;
	uint8_t mode = 0;

	memcpy(&sock->dst, addr, sizeof(*addr));
	if (addr->sa_family == AF_INET6) {
		dst_port = ntohs(net_sin6(addr)->sin6_port);
		af = MDM_HL78XX_SOCKET_AF_IPV6;
	} else if (addr->sa_family == AF_INET) {
		dst_port = ntohs(net_sin(addr)->sin_port);
		af = MDM_HL78XX_SOCKET_AF_IPV4;
	} else {
		errno = EAFNOSUPPORT;
		return -1;
	}

	if (sock->ip_proto == IPPROTO_UDP) {
		udp_configproto = true;
	}

	ret = modem_context_sprint_ip_addr(addr, ip_str, sizeof(ip_str));
	if (ret != 0) {
		LOG_ERR("Failed to format IP!");
		errno = ENOMEM;
		return -1;
	}
	if (!udp_configproto) {
		if (sock->ip_proto == IPPROTO_TCP) {
			mode = 0; /* TCP */
		} else if (sock->ip_proto == IPPROTO_TLS_1_2) {
			mode = 3; /* TLS */
		} else {
			LOG_ERR("Unsupported protocol: %d", sock->ip_proto);
			errno = EPROTONOSUPPORT;
			return -1;
		}
		snprintk(cmd_buf, sizeof(cmd_buf), "AT+KTCPCFG=1,%d,\"%s\",%u,,,,%d,%s,0", mode,
			 mode == 3 ? socket_data.tls_hostname : ip_str, dst_port, af,
			 mode == 3 ? "0" : "");
		ret = modem_cmd_send_int(data, NULL, cmd_buf, strlen(cmd_buf), &ktcp_match, 1,
					 false);
		if (ret < 0) {
			goto error;
		}
	} else {

		uint8_t display_data_urc = 2;

#if defined(CONFIG_MODEM_HL78XX_SOCKET_UDP_DISPLAY_DATA_URC)
		display_data_urc = CONFIG_MODEM_HL78XX_SOCKET_UDP_DISPLAY_DATA_URC;
#endif
		snprintk(cmd_buf, sizeof(cmd_buf), "AT+KUDPCFG=1,%u,,%d,,,%d,%d", mode,
			 display_data_urc, (addr->sa_family - 1), 0);

		ret = modem_cmd_send_int(data, NULL, cmd_buf, strlen(cmd_buf), &kudpind_match, 1,
					 false);
		if (ret < 0) {
			goto error;
		}
	}
	errno = 0;
	return 0;
error:
	LOG_ERR("%s ret:%d", cmd_buf, ret);
	modem_socket_put(&socket_data.socket_config, sock->sock_fd);
	errno = ret;
	return -1;
}

static int socket_close(struct modem_socket *sock)
{
	char buf[sizeof("AT+KTCPCLOSE=##\r")];
	int ret = 0;

	if (sock->ip_proto == IPPROTO_UDP) {
		snprintk(buf, sizeof(buf), "AT+KUDPCLOSE=%d", sock->id);
	} else {
		snprintk(buf, sizeof(buf), "AT+KTCPCLOSE=%d", sock->id);
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
		snprintk(buf, sizeof(buf), "AT+KTCPDEL=%d", sock->id);
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
	/* defer modem's socket create call to bind() */
	ret = modem_socket_get(&socket_data.socket_config, family, type, proto);
	if (ret < 0) {
		errno = ret;
		return -1;
	}

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
	int mode;
	char cmd_buf[sizeof("AT+KTCPCFG=#,#,\"" MODEM_HL78XX_ADDRESS_FAMILY_FORMAT
			    "\",#####,,,,#,,#\r") +
		     MODEM_MAX_HOSTNAME_LEN];
	char ip_str[NET_IPV6_ADDR_LEN];
	uint16_t dst_port = 0U;

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
	if (modem_socket_id_is_assigned(&socket_data.socket_config, sock) == false &&
	    sock->ip_proto == IPPROTO_UDP) {
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
#ifdef CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS
	ret = hl78xx_configure_chipper_suit();
	if (ret < 0) {
		LOG_ERR("Failed to configure chipper suit: %d", ret);
		errno = ret;
		return -1;
	}
#endif /* CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS */
	ret = modem_context_sprint_ip_addr(addr, ip_str, sizeof(ip_str));
	if (ret != 0) {
		errno = -ret;
		LOG_ERR("Error formatting IP string %d", ret);
		return -1;
	}
	if (sock->ip_proto == IPPROTO_TCP) {
		mode = 0; /* TCP */
	} else if (sock->ip_proto == IPPROTO_TLS_1_2) {
		mode = 3; /* TLS */
	} else {
		LOG_ERR("Unsupported protocol: %d", sock->ip_proto);
		errno = EPROTONOSUPPORT;
		return -1;
	}
	snprintk(cmd_buf, sizeof(cmd_buf), "AT+KTCPCFG=1,%d,\"%s\",%u,,,,%d,%s,0", mode,
		 mode == 3 ? socket_data.tls_hostname : ip_str, dst_port, af, mode == 3 ? "0" : "");

	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, cmd_buf, strlen(cmd_buf),
				 &ktcp_match, 1, false);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", cmd_buf, ret);
		errno = ret;
		return -1;
	}

	snprintk(cmd_buf, sizeof(cmd_buf), "AT+KTCPCNX=%d", sock->id);
	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, cmd_buf, strlen(cmd_buf),
				 &ok_match, 1, false);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", cmd_buf, ret);
		errno = ret;
		return -1;
	}
	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, "", 0, &ktcpind_match, 1, false);
	if (ret) {
		LOG_ERR("Error sending data %d", ret);
		ret = ETIMEDOUT;
		LOG_ERR("%d No TCP_IND received, ret: %d", __LINE__, ret);
		return -1;
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

	prepare_read_command(sendbuf, sizeof(sendbuf), sock, read_size);
	setup_socket_data(sock, &sock_data, buf, len, from, read_size);

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
			/* if there is any socket  connected*/
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

/* send binary data via the +KUDPSND/+KTCPSND commands */
static ssize_t send_socket_data(void *obj, const struct sockaddr *dst_addr, const char *buf,
				size_t buf_len, k_timeout_t timeout)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	char cmd_buf[82] = {0}; /* AT+KUDPSND/KTCP=,IP,PORT,LENGTH */
	int ret;
	int sock_written = 0;

	ret = validate_socket(sock);
	if (ret < 0) {
		return ret;
	}
	if (!dst_addr && sock->ip_proto == IPPROTO_UDP) {
		dst_addr = &sock->dst;
	}
	if (buf_len > MDM_MAX_DATA_LENGTH) {
		if (sock->type == SOCK_DGRAM) {
			errno = EMSGSIZE;
			return -1;
		}
		buf_len = MDM_MAX_DATA_LENGTH;
	}
	if (sock->ip_proto == IPPROTO_UDP) {
		ret = prepare_udp_send_cmd(sock, dst_addr, buf_len, cmd_buf, sizeof(cmd_buf));
	} else {
		ret = prepare_tcp_send_cmd(sock, buf_len, cmd_buf, sizeof(cmd_buf));
	}
	if (ret < 0) {
		return -1;
	}

	socket_data.socket_data_error = false;

	if (k_mutex_lock(&socket_data.mdata_global->tx_lock, K_SECONDS(1)) < 0) {
		errno = ret;
		return -1;
	}
	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, cmd_buf, strlen(cmd_buf),
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

	ret = send_data_buffer(buf, buf_len, &sock_written);
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
}

#ifdef CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS
static int offload_setsockopt(void *obj, int level, int optname, const void *optval,
			      socklen_t optlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;

	int ret;
	/* Currently only TLS options are supported */
	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && level == SOL_TLS) {
		switch (optname) {
		case TLS_SEC_TAG_LIST:
			ret = map_credentials(sock, optval, optlen);
			break;
		case TLS_HOSTNAME:
			if (optlen >= MODEM_MAX_HOSTNAME_LEN) {
				return -EINVAL;
			}

			memset(socket_data.tls_hostname, 0, MODEM_MAX_HOSTNAME_LEN);
			memcpy(socket_data.tls_hostname, optval, optlen);
			socket_data.tls_hostname[optlen] = '\0';
			socket_data.tls_hostname_set = true;

			LOG_DBG("TLS hostname set to: %s", socket_data.tls_hostname);
			ret = 0;
			return 0;
		case TLS_PEER_VERIFY:
			if (*(const uint32_t *)optval != TLS_PEER_VERIFY_REQUIRED) {
				LOG_WRN("Disabling peer verification is not supported");
				return 0;
			}
			ret = 0;
			break;
		default:
			LOG_DBG("Unsupported TLS option: %d", optname);
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	return ret;
}
#endif /* CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS */

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

	ret = send_socket_data(obj, to, buf, len, K_SECONDS(MDM_CMD_TIMEOUT));
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
				       crafted_msg.msg_iovlen, K_SECONDS(MDM_CMD_TIMEOUT));

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
	#ifdef CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS
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
	LOG_DBG("%d new total: %d sckid%d", __LINE__, new_total, socket_id);
	ret = modem_socket_packet_size_update(&socket_data.socket_config, sock, new_total);
	if (ret < 0) {
		LOG_ERR("socket_id:%d left_bytes:%d err: %d", socket_id, new_total, ret);
	}
	if (new_total > 0) {
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

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS)
static int hl78xx_configure_chipper_suit(void)
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
	LOG_DBG("%d", __LINE__);
	ret = send_data_buffer(cert_data, cert_len, &sock_written);
	if (ret < 0) {
		goto cleanup;
	}
	LOG_DBG("%d", __LINE__);
	ret = k_sem_take(&socket_data.mdata_global->script_stopped_sem_tx_int, K_FOREVER);
	if (ret < 0) {
		goto cleanup;
	}
	LOG_DBG("%d", __LINE__);
	ret = modem_pipe_transmit(socket_data.mdata_global->uart_pipe,
				  (uint8_t *)socket_data.mdata_global->buffers.eof_pattern,
				  socket_data.mdata_global->buffers.eof_pattern_size);
	if (ret < 0) {
		LOG_ERR("Error sending EOF pattern: %d", ret);
	}
	LOG_DBG("%d", __LINE__);
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
	/* For each tag, retrieve the credentials value and type: */
	for (i = 0; i < tags_len; i++) {
		tag = sec_tags[i];
		cert = credential_next_get(tag, NULL);
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
			data->dns_v4_string, data->dns_v6_string);
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
