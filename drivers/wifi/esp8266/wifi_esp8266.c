/**
 * Copyright (c) 2018 Linaro, Lmtd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME wifi_esp8266
#define LOG_LEVEL CONFIG_WIFI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME)

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <misc/printk.h>
#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <net/net_if.h>
#include <net/wifi_mgmt.h>
#include <net/net_offload.h>
#include <uart.h>
#include <gpio.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_l2.h>
#include <net/net_context.h>
#include <net/net_offload.h>
#include <net/wifi_mgmt.h>
#include <net/socket.h>
#include <net/net_app.h>
#if defined(CONFIG_NET_UDP)
#include <net/udp.h>
#endif
#if defined(CONFIG_NET_TCP)
#include <net/tcp.h>
#endif
#include <drivers/modem/modem_receiver.h>


#define ESP8266_MAX_CONNECTIONS	5
#define BUF_ALLOC_TIMEOUT K_SECONDS(1)
#define MDM_MAX_DATA_LENGTH	1600

struct cmd_handler {
	const char *cmd;
	u16_t cmd_len;
	void (*func)(struct net_buf **buf, u16_t len);
	int skip;
};

#define CMD_HANDLER_NO_SKIP(cmd_, cb_) { \
	.cmd = cmd_, \
	.cmd_len = (u16_t)sizeof(cmd_)-1, \
	.func = on_cmd_ ## cb_, \
	.skip = 0 \
}

#define CMD_HANDLER(cmd_, cb_) { \
	.cmd = cmd_, \
	.cmd_len = (u16_t)sizeof(cmd_)-1, \
	.func = on_cmd_ ## cb_, \
	.skip = 1 \
}

static u8_t mdm_recv_buf[MDM_MAX_DATA_LENGTH];

/* net bufs */
NET_BUF_POOL_DEFINE(esp8266_recv_pool, 12, 128, 0, NULL);
static struct k_work_q esp8266_workq;

/* RX thread structures */
K_THREAD_STACK_DEFINE(esp8266_rx_stack, 1024);

/* RX thread work queue */
K_THREAD_STACK_DEFINE(esp8266_workq_stack,
		      1024);

struct k_thread esp8266_rx_thread;

K_SEM_DEFINE(sock_sem, 1, 1);
K_MUTEX_DEFINE(dev_mutex);

struct socket_data {
	struct net_context	*context;
	sa_family_t family;
	enum net_sock_type type;
	enum net_ip_protocol ip_proto;
	net_context_send_cb_t		send_cb;
	net_context_recv_cb_t		recv_cb;

	struct k_work recv_cb_work;
	void *recv_user_data;

	struct sockaddr		src;
	struct sockaddr		dst;
	struct net_pkt		*rx_pkt;
	struct net_buf		*pkt_buf;
	int			ret;
	char tmp[16];
	/** semaphore */
	int connected;
	int read_error;
};

struct esp8266_data {
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
	struct socket_data socket_data[ESP8266_MAX_CONNECTIONS];
	u8_t sock_map;

	/* wifi scan callbacks */
	scan_result_cb_t wifi_scan_cb;

	int sending_data;

	int data_id;
	int data_len;

	/* delayed work requests */

	/* device management */
	struct device *uart_dev;
	struct device *gpio_dev;
};

static struct esp8266_data foo_data;

#define MDM_CMD_TIMEOUT		K_SECONDS(5)

static inline void _hexdump(const u8_t *packet, size_t length)
{
	char output[sizeof("xxxxyyyy xxxxyyyy")];
	int n = 0, k = 0;
	u8_t byte;

	while (length--) {
		if (n % 16 == 0) {
			printk(" %08X ", n);
		}

		byte = *packet++;

		printk("%02X ", byte);

		if (byte < 0x20 || byte > 0x7f) {
			output[k++] = '.';
		} else {
			output[k++] = byte;
		}

		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				output[k] = '\0';
				printk(" [%s]\n", output);
				k = 0;
			} else {
				printk(" ");
			}
		}
	}

	if (n % 16) {
		int i;

		output[k] = '\0';

		for (i = 0; i < (16 - (n % 16)); i++) {
			printk("   ");
		}

		if ((n % 16) < 8) {
			printk(" "); /* one extra delimiter after 8 chars */
		}

		printk(" [%s]\n", output);
	}
}


