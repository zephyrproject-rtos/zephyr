/**
 * Copyright (c) 2019 Manulytica Ltd
 * Copyright (c) 2018 Linaro, Lmtd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <logging/log.h>
LOG_MODULE_REGISTER(wifi_esp32, CONFIG_WIFI_LOG_LEVEL);

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <gpio.h>
#include <net/net_if.h>
#include <net/net_offload.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_context.h>
#include <net/net_ip.h>
#include <net/wifi_mgmt.h>

#include "ipv4.h"
#if defined(CONFIG_NET_UDP)
#include "udp_internal.h"
#endif

#include "../../modem/modem_receiver.h"

#define ESP32_MAX_CONNECTIONS 5
#define BUF_ALLOC_TIMEOUT K_SECONDS(1)
#define MDM_MAX_DATA_LENGTH     1600  /*  Suggesting 1024 ? */
#define MDM_RECV_MAX_BUF                32
#define MDM_RECV_BUF_SIZE               128

#define MDM_PIN_ENABLE          0x1
#define MDM_PIN_DISABLE         0x0

#define MDM_MAX_RETRIES 5

struct cmd_handler {
	const char *cmd;
	u16_t cmd_len;
	void (*func)(struct net_buf **buf, u16_t len);
	bool skip;
};

#define CMD_HANDLER_NO_SKIP(cmd_, cb_) {	    \
		.cmd = cmd_,			    \
		.cmd_len = (u16_t)sizeof(cmd_) - 1, \
		.func = on_cmd_ ## cb_,		    \
		.skip = false			    \
}

#define CMD_HANDLER(cmd_, cb_) {		    \
		.cmd = cmd_,			    \
		.cmd_len = (u16_t)sizeof(cmd_) - 1, \
		.func = on_cmd_ ## cb_,		    \
		.skip = true			    \
}

static u8_t mdm_recv_buf[MDM_MAX_DATA_LENGTH];

/* net bufs */
NET_BUF_POOL_DEFINE(esp32_recv_pool, 12, 128, 0, NULL);
static struct k_work_q esp32_workq;

/* RX thread structures */
K_THREAD_STACK_DEFINE(esp32_rx_stack, 1024);

/* RX thread work queue */
K_THREAD_STACK_DEFINE(esp32_workq_stack, 1024);

struct k_thread esp32_rx_thread;

K_SEM_DEFINE(sock_sem, 1, 1);
K_MUTEX_DEFINE(dev_mutex);

struct socket_data {
	struct net_context      *context;
	sa_family_t family;
	enum net_sock_type type;
	enum net_ip_protocol ip_proto;
	net_context_send_cb_t send_cb;
	net_context_recv_cb_t recv_cb;


	struct k_work recv_cb_work;
	void *recv_user_data;

	struct sockaddr src;
	struct sockaddr dst;
	struct net_pkt          *rx_pkt;
	int ret;
	int conn_id;
	/** semaphore */
	int connected;
	int read_error;
};

struct conn_data {
	struct socket_data *sock;
};

struct esp32_data {
	struct net_if *iface;
	struct in_addr ip;
	struct in_addr gw;
	struct in_addr netmask;
	u8_t mac[6];

	/* modem stuff */
	struct mdm_receiver_context mdm_ctx;
	int last_error;

	struct k_delayed_work ip_addr_work;

	/* semaphores */
	struct k_sem response_sem;

	/* max sockets */
	struct socket_data socket_data[ESP32_MAX_CONNECTIONS];
	u8_t sock_map;
	struct conn_data conn_data[ESP32_MAX_CONNECTIONS];
	u8_t conn_map;

	/* variables required by tcp server */
	net_tcp_accept_cb_t accept_cb;
	void *accept_data;
	struct socket_data server_sock;

	/* wifi scan callbacks */
	scan_result_cb_t wifi_scan_cb;

	int sending_data;

	int data_id;
	int data_len;

	/* device management */
	struct device *uart_dev;
#ifdef CONFIG_WIFI_ESP32_HAS_ENABLE_PIN
	struct device *gpio_dev;
#endif
};

/**
 * get_token()
 * Function breaks a string into a sequence of zero or more
 * nonempty tokens.
 * On the first call to get_token() func the string to be
 * parsed should be specified in psrc.
 * In each subsequent call that should parse the same string,
 * psrc must be NULL.
 * The delimit argument specifies  parsed string for tokens.
 * Functions return a pointer to the next token,
 * or NULL if there are no more tokens.
 **/
char *get_token(char *psrc, const char delimit)
{
	static char sret[512];
	static char psave[512];

	register char *ptr = psave;

	memset(sret, 0, sizeof(sret));

	if (psrc != NULL) {
		strcpy(ptr, psrc);
	}
	if (ptr == NULL) {
		return NULL;
	}
	if (!delimit) {
		return NULL;
	}

	int i = 0, nlength = strlen(ptr);

	for (i = 0; i < nlength; i++) {
		if (ptr[i] == delimit) {
			break;
		}
		if (ptr[i] == '\0') {
			ptr = NULL;
			break;
		}
		sret[i] = ptr[i];
	}
	if (ptr != NULL) {
		strcpy(ptr, &ptr[i + 1]);
	}

	return sret;
}





static struct esp32_data mdm_data;

static void sockreadrecv_cb_work(struct k_work *work);

static struct socket_data *allocate_socket_data(void)
{
	int i;

	k_sem_take(&sock_sem, K_FOREVER);
	for (i = 0; i < ESP32_MAX_CONNECTIONS &&
	     mdm_data.sock_map & BIT(i); i++) {
		;
	}
	if (i >= ESP32_MAX_CONNECTIONS) {
		k_sem_give(&sock_sem);
		return NULL;
	}

	mdm_data.sock_map |= 1 << i;
	k_sem_give(&sock_sem);

	return &mdm_data.socket_data[i];
}

static void free_socket_data(struct socket_data *sock)
{
	int i;

	if (sock) {
		k_sem_take(&sock_sem, K_FOREVER);
		i = sock - mdm_data.socket_data;
		mdm_data.sock_map &= ~(1 << i);
		memset(sock, 0, sizeof(struct socket_data));
		k_sem_give(&sock_sem);
	}
}

static int allocate_conn_id(void)
{
	int i;

	k_sem_take(&sock_sem, K_FOREVER);
	for (i = 0; i < ESP32_MAX_CONNECTIONS &&
	     mdm_data.conn_map & BIT(i); i++) {
		;
	}
	if (i >= ESP32_MAX_CONNECTIONS) {
		k_sem_give(&sock_sem);
		return -ENOMEM;
	}

	mdm_data.conn_map |= 1 << i;
	k_sem_give(&sock_sem);

	return i;
}

static void free_conn_id(unsigned int id)
{
	k_sem_take(&sock_sem, K_FOREVER);
	mdm_data.conn_map &= ~(1 << id);
	mdm_data.conn_data[id].sock = NULL;
	k_sem_give(&sock_sem);
}

