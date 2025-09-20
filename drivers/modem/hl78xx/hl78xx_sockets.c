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

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS)
#include "tls_internal.h"
#include <zephyr/net/tls_credentials.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include "hl78xx.h"

LOG_MODULE_REGISTER(hl78xx_socket, CONFIG_MODEM_LOG_LEVEL);

/* Helper macros and constants */
#define MODEM_STREAM_STARTER_WORD "\r\n" CONNECT_STRING "\r\n"
#define MODEM_STREAM_END_WORD     "\r\n" OK_STRING "\r\n"

#define MODEM_SOCKET_DATA_LEFTOVER_STATE_BIT     (0)
#define HL78XX_UART_PIPE_WORK_SOCKET_BUFFER_SIZE 32

#define DNS_SERVERS_COUNT                                                                          \
	(0 + (IS_ENABLED(CONFIG_NET_IPV6) ? 1 : 0) + (IS_ENABLED(CONFIG_NET_IPV4) ? 1 : 0) +       \
	 1 /* for NULL terminator */                                                               \
	)
RING_BUF_DECLARE(mdm_recv_pool, CONFIG_MODEM_HL78XX_UART_BUFFER_SIZES);

struct hl78xx_dns_info {
#ifdef CONFIG_NET_IPV4
	char v4_string[NET_IPV4_ADDR_LEN];
	struct in_addr v4;
#endif
#ifdef CONFIG_NET_IPV6
	char v6_string[NET_IPV6_ADDR_LEN];
	struct in6_addr v6;
#endif
	bool ready;
};

/* IPv4 information is optional and only present when IPv4 is enabled */
#ifdef CONFIG_NET_IPV4
struct hl78xx_ipv4_info {
	struct in_addr addr;
	struct in_addr subnet;
	struct in_addr gateway;
	struct in_addr new_addr;
};
#endif
/* IPv6 information is optional and only present when IPv6 is enabled */
#ifdef CONFIG_NET_IPV6
struct hl78xx_ipv6_info {
	struct in6_addr addr;
	struct in6_addr subnet;
	struct in6_addr gateway;
	struct in6_addr new_addr;
};
#endif
/* TLS information is optional and only present when TLS is enabled */
struct hl78xx_tls_info {
	char hostname[MDM_MAX_HOSTNAME_LEN];
	bool hostname_set;
};

enum hl78xx_tcp_socket_status_code {
	/** Connection is up, socket can be used to send/receive data */
	TCP_SOCKET_CONNECTED = 1,
};

enum hl78xx_udp_socket_status_code {
	/** Connection is up, socket can be used to send/receive data */
	UDP_SOCKET_CONNECTED = 1,
};
struct hl78xx_tcp_status {
	enum hl78xx_tcp_socket_status_code err_code;
	bool is_connected;
};
struct hl78xx_udp_status {
	enum hl78xx_udp_socket_status_code err_code;
	bool is_connected;
};

struct receive_socket_data {
	char buf[MDM_MAX_DATA_LENGTH + ARRAY_SIZE(MODEM_STREAM_STARTER_WORD) +
		 ARRAY_SIZE(MODEM_STREAM_END_WORD)];
	uint16_t len;
};
struct hl78xx_socket_data {
	struct net_if *net_iface;
	uint8_t mac_addr[6];
	/* socket data */
	struct modem_socket_config socket_config;
	struct modem_socket sockets[MDM_MAX_SOCKETS];
	int current_sock_fd;
	int sizeof_socket_data;
	int requested_socket_id;
	bool socket_data_error;
#if defined(CONFIG_NET_IPV4) || defined(CONFIG_NET_IPV6)
	struct hl78xx_dns_info dns;
#endif
#ifdef CONFIG_NET_IPV4
	struct hl78xx_ipv4_info ipv4;
#endif
#ifdef CONFIG_NET_IPV6
	struct hl78xx_ipv6_info ipv6;
#endif
	/* rx net buffer */
	struct ring_buf *buf_pool;
	uint32_t expected_buf_len;
	uint32_t collected_buf_len;
	struct receive_socket_data receive_buf;
	/* device information */
	const struct device *modem_dev;
	const struct device *offload_dev;
	struct hl78xx_data *mdata_global;
	/* socket state */
	struct hl78xx_tls_info tls;
	struct hl78xx_tcp_status tcp_conn_status;
	struct hl78xx_udp_status udp_conn_status;
};
/* work buffer for socket data */
struct work_socket_data {
	char buf[HL78XX_UART_PIPE_WORK_SOCKET_BUFFER_SIZE];
	uint16_t len;
};
/* work buffer for socket data */
struct work_socket_data work_buf;
/* socket data matching flags */
bool match_connect_found;
bool match_eof_ok_found;
bool match_found;
bool socket_data_received;
uint16_t start_index_eof;
uint16_t size_of_socketdata;
atomic_t state_leftover;
/**
 * have to keep this global pointer only for
 * static int offload_socket(int family, int type, int proto)
 */
static struct hl78xx_socket_data *socket_data_global;

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS)
static int map_credentials(struct hl78xx_socket_data *socket_data_lcl, const void *optval,
			   socklen_t optlen);
static int hl78xx_configure_chipper_suit(struct hl78xx_socket_data *socket_data_lcl);
#endif /* CONFIG_NET_SOCKETS_SOCKOPT_TLS */

/* Helper: retrieve parent hl78xx_socket_data from a modem_socket pointer.
 * Use sockets[0] as the member expression so the macro does proper type
 * checking for array members. This centralizes the container_of usage and
 * provides a single place to add assertions or alternate lookup logic later.
 */
static inline struct hl78xx_socket_data *hl78xx_socket_data_from_sock(struct modem_socket *sock)
{
	/* Robustly recover the parent `hl78xx_socket_data` for any element
	 * address within the `sockets[]` array. Using CONTAINER_OF with
	 * `sockets[0]` is not safe when `sock` points to `sockets[i]` (i>0),
	 * because CONTAINER_OF assumes the pointer is to the member named
	 * in the macro (sockets[0]). That yields a pointer offset by
	 * i * sizeof(sockets[0]).
	 *
	 * Strategy: for each possible index i, compute the candidate parent
	 * base address so that &candidate->sockets[i] == sock. If the math
	 * yields a candidate that looks like a valid container, return it.
	 */
	if (!sock) {
		return NULL;
	}

	const size_t elem_size = sizeof(((struct hl78xx_socket_data *)0)->sockets[0]);
	const size_t sockets_off = offsetof(struct hl78xx_socket_data, sockets);
	struct hl78xx_socket_data *result = NULL;

	for (int i = 0; i < MDM_MAX_SOCKETS; i++) {
		struct hl78xx_socket_data *candidate =
			(struct hl78xx_socket_data *)((char *)sock -
						      (ptrdiff_t)(sockets_off +
								  (size_t)i * elem_size));
		/* Quick sanity: does candidate->sockets[i] point back to sock? */
		if ((struct modem_socket *)&candidate->sockets[i] != sock) {
			continue;
		}
		/* Prefer candidates that look like a real initialized parent: the
		 * instance should have non-NULL pointers to the offload device and
		 * modem data. If we find such a candidate, return it immediately.
		 * Otherwise keep the first matching candidate and return it as a
		 * fallback at the end. This avoids returning a shifted base when the
		 * shifted candidate's fields are clearly uninitialized.
		 */
		if (candidate->offload_dev && candidate->mdata_global) {
			return candidate;
		}

		/* Remember the first match as a fallback */
		if (!result) {
			result = candidate;
		}
	}
	return result;
}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
/**
 * @brief Handle modem state update from +KSTATE URC of RAT Scan Finish.
 * This command is intended to report events for different important state transitions and system
 * occurrences.
 * Actually this eventc'state is really important functionality to understand networks
 * searching phase of the modem.
 * Verbose debug logging for KSTATEV events
 */
void hl78xx_on_kstatev_parser(struct hl78xx_data *data, int state, int rat_mode)
{
	switch (state) {
	case EVENT_START_SCAN:
		break;
	case EVENT_FAIL_SCAN:
		LOG_DBG("Modem failed to find a suitable network");
		break;
	case EVENT_ENTER_CAMPED:
		LOG_DBG("Modem entered camped state on a suitable or acceptable cell");
		break;
	case EVENT_CONNECTION_ESTABLISHMENT:
		LOG_DBG("Modem successfully established a connection to the network");
		break;
	case EVENT_START_RESCAN:
		LOG_DBG("Modem is starting a rescan for available networks");
		break;
	case EVENT_RRC_CONNECTED:
		LOG_DBG("Modem has established an RRC connection with the network");
		break;
	case EVENT_NO_SUITABLE_CELLS:
		LOG_DBG("Modem did not find any suitable cells during the scan");
		break;
	case EVENT_ALL_REGISTRATION_FAILED:
		LOG_DBG("Modem failed to register to any network");
		break;
	default:
		LOG_DBG("Unhandled KSTATEV for state %d", state);
		break;
	}
}
#endif

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

