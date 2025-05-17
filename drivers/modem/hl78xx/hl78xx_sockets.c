/*
 * Copyright (c) 2025 Netfeasa
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
#include <string.h>
#include <stdlib.h>
#include "hl78xx.h"

LOG_MODULE_REGISTER(hl78xx_socket, CONFIG_MODEM_LOG_LEVEL);

/* Helper macros and constants */
#define CGCONTRDP_RESPONSE_NUM_DELIMS 7
#define MDM_IP_INFO_RESP_SIZE         256

#define MODEM_STREAM_STARTER_WORD "\r\n" CONNECT_STRING "\r\n"
#define DNS_SERVERS_COUNT                                                                          \
	(0 + (IS_ENABLED(CONFIG_NET_IPV6) ? 1 : 0) + (IS_ENABLED(CONFIG_NET_IPV4) ? 1 : 0) +       \
	 1 /* for NULL terminator */                                                               \
	)
RING_BUF_DECLARE(mdm_recv_pool, CONFIG_MODEM_HL78XX_UART_BUFFER_SIZES);

/* ---------------- Global Data Structures ---------------- */
struct hl78xx_socket_data {
	struct net_if *net_iface;
	uint8_t mac_addr[6];

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
};

struct hl78xx_socket_data socket_data;

static int offload_socket(int family, int type, int protom);

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
	LOG_DBG("%d %s", __LINE__, __func__);
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
void hl78xx_on_kstatev_parser(struct hl78xx_data *data, int state)
{
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("KSTATEV: socket %d state %d", socket_data.current_sock_fd, state);
#endif
	switch (state) {
	case 1: /* CLOSED */
		LOG_DBG("Socket %d closed by modem (KSTATEV: 2 1), resetting socket",
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

static void hl78xx_on_cgdcontrdp(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data)
{
	if (argc < 8 || !argv[4] || !argv[5] || !argv[6]) {
		LOG_ERR("Incomplete CGCONTRDP response: argc = %d", argc);
		return;
	}

	LOG_DBG("Parsing CGCONTRDP: addr=%s gw=%s dns=%s", argv[4], argv[5], argv[6]);

	bool is_ipv4 = (strchr(argv[4], ':') == NULL);

	char ip_addr[NET_IPV6_ADDR_LEN];
	char subnet_mask[NET_IPV6_ADDR_LEN];

	if (!split_ipv4_and_subnet(argv[4], ip_addr, sizeof(ip_addr), subnet_mask,
				   sizeof(subnet_mask))) {
		return;
	}

	LOG_DBG("Extracted IP: %s, Subnet: %s", ip_addr, subnet_mask);

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

MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", NULL);
MODEM_CHAT_MATCHES_DEFINE(connect_matches, MODEM_CHAT_MATCH(CONNECT_STRING, "", NULL),
			  MODEM_CHAT_MATCH("ERROR", "", NULL));
MODEM_CHAT_MATCH_DEFINE(kudpind_match, "+KUDP_IND: ", ",", hl78xx_on_kudpsnd);
MODEM_CHAT_MATCH_DEFINE(ktcpind_match, "+KTCP_IND: ", ",", hl78xx_on_kudpsnd);
MODEM_CHAT_MATCH_DEFINE(ktcp_match, "+KTCPCFG: ", "", hl78xx_on_cmd_sockcreate);
MODEM_CHAT_MATCH_DEFINE(cgdcontrdp_match, "+CGCONTRDP: ", ",", hl78xx_on_cgdcontrdp);

static int modem_process_handler(void)
{
	int ret = 0;
	int err = 0;
	uint8_t *data_recv;
	size_t bytes_to_read = socket_data.expected_buf_len;
	size_t to_copy;
	uint32_t size = ring_buf_put_claim(socket_data.buf_pool, &data_recv,
					   CONFIG_MODEM_HL78XX_UART_BUFFER_SIZES);

	if (size == 0 || data_recv == NULL) {
		LOG_ERR("Failed to claim ring buffer");
		return -ENOMEM;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("Processing handler - Bytes to read: %zu, Collected: %zu, Buffer size: %u",
		bytes_to_read, socket_data.collected_buf_len, size);
#endif
	to_copy = MIN(size, (bytes_to_read - socket_data.collected_buf_len));

	ret = modem_pipe_receive(socket_data.mdata_global->uart_pipe, data_recv, to_copy);
	if (ret < 0) {
		LOG_ERR("Receive failed: %d", ret);
		return ret;
	}
	if (ret == 0) {
		LOG_DBG("No data received");
		return 0;
	}
	socket_data.collected_buf_len += ret;
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s %d %d %d", __LINE__, __func__, to_copy, bytes_to_read,
		socket_data.collected_buf_len);
#endif
	err = ring_buf_put_finish(socket_data.buf_pool, ret);
	if (err != 0) {
		LOG_ERR("ring_buf_put_finish() failed: %d", err);
		return -EIO;
	}

	return socket_data.collected_buf_len;
}

static void modem_pipe_callback(struct modem_pipe *pipe, enum modem_pipe_event event,
				void *user_data)
{
	int ret = 0;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s Pipe event received: %d", __LINE__, __func__, event);
#endif
	switch (event) {
	case MODEM_PIPE_EVENT_RECEIVE_READY:
		if (socket_data.expected_buf_len > socket_data.collected_buf_len) {
			ret = modem_process_handler();
			if (ret > 0 &&
			    socket_data.collected_buf_len == socket_data.expected_buf_len) {
				k_sem_give(&socket_data.mdata_global->script_stopped_sem_rx_int);
			}
		}
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
void dns_work_cb(void)
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
		ret = dns_resolve_reconfigure(dnsCtx, dns_servers_wrapped, NULL);
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

static int on_cmd_sockread_common(int socket_id, int socket_data_length, uint16_t len,
				  void *user_data)
{
	struct modem_socket *sock;
	struct socket_read_data *sock_data;
	int ret = 0;
	int packet_size = 0;
	const uint8_t *eof_ptr;
	uint8_t sizeto_eliminate;

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

	if (!len || socket_data_length <= 0) {
		LOG_ERR("Invalid data length: %d. Aborting!", socket_data_length);
		return -EAGAIN;
	}

	if (len < socket_data_length) {
		LOG_DBG("Incomplete data received! Expected: %d, Received: %d", socket_data_length,
			len);
		return -EAGAIN;
	}

	/* Process received data */
	const char *pattern = EOF_PATTERN;
	size_t pat_len = strlen(pattern);
	uint8_t temp_buf[len + 1];
	size_t eof_pos;

	ret = ring_buf_get(socket_data.buf_pool, temp_buf, len);
	if (ret != len) {
		LOG_ERR("Data retrieval mismatch: expected %zu, got %d", len, ret);
		return -EAGAIN;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_HEXDUMP_DBG(temp_buf, len, "Received Data:");
#endif
	eof_ptr = c99_memmem(temp_buf, len, pattern, pat_len);
	if (!eof_ptr) {
		LOG_DBG("EOF pattern not found!");
		return -EINVAL;
	}

	/* Extract actual data */
	eof_pos = eof_ptr - temp_buf;
	temp_buf[eof_pos] = '\0';

	sizeto_eliminate = sizeof(MODEM_STREAM_STARTER_WORD) - 1;
	if (eof_pos < sizeto_eliminate) {
		LOG_ERR("Invalid data size! EOF pos: %zu", eof_pos);
		return -EINVAL;
	}

	eof_pos -= sizeto_eliminate;

	if (sock_data->recv_buf_len < eof_pos) {
		LOG_ERR("Buffer overflow! Received: %zu vs. Available: %zu", eof_pos,
			sock_data->recv_buf_len);
		return -EINVAL;
	}

	memcpy(sock_data->recv_buf, temp_buf + sizeto_eliminate, eof_pos);
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_HEXDUMP_DBG(sock_data->recv_buf, eof_pos, "Processed Data");
#endif
	sock_data->recv_read_len = eof_pos;

	if (sock_data->recv_read_len != socket_data_length) {
		LOG_ERR("Data mismatch! Copied: %zu vs. Received: %d", eof_pos, socket_data_length);
		return -EINVAL;
	}

	/* Remove packet from list */
	packet_size = modem_socket_next_packet_size(&socket_data.socket_config, sock);
	modem_socket_packet_size_update(&socket_data.socket_config, sock, -packet_size);
	ring_buf_reset(socket_data.buf_pool);
	socket_data.collected_buf_len = 0;

	return eof_pos;
}

int modem_handle_data_capture(size_t target_len, struct hl78xx_data *data)
{
	bool collection_finished = false;

	if ((socket_data.collected_buf_len) >= target_len) {
		collection_finished = true;
	}
	if (collection_finished) {
		return on_cmd_sockread_common(socket_data.current_sock_fd,
					      socket_data.sizeof_socket_data,
					      socket_data.collected_buf_len, data);
	}
	/* Captured or waiting — caller should exit parsing */
	return -ENOMEM;
}
/*
 * generic socket creation function
 * which can be called in bind() or connect()
 */
static int create_socket(struct modem_socket *sock, const struct sockaddr *addr,
			 struct hl78xx_data *data)
{
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s", __LINE__, __func__);
#endif
	int ret;
	int af;
	char cmd_buf[sizeof("AT+KTCPCFG=#,#,\"" MODEM_HL78XX_ADDRESS_FAMILY_FORMAT
			    "\",#####,,,,#,,#") +
		     NET_IPV6_ADDR_LEN];
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
		snprintk(cmd_buf, sizeof(cmd_buf), "AT+KTCPCFG=%d,%d,\"%s\",%u,,,,%d,,%d", 1U, 0U,
			 ip_str, dst_port, af, 0U);
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
	errno = -ret;
	return -1;
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
		errno = -ret;
		return -1;
	}

	errno = 0;
	return ret;
}

static int offload_close(void *obj)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	char buf[sizeof("AT+KTCPCLOSE=##\r")];
	int ret;

	if (!hl78xx_is_registered(socket_data.mdata_global)) {
		LOG_ERR("Modem currently not attached to the network!");
		return -EAGAIN;
	}
	/* make sure socket is allocated and assigned an id */
	if (modem_socket_id_is_assigned(&socket_data.socket_config, sock) == false) {
		return 0;
	}

	if (sock->is_connected) {
		if (sock->ip_proto == IPPROTO_UDP) {
			snprintk(buf, sizeof(buf), "AT+KUDPCLOSE=%d", sock->id);

			ret = modem_cmd_send_int(socket_data.mdata_global, NULL, buf, strlen(buf),
						 NULL, 0, false);
			if (ret < 0) {
				LOG_ERR("%s ret:%d", buf, ret);
			}
		} else {
			snprintk(buf, sizeof(buf), "AT+KTCPCLOSE=%d", sock->id);
			ret = modem_cmd_send_int(socket_data.mdata_global, NULL, buf, strlen(buf),
						 NULL, 0, false);
			if (ret < 0) {
				LOG_ERR("%s ret:%d", buf, ret);
			}
		}
		sock->is_connected = false;
	}

	modem_socket_put(&socket_data.socket_config, sock->sock_fd);
	/* Consider here successfully socket is closed */
	hl78xx_delegate_event(socket_data.mdata_global, MODEM_HL78XX_EVENT_SCRIPT_SUCCESS);
	return 0;
}

static int offload_bind(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s", __LINE__, __func__);
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
	char buf[sizeof("AT+KTCPCFG=#,#,\"" MODEM_HL78XX_ADDRESS_FAMILY_FORMAT
			"\",#####,,,,#,,#\r")];
	char ip_str[NET_IPV6_ADDR_LEN];
	uint16_t dst_port = 0U;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s", __LINE__, __func__);
#endif
	if (!addr) {
		errno = EINVAL;
		return -1;
	}
	if (!hl78xx_is_registered(socket_data.mdata_global)) {
		errno = ENETUNREACH;
		return -1;
	}
	if (sock->is_connected == true) {
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
		LOG_DBG("%d %s no socket assigned", __LINE__, __func__);
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
	ret = modem_context_sprint_ip_addr(addr, ip_str, sizeof(ip_str));
	if (ret != 0) {
		errno = -ret;
		LOG_ERR("Error formatting IP string %d", ret);
		return -1;
	}
	snprintk(buf, sizeof(buf), "AT+KTCPCFG=%d,%d,\"%s\",%u,,,,%d,,%d", 1U, 0U, ip_str, dst_port,
		 af, 0U);
	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, buf, strlen(buf), &ktcp_match, 1,
				 false);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", buf, ret);
		errno = -ret;
		return -1;
	}

	snprintk(buf, sizeof(buf), "AT+KTCPCNX=%d", sock->id);
	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, buf, strlen(buf), &ok_match, 1,
				 false);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", buf, ret);
		errno = -ret;
		return -1;
	}
	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, "", 0, &ktcpind_match, 1, false);
	if (ret) {
		LOG_ERR("Error sending data %d", ret);
		ret = -ETIMEDOUT;
		LOG_ERR("%d No TCP_IND received, ret: %d", __LINE__, ret);
		return -1;
	}
	sock->is_connected = true;
	errno = 0;
	return 0;
}

static ssize_t offload_recvfrom(void *obj, void *buf, size_t len, int flags, struct sockaddr *from,
				socklen_t *fromlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	int ret;
	int next_packet_size;
	char sendbuf[sizeof("AT+KUDPRCV=#,##########\r\n")];
	struct socket_read_data sock_data;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s", __LINE__, __func__);
#endif
	if (!buf || len == 0) {
		errno = EINVAL;
		return -1;
	}

	if (flags & ZSOCK_MSG_PEEK) {
		errno = ENOTSUP;
		return -1;
	}

	ret = k_mutex_lock(&socket_data.mdata_global->tx_lock, K_SECONDS(1));
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	next_packet_size = modem_socket_next_packet_size(&socket_data.socket_config, sock);
	if (!next_packet_size) {
		if (flags & ZSOCK_MSG_DONTWAIT) {
			errno = EAGAIN;
			ret = -1;
			goto exit;
		}

		if (!sock->is_connected && sock->ip_proto != IPPROTO_UDP) {
			errno = 0;
			ret = 0;
			goto exit;
		}

		modem_socket_wait_data(&socket_data.socket_config, sock);
		next_packet_size = modem_socket_next_packet_size(&socket_data.socket_config, sock);
	}

	/*
	 * Binary and ASCII mode allows sending MDM_MAX_DATA_LENGTH bytes to
	 * the socket in one command
	 */
	uint32_t max_data_length =
		MDM_MAX_DATA_LENGTH - (socket_data.mdata_global->buffers.eof_pattern_size +
				       sizeof(MODEM_STREAM_STARTER_WORD) - 1);

	if (next_packet_size > max_data_length) {
		next_packet_size = max_data_length;
	}
	uint16_t read_size = MIN(next_packet_size, len);
	/* +KTCPRCV / +KUDPRCV */
	snprintk(sendbuf, sizeof(sendbuf), "AT+K%sRCV=%d,%zd%s",
		 sock->ip_proto == IPPROTO_UDP ? "UDP" : "TCP", sock->id, read_size,
		 socket_data.mdata_global->chat.delimiter);

	/* socket read settings */
	(void)memset(&sock_data, 0, sizeof(sock_data));
	sock_data.recv_buf = buf;
	sock_data.recv_buf_len = len;
	sock_data.recv_addr = from;
	sock->data = &sock_data;
	socket_data.sizeof_socket_data = read_size;
	socket_data.requested_socket_id = sock->id;
	socket_data.current_sock_fd = sock->sock_fd;
	socket_data.expected_buf_len = read_size +
				       socket_data.mdata_global->buffers.eof_pattern_size +
				       sizeof(MODEM_STREAM_STARTER_WORD) - 1;
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s socket_fd: %d, socket_id: %d, expected_data_len: %d", __LINE__, __func__,
		socket_data.current_sock_fd, socket_data.requested_socket_id,
		socket_data.expected_buf_len);
#endif
	modem_chat_release(&socket_data.mdata_global->chat);
	modem_pipe_attach(socket_data.mdata_global->uart_pipe, modem_pipe_callback,
			  socket_data.mdata_global);
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_HEXDUMP_DBG(sendbuf, strlen(sendbuf), "sending");
#endif
	/* Transmit the receive command */
	ret = k_sem_take(&socket_data.mdata_global->script_stopped_sem_tx_int, K_FOREVER);
	if (ret < 0) {
		goto exit;
	}

	ret = modem_pipe_transmit(socket_data.mdata_global->uart_pipe, sendbuf, strlen(sendbuf));
	if (ret < 0) {
		goto exit;
	}

	ret = k_sem_take(&socket_data.mdata_global->script_stopped_sem_rx_int, K_FOREVER);
	if (ret < 0) {
		goto exit;
	}
	ret = modem_handle_data_capture(socket_data.expected_buf_len, socket_data.mdata_global);
	if (ret < 0) {
		goto exit;
	}
	/* HACK: use dst address as from */
	if (from && fromlen) {
		*fromlen = sizeof(sock->dst);
		memcpy(from, &sock->dst, *fromlen);
	}
	errno = 0;
	ret = sock_data.recv_read_len;

exit:
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s %d %d", __LINE__, __func__, errno, ret);
#endif
	k_mutex_unlock(&socket_data.mdata_global->tx_lock);
	modem_chat_attach(&socket_data.mdata_global->chat, socket_data.mdata_global->uart_pipe);
	k_work_submit(&socket_data.mdata_global->chat.receive_work);
	socket_data.expected_buf_len = 0;
	/* clear socket data */
	return ret;
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
		LOG_DBG("%d %s No data to send", __LINE__, __func__);
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
/* send binary data via the +KUDPSND commands */
static ssize_t send_socket_data(void *obj, const struct sockaddr *dst_addr, const char *buf,
				size_t buf_len, k_timeout_t timeout)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	char cmd_buf[64];
	int ret;
	int sock_written = 0;
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s", __LINE__, __func__);
#endif
	ret = validate_socket(sock);
	if (ret < 0) {
		return ret;
	}

	if (!dst_addr && sock->ip_proto == IPPROTO_UDP) {
		dst_addr = &sock->dst;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s %d", __LINE__, __func__, buf_len);
#endif
	if (buf_len > MDM_MAX_DATA_LENGTH) {
		if (sock->type == SOCK_DGRAM) {
			errno = EMSGSIZE;
			return -1;
		}
		buf_len = MDM_MAX_DATA_LENGTH;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s", __LINE__, __func__);
#endif
	if (sock->ip_proto == IPPROTO_UDP) {
		ret = prepare_udp_send_cmd(sock, dst_addr, buf_len, cmd_buf, sizeof(cmd_buf));
	} else {
		ret = prepare_tcp_send_cmd(sock, buf_len, cmd_buf, sizeof(cmd_buf));
	}
	if (ret < 0) {
		return -1;
	}

	if (k_mutex_lock(&socket_data.mdata_global->tx_lock, K_SECONDS(1)) < 0) {
		errno = -ret;
		return -1;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s", __LINE__, __func__);
#endif
	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, cmd_buf, strlen(cmd_buf),
				 connect_matches, ARRAY_SIZE(connect_matches), false);
	if (ret < 0) {
		LOG_ERR("Error sending AT command %d", ret);
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
	LOG_DBG("%d %s %d %d", __LINE__, __func__, sock_written, ret);
#endif
cleanup:
	k_mutex_unlock(&socket_data.mdata_global->tx_lock);
	return (ret < 0) ? -1 : sock_written;
}
static ssize_t offload_sendto(void *obj, const void *buf, size_t len, int flags,
			      const struct sockaddr *to, socklen_t tolen)
{
	int ret = 0;
	struct modem_socket *sock = (struct modem_socket *)obj;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s", __LINE__, __func__);
#endif
	if (!hl78xx_is_registered(socket_data.mdata_global)) {
		LOG_ERR("Modem currently not attached to the network!");
		return -EAGAIN;
	}
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
		errno = -ret;
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
	LOG_DBG("%d %s %d", __LINE__, __func__, request);
#endif
	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;
		struct k_poll_event *pev_end;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);
		LOG_DBG("poll_prepare: fd=%d, events=0x%x", pfd->fd, pfd->events);

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
		LOG_DBG("poll_update: fd=%d", pfd->fd);
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
	LOG_DBG("%d %s", __LINE__, __func__);
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
			errno = -ret;
			return -1;
		}

		sent += ret;
	}

	return sent;
}

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
	.setsockopt = NULL,
};