static void set_conn_map(unsigned int id)
{
	k_sem_take(&sock_sem, K_FOREVER);
	mdm_data.conn_map |= 1 << id;
	k_sem_give(&sock_sem);
}

static void esp32_context_put(struct net_context *context)
{
	/* need to do this?
	 * zsock_flush_queue(context);
	 */
	context->connect_cb = NULL;
	context->recv_cb = NULL;
	context->send_cb = NULL;
	net_context_unref(context);
}

static inline struct socket_data *get_socket_from_conn_id(unsigned int id)
{
	return mdm_data.conn_data[id].sock;
}

static inline void link_conn_id_to_socket_data(int id,
					       struct socket_data *sock)
{
	mdm_data.conn_data[id].sock = sock;
	sock->conn_id = id;
}

#define MDM_CMD_TIMEOUT         K_SECONDS(3)

/* Send an AT command with a series of response handlers */
static int send_at_cmd(struct socket_data *sock,
		       const u8_t *data, int timeout)
{
	int ret;

	mdm_data.last_error = 0;

	k_mutex_lock(&dev_mutex, K_FOREVER);
	k_sem_reset(&mdm_data.response_sem);
	mdm_receiver_send(&mdm_data.mdm_ctx, data, strlen(data));
	mdm_receiver_send(&mdm_data.mdm_ctx, "\r\n", 2);
	k_mutex_unlock(&dev_mutex);

	if (timeout == K_NO_WAIT) {
		return 0U;
	}

	/* wait for cmd response */
	ret = k_sem_take(&mdm_data.response_sem, timeout);

	if (ret == 0) {
		ret = mdm_data.last_error;
	} else if (ret == -EAGAIN) {
		ret = -ETIMEDOUT;
	}

	return ret;
}

static int net_buf_find_next_delimiter(struct net_buf *buf, const u8_t d,
				       size_t index)
{
	struct net_buf *frag = buf;
	u16_t offset = 0;
	u16_t pos = 0;

	/* locate the start index */
	while (frag && index) {
		if (frag->len > index) {
			offset += index;
			pos += index;
			index = 0;
		} else {
			index -= frag->len;
			pos += frag->len;
			offset = 0;
			if (!frag->frags) {
				return -EINVAL;
			}
			frag = frag->frags;
		}
	}

	while (*(frag->data + offset) != d) {
		if (offset == frag->len) {
			if (!frag->frags) {
				return -EINVAL;
			}
			frag = frag->frags;
			offset = 0;
		} else {
			offset++;
		}
		pos++;
	}

	return pos;
}

static int net_buf_ncmp(struct net_buf *buf, const u8_t *s2, size_t n)
{
	struct net_buf *frag = buf;
	u16_t offset = 0;

	while ((n > 0) && (*(frag->data + offset) == *s2) && (*s2 != '\0')) {
		if (offset == frag->len) {
			if (!frag->frags) {
				break;
			}
			frag = frag->frags;
			offset = 0;
		} else {
			offset++;
		}

		s2++;
		n--;
	}

	return (n == 0) ? 0 : (*(frag->data + offset) - *s2);
}

/* Echo Handler for commands without related sockets */
static void on_cmd_atcmdecho_nosock(struct net_buf **buf, u16_t len)
{
}

static void on_cmd_esp32_ready(struct net_buf **buf, u16_t len)
{
	k_sem_give(&mdm_data.response_sem);
}

static void esp32_ip_addr_work(struct k_work *work)
{
	int ret;

	ret = send_at_cmd(NULL, "AT+CIPSTA?", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("failed to get ip address information");
		return;
	}

	/* update interface addresses */
	/* Currently netmask and gateway cause hard crash !!! */
	net_if_ipv4_addr_add(mdm_data.iface, &mdm_data.ip, NET_ADDR_DHCP, 0);
	/* net_if_ipv4_set_gw(mdm_data.iface, &mdm_data.gw); */
	/* net_if_ipv4_set_netmask(mdm_data.iface, &mdm_data.netmask); */
}

static void on_cmd_ip_addr_get(struct net_buf **buf, u16_t len)
{
	k_delayed_work_submit_to_queue(&esp32_workq,
				       &mdm_data.ip_addr_work, K_SECONDS(1));
}

static void on_cmd_conn_open(struct net_buf **buf, u16_t len)
{
	int id, ret;
	u8_t msg[30];
	struct socket_data *sock;

	id = *((*buf)->data) - '0';

	sock = get_socket_from_conn_id(id);

	if (sock == NULL) {
		struct net_context *context = NULL;

		ret = net_context_get(AF_INET, SOCK_STREAM,
				      IPPROTO_TCP, &context);

		if (ret < 0) {
			LOG_ERR("Cannot get new net context for ACCEPT");

			len = snprintk(msg, 30, "AT+CIPCLOSE=%d", id);

			if (send_at_cmd(&mdm_data.socket_data[id], msg,
					MDM_CMD_TIMEOUT)) {
				LOG_ERR("failed to close");
			}
		} else {
			/* need to fill dst addr info which will not be reported
			 * by esp32 in <connect_id>, CONNECT message. Here
			 * we use gate address as dst address.
			 */
			net_context_bind(context,
					 &mdm_data.server_sock.src,
					 sizeof(struct sockaddr));
			net_ipaddr_copy(&net_sin(&context->remote)->sin_addr,
					&mdm_data.gw);
			context->remote.sa_family = AF_INET;
			net_sin(&context->remote)->sin_port = 1000 + id;
			context->flags |= NET_CONTEXT_REMOTE_ADDR_SET;
			sock = (struct socket_data *)context->offload_context;
			sock->connected = 1;
			set_conn_map(id);
			link_conn_id_to_socket_data(id, sock);
		}

		if (mdm_data.accept_cb) {
			mdm_data.accept_cb(context,
					   &mdm_data.server_sock.src,
					   sizeof(struct sockaddr),
					   ret, mdm_data.accept_data);
		}
	} else {
		sock->connected = 1;
	}
}

static void on_cmd_conn_closed(struct net_buf **buf, u16_t len)
{
	int id;
	struct socket_data *sock = NULL;

	id = '0' - *((*buf)->data);

	sock = get_socket_from_conn_id(id);
	if (sock) {
		esp32_context_put(sock->context);
		free_socket_data(sock);
		free_conn_id(id);
	}
}

static void on_cmd_sent_bytes(struct net_buf **buf, u16_t len)
{
	char temp[MDM_RECV_MAX_BUF];

	/* Part of Tokenisation
	 *  char token[MDM_RECV_MAX_BUF];
	 *  char *tok;
	 */

	net_buf_linearize(temp, sizeof(temp), *buf, 0, len);
	temp[len] = '\0';

	LOG_HEXDUMP_DBG(temp, len, "Recv data");

	/** Originally tokenised buffer..?
	 * Util_strTok(temp, token, " ");
	 * while (token != NULL) {
	 *     Util_strTok(temp, token, " ");
	 *     LOG_DBG("on_cmd_sent_bytes: %d", len);
	 *  }
	 **/
}