/* Send an AT command with a series of response handlers */
static int send_at_cmd(struct socket_data *sock,
			const u8_t *data, int timeout)
{
	int ret;

	foo_data.last_error = 0;

	k_mutex_lock(&dev_mutex, K_FOREVER);
	k_sem_reset(&foo_data.response_sem);
	mdm_receiver_send(&foo_data.mdm_ctx, data, strlen(data));
	mdm_receiver_send(&foo_data.mdm_ctx, "\r\n", 2);
	k_mutex_unlock(&dev_mutex);

	if (timeout == K_NO_WAIT) {
		return 0;
	}

	ret = k_sem_take(&foo_data.response_sem, timeout);

	if (ret == 0) {
		ret = foo_data.last_error;
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

	while (frag && index) {
		if (frag->len > index) {
			offset += index;
			pos += index;
			break;
		} else {
			index -= frag->len;
			pos += frag->len;
			offset = 0;
			if (!frag->frags) {
				return -1;
			}
			frag = frag->frags;
		}
	}

	while(*(frag->data + offset) != d) {
		if (offset == frag->len) {
			if (!frag->frags) {
				return -1;
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

static void on_cmd_esp8266_ready(struct net_buf **buf, u16_t len)
{
	k_sem_give(&foo_data.response_sem);
}

static void esp8266_ip_addr_work(struct k_work *work)
{
	int ret;

	ret = send_at_cmd(NULL, "AT+CIPSTA_CUR?", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("failed to get ip address information\n");
		return;
	}

	/* update interface addresses */
	net_if_ipv4_set_gw(foo_data.iface, &foo_data.gw);
	net_if_ipv4_set_netmask(foo_data.iface,
			&foo_data.netmask);
	net_if_ipv4_addr_add(foo_data.iface, &foo_data.ip, NET_ADDR_DHCP,
		0);
}

static void on_cmd_ip_addr_get(struct net_buf **buf, u16_t len)
{
		k_delayed_work_submit_to_queue(&esp8266_workq,
			&foo_data.ip_addr_work, K_SECONDS(2));
}

static void on_cmd_conn_open(struct net_buf **buf, u16_t len)
{
	int id;
	u8_t msg[2];

	net_buf_linearize(msg, 1, *buf, 0, 1);
	msg[1] = '\0';
	id = atoi(msg);

	foo_data.socket_data[id].connected = 1;
}

static void on_cmd_conn_closed(struct net_buf **buf, u16_t len)
{
	int id;
	u8_t msg[2];
	struct socket_data *sock = NULL;

	net_buf_linearize(msg, 1, *buf, 0, 1);
	msg[1] = '\0';
	id = atoi(msg);

	sock = &foo_data.socket_data[id];
	sock->connected = 0;
}

static void on_cmd_sent_bytes(struct net_buf **buf, u16_t len)
{
	char temp[32];
	char *tok;

	net_buf_linearize(temp, 32, *buf, 0, len);
	temp[len] = '\0';

	tok = strtok(temp, " ");
	while(tok) {
		tok = strtok(NULL, " ");
	}
}


static void on_cmd_wifi_scan_resp(struct net_buf **buf, u16_t len)
{
	int i;
	char temp[32];
	struct wifi_scan_result result;
	int delimiters[6];
	int slen;

	/* ecn, ssid, rssi, mac, channel, freq */

	delimiters[0] = 1;
	for (i = 1; i < 6; i++) {
		delimiters[i] = net_buf_find_next_delimiter(*buf, ',',
					delimiters[i-1] + 1);
		if (delimiters[i] == -1) {
			return;
		}
		delimiters[i]++;
	}

	/* ecn */
	net_buf_linearize(temp, 1, *buf, delimiters[0],
			  delimiters[1] - delimiters[0]);
	if (temp[0] != '0') {
		result.security = WIFI_SECURITY_TYPE_PSK;
	} else {
		result.security = WIFI_SECURITY_TYPE_PSK;
	}

	/* ssid */
	slen = delimiters[2] - delimiters[1] - 3;
	net_buf_linearize(result.ssid, 32, *buf, delimiters[1] + 1,
			  slen);
	result.ssid_length = slen;

	/* rssi */
	slen = delimiters[3] - delimiters[2];
	net_buf_linearize(temp, 32, *buf, delimiters[2], slen);
	temp[slen] = '\0';
	result.rssi = strtol(temp, NULL, 10);

	/* channel */
	slen = delimiters[5] - delimiters[4];
	net_buf_linearize(temp, 32, *buf, delimiters[4], slen);
	temp[slen] = '\0';
	result.channel = strtol(temp, NULL, 10);

	/* issue callback to report scan results */
	if (foo_data.wifi_scan_cb) {
		foo_data.wifi_scan_cb(foo_data.iface, 0, &result);
	}
}

static const char nm_label[] = "netmask";
static const char gw_label[] = "gateway";
static const char ip_label[] = "ip";

static void on_cmd_ip_addr_resp(struct net_buf **buf, u16_t len)
{
	char ip_addr[] = "xxx.xxx.xxx.xxx";
	int d[3];
	int slen;

	d[0] = net_buf_find_next_delimiter(*buf, ':', 0);
	d[1] = net_buf_find_next_delimiter(*buf, '\"', d[0] + 1);
	d[2] = net_buf_find_next_delimiter(*buf, '\"', d[1] + 1);

	slen = d[2] - d[1] - 1;

	net_buf_linearize(ip_addr, 16, *buf, d[0] + 2, slen);
	ip_addr[slen] = '\0';

	if (net_buf_ncmp(*buf, nm_label, strlen(nm_label)) == 0) {
		inet_pton(AF_INET, ip_addr,
			  &foo_data.netmask);
	} else if (net_buf_ncmp(*buf, ip_label, strlen(ip_label)) == 0) {
		inet_pton(AF_INET, ip_addr,
			  &foo_data.ip);
	} else if (net_buf_ncmp(*buf, gw_label, strlen(gw_label)) == 0) {
		inet_pton(AF_INET, ip_addr,
			  &foo_data.gw);
	} else {
		return;
	}
}

static void on_cmd_mac_addr_resp(struct net_buf **buf, u16_t len)
{
	int i;
	char octet[] = "xx";
	for (i = 0; i < 6; i++) {
		net_buf_pull_u8(*buf);
		octet[0]= net_buf_pull_u8(*buf);
		octet[1]= net_buf_pull_u8(*buf);
		foo_data.mac[i] = strtoul(octet, NULL, 16);
	}
}

static void on_cmd_sendok(struct net_buf **buf, u16_t len)
{
	foo_data.last_error = 0;

	/* only signal semaphore if we are not waiting on a data send command */
	k_sem_give(&foo_data.response_sem);
}

/* Handler: OK */
static void on_cmd_sockok(struct net_buf **buf, u16_t len)
{
	foo_data.last_error = 0;

	if (!foo_data.sending_data) {
		k_sem_give(&foo_data.response_sem);
	}
}

/* Handler: ERROR */
static void on_cmd_sockerror(struct net_buf **buf, u16_t len)
{
	foo_data.last_error = -EIO;
	k_sem_give(&foo_data.response_sem);
}

static void on_cmd_wifi_connected_resp(struct net_buf **buf, u16_t len)
{
	wifi_mgmt_raise_connect_result_event(foo_data.iface, 0);
}

static void on_cmd_wifi_disconnected_resp(struct net_buf **buf, u16_t len)
{
	wifi_mgmt_raise_disconnect_result_event(foo_data.iface, 0);
}

static int esp8266_get(sa_family_t family,
			enum net_sock_type type,
			enum net_ip_protocol ip_proto,
			struct net_context **context)
{
	int i;
	struct net_context *c = *context;

	if (family != AF_INET) {
		return -1;
	}

	k_sem_take(&sock_sem, K_FOREVER);
	for (i = 0; i < ESP8266_MAX_CONNECTIONS && foo_data.sock_map & BIT(i); i++){}
	if (i >= ESP8266_MAX_CONNECTIONS) {
		k_sem_give(&sock_sem);
		return 1;
	}

	foo_data.sock_map |= 1 << i;
	c->offload_context = (void *)&foo_data.socket_data[i];
	foo_data.socket_data[i].context = c;
	foo_data.socket_data[i].family = family;
	foo_data.socket_data[i].type = type;
	foo_data.socket_data[i].ip_proto = ip_proto;
	foo_data.socket_data[i].connected = 0;

	k_sem_give(&sock_sem);
	return 0;
}

static const char type_tcp[] = "TCP";
static const char type_udp[] = "UDP";

static int esp8266_connect(struct net_context *context,
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

	if (!context || !addr) {
		return -EINVAL;
	}

	sock = (struct socket_data *)context->offload_context;
	if (!sock) {
		LOG_ERR("Can't find socket info for ctx: %p\n", context);
		return -EINVAL;
	}

	sock = (struct socket_data *)context->offload_context;
	sock->dst.sa_family = addr->sa_family;
#if defined (CONFIG_NET_IPV4)
	if (addr->sa_family == AF_INET) {
		net_ipaddr_copy(&net_sin(&sock->dst)->sin_addr,
				&net_sin(addr)->sin_addr);
		net_sin(&sock->dst)->sin_port = net_sin(addr)->sin_port;
	} else
#endif
	{
		return -EINVAL;
	}

	if (net_sin(&sock->dst)->sin_port < 0) {
		LOG_ERR("invalid port: %d\n",
			    net_sin(&sock->dst)->sin_port);
		return -EINVAL;
	}

	if (net_context_get_type(context) == SOCK_STREAM) {
		type = type_tcp;
	} else {
		type = type_udp;
	}

	if (type == type_tcp) {
	inet_ntop(sock->dst.sa_family, &net_sin(&sock->dst)->sin_addr,
			     addr_str, sizeof(addr_str));
	len = sprintf(connect_msg, "AT+CIPSTART=%d,\"%s\",\"%s\",%d,7200",
			sock - foo_data.socket_data, type, addr_str,
			ntohs(net_sin(&sock->dst)->sin_port));
	} else {
	inet_ntop(sock->dst.sa_family, &net_sin(&sock->dst)->sin_addr,
			     addr_str, sizeof(addr_str));
	len = sprintf(connect_msg, "AT+CIPSTART=%d,\"%s\",\"%s\",%d",
			sock - foo_data.socket_data, type, addr_str,
			ntohs(net_sin(&sock->dst)->sin_port));
	}

	ret = send_at_cmd(sock, connect_msg, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("failed to send connect\n");
		ret = -EINVAL;
	} else {
		sock->connected = 1;
	}

	if (cb) {
		cb(context, ret, user_data);
	}

	return 0;
}

static int esp8266_bind(struct net_context *context,
			const struct sockaddr *addr,
			socklen_t addrlen)
{
	struct socket_data *sock = NULL;

	if (!context) {
		return -EINVAL;
	}

	sock = (struct socket_data *)context->offload_context;
	if (!sock) {
		LOG_ERR("Missing socket for ctx: %p\n", context);
		return -EINVAL;
	}

	sock->src.sa_family = addr->sa_family;
#if defined (CONFIG_NET_IPV4)
	if (addr->sa_family == AF_INET) {
		net_ipaddr_copy(&net_sin(&sock->src)->sin_addr,
				&net_sin(addr)->sin_addr);
		net_sin(&sock->src)->sin_port = net_sin(addr)->sin_port;
	} else
#endif
	{
		return -EPFNOSUPPORT;
	}
	return 0;
}

static int esp8266_listen(struct net_context *context, int backlog)
{
	return -EPFNOSUPPORT;
}

static int esp8266_accept(struct net_context *context,
			net_tcp_accept_cb_t cb,
			s32_t timeout,
			void *user_data)
{
	return -EPFNOSUPPORT;
}

static int esp8266_sendto(struct net_pkt *pkt,
		const struct sockaddr *dst_addr,
		socklen_t addrlen,
		net_context_send_cb_t cb,
		s32_t timeout,
		void *token,
		void *user_data)
{
	u8_t send_msg[48];
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
	id = data - foo_data.socket_data;

	if (!data->connected) {
		esp8266_connect(data->context,
			dst_addr,
			addrlen,
			NULL,
			NULL, NULL);
	}

	frag = pkt->frags;
	send_len = net_buf_frags_len(frag);
	foo_data.sending_data = 1;
	if (net_context_get_type(context) == SOCK_STREAM) {
		len = sprintf(send_msg, "AT+CIPSEND=%d,%d", id,
			      net_buf_frags_len(frag));
	} else {
		inet_ntop(data->dst.sa_family, &net_sin(&data->dst)->sin_addr,
			     addr_str, sizeof(addr_str));
		len = sprintf(send_msg, "AT+CIPSEND=%d,%d,\"%s\",%d",
			id, net_buf_frags_len(frag), addr_str,
				ntohs(net_sin(&data->dst)->sin_port));
	}

	ret = send_at_cmd(NULL, send_msg, MDM_CMD_TIMEOUT * 2);
	if (ret < 0) {
		ret = -EINVAL;
		goto ret_early;
	}

	foo_data.sending_data = 0;
	k_sem_reset(&foo_data.response_sem);
	k_mutex_lock(&dev_mutex, K_FOREVER);
	while (send_len && frag) {
		if (frag->len > send_len) {
			mdm_receiver_send(&foo_data.mdm_ctx, frag->data,
					  send_len);

			send_len = 0;
			break;
		} else {
			mdm_receiver_send(&foo_data.mdm_ctx, frag->data,
					  frag->len);
			send_len -= frag->len;
			frag = frag->frags;
		}
	}
	k_mutex_unlock(&dev_mutex);

	ret = k_sem_take(&foo_data.response_sem, MDM_CMD_TIMEOUT*2);

	if (ret == 0) {
		ret = foo_data.last_error;
	} else if (ret == -EAGAIN) {
		ret = -ETIMEDOUT;
	}

ret_early:
	net_pkt_unref(pkt);
	if (cb) {
		cb(context, ret, token, user_data);
	}

	return ret;
}

static int esp8266_put(struct net_context *context)
{
	struct socket_data *data;
	u8_t msg[20];
	int len;

	if (!context || !context->offload_context) {
		return -EINVAL;
	}

	data = context->offload_context;
	int id = data - foo_data.socket_data;
	foo_data.sock_map &= ~(1 << id);

	foo_data.socket_data[id].recv_cb = NULL;
	foo_data.socket_data[id].send_cb = NULL;

	if (foo_data.socket_data[id].connected) {
		len = sprintf(msg, "AT+CIPCLOSE=%d", id);
		msg[len] = '\0';

		if (send_at_cmd(&foo_data.socket_data[id], msg,
				MDM_CMD_TIMEOUT)) {
			LOG_ERR("failed to close\n");
		}
	}

	net_context_unref(context);

	data->context = NULL;
	memset(&data->src, 0, sizeof(struct sockaddr));
	memset(&data->dst, 0, sizeof(struct sockaddr));
	return 0;
}

static int esp8266_send(struct net_pkt *pkt,
		net_context_send_cb_t cb,
		s32_t timeout,
		void *token,
		void *user_data)
{
	struct net_context *context = net_pkt_context(pkt);
	socklen_t addrlen;
	int ret;

	addrlen = 0;
#if defined(CONFIG_NET_IPV4)
	if (net_pkt_family(pkt) == AF_INET) {
		addrlen = sizeof(struct sockaddr_in);
	} else
#endif /* CONFIG_NET_IPV4 */
	{
		return -EPFNOSUPPORT;
	}

	ret = esp8266_sendto(pkt, &context->remote, addrlen, cb,
			      timeout, token, user_data);
	return ret;
}

static int esp8266_recv(struct net_context *context,
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

	return 0;
}

static struct net_offload esp8266_offload = {
	.get            = esp8266_get,
	.bind		= esp8266_bind,
	.listen		= esp8266_listen,
	.connect	= esp8266_connect,
	.accept		= esp8266_accept,
	.send		= esp8266_send,
	.sendto		= esp8266_sendto,
	.recv		= esp8266_recv,
	.put		= esp8266_put,
};

static int esp8266_mgmt_scan(struct device *dev, scan_result_cb_t cb)
{
	int ret;

	foo_data.wifi_scan_cb = cb;

	ret = send_at_cmd(NULL, "AT+CWLAP", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("failed to send scan\n");
		ret = -EINVAL;
	}

	foo_data.wifi_scan_cb = NULL;
	return 0;
};

static u8_t connect_msg[100];

static int esp8266_mgmt_connect(struct device *dev,
				   struct wifi_connect_req_params *params)
{
	int len;
	int ret;

	if (params->security == WIFI_SECURITY_TYPE_PSK) {
		len = sprintf(connect_msg, "AT+CWJAP_CUR=\"");
		memcpy(&connect_msg[len], params->ssid,
		       params->ssid_length);
		len += params->ssid_length;
		len += sprintf(&connect_msg[len], "\",\"");
		memcpy(&connect_msg[len], params->psk,
		       params->psk_length);
		len += params->psk_length;
		len += sprintf(&connect_msg[len], "\"");
		connect_msg[len] = '\0';
	} else {
		len = sprintf(connect_msg, "AT+CWJAP_CUR=\"");
		memcpy(&connect_msg[len], params->ssid,
		       params->ssid_length);
		len += params->ssid_length;
		len += sprintf(&connect_msg[len], "\"");
		connect_msg[len] = '\0';
	}

	ret = send_at_cmd(NULL, connect_msg, MDM_CMD_TIMEOUT * 2);
	if (ret < 0) {
		LOG_ERR("failed to send scan\n");
		return -EINVAL;
	}

	return 0;
}

static int esp8266_mgmt_disconnect(struct device *dev)
{
	int ret;

	ret = send_at_cmd(NULL, "AT+CWQAP", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("failed to query mac address\n");
		return -EFAULT;
	}

	return 0;
}

static void esp8266_iface_init(struct net_if *iface)
{
	atomic_clear_bit(iface->if_dev->flags, NET_IF_UP);

	net_if_set_link_addr(iface, foo_data.mac,
			     sizeof(foo_data.mac),
			     NET_LINK_ETHERNET);

	iface->if_dev->offload = &esp8266_offload;
	foo_data.iface = iface;

	atomic_set_bit(foo_data.iface->if_dev->flags, NET_IF_UP);
}

static const struct net_wifi_mgmt_offload esp8266_api = {
	.iface_api.init = esp8266_iface_init,
	.scan		= esp8266_mgmt_scan,
	.connect	= esp8266_mgmt_connect,
	.disconnect	= esp8266_mgmt_disconnect,
};

static inline struct net_buf *read_rx_allocator(s32_t timeout, void *user_data)
{
	return net_buf_alloc((struct net_buf_pool *)user_data, timeout);
}

static void esp8266_read_rx(struct net_buf **buf)
{
	u8_t uart_buffer[128];
	size_t bytes_read = 0;
	int ret;
	u16_t rx_len;

	/* read all of the data from mdm_receiver */
	while (true) {
		ret = mdm_receiver_recv(&foo_data.mdm_ctx,
					uart_buffer,
					sizeof(uart_buffer),
					&bytes_read);
		if (ret < 0) {
			/* mdm_receiver buffer is empty */
			break;
		}
//		_hexdump(uart_buffer, bytes_read);

		/* make sure we have storage */
		if (!*buf) {
			*buf = net_buf_alloc(&esp8266_recv_pool, BUF_ALLOC_TIMEOUT);
			if (!*buf) {
				LOG_ERR("Can't allocate RX data! "
					    "Skipping data!");
				break;
			}
		}

		rx_len = net_buf_append_bytes(*buf, bytes_read, uart_buffer,
					      BUF_ALLOC_TIMEOUT,
					      read_rx_allocator,
					      &esp8266_recv_pool);
		if (rx_len < bytes_read) {
			LOG_ERR("Data was lost! read %u of %u!",
				    rx_len, bytes_read);
		}
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

	return 0;
}

/* Setup IP header data to be used by some network applications.
 * While much is dummy data, some fields such as dst, port and family are
 * important.
 * Return the IP + protocol header length.
 */
static int net_pkt_setup_ip_data(struct net_pkt *pkt,
	enum net_ip_protocol proto,
	struct sockaddr *src,
	struct sockaddr *dst)
{
	int hdr_len = 0;
	u16_t src_port = 0, dst_port = 0;

#if defined(CONFIG_NET_IPV6)
	if (net_pkt_family(pkt) == AF_INET6) {
		net_buf_add(pkt->frags, NET_IPV6H_LEN);

		/* set IPv6 data */
		NET_IPV6_HDR(pkt)->vtc = 0x60;
		NET_IPV6_HDR(pkt)->tcflow = 0;
		NET_IPV6_HDR(pkt)->flow = 0;
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src,
			&((struct sockaddr_in6 *)dst)->sin6_addr);
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst,
			&((struct sockaddr_in6 *)src)->sin6_addr);
		NET_IPV6_HDR(pkt)->nexthdr = proto;

		src_port = net_sin6(&dst)->sin6_port;
		dst_port = net_sin6(&src)->sin6_port;

		net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
		net_pkt_set_ipv6_ext_len(pkt, 0);
		hdr_len = sizeof(struct net_ipv6_hdr);
	} else
#endif
#if defined(CONFIG_NET_IPV4)
	if (net_pkt_family(pkt) == AF_INET) {
		net_buf_add(pkt->frags, NET_IPV4H_LEN);

		/* set IPv4 data */
		NET_IPV4_HDR(pkt)->vhl = 0x45;
		NET_IPV4_HDR(pkt)->tos = 0x00;
		net_ipaddr_copy(&NET_IPV4_HDR(pkt)->src,
			&((struct sockaddr_in *)dst)->sin_addr);
		net_ipaddr_copy(&NET_IPV4_HDR(pkt)->dst,
			&((struct sockaddr_in *)src)->sin_addr);
		NET_IPV4_HDR(pkt)->proto = proto;

		src_port = net_sin(dst)->sin_port;
		dst_port = net_sin(src)->sin_port;

		net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));
		hdr_len = sizeof(struct net_ipv4_hdr);
	} else
#endif
	{
		/* no error here as hdr_len is checked later for 0 value */
	}

#if defined(CONFIG_NET_UDP)
	if (proto == IPPROTO_UDP) {
		struct net_udp_hdr hdr, *udp;

		net_buf_add(pkt->frags, NET_UDPH_LEN);
		udp = net_udp_get_hdr(pkt, &hdr);
		memset(udp, 0, NET_UDPH_LEN);

		/* Setup UDP header */
		udp->src_port = htons(src_port);
		udp->dst_port = htons(dst_port);
		net_udp_set_hdr(pkt, udp);
		hdr_len += NET_UDPH_LEN;
	} else
#endif
#if defined(CONFIG_NET_TCP)
	if (proto == IPPROTO_TCP) {
		struct net_tcp_hdr hdr, *tcp;

		net_buf_add(pkt->frags, NET_TCPH_LEN);
		tcp = net_tcp_get_hdr(pkt, &hdr);
		memset(tcp, 0, NET_TCPH_LEN);

		/* Setup TCP header */
		tcp->src_port = src_port;
		tcp->dst_port = dst_port;
		net_tcp_set_hdr(pkt, tcp);
		hdr_len += NET_TCPH_LEN;
	} else
#endif /* CONFIG_NET_TCP */
	{
		/* no error here as hdr_len is checked later for 0 value */
	}

	return hdr_len;
}

static void esp8266_read_data(struct net_buf **buf)
{
	struct net_buf *frag;
	struct net_buf *inc = *buf;
	struct socket_data *sock = &foo_data.socket_data[foo_data.data_id];
	u16_t pos;
	int bytes_left = foo_data.data_len;
	int hdr_len;


	sock->read_error = 0;
	sock->rx_pkt = net_pkt_get_rx(sock->context, BUF_ALLOC_TIMEOUT);
	if (!sock->rx_pkt) {
		sock->read_error = -ENOMEM;
		LOG_ERR("Failed to get net pkt");
		goto fail;
	}

	/* set up packet data */
	net_pkt_set_context(sock->rx_pkt, sock->context);
	net_pkt_set_family(sock->rx_pkt, sock->family);

	frag = net_pkt_get_frag(sock->rx_pkt, BUF_ALLOC_TIMEOUT);
	if (!frag) {
		sock->read_error = -ENOMEM;
		net_pkt_unref(sock->rx_pkt);
		sock->rx_pkt = NULL;
		goto fail;
	}

	net_pkt_frag_add(sock->rx_pkt, frag);

	hdr_len = net_pkt_setup_ip_data(sock->rx_pkt, sock->ip_proto,
			&sock->src, &sock->dst);

	while (inc && bytes_left) {
		if (inc->len > bytes_left) {
			pos = net_pkt_append(sock->rx_pkt, bytes_left,
				inc->data, BUF_ALLOC_TIMEOUT);
			if (pos != bytes_left) {
				sock->read_error = -ENOMEM;
				net_pkt_unref(sock->rx_pkt);
				sock->rx_pkt = NULL;
				break;
			}
			bytes_left = 0;
		} else {
			pos = net_pkt_append(sock->rx_pkt, inc->len, inc->data,
				BUF_ALLOC_TIMEOUT);
			if (pos != inc->len) {
				sock->read_error = -ENOMEM;
				net_pkt_unref(sock->rx_pkt);
				sock->rx_pkt = NULL;
				break;
			}
			bytes_left -= inc->len;
			inc = inc->frags;
		}
	}

	net_pkt_set_appdatalen(sock->rx_pkt, foo_data.data_len);
	if (hdr_len > 0) {
		frag = net_frag_get_pos(sock->rx_pkt, hdr_len, &pos);
		NET_ASSERT(frag);
		net_pkt_set_appdata(sock->rx_pkt, frag->data + pos);
	} else {
		net_pkt_set_appdata(sock->rx_pkt,
				    sock->rx_pkt->frags->data);
	}

fail:
	/* return data */
	if (sock->recv_cb) {
		if (sock->read_error) {
			sock->recv_cb(sock->context, NULL, sock->read_error,
				      sock->recv_user_data);
		} else {
			sock->recv_cb(sock->context, sock->rx_pkt, 0,
				      sock->recv_user_data);
		}
	} else {
		if (sock->rx_pkt) {
			net_pkt_unref(sock->rx_pkt);
		}
	}

	sock->rx_pkt = NULL;
	*buf = net_buf_skip(*buf, foo_data.data_len);
	foo_data.data_len = 0;
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

static int esp8266_process_setup_read(struct net_buf **buf)
{
	struct net_buf *frag;
	u8_t temp[32];
	int hdr_len;
	int offset;
	char *colon;
	char *comma;
	int found;
	int end;

	frag = *buf;
	end = 0;
	offset = 0;
	found = -1;
	while (frag) {
		temp[end] = *(frag->data + offset);
		if (temp[end] == ':'){
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

	if (found < 0)
		return found;

	temp[found+1] = '\0';
	hdr_len = found+1;
	colon = strtok(temp, ":");

	comma = strtok(colon, ",");
	comma = strtok(NULL, ",");
	foo_data.data_id = atoi(comma);
	comma = strtok(NULL, ",");
	foo_data.data_len = atoi(comma);
	*buf = net_buf_skip(*buf, hdr_len);

	LOG_INF("MATCH +IPD (len:%u)", foo_data.data_len);
	return 0;
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
			sock->recv_cb(sock->context, NULL, sock->read_error, sock->recv_user_data);
		} else {
			sock->recv_cb(sock->context, pkt, 0, sock->recv_user_data);
		}
	} else {
		net_pkt_unref(pkt);
	}
}

/* RX thread */
static void esp8266_rx(void)
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
		CMD_HANDLER("AT+CWJAP_CUR=", atcmdecho_nosock),
		CMD_HANDLER("WIFI CONNECTED", wifi_connected_resp),
		CMD_HANDLER("WIFI DISCONNECT",wifi_disconnected_resp),
		CMD_HANDLER("SEND OK", sendok),
		CMD_HANDLER("link is not valid", atcmdecho_nosock),
		CMD_HANDLER("busy p...", atcmdecho_nosock),
		CMD_HANDLER("busy s...", atcmdecho_nosock),
		CMD_HANDLER("ready", esp8266_ready),
		CMD_HANDLER("AT+CIPAPMAC_CUR?", atcmdecho_nosock),
		CMD_HANDLER("+CIPAPMAC_CUR:", mac_addr_resp),
		CMD_HANDLER("AT+CIPSTA_CUR?", atcmdecho_nosock),
		CMD_HANDLER("+CIPSTA_CUR:", ip_addr_resp),
		CMD_HANDLER("AT+CWLAP", atcmdecho_nosock),
		CMD_HANDLER("+CWLAP:", wifi_scan_resp),
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
		k_sem_take(&foo_data.mdm_ctx.rx_sem, K_FOREVER);

		esp8266_read_rx(&rx_buf);

		while (rx_buf) {

			if (foo_data.data_len) {
				if (net_buf_frags_len(rx_buf) >=
					foo_data.data_len) {
					esp8266_read_data(&rx_buf);

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
				if (esp8266_process_setup_read(&rx_buf) < 0) {
					break;
				}

				if (!rx_buf) {
					break;
				}

				continue;
			}

			if (match(rx_buf, ">") >= 0) {
				rx_buf = net_buf_skip(rx_buf, 1);
				k_sem_give(&foo_data.response_sem);
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
					LOG_INF("MATCH %s (len:%u)\n",
						    handlers[i].cmd, len);

					/* skip cmd_len */
					if (handlers[i].skip) {
						rx_buf = net_buf_skip(rx_buf,
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

#if defined(CONFIG_WIFI_ESP8266_HAS_ENABLE_PIN)
static void esp8266_gpio_reset(void)
{
	struct device *gpio_dev = device_get_binding(CONFIG_WIFI_ESP8266_GPIO_DEVICE);

	if (!gpio_dev) {
		LOG_ERR("gpio device is not found: %s",
			    CONFIG_WIFI_ESP8266_GPIO_DEVICE);
	}

	gpio_pin_configure(gpio_dev, CONFIG_WIFI_ESP8266_GPIO_ENABLE_PIN,
			   GPIO_DIR_OUT);

	/* disable device until we want to configure it*/
	gpio_pin_write(drv_data->gpio_dev, CONFIG_WIFI_ESP8266_GPIO_ENABLE_PIN, 0);

	/* enable device and check for ready */
	k_sleep(100);
	gpio_pin_write(drv_data->gpio_dev, CONFIG_WIFI_ESP8266_GPIO_ENABLE_PIN, 1);
}
#endif

static void esp8266_reset_work(void)
{
	int ret = -1;
	int retry_count = 3;

#if defined(CONFIG_WIFI_ESP8266_HAS_ENABLE_PIN)
	esp8266_gpio_reset();
#else
	/* send AT+RST command */
	while(retry_count-- && ret < 0) {
		k_sleep(K_MSEC(100));
		ret = send_at_cmd(NULL, "AT+RST", MDM_CMD_TIMEOUT);
		if (ret < 0 && ret != -ETIMEDOUT) {
			break;
		}
	}

	if (ret < 0) {
		LOG_ERR("cannot send reset %d\n", retry_count);
		return;
	}
#endif

	k_sem_reset(&foo_data.response_sem);
	if (k_sem_take(&foo_data.response_sem, MDM_CMD_TIMEOUT)) {
		LOG_ERR("timed out waiting for device to become ready\n");
		return;
	}

	ret = send_at_cmd(NULL, "ATE1", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("failed to set echo mode\n");
		return;
	}

	ret = send_at_cmd(NULL, "AT+CIPMUX=1", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("failed to set multiple socket support\n");
		return;
	}

	ret = send_at_cmd(NULL, "AT+CIPAPMAC_CUR?", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("failed to set multiple socket support\n");
		return;
	}


	ret = send_at_cmd(NULL, "AT+CWMODE_CUR=3", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("failed to set multiple socket support\n");
		return;
	}

}


static int esp8266_init(struct device *dev)
{
	int i;


	memset(&foo_data, 0, sizeof(foo_data));
	for (i = 0; i < ESP8266_MAX_CONNECTIONS; i++) {
		k_work_init(&foo_data.socket_data[i].recv_cb_work,
			sockreadrecv_cb_work);
	}

	k_sem_init(&foo_data.response_sem, 0, 1);

	k_delayed_work_init(&foo_data.ip_addr_work, esp8266_ip_addr_work);

	/* initialize the work queue */
	k_work_q_start(&esp8266_workq,
		       esp8266_workq_stack,
		       K_THREAD_STACK_SIZEOF(esp8266_workq_stack),
		       K_PRIO_COOP(7));

	if (mdm_receiver_register(&foo_data.mdm_ctx, CONFIG_WIFI_ESP8266_UART_DEVICE,
				  mdm_recv_buf, sizeof(mdm_recv_buf)) < 0) {
		LOG_ERR("Error registering modem receiver");
		return -EINVAL;
	}

	/* start RX thread */
	k_thread_create(&esp8266_rx_thread, esp8266_rx_stack,
			K_THREAD_STACK_SIZEOF(esp8266_rx_stack),
			(k_thread_entry_t) esp8266_rx,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	/* reset device and wait */
	esp8266_reset_work();
	LOG_INF("ESP8266 initialized\n");
	return 0;
}

NET_DEVICE_OFFLOAD_INIT(esp8266, "ESP8266",
			esp8266_init, &foo_data, NULL,
			80, &esp8266_api,
			1500);
