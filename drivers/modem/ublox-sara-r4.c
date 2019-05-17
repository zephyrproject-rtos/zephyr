/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_DOMAIN modem_ublox_sara_r4
#define LOG_LEVEL CONFIG_MODEM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_DOMAIN);

#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <zephyr.h>
#include <gpio.h>
#include <device.h>
#include <init.h>

#include <net/net_context.h>
#include <net/net_if.h>
#include <net/net_offload.h>
#include <net/net_pkt.h>
#if defined(CONFIG_NET_IPV6)
#include "ipv6.h"
#endif
#if defined(CONFIG_NET_IPV4)
#include "ipv4.h"
#endif
#if defined(CONFIG_NET_UDP)
#include "udp_internal.h"
#endif

#include <drivers/modem/modem_receiver.h>

#if !defined(CONFIG_MODEM_UBLOX_SARA_R4_MANUAL_MCCMNO)
#define CONFIG_MODEM_UBLOX_SARA_R4_MANUAL_MCCMNO ""
#endif

/* Uncomment the #define below to enable a hexdump of all incoming
 * data from the modem receiver
 */
/* #define ENABLE_VERBOSE_MODEM_RECV_HEXDUMP	1 */

struct mdm_control_pinconfig {
	char *dev_name;
	u32_t pin;
};

#define PINCONFIG(name_, pin_) { \
	.dev_name = name_, \
	.pin = pin_ \
}

/* pin settings */
enum mdm_control_pins {
	MDM_POWER = 0,
	MDM_RESET,
	MAX_MDM_CONTROL_PINS,
};

static const struct mdm_control_pinconfig pinconfig[] = {
	/* MDM_POWER */
	PINCONFIG(DT_UBLOX_SARA_R4_0_MDM_POWER_GPIOS_CONTROLLER,
		  DT_UBLOX_SARA_R4_0_MDM_POWER_GPIOS_PIN),

	/* MDM_RESET */
	PINCONFIG(DT_UBLOX_SARA_R4_0_MDM_RESET_GPIOS_CONTROLLER,
		  DT_UBLOX_SARA_R4_0_MDM_RESET_GPIOS_PIN),
};

#define MDM_UART_DEV_NAME		DT_UBLOX_SARA_R4_0_BUS_NAME

#define MDM_POWER_ENABLE		1
#define MDM_POWER_DISABLE		0
#define MDM_RESET_NOT_ASSERTED		1
#define MDM_RESET_ASSERTED		0

#define MDM_CMD_TIMEOUT			K_SECONDS(5)
#define MDM_CMD_SEND_TIMEOUT		K_SECONDS(10)
#define MDM_CMD_CONN_TIMEOUT		K_SECONDS(31)
#define MDM_REGISTRATION_TIMEOUT	K_SECONDS(180)
#define MDM_PROMPT_CMD_DELAY		K_MSEC(10)

#define MDM_MAX_DATA_LENGTH		1024

#define MDM_RECV_MAX_BUF		30
#define MDM_RECV_BUF_SIZE		128

#define MDM_MAX_SOCKETS			6
#define MDM_BASE_SOCKET_NUM		0

#define MDM_NETWORK_RETRY_COUNT		3
#define MDM_WAIT_FOR_RSSI_COUNT		10
#define MDM_WAIT_FOR_RSSI_DELAY		K_SECONDS(2)

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

#define RSSI_TIMEOUT_SECS		30

NET_BUF_POOL_DEFINE(mdm_recv_pool, MDM_RECV_MAX_BUF, MDM_RECV_BUF_SIZE,
		    0, NULL);

static u8_t mdm_recv_buf[MDM_MAX_DATA_LENGTH];

/* RX thread structures */
K_THREAD_STACK_DEFINE(modem_rx_stack,
		       CONFIG_MODEM_UBLOX_SARA_R4_RX_STACK_SIZE);
struct k_thread modem_rx_thread;

/* RX thread work queue */
K_THREAD_STACK_DEFINE(modem_workq_stack,
		      CONFIG_MODEM_UBLOX_SARA_R4_RX_WORKQ_STACK_SIZE);
static struct k_work_q modem_workq;

struct modem_socket {
	struct net_context *context;
	sa_family_t family;
	enum net_sock_type type;
	enum net_ip_protocol ip_proto;
	struct sockaddr src;
	struct sockaddr dst;
	int dst_port;

	int socket_id;

	/** semaphore */
	struct k_sem sock_send_sem;

	/** socket callbacks */
	struct k_work recv_cb_work;
	net_context_recv_cb_t recv_cb;
	struct net_pkt *recv_pkt;
	void *recv_user_data;
};

struct modem_iface_ctx {
	struct net_if *iface;
	u8_t mac_addr[6];

	/* GPIO PORT devices */
	struct device *gpio_port_dev[MAX_MDM_CONTROL_PINS];

	/* RX specific attributes */
	struct mdm_receiver_context mdm_ctx;

	/* socket data */
	struct modem_socket sockets[MDM_MAX_SOCKETS];
	int last_socket_id;
	int last_error;

	/* semaphores */
	struct k_sem response_sem;

	/* RSSI work */
	struct k_delayed_work rssi_query_work;

	/* modem data */
	char mdm_manufacturer[MDM_MANUFACTURER_LENGTH];
	char mdm_model[MDM_MODEL_LENGTH];
	char mdm_revision[MDM_REVISION_LENGTH];
	char mdm_imei[MDM_IMEI_LENGTH];

	/* modem state */
	int ev_creg;
};

struct cmd_handler {
	const char *cmd;
	u16_t cmd_len;
	void (*func)(struct net_buf **buf, u16_t len);
};

static struct modem_iface_ctx ictx;

static void modem_read_rx(struct net_buf **buf);

/*** Verbose Debugging Functions ***/
#if defined(ENABLE_VERBOSE_MODEM_RECV_HEXDUMP)
static inline void hexdump(const u8_t *packet, size_t length)
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
#define hexdump(...)
#endif

static struct modem_socket *socket_get(void)
{
	int i;
	struct modem_socket *sock = NULL;