static void on_cmd_wifi_scan_resp(struct net_buf **buf, u16_t len)
{
	int i = 0;
	char temp[MDM_RECV_MAX_BUF];
	struct wifi_scan_result result;
	int delimiters[6];
	int slen;

	/* ecn, ssid, rssi, mac, channel */
	delimiters[0] = 1;
	for (i = 1; i < sizeof(delimiters) / sizeof(int); i++) {
		delimiters[i] = net_buf_find_next_delimiter(
			*buf, ',', delimiters[i - 1] + 1);
		if (i < 5 && delimiters[i] == -1) {
			LOG_DBG("Not enough delimiters found: %d: %d", i,
				sizeof(delimiters) / sizeof(int));
			return;
		}

		delimiters[i]++;
	}

	/* ecn */
	slen = delimiters[1] - delimiters[0] - 1;
	net_buf_linearize(temp, 1, *buf, delimiters[0], slen);
	if (temp[0] != '0') {
		result.security = WIFI_SECURITY_TYPE_PSK;
	} else {
		result.security = WIFI_SECURITY_TYPE_NONE;
	}
/*
 *   temp[slen] = '\0';
 *   result.security = strtol(temp, NULL, 10);
 *   LOG_DBG("ecn: %d: %d", atoi(temp), result.security);
 */

	/* ssid */
	slen = delimiters[2] - delimiters[1] - 3;
	net_buf_linearize(result.ssid, sizeof(temp), *buf, delimiters[1] + 1,
			  slen);
	result.ssid_length = slen;

/**
 * char ssidstr[WIFI_SSID_MAX_LEN];
 * memcpy(ssidstr, result.ssid, slen);
 * ssidstr[slen] = '\0';
 * LOG_DBG("ssid: %s", ssidstr);
 * k_sleep(K_MSEC(50));
 */

	/* rssi */
	slen = delimiters[3] - delimiters[2];
	net_buf_linearize(temp, sizeof(temp), *buf, delimiters[2], slen);
	temp[slen] = '\0';
	result.rssi = strtol(temp, NULL, 10);

/**
 *    LOG_DBG("rssi: %d", result.rssi);
 */

	/* channel */
	slen = delimiters[5] - delimiters[4];
	net_buf_linearize(temp, sizeof(temp), *buf, delimiters[4], slen);
	temp[slen] = '\0';
	result.channel = strtol(temp, NULL, 10);

/**
 *    LOG_DBG("channel: %d", result.channel);
 */

	/* issue callback to report scan results */
	if (mdm_data.wifi_scan_cb) {
		mdm_data.wifi_scan_cb(mdm_data.iface, 0, &result);
	}
}

static const char nm_label[] = "netmask";
static const char gw_label[] = "gateway";
static const char ip_label[] = "ip";

static void on_cmd_ip_addr_resp(struct net_buf **buf, u16_t len)
{
	char ip_addr[sizeof("xxx.xxx.xxx.xxx")];
	int d[3];
	int slen;

	d[0] = net_buf_find_next_delimiter(*buf, ':', 0);
	d[1] = net_buf_find_next_delimiter(*buf, '\"', d[0] + 1);
	d[2] = net_buf_find_next_delimiter(*buf, '\"', d[1] + 1);

	slen = d[2] - d[1] - 1;

/**
 *    LOG_DBG("on_cmd_ip_addr_resp slen: %d", slen);
 */

	net_buf_linearize(ip_addr, 16, *buf, d[0] + 2, slen);
	ip_addr[slen] = '\0';

	/* update interface addresses */
	if (net_buf_ncmp(*buf, nm_label, strlen(nm_label)) == 0) {
		net_addr_pton(AF_INET, ip_addr, &mdm_data.netmask);
	} else if (net_buf_ncmp(*buf, ip_label, strlen(ip_label)) == 0) {
		net_addr_pton(AF_INET, ip_addr, &mdm_data.ip);
	} else if (net_buf_ncmp(*buf, gw_label, strlen(gw_label)) == 0) {
		net_addr_pton(AF_INET, ip_addr, &mdm_data.gw);
	} else {
		LOG_DBG("ERROR: No IP Data (Len:%d)", slen);
		return;
	}
}

static void on_cmd_mac_addr_resp(struct net_buf **buf, u16_t len)
{
	int i;
	char octet[sizeof("xx")];

	for (i = 0; i < 6; i++) {
		net_buf_pull_u8(*buf);
		octet[0] = net_buf_pull_u8(*buf);
		octet[1] = net_buf_pull_u8(*buf);
		hex2bin(octet, 2, &mdm_data.mac[i], 1);
	}
}

static void on_cmd_sendok(struct net_buf **buf, u16_t len)
{
	mdm_data.last_error = 0;

	/* only signal semaphore if we are not waiting on a data send command */
	k_sem_give(&mdm_data.response_sem);
}

/* Handler: OK */
static void on_cmd_sockok(struct net_buf **buf, u16_t len)
{
	mdm_data.last_error = 0;

	if (!mdm_data.sending_data) {
		k_sem_give(&mdm_data.response_sem);
	}
}

/* Handler: ERROR */
static void on_cmd_sockerror(struct net_buf **buf, u16_t len)
{
	mdm_data.last_error = -EIO;
	k_sem_give(&mdm_data.response_sem);
}

static void on_cmd_wifi_connected_resp(struct net_buf **buf, u16_t len)
{
	wifi_mgmt_raise_connect_result_event(mdm_data.iface, 0);
}

static void on_cmd_wifi_disconnected_resp(struct net_buf **buf, u16_t len)
{
	wifi_mgmt_raise_disconnect_result_event(mdm_data.iface, 0);
}

static int esp32_get(sa_family_t family,
		     enum net_sock_type type,
		     enum net_ip_protocol ip_proto,
		     struct net_context **context)
{
	struct net_context *c;
	struct socket_data *sock;
	int err = 0U;

	k_mutex_lock(&dev_mutex, K_FOREVER);


	if (family != AF_INET) {
		err = -EBUSY;
		goto unlock;
	}

	c = *context;
	sock = allocate_socket_data();
	if (!sock) {
		err = -EBUSY;
		goto unlock;
	}

	c->offload_context = (void *)sock;
	sock->context = c;
	sock->family = family;
	sock->type = type;
	sock->ip_proto = ip_proto;
	sock->connected = 0;
	k_work_init(&sock->recv_cb_work, sockreadrecv_cb_work);

unlock:
	k_mutex_unlock(&dev_mutex);
	return err;
}

static const char type_tcp[] = "TCP";
static const char type_udp[] = "UDP";

