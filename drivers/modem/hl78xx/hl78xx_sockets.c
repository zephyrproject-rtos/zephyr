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

	/* rx net buffer */
	struct ring_buf *buf_pool;
	uint32_t expected_buf_len;
	uint32_t collected_buf_len;

	struct modem_hl78xx_data *mdata_global;
};

struct hl78xx_socket_data socket_data;
static const struct socket_op_vtable offload_socket_fd_op_vtable;

static int modem_cmd_send_int(struct modem_hl78xx_data *user_data,
			      modem_chat_script_callback script_user_callback, const uint8_t *cmd,
			      uint16_t cmd_len, const struct modem_chat_match *response_matches,
			      uint16_t matches_size);
static int offload_socket(int family, int type, int protom);

static void modem_cellular_chat_on_kudpsnd(struct modem_chat *chat, char **argv, uint16_t argc,
					   void *user_data)
{
	struct modem_socket *sock = NULL;
	int id;

	/* look up new socket by special id */
	sock = modem_socket_from_newid(&socket_data.socket_config);
	if (sock) {
		id = ATOI(argv[1], -1, "socket_id");

		/* on error give up modem socket */
		if (modem_socket_id_assign(&socket_data.socket_config, sock, id) < 0) {

			modem_socket_put(&socket_data.socket_config, sock->sock_fd);
		}
	}
}

/* Handler: +USOCR: <socket_id>[0] */
static void modem_cellular_chat_on_cmd_sockcreate(struct modem_chat *chat, char **argv,
						  uint16_t argc, void *user_data)
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

/* Convert HL7800 IPv6 address string in format
 * a01.a02.a03.a04.a05.a06.a07.a08.a09.a10.a11.a12.a13.a14.a15.a16 to
 * an IPv6 address.
 */
#ifdef CONFIG_NET_IPV6
static int hl7800_net_addr6_pton(const char *src, struct in6_addr *dst)
{
	int num_sections = 8;
	int i, len;
	uint16_t ipv6_section;

	len = strlen(src);
	for (i = 0; i < len; i++) {
		if (!(src[i] >= '0' && src[i] <= '9') && src[i] != '.') {
			return -EINVAL;
		}
	}

	for (i = 0; i < num_sections; i++) {
		if (!src || *src == '\0') {
			return -EINVAL;
		}

		ipv6_section = (uint16_t)strtol(src, NULL, 10);
		src = strchr(src, '.');
		if (!src) {
			return -EINVAL;
		}
		src++;
		if (*src == '\0') {
			return -EINVAL;
		}
		ipv6_section = (ipv6_section << 8) | (uint16_t)strtol(src, NULL, 10);
		UNALIGNED_PUT(htons(ipv6_section), &dst->s6_addr16[i]);

		src = strchr(src, '.');
		if (src) {
			src++;
		} else {
			if (i < num_sections - 1) {
				return -EINVAL;
			}
		}
	}

	return 0;
}
#endif
/* Handler: +CGCONTRDP: <cid>,<bearer_id>,<apn>,<local_addr and subnet_mask>,
 *			<gw_addr>,<DNS_prim_addr>,<DNS_sec_addr>
 */