	for (i = 0; i < MDM_MAX_SOCKETS; i++) {
		if (!ictx.sockets[i].context) {
			sock = &ictx.sockets[i];
			break;
		}
	}

	return sock;
}

static struct modem_socket *socket_from_id(int socket_id)
{
	int i;
	struct modem_socket *sock = NULL;

	if (socket_id < MDM_BASE_SOCKET_NUM) {
		return NULL;
	}

	for (i = 0; i < MDM_MAX_SOCKETS; i++) {
		if (ictx.sockets[i].socket_id == socket_id) {
			sock = &ictx.sockets[i];
			break;
		}
	}

	return sock;
}

static void socket_put(struct modem_socket *sock)
{
	if (!sock) {
		return;
	}

	sock->context = NULL;
	sock->socket_id = MDM_BASE_SOCKET_NUM - 1;
	(void)memset(&sock->src, 0, sizeof(struct sockaddr));
	(void)memset(&sock->dst, 0, sizeof(struct sockaddr));
}

static char *modem_sprint_ip_addr(const struct sockaddr *addr)
{
	static char buf[NET_IPV6_ADDR_LEN];

#if defined(CONFIG_NET_IPV6)
	if (addr->sa_family == AF_INET6) {
		return net_addr_ntop(AF_INET6, &net_sin6(addr)->sin6_addr,
				     buf, sizeof(buf));
	} else
#endif
#if defined(CONFIG_NET_IPV4)
	if (addr->sa_family == AF_INET) {
		return net_addr_ntop(AF_INET, &net_sin(addr)->sin_addr,
				     buf, sizeof(buf));
	} else
#endif
	{
		LOG_ERR("Unknown IP address family:%d", addr->sa_family);
		return NULL;
	}
}

/* Send an AT command with a series of response handlers */
static int send_at_cmd(struct modem_socket *sock,
			const u8_t *data, int timeout)
{
	int ret;

	ictx.last_error = 0;

	LOG_DBG("OUT: [%s]", data);
	mdm_receiver_send(&ictx.mdm_ctx, data, strlen(data));
	mdm_receiver_send(&ictx.mdm_ctx, "\r\n", 2);

	if (timeout == K_NO_WAIT) {
		return 0;
	}

	if (!sock) {
		k_sem_reset(&ictx.response_sem);
		ret = k_sem_take(&ictx.response_sem, timeout);
	} else {
		k_sem_reset(&sock->sock_send_sem);
		ret = k_sem_take(&sock->sock_send_sem, timeout);
	}

	if (ret == 0) {
		ret = ictx.last_error;
	} else if (ret == -EAGAIN) {
		ret = -ETIMEDOUT;
	}

	return ret;
}

static int send_data(struct modem_socket *sock,
		     const struct sockaddr *dst_addr, int dst_port,
		     struct net_pkt *pkt)
{
	int ret, i;
	struct net_buf *frag;
	char buf[sizeof("AT+USO**=#,!###.###.###.###!,#####,####\r\n")];

	if (!sock) {
		return -EINVAL;
	}

	ictx.last_error = 0;

	frag = pkt->frags;

	/* use SOCKWRITE with binary mode formatting */
	if (sock->ip_proto == IPPROTO_UDP) {
		snprintk(buf, sizeof(buf), "AT+USOST=%d,\"%s\",%u,%u\r\n",
			 sock->socket_id, modem_sprint_ip_addr(dst_addr),
			 dst_port, net_buf_frags_len(frag));
	} else {
		snprintk(buf, sizeof(buf), "AT+USOWR=%d,%u\r\n",
			 sock->socket_id, net_buf_frags_len(frag));
	}
	mdm_receiver_send(&ictx.mdm_ctx, buf, strlen(buf));

	/* Slight pause per spec so that @ prompt is received */
	k_sleep(MDM_PROMPT_CMD_DELAY);

	/* Loop through packet data and send */
	while (frag) {
		if (frag->len > 0) {
			/*
			 * HACK: Apparently enabling HEX transmit mode also
			 * affects the BINARY send method.  We need to encode
			 * the "binary" data as HEX values here
			 * TODO: Something faster here.
			 */
			for (i = 0; i < frag->len; i++) {
				snprintk(buf, sizeof(buf), "%02x", frag->data[i]);
				mdm_receiver_send(&ictx.mdm_ctx, buf, 2);
			}
		}
		frag = frag->frags;
	}