static bool update_dns(struct hl78xx_socket_data *socket_data_lcl, bool is_ipv4,
		       const char *dns_str)
{
	int ret;

	LOG_DBG("Updating DNS (%s): %s", is_ipv4 ? "IPv4" : "IPv6", dns_str);
#ifdef CONFIG_NET_IPV4
	if (is_ipv4) {
		ret = strncmp(dns_str, socket_data_lcl->dns.v4_string, strlen(dns_str));
		if (ret != 0) {
			LOG_DBG("New IPv4 DNS differs from current, marking dns_ready = false");
			socket_data_lcl->dns.ready = false;
		}
		strncpy(socket_data_lcl->dns.v4_string, dns_str,
			sizeof(socket_data_lcl->dns.v4_string));
		socket_data_lcl->dns.v4_string[sizeof(socket_data_lcl->dns.v4_string) - 1] = '\0';
		return parse_ip(true, socket_data_lcl->dns.v4_string, &socket_data_lcl->dns.v4);
	}
#else
	if (is_ipv4) {
		LOG_DBG("IPv4 DNS reported but IPv4 disabled in build; ignoring");
		return false;
	}
#endif /* CONFIG_NET_IPV4 */
#ifdef CONFIG_NET_IPV6
	else {
		ret = strncmp(dns_str, socket_data_lcl->dns.v6_string, strlen(dns_str));
		if (ret != 0) {
			LOG_DBG("New IPv6 DNS differs from current, marking dns_ready = false");
			socket_data_lcl->dns.ready = false;
		}
		strncpy(socket_data_lcl->dns.v6_string, dns_str,
			sizeof(socket_data_lcl->dns.v6_string));
		socket_data_lcl->dns.v6_string[sizeof(socket_data_lcl->dns.v6_string) - 1] = '\0';

		if (!parse_ip(false, socket_data_lcl->dns.v6_string, &socket_data_lcl->dns.v6)) {
			return false;
		}

		net_addr_ntop(AF_INET6, &socket_data_lcl->dns.v6, socket_data_lcl->dns.v6_string,
			      sizeof(socket_data_lcl->dns.v6_string));
		LOG_DBG("Parsed IPv6 DNS: %s", socket_data_lcl->dns.v6_string);
	}
#endif /* CONFIG_NET_IPV6 */
	return true;
}

bool is_valid_ipv4_addr(struct in_addr *addr)
{
	return addr && (addr->s_addr != 0);
}

static void set_iface(struct hl78xx_socket_data *socket_data_lcl, bool is_ipv4)
{
	if (!socket_data_lcl->net_iface) {
		LOG_DBG("No network interface set. Skipping iface config.");
		return;
	}

	LOG_DBG("Setting %s interface address...", is_ipv4 ? "IPv4" : "IPv6");

	if (is_ipv4) {
#ifdef CONFIG_NET_IPV4
		if (is_valid_ipv4_addr(&socket_data_lcl->ipv4.addr)) {
			net_if_ipv4_addr_rm(socket_data_lcl->net_iface,
					    &socket_data_lcl->ipv4.addr);
		}

		if (!net_if_ipv4_addr_add(socket_data_lcl->net_iface,
					  &socket_data_lcl->ipv4.new_addr, NET_ADDR_DHCP, 0)) {
			LOG_ERR("Failed to set IPv4 interface address.");
		}

		net_if_ipv4_set_netmask_by_addr(socket_data_lcl->net_iface,
						&socket_data_lcl->ipv4.new_addr,
						&socket_data_lcl->ipv4.subnet);
		net_if_ipv4_set_gw(socket_data_lcl->net_iface, &socket_data_lcl->ipv4.gateway);

		net_ipaddr_copy(&socket_data_lcl->ipv4.addr, &socket_data_lcl->ipv4.new_addr);
		LOG_DBG("IPv4 interface configuration complete.");
#else
		LOG_DBG("IPv4 disabled: skipping IPv4 interface configuration");
#endif
	}
#ifdef CONFIG_NET_IPV6
	else {
		net_if_ipv6_addr_rm(socket_data_lcl->net_iface, &socket_data_lcl->ipv6.addr);

		if (!net_if_ipv6_addr_add(socket_data_lcl->net_iface,
					  &socket_data_lcl->ipv6.new_addr, NET_ADDR_AUTOCONF, 0)) {
			LOG_ERR("Failed to set IPv6 interface address.");
		} else {
			LOG_DBG("IPv6 interface configuration complete.");
		}
	}
#endif
}

static bool split_ipv4_and_subnet(const char *combined, char *ip_out, size_t ip_out_len,
				  char *subnet_out, size_t subnet_out_len)
{
	int dot_count = 0;
	const char *ptr = combined;
	const char *split = NULL;
	size_t ip_len = 0;

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

	ip_len = split - combined;
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
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_socket_data *socket_data_lcl =
		(struct hl78xx_socket_data *)data->offload_dev->data;
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
	sock = modem_socket_from_id(&socket_data_lcl->socket_config, socket_id);
	tcp_conn_stat = ATOI(argv[2], -1, "tcp_status");
	if (tcp_conn_stat == TCP_SOCKET_CONNECTED) {
		socket_data_lcl->tcp_conn_status.err_code = tcp_conn_stat;
		socket_data_lcl->tcp_conn_status.is_connected = true;
		return;
	}
exit:
	socket_data_lcl->tcp_conn_status.err_code = tcp_conn_stat;
	socket_data_lcl->tcp_conn_status.is_connected = false;
	if (socket_id != -1) {
		modem_socket_put(&socket_data_lcl->socket_config, sock->sock_fd);
	}
}

static void hl78xx_on_kudpind(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_socket_data *socket_data_lcl =
		(struct hl78xx_socket_data *)data->offload_dev->data;
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
	sock = modem_socket_from_newid(&socket_data_lcl->socket_config);
	if (sock) {
		sock->is_connected = true;
		socket_id = ATOI(argv[1], -1, "socket_id");
		/* on error give up modem socket */
		if (modem_socket_id_assign(&socket_data_lcl->socket_config, sock, socket_id) < 0) {
			modem_socket_put(&socket_data_lcl->socket_config, sock->sock_fd);
			sock->is_connected = false;
		}
	}
	udp_conn_stat = ATOI(argv[2], -1, "udp_status");
	if (udp_conn_stat == UDP_SOCKET_CONNECTED) {
		socket_data_lcl->udp_conn_status.err_code = udp_conn_stat;
		socket_data_lcl->udp_conn_status.is_connected = true;
		return;
	}
exit:
	socket_data_lcl->udp_conn_status.err_code = udp_conn_stat;
	socket_data_lcl->udp_conn_status.is_connected = false;
	if (socket_id != -1 && sock) {
		modem_socket_put(&socket_data_lcl->socket_config, sock->sock_fd);
	}
}

/* Handler: +KTCP/KUDP/KMQTT: <socket_id>[0] */
static void hl78xx_on_cmd_kxxxsockcreate(struct modem_chat *chat, char **argv, uint16_t argc,
					 void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_socket_data *socket_data_lcl =
		(struct hl78xx_socket_data *)data->offload_dev->data;
	struct modem_socket *sock = NULL;
	int socket_id;

	HL78XX_LOG_DBG("%d", __LINE__);
	if (argc < 2 || !argv[1]) {
		LOG_ERR("%s: Incomplete response", __func__);
		return;
	}
	/* look up new socket by special id */
	sock = modem_socket_from_newid(&socket_data_lcl->socket_config);
	if (sock) {
		sock->is_connected = true;
		socket_id = ATOI(argv[1], -1, "socket_id");
		/* on error give up modem socket */
		if (modem_socket_id_assign(&socket_data_lcl->socket_config, sock, socket_id) < 0) {
			modem_socket_put(&socket_data_lcl->socket_config, sock->sock_fd);
			sock->is_connected = false;
		}
	}
	/* don't give back semaphore -- OK to follow */
}