static int esp32_connect(struct net_context *context,
			 const struct sockaddr *addr,
			 socklen_t addrlen,
			 net_context_connect_cb_t cb,
			 s32_t timeout,
			 void *user_data)
{
	char connect_msg[100];
	int len;
	const char *type;
	struct socket_data *sock;
	char addr_str[32];
	int ret = -EFAULT;
	unsigned int conn_id;

	if (!context || !addr) {
		return -EINVAL;
	}

	sock = (struct socket_data *)context->offload_context;
	if (!sock) {
		LOG_ERR("Can't find socket info for ctx: %p", context);
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && addr->sa_family == AF_INET) {
		net_ipaddr_copy(&net_sin(&sock->dst)->sin_addr,
				&net_sin(addr)->sin_addr);
		net_sin(&sock->dst)->sin_port = net_sin(addr)->sin_port;
		sock->dst.sa_family = AF_INET;
	} else {
		return -EPFNOSUPPORT;
	}

	if (net_sin(&sock->dst)->sin_port < 0) {
		LOG_ERR("invalid port: %d",
			net_sin(&sock->dst)->sin_port);
		return -EINVAL;
	}

	if (net_context_get_type(context) == SOCK_STREAM) {
		type = type_tcp;
	} else {
		type = type_udp;
	}

	conn_id = allocate_conn_id();

	if (conn_id < 0) {
		return -EINVAL;
	}

	if (type == type_tcp) {
		net_addr_ntop(sock->dst.sa_family,
			      &net_sin(&sock->dst)->sin_addr,
			      addr_str, sizeof(addr_str));
		len = snprintk(connect_msg, 100,
			       "AT+CIPSTART=%d,\"%s\",\"%s\",%d,7200",
			       conn_id, type, addr_str,
			       ntohs(net_sin(&sock->dst)->sin_port));
	} else {
		int src_port = ntohs(net_sin(&sock->src)->sin_port);

		if (src_port == 0) {
			src_port = ntohs(net_sin(&sock->dst)->sin_port);
		}
		net_addr_ntop(sock->dst.sa_family,
			      &net_sin(&sock->dst)->sin_addr,
			      addr_str, sizeof(addr_str));
		len = snprintk(connect_msg, 100,
			       "AT+CIPSTART=%d,\"%s\",\"%s\",%d,%d,2",
			       conn_id, type, addr_str,
			       ntohs(net_sin(&sock->dst)->sin_port),
			       src_port);
	}

/**
 *    LOG_DBG("esp32_connect:OUTPUT: [%s]", connect_msg);
 */
	link_conn_id_to_socket_data(conn_id, sock);
	ret = send_at_cmd(sock, connect_msg, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Failed to send connect");
		free_conn_id(conn_id);
		ret = -EINVAL;
	}

	if (cb) {
		cb(context, ret, user_data);
	}

	return 0U;
}

static int esp32_bind(struct net_context *context,
		      const struct sockaddr *addr,
		      socklen_t addrlen)
{
	struct socket_data *sock = NULL;

	if (!context) {
		return -EINVAL;
	}

	sock = (struct socket_data *)context->offload_context;
	if (!sock) {
		LOG_ERR("Missing socket for ctx: %p", context);
		return -EINVAL;
	}

	sock->src.sa_family = addr->sa_family;

	if (IS_ENABLED(CONFIG_NET_IPV4) && addr->sa_family == AF_INET) {
		net_ipaddr_copy(&net_sin(&sock->src)->sin_addr,
				&net_sin(addr)->sin_addr);
		net_sin(&sock->src)->sin_port = net_sin(addr)->sin_port;
		net_sin_ptr(&context->local)->sin_addr = &mdm_data.ip;
		net_sin_ptr(&context->local)->sin_port =
			net_sin(addr)->sin_port;
		context->local.family = AF_INET;
	} else {
		return -EPFNOSUPPORT;
	}

	return 0U;
}

static int esp32_listen(struct net_context *context, int backlog)
{
	struct socket_data *sock = NULL;
	char listen_msg[50];
	int len;
	int ret;

	if (!context) {
		return -EINVAL;
	}

	/* esp32 only supports one server to listen */
	if (mdm_data.server_sock.context) {
		return -EINVAL;
	}

	sock = (struct socket_data *)context->offload_context;

	if (!sock) {
		LOG_ERR("Missing socket for ctx: %p", context);
		return -EINVAL;
	}

	if (backlog > ESP32_MAX_CONNECTIONS) {
		backlog = ESP32_MAX_CONNECTIONS;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && sock->src.sa_family == AF_INET &&
	    net_context_get_type(context) == SOCK_STREAM) {

		len = snprintk(listen_msg, 50,
			       "AT+CIPSERVERMAXCONN=%d", backlog);

		send_at_cmd(sock, listen_msg, MDM_CMD_TIMEOUT);

		len = snprintk(listen_msg, 50, "AT+CIPSERVER=1,%d",
			       ntohs(net_sin(&sock->src)->sin_port));

		ret = send_at_cmd(sock, listen_msg, MDM_CMD_TIMEOUT);

		if (ret < 0) {
			LOG_ERR("failed to send AT+CIPSERVER=1");
			return -EINVAL;
		}

		/* use server sock to trace and free the allocated sock */
		memcpy(&mdm_data.server_sock, sock,
		       sizeof(struct socket_data));
		memset(sock, 0, sizeof(struct socket_data));
		context->offload_context = &mdm_data.server_sock;
		mdm_data.sock_map &= ~(1 << (sock - mdm_data.socket_data));

		return 0U;
	}

	return -EPFNOSUPPORT;
}

static int esp32_accept(struct net_context *context,
			net_tcp_accept_cb_t cb,
			s32_t timeout,
			void *user_data)
{
	struct socket_data *sock = NULL;

	if (!context) {
		return -EINVAL;
	}

	sock = (struct socket_data *)context->offload_context;

	if (!sock) {
		LOG_ERR("Missing socket for ctx: %p", context);
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		mdm_data.accept_cb = cb;
		mdm_data.accept_data = user_data;
		return 0U;
	} else {
		return -EPFNOSUPPORT;
	}
}

