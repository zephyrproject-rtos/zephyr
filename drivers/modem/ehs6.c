/*
 * Copyright (c) 2018 Foundries.io
 * Copyright (c) 2019 Song Qiang <songqiang1304521@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_DOMAIN modem_ehs6
#define LOG_LEVEL CONFIG_MODEM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_DOMAIN);

#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <zephyr.h>
#include <gpio.h>
#include <device.h>
#include <init.h>

#include <net/net_context.h>
#include <net/net_if.h>
#include <net/socket_offload.h>
#include <net/socket.h>
#include <net/net_offload.h>
#include <net/net_pkt.h>
#if defined(CONFIG_NET_IPV4)
#include "ipv4.h"
#endif
#if defined(CONFIG_NET_UDP)
#include <net/udp.h>
#include "udp_internal.h"
#endif
#if defined(CONFIG_NET_TCP)
#include <net/tcp.h>
#endif

#include <drivers/modem/modem_receiver.h>

/* Uncomment the #define below to enable a hexdump of all incoming
 * data from the modem receiver
 */
/* #define ENABLE_VERBOSE_MODEM_RECV_HEXDUMP	1 */

#define	MDM_UART_DEV_NAME		DT_GEMALTO_EHS6_0_BUS_NAME

#define MDM_CMD_TIMEOUT			K_SECONDS(5)
#define MDM_CMD_SEND_TIMEOUT		K_SECONDS(10)
#define MDM_CMD_READ_TIMEOUT		K_SECONDS(10)
#define MDM_CMD_CONN_TIMEOUT		K_SECONDS(30)

#define MDM_MAX_DATA_LENGTH		1500

#define MDM_RECV_MAX_BUF		30
#define MDM_RECV_BUF_SIZE		128

#define MDM_MAX_SOCKETS			6

#define BUF_ALLOC_TIMEOUT K_SECONDS(1)

#define CMD_HANDLER(cmd_, cb_) { \
	.cmd = cmd_, \
	.cmd_len = (u16_t)sizeof(cmd_)-1, \
	.func = on_cmd_ ## cb_ \
}

#define MDM_MANUFACTURER_LENGTH		10
#define MDM_MODEL_LENGTH		16
#define MDM_REVISION_LENGTH		64
#define MDM_IMEI_LENGTH			16
#define MDM_MAX_BUF_LENGTH		1500

#define RSSI_TIMEOUT_SECS		30

NET_BUF_POOL_DEFINE(mdm_recv_pool, MDM_RECV_MAX_BUF, MDM_RECV_BUF_SIZE,
		    0, NULL);

static u8_t mdm_recv_buf[MDM_MAX_DATA_LENGTH];

/* RX thread structures */
K_THREAD_STACK_DEFINE(ehs6_rx_stack,
		       CONFIG_MODEM_EHS6_RX_STACK_SIZE);
struct k_thread ehs6_rx_thread;

struct ehs6_socket {
	struct net_context *context;
	sa_family_t family;
	enum net_sock_type type;
	enum net_ip_protocol ip_proto;

	bool data_ready;

	/** semaphore */
	struct k_sem sem_write_ready;
	struct k_sem sem_read_ready;

	/* Read related parameters. */
	u8_t *p_recv_addr;
	size_t recv_max_len;
	u16_t bytes_read;
	bool is_in_reading;

	bool is_udp_opened;
	bool is_polled;
	bool in_use;
};

struct ehs6_iface_ctx {
	struct net_if *iface;
	u8_t mac_addr[6];

	/* RX specific attributes */
	struct mdm_receiver_context mdm_ctx;

	/* socket data */
	struct ehs6_socket sockets[MDM_MAX_SOCKETS];
	int last_socket_id;
	int last_error;

	/* Response sem for waiting on OK or +CME. */
	struct k_sem sem_response;
	struct k_sem sem_poll;

	/* modem data */
	char mdm_manufacturer[MDM_MANUFACTURER_LENGTH];
	char mdm_model[MDM_MODEL_LENGTH];
	char mdm_revision[MDM_REVISION_LENGTH];
	char mdm_imei[MDM_IMEI_LENGTH];
};