/* Handler: +CGCONTRDP: <cid>,<bearer>,<apn>,<addr>,<dcomp>,<hcomp>,<dns1>[,<dns2>] */
static void hl78xx_on_cgdcontrdp(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_socket_data *socket_data_lcl =
		(struct hl78xx_socket_data *)data->offload_dev->data;

	if (argc < 8 || !argv[4] || !argv[5] || !argv[6]) {
		LOG_ERR("Incomplete CGCONTRDP response: argc = %d", argc);
		return;
	}

	LOG_INF("Apn=%s", argv[3]);
	LOG_INF("Addr=%s", argv[4]);
	LOG_INF("Gw=%s", argv[5]);
	LOG_INF("DNS=%s", argv[6]);

	bool is_ipv4 = (strchr(argv[4], ':') == NULL);
	char ip_addr[NET_IPV6_ADDR_LEN];
	char subnet_mask[NET_IPV6_ADDR_LEN];
#ifdef CONFIG_MODEM_HL78XX_APN_SOURCE_NETWORK
	bool is_apn_exists = (argv[3] && strlen(argv[3]) > 0);

	if (is_apn_exists) {
		hl78xx_extract_essential_part_apn(
			argv[3], socket_data_lcl->mdata_global->identity.apn,
			sizeof(socket_data_lcl->mdata_global->identity.apn));
	}
#endif
	if (!split_ipv4_and_subnet(argv[4], ip_addr, sizeof(ip_addr), subnet_mask,
				   sizeof(subnet_mask))) {
		return;
	}
	LOG_INF("Extracted IP: %s", ip_addr);
	LOG_INF("Extracted Subnet: %s", subnet_mask);
#ifdef CONFIG_NET_IPV4
	if (is_ipv4) {
		if (!parse_ip(true, ip_addr, &socket_data_lcl->ipv4.new_addr)) {
			return;
		}
		if (!parse_ip(true, subnet_mask, &socket_data_lcl->ipv4.subnet)) {
			return;
		}
		if (!parse_ip(true, argv[5], &socket_data_lcl->ipv4.gateway)) {
			return;
		}
	}
#else
	if (is_ipv4) {
		LOG_DBG("IPv4 configuration present but IPv4 disabled in build; skipping.");
		return;
	}
#endif
#ifdef CONFIG_NET_IPV6
	else if (!parse_ip(false, ip_addr, &socket_data_lcl->ipv6.new_addr)) {
		return;
	}
#endif
	else {
		/* Neither IPv4 nor IPv6 handled */
		LOG_WRN("Unsupported address family in CGCONTRDP response");
		return;
	}
	if (!update_dns(socket_data_lcl, is_ipv4, argv[6])) {
		return;
	}
	set_iface(socket_data_lcl, is_ipv4);
}

static void hl78xx_on_ktcpstate(struct modem_chat *chat, char **argv, uint16_t argc,
				void *user_data)
{
	uint8_t tcp_session_id = 0;
	uint8_t tcp_status = 0;
	int8_t tcp_notif = 0;
	uint16_t rcv_data = 0;

	if (argc < 4) {
		return;
	}
	tcp_session_id = ATOI(argv[1], 0, "tcp_session_id");
	tcp_status = ATOI(argv[2], 0, "tcp_status");
	tcp_notif = ATOI(argv[3], 0, "tcp_notif");
	rcv_data = ATOI(argv[5], 0, "tcp_rcv_data");
	if (tcp_status != 3 && tcp_notif != -1) {
		return;
	}
	socket_notify_data(tcp_session_id, rcv_data, user_data);
	HL78XX_LOG_DBG("%d %s %s %s %s %s", __LINE__, argv[1], argv[2], argv[3], argv[4], argv[5]);
}

static void hl78xx_on_cme_error(struct modem_chat *chat, char **argv, uint16_t argc,
				void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_socket_data *socket_data_lcl =
		(struct hl78xx_socket_data *)data->offload_dev->data;
	int error_code;

	if (argc < 2 || !argv[1]) {
		LOG_ERR("%d %s CME ERROR: Incomplete response", __LINE__, __func__);
		return;
	}
	error_code = ATOI(argv[1], -1, "CME error code");
	if (error_code < 0) {
		LOG_ERR("Invalid CME error code: %d", error_code);
		return;
	}
	socket_data_lcl->socket_data_error = true;
	LOG_ERR("%d %s CME ERROR: %d", __LINE__, __func__, error_code);
}

/* Define modem chat matches and scripts */
MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", NULL);
MODEM_CHAT_MATCHES_DEFINE(connect_matches, MODEM_CHAT_MATCH(CONNECT_STRING, "", NULL),
			  MODEM_CHAT_MATCH(CME_ERROR_STRING, "", hl78xx_on_cme_error));
MODEM_CHAT_MATCHES_DEFINE(allow_matches, MODEM_CHAT_MATCH("OK", "", NULL),
			  MODEM_CHAT_MATCH(CME_ERROR_STRING, "", NULL));
MODEM_CHAT_MATCH_DEFINE(kudpind_match, "+KUDP_IND: ", ",", hl78xx_on_kudpind);
MODEM_CHAT_MATCH_DEFINE(ktcpind_match, "+KTCP_IND: ", ",", hl78xx_on_ktcpind);
MODEM_CHAT_MATCH_DEFINE(ktcpcfg_match, "+KTCPCFG: ", "", hl78xx_on_cmd_kxxxsockcreate);
MODEM_CHAT_MATCH_DEFINE(cgdcontrdp_match, "+CGCONTRDP: ", ",", hl78xx_on_cgdcontrdp);
MODEM_CHAT_MATCH_DEFINE(ktcp_state_match, "+KTCPSTAT: ", ",", hl78xx_on_ktcpstate);

static void parser_reset(struct hl78xx_socket_data *socket_data_lcl)
{
	memset(&socket_data_lcl->receive_buf, 0, sizeof(socket_data_lcl->receive_buf));
	match_found = false;
}

static void found_reset(void)
{
	match_connect_found = false;
	match_eof_ok_found = false;
	socket_data_received = false;
}

static bool modem_chat_parse_end_del_start(struct hl78xx_socket_data *socket_data_lcl,
					   struct modem_chat *chat)
{
	uint8_t i = 0;

	if (socket_data_lcl->receive_buf.len == 0) {
		return false;
	}
	for (i = 0; i < chat->delimiter_size; i++) {
		if (socket_data_lcl->receive_buf.buf[socket_data_lcl->receive_buf.len - 1] ==
		    chat->delimiter[i]) {
			return true;
		}
	}
	return false;
}

static bool modem_chat_parse_end_del_complete(struct hl78xx_socket_data *socket_data_lcl,
					      struct modem_chat *chat)
{
	if (socket_data_lcl->receive_buf.len < chat->delimiter_size) {
		return false;
	}

	return memcmp(&socket_data_lcl->receive_buf
			       .buf[socket_data_lcl->receive_buf.len - chat->delimiter_size],
		      chat->delimiter, chat->delimiter_size) == 0;
}

static bool modem_chat_match_matches_received(struct hl78xx_socket_data *socket_data_lcl,
					      const char *match, uint16_t match_size)
{
	uint16_t i = 0;
	/* match size must be less than or equal to received data size */
	if (socket_data_lcl->receive_buf.len < match_size) {
		return false;
	}
	for (i = 0; i < match_size; i++) {
		if (match[i] != socket_data_lcl->receive_buf.buf[i]) {
			return false;
		}
	}
	return true;
}

static bool is_receive_buffer_full(struct hl78xx_socket_data *socket_data_lcl)
{
	return socket_data_lcl->receive_buf.len >= ARRAY_SIZE(socket_data_lcl->receive_buf.buf);
}

static void handle_expected_length_decrement(struct hl78xx_socket_data *socket_data_lcl)
{
	/* Decrement expected length if CONNECT matched and expected length > 0 */
	if (match_connect_found && socket_data_lcl->expected_buf_len > 0) {
		socket_data_lcl->expected_buf_len--;
	}
}

static bool is_end_delimiter_only(struct hl78xx_socket_data *socket_data_lcl)
{
	return socket_data_lcl->receive_buf.len ==
	       socket_data_lcl->mdata_global->chat.delimiter_size;
}

static bool is_valid_eof_index(struct hl78xx_socket_data *socket_data_lcl, uint8_t size_match)
{
	start_index_eof = socket_data_lcl->receive_buf.len - size_match - 2;
	return start_index_eof < ARRAY_SIZE(socket_data_lcl->receive_buf.buf);
}

static void try_handle_eof_pattern(struct hl78xx_socket_data *socket_data_lcl)
{
	uint8_t size_match = strlen(EOF_PATTERN);

	if (socket_data_lcl->receive_buf.len < size_match + 2) {
		return;
	}
	if (!is_valid_eof_index(socket_data_lcl, size_match)) {
		return;
	}
	if (strncmp(&socket_data_lcl->receive_buf.buf[start_index_eof], EOF_PATTERN, size_match) ==
	    0) {
		int ret = ring_buf_put(socket_data_lcl->buf_pool, socket_data_lcl->receive_buf.buf,
				       start_index_eof);

		if (ret <= 0) {
			LOG_ERR("ring_buf_put failed: %d", ret);
		}
		socket_data_received = true;
		socket_data_lcl->collected_buf_len += ret;
	}
}

static bool is_cme_error_match(struct hl78xx_socket_data *socket_data_lcl)
{
	uint8_t size_match = strlen(CME_ERROR_STRING);

	return socket_data_lcl->receive_buf.len >= size_match &&
	       modem_chat_match_matches_received(socket_data_lcl, CME_ERROR_STRING, size_match);
}

static bool is_connect_match(struct hl78xx_socket_data *socket_data_lcl)
{
	uint8_t size_match = strlen(CONNECT_STRING);

	return socket_data_lcl->receive_buf.len == size_match &&
	       modem_chat_match_matches_received(socket_data_lcl, CONNECT_STRING, size_match);
}

static bool is_ok_match(struct hl78xx_socket_data *socket_data_lcl)
{
	uint8_t size_match = strlen(OK_STRING);

	return socket_data_lcl->receive_buf.len == size_match &&
	       modem_chat_match_matches_received(socket_data_lcl, OK_STRING, size_match);
}