static int esp32_sendto(struct net_pkt *pkt,
			const struct sockaddr *dst_addr,
			socklen_t addrlen,
			net_context_send_cb_t cb,
			s32_t timeout,
			void *user_data)
{
	u8_t send_msg[80];
	struct net_context *context = net_pkt_context(pkt);
	struct socket_data *data;
	struct net_buf *frag;
	int len;
	int ret = -EFAULT;
	int send_len;
	u8_t addr_str[32];
	int id;

	if (!context || !context->offload_context) {
		return -EINVAL;
	}

	data = context->offload_context;

	if (!data->connected) {
		ret = esp32_connect(data->context,
				    dst_addr,
				    addrlen,
				    NULL,
				    0, NULL);

		if (ret < 0) {
			return ret;
		}
	}

	id = data->conn_id;

	frag = pkt->frags;
	send_len = net_buf_frags_len(frag);
	mdm_data.sending_data = 1;
	if (net_context_get_type(context) == SOCK_STREAM) {
		len = snprintk(send_msg, 80, "AT+CIPSEND=%d,%d", id,
			       net_buf_frags_len(frag));
	} else {
		net_addr_ntop(data->dst.sa_family,
			      &net_sin(&data->dst)->sin_addr,
			      addr_str, sizeof(addr_str));
		len = snprintk(send_msg, 80, "AT+CIPSEND=%d,%d,\"%s\",%d",
			       id, net_buf_frags_len(frag), addr_str,
			       ntohs(net_sin(&data->dst)->sin_port));
	}

	ret = send_at_cmd(NULL, send_msg, MDM_CMD_TIMEOUT * 2);
	if (ret < 0) {
		ret = -EINVAL;
		goto ret_early;
	}

	mdm_data.sending_data = 0;
	k_sem_reset(&mdm_data.response_sem);
	k_mutex_lock(&dev_mutex, K_FOREVER);
	while (send_len && frag) {
		if (frag->len > send_len) {
			mdm_receiver_send(&mdm_data.mdm_ctx, frag->data,
					  send_len);
			send_len = 0;
		} else {
			mdm_receiver_send(&mdm_data.mdm_ctx, frag->data,
					  frag->len);
			send_len -= frag->len;
			frag = frag->frags;
		}
	}
	k_mutex_unlock(&dev_mutex);

	ret = k_sem_take(&mdm_data.response_sem, MDM_CMD_TIMEOUT * 2);

	if (ret == 0) {
		ret = mdm_data.last_error;
	} else if (ret == -EAGAIN) {
		ret = -ETIMEDOUT;
	}

ret_early:
	net_pkt_unref(pkt);
	if (cb) {
		cb(context, ret, user_data);
	}

	return ret;
}

static int esp32_put(struct net_context *context)
{
	struct socket_data *data;
	u8_t msg[30];
	int len;

	if (!context || !context->offload_context) {
		return -EINVAL;
	}

	data = context->offload_context;

	/* close server sock */
	if (data == &mdm_data.server_sock) {
		len = snprintk(msg, 30, "AT+CIPSERVER=0");
		msg[len] = '\0';

		if (send_at_cmd(&mdm_data.server_sock, msg,
				MDM_CMD_TIMEOUT)) {
			LOG_ERR("failed to close server");
		}
		mdm_data.accept_data = NULL;
		mdm_data.accept_cb = NULL;
		memset(&mdm_data.server_sock, 0,
		       sizeof(struct socket_data));
		esp32_context_put(context);
		return 0U;
	}

	if (data->connected) {
		len = snprintk(msg, 30, "AT+CIPCLOSE=%d", data->conn_id);
		msg[len] = '\0';

		if (send_at_cmd(data, msg,
				MDM_CMD_TIMEOUT)) {
			LOG_ERR("failed to close");
			free_conn_id(data->conn_id);
			free_socket_data(data);
			esp32_context_put(context);
		}
	}

	return 0U;
}

static int esp32_send(struct net_pkt *pkt,
		      net_context_send_cb_t cb,
		      s32_t timeout,
		      void *user_data)
{
	struct net_context *context = net_pkt_context(pkt);
	socklen_t addrlen;
	int ret;

	addrlen = 0;
#if defined(CONFIG_NET_IPV6)
	if (net_pkt_family(pkt) == AF_INET6) {
		addrlen = sizeof(struct sockaddr_in6);
	} else
#endif /* CONFIG_NET_IPV6 */
#if defined(CONFIG_NET_IPV4)
	if (net_pkt_family(pkt) == AF_INET) {
		addrlen = sizeof(struct sockaddr_in);
	} else
#endif /* CONFIG_NET_IPV4 */
	{
		return -EPFNOSUPPORT;
	}

	ret = esp32_sendto(pkt, &context->remote, addrlen, cb,
			   timeout, user_data);
	return ret;
}

static int esp32_recv(struct net_context *context,
		      net_context_recv_cb_t cb,
		      s32_t timeout,
		      void *user_data)
{
	struct socket_data *data;

	if (!context || !context->offload_context) {
		return -EINVAL;
	}

	data = context->offload_context;

	data->recv_cb = cb;
	data->recv_user_data = user_data;

	/* listening on an UDP port */
	if (!data->connected && net_context_get_type(context) == SOCK_DGRAM) {
		struct sockaddr dst_addr;

		net_addr_pton(AF_INET, "0.0.0.0",
			      &(net_sin(&dst_addr)->sin_addr));
		net_sin(&dst_addr)->sin_port = net_sin(&data->src)->sin_port;
		esp32_connect(data->context,
			      &dst_addr,
			      sizeof(struct sockaddr),
			      NULL,
			      0, NULL);
	}

	return 0U;
}

static struct net_offload esp32_offload = {
	.get = esp32_get,
	.bind = esp32_bind,
	.listen = esp32_listen,
	.connect = esp32_connect,
	.accept = esp32_accept,
	.send = esp32_send,
	.sendto = esp32_sendto,
	.recv = esp32_recv,
	.put = esp32_put,
};

static int esp32_mgmt_scan(struct device *dev, scan_result_cb_t cb)
{
	int ret;

	mdm_data.wifi_scan_cb = cb;

	ret = send_at_cmd(NULL, "AT+CWLAP", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("failed to send scan");
		ret = -EINVAL;
	}

	mdm_data.wifi_scan_cb = NULL;
	return 0U;
};

static int esp32_mgmt_connect(struct device *dev,
			      struct wifi_connect_req_params *params)
{
	char connect_msg[100];
	int len;
	int ret;

	LOG_DBG("esp32_mgmt_connect");

	if (params->security == WIFI_SECURITY_TYPE_PSK) {
		len = snprintk(connect_msg, 100, "AT+CWJAP=\"");
		memcpy(&connect_msg[len], params->ssid,
		       params->ssid_length);
		len += params->ssid_length;
		len += snprintk(&connect_msg[len], 100 - len, "\",\"");
		memcpy(&connect_msg[len], params->psk,
		       params->psk_length);
		len += params->psk_length;
		len += snprintk(&connect_msg[len], 100 - len, "\"");
		connect_msg[len] = '\0';
	} else {
		len = snprintk(connect_msg, 100, "AT+CWJAP=\"");
		memcpy(&connect_msg[len], params->ssid,
		       params->ssid_length);
		len += params->ssid_length;
		len += snprintk(&connect_msg[len], 100 - len, "\"");
		connect_msg[len] = '\0';
	}

	ret = send_at_cmd(NULL, connect_msg, MDM_CMD_TIMEOUT * 2);
	if (ret < 0) {
		LOG_ERR("esp32_mgmt_scan: failed to send connect");
		return -EINVAL;
	}

	return 0U;
}

static int esp32_mgmt_disconnect(struct device *dev)
{
	int ret;

	ret = send_at_cmd(NULL, "AT+CWQAP", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("failed to query mac address");
		return -EFAULT;
	}

	return 0U;
}