	k_sem_reset(&sock->sock_send_sem);
	ret = k_sem_take(&sock->sock_send_sem, MDM_CMD_SEND_TIMEOUT);
	if (ret == 0) {
		ret = ictx.last_error;
	} else if (ret == -EAGAIN) {
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

/*** UDP / TCP Helper Function ***/

/* Setup IP header data to be used by some network applications.
 * While much is dummy data, some fields such as dst, port and family are
 * important.
 * Return the IP + protocol header length.
 */
static int pkt_setup_ip_data(struct net_pkt *pkt, struct modem_socket *sock)
{
	int hdr_len = 0;
	u16_t src_port = 0U;

#if defined(CONFIG_NET_IPV6)
	if (net_pkt_family(pkt) == AF_INET6) {
		if (net_ipv6_create(
			    pkt,
			    &((struct sockaddr_in6 *)&sock->dst)->sin6_addr,
			    &((struct sockaddr_in6 *)&sock->src)->sin6_addr)) {
			return -1;
		}

		src_port = net_sin6(&sock->dst)->sin6_port;
		hdr_len = sizeof(struct net_ipv6_hdr);
	} else
#endif
#if defined(CONFIG_NET_IPV4)
	if (net_pkt_family(pkt) == AF_INET) {
		if (net_ipv4_create(
			    pkt,
			    &((struct sockaddr_in *)&sock->dst)->sin_addr,
			    &((struct sockaddr_in *)&sock->src)->sin_addr)) {
			return -1;
		}

		src_port = net_sin(&sock->dst)->sin_port;
		hdr_len = sizeof(struct net_ipv4_hdr);
	} else
#endif
	{
		/* no error here as hdr_len is checked later for 0 value */
	}

#if defined(CONFIG_NET_UDP)
	if (sock->ip_proto == IPPROTO_UDP) {
		if (net_udp_create(pkt, src_port, sock->dst_port)) {
			return -1;
		}

		hdr_len += NET_UDPH_LEN;
	} else
#endif
#if defined(CONFIG_NET_TCP)
	if (sock->ip_proto == IPPROTO_TCP) {
		NET_PKT_DATA_ACCESS_DEFINE(tcp_access, struct net_tcp_hdr);
		struct net_tcp_hdr *tcp;

		tcp = (struct net_tcp_hdr *)net_pkt_get_data(pkt, &tcp_access);
		if (!tcp) {
			return -1;
		}

		(void)memset(tcp, 0, NET_TCPH_LEN);

		/* Setup TCP header */
		tcp->src_port = src_port;
		tcp->dst_port = sock->dst_port;

		if (net_pkt_set_data(pkt, &tcp_access)) {
			return -1;
		}

		hdr_len += NET_TCPH_LEN;
	} else
#endif /* CONFIG_NET_TCP */
	{
		/* no error here as hdr_len is checked later for 0 value */
	}

	return hdr_len;
}

/*** MODEM RESPONSE HANDLERS ***/

/* Last Socket ID Handler */
static void on_cmd_atcmdecho(struct net_buf **buf, u16_t len)
{
	char value[2];
	/* make sure only a single digit is picked up for socket_id */
	value[0] = net_buf_pull_u8(*buf);
	ictx.last_socket_id = atoi(value);
}

/* Echo Handler for commands without related sockets */
static void on_cmd_atcmdecho_nosock(struct net_buf **buf, u16_t len)
{
	/* clear last_socket_id */
	ictx.last_socket_id = MDM_BASE_SOCKET_NUM - 1;
}

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

static void on_cmd_atcmdecho_nosock_imei(struct net_buf **buf, u16_t len)
{
	struct net_buf *frag = NULL;
	u16_t offset;
	size_t out_len;

	/* skip CR/LF */
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

/* Handler: +CESQ: <rxlev>[0],<ber>[1],<rscp>[2],<ecn0>[3],<rsrq>[4],<rsrp>[5] */
static void on_cmd_atcmdinfo_rssi(struct net_buf **buf, u16_t len)
{
	int i = 0, rssi, param_count = 0;
	size_t value_size;
	char value[12];

	value_size = sizeof(value);
	while (*buf && len > 0 && param_count < 6) {
		i = 0;
		(void)memset(value, 0, value_size);

		while (*buf && len > 0 && i < value_size) {
			value[i] = net_buf_pull_u8(*buf);
			len--;
			if (!(*buf)->len) {
				*buf = net_buf_frag_del(NULL, *buf);
			}

			/* "," marks the end of each value */
			if (value[i] == ',') {
				value[i] = '\0';
				break;
			}

			i++;
		}

		if (i == value_size) {
			i = -1;
			break;
		}

		param_count++;
	}

	if (param_count == 6 && i > 0) {
		rssi = atoi(value);
		if (rssi >= 0 && rssi <= 97) {
			ictx.mdm_ctx.data_rssi = -140 + rssi;
		} else {
			ictx.mdm_ctx.data_rssi = -1000;
		}

		LOG_INF("RSSI: %d", ictx.mdm_ctx.data_rssi);
		return;
	}

	LOG_WRN("Bad format found for RSSI");
	ictx.mdm_ctx.data_rssi = -1000;
}

/* Handler: OK */
static void on_cmd_sockok(struct net_buf **buf, u16_t len)
{
	struct modem_socket *sock = NULL;

	ictx.last_error = 0;
	sock = socket_from_id(ictx.last_socket_id);
	if (!sock) {
		k_sem_give(&ictx.response_sem);
	} else {
		k_sem_give(&sock->sock_send_sem);
	}
}

/* Handler: ERROR */
static void on_cmd_sockerror(struct net_buf **buf, u16_t len)
{
	struct modem_socket *sock = NULL;

	ictx.last_error = -EIO;
	sock = socket_from_id(ictx.last_socket_id);
	if (!sock) {
		k_sem_give(&ictx.response_sem);
	} else {
		k_sem_give(&sock->sock_send_sem);
	}
}

/* Handler: +USOCR: <socket_id> */
static void on_cmd_sockcreate(struct net_buf **buf, u16_t len)
{
	char value[2];
	struct modem_socket *sock = NULL;

	/* look up new socket by special id */
	sock = socket_from_id(MDM_MAX_SOCKETS + 1);
	if (sock) {
		/* make sure only a single digit is picked up for socket_id */
		value[0] = net_buf_pull_u8(*buf);
		value[1] = 0;
		sock->socket_id = atoi(value);
	}

	/* don't give back semaphore -- OK to follow */
}

/* Handler: +USO[WR|ST]: <socket_id>,<length> */
static void on_cmd_sockwrite(struct net_buf **buf, u16_t len)
{
	char value[2];

	/* TODO: check length against original send length */

	if (!*buf) {
		return;
	}

	/* make sure only a single digit is picked up for socket_id */
	value[0] = net_buf_pull_u8(*buf);
	value[1] = 0;

	ictx.last_socket_id = atoi(value);
	if (ictx.last_socket_id < MDM_BASE_SOCKET_NUM) {
		return;
	}

	/* don't give back semaphore -- OK to follow */
}

static void sockreadrecv_cb_work(struct k_work *work)
{
	struct modem_socket *sock = NULL;
	struct net_pkt *pkt;

	sock = CONTAINER_OF(work, struct modem_socket, recv_cb_work);

	/* return data */
	pkt = sock->recv_pkt;
	sock->recv_pkt = NULL;
	if (sock->recv_cb) {
		sock->recv_cb(sock->context, pkt, NULL, NULL,
			      0, sock->recv_user_data);
	} else {
		net_pkt_unref(pkt);
	}
}

/* Common code for +USOR[D|F] */
static void on_cmd_sockread_common(int socket_id, struct net_buf **buf,
				   u16_t len)
{
	struct modem_socket *sock = NULL;
	int i, actual_length, hdr_len = 0;
	size_t value_size;
	char value[10];
	u8_t c = 0U;

	/* comma marks the end of length */
	i = 0;
	value_size = sizeof(value);
	(void)memset(value, 0, value_size);
	while (*buf && i < value_size) {
		value[i] = net_buf_pull_u8(*buf);
		len--;
		if (!(*buf)->len) {
			*buf = net_buf_frag_del(NULL, *buf);
		}

		if (value[i] == ',') {
			break;
		}

		i++;
	}

	/* make sure we still have buf data, the last pulled character was
	 * a comma and that the next char in the buffer is a quote.
	 */
	if (!*buf || value[i] != ',' || *(*buf)->data != '\"') {
		LOG_ERR("Incorrect format! Ignoring data!");
		return;
	}

	/* clear the comma */
	value[i] = '\0';
	actual_length = atoi(value);

	/* skip quote */
	len--;
	net_buf_pull_u8(*buf);
	if (!(*buf)->len) {
		*buf = net_buf_frag_del(NULL, *buf);
	}

	/* check that we have enough data */
	if (!*buf || len > (actual_length * 2) + 1) {
		LOG_ERR("Incorrect format! Ignoring data!");
		return;
	}

	sock = socket_from_id(socket_id);
	if (!sock) {
		LOG_ERR("Socket not found! (%d)", socket_id);
		return;
	}

	/* update last_socket_id */
	ictx.last_socket_id = socket_id;

	/* allocate an RX pkt */
	sock->recv_pkt = net_pkt_rx_alloc_with_buffer(
			net_context_get_iface(sock->context),
			actual_length, sock->family, sock->ip_proto,
			BUF_ALLOC_TIMEOUT);
	if (!sock->recv_pkt) {
		LOG_ERR("Failed net_pkt_get_reserve_rx!");
		return;
	}

	/* set pkt data */
	net_pkt_set_context(sock->recv_pkt, sock->context);

	/* add IP / protocol headers */
	hdr_len = pkt_setup_ip_data(sock->recv_pkt, sock);

	/* move hex encoded data from the buffer to the recv_pkt */
	for (i = 0; i < actual_length * 2; i++) {
		char c2 = *(*buf)->data;

		if (isdigit(c2)) {
			c += c2 - '0';
		} else if (isalpha(c2)) {
			c += c2 - (isupper(c2) ? 'A' - 10 : 'a' - 10);
		} else {
			/* TODO: unexpected input! skip? */
		}

		if (i % 2) {
			if (net_pkt_write_u8(sock->recv_pkt, c)) {
				LOG_ERR("Unable to add data! Aborting!");
				net_pkt_unref(sock->recv_pkt);
				sock->recv_pkt = NULL;
				return;
			}

			c = 0U;
		} else {
			c = c << 4;
		}

		/* pull data from buf and advance to the next frag if needed */
		net_buf_pull_u8(*buf);
		if (!(*buf)->len) {
			*buf = net_buf_frag_del(NULL, *buf);
		}
	}

	net_pkt_cursor_init(sock->recv_pkt);
	net_pkt_set_overwrite(sock->recv_pkt, true);

	if (hdr_len > 0) {
		net_pkt_skip(sock->recv_pkt, hdr_len);
	}

	/* Let's do the callback processing in a different work queue in
	 * case the app takes a long time.
	 */
	k_work_submit_to_queue(&modem_workq, &sock->recv_cb_work);
}

/*
 * Handler: +USORF: <socket_id>,<remote_ip_addr>,<remote_port>,<length>,
 *          "<hex_data>"
*/
static void on_cmd_sockread_udp(struct net_buf **buf, u16_t len)
{
	int socket_id;
	char value[2];

	/* make sure only a single digit is picked up for socket_id */
	value[0] = net_buf_pull_u8(*buf);
	len--;

	/* skip first comma */
	net_buf_pull_u8(*buf);
	len--;

	value[1] = 0;
	socket_id = atoi(value);
	if (socket_id < MDM_BASE_SOCKET_NUM - 1) {
		return;
	}

	/* TODO: handle remote_ip_addr */

	/* skip remote_ip */
	while (*buf && len > 0 && net_buf_pull_u8(*buf) != ',') {
		len--;
	}

	len--;
	/* skip remote_port */
	while (*buf && len > 0 && net_buf_pull_u8(*buf) != ',') {
		len--;
	}

	len--;
	return on_cmd_sockread_common(socket_id, buf, len);
}

/* Handler: +USORD: <socket_id>,<length>,"<hex_data>" */
static void on_cmd_sockread_tcp(struct net_buf **buf, u16_t len)
{
	int socket_id;
	char value[2];

	/* make sure only a single digit is picked up for socket_id */
	value[0] = net_buf_pull_u8(*buf);
	len--;

	/* skip first comma */
	net_buf_pull_u8(*buf);
	len--;

	value[1] = 0;
	socket_id = atoi(value);
	if (socket_id < MDM_BASE_SOCKET_NUM) {
		return;
	}

	return on_cmd_sockread_common(socket_id, buf, len);
}

/* Handler: +UUSOCL: <socket_id> */
static void on_cmd_socknotifyclose(struct net_buf **buf, u16_t len)
{
	char value[2];
	int socket_id;

	/* make sure only a single digit is picked up for socket_id */
	value[0] = net_buf_pull_u8(*buf);
	len--;
	value[1] = 0;

	socket_id = atoi(value);
	if (socket_id < MDM_BASE_SOCKET_NUM) {
		return;
	}

	/* TODO: handle URC socket close */
}

/* Handler: +UUSOR[D|F]: <socket_id>,<length> */
static void on_cmd_socknotifydata(struct net_buf **buf, u16_t len)
{
	int socket_id, left_bytes;
	size_t out_len;
	char value[sizeof("#,####\r")];
	char sendbuf[sizeof("AT+USOR*=#,#####\r")];
	struct modem_socket *sock = NULL;

	/* make sure only a single digit is picked up for socket_id */
	value[0] = net_buf_pull_u8(*buf);
	len--;
	value[1] = 0;

	socket_id = atoi(value);
	if (socket_id < MDM_BASE_SOCKET_NUM) {
		return;
	}

	/* skip first comma */
	net_buf_pull_u8(*buf);
	len--;

	/* Second parameter is length */
	out_len = net_buf_linearize(value, sizeof(value) - 1, *buf, 0, len);
	value[out_len] = 0;
	left_bytes = atoi(value);

	sock = socket_from_id(socket_id);
	if (!sock) {
		LOG_ERR("Unable to find socket_id:%d", socket_id);
		return;
	}

	if (left_bytes > 0) {
		LOG_DBG("socket_id:%d left_bytes:%d", socket_id, left_bytes);

		snprintk(sendbuf, sizeof(sendbuf), "AT+USOR%s=%d,%d",
			 (sock->ip_proto == IPPROTO_UDP) ? "F" : "D",
			 sock->socket_id, left_bytes);

		/* We entered this trigger due to an unsolicited modem response.
		 * When we send the AT+USOR* command it won't generate an
		 * "OK" response directly.  The modem will respond with
		 * "+USOR*: ..." and the data requested and then "OK" or
		 * "ERROR".  Let's not wait here by passing in a timeout to
		 * send_at_cmd().  Instead, when the resulting response is
		 * received, we trigger on_cmd_sockread() to handle it.
		 */
		send_at_cmd(sock, sendbuf, K_NO_WAIT);
	}
}

static void on_cmd_socknotifycreg(struct net_buf **buf, u16_t len)
{
	char value[8];
	size_t out_len;

	out_len = net_buf_linearize(value, sizeof(value) - 1, *buf, 0, len);
	value[out_len] = 0;
	ictx.ev_creg = atoi(value);
	LOG_DBG("CREG:%d", ictx.ev_creg);
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

static void modem_read_rx(struct net_buf **buf)
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

		hexdump(uart_buffer, bytes_read);

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
static void modem_rx(void)
{
	struct net_buf *rx_buf = NULL;
	struct net_buf *frag = NULL;
	int i;
	u16_t offset, len;

	static const struct cmd_handler handlers[] = {
		/* NON-SOCKET COMMAND ECHOES to clear last_socket_id */
		CMD_HANDLER("ATE1", atcmdecho_nosock),
		CMD_HANDLER("AT+CFUN=", atcmdecho_nosock),
		CMD_HANDLER("AT+CREG=", atcmdecho_nosock),
		CMD_HANDLER("AT+UDCONF=", atcmdecho_nosock),
		CMD_HANDLER("ATI", atcmdecho_nosock),
		CMD_HANDLER("AT+CGDCONT=", atcmdecho_nosock),
		CMD_HANDLER("AT+COPS=", atcmdecho_nosock),
		CMD_HANDLER("AT+CESQ", atcmdecho_nosock),
		CMD_HANDLER("AT+USOCR=", atcmdecho_nosock),
		CMD_HANDLER("AT+CGSN", atcmdecho_nosock_imei),

		/* SOCKET COMMAND ECHOES for last_socket_id processing */
		CMD_HANDLER("AT+USOCO=", atcmdecho),
		CMD_HANDLER("AT+USOWR=", atcmdecho),
		CMD_HANDLER("AT+USOST=", atcmdecho),
		CMD_HANDLER("AT+USOCL=", atcmdecho),

		/* MODEM Information */
		CMD_HANDLER("Manufacturer: ", atcmdinfo_manufacturer),
		CMD_HANDLER("Model: ", atcmdinfo_model),
		CMD_HANDLER("Revision: ", atcmdinfo_revision),
		CMD_HANDLER("+CESQ: ", atcmdinfo_rssi),

		/* SOLICITED SOCKET RESPONSES */
		CMD_HANDLER("OK", sockok),
		CMD_HANDLER("ERROR", sockerror),
		CMD_HANDLER("+USOCR: ", sockcreate),
		CMD_HANDLER("+USOWR: ", sockwrite),
		CMD_HANDLER("+USOST: ", sockwrite),
		CMD_HANDLER("+USORD: ", sockread_tcp),
		CMD_HANDLER("+USORF: ", sockread_udp),

		/* UNSOLICITED RESPONSE CODES */
		CMD_HANDLER("+UUSOCL: ", socknotifyclose),
		CMD_HANDLER("+UUSORD: ", socknotifydata),
		CMD_HANDLER("+UUSORF: ", socknotifydata),
		CMD_HANDLER("+CREG: ", socknotifycreg),
	};

	while (true) {
		/* wait for incoming data */
		k_sem_take(&ictx.mdm_ctx.rx_sem, K_FOREVER);

		modem_read_rx(&rx_buf);

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

					/*
					 * We've handled the current line
					 * and need to exit the "search for
					 * handler loop".  Let's skip any
					 * "extra" data and look for the next
					 * CR/LF, leaving us ready for the
					 * next handler search.  Ignore the
					 * length returned.
					 */
					(void)net_buf_findcrlf(rx_buf,
							       &frag, &offset);
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

static int modem_pin_init(void)
{
	LOG_INF("Setting Modem Pins");

	gpio_pin_configure(ictx.gpio_port_dev[MDM_RESET],
			  pinconfig[MDM_RESET].pin, GPIO_DIR_OUT);
	gpio_pin_configure(ictx.gpio_port_dev[MDM_POWER],
			  pinconfig[MDM_POWER].pin, GPIO_DIR_OUT);

	LOG_DBG("MDM_RESET_PIN -> NOT_ASSERTED");
	gpio_pin_write(ictx.gpio_port_dev[MDM_RESET],
		       pinconfig[MDM_RESET].pin, MDM_RESET_NOT_ASSERTED);

	LOG_DBG("MDM_POWER_PIN -> DISABLE");
	gpio_pin_write(ictx.gpio_port_dev[MDM_POWER],
		       pinconfig[MDM_POWER].pin, MDM_POWER_DISABLE);
	/* make sure module is powered off */
	k_sleep(K_SECONDS(12));

	LOG_DBG("MDM_POWER_PIN -> ENABLE");
	gpio_pin_write(ictx.gpio_port_dev[MDM_POWER],
		       pinconfig[MDM_POWER].pin, MDM_POWER_ENABLE);
	k_sleep(K_SECONDS(1));

	LOG_DBG("MDM_POWER_PIN -> DISABLE");
	gpio_pin_write(ictx.gpio_port_dev[MDM_POWER],
		       pinconfig[MDM_POWER].pin, MDM_POWER_DISABLE);
	k_sleep(K_SECONDS(1));

	LOG_DBG("MDM_POWER_PIN -> ENABLE");
	gpio_pin_write(ictx.gpio_port_dev[MDM_POWER],
		       pinconfig[MDM_POWER].pin, MDM_POWER_ENABLE);
	k_sleep(K_SECONDS(10));

	gpio_pin_configure(ictx.gpio_port_dev[MDM_POWER],
			  pinconfig[MDM_POWER].pin, GPIO_DIR_IN);

	LOG_INF("... Done!");

	return 0;
}

static void modem_rssi_query_work(struct k_work *work)
{
	int ret;

	/* query modem RSSI */
	ret = send_at_cmd(NULL, "AT+CESQ", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("AT+CESQ ret:%d", ret);
	}

	/* re-start RSSI query work */
	k_delayed_work_submit_to_queue(&modem_workq,
				       &ictx.rssi_query_work,
				       K_SECONDS(RSSI_TIMEOUT_SECS));
}

static void modem_reset(void)
{
	int ret = 0, retry_count = 0, counter = 0;

	/* bring down network interface */
	atomic_clear_bit(ictx.iface->if_dev->flags, NET_IF_UP);

restart:
	/* stop RSSI delay work */
	k_delayed_work_cancel(&ictx.rssi_query_work);

	modem_pin_init();

	LOG_INF("Waiting for modem to respond");

	/* Give the modem a while to start responding to simple 'AT' commands.
	 * Also wait for CSPS=1 or RRCSTATE=1 notification
	 */
	ret = -1;
	while (counter++ < 50 && ret < 0) {
		k_sleep(K_SECONDS(2));
		ret = send_at_cmd(NULL, "AT", MDM_CMD_TIMEOUT);
		if (ret < 0 && ret != -ETIMEDOUT) {
			break;
		}
	}

	if (ret < 0) {
		LOG_ERR("MODEM WAIT LOOP ERROR: %d", ret);
		goto error;
	}

	/* echo on */
	ret = send_at_cmd(NULL, "ATE1", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("ATE1 ret:%d", ret);
		goto error;
	}

	/* stop functionality */
	ret = send_at_cmd(NULL, "AT+CFUN=0", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("AT+CFUN=0 ret:%d", ret);
		goto error;
	}

#if defined(CONFIG_BOARD_PARTICLE_BORON)
	/* use external SIM */
	ret = send_at_cmd(NULL, "AT+UGPIOC=23,0,0", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("AT+UGPIOC=23,0,0 ret:%d", ret);
		goto error;
	}

	/* Let SIM settle */
	k_sleep(MDM_CMD_TIMEOUT);
#endif

	/* UNC messages for registration */
	ret = send_at_cmd(NULL, "AT+CREG=1", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("AT+CREG=1 ret:%d", ret);
		goto error;
	}

	/* HEX receive data mode */
	ret = send_at_cmd(NULL, "AT+UDCONF=1,1", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("AT+UDCONF=1 ret:%d", ret);
	}

	/* query modem info */
	LOG_INF("Querying modem information");
	ret = send_at_cmd(NULL, "ATI", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("ATI ret:%d", ret);
		goto error;
	}

	/* query modem IMEI */
	ret = send_at_cmd(NULL, "AT+CGSN", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("AT+CGSN ret:%d", ret);
		goto error;
	}

	/* setup PDP context definition */

	ret = send_at_cmd(NULL, "AT+CGDCONT=1,\"IP\",\""
				CONFIG_MODEM_UBLOX_SARA_R4_APN "\"",
			  MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("AT+CGDCONT ret:%d", ret);
		goto error;
	}

	/* start functionality */
	ret = send_at_cmd(NULL, "AT+CFUN=1", MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("AT+CFUN=1 ret:%d", ret);
		goto error;
	}

	if (strlen(CONFIG_MODEM_UBLOX_SARA_R4_MANUAL_MCCMNO) > 0) {
		/* use manual MCC/MNO entry */
		ret = send_at_cmd(NULL, "AT+COPS=1,2,\""
				  CONFIG_MODEM_UBLOX_SARA_R4_MANUAL_MCCMNO "\"",
				  MDM_CMD_TIMEOUT);
	} else {
		/* register operator automatically */
		ret = send_at_cmd(NULL, "AT+COPS=0,0", MDM_REGISTRATION_TIMEOUT);
	}

	if (ret < 0) {
		LOG_ERR("AT+COPS ret:%d", ret);
		goto error;
	}

	LOG_INF("Waiting for network");

	/* wait for +CREG: 1 notification (20 seconds max) */
	counter = 0;
	while (counter++ < 20 && ictx.ev_creg != 1) {
		k_sleep(K_SECONDS(1));
	}

	/* query modem RSSI */
	modem_rssi_query_work(NULL);
	k_sleep(MDM_WAIT_FOR_RSSI_DELAY);

	counter = 0;
	/* wait for RSSI < 0 and > -1000 */
	while (counter++ < MDM_WAIT_FOR_RSSI_COUNT &&
	       (ictx.mdm_ctx.data_rssi >= 0 ||
		ictx.mdm_ctx.data_rssi <= -1000)) {
		/* stop RSSI delay work */
		k_delayed_work_cancel(&ictx.rssi_query_work);
		modem_rssi_query_work(NULL);
		k_sleep(MDM_WAIT_FOR_RSSI_DELAY);
	}

	if (ictx.mdm_ctx.data_rssi >= 0 || ictx.mdm_ctx.data_rssi <= -1000) {
		retry_count++;
		if (retry_count >= MDM_NETWORK_RETRY_COUNT) {
			LOG_ERR("Failed network init.  Too many attempts!");
			ret = -ENETUNREACH;
			goto error;
		}

		LOG_ERR("Failed network init.  Restarting process.");
		goto restart;
	}

	LOG_INF("Network is ready.");

	/* Set iface up */
	net_if_up(ictx.iface);

error:
	return;
}

static int modem_init(struct device *dev)
{
	int i, ret = 0;

	ARG_UNUSED(dev);

	/* check for valid pinconfig */
	__ASSERT(ARRAY_SIZE(pinconfig) == MAX_MDM_CONTROL_PINS,
	       "Incorrect modem pinconfig!");

	(void)memset(&ictx, 0, sizeof(ictx));
	for (i = 0; i < MDM_MAX_SOCKETS; i++) {
		k_work_init(&ictx.sockets[i].recv_cb_work,
			    sockreadrecv_cb_work);
		k_sem_init(&ictx.sockets[i].sock_send_sem, 0, 1);
		ictx.sockets[i].socket_id = MDM_BASE_SOCKET_NUM - 1;
	}
	k_sem_init(&ictx.response_sem, 0, 1);

	/* initialize the work queue */
	k_work_q_start(&modem_workq,
		       modem_workq_stack,
		       K_THREAD_STACK_SIZEOF(modem_workq_stack),
		       K_PRIO_COOP(7));

	ictx.last_socket_id = MDM_BASE_SOCKET_NUM - 1;

	/* setup port devices and pin directions */
	for (i = 0; i < MAX_MDM_CONTROL_PINS; i++) {
		ictx.gpio_port_dev[i] =
				device_get_binding(pinconfig[i].dev_name);
		if (!ictx.gpio_port_dev[i]) {
			LOG_ERR("gpio port (%s) not found!",
				    pinconfig[i].dev_name);
			return -ENODEV;
		}
	}

	/* Set modem data storage */
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

	/* start RX thread */
	k_thread_create(&modem_rx_thread, modem_rx_stack,
			K_THREAD_STACK_SIZEOF(modem_rx_stack),
			(k_thread_entry_t) modem_rx,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	/* init RSSI query */
	k_delayed_work_init(&ictx.rssi_query_work, modem_rssi_query_work);

	modem_reset();

error:
	return ret;
}

/*** OFFLOAD FUNCTIONS ***/

static int offload_get(sa_family_t family,
		       enum net_sock_type type,
		       enum net_ip_protocol ip_proto,
		       struct net_context **context)
{
	int ret;
	char buf[sizeof("AT+USOCR=#,#####\r")];
	struct modem_socket *sock = NULL;
	int local_port = 0;

	/* new socket */
	sock = socket_get();
	if (!sock) {
		return -ENOMEM;
	}

	(*context)->offload_context = sock;
	sock->family = family;
	sock->type = type;
	sock->ip_proto = ip_proto;
	sock->context = *context;
	sock->socket_id = MDM_MAX_SOCKETS + 1; /* socket # needs assigning */

	local_port = ntohs(net_sin((struct sockaddr *)&(*context)->local)->sin_port);
	if (local_port > 0) {
		snprintk(buf, sizeof(buf), "AT+USOCR=%d,%d", ip_proto,
			 local_port);
	} else {
		snprintk(buf, sizeof(buf), "AT+USOCR=%d", ip_proto);
	}

	ret = send_at_cmd(NULL, buf, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", buf, ret);
		socket_put(sock);
	}

	return ret;
}

static int offload_bind(struct net_context *context,
			const struct sockaddr *addr,
			socklen_t addrlen)
{
	struct modem_socket *sock = NULL;

	if (!context) {
		return -EINVAL;
	}

	sock = (struct modem_socket *)context->offload_context;
	if (!sock) {
		LOG_ERR("Can't locate socket for net_ctx:%p!", context);
		return -EINVAL;
	}

	/* save bind address information */
	sock->src.sa_family = addr->sa_family;
#if defined(CONFIG_NET_IPV6)
	if (addr->sa_family == AF_INET6) {
		net_ipaddr_copy(&net_sin6(&sock->src)->sin6_addr,
				&net_sin6(addr)->sin6_addr);
		net_sin6(&sock->src)->sin6_port = net_sin6(addr)->sin6_port;
	} else
#endif
#if defined(CONFIG_NET_IPV4)
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

static int offload_listen(struct net_context *context, int backlog)
{
	/* NOT IMPLEMENTED */
	return -ENOTSUP;
}

static int offload_connect(struct net_context *context,
			   const struct sockaddr *addr,
			   socklen_t addrlen,
			   net_context_connect_cb_t cb,
			   s32_t timeout,
			   void *user_data)
{
	int ret;
	char buf[sizeof("AT+USOCO=#,!###.###.###.###!,#####,#\r")];
	struct modem_socket *sock;

	if (!context || !addr) {
		return -EINVAL;
	}

	sock = (struct modem_socket *)context->offload_context;
	if (!sock) {
		LOG_ERR("Can't locate socket for net_ctx:%p!", context);
		return -EINVAL;
	}

	if (sock->socket_id < MDM_BASE_SOCKET_NUM - 1) {
		LOG_ERR("Invalid socket_id(%d) for net_ctx:%p!",
			    sock->socket_id, context);
		return -EINVAL;
	}

	sock->dst.sa_family = addr->sa_family;

#if defined(CONFIG_NET_IPV6)
	if (addr->sa_family == AF_INET6) {
		net_ipaddr_copy(&net_sin6(&sock->dst)->sin6_addr,
				&net_sin6(addr)->sin6_addr);
		sock->dst_port = ntohs(net_sin6(addr)->sin6_port);
		net_sin6(&sock->dst)->sin6_port = net_sin6(addr)->sin6_port;
	} else
#endif
#if defined(CONFIG_NET_IPV4)
	if (addr->sa_family == AF_INET) {
		net_ipaddr_copy(&net_sin(&sock->dst)->sin_addr,
				&net_sin(addr)->sin_addr);
		sock->dst_port = ntohs(net_sin(addr)->sin_port);
		net_sin(&sock->dst)->sin_port = net_sin6(addr)->sin6_port;
	} else
#endif
	{
		return -EINVAL;
	}

	if (sock->dst_port < 0) {
		LOG_ERR("Invalid port: %d", sock->dst_port);
		return -EINVAL;
	}

	/* TODO: timeout is currently ignored */

	/* skip socket connect if UDP */
	if (sock->ip_proto == IPPROTO_UDP) {
		return 0;
	}

	snprintk(buf, sizeof(buf), "AT+USOCO=%d,\"%s\",%d", sock->socket_id,
		 modem_sprint_ip_addr(addr), sock->dst_port);
	ret = send_at_cmd(sock, buf, MDM_CMD_CONN_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", buf, ret);
	}

	if (cb) {
		cb(context, ret, user_data);
	}

	return ret;
}

static int offload_accept(struct net_context *context,
			  net_tcp_accept_cb_t cb,
			  s32_t timeout,
			  void *user_data)
{
	/* NOT IMPLEMENTED */
	return -ENOTSUP;
}

static int offload_sendto(struct net_pkt *pkt,
			  const struct sockaddr *dst_addr,
			  socklen_t addrlen,
			  net_context_send_cb_t cb,
			  s32_t timeout,
			  void *user_data)
{
	struct net_context *context = net_pkt_context(pkt);
	struct modem_socket *sock;
	int ret = 0, dst_port = -1;

	if (!context) {
		return -EINVAL;
	}

	sock = (struct modem_socket *)context->offload_context;
	if (!sock) {
		LOG_ERR("Can't locate socket for net_ctx:%p!", context);
		return -EINVAL;
	}

#if defined(CONFIG_NET_IPV6)
	if (dst_addr->sa_family == AF_INET6) {
		dst_port = ntohs(net_sin6(dst_addr)->sin6_port);
	} else
#endif
#if defined(CONFIG_NET_IPV4)
	if (dst_addr->sa_family == AF_INET) {
		dst_port = ntohs(net_sin(dst_addr)->sin_port);
	} else
#endif
	{
		return -EINVAL;
	}

	if (dst_port < 0) {
		LOG_ERR("Invalid port: %d", dst_port);
		return -EINVAL;
	}

	ret = send_data(sock, dst_addr, dst_port, pkt);
	if (ret < 0) {
		LOG_ERR("send_data error: %d", ret);
	} else {
		net_pkt_unref(pkt);
	}

	if (cb) {
		cb(context, ret, user_data);
	}

	return ret;
}

static int offload_send(struct net_pkt *pkt,
			net_context_send_cb_t cb,
			s32_t timeout,
			void *user_data)
{
	struct net_context *context = net_pkt_context(pkt);
	socklen_t addrlen;

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

	return offload_sendto(pkt, &context->remote, addrlen, cb,
			      timeout, user_data);
}

static int offload_recv(struct net_context *context,
			net_context_recv_cb_t cb,
			s32_t timeout,
			void *user_data)
{
	struct modem_socket *sock;

	if (!context) {
		return -EINVAL;
	}

	sock = (struct modem_socket *)context->offload_context;
	if (!sock) {
		LOG_ERR("Can't locate socket for net_ctx:%p!", context);
		return -EINVAL;
	}

	sock->recv_cb = cb;
	sock->recv_user_data = user_data;

	return 0;
}

static int offload_put(struct net_context *context)
{
	struct modem_socket *sock;
	char buf[sizeof("AT+USOCL=#\r")];
	int ret;

	if (!context) {
		return -EINVAL;
	}

	sock = (struct modem_socket *)context->offload_context;
	if (!sock) {
		/* socket was already closed?  Exit quietly here. */
		return 0;
	}

	snprintk(buf, sizeof(buf), "AT+USOCL=%d", sock->socket_id);

	ret = send_at_cmd(sock, buf, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("AT+USOCL=%d ret:%d", sock->socket_id, ret);
	}

	/* clear last_socket_id */
	ictx.last_socket_id = MDM_BASE_SOCKET_NUM - 1;

	sock->context->connect_cb = NULL;
	sock->context->recv_cb = NULL;
	sock->context->send_cb = NULL;
	socket_put(sock);
	net_context_unref(context);

	return 0;
}

static struct net_offload offload_funcs = {
	.get = offload_get,
	.bind = offload_bind,
	.listen = offload_listen,	/* TODO */
	.connect = offload_connect,
	.accept = offload_accept,	/* TODO */
	.send = offload_send,
	.sendto = offload_sendto,
	.recv = offload_recv,
	.put = offload_put,
};

static inline u8_t *modem_get_mac(struct device *dev)
{
	struct modem_iface_ctx *ctx = dev->driver_data;

	ctx->mac_addr[0] = 0x00;
	ctx->mac_addr[1] = 0x10;

	UNALIGNED_PUT(sys_cpu_to_be32(sys_rand32_get()),
		      (u32_t *)(ctx->mac_addr + 2));

	return ctx->mac_addr;
}

static void offload_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct modem_iface_ctx *ctx = dev->driver_data;

	iface->if_dev->offload = &offload_funcs;
	net_if_set_link_addr(iface, modem_get_mac(dev),
			     sizeof(ctx->mac_addr),
			     NET_LINK_ETHERNET);
	ctx->iface = iface;
}

static struct net_if_api api_funcs = {
	.init	= offload_iface_init,
};

NET_DEVICE_OFFLOAD_INIT(modem_sara_r4, "MODEM_SARA_R4",
			modem_init, &ictx,
			NULL, CONFIG_MODEM_UBLOX_SARA_R4_INIT_PRIORITY, &api_funcs,
			MDM_MAX_DATA_LENGTH);