static void socket_process_bytes(struct hl78xx_socket_data *socket_data_lcl, char byte)
{
	if (is_receive_buffer_full(socket_data_lcl)) {
		LOG_WRN("Receive buffer overrun");
		parser_reset(socket_data_lcl);
		return;
	}
	socket_data_lcl->receive_buf.buf[socket_data_lcl->receive_buf.len++] = byte;
	handle_expected_length_decrement(socket_data_lcl);
	if (modem_chat_parse_end_del_complete(socket_data_lcl,
					      &socket_data_lcl->mdata_global->chat)) {
		if (is_end_delimiter_only(socket_data_lcl)) {
			parser_reset(socket_data_lcl);
			return;
		}
		size_of_socketdata = socket_data_lcl->receive_buf.len;
		if (match_connect_found && !match_eof_ok_found) {
			try_handle_eof_pattern(socket_data_lcl);
		}
		parser_reset(socket_data_lcl);
		return;
	}
	if (modem_chat_parse_end_del_start(socket_data_lcl, &socket_data_lcl->mdata_global->chat)) {
		return;
	}
	if (!match_found && !match_connect_found) {
		if (is_connect_match(socket_data_lcl)) {
			match_connect_found = true;
			LOG_DBG("CONNECT matched. Expecting %d more bytes.",
				socket_data_lcl->expected_buf_len);
			return;
		} else if (is_cme_error_match(socket_data_lcl)) {
			match_found = true; /* mark this to prevent further parsing */
			LOG_ERR("CME ERROR received. Connection failed.");
			socket_data_lcl->expected_buf_len = 0;
			socket_data_lcl->collected_buf_len = 0;
			parser_reset(socket_data_lcl);
			socket_data_lcl->socket_data_error = true;
			/*  Optionally notify script or raise an event here */
			k_sem_give(&socket_data_lcl->mdata_global->script_stopped_sem_rx_int);
			return;
		}
	}
	if (match_connect_found && !match_eof_ok_found && is_ok_match(socket_data_lcl)) {
		match_eof_ok_found = true;
		LOG_DBG("OK matched.");
	}
}

static int modem_process_handler(struct hl78xx_data *data)
{
	struct hl78xx_socket_data *socket_data_lcl =
		(struct hl78xx_socket_data *)data->offload_dev->data;
	int recv_len = 0;
	/* If no more data is expected, set leftover state and return */
	if (socket_data_lcl->expected_buf_len == 0) {
		LOG_DBG("No more data expected");
		atomic_set_bit(&state_leftover, MODEM_SOCKET_DATA_LEFTOVER_STATE_BIT);
		return 0;
	}
	recv_len = modem_pipe_receive(socket_data_lcl->mdata_global->uart_pipe, work_buf.buf,
				      MIN(sizeof(work_buf.buf), socket_data_lcl->expected_buf_len));
	if (recv_len <= 0) {
		LOG_WRN("modem_pipe_receive returned %d", recv_len);
		return recv_len;
	}
	work_buf.len = recv_len;
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_HEXDUMP_DBG(work_buf.buf, recv_len, "Received bytes:");
#endif /* CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG */
	for (int i = 0; i < recv_len; i++) {
		socket_process_bytes(socket_data_lcl, work_buf.buf[i]);
	}
	if (match_eof_ok_found && socket_data_received) {
		LOG_DBG("All data received: %d bytes", size_of_socketdata);
		socket_data_lcl->expected_buf_len = 0;
		found_reset();
		k_sem_give(&socket_data_lcl->mdata_global->script_stopped_sem_rx_int);
	}
	return 0;
}

static void modem_pipe_callback(struct modem_pipe *pipe, enum modem_pipe_event event,
				void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	HL78XX_LOG_DBG("%d Pipe event received: %d", __LINE__, event);
	switch (event) {
	case MODEM_PIPE_EVENT_RECEIVE_READY:
		modem_process_handler(data);
		break;

	case MODEM_PIPE_EVENT_TRANSMIT_IDLE:
		k_sem_give(&data->script_stopped_sem_tx_int);
		break;

	default:
		LOG_DBG("Unhandled event: %d", event);
		break;
	}
}

void notif_carrier_off(const struct device *dev)
{
	struct hl78xx_data *data = dev->data;
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;

	net_if_carrier_off(socket_data->net_iface);
}

void notif_carrier_on(const struct device *dev)
{
	struct hl78xx_data *data = dev->data;
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;

	net_if_carrier_on(socket_data->net_iface);
}

void iface_status_work_cb(struct hl78xx_data *data, modem_chat_script_callback script_user_callback)
{

	const char *cmd = "AT+CGCONTRDP=1";
	int ret = 0;

	ret = modem_dynamic_cmd_send(data, script_user_callback, cmd, strlen(cmd),
				     &cgdcontrdp_match, 1, false);
	if (ret < 0) {
		LOG_ERR("Failed to send AT+CGCONTRDP command: %d", ret);
		return;
	}
}