static void esp32_iface_init(struct net_if *iface)
{
	atomic_clear_bit(iface->if_dev->flags, NET_IF_UP);

	net_if_set_link_addr(iface, mdm_data.mac,
			     sizeof(mdm_data.mac),
			     NET_LINK_ETHERNET);

	iface->if_dev->offload = &esp32_offload;
	mdm_data.iface = iface;

	atomic_set_bit(mdm_data.iface->if_dev->flags, NET_IF_UP);
}

static const struct net_wifi_mgmt_offload esp32_api = {
	.iface_api.init = esp32_iface_init,
	.scan = esp32_mgmt_scan,
	.connect = esp32_mgmt_connect,
	.disconnect = esp32_mgmt_disconnect,
};

static inline struct net_buf *read_rx_allocator(s32_t timeout, void *user_data)
{
	return net_buf_alloc((struct net_buf_pool *)user_data, timeout);
}

static void esp32_read_rx(struct net_buf **buf)
{
	u8_t uart_buffer[MDM_RECV_BUF_SIZE];
	size_t bytes_read = 0;
	int ret;
	u16_t rx_len;

	k_sleep(K_MSEC(10));
	/* read all of the data from mdm_receiver */
	while (true) {
		ret = mdm_receiver_recv(&mdm_data.mdm_ctx,
					uart_buffer,
					sizeof(uart_buffer),
					&bytes_read);

		if (ret < 0 || bytes_read == 0) {
			/* mdm_receiver buffer is empty */
			break;
		}

		LOG_HEXDUMP_DBG(uart_buffer, bytes_read, "raw data");

		/* make sure we have storage */
		if (!*buf) {
			*buf = net_buf_alloc(&esp32_recv_pool,
					     BUF_ALLOC_TIMEOUT);
			if (!*buf) {
				LOG_ERR("Can't allocate RX data! "
					"Skipping data!");
				break;
			}
		}

		rx_len = net_buf_append_bytes(*buf, bytes_read, uart_buffer,
					      BUF_ALLOC_TIMEOUT,
					      read_rx_allocator,
					      &esp32_recv_pool);
		if (rx_len < bytes_read) {
			LOG_ERR("Data was lost! read %u of %u!",
				rx_len, bytes_read);
		}
		k_sleep(K_MSEC(5));
	}
}

static bool is_crlf(u8_t c)
{
	if (c == '\n' || c == '\r') {
		return true;
	} else {
		return false;
	}
}

static void net_buf_skipcrlf(struct net_buf **buf)
{
	/* chop off any /n or /r */
	while (*buf && is_crlf(*(*buf)->data)) {
		net_buf_pull_u8(*buf);
		if (!(*buf)->len) {
			*buf = net_buf_frag_del(NULL, *buf);
		}
	}
}

static u16_t net_buf_findcrlf(struct net_buf *buf, struct net_buf **frag,
			      u16_t *offset)
{
	u16_t len = 0, pos = 0;

	while (buf && !is_crlf(*(buf->data + pos))) {
		if (pos + 1 >= buf->len) {
			len += buf->len;
			buf = buf->frags;
			pos = 0;
		} else {
			pos++;
		}
	}

	if (buf && is_crlf(*(buf->data + pos))) {
		len += pos;
		*offset = pos;
		*frag = buf;
		return len;
	}

	return 0U;
}

/* Setup IP header data to be used by some network applications.
 * While much is dummy data, some fields such as dst, port and family are
 * important.
 * Return the IP + protocol header length.
 */
static int net_pkt_setup_ip_data(struct net_pkt *pkt, struct socket_data *sock)
{
	int hdr_len = 0;
	u16_t src_port = 0, dst_port = 0;

	if (net_pkt_family(pkt) == AF_INET) {
		if (net_ipv4_create(
			    pkt,
			    &((struct sockaddr_in *)&sock->dst)->sin_addr,
			    &((struct sockaddr_in *)&sock->src)->sin_addr)) {
			return -EINVAL;
		}

		hdr_len = sizeof(struct net_ipv4_hdr);
	}


	if (IS_ENABLED(CONFIG_NET_UDP) && sock->ip_proto == IPPROTO_UDP) {
		if (net_udp_create(pkt, src_port, dst_port)) {
			return -EINVAL;
		}

		hdr_len += NET_UDPH_LEN;
	} else if (IS_ENABLED(CONFIG_NET_TCP) &&
		   sock->ip_proto == IPPROTO_TCP) {
		NET_PKT_DATA_ACCESS_DEFINE(tcp_access, struct net_tcp_hdr);
		struct net_tcp_hdr *tcp;

		tcp = (struct net_tcp_hdr *)net_pkt_get_data(pkt, &tcp_access);

		if (!tcp) {
			return -EINVAL;
		}

		(void)memset(tcp, 0, NET_TCPH_LEN);

		/* Setup TCP header */
		tcp->src_port = src_port;
		tcp->dst_port = dst_port;

		if (net_pkt_set_data(pkt, &tcp_access)) {
			return -EINVAL;
		}

		hdr_len += NET_TCPH_LEN;
	}

	return hdr_len;
}

static void esp32_read_data(struct net_buf **buf)
{
	struct net_buf *inc = *buf;
	struct socket_data *sock;
	int bytes_left = mdm_data.data_len;
	int hdr_len;

	sock = get_socket_from_conn_id(mdm_data.data_id);

	if (sock == NULL) {
		return;
	}

	sock->read_error = 0;
	sock->rx_pkt =  net_pkt_rx_alloc_with_buffer(
		net_context_get_iface(sock->context),
		bytes_left, sock->family, sock->ip_proto,
		BUF_ALLOC_TIMEOUT);

	if (!sock->rx_pkt) {
		sock->read_error = -ENOMEM;
		LOG_ERR("Failed to get net pkt");

		/* clear buf */
		*buf = net_buf_skip(*buf, mdm_data.data_len);
		mdm_data.data_len = 0;
		return;
	}

	/* set pkt data */
	net_pkt_set_context(sock->rx_pkt, sock->context);

	hdr_len = net_pkt_setup_ip_data(sock->rx_pkt, sock);

	while (inc && bytes_left) {
		if (inc->len > bytes_left) {
			if (net_pkt_write(sock->rx_pkt, inc->data,
					  bytes_left) < 0) {
				sock->read_error = -ENOMEM;
				break;
			}
			inc = net_buf_skip(inc, bytes_left);
			bytes_left = 0;
		} else {
			if (net_pkt_write(sock->rx_pkt, inc->data,
					  inc->len) < 0) {
				sock->read_error = -ENOMEM;
				break;
			}
			bytes_left -= inc->len;
			inc = net_buf_frag_del(NULL, inc);
		}
	}

	if (bytes_left) {
		/* error, need to skip left bytes */
		inc = net_buf_skip(inc, bytes_left);
	} else {
		net_pkt_cursor_init(sock->rx_pkt);
		net_pkt_set_overwrite(sock->rx_pkt, true);

		if (hdr_len > 0) {
			net_pkt_skip(sock->rx_pkt, hdr_len);
		}
	}

	/* update the buf pointer */
	*buf = inc;
	/* Let's do the callback processing in a different work queue in
	 * case the app takes a long time.
	 */
	k_work_submit_to_queue(&esp32_workq, &sock->recv_cb_work);
	mdm_data.data_len = 0;
}