static void modem_cellular_chat_on_cgdcontrdp(struct modem_chat *chat, char **argv, uint16_t argc,
					      void *user_data)
{
	char *search_start, *addr_start, *sm_start;
	bool is_ipv4;
	int addr_len;
	char temp_addr_str[MODEM_HL78XX_ADDRESS_FAMILY_FORMAT_LEN];
	int ret = 0;
	struct in_addr ipv4Addr, subnet, gateway, dns_v4;
	struct in_addr new_ipv4_addr;
	bool dns_ready = false;
#ifdef CONFIG_NET_IPV6
	struct in6_addr new_ipv6_addr;

	struct in6_addr ipv6Addr, dns_v6;
#endif
	char *delims[CGCONTRDP_RESPONSE_NUM_DELIMS];
	char value[MDM_IP_INFO_RESP_SIZE];
	int num_delims = CGCONTRDP_RESPONSE_NUM_DELIMS;

	search_start = argv[1];
	/* find all delimiters (,) */
	for (int i = 0; i < num_delims; i++) {
		delims[i] = strchr(search_start, ',');
		if (!delims[i]) {
			LOG_ERR("Could not find delim %d, val: %s", i, value);
			return;
		}
		/* Start next search after current delim location */
		search_start = delims[i] + 1;
	}

	/* determine if IPv4 or IPv6 by checking length of ip address plus
	 * gateway string.
	 */
	is_ipv4 = false;
	addr_len = delims[3] - delims[2];
	LOG_DBG("IP string len: %d", addr_len);
	if (addr_len <= (NET_IPV4_ADDR_LEN * 2)) {
		is_ipv4 = true;
	}

	/* Find start of subnet mask */
	addr_start = delims[2] + 1;
	if (is_ipv4) {
		num_delims = 4;
	} else {
		num_delims = 16;
	}
	search_start = addr_start;
	sm_start = addr_start;
	for (int i = 0; i < num_delims; i++) {
		sm_start = strchr(search_start, '.');
		if (!sm_start) {
			LOG_ERR("Could not find submask start");
			return;
		}
		/* Start next search after current delim location */
		search_start = sm_start + 1;
	}
	/* get new IP addr */
	addr_len = sm_start - addr_start;
	strncpy(temp_addr_str, addr_start, addr_len);
	temp_addr_str[addr_len] = 0;
	LOG_DBG("IP addr: %s", temp_addr_str);
	if (is_ipv4) {
		ret = net_addr_pton(AF_INET, temp_addr_str, &new_ipv4_addr);
	}
#ifdef CONFIG_NET_IPV6
	else {
		ret = hl7800_net_addr6_pton(temp_addr_str, &new_ipv6_addr);
	}
#endif
	if (ret < 0) {
		LOG_ERR("Invalid IP addr");
		return;
	}

	if (is_ipv4) {
		/* move past the '.' */
		sm_start += 1;
		/* store new subnet mask */
		addr_len = delims[3] - sm_start;
		strncpy(temp_addr_str, sm_start, addr_len);
		temp_addr_str[addr_len] = 0;
		ret = net_addr_pton(AF_INET, temp_addr_str, &subnet);
		if (ret < 0) {
			LOG_ERR("Invalid subnet");
			return;
		}

		/* store new gateway */
		addr_start = delims[3] + 1;
		addr_len = delims[4] - addr_start;
		strncpy(temp_addr_str, addr_start, addr_len);
		temp_addr_str[addr_len] = 0;
		ret = net_addr_pton(AF_INET, temp_addr_str, &gateway);
		if (ret < 0) {
			LOG_ERR("Invalid gateway");
			return;
		}
	}

	/* store new dns */
	addr_start = delims[4] + 1;
	addr_len = delims[5] - addr_start;
	strncpy(temp_addr_str, addr_start, addr_len);
	temp_addr_str[addr_len] = 0;
	if (is_ipv4) {
		ret = strncmp(temp_addr_str, socket_data.dns_v4_string, addr_len);
		if (ret != 0) {
			dns_ready = false;
		}
		strncpy(socket_data.dns_v4_string, addr_start, addr_len);
		socket_data.dns_v4_string[addr_len] = 0;
		ret = net_addr_pton(AF_INET, socket_data.dns_v4_string, &dns_v4);
		LOG_DBG("IPv4 DNS addr: %s", socket_data.dns_v4_string);
	}
#ifdef CONFIG_NET_IPV6
	else {
		ret = strncmp(temp_addr_str, socket_data.dns_v6_string, addr_len);
		if (ret != 0) {
			dns_ready = false;
		}
		/* store HL7800 formatted IPv6 DNS string temporarily */
		strncpy(socket_data.dns_v6_string, addr_start, addr_len);
		LOG_DBG("%d %s", __LINE__, __func__);
		ret = hl7800_net_addr6_pton(socket_data.dns_v6_string, &dns_v6);
		net_addr_ntop(AF_INET6, &dns_v6, socket_data.dns_v6_string,
			      sizeof(socket_data.dns_v6_string));
		LOG_DBG("IPv6 DNS addr: %s", socket_data.dns_v6_string);
	}
#endif
	if (ret < 0) {
		LOG_ERR("Invalid dns");
		return;
	}
	if (socket_data.net_iface) {
		if (is_ipv4) {
#ifdef CONFIG_NET_IPV4
			/* remove the current IPv4 addr before adding a new one.
			 * We dont care if it is successful or not.
			 */
			if (!net_if_ipv4_addr_add(socket_data.net_iface, &new_ipv4_addr,
						  NET_ADDR_DHCP, 0)) {
				LOG_ERR("Cannot set iface IPv4 addr");
			}
			net_if_ipv4_set_netmask_by_addr(socket_data.net_iface, &new_ipv4_addr,
							&subnet);
			net_if_ipv4_set_gw(socket_data.net_iface, &gateway);
#endif
			/* store the new IP addr */
			net_ipaddr_copy(&ipv4Addr, &new_ipv4_addr);
		} else {
#if CONFIG_NET_IPV6
			net_if_ipv6_addr_rm(socket_data.net_iface, &ipv6Addr);
			if (!net_if_ipv6_addr_add(socket_data.net_iface, &new_ipv6_addr,
						  NET_ADDR_AUTOCONF, 0)) {
				LOG_ERR("Cannot set iface IPv6 addr");
			}
			LOG_DBG("%d %s", __LINE__, __func__);
#endif
		}
	}
}

MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", NULL);
MODEM_CHAT_MATCHES_DEFINE(connect_matches, MODEM_CHAT_MATCH(CONNECT_STRING, "", NULL),
			  MODEM_CHAT_MATCH("ERROR", "", NULL));
MODEM_CHAT_MATCH_DEFINE(kudpind_match, "+KUDP_IND: ", ",", modem_cellular_chat_on_kudpsnd);
MODEM_CHAT_MATCH_DEFINE(ktcpind_match, "+KTCP_IND: ", ",", modem_cellular_chat_on_kudpsnd);
MODEM_CHAT_MATCHES_DEFINE(abort_matches, MODEM_CHAT_MATCH("ERROR", "", NULL));
MODEM_CHAT_MATCH_DEFINE(ktcp_match, "+KTCPCFG: ", "", modem_cellular_chat_on_cmd_sockcreate);
MODEM_CHAT_MATCH_DEFINE(cgdcontrdp_match, "+CGCONTRDP: ", "", modem_cellular_chat_on_cgdcontrdp);

static int modem_chat_process_handler(void)
{
	int ret = 0, err = 0;
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

static void modem_chat_pipe_callback(struct modem_pipe *pipe, enum modem_pipe_event event,
				     void *user_data)
{
	int ret = 0;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s Pipe event received: %d", __LINE__, __func__, event);
#endif
	switch (event) {
	case MODEM_PIPE_EVENT_RECEIVE_READY:
		if (socket_data.expected_buf_len > socket_data.collected_buf_len) {
			ret = modem_chat_process_handler();
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

int modem_cmd_send_int(struct modem_hl78xx_data *user_data,
		       modem_chat_script_callback script_user_callback, const uint8_t *cmd,
		       uint16_t cmd_size, const struct modem_chat_match *response_matches,
		       uint16_t matches_size)
{
	int ret = 0;

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s sending: [%s] size: %d", __LINE__, __func__, cmd, cmd_size);
#endif
	ret = k_mutex_lock(&socket_data.mdata_global->tx_lock, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to lock tx mutex (%d)", ret);
		errno = -ret;
		return -1;
	}

	struct modem_chat_script_chat dynamic_script = {
		.request = cmd,
		.request_size = cmd_size,
		.response_matches = response_matches,
		.response_matches_size = matches_size,
		.timeout = 1000,
	};

	struct modem_chat_script chat_script = {
		.name = "dynamic_script",
		.script_chats = &dynamic_script,
		.script_chats_size = 1,
		.abort_matches = abort_matches,
		.abort_matches_size = ARRAY_SIZE(abort_matches),
		.callback = script_user_callback,
		.timeout = 1000,
	};

	ret = modem_chat_run_script(&user_data->chat, &chat_script);
	if (ret < 0) {
		LOG_ERR("Chat script execution failed (%d)", ret);
	} else {
		LOG_DBG("Chat script executed successfully.");
	}

	ret = k_mutex_unlock(&socket_data.mdata_global->tx_lock);
	if (ret < 0) {
		LOG_ERR("Failed to unlock tx mutex (%d)", ret);
		errno = -ret;
		return -1;
	}

	return ret;
}

void iface_status_work_cb(struct modem_hl78xx_data *data)
{

	char *cmd = "AT+CGCONTRDP=1";
	int ret = 0;

	ret = modem_cmd_send_int(data, NULL, cmd, strlen(cmd), &cgdcontrdp_match, 1);
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

	const char *const dns_servers_str[] = {
#ifdef CONFIG_NET_IPV6
		socket_data.dns_v6_string,
#endif
#ifdef CONFIG_NET_IPV4
		socket_data.dns_v4_string,
#endif
		NULL};

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

	/* set new DNS addr in DNS resolver */
	LOG_DBG("Refresh DNS resolver");
	dnsCtx = dns_resolve_get_default();
	if (dnsCtx->state == DNS_RESOLVE_CONTEXT_INACTIVE) {
		LOG_DBG("Initializing DNS resolver");
		ret = dns_resolve_init(dnsCtx, (const char **)dns_servers_str, NULL);
	} else {
		LOG_DBG("Reconfiguring DNS resolver");
		ret = dns_resolve_reconfigure(dnsCtx, (const char **)dns_servers_str, NULL);
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
	int ret = 0, packet_size = 0;
	uint8_t *eof_ptr;
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

int modem_chat_handle_data_capture(size_t target_len, struct modem_hl78xx_data *data)
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
			 struct modem_hl78xx_data *data)
{
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s", __LINE__, __func__);
#endif
	int ret, af;

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

	if (sock->ip_proto == IPPROTO_TLS_1_2) {

		/* Enable socket security */
		mode = 3; /* TLS 1.2 */
			  /* Reset the security profile */
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
		ret = modem_cmd_send_int(data, NULL, cmd_buf, strlen(cmd_buf), &ktcp_match, 1);
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

		ret = modem_cmd_send_int(data, NULL, cmd_buf, strlen(cmd_buf), &kudpind_match, 1);
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

	if (!modem_cellular_is_registered(socket_data.mdata_global)) {
		LOG_ERR("Modem currently not attached to the network!");
		return -EAGAIN;
	}
	/* make sure socket is allocated and assigned an id */
	if (modem_socket_id_is_assigned(&socket_data.socket_config, sock) == false) {
		return 0;
	}

	if (sock->is_connected) {
		if (sock->ip_proto == IPPROTO_UDP) {
			snprintk(buf, sizeof(buf), "AT+KTCPCLOSE=%d", sock->id);

			ret = modem_cmd_send_int(socket_data.mdata_global, NULL, buf, strlen(buf),
						 NULL, 0);
			if (ret < 0) {
				LOG_ERR("%s ret:%d", buf, ret);
			}
		} else {
			snprintk(buf, sizeof(buf), "AT+KTCPCLOSE=%d", sock->id);
			ret = modem_cmd_send_int(socket_data.mdata_global, NULL, buf, strlen(buf),
						 NULL, 0);
			if (ret < 0) {
				LOG_ERR("%s ret:%d", buf, ret);
			}
		}
	}

	modem_socket_put(&socket_data.socket_config, sock->sock_fd);
	return 0;
}

static int offload_bind(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
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
	int ret = 0, af;
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
	if (!modem_cellular_is_registered(socket_data.mdata_global)) {
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
	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, buf, strlen(buf), &ktcp_match, 1);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", buf, ret);
		errno = -ret;
		return -1;
	}

	snprintk(buf, sizeof(buf), "AT+KTCPCNX=%d", sock->id);
	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, buf, strlen(buf), &ok_match, 1);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", buf, ret);
		errno = -ret;
		return -1;
	}
	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, "", 0, &ktcpind_match, 1);
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
	int ret, next_packet_size;
	char sendbuf[sizeof("AT+KUDPRCV=#,##########\r\n")];
	struct socket_read_data sock_data;

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
		MDM_MAX_DATA_LENGTH - (socket_data.mdata_global->chat_eof_pattern_size +
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
	socket_data.expected_buf_len = read_size + socket_data.mdata_global->chat_eof_pattern_size +
				       sizeof(MODEM_STREAM_STARTER_WORD) - 1;
	modem_chat_release(&socket_data.mdata_global->chat);
	modem_pipe_attach(socket_data.mdata_global->uart_pipe, modem_chat_pipe_callback,
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
	ret = modem_chat_handle_data_capture(socket_data.expected_buf_len,
					     socket_data.mdata_global);
	if (ret) {
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
	k_mutex_unlock(&socket_data.mdata_global->tx_lock);
	modem_chat_attach(&socket_data.mdata_global->chat, socket_data.mdata_global->uart_pipe);
	k_work_submit(&socket_data.mdata_global->chat.receive_work);
	socket_data.expected_buf_len = 0;
	/* clear socket data */
	socket_data.current_sock_fd = -1;
	sock->data = NULL;
	return ret;
}

/* send binary data via the +USO[ST/WR] commands */
static ssize_t send_socket_data(void *obj, const struct msghdr *msg, k_timeout_t timeout)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	uint16_t dst_port = 0U;
	int ret, sock_written = 0;
	char buf[sizeof("AT+KUDPSND=##,\"" MODEM_HL78XX_ADDRESS_FAMILY_FORMAT "\",#####,####")];
	struct sockaddr *dst_addr = msg->msg_name;
	size_t buf_len = 0;

	if (!sock) {
		return -EINVAL;
	}
	for (int i = 0; i < msg->msg_iovlen; i++) {
		if (!msg->msg_iov[i].iov_base || msg->msg_iov[i].iov_len == 0) {
			errno = EINVAL;
			return -1;
		}
		buf_len += msg->msg_iov[i].iov_len;
	}

	if (!sock->is_connected && sock->ip_proto != IPPROTO_UDP) {
		errno = ENOTCONN;
		return -1;
	}

	if (!dst_addr && sock->ip_proto == IPPROTO_UDP) {
		dst_addr = &sock->dst;
	}
	/*
	 * Binary and ASCII mode allows sending MDM_MAX_DATA_LENGTH bytes to
	 * the socket in one command
	 */
	if (buf_len > MDM_MAX_DATA_LENGTH) {
		if (sock->type == SOCK_DGRAM) {
			errno = EMSGSIZE;
			return -1;
		}
		buf_len = MDM_MAX_DATA_LENGTH;
	}
	/* The number of bytes written will be reported by the modem */

	if (sock->ip_proto == IPPROTO_UDP) {
		char ip_str[NET_IPV6_ADDR_LEN];

		ret = modem_context_sprint_ip_addr(dst_addr, ip_str, sizeof(ip_str));
		if (ret != 0) {
			LOG_ERR("Error formatting IP string %d", ret);
			goto exit;
		}

		ret = modem_context_get_addr_port(dst_addr, &dst_port);
		if (ret != 0) {
			LOG_ERR("Error getting port from IP address %d", ret);
			goto exit;
		}

		snprintk(buf, sizeof(buf), "AT+KUDPSND=%d,\"%s\",%u,%zu", sock->id, ip_str,
			 dst_port, buf_len);
	} else {
		snprintk(buf, sizeof(buf), "AT+KTCPSND=%d,%zu", sock->id, buf_len);
	}
	ret = k_mutex_lock(&socket_data.mdata_global->tx_lock, K_SECONDS(1));
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, buf, strlen(buf), connect_matches,
				 ARRAY_SIZE(connect_matches));
	if (ret) {
		LOG_ERR("Error sending data %d", ret);
		goto exit;
	}

	modem_pipe_attach(socket_data.mdata_global->chat.pipe, modem_chat_pipe_callback,
			  &socket_data.mdata_global->chat);
	/* bytes written to socket in last transaction */
	/* wait for CONNECT  or error */
	/* Send data directly on modem iface */
	for (int i = 0; i < msg->msg_iovlen; i++) {
		int len = MIN(buf_len, msg->msg_iov[i].iov_len);

		if (len == 0) {
			LOG_DBG("No data to send, skipping iov %d", i);
			continue;
		}

		uint32_t offset = 0;

		while (len > 0) {
			LOG_DBG("Sending %d bytes", len);

			ret = k_sem_take(&socket_data.mdata_global->script_stopped_sem_tx_int,
					 K_FOREVER);
			if (ret < 0) {
				goto exit;
			}
			ret = modem_pipe_transmit(socket_data.mdata_global->uart_pipe,
						  ((uint8_t *)msg->msg_iov[i].iov_base) + offset,
						  len);
			if (ret < 0) {
				LOG_ERR("%d Error writing to modem pipe %d", __LINE__, ret);
				break;
			}
			if (ret == 0) {
				LOG_ERR("Transmit returned 0 bytes; breaking to avoid infinite "
					"loop");
				break;
			}
			offset += ret;
			len -= ret;
			buf_len -= ret;
			sock_written += ret;
		}
	}
	ret = k_sem_take(&socket_data.mdata_global->script_stopped_sem_tx_int, K_FOREVER);
	if (ret < 0) {
		goto exit;
	}
	ret = modem_pipe_transmit(socket_data.mdata_global->uart_pipe,
				  (uint8_t *)socket_data.mdata_global->chat_eof_pattern,
				  socket_data.mdata_global->chat_eof_pattern_size);
	if (ret < 0) {
		LOG_ERR("%d Error writing to modem pipe %d", __LINE__, ret);
	}

	modem_chat_attach(&socket_data.mdata_global->chat, socket_data.mdata_global->uart_pipe);
	ret = modem_cmd_send_int(socket_data.mdata_global, NULL, "", 0, &ok_match, 1);
	if (ret) {
		LOG_ERR("Error sending data %d", ret);
		goto exit;
	}

exit:
	ret = k_mutex_unlock(&socket_data.mdata_global->tx_lock);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	if (ret < 0) {
		return ret;
	}
	return sock_written;
}
static ssize_t offload_sendto(void *obj, const void *buf, size_t len, int flags,
			      const struct sockaddr *to, socklen_t tolen)
{
	int ret = 0;
	struct iovec msg_iov = {
		.iov_base = (void *)buf,
		.iov_len = len,
	};
	struct msghdr msg = {
		.msg_iovlen = 1,
		.msg_name = (struct sockaddr *)to,
		.msg_namelen = tolen,
		.msg_iov = &msg_iov,
	};

	if (!modem_cellular_is_registered(socket_data.mdata_global)) {
		LOG_ERR("Modem currently not attached to the network!");
		return -EAGAIN;
	}
	/* Do some sanity checks. */
	if (!buf || len == 0) {
		errno = EINVAL;
		return -1;
	}

	/* Only send up to MTU bytes. */
	if (len > MDM_MAX_DATA_LENGTH) {
		len = MDM_MAX_DATA_LENGTH;
	}

	ret = send_socket_data(obj, &msg, K_SECONDS(MDM_CMD_TIMEOUT));
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	errno = 0;
	return ret;
}
static int offload_ioctl(void *obj, unsigned int request, va_list args)
{
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

		return modem_socket_poll_prepare(&socket_data.socket_config, obj, pfd, pev,
						 pev_end);
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
		int removed = 0, i = 0, bkp_iovec_idx = -1;

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

		ret = send_socket_data(obj, &crafted_msg, K_SECONDS(MDM_CMD_TIMEOUT));

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

void modem_hl78xx_socket_init(struct modem_hl78xx_data *data)
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
	net_if_set_link_addr(iface, modem_get_mac(socket_data.mac_addr), sizeof(data->mac_addr),
				NET_LINK_ETHERNET);
	data->net_iface = iface;
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

#define MODEM_HL78XX_DEFINE_INSTANCE(inst)                                                         \
                                                                                                   \
	NET_DEVICE_OFFLOAD_INIT(inst, "hl78xx_dev", hl78xx_init_sockets, NULL, &socket_data, NULL, \
				CONFIG_MODEM_HL78XX_OFFLOAD_INIT_PRIORITY, &api_funcs,             \
				MDM_MAX_DATA_LENGTH);                                              \
                                                                                                   \
	NET_SOCKET_OFFLOAD_REGISTER(inst, CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY, AF_UNSPEC,          \
				    offload_is_supported, offload_socket);

#define MODEM_DEVICE_SWIR_HL78XX(inst) MODEM_HL78XX_DEFINE_INSTANCE(inst)

#define DT_DRV_COMPAT swir_hl7812
DT_INST_FOREACH_STATUS_OKAY(MODEM_DEVICE_SWIR_HL78XX)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT swir_hl7800
DT_INST_FOREACH_STATUS_OKAY(MODEM_DEVICE_SWIR_HL78XX)
#undef DT_DRV_COMPAT