void dns_work_cb(const struct device *dev, bool hard_reset)
{
#if defined(CONFIG_DNS_RESOLVER) && !defined(CONFIG_DNS_SERVER_IP_ADDRESSES)
	int ret;
	struct hl78xx_data *data = dev->data;
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;
	struct dns_resolve_context *dnsCtx;
	struct sockaddr temp_addr;
	bool valid_address = false;
	bool retry = false;
	const char *const dns_servers_str[DNS_SERVERS_COUNT] = {
#ifdef CONFIG_NET_IPV6
		socket_data->dns.v6_string,
#endif
#ifdef CONFIG_NET_IPV4
		socket_data->dns.v4_string,
#endif
		NULL};
	const char *dns_servers_wrapped[ARRAY_SIZE(dns_servers_str)];

	if (hard_reset) {
		LOG_DBG("Resetting DNS resolver");
		dnsCtx = dns_resolve_get_default();
		if (dnsCtx->state != DNS_RESOLVE_CONTEXT_INACTIVE) {
			dns_resolve_close(dnsCtx);
		}
		socket_data->dns.ready = false;
	}

#ifdef CONFIG_NET_IPV6
	valid_address = net_ipaddr_parse(socket_data->dns.v6_string,
					 strlen(socket_data->dns.v6_string), &temp_addr);
	if (!valid_address && IS_ENABLED(CONFIG_NET_IPV4)) {
		/* IPv6 DNS string is not valid, replace it with IPv4 address and recheck */
#ifdef CONFIG_NET_IPV4
		strncpy(socket_data->dns.v6_string, socket_data->dns.v4_string,
			sizeof(socket_data->dns.v4_string) - 1);
		valid_address = net_ipaddr_parse(socket_data->dns.v6_string,
						 strlen(socket_data->dns.v6_string), &temp_addr);
#endif
	}
#elif defined(CONFIG_NET_IPV4)
	valid_address = net_ipaddr_parse(socket_data->dns.v4_string,
					 strlen(socket_data->dns.v4_string), &temp_addr);
#else
	/* No IP stack configured */
	valid_address = false;
#endif
	if (!valid_address) {
		LOG_WRN("No valid DNS address!");
		return;
	}
	if (!socket_data->net_iface || !net_if_is_up(socket_data->net_iface) ||
	    socket_data->dns.ready) {
		LOG_DBG("DNS already ready or net_iface problem %d %d %d", !socket_data->net_iface,
			!net_if_is_up(socket_data->net_iface), socket_data->dns.ready);
		return;
	}
	memcpy(dns_servers_wrapped, dns_servers_str, sizeof(dns_servers_wrapped));
	/* set new DNS addr in DNS resolver */
	LOG_DBG("Refresh DNS resolver");
	dnsCtx = dns_resolve_get_default();
	ret = dns_resolve_reconfigure(dnsCtx, dns_servers_wrapped, NULL, DNS_SOURCE_MANUAL);
	if (ret < 0) {
		LOG_ERR("dns_resolve_reconfigure fail (%d)", ret);
		retry = true;
	} else {
		LOG_DBG("DNS ready");
		socket_data->dns.ready = true;
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
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_socket_data *socket_data_lcl =
		(struct hl78xx_socket_data *)data->offload_dev->data;
	int ret = 0;

	sock = modem_socket_from_fd(&socket_data_lcl->socket_config, socket_id);
	if (!sock) {
		LOG_ERR("Socket not found! (%d)", socket_id);
		return -EINVAL;
	}
	sock_data = sock->data;
	if (!sock_data) {
		LOG_ERR("Socket data missing! Ignoring (%d)", socket_id);
		return -EINVAL;
	}
	if (socket_data_lcl->socket_data_error && socket_data_lcl->collected_buf_len == 0) {
		errno = ECONNABORTED;
		return -ECONNABORTED;
	}
	if ((len <= 0) || socket_data_length <= 0 ||
	    socket_data_lcl->collected_buf_len < (size_t)len) {
		LOG_ERR("%d Invalid data length: %d %d %d Aborting!", __LINE__, socket_data_length,
			(int)len, socket_data_lcl->collected_buf_len);
		return -EAGAIN;
	}
	if (len < socket_data_length) {
		LOG_DBG("Incomplete data received! Expected: %d, Received: %d", socket_data_length,
			len);
		return -EAGAIN;
	}
	ret = ring_buf_get(socket_data_lcl->buf_pool, sock_data->recv_buf, len);
	if (ret != len) {
		LOG_ERR("%d Data retrieval mismatch: expected %u, got %d", __LINE__, len, ret);
		return -EAGAIN;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_HEXDUMP_DBG(sock_data->recv_buf, ret, "Received Data:");
#endif /* CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG */
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
	modem_socket_next_packet_size(&socket_data_lcl->socket_config, sock);
	modem_socket_packet_size_update(&socket_data_lcl->socket_config, sock, -socket_data_length);
	socket_data_lcl->collected_buf_len = 0;
	return len;
}

int modem_handle_data_capture(size_t target_len, struct hl78xx_data *data)
{
	struct hl78xx_socket_data *socket_data_lcl =
		(struct hl78xx_socket_data *)data->offload_dev->data;

	return on_cmd_sockread_common(socket_data_lcl->current_sock_fd,
				      socket_data_lcl->sizeof_socket_data, target_len, data);
}

/*
 * generic socket creation function
 * which can be called in bind() or connect()
 */
static int extract_ip_family_and_port(const struct sockaddr *addr, int *af, uint16_t *port)
{
#if defined(CONFIG_NET_IPV6)
	if (addr->sa_family == AF_INET6) {
		*port = ntohs(net_sin6(addr)->sin6_port);
		*af = MDM_HL78XX_SOCKET_AF_IPV6;
	} else {
#endif /* CONFIG_NET_IPV6 */
#if defined(CONFIG_NET_IPV4)
		if (addr->sa_family == AF_INET) {
			*port = ntohs(net_sin(addr)->sin_port);
			*af = MDM_HL78XX_SOCKET_AF_IPV4;
		} else {
#endif /* CONFIG_NET_IPV4 */
			errno = EAFNOSUPPORT;
			return -1;
#if defined(CONFIG_NET_IPV4)
		}
#endif /* CONFIG_NET_IPV4 */
#if defined(CONFIG_NET_IPV6)
	}
#endif /* CONFIG_NET_IPV6 */
	return 0;
}

static int format_ip_and_setup_tls(struct hl78xx_socket_data *socket_data_lcl,
				   const struct sockaddr *addr, char *ip_str, size_t ip_str_len,
				   struct modem_socket *sock)
{
	int ret = modem_context_sprint_ip_addr(addr, ip_str, ip_str_len);

	if (ret != 0) {
		LOG_ERR("Failed to format IP!");
		errno = ENOMEM;
		return -1;
	}
	if (sock->ip_proto == IPPROTO_TCP) {
		/* Determine actual length of the formatted IP string (it may be
		 * shorter than the provided buffer size). Copy at most
		 * MDM_MAX_HOSTNAME_LEN - 1 bytes and ensure NUL-termination to
		 * avoid writing past the hostname buffer.
		 */
		size_t actual_len = strnlen(ip_str, ip_str_len);
		size_t copy_len = MIN(actual_len, (size_t)MDM_MAX_HOSTNAME_LEN - 1);

		if (copy_len > 0) {
			memcpy(socket_data_lcl->tls.hostname, ip_str, copy_len);
		}
		socket_data_lcl->tls.hostname[copy_len] = '\0';
		socket_data_lcl->tls.hostname_set = false;
	}
	return 0;
}

static int send_tcp_or_tls_config(struct modem_socket *sock, uint16_t dst_port, int af, int mode,
				  struct hl78xx_socket_data *socket_data_lcl)
{
	int ret = 0;
	char cmd_buf[sizeof("AT+KTCPCFG=#,#,\"" MODEM_HL78XX_ADDRESS_FAMILY_FORMAT
			    "\",#####,,,,#,,#") +
		     MDM_MAX_HOSTNAME_LEN + NET_IPV6_ADDR_LEN];

	snprintk(cmd_buf, sizeof(cmd_buf), "AT+KTCPCFG=1,%d,\"%s\",%u,,,,%d,%s,0", mode,
		 socket_data_lcl->tls.hostname, dst_port, af, mode == 3 ? "0" : "");
	ret = modem_dynamic_cmd_send(socket_data_lcl->mdata_global, NULL, cmd_buf, strlen(cmd_buf),
				     &ktcpcfg_match, 1, false);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", cmd_buf, ret);
		modem_socket_put(&socket_data_lcl->socket_config, sock->sock_fd);
		errno = ret;
		return -1;
	}
	return 0;
}

static int send_udp_config(const struct sockaddr *addr, struct hl78xx_socket_data *socket_data_lcl,
			   struct modem_socket *sock)
{
	int ret = 0;
	char cmd_buf[64];
	uint8_t display_data_urc = 2;

#if defined(CONFIG_MODEM_HL78XX_SOCKET_UDP_DISPLAY_DATA_URC)
	display_data_urc = CONFIG_MODEM_HL78XX_SOCKET_UDP_DISPLAY_DATA_URC;
#endif
	snprintk(cmd_buf, sizeof(cmd_buf), "AT+KUDPCFG=1,%u,,%d,,,%d,%d", 0, display_data_urc,
		 (addr->sa_family - 1), 0);

	ret = modem_dynamic_cmd_send(socket_data_lcl->mdata_global, NULL, cmd_buf, strlen(cmd_buf),
				     &kudpind_match, 1, false);
	if (ret < 0) {
		goto error;
	}
	if (!socket_data_lcl->udp_conn_status.is_connected) {
		ret = socket_data_lcl->udp_conn_status.err_code;
		goto error;
	}
	return 0;
error:
	LOG_ERR("%s ret:%d", cmd_buf, ret);
	modem_socket_put(&socket_data_lcl->socket_config, sock->sock_fd);
	errno = ret;
	return -1;
}

static int create_socket(struct modem_socket *sock, const struct sockaddr *addr,
			 struct hl78xx_socket_data *data)
{
	int af;
	uint16_t dst_port;
	char ip_str[NET_IPV6_ADDR_LEN];
	bool is_udp;
	int mode;
	/* save destination address */
	memcpy(&sock->dst, addr, sizeof(*addr));
	if (extract_ip_family_and_port(addr, &af, &dst_port) < 0) {
		return -1;
	}
	if (format_ip_and_setup_tls(data, addr, ip_str, sizeof(ip_str), sock) < 0) {
		return -1;
	}
	is_udp = (sock->ip_proto == IPPROTO_UDP);
	if (is_udp) {
		return send_udp_config(addr, data, sock);
	}
	mode = (sock->ip_proto == IPPROTO_TLS_1_2) ? 3 : 0;
	/* only TCP and TLS are supported */
	if (sock->ip_proto != IPPROTO_TCP && sock->ip_proto != IPPROTO_TLS_1_2) {
		LOG_ERR("Unsupported protocol: %d", sock->ip_proto);
		errno = EPROTONOSUPPORT;
		return -1;
	}
	return send_tcp_or_tls_config(sock, dst_port, af, mode, data);
}

static int socket_close(struct hl78xx_socket_data *socket_data_lcl, struct modem_socket *sock)
{
	char buf[sizeof("AT+KTCPCLOSE=##\r")];
	int ret = 0;

	if (sock->ip_proto == IPPROTO_UDP) {
		snprintk(buf, sizeof(buf), "AT+KUDPCLOSE=%d", sock->id);
	} else {
		snprintk(buf, sizeof(buf), "AT+KTCPCLOSE=%d", sock->id);
	}
	ret = modem_dynamic_cmd_send(socket_data_lcl->mdata_global, NULL, buf, strlen(buf),
				     allow_matches, 2, false);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", buf, ret);
	}
	return ret;
}

static int socket_delete(struct hl78xx_socket_data *socket_data_lcl, struct modem_socket *sock)
{
	char buf[sizeof("AT+KTCPDEL=##\r")];
	int ret = 0;

	if (sock->ip_proto == IPPROTO_UDP) {
		/**
		 * snprintk(buf, sizeof(buf), "AT+KUDPDEL=%d", sock->id);
		 * No need to delete udp config here according to ref guide. The at UDPCLOSE
		 * automatically deletes the session
		 */
		return 0;
	}
	snprintk(buf, sizeof(buf), "AT+KTCPDEL=%d", sock->id);
	ret = modem_dynamic_cmd_send(socket_data_lcl->mdata_global, NULL, buf, strlen(buf),
				     allow_matches, 2, false);
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

	HL78XX_LOG_DBG("%d %d %d %d", __LINE__, family, type, proto);
	/* defer modem's socket create call to bind() */
	ret = modem_socket_get(&socket_data_global->socket_config, family, type, proto);
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
	struct hl78xx_socket_data *socket_data_lcl = hl78xx_socket_data_from_sock(sock);

	if (!sock) {
		return -EINVAL;
	}
	/* Recover the containing instance; guard in case sock isn't from this driver */

	if (!socket_data_lcl) {
		LOG_ERR("Invalid socket container");
		return -EINVAL;
	}
	/* make sure socket is allocated and assigned an id */
	if (modem_socket_id_is_assigned(&socket_data_lcl->socket_config, sock) == false) {
		return 0;
	}
	if (validate_socket(sock) == 0) {
		socket_close(socket_data_lcl, sock);
		socket_delete(socket_data_lcl, sock);
		modem_socket_put(&socket_data_lcl->socket_config, sock->sock_fd);
		sock->is_connected = false;
	}
	/* Consider here successfully socket is closed */
	return 0;
}