static int match(struct net_buf *buf, const char *str)
{
	struct net_buf *frag = buf;
	int offset = 0, pos = 0;

	while (frag && frag->len && *str) {
		if (*(frag->data + offset) == *str) {
			str++;
			offset++;
			pos++;
			if (offset >= frag->len) {
				frag = frag->frags;
				offset = 0;
			}
		} else {
			pos = -1;
			break;
		}
	}

	return pos;
}

static int esp32_process_setup_read(struct net_buf **buf)
{
	struct net_buf *frag;
	u8_t temp[MDM_RECV_MAX_BUF];
	int hdr_len;
	int offset;
	char *comma;
	int found;
	int end;

	frag = *buf;
	end = 0;
	offset = 0;
	found = -1;
	while (frag) {
		temp[end] = *(frag->data + offset);
		if (temp[end] == ':') {
			found = end;
			break;
		}

		end++;
		offset++;
		if (offset >= frag->len) {
			frag = frag->frags;
			offset = 0;
			if (!frag) {
				break;
			}
		}
	}

	if (found < 0) {
		return found;
	}

	temp[found + 1] = '\0';
	hdr_len = found + 1;

/**
 * Set to Multiple Connections as default
 *   Multiple connections:
 *   (+CIPMUX=1)+IPD,<linkID>,<len>[,<remoteIP>,<remote	port>]:<data>
 *
 *   Single connection:
 *   (+CIPMUX=0)+IPD,<len>[,<remoteIP>,<remote	port>]:<data>
 **/
	comma = get_token(temp, ',');
	comma = get_token(NULL, ',');
	mdm_data.data_id = atoi(comma);
	comma = get_token(NULL, ',');
	mdm_data.data_len = atoi(comma);

	*buf = net_buf_skip(*buf, hdr_len);

	LOG_INF("MATCH +IPD (len:%u)", mdm_data.data_len);

	return 0U;
}

static void sockreadrecv_cb_work(struct k_work *work)
{
	struct socket_data *sock = NULL;
	struct net_pkt *pkt;

	sock = CONTAINER_OF(work, struct socket_data, recv_cb_work);

	/* return data */
	pkt = sock->rx_pkt;
	sock->rx_pkt = NULL;
	if (sock->recv_cb) {
		if (sock->read_error) {
			if (pkt) {
				net_pkt_unref(pkt);
			}
			sock->recv_cb(sock->context, NULL, NULL, NULL,
				      sock->read_error, sock->recv_user_data);
		} else {
			sock->recv_cb(sock->context, pkt, NULL, NULL, 0,
				      sock->recv_user_data);
		}

	} else {
		net_pkt_unref(pkt);
	}
}

/* RX thread */
static void esp32_rx(void)
{
	struct net_buf *rx_buf = NULL;
	struct net_buf *frag = NULL;
	int i;
	u16_t offset, len;

	static const struct cmd_handler handlers[] = {
		CMD_HANDLER("AT+RST", atcmdecho_nosock),
		CMD_HANDLER("ATE1", atcmdecho_nosock),
		CMD_HANDLER("OK", sockok),
		CMD_HANDLER("ERROR", sockerror),
		CMD_HANDLER("FAIL", sockerror),
		CMD_HANDLER("SEND FAIL", sockerror),
		CMD_HANDLER("WIFI GOT IP", ip_addr_get),
		CMD_HANDLER("AT+CWJAP?=", atcmdecho_nosock),
		CMD_HANDLER("WIFI CONNECTED", wifi_connected_resp),
		CMD_HANDLER("WIFI DISCONNECT", wifi_disconnected_resp),
		CMD_HANDLER("SEND OK", sendok),
		CMD_HANDLER("link is not valid", atcmdecho_nosock),
		CMD_HANDLER("busy p...", atcmdecho_nosock),
		CMD_HANDLER("busy s...", atcmdecho_nosock),
		CMD_HANDLER("ready", esp32_ready),
		CMD_HANDLER("AT+CIPAPMAC?", atcmdecho_nosock),
		CMD_HANDLER("+CIPAPMAC:", mac_addr_resp),
		/* Particle Firmware Command */
		CMD_HANDLER("+GETAC:", mac_addr_resp),

		CMD_HANDLER("AT+CIPSTA?", atcmdecho_nosock),
		CMD_HANDLER("+CIPSTA:", ip_addr_resp),
		CMD_HANDLER("AT+CWLAP", atcmdecho_nosock),
		CMD_HANDLER("+CWLAP:", wifi_scan_resp),
		CMD_HANDLER("AT+CIPMUX", atcmdecho_nosock),
		CMD_HANDLER("AT+CWQAP", atcmdecho_nosock),
		CMD_HANDLER("AT+CIPSEND=", atcmdecho_nosock),
		CMD_HANDLER_NO_SKIP("0,CONNECT", conn_open),
		CMD_HANDLER_NO_SKIP("1,CONNECT", conn_open),
		CMD_HANDLER_NO_SKIP("2,CONNECT", conn_open),
		CMD_HANDLER_NO_SKIP("3,CONNECT", conn_open),
		CMD_HANDLER_NO_SKIP("4,CONNECT", conn_open),
		CMD_HANDLER_NO_SKIP("0,CLOSED", conn_closed),
		CMD_HANDLER_NO_SKIP("1,CLOSED", conn_closed),
		CMD_HANDLER_NO_SKIP("2,CLOSED", conn_closed),
		CMD_HANDLER_NO_SKIP("3,CLOSED", conn_closed),
		CMD_HANDLER_NO_SKIP("4,CLOSED", conn_closed),
		CMD_HANDLER("Recv ", sent_bytes),
	};

	while (true) {
		k_sem_take(&mdm_data.mdm_ctx.rx_sem, K_FOREVER);

		esp32_read_rx(&rx_buf);

		while (rx_buf) {

			if (mdm_data.data_len) {
				if (net_buf_frags_len(rx_buf) >=
				    mdm_data.data_len) {
					esp32_read_data(&rx_buf);

					if (!rx_buf) {
						break;
					}
					continue;
				} else {
					break;
				}
			}

			net_buf_skipcrlf(&rx_buf);
			if (!rx_buf) {
				break;
			}

			/* check for incoming IPD data */
			if (match(rx_buf, "+IPD") >= 0) {
				if (esp32_process_setup_read(&rx_buf) < 0) {
					break;
				}

				if (!rx_buf) {
					break;
				}

				continue;
			}

			if (match(rx_buf, ">") >= 0) {
				rx_buf = net_buf_skip(rx_buf, 1);
				k_sem_give(&mdm_data.response_sem);
			}

			frag = NULL;
			len = net_buf_findcrlf(rx_buf, &frag, &offset);
			if (!frag) {
				break;
			}

			/* look for matching data handlers */
			i = -1;
			for (i = 0; i < ARRAY_SIZE(handlers); i++) {
				if (match(rx_buf, handlers[i].cmd) >= 0) {

					/* found a matching handler */
					LOG_INF("MATCH %s (len:%u)",
						handlers[i].cmd, len);

					/* skip cmd_len */
					if (handlers[i].skip) {
						rx_buf = net_buf_skip(
							rx_buf,
							handlers[i].cmd_len);
					}

					/* locate next cr/lf */
					frag = NULL;
					len = net_buf_findcrlf(rx_buf,
							       &frag, &offset);

					/* call handler */
					if (handlers[i].func) {
						handlers[i].func(&rx_buf, len);
					}

					frag = NULL;
					/* make sure buf still has data */
					if (!rx_buf) {
						break;
					}

					/* locate next cr/lf */
					len = net_buf_findcrlf(rx_buf,
							       &frag, &offset);
					if (!frag) {
						break;
					}

					break;
				}
			}

			if (frag && rx_buf) {
				/* clear out processed line (buffers) */
				while (frag && rx_buf != frag) {
					rx_buf = net_buf_frag_del(NULL, rx_buf);
				}

				net_buf_pull(rx_buf, offset);
			}
		}

		/* give up time if we have a solid stream of data */
		k_yield();
	}
}