static int hl78xx_init_sockets(const struct device *dev)
{
	int ret;

	socket_data.buf_pool = &mdm_recv_pool;
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s", __LINE__, __func__);
#endif
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

void socknotifydata(int socket_id, int new_total)
{
	struct modem_socket *sock;
	int ret = 0;

	sock = modem_socket_from_id(&socket_data.socket_config, socket_id);
	if (!sock) {
		return;
	}
	ret = modem_socket_packet_size_update(&socket_data.socket_config, sock, new_total);
	if (ret < 0) {
		LOG_ERR("socket_id:%d left_bytes:%d err: %d", socket_id, new_total, ret);
	}
	if (new_total > 0) {
		modem_socket_data_ready(&socket_data.socket_config, sock);
	}
}

void hl78xx_socket_init(struct hl78xx_data *data)
{
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s", __LINE__, __func__);
#endif
	socket_data.mdata_global = data;
}
static void modem_net_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct hl78xx_socket_data *data = dev->data;
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s", __LINE__, __func__);
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
	       (proto == IPPROTO_TCP || proto == IPPROTO_UDP);
}

#define MODEM_HL78XX_DEFINE_INSTANCE(inst)						\
	NET_DEVICE_OFFLOAD_INIT(inst, "hl78xx_dev", NULL, NULL, &socket_data, NULL,		\
				CONFIG_MODEM_HL78XX_OFFLOAD_INIT_PRIORITY,	\
				&api_funcs, MDM_MAX_DATA_LENGTH);	\
	NET_SOCKET_OFFLOAD_REGISTER(inst, CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY, AF_UNSPEC,	\
				    offload_is_supported, offload_socket);

#define MODEM_DEVICE_SWIR_HL78XX(inst) MODEM_HL78XX_DEFINE_INSTANCE(inst)

#define DT_DRV_COMPAT swir_hl7812
DT_INST_FOREACH_STATUS_OKAY(MODEM_DEVICE_SWIR_HL78XX)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT swir_hl7800
DT_INST_FOREACH_STATUS_OKAY(MODEM_DEVICE_SWIR_HL78XX)
#undef DT_DRV_COMPAT