static int offload_bind(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct hl78xx_socket_data *socket_data_lcl = hl78xx_socket_data_from_sock(sock);

	/* sanity check: does parent == parent->offload_dev->data ? */
	if (socket_data_lcl && socket_data_lcl->offload_dev &&
	    socket_data_lcl->offload_dev->data != socket_data_lcl) {
		LOG_WRN("parent mismatch: parent != offload_dev->data (%p != %p)", socket_data_lcl,
			socket_data_lcl->offload_dev->data);
	}
	HL78XX_LOG_DBG("%d", __LINE__);
	/*  Save bind address information */
	memcpy(&sock->src, addr, sizeof(*addr));
	/* Check if socket is allocated */
	if (modem_socket_is_allocated(&socket_data_lcl->socket_config, sock)) {
		/* Trigger socket creation */
		if (create_socket(sock, addr, socket_data_lcl) < 0) {
			LOG_ERR("%d %s SOCKET CREATION", __LINE__, __func__);
			return -1;
		}
	}
	return 0;
}

static int offload_connect(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct hl78xx_socket_data *socket_data_lcl = hl78xx_socket_data_from_sock(sock);
	int ret = 0;
	char cmd_buf[sizeof("AT+KTCPCFG=#\r")];
	char ip_str[NET_IPV6_ADDR_LEN];

	HL78XX_LOG_DBG("%d", __LINE__);
	if (!addr) {
		errno = EINVAL;
		return -1;
	}
	if (!socket_data_lcl || !socket_data_lcl->offload_dev) {
		errno = EINVAL;
		return -1;
	}
	/* sanity check: does parent == parent->offload_dev->data ? */
	if (socket_data_lcl && socket_data_lcl->offload_dev &&
	    socket_data_lcl->offload_dev->data != socket_data_lcl) {
		LOG_WRN("parent mismatch: parent != offload_dev->data (%p != %p)", socket_data_lcl,
			socket_data_lcl->offload_dev->data);
		return -1;
	}
	if (!hl78xx_is_registered(socket_data_lcl->mdata_global)) {
		errno = ENETUNREACH;
		return -1;
	}
	if (validate_socket(sock) == 0) {
		LOG_ERR("Socket is already connected! id: %d, fd: %d", sock->id, sock->sock_fd);
		errno = EISCONN;
		return -1;
	}
	/* make sure socket has been allocated */
	if (modem_socket_is_allocated(&socket_data_lcl->socket_config, sock) == false) {
		LOG_ERR("Invalid socket_id(%d) from fd:%d", sock->id, sock->sock_fd);
		errno = EINVAL;
		return -1;
	}
	/* make sure we've created the socket */
	if (modem_socket_id_is_assigned(&socket_data_lcl->socket_config, sock) == false) {
		LOG_DBG("%d no socket assigned", __LINE__);
		if (create_socket(sock, addr, socket_data_lcl) < 0) {
			return -1;
		}
	}
	memcpy(&sock->dst, addr, sizeof(*addr));
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
	/* send connect command */
	snprintk(cmd_buf, sizeof(cmd_buf), "AT+KTCPCNX=%d", sock->id);
	ret = modem_dynamic_cmd_send(socket_data_lcl->mdata_global, NULL, cmd_buf, strlen(cmd_buf),
				     &ktcpind_match, 1, false);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", cmd_buf, ret);
		errno = ret;
		return -1;
	}
	if (socket_data_lcl->tcp_conn_status.is_connected == false) {
		sock->is_connected = false;
		errno = socket_data_lcl->tcp_conn_status.err_code;
		return -(int)socket_data_lcl->tcp_conn_status.err_code;
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

static int lock_socket_mutex(struct hl78xx_socket_data *socket_data_lcl)
{
	int ret = k_mutex_lock(&socket_data_lcl->mdata_global->tx_lock, K_SECONDS(1));

	if (ret < 0) {
		LOG_ERR("Failed to acquire TX lock: %d", ret);
		errno = ret;
	}
	return ret;
}

static int wait_for_data_if_needed(struct hl78xx_socket_data *socket_data_lcl,
				   struct modem_socket *sock, int flags)
{
	int size = modem_socket_next_packet_size(&socket_data_lcl->socket_config, sock);

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

	modem_socket_wait_data(&socket_data_lcl->socket_config, sock);
	return modem_socket_next_packet_size(&socket_data_lcl->socket_config, sock);
}

static void prepare_read_command(struct hl78xx_socket_data *socket_data_lcl, char *sendbuf,
				 size_t bufsize, struct modem_socket *sock, size_t read_size)
{
	snprintk(sendbuf, bufsize, "AT+K%sRCV=%d,%zd%s",
		 sock->ip_proto == IPPROTO_UDP ? "UDP" : "TCP", sock->id, read_size,
		 socket_data_lcl->mdata_global->chat.delimiter);
}

static void setup_socket_data(struct hl78xx_socket_data *socket_data_lcl, struct modem_socket *sock,
			      struct socket_read_data *sock_data, void *buf, size_t len,
			      struct sockaddr *from, uint16_t read_size)
{
	memset(sock_data, 0, sizeof(*sock_data));
	sock_data->recv_buf = buf;
	sock_data->recv_buf_len = len;
	sock_data->recv_addr = from;
	sock->data = sock_data;

	socket_data_lcl->sizeof_socket_data = read_size;
	socket_data_lcl->requested_socket_id = sock->id;
	socket_data_lcl->current_sock_fd = sock->sock_fd;
	socket_data_lcl->expected_buf_len =
		read_size + sizeof("\r\n") - 1 +
		socket_data_lcl->mdata_global->buffers.eof_pattern_size +
		sizeof(MODEM_STREAM_END_WORD) - 1;
	socket_data_lcl->collected_buf_len = 0;
	socket_data_lcl->socket_data_error = false;
}

static void restore_socket_state(struct hl78xx_socket_data *socket_data_lcl)
{
	k_mutex_unlock(&socket_data_lcl->mdata_global->tx_lock);
	modem_chat_attach(&socket_data_lcl->mdata_global->chat,
			  socket_data_lcl->mdata_global->uart_pipe);
	socket_data_lcl->expected_buf_len = 0;
}

static void check_tcp_state_if_needed(struct hl78xx_socket_data *socket_data_lcl,
				      struct modem_socket *sock)
{
	if (atomic_test_and_clear_bit(&state_leftover, MODEM_SOCKET_DATA_LEFTOVER_STATE_BIT) &&
	    sock->ip_proto == IPPROTO_TCP) {
		const char *check_ktcp_stat = "AT+KTCPSTAT";

		modem_dynamic_cmd_send(socket_data_lcl->mdata_global, NULL, check_ktcp_stat,
				       strlen(check_ktcp_stat), &ktcp_state_match, 1, true);
	}
}

static ssize_t offload_recvfrom(void *obj, void *buf, size_t len, int flags, struct sockaddr *from,
				socklen_t *fromlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct hl78xx_socket_data *socket_data_lcl = hl78xx_socket_data_from_sock(sock);
	int ret;
	char sendbuf[sizeof("AT+KUDPRCV=#,##########\r\n")];
	struct socket_read_data sock_data;
	int next_packet_size = 0;
	uint32_t max_data_length = 0;
	uint16_t read_size = 0;

	if (!hl78xx_is_registered(socket_data_lcl->mdata_global) || validate_socket(sock) == -1) {
		errno = EAGAIN;
		return -1;
	}
	if (!validate_recv_args(buf, len, flags)) {
		return -1;
	}

	ret = lock_socket_mutex(socket_data_lcl);
	if (ret < 0) {
		return -1;
	}

	next_packet_size = wait_for_data_if_needed(socket_data_lcl, sock, flags);

	if (next_packet_size <= 0) {
		ret = next_packet_size;
		goto exit;
	}

	max_data_length =
		MDM_MAX_DATA_LENGTH - (socket_data_lcl->mdata_global->buffers.eof_pattern_size +
				       sizeof(MODEM_STREAM_STARTER_WORD) - 1);
	/* limit read size to modem max data length */
	next_packet_size = MIN(next_packet_size, max_data_length);
	/* limit read size to user buffer length */
	read_size = MIN(next_packet_size, len);
	setup_socket_data(socket_data_lcl, sock, &sock_data, buf, len, from, read_size);
	prepare_read_command(socket_data_lcl, sendbuf, sizeof(sendbuf), sock, read_size);
	HL78XX_LOG_DBG("%d socket_fd: %d, socket_id: %d, expected_data_len: %d", __LINE__,
		       socket_data_lcl->current_sock_fd, socket_data_lcl->requested_socket_id,
		       socket_data_lcl->expected_buf_len);
	LOG_HEXDUMP_DBG(sendbuf, strlen(sendbuf), "sending");
	modem_chat_release(&socket_data_lcl->mdata_global->chat);
	modem_pipe_attach(socket_data_lcl->mdata_global->uart_pipe, modem_pipe_callback,
			  socket_data_lcl->mdata_global);
	if (k_sem_take(&socket_data_lcl->mdata_global->script_stopped_sem_tx_int, K_FOREVER) < 0 ||
	    modem_pipe_transmit(socket_data_lcl->mdata_global->uart_pipe, sendbuf,
				strlen(sendbuf)) < 0 ||
	    k_sem_take(&socket_data_lcl->mdata_global->script_stopped_sem_rx_int, K_FOREVER) < 0 ||
	    modem_handle_data_capture(read_size, socket_data_lcl->mdata_global) < 0) {
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
	HL78XX_LOG_DBG("%d %d %d", __LINE__, errno, ret);
	restore_socket_state(socket_data_lcl);
	check_tcp_state_if_needed(socket_data_lcl, sock);
	return ret;
}

int check_if_any_socket_connected(const struct device *dev)
{
	struct hl78xx_data *data = dev->data;
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;
	struct modem_socket_config *cfg = &socket_data->socket_config;

	k_sem_take(&cfg->sem_lock, K_FOREVER);
	for (int i = 0; i < cfg->sockets_len; i++) {
		if (cfg->sockets[i].is_connected) {
			/* if there is any socket  connected */
			k_sem_give(&cfg->sem_lock);
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

static int send_data_buffer(struct hl78xx_socket_data *socket_data_lcl, const char *buf,
			    const size_t buf_len, int *sock_written)
{
	uint32_t offset = 0;
	int len = buf_len;
	int ret = 0;

	if (len == 0) {
		LOG_DBG("%d No data to send", __LINE__);
		return 0;
	}
	while (len > 0) {
		if (k_sem_take(&socket_data_lcl->mdata_global->script_stopped_sem_tx_int,
			       K_FOREVER) < 0) {
			return -1;
		}
		ret = modem_pipe_transmit(socket_data_lcl->mdata_global->uart_pipe,
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
	return prepare_tcp_send_cmd(sock, *buf_len, cmd_buf, cmd_buf_len);
}

static int transmit_regular_data(struct hl78xx_socket_data *socket_data_lcl, const char *buf,
				 size_t buf_len, int *sock_written)
{
	int ret;

	ret = send_data_buffer(socket_data_lcl, buf, buf_len, sock_written);
	if (ret < 0) {
		return ret;
	}
	ret = k_sem_take(&socket_data_lcl->mdata_global->script_stopped_sem_tx_int, K_FOREVER);
	if (ret < 0) {
		return ret;
	}
	return modem_pipe_transmit(socket_data_lcl->mdata_global->uart_pipe,
				   (uint8_t *)socket_data_lcl->mdata_global->buffers.eof_pattern,
				   socket_data_lcl->mdata_global->buffers.eof_pattern_size);
}

/* send binary data via the +KUDPSND/+KTCPSND commands */
static ssize_t send_socket_data(void *obj, const struct sockaddr *dst_addr, const char *buf,
				size_t buf_len, k_timeout_t timeout)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct hl78xx_socket_data *socket_data_lcl = hl78xx_socket_data_from_sock(sock);
	char cmd_buf[82] = {0}; /* AT+KUDPSND/KTCP=,IP,PORT,LENGTH */
	int ret;
	int sock_written = 0;

	ret = validate_and_prepare(sock, &dst_addr, &buf_len, cmd_buf, sizeof(cmd_buf));
	if (ret < 0) {
		return ret;
	}
	socket_data_lcl->socket_data_error = false;
	if (k_mutex_lock(&socket_data_lcl->mdata_global->tx_lock, K_SECONDS(1)) < 0) {
		return -1;
	}
	ret = modem_dynamic_cmd_send(socket_data_lcl->mdata_global, NULL, cmd_buf, strlen(cmd_buf),
				     connect_matches, ARRAY_SIZE(connect_matches), false);
	if (ret < 0 || socket_data_lcl->socket_data_error) {
		LOG_ERR("%d %s %d %d", __LINE__, __func__, ret, socket_data_lcl->socket_data_error);
		errno = (ret < 0) ? ret : EIO;
		ret = -1;
		goto cleanup;
	}
	modem_pipe_attach(socket_data_lcl->mdata_global->chat.pipe, modem_pipe_callback,
			  socket_data_lcl->mdata_global);
	ret = transmit_regular_data(socket_data_lcl, buf, buf_len, &sock_written);
	if (ret < 0) {
		goto cleanup;
	}
	modem_chat_attach(&socket_data_lcl->mdata_global->chat,
			  socket_data_lcl->mdata_global->uart_pipe);
	ret = modem_dynamic_cmd_send(socket_data_lcl->mdata_global, NULL, "", 0, &ok_match, 1,
				     false);
	if (ret < 0) {
		LOG_ERR("Final confirmation failed: %d", ret);
		goto cleanup;
	}
	HL78XX_LOG_DBG("%d %d %d", __LINE__, sock_written, ret);
cleanup:
	k_mutex_unlock(&socket_data_lcl->mdata_global->tx_lock);
	return (ret < 0) ? -1 : sock_written;
}

#ifdef CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS
static int handle_tls_sockopts(void *obj, int optname, const void *optval, socklen_t optlen)
{
	int ret;
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct hl78xx_socket_data *socket_data_lcl = hl78xx_socket_data_from_sock(sock);

	switch (optname) {
	case TLS_SEC_TAG_LIST:
		ret = map_credentials(socket_data_lcl, optval, optlen);
		return ret;

	case TLS_HOSTNAME:
		if (optlen >= MDM_MAX_HOSTNAME_LEN) {
			return -EINVAL;
		}
		memset(socket_data_lcl->tls.hostname, 0, MDM_MAX_HOSTNAME_LEN);
		memcpy(socket_data_lcl->tls.hostname, optval, optlen);
		socket_data_lcl->tls.hostname[optlen] = '\0';
		socket_data_lcl->tls.hostname_set = true;
		ret = hl78xx_configure_chipper_suit(socket_data_lcl);
		if (ret < 0) {
			LOG_ERR("Failed to configure chipper suit: %d", ret);
			errno = ret;
			return -1;
		}
		LOG_DBG("TLS hostname set to: %s", socket_data_lcl->tls.hostname);
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

static int offload_setsockopt(void *obj, int level, int optname, const void *optval,
			      socklen_t optlen)
{
	HL78XX_LOG_DBG("%d level: %d optname: %d", __LINE__, level, optname);

	if (!IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)) {
		return -EINVAL;
	}
	if (level == SOL_TLS) {
		return handle_tls_sockopts(obj, optname, optval, optlen);
	}
	LOG_DBG("Unsupported socket option: %d", optname);
	return -EINVAL;
}
#endif /* CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS */

static ssize_t offload_sendto(void *obj, const void *buf, size_t len, int flags,
			      const struct sockaddr *to, socklen_t tolen)
{
	int ret = 0;
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct hl78xx_socket_data *socket_data_lcl = hl78xx_socket_data_from_sock(sock);

	HL78XX_LOG_DBG("%d %d %d", __LINE__,
		       socket_data_lcl->mdata_global->status.registration.is_registered_currently,
		       socket_data_lcl->mdata_global->status.registration.network_state_current);

	if (!hl78xx_is_registered(socket_data_lcl->mdata_global)) {
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
	struct hl78xx_socket_data *socket_data_lcl = hl78xx_socket_data_from_sock(sock);
	struct zsock_pollfd *pfd;
	struct k_poll_event **pev;
	struct k_poll_event *pev_end;
	/* sanity check: does parent == parent->offload_dev->data ? */
	if (socket_data_lcl && socket_data_lcl->offload_dev &&
	    socket_data_lcl->offload_dev->data != socket_data_lcl) {
		LOG_WRN("parent mismatch: parent != offload_dev->data (%p != %p)", socket_data_lcl,
			socket_data_lcl->offload_dev->data);
	}
	HL78XX_LOG_DBG("%d %d", __LINE__, request);
	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE:
		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);
		ret = modem_socket_poll_prepare(&socket_data_lcl->socket_config, obj, pfd, pev,
						pev_end);

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

	case ZFD_IOCTL_POLL_UPDATE:
		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		return modem_socket_poll_update(obj, pfd, pev);

	case F_GETFL:
		return 0;

	case F_SETFL: {
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
		int flags = va_arg(args, int);

		LOG_DBG("F_SETFL called with flags=0x%x", flags);
		ARG_UNUSED(flags);
#endif
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
	/* Compute the full length to send and validate input */
	for (int i = 0; i < msg->msg_iovlen; i++) {
		if (!msg->msg_iov[i].iov_base || msg->msg_iov[i].iov_len == 0) {
			errno = EINVAL;
			return -1;
		}
		full_len += msg->msg_iov[i].iov_len;
	}
	HL78XX_LOG_DBG("msg_iovlen:%zd flags:%d, full_len:%zd", msg->msg_iovlen, flags, full_len);
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
#if defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS)
	.setsockopt = offload_setsockopt,
#else
	.setsockopt = NULL,
#endif
};
/* clang-format on */
static int hl78xx_init_sockets(const struct device *dev)
{
	int ret;
	struct hl78xx_socket_data *socket_data_lcl = (struct hl78xx_socket_data *)dev->data;

	socket_data_lcl->buf_pool = &mdm_recv_pool;
	/* socket config */
	ret = modem_socket_init(&socket_data_lcl->socket_config, &socket_data_lcl->sockets[0],
				ARRAY_SIZE(socket_data_lcl->sockets), MDM_BASE_SOCKET_NUM, false,
				&offload_socket_fd_op_vtable);
	if (ret) {
		goto error;
	}
	return 0;
error:
	return ret;
}

void socket_notify_data(int socket_id, int new_total, void *user_data)
{
	struct modem_socket *sock;
	int ret = 0;
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;

	sock = modem_socket_from_id(&socket_data->socket_config, socket_id);
	if (!sock) {
		return;
	}
	HL78XX_LOG_DBG("%d new total: %d sckid%d", __LINE__, new_total, socket_id);
	/* Update the packet size */
	ret = modem_socket_packet_size_update(&socket_data->socket_config, sock, new_total);
	if (ret < 0) {
		LOG_ERR("socket_id:%d left_bytes:%d err: %d", socket_id, new_total, ret);
	}
	if (new_total > 0) {
		modem_socket_data_ready(&socket_data->socket_config, sock);
	}
}

void tcp_notify_data(int socket_id, int tcp_notif, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;
	enum hl78xx_tcp_notif tcp_notif_received = tcp_notif;

	socket_data->requested_socket_id = socket_id;
	switch (tcp_notif_received) {
	case TCP_NOTIF_REMOTE_DISCONNECTION:
		socket_notify_data(socket_id, 1, user_data);
		break;
	case TCP_NOTIF_NETWORK_ERROR:
		/* Handle network error */
		break;
	default:
		break;
	}
}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS)
static int hl78xx_configure_chipper_suit(struct hl78xx_socket_data *socket_data_lcl)
{
	const char *cmd_chipper_suit = "AT+KSSLCRYPTO=0,8,1,8192,4,4,3,0";

	return modem_dynamic_cmd_send(socket_data_lcl->mdata_global, NULL, cmd_chipper_suit,
				      strlen(cmd_chipper_suit), &ok_match, 1, false);
}
/* send binary data via the K....STORE commands */
static ssize_t hl78xx_send_cert(struct hl78xx_socket_data *socket_data_lcl, const char *cert_data,
				size_t cert_len, enum tls_credential_type cert_type)
{
	int ret;
	char send_buf[sizeof("AT+KPRIVKSTORE=#,####\r\n")];
	int sock_written = 0;

	if (!socket_data_lcl || !socket_data_lcl->mdata_global) {
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
	socket_data_lcl->socket_data_error = false;
	if (k_mutex_lock(&socket_data_lcl->mdata_global->tx_lock, K_SECONDS(1)) < 0) {
		errno = EBUSY;
		return -1;
	}
	ret = modem_dynamic_cmd_send(socket_data_lcl->mdata_global, NULL, send_buf,
				     strlen(send_buf), connect_matches, ARRAY_SIZE(connect_matches),
				     false);
	if (ret < 0) {
		LOG_ERR("Error sending AT command %d", ret);
	}
	if (socket_data_lcl->socket_data_error) {
		ret = -ENODEV;
		errno = ENODEV;
		goto cleanup;
	}
	modem_pipe_attach(socket_data_lcl->mdata_global->chat.pipe, modem_pipe_callback,
			  socket_data_lcl->mdata_global);
	ret = send_data_buffer(socket_data_lcl, cert_data, cert_len, &sock_written);
	if (ret < 0) {
		goto cleanup;
	}
	ret = k_sem_take(&socket_data_lcl->mdata_global->script_stopped_sem_tx_int, K_FOREVER);
	if (ret < 0) {
		goto cleanup;
	}
	ret = modem_pipe_transmit(socket_data_lcl->mdata_global->uart_pipe,
				  (uint8_t *)socket_data_lcl->mdata_global->buffers.eof_pattern,
				  socket_data_lcl->mdata_global->buffers.eof_pattern_size);
	if (ret < 0) {
		LOG_ERR("Error sending EOF pattern: %d", ret);
	}
	modem_chat_attach(&socket_data_lcl->mdata_global->chat,
			  socket_data_lcl->mdata_global->uart_pipe);
	ret = modem_dynamic_cmd_send(socket_data_lcl->mdata_global, NULL, "", 0, &ok_match, 1,
				     false);
	if (ret < 0) {
		LOG_ERR("Final confirmation failed: %d", ret);
		goto cleanup;
	}
	HL78XX_LOG_DBG("%d %d %d", __LINE__, sock_written, ret);
cleanup:
	k_mutex_unlock(&socket_data_lcl->mdata_global->tx_lock);
	return (ret < 0) ? -1 : sock_written;

	return ret;
}

static int map_credentials(struct hl78xx_socket_data *socket_data_lcl, const void *optval,
			   socklen_t optlen)
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
			ret = hl78xx_send_cert(socket_data_lcl, cert->buf, cert->len, cert->type);
			if (ret < 0) {
				return ret;
			}
			cert = credential_next_get(tag, cert);
		}
	}

	return 0;
}
#endif

static int hl78xx_socket_init(const struct device *dev)
{

	struct hl78xx_socket_data *data = (struct hl78xx_socket_data *)dev->data;

	data->offload_dev = dev;
	HL78XX_LOG_DBG("%d %s", __LINE__, dev->name);
	/* Ensure the parent modem device pointer was set at static init time */
	if (data->modem_dev == NULL) {
		LOG_ERR("modem_dev not initialized for %s", dev->name);
		return -EINVAL;
	}
	/* Ensure the modem device is ready before accessing its driver data */
	if (!device_is_ready(data->modem_dev)) {
		LOG_ERR("modem device %s not ready", data->modem_dev->name);
		return -ENODEV;
	}
	if (data->modem_dev->data == NULL) {
		LOG_ERR("modem device %s has no driver data yet", data->modem_dev->name);
		return -EAGAIN;
	}
	data->mdata_global = (struct hl78xx_data *)data->modem_dev->data;
	data->mdata_global->offload_dev = dev;
	/* Note: a single global pointer is used in the original driver; keep
	 * that behaviour.
	 */
	socket_data_global = data;
	atomic_set(&state_leftover, 0);

	return 0;
}

static void modem_net_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct hl78xx_socket_data *data = dev->data;

	HL78XX_LOG_DBG("%d", __LINE__);
	if (!data->mdata_global) {
		LOG_WRN("mdata_global not set for net iface init on %s", dev->name);
	}
	net_if_set_link_addr(
		iface,
		modem_get_mac(data->mac_addr,
			      data->mdata_global ? data->mdata_global->identity.imei : NULL),
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
	bool fam_ok = false;

#ifdef CONFIG_NET_IPV4
	if (family == AF_INET) {
		fam_ok = true;
	}
#endif
#ifdef CONFIG_NET_IPV6
	if (family == AF_INET6) {
		fam_ok = true;
	}
#endif
	if (!fam_ok) {
		return false;
	}
	if (!(type == SOCK_DGRAM || type == SOCK_STREAM)) {
		return false;
	}
	if (proto == IPPROTO_TCP || proto == IPPROTO_UDP) {
		return true;
	}
#if defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS)
	if (proto == IPPROTO_TLS_1_2) {
		return true;
	}
#endif
	return false;
}

#define MODEM_HL78XX_DEFINE_OFFLOAD_INSTANCE(inst)                                                 \
	static struct hl78xx_socket_data hl78xx_socket_data_##inst = {                             \
		.modem_dev = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                          \
	};                                                                                         \
	NET_DEVICE_OFFLOAD_INIT(                                                                   \
		inst, "hl78xx_dev", hl78xx_socket_init, NULL, &hl78xx_socket_data_##inst, NULL,    \
		CONFIG_MODEM_HL78XX_OFFLOAD_INIT_PRIORITY, &api_funcs, MDM_MAX_DATA_LENGTH);       \
                                                                                                   \
	NET_SOCKET_OFFLOAD_REGISTER(inst, CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY, AF_UNSPEC,          \
				    offload_is_supported, offload_socket);

#define MODEM_OFFLOAD_DEVICE_SWIR_HL78XX(inst) MODEM_HL78XX_DEFINE_OFFLOAD_INSTANCE(inst)

#define DT_DRV_COMPAT swir_hl7812_offload
DT_INST_FOREACH_STATUS_OKAY(MODEM_OFFLOAD_DEVICE_SWIR_HL78XX)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT swir_hl7800_offload
DT_INST_FOREACH_STATUS_OKAY(MODEM_OFFLOAD_DEVICE_SWIR_HL78XX)
#undef DT_DRV_COMPAT