#if defined(DT_INST_0_EXPRESSIF_ESP32_WIFI_MDM_WIFI_GPIOS_CONTROLLER)
static void esp32_gpio_reset(void)
{
	struct device *gpio_dev = device_get_binding(
		DT_INST_0_EXPRESSIF_ESP32_WIFI_MDM_WIFI_GPIOS_CONTROLLER);

	if (!gpio_dev) {
		LOG_ERR("gpio device is not found: %s",
			CONFIG_WIFI_ESP32_GPIO_DEVICE);
	}

	gpio_pin_configure(gpio_dev,
			   DT_INST_0_EXPRESSIF_ESP32_WIFI_MDM_BOOT_GPIOS_PIN,
			   GPIO_DIR_OUT);
	gpio_pin_configure(gpio_dev,
			   DT_INST_0_EXPRESSIF_ESP32_WIFI_MDM_WIFI_GPIOS_PIN,
			   GPIO_DIR_OUT);
	gpio_pin_write(gpio_dev,
		       DT_INST_0_EXPRESSIF_ESP32_WIFI_MDM_WIFI_GPIOS_PIN,
		       MDM_PIN_DISABLE);

	/* enable device and check for ready */
	k_sleep(100);

	gpio_pin_write(gpio_dev,
		       DT_INST_0_EXPRESSIF_ESP32_WIFI_MDM_BOOT_GPIOS_PIN,
		       MDM_PIN_ENABLE);
	gpio_pin_write(gpio_dev,
		       DT_INST_0_EXPRESSIF_ESP32_WIFI_MDM_WIFI_GPIOS_PIN,
		       MDM_PIN_ENABLE);
	gpio_pin_write(gpio_dev,
		       DT_INST_0_EXPRESSIF_ESP32_WIFI_MDM_HOST_WK_GPIOS_PIN,
		       MDM_PIN_ENABLE);
}
#endif

static void esp32_reset_work(void)
{
	int ret = -EINVAL;
	int retries = 0U;

/* Label for boot issues */
reset:
	retries++;

	/* TODO: Option setting for restore on boot */
	/* send AT+RESTORE command to remove saved settings*/
	LOG_INF("AT+RESTORE");
	ret = send_at_cmd(NULL, "AT+RESTORE", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("failed to resore factory defaults");
		goto exit;
	}

#if defined(CONFIG_WIFI_ESP32_HAS_ENABLE_PIN)
	esp32_gpio_reset();
#else
	/* send AT+RST command */
	int retry_count = 3;

	while (retry_count-- && ret < 0) {
		k_sleep(K_MSEC(100));
		ret = send_at_cmd(NULL, "AT+RST", MDM_CMD_TIMEOUT);
		if (ret < 0 && ret != -ETIMEDOUT) {
			break;
		}
	}

	if (ret < 0) {
		LOG_ERR("Cannot send reset %d", retry_count);
		goto exit;
	}
#endif

	k_sem_reset(&mdm_data.response_sem);
	if (k_sem_take(&mdm_data.response_sem, MDM_CMD_TIMEOUT)) {
		LOG_ERR("Reset timed out..");
		goto exit;
	}

	/* Give modem a chance to wake-up */
	k_sleep(K_SECONDS(5));

	ret = send_at_cmd(NULL, "ATE1", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("failed to set echo mode");
		goto exit;
	}

	ret = send_at_cmd(NULL, "AT+CWMODE=1", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("failed to set current Wi-Fi mode to 1");
		goto exit;
	}

	ret = send_at_cmd(NULL, "AT+CIPMUX=1", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("failed to set multiple socket support");
		goto exit;
	}

	ret = send_at_cmd(NULL, "AT+CIPSTAMAC?", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("failed to get the MAC address");
		goto exit;
	}

	ret = send_at_cmd(NULL, "AT+CWDHCP=1,1", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("failed to set DHCP for Station");
		goto exit;
	}

exit:
	if (ret < 0 && retries < MDM_MAX_RETRIES) {
		goto reset;
	}
}

static int esp32_init(struct device *dev)
{
	memset(&mdm_data, 0, sizeof(mdm_data));

	if (mdm_receiver_register(&mdm_data.mdm_ctx,
				  CONFIG_WIFI_ESP32_UART_DEVICE,
				  mdm_recv_buf, sizeof(mdm_recv_buf)) < 0) {
		LOG_ERR("Error registering modem receiver");
		return -EINVAL;
	}

	k_sem_init(&mdm_data.response_sem, 0, 1);

	k_delayed_work_init(&mdm_data.ip_addr_work, esp32_ip_addr_work);

	/* initialize the work queue */
	k_work_q_start(&esp32_workq,
		       esp32_workq_stack,
		       K_THREAD_STACK_SIZEOF(esp32_workq_stack),
		       K_PRIO_COOP(7));

	/* start RX thread */
	k_thread_create(&esp32_rx_thread, esp32_rx_stack,
			K_THREAD_STACK_SIZEOF(esp32_rx_stack),
			(k_thread_entry_t) esp32_rx,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	/* reset device and wait */
	esp32_reset_work();
	LOG_INF("esp32 initialized");

	return 0U;
}

NET_DEVICE_OFFLOAD_INIT(esp32, "ESP32",
			esp32_init, &mdm_data, NULL,
			80, &esp32_api,
			MDM_MAX_DATA_LENGTH);