struct cmd_handler {
	const char *cmd;
	u16_t cmd_len;
	void (*func)(struct net_buf **buf, u16_t len);
};

static struct ehs6_iface_ctx ictx;

static void ehs6_read_rx(struct net_buf **buf);

/*** Verbose Debugging Functions ***/
#if defined(ENABLE_VERBOSE_MODEM_RECV_HEXDUMP)
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
#else
#define _hexdump(...)
#endif

static u8_t socket_get(void)
{
	for (int i = 0; i < MDM_MAX_SOCKETS; i++) {
		if (!ictx.sockets[i].in_use) {
			return i;
		}
	}

	return -ENOMEM;
}

/* Send an AT command with a series of response handlers */
static int send_at_cmd(const u8_t *data, struct k_sem *sem, int timeout)
{
	int ret;

	ictx.last_error = 0;

	LOG_DBG("OUT: [%s]", data);
	mdm_receiver_send(&ictx.mdm_ctx, data, strlen(data));
	mdm_receiver_send(&ictx.mdm_ctx, "\r\n", 2);

	if (timeout == K_NO_WAIT) {
		return 0;
	}

	k_sem_reset(sem);
	ret = k_sem_take(sem, timeout);

	if (ret == 0) {
		ret = ictx.last_error;
	} else if (ret == -EAGAIN) {
		/* 121 is not a valid error code of ehs6. */
		ret = -ETIMEDOUT;
	}

	return ret;
}

/*** NET_BUF HELPERS ***/

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
	u16_t len = 0U, pos = 0U;

	while (buf && !is_crlf(*(buf->data + pos))) {
		if (pos + 1 >= buf->len) {
			len += buf->len;
			buf = buf->frags;
			pos = 0U;
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

/*** MODEM RESPONSE HANDLERS ***/

static void on_cmd_atcmdinfo_manufacturer(struct net_buf **buf, u16_t len)
{
	size_t out_len;

	out_len = net_buf_linearize(ictx.mdm_manufacturer,
				    sizeof(ictx.mdm_manufacturer) - 1,
				    *buf, 0, len);
	ictx.mdm_manufacturer[out_len] = 0;
	LOG_INF("Manufacturer: %s", ictx.mdm_manufacturer);
}

static void on_cmd_atcmdinfo_model(struct net_buf **buf, u16_t len)
{
	size_t out_len;

	out_len = net_buf_linearize(ictx.mdm_model,
				    sizeof(ictx.mdm_model) - 1,
				    *buf, 0, len);
	ictx.mdm_model[out_len] = 0;
	LOG_INF("Model: %s", ictx.mdm_model);
}

static void on_cmd_atcmdinfo_revision(struct net_buf **buf, u16_t len)
{
	size_t out_len;

	out_len = net_buf_linearize(ictx.mdm_revision,
				    sizeof(ictx.mdm_revision) - 1,
				    *buf, 0, len);
	ictx.mdm_revision[out_len] = 0;
	LOG_INF("Revision: %s", ictx.mdm_revision);
}

static void on_cmd_atcmdinfo_imei(struct net_buf **buf, u16_t len)
{
	struct net_buf *frag = NULL;
	u16_t offset;
	size_t out_len;

	/* make sure IMEI data is received */
	if (len < MDM_IMEI_LENGTH) {
		LOG_DBG("Waiting for data");
		/* wait for more data */
		k_sleep(K_MSEC(500));
		ehs6_read_rx(buf);
	}

	net_buf_skipcrlf(buf);
	if (!*buf) {
		LOG_DBG("Unable to find IMEI (net_buf_skipcrlf)");
		return;
	}

	frag = NULL;
	len = net_buf_findcrlf(*buf, &frag, &offset);
	if (!frag) {
		LOG_DBG("Unable to find IMEI (net_buf_findcrlf)");
		return;
	}

	out_len = net_buf_linearize(ictx.mdm_imei, sizeof(ictx.mdm_imei) - 1,
				    *buf, 0, len);
	ictx.mdm_imei[out_len] = 0;

	LOG_INF("IMEI: %s", ictx.mdm_imei);
}

/* Handler: OK */
static void on_cmd_sockok(struct net_buf **buf, u16_t len)
{
	k_sem_give(&ictx.sem_response);
	LOG_INF("OK");
}

/* Handler: @EXTERR:<exterror_id> */
static void on_cmd_sockexterror(struct net_buf **buf, u16_t len)
{
	char value[8];
	size_t out_len;

	out_len = net_buf_linearize(value, sizeof(value) - 1, *buf, 0, len);
	value[out_len] = 0;
	ictx.last_error = -atoi(value);
	LOG_ERR("+CME %d", ictx.last_error);
	k_sem_give(&ictx.sem_response);
}

static void on_cmd_write_ready(struct net_buf **buf, u16_t len)
{
	struct ehs6_socket *socket;
	size_t out_len;
	char buffer[20];
	char *temp[2];
	u8_t id;

	out_len = net_buf_linearize(buffer, sizeof(buffer) - 1, *buf, 0, len);
	buffer[out_len] = 0;
	id = atoi(strtok(buffer, ","));
	socket = &ictx.sockets[id];
	temp[0] = strtok(NULL, ",");
	temp[1] = strtok(NULL, ",");
	if (temp[1] == NULL) {
		/* URC respond ready to write like '0,1' */
		LOG_DBG("Write ready.");
	} else {
		/* URC respond ready to accept write like '0,10,0' */
		LOG_DBG("Write data accept ready.");
	}
	k_sem_give(&socket->sem_write_ready);
}

static void on_cmd_read_ready(struct net_buf **buf, u16_t len)
{
	struct ehs6_socket *sock;
	char buffer[10];
	u16_t bytes_read;
	u8_t id;

	/* skip the first line, which should be ^SISR: */
	net_buf_linearize(buffer, sizeof(buffer), *buf, 0, len);
	id = atoi(strtok(buffer, ","));
	sock = &ictx.sockets[id];
	if (sock->is_in_reading) {
		ehs6_read_rx(buf);

		bytes_read = atoi(strtok(NULL, ","));
		LOG_DBG("Reported %d bytes to be read.", bytes_read);
		net_buf_skip(*buf, len);
		net_buf_skipcrlf(buf);
		if (!*buf) {
			LOG_DBG("Data read error.");
			return;
		}
		net_buf_linearize(sock->p_recv_addr, sock->recv_max_len,
				  *buf, 0, bytes_read);
		sock->bytes_read = bytes_read;
		sock->is_in_reading = false;
	} else {
		sock->data_ready = true;
		if (sock->is_polled) {
			k_sem_give(&ictx.sem_poll);
		}
	}
	k_sem_give(&sock->sem_read_ready);
}

static void on_cmd_socket_error(struct net_buf **buf, u16_t len)
{
	char buffer[10];
	size_t out_len;

	out_len = net_buf_linearize(buffer, sizeof(buffer) - 1, *buf, 0, len);
	buffer[out_len] = 0;
	strtok(buffer, ",");
	strtok(buffer, ",");
	ictx.last_error = -atoi(strtok(buffer, ","));
	LOG_ERR("+CME %d", ictx.last_error);
	k_sem_give(&ictx.sem_response);
}

static int net_buf_ncmp(struct net_buf *buf, const u8_t *s2, size_t n)
{
	struct net_buf *frag = buf;
	u16_t offset = 0U;

	while ((n > 0) && (*(frag->data + offset) == *s2) && (*s2 != '\0')) {
		if (offset == frag->len) {
			if (!frag->frags) {
				break;
			}
			frag = frag->frags;
			offset = 0U;
		} else {
			offset++;
		}

		s2++;
		n--;
	}

	return (n == 0) ? 0 : (*(frag->data + offset) - *s2);
}

static inline struct net_buf *read_rx_allocator(s32_t timeout, void *user_data)
{
	return net_buf_alloc((struct net_buf_pool *)user_data, timeout);
}

static void ehs6_read_rx(struct net_buf **buf)
{
	u8_t uart_buffer[MDM_RECV_BUF_SIZE];
	size_t bytes_read = 0;
	int ret;
	u16_t rx_len;

	/* read all of the data from mdm_receiver */
	while (true) {
		ret = mdm_receiver_recv(&ictx.mdm_ctx,
					uart_buffer,
					sizeof(uart_buffer),
					&bytes_read);
		if (ret < 0 || bytes_read == 0) {
			/* mdm_receiver buffer is empty */
			break;
		}

		_hexdump(uart_buffer, bytes_read);

		/* make sure we have storage */
		if (!*buf) {
			*buf = net_buf_alloc(&mdm_recv_pool, BUF_ALLOC_TIMEOUT);
			if (!*buf) {
				LOG_ERR("Can't allocate RX data! "
					    "Skipping data!");
				break;
			}
		}

		rx_len = net_buf_append_bytes(*buf, bytes_read, uart_buffer,
					      BUF_ALLOC_TIMEOUT,
					      read_rx_allocator,
					      &mdm_recv_pool);
		if (rx_len < bytes_read) {
			LOG_ERR("Data was lost! read %u of %u!",
				    rx_len, bytes_read);
		}
	}
}

/* RX thread */
static void ehs6_rx(void)
{
	struct net_buf *rx_buf = NULL;
	struct net_buf *frag = NULL;
	int i;
	u16_t offset, len;

	static const struct cmd_handler handlers[] = {
		/* MODEM Information */
		CMD_HANDLER("AT+CGMI", atcmdinfo_manufacturer),
		CMD_HANDLER("Model: ", atcmdinfo_model),
		CMD_HANDLER("Revision: ", atcmdinfo_revision),
		CMD_HANDLER("AT+CGSN", atcmdinfo_imei),

		/* SOLICITED SOCKET RESPONSES */
		CMD_HANDLER("OK", sockok),
		CMD_HANDLER("+CME ERROR: ", sockexterror),

		/* SOCKET OPERATION RESPONSES */
		CMD_HANDLER("^SISW:", write_ready),
		CMD_HANDLER("^SISR:", read_ready),
		CMD_HANDLER("^SIS:", socket_error),
	};

	while (true) {
		/* wait for incoming data */
		k_sem_take(&ictx.mdm_ctx.rx_sem, K_FOREVER);

		ehs6_read_rx(&rx_buf);

		while (rx_buf) {
			net_buf_skipcrlf(&rx_buf);
			if (!rx_buf) {
				break;
			}

			frag = NULL;
			len = net_buf_findcrlf(rx_buf, &frag, &offset);
			if (!frag) {
				break;
			}

			/* look for matching data handlers */
			i = -1;
			for (i = 0; i < ARRAY_SIZE(handlers); i++) {
				if (net_buf_ncmp(rx_buf, handlers[i].cmd,
						 handlers[i].cmd_len) == 0) {
					/* found a matching handler */
					LOG_DBG("MATCH %s (len:%u)",
						    handlers[i].cmd, len);

					/* skip cmd_len */
					rx_buf = net_buf_skip(rx_buf,
							handlers[i].cmd_len);

					/* locate next cr/lf */
					frag = NULL;
					len = net_buf_findcrlf(rx_buf,
							       &frag, &offset);
					if (!frag) {
						break;
					}

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

static int ehs6_init(struct device *dev)
{
	int i, ret = 0;
	char buffer[100];

	ARG_UNUSED(dev);

	(void)memset(&ictx, 0, sizeof(ictx));
	k_sem_init(&ictx.sem_response, 0, 1);
	k_sem_init(&ictx.sem_poll, 0, 1);
	for (i = 0; i < MDM_MAX_SOCKETS; i++) {
		k_sem_init(&ictx.sockets[i].sem_write_ready, 0, 1);
		k_sem_init(&ictx.sockets[i].sem_read_ready, 0, 1);
	}

	ictx.last_socket_id = 0;

	ictx.mdm_ctx.data_manufacturer = ictx.mdm_manufacturer;
	ictx.mdm_ctx.data_model = ictx.mdm_model;
	ictx.mdm_ctx.data_revision = ictx.mdm_revision;
	ictx.mdm_ctx.data_imei = ictx.mdm_imei;

	ret = mdm_receiver_register(&ictx.mdm_ctx, MDM_UART_DEV_NAME,
				    mdm_recv_buf, sizeof(mdm_recv_buf));
	if (ret < 0) {
		LOG_ERR("Error registering modem receiver (%d)!", ret);
		goto error;
	}

	k_thread_create(&ehs6_rx_thread, ehs6_rx_stack,
			K_THREAD_STACK_SIZEOF(ehs6_rx_stack),
			(k_thread_entry_t) ehs6_rx,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	send_at_cmd("AT+CMEE=1", &ictx.sem_response, MDM_CMD_TIMEOUT);
	for (int i = 0; i < 1; i++) {
		snprintf(buffer, sizeof(buffer),
			 "AT^SIPS=\"all\",\"reset\", %u", i);
		ret = send_at_cmd(buffer, &ictx.sem_response, MDM_CMD_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("Reset internet profile failed.");
			goto error;
		}

		k_sleep(K_SECONDS(1));
		snprintf(buffer, sizeof(buffer), "AT^SICS=%d,conType,GPRS0",
			i);
		ret = send_at_cmd(buffer, &ictx.sem_response, MDM_CMD_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("Connection type setup failed.");
			goto error;
		}

		snprintf(buffer, sizeof(buffer), "AT^SICS=%d,\"inactTO\",\""
			CONFIG_MODEM_EHS6_TIMEOUT_TIME "\"", i);
		ret = send_at_cmd(buffer, &ictx.sem_response, MDM_CMD_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("Connection timeout setup failed.");
			goto error;
		}
#ifdef DNS_SERVER1
		sprintf(buffer, sizeof(buffer), "AT^SICS=%d,dns1,\""
			DNS_SERVER1"\"");
#else
		snprintf(buffer, sizeof(buffer), "AT^SICS=%d,dns1,\""
			CONFIG_MODEM_EHS6_DNS_SERVER_ADDRESS "\"", i);
#endif
		ret = send_at_cmd(buffer, &ictx.sem_response, MDM_CMD_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("dns address setup failed.");
			goto error;
		}

		snprintf(buffer, sizeof(buffer), "AT^SICS=%d,apn,\"	\
			" CONFIG_MODEM_EHS6_APN_NAME "\"", i);
		ret = send_at_cmd(buffer, &ictx.sem_response, MDM_CMD_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("apn setup failed.");
			goto error;
		}
	}
	atomic_clear_bit(ictx.iface->if_dev->flags, NET_IF_UP);
	return 0;
error:
	if (ret == -ETIMEDOUT) {
		return -ETIMEDOUT;
	} else {
		return -EIO;
	}
}

static int ehs6_socket(int family, int type, int proto)
{
	char buffer[sizeof("AT^SISS=#,srvType,\"Socket\"")];
	struct ehs6_socket *sock;
	u8_t id;
	int ret;

	if (family != AF_INET) {
		return -ENOTSUP;
	}

	if (type != SOCK_STREAM && type != SOCK_DGRAM) {
		return -ENOTSUP;
	}

	id = socket_get();
	if (id < 0) {
		return -ENOMEM;
	}

	sock = &ictx.sockets[id];
	sock->ip_proto = proto;
	sock->family = family;
	sock->ip_proto = proto;

	sock->in_use = true;

	snprintf(buffer, sizeof(buffer), "AT^SISS=%1u,srvType,\"Socket\"", id);
	ret = send_at_cmd(buffer, &ictx.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Service type setup failed.");
		goto error;
	}

	snprintf(buffer, sizeof(buffer), "AT^SISS=%d,conId,%d", id, id);
	ret = send_at_cmd(buffer, &ictx.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Service and connection bind failed.");
		goto error;
	}

	return id;
error:
	if (ret == -ETIMEDOUT) {
		return -ETIMEDOUT;
	} else {
		return -EIO;
	}
}

static int ehs6_close(int id)
{
	char buffer[sizeof("AT^SISC=#")];

	snprintf(buffer, sizeof(buffer), "AT^SISC=%u", id);
	send_at_cmd(buffer, &ictx.sem_response, MDM_CMD_TIMEOUT);

	ictx.sockets[id].context = NULL;
	return 0;
}

static int ehs6_connect(int id, const struct sockaddr *addr, socklen_t addrlen)
{

	char buffer[sizeof(
		"AT^SISS=#,address,\"socktcp://###.###.###.###:#####\"")];
	char buffer_addr[sizeof("###.###.###.###")];
	struct ehs6_socket *sock;
	int ret;

	sock = (struct ehs6_socket *)&ictx.sockets[id];
	if (!sock) {
		LOG_ERR("Can not locate socket id: %u", id);
	}

	int port = ntohs((net_sin(addr)->sin_port));

	if (sock->type == SOCK_STREAM) {
		snprintf(buffer, sizeof(buffer),
			"AT^SISS=%d,address,\"socktcp://%s:%d\"", id,
			net_addr_ntop(AF_INET, &net_sin(addr)->sin_addr,
				      buffer_addr, sizeof(buffer_addr)), port);
	} else {
		snprintf(buffer, sizeof(buffer),
			"AT^SISS=%d,address,\"sockudp://%s:%d\"", id,
			net_addr_ntop(AF_INET, &net_sin(addr)->sin_addr,
				      buffer_addr, sizeof(buffer_addr)), port);
	}
	ret = send_at_cmd(buffer, &ictx.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Address and port setup failed.");
		goto error;
	}

	snprintf(buffer, sizeof(buffer), "AT^SISO=%u", id);
	ret = send_at_cmd(buffer, &ictx.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Socket open failed.");
		goto error;
	}

	/* Wait until ^SISW returns 0,1. */
	ret = k_sem_take(&sock->sem_write_ready, MDM_CMD_CONN_TIMEOUT);
	if (ret < 0) {
		ehs6_close(id);
		return ret;
	}
	return 0;
error:
	if (ret == -ETIMEDOUT) {
		return -ETIMEDOUT;
	} else {
		return -EIO;
	}
}

static ssize_t ehs6_sendto(int id, const void *buf, size_t len, int flags,
			   const struct sockaddr *to, socklen_t tolen)
{
	return -ENOTSUP;
}

static ssize_t ehs6_recv(int id, void *buf, size_t max_len, int flags)
{
	struct ehs6_socket *sock = &ictx.sockets[id];
	char buffer_send[sizeof("AT^SISR=#,####")];
	int ret;

	if (max_len > MDM_MAX_BUF_LENGTH) {
		return -EMSGSIZE;
	}

	if (!sock->data_ready) {
		k_sem_take(&sock->sem_read_ready, K_FOREVER);
	}
	sock->data_ready = false;
	k_sem_reset(&sock->sem_read_ready);
	k_sem_reset(&ictx.sem_response);
	sock->is_in_reading = true;
	sock->p_recv_addr = buf;
	sock->recv_max_len = max_len;
	snprintf(buffer_send, sizeof(buffer_send), "AT^SISR=%d,%d", id,
		 max_len);
	ret = send_at_cmd(buffer_send, &sock->sem_read_ready,
			  MDM_CMD_READ_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Read request failed.");
		goto error;
	}
	k_sem_take(&ictx.sem_response, MDM_CMD_READ_TIMEOUT);

	LOG_DBG("Socket read %d bytes.", sock->bytes_read);
	return sock->bytes_read;
error:
	if (ret == -ETIMEDOUT) {
		return -ETIMEDOUT;
	} else {
		return -EIO;
	}
}

static ssize_t ehs6_recvfrom(int id, void *buf, short int len,
			     short int flags, struct sockaddr *from,
			     socklen_t *fromlen)
{
	ARG_UNUSED(from);
	ARG_UNUSED(fromlen);

	return ehs6_recv(id, buf, len, flags);
}

static ssize_t ehs6_send(int id, const void *buf, size_t len, int flags)
{
	struct ehs6_socket *sock;
	char buf_cmd[sizeof("AT^SISW=#,####")];
	int ret;

	if (len > MDM_MAX_BUF_LENGTH) {
		return -EMSGSIZE;
	}

	sock = (struct ehs6_socket *)&ictx.sockets[id];
	if (!sock) {
		LOG_ERR("Can't locate socket for id: %u", id);
		return -EINVAL;
	}

	snprintf(buf_cmd, sizeof(buf_cmd), "AT^SISW=%u,%u", id, len);
	ret = send_at_cmd(buf_cmd, &sock->sem_write_ready, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Write request failed.");
		goto error;
	}

	k_sem_reset(&ictx.sem_response);
	mdm_receiver_send(&ictx.mdm_ctx, buf, len);
	k_sem_take(&ictx.sem_response, MDM_CMD_SEND_TIMEOUT);

	return len;
error:
	if (ret == -ETIMEDOUT) {
		return -ETIMEDOUT;
	} else {
		return -EIO;
	}
}

/* Support for POLLIN only for now. */
int ehs6_poll(struct pollfd *fds, int nfds, int timeout)
{
	struct ehs6_socket *sock;
	u8_t countFound = 0;
	int ret;

	for (int i = 0; i < nfds; i++) {
		ictx.sockets[fds[i].fd].is_polled = true;
	}
	ret = k_sem_take(&ictx.sem_poll, timeout);
	for (int i = 0; i < nfds; i++) {
		sock = &ictx.sockets[fds[i].fd];
		if (sock->data_ready == true) {
			fds[i].revents = POLLIN;
			countFound++;
		}
	}

	for (int i = 0; i < nfds; i++) {
		ictx.sockets[fds[i].fd].is_polled = false;
	}

	if (ret == -EBUSY) {
		return -1;
	} else {
		return countFound;
	}
}

static const struct socket_offload ehs6_socket_ops = {
	.socket = ehs6_socket,
	.close = ehs6_close,
	.connect = ehs6_connect,
	.send = ehs6_send,
	.sendto = ehs6_sendto,
	.recv = ehs6_recv,
	.recvfrom = ehs6_recvfrom,
	.poll = ehs6_poll,
};

/*** OFFLOAD FUNCTIONS ***/

static int offload_get(sa_family_t family,
		       enum net_sock_type type,
		       enum net_ip_protocol ip_proto,
		       struct net_context **context)
{
	return -ENOTSUP;
}

static struct net_offload offload_funcs = {
	.get = offload_get,
};

static inline u8_t *ehs6_get_mac(struct device *dev)
{
	struct ehs6_iface_ctx *ctx = dev->driver_data;

	ctx->mac_addr[0] = 0x00;
	ctx->mac_addr[1] = 0x10;

	UNALIGNED_PUT(sys_cpu_to_be32(sys_rand32_get()),
		      (u32_t *)(ctx->mac_addr + 2));

	return ctx->mac_addr;
}

static void offload_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct ehs6_iface_ctx *ctx = dev->driver_data;

	iface->if_dev->offload = &offload_funcs;
	net_if_set_link_addr(iface, ehs6_get_mac(dev),
			     sizeof(ctx->mac_addr),
			     NET_LINK_ETHERNET);
	ctx->iface = iface;
	socket_offload_register(&ehs6_socket_ops);
}

static struct net_if_api api_funcs = {
	.init	= offload_iface_init,
};

NET_DEVICE_OFFLOAD_INIT(modem_ehs6, "MODEM_EHS6",
			ehs6_init, &ictx,
			NULL, CONFIG_MODEM_EHS6_INIT_PRIORITY, &api_funcs,
			MDM_MAX_DATA_LENGTH);
