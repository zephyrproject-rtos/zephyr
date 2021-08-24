/*
 * Copyright (c) 2021 Abel Sensors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#include <net/net_if.h>
#include <net/net_offload.h>
#include <net/socket_offload.h>

#include <kernel.h>
#include <errno.h>
#include <zephyr.h>

#include <drivers/modem/ublox-sara-n310.h>

#include "modem_context.h"
#include "modem_iface_uart.h"
#include "modem_cmd_handler.h"
#include "modem_socket.h"

#define MDM_UART_DEV_NAME DT_INST_BUS_LABEL(0)
#define MDM_MAX_DATA_LENGTH 512
#define MDM_MAX_SOCKETS 2

#define MDM_RECV_MAX_BUF 10
#define MDM_RECV_BUF_SIZE 64
#define MDM_CMD_TIMEOUT K_SECONDS(6)
#define MDM_REGISTRATION_TIMEOUT K_SECONDS(20)

#define RX_PRIORITY K_PRIO_COOP(7)

#define MDM_MANUFACTURER_LENGTH 10
#define MDM_MODEL_LENGTH 16
#define MDM_REVISION_LENGTH 32
#define MDM_IMEI_LENGTH 24
#define MDM_ICCID_LENGTH 24
#define MDM_IP_LENGTH 16
#define MDM_POWER_ENABLE 0
#define MDM_POWER_DISABLE 1

#define DT_DRV_COMPAT ublox_sara_n310
LOG_MODULE_REGISTER(modem_ublox_sara_n310, CONFIG_MODEM_LOG_LEVEL);
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL

/* forward declaration */
static int is_awake(void);
static int turn_on_module(void);

/* pin settings */
enum mdm_control_pins {
	MDM_POWER = 0,
	MDM_VINT,
};

static struct modem_pin modem_pins[] = {
	/* MDM_POWER */
	MODEM_PIN(DT_INST_GPIO_LABEL(0, mdm_power_gpios),
		  DT_INST_GPIO_PIN(0, mdm_power_gpios),
		  (GPIO_OPEN_DRAIN | GPIO_OUTPUT)),

	/* MDM_VINT */
	MODEM_PIN(DT_INST_GPIO_LABEL(0, mdm_vint_gpios),
		  DT_INST_GPIO_PIN(0, mdm_vint_gpios),
		  DT_INST_GPIO_FLAGS(0, mdm_vint_gpios) | GPIO_INPUT),
};

/* modem info */
struct modem_info {
	char mdm_iccid[MDM_ICCID_LENGTH];
	char mdm_model[MDM_MODEL_LENGTH];
	char mdm_manufacturer[MDM_MANUFACTURER_LENGTH];
	char mdm_revision[MDM_REVISION_LENGTH];
	char mdm_imei[MDM_IMEI_LENGTH];
	char mdm_ip[MDM_IP_LENGTH];
};

static struct modem_info minfo;

/* modem_info getters */
char *n310_get_model(void)
{
	return minfo.mdm_model;
}

char *n310_get_iccid(void)
{
	return minfo.mdm_iccid;
}

char *n310_get_manufacturer(void)
{
	return minfo.mdm_manufacturer;
}

char *n310_get_revision(void)
{
	return minfo.mdm_revision;
}

char *n310_get_imei(void)
{
	return minfo.mdm_imei;
}

char *n310_get_ip(void)
{
	return minfo.mdm_ip;
}

/* data struct */
struct modem_data {
	struct net_if *net_iface;
	uint8_t mac_addr[6];

	struct modem_context context;
	struct modem_cmd_handler_data cmd_handler_data;
	struct modem_iface_uart_data mdm_data;

	uint8_t cmd_match_buf[MDM_RECV_BUF_SIZE + 1];
	char mdm_rx_rb_buf[MDM_MAX_DATA_LENGTH];

	struct k_sem sem_response;
	enum n310_network_state network_state;

	/* socket data */
	struct modem_socket_config socket_config;
	struct modem_socket sockets[MDM_MAX_SOCKETS];
	int sock_written;
};

static struct modem_data mdata;

/* socket read callback data */
struct socket_read_data {
	char *recv_buf;
	size_t recv_buf_len;
	struct sockaddr *recv_addr;
	uint16_t recv_read_len;
};

/* rx thread data */
NET_BUF_POOL_DEFINE(mdm_recv_pool, MDM_RECV_MAX_BUF, MDM_RECV_BUF_SIZE, 0,
		    NULL);
K_THREAD_STACK_DEFINE(modem_rx_stack, MDM_MAX_DATA_LENGTH);
static struct k_thread rx_data;

/* helper macro to keep readability */
#define ATOI(s_, value_, desc_) modem_atoi((s_), (value_), (desc_), (__func__))
static int modem_atoi(const char *s, const int err_value, const char *desc,
		      const char *func)
{
	int ret = 0;
	char *endptr = { 0 };

	ret = (int)strtol(s, &endptr, 10);
	if (!endptr || *endptr != '\0') {
		LOG_ERR("bad %s '%s' in %s", log_strdup(s), log_strdup(desc),
			log_strdup(func));
		return err_value;
	}

	return ret;
}

/* send AT command */
static int send_at_command(struct modem_iface *iface,
			   struct modem_cmd_handler *handler,
			   const struct modem_cmd *handler_cmds,
			   size_t handler_cmds_len, const uint8_t *buf,
			   struct k_sem *sem, k_timeout_t timeout, bool tx_lock)
{
	int ret = 0;

	/* wake module if asleep */
	if (is_awake() == 0) {
		ret = turn_on_module();

		if (ret < 0) {
			return ret;
		}
	}

	if (tx_lock) {
		ret = modem_cmd_send(iface, handler, handler_cmds,
				     handler_cmds_len, buf, sem, timeout);
	} else {
		ret = modem_cmd_send_nolock(iface, handler, handler_cmds,
					    handler_cmds_len, buf, sem,
					    timeout);
	}

	return ret;
}

/* network state getter */
int n310_get_network_state(void)
{
	int ret = 0;

	ret = send_at_command(&mdata.context.iface, &mdata.context.cmd_handler,
			      NULL, 0U, "AT+CEREG?", &mdata.sem_response,
			      MDM_CMD_TIMEOUT, true);

	if (ret < 0) {
		return ret;
	}

	return mdata.network_state;
}

/*
 * Modem CMD handlers
 */

/* CMD: OK */
MODEM_CMD_DEFINE(on_cmd_ok)
{
	modem_cmd_handler_set_error(data, 0);
	k_sem_give(&mdata.sem_response);
	return 0;
}

/* CMD: ERROR */
MODEM_CMD_DEFINE(on_cmd_error)
{
	modem_cmd_handler_set_error(data, -EINVAL);
	k_sem_give(&mdata.sem_response);
	LOG_ERR("error");
	return 0;
}

/* CMD: +CEREG */
MODEM_CMD_DEFINE(on_cmd_socknotifycereg)
{
	char temp[1] = { 0 };

	memset(temp, '\0', sizeof(temp));
	strncpy(temp, argv[0], 1);
	mdata.network_state = ATOI(temp, 0, "stat");
	LOG_DBG("CEREG:%d", mdata.network_state);
	return 0;
}

/* CMD: +UUSOR[D|F] */
MODEM_CMD_DEFINE(on_cmd_socknotifydata)
{
	LOG_INF("\033[0;32m+UUSOR[D|F] received");

	int ret = 0;
	int socket_id = ATOI(argv[0], 0, "socket_id");
	int new_total = ATOI(argv[1], 0, "length");
	struct modem_socket *sock =
		modem_socket_from_id(&mdata.socket_config, socket_id);

	if (!sock) {
		return 0;
	}

	ret = modem_socket_packet_size_update(&mdata.socket_config, sock,
					      new_total);
	if (ret < 0) {
		LOG_ERR("socket_id:%d left_bytes:%d err: %d", socket_id,
			new_total, ret);
	}

	if (new_total > 0) {
		modem_socket_data_ready(&mdata.socket_config, sock);
	}

	return 0;
}

/* CMD: +CCID */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_iccid)
{
	size_t out_len;

	out_len =
		net_buf_linearize(minfo.mdm_iccid, sizeof(minfo.mdm_iccid) - 1,
				  data->rx_buf, 0, len);
	minfo.mdm_iccid[out_len] = '\0';

	return 0;
}

/* CMD: +CGMR */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_revision)
{
	size_t out_len;

	out_len = net_buf_linearize(minfo.mdm_revision,
				    sizeof(minfo.mdm_revision) - 1,
				    data->rx_buf, 0, len);
	minfo.mdm_revision[out_len] = '\0';

	return 0;
}

/* CMD: +CGMM */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_model)
{
	size_t out_len;

	out_len =
		net_buf_linearize(minfo.mdm_model, sizeof(minfo.mdm_model) - 1,
				  data->rx_buf, 0, len);
	minfo.mdm_model[out_len] = '\0';

	return 0;
}

/* CMD: +NPSMR */
MODEM_CMD_DEFINE(on_cmd_npsmr)
{
	char buf[2] = { 0 };
	size_t out_len = 0;

	out_len = net_buf_linearize(buf, sizeof(buf) - 1, data->rx_buf, 0, len);

	LOG_INF("Sleep mode URC: %s", log_strdup(buf));

	return 0;
}

/* CMD: +CGMI */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_manufacturer)
{
	size_t out_len = net_buf_linearize(minfo.mdm_manufacturer,
					   sizeof(minfo.mdm_manufacturer) - 1,
					   data->rx_buf, 0, len);
	minfo.mdm_manufacturer[out_len] = '\0';

	return 0;
}

/* CMD: +CGSN */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_imei)
{
	size_t out_len =
		net_buf_linearize(minfo.mdm_imei, sizeof(minfo.mdm_imei) - 1,
				  data->rx_buf, 0, len);
	minfo.mdm_imei[out_len] = '\0';

	return 0;
}

/* CMD: +CPGADDR */
MODEM_CMD_DEFINE(on_cmd_cpgaddr)
{
	char buf[sizeof(minfo.mdm_ip)] = { 0 };

	size_t out_len =
		net_buf_linearize(buf, sizeof(buf) - 1, data->rx_buf, 0, len);
	buf[out_len] = '\0';

	/* extract IP by finding the enclosing quotation marks */
	char *p = strchr(buf, '\"');

	if (p) {
		out_len = strlen(p + 1);

		memmove(buf, p + 1, out_len + 1);

		p = strchr(buf, '\"');
		if (p) {
			out_len = p - buf;

			memset(minfo.mdm_ip, 0, sizeof(minfo.mdm_ip));
			memcpy(minfo.mdm_ip, buf, out_len);
		}
	}

	return 0;
}

/* CMD: +USOCL */
MODEM_CMD_DEFINE(on_cmd_socknotifyclose)
{
	struct modem_socket *sock = NULL;

	sock = modem_socket_from_id(&mdata.socket_config,
				    ATOI(argv[0], 0, "socket_id"));
	if (sock) {
		sock->is_connected = false;
	}

	return 0;
}

/* CMD: +USOCR */
MODEM_CMD_DEFINE(on_cmd_sockcreate)
{
	struct modem_socket *sock = NULL;

	sock = modem_socket_from_newid(&mdata.socket_config);
	if (sock) {
		sock->id =
			ATOI(argv[0], mdata.socket_config.base_socket_num - 1,
			     "socket_id");
		if (sock->id == mdata.socket_config.base_socket_num - 1) {
			modem_socket_put(&mdata.socket_config, sock->sock_fd);
		}
	}
	LOG_INF("Socket %d created.", sock->id);

	return 0;
}

/* Common code for +USOR[D|F]: "<data>" */
static int on_cmd_sockread_common(int socket_id,
				  struct modem_cmd_handler_data *data,
				  int socket_data_length, uint16_t len)
{
	struct modem_socket *sock = NULL;
	struct socket_read_data *sock_data = NULL;
	int ret = 0;

	if (!len) {
		LOG_ERR("Short +USOR[D|F] value.  Aborting!");
		return -EAGAIN;
	}

	if (!data->rx_buf || *data->rx_buf->data != '\"') {
		LOG_ERR("Incorrect format! Ignoring data!");
		return -EINVAL;
	}

	/* zero length */
	if (socket_data_length <= 0) {
		LOG_ERR("Length problem (%d).  Aborting!", socket_data_length);
		return -EAGAIN;
	}

	/* check to make sure we have all of the data (minus quotes) */
	if ((net_buf_frags_len(data->rx_buf) - 2) < (socket_data_length * 2)) {
		LOG_DBG("Not enough data -- wait!");
		return -EAGAIN;
	}

	/* skip quote */
	len--;
	net_buf_pull_u8(data->rx_buf);
	if (!data->rx_buf->len) {
		data->rx_buf = net_buf_frag_del(NULL, data->rx_buf);
	}

	sock = modem_socket_from_id(&mdata.socket_config, socket_id);
	if (!sock) {
		LOG_ERR("Socket not found! (%d)", socket_id);
		ret = -EINVAL;
		goto exit;
	}

	sock_data = (struct socket_read_data *)sock->data;
	if (!sock_data) {
		LOG_ERR("Socket data not found! Skip handling (%d)", socket_id);
		ret = -EINVAL;
		goto exit;
	}

	ret = net_buf_linearize(sock_data->recv_buf, sock_data->recv_buf_len,
				data->rx_buf, 0,
				(uint16_t)socket_data_length * 2);
	data->rx_buf = net_buf_skip(data->rx_buf, ret);
	sock_data->recv_read_len = ret;

	if (ret != socket_data_length * 2) {
		LOG_ERR("Total copied data is different then received data!"
			" copied:%d vs. received:%d",
			ret, socket_data_length);
		ret = -EINVAL;
	}

exit:
	/* remove packet from list (ignore errors) */
	(void)modem_socket_packet_size_update(&mdata.socket_config, sock,
					      -socket_data_length);

	return ret;
}

/* CMD: +USORF */
MODEM_CMD_DEFINE(on_cmd_sockreadfrom)
{
	/* TODO: handle remote_ip_addr */

	return on_cmd_sockread_common(ATOI(argv[0], 0, "socket_id"), data,
				      ATOI(argv[3], 0, "length"), len);
}

/* response command handling */
static const struct modem_cmd response_cmds[] = {
	MODEM_CMD("OK", on_cmd_ok, 0U, ""),
	MODEM_CMD("ERROR", on_cmd_error, 0U, ""),
	MODEM_CMD("REBOOTING", on_cmd_ok, 0U, ""),
	MODEM_CMD("+CCID: ", on_cmd_atcmdinfo_iccid, 0U, ""),
	MODEM_CMD("+CGPADDR: ", on_cmd_cpgaddr, 1U, ""),
};

/* unsolicited command handling */
static const struct modem_cmd unsol_cmds[] = {
	MODEM_CMD("+UUSOCL: ", on_cmd_socknotifyclose, 1U, ""),
	MODEM_CMD("+UUSORD: ", on_cmd_socknotifydata, 2U, ","),
	MODEM_CMD("+UUSORF: ", on_cmd_socknotifydata, 2U, ","),
	MODEM_CMD("+CEREG: ", on_cmd_socknotifycereg, 1U, ""),
	MODEM_CMD("+NPSMR: ", on_cmd_npsmr, 1U, ""),
};

/* RX thread */
static void n310_recv(void)
{
	/* loop indefinitely in a k_thread */
	while (1) {
		/* wait until there is data in RX buffer */
		k_sem_take(&mdata.mdm_data.rx_sem, K_FOREVER);

		mdata.context.cmd_handler.process(&mdata.context.cmd_handler,
						  &mdata.context.iface);

		k_yield();
	}
}

/* create socket */
static int create_socket(struct modem_socket *sock, const struct sockaddr *addr)
{
	int ret = 0;
	static const struct modem_cmd cmd =
		MODEM_CMD("+USOCR: ", on_cmd_sockcreate, 1U, "");
	char buf[sizeof("AT+USOCR=#,#####\r")] = { 0 };
	uint16_t local_port = 0U, proto = 6U;

	if (addr) {
		if (addr->sa_family == AF_INET6) {
			local_port = ntohs(net_sin6(addr)->sin6_port);
		} else if (addr->sa_family == AF_INET) {
			local_port = ntohs(net_sin(addr)->sin_port);
		}
	}

	if (sock->ip_proto == IPPROTO_UDP) {
		proto = 17U;
	}

	if (local_port > 0U) {
		snprintk(buf, sizeof(buf), "AT+USOCR=%d,%u", proto, local_port);
	} else {
		snprintk(buf, sizeof(buf), "AT+USOCR=%d", proto);
	}

	ret = send_at_command(&mdata.context.iface, &mdata.context.cmd_handler,
			      &cmd, 1U, buf, &mdata.sem_response,
			      MDM_CMD_TIMEOUT, true);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		modem_socket_put(&mdata.socket_config, sock->sock_fd);
		errno = -ret;
		return -1;
	}

	errno = 0;
	return 0;
}

/* send hex data via the +NSOSTF command with optional flags */
static ssize_t send_socket_data(void *obj, const struct msghdr *msg, int flags,
				k_timeout_t timeout)
{
	int ret = 0;
	char send_buf[sizeof(
		"AT+NSOSTF=#,!###.###.###.###!,#####,#,##,###############################\r\n")] = {
		0
	};
	uint16_t dst_port = 0U;
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct sockaddr *dst_addr = msg->msg_name;
	size_t buf_len = 0;
	int release_flag = 0;

	if (sock->ip_proto != IPPROTO_UDP) {
		/* only UDP is currently supported */
		return -ENOTSUP;
	}

	for (int i = 0; i < msg->msg_iovlen; i++) {
		if (!msg->msg_iov[i].iov_base || msg->msg_iov[i].iov_len == 0) {
			errno = EINVAL;
			return -1;
		}
		buf_len += msg->msg_iov[i].iov_len;
	}

	if (!dst_addr && sock->ip_proto == IPPROTO_UDP) {
		dst_addr = &sock->dst;
	}

	if (!sock) {
		return -EINVAL;
	}

	/* HEX mode allows a maximum packet size of 512 */
	if (buf_len > MDM_MAX_DATA_LENGTH) {
		buf_len = MDM_MAX_DATA_LENGTH;
	}

	if ((flags & RELEASE_AFTER_FIRST_DOWNLINK) ||
	    (flags & RELEASE_AFTER_UPLINK)) {
		release_flag = (int)flags;
	}

	/* The number of bytes written will be reported by the modem */
	mdata.sock_written = 0;

	ret = modem_context_get_addr_port(dst_addr, &dst_port);
	snprintk(send_buf, sizeof(send_buf),
		 "AT+NSOSTF=%d,\"%s\",%u,%d,%zu,\"%s\"", sock->id,
		 modem_context_sprint_ip_addr(dst_addr), dst_port, release_flag,
		 buf_len, (char *)msg->msg_iov->iov_base);

	k_sem_take(&mdata.cmd_handler_data.sem_tx_lock, K_FOREVER);

	ret = send_at_command(&mdata.context.iface, &mdata.context.cmd_handler,
			      NULL, 0U, send_buf, NULL, K_NO_WAIT, false);
	if (ret < 0) {
		LOG_ERR("send_nolock err");
	}

	/* set command handlers */
	k_sem_give(&mdata.cmd_handler_data.sem_tx_lock);

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		ret = 0;
		goto exit;
	}

	if (ret == 0) {
		ret = modem_cmd_handler_get_error(&mdata.cmd_handler_data);
	} else if (ret == -EAGAIN) {
		ret = -ETIMEDOUT;
	}

exit:
	/* unset handler commands and ignore any errors */
	(void)modem_cmd_handler_update_cmds(&mdata.cmd_handler_data, NULL, 0U,
					    false);
	k_sem_give(&mdata.cmd_handler_data.sem_tx_lock);

	if (ret < 0) {
		return ret;
	}

	return mdata.sock_written;
}

static const struct socket_op_vtable offload_socket_fd_op_vtable;

static int offload_socket(int family, int type, int proto)
{
	int ret = 0;

	if (proto != IPPROTO_UDP) {
		/* currently only UDP is supported */
		return -ENOTSUP;
	}

	/* defer modem's socket create call to bind() */
	ret = modem_socket_get(&mdata.socket_config, family, type, proto);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	errno = 0;
	return ret;
}

static int offload_connect(void *obj, const struct sockaddr *addr,
			   socklen_t addrlen)
{
	/* TCP FUNCTIONALITY NOT IMPLEMENTED */
	struct modem_socket *sock = (struct modem_socket *)obj;
	uint16_t dst_port = 0U;

	if (!addr) {
		errno = EINVAL;
		return -1;
	}

	if (sock->id < mdata.socket_config.base_socket_num - 1) {
		LOG_ERR("Invalid socket_id(%d) from fd:%d", sock->id,
			sock->sock_fd);
		errno = EINVAL;
		return -1;
	}

	/* make sure we've created the socket */
	if (sock->id == mdata.socket_config.sockets_len + 1) {
		if (create_socket(sock, NULL) < 0) {
			return -1;
		}
	}

	memcpy(&sock->dst, addr, sizeof(*addr));
	if (addr->sa_family == AF_INET6) {
		dst_port = ntohs(net_sin6(addr)->sin6_port);
	} else if (addr->sa_family == AF_INET) {
		dst_port = ntohs(net_sin(addr)->sin_port);
	} else {
		errno = EAFNOSUPPORT;
		return -1;
	}

	/* currently only UDP is supported, so socket connection setup is skipped */
	errno = 0;
	return 0;
}

static int offload_close(void *obj)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	char buf[sizeof("AT+USOCL=#\r")] = { 0 };
	int ret = 0;

	/* make sure we assigned an id */
	if (sock->id < mdata.socket_config.base_socket_num) {
		return 0;
	}

	if (sock->ip_proto == IPPROTO_UDP) {
		snprintk(buf, sizeof(buf), "AT+USOCL=%d", sock->id);

		ret = send_at_command(&mdata.context.iface,
				      &mdata.context.cmd_handler, NULL, 0U, buf,
				      &mdata.sem_response, MDM_CMD_TIMEOUT,
				      true);
		if (ret < 0) {
			LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		}
	}

	LOG_INF("Socket %d closed.", sock->id);

	modem_socket_put(&mdata.socket_config, sock->sock_fd);
	return 0;
}

static int offload_bind(void *obj, const struct sockaddr *addr,
			socklen_t addrlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;

	/* save bind address information */
	memcpy(&sock->src, addr, sizeof(*addr));

	/* make sure we've created the socket */
	if (sock->id == mdata.socket_config.sockets_len + 1) {
		if (create_socket(sock, addr) < 0) {
			return -1;
		}
	}

	return 0;
}

static ssize_t offload_sendto(void *obj, const void *buf, size_t len, int flags,
			      const struct sockaddr *to, socklen_t tolen)
{
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

	int ret = send_socket_data(obj, &msg, flags, MDM_CMD_TIMEOUT);

	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	errno = 0;
	return ret;
}

static ssize_t offload_recvfrom(void *obj, void *buf, size_t len, int flags,
				struct sockaddr *from, socklen_t *fromlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	int ret = 0, next_packet_size = 0;
	static const struct modem_cmd cmd[] = {
		MODEM_CMD("+USORF: ", on_cmd_sockreadfrom, 4U, ","),
	};
	char sendbuf[sizeof("AT+USORF=#\r")];
	struct socket_read_data sock_data;

	if (!buf || len == 0) {
		errno = EINVAL;
		return -1;
	}

	if (flags & ZSOCK_MSG_PEEK) {
		errno = ENOTSUP;
		return -1;
	}

	next_packet_size =
		modem_socket_next_packet_size(&mdata.socket_config, sock);
	if (!next_packet_size) {
		if (flags & ZSOCK_MSG_DONTWAIT) {
			errno = EAGAIN;
			return -1;
		}

		if (!sock->is_connected && sock->ip_proto != IPPROTO_UDP) {
			errno = 0;
			return 0;
		}

		modem_socket_wait_data(&mdata.socket_config, sock);
		next_packet_size = modem_socket_next_packet_size(
			&mdata.socket_config, sock);
	}

	/* HEX mode allows a maximum packet size of 512 */
	if (next_packet_size > MDM_MAX_DATA_LENGTH) {
		next_packet_size = MDM_MAX_DATA_LENGTH;
	}

	snprintk(sendbuf, sizeof(sendbuf), "AT+USORF=%d", sock->id);

	/* socket read settings */
	(void)memset(&sock_data, 0, sizeof(sock_data));
	sock_data.recv_buf = buf;
	sock_data.recv_buf_len = len;
	sock_data.recv_addr = from;
	sock->data = &sock_data;

	ret = send_at_command(&mdata.context.iface, &mdata.context.cmd_handler,
			      cmd, ARRAY_SIZE(cmd), sendbuf,
			      &mdata.sem_response, MDM_CMD_TIMEOUT, true);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
		goto exit;
	}

	/* HACK: use dst address as from */
	if (from && fromlen) {
		*fromlen = sizeof(sock->dst);
		memcpy(from, &sock->dst, *fromlen);
	}

	/* return length of received data */
	errno = 0;
	ret = sock_data.recv_read_len;

exit:
	/* clear socket data */
	sock->data = NULL;
	return ret;
}

static const struct socket_op_vtable offload_socket_fd_op_vtable = {
	/* these need to be implemented */
	.fd_vtable = {
		.read = NULL,
		.write = NULL,
		.close = offload_close,
		.ioctl = NULL,
	},
	.bind = offload_bind,
	.connect = offload_connect,
	.sendto = offload_sendto,
	.recvfrom = offload_recvfrom,
	.listen = NULL,
	.accept = NULL,
	.sendmsg = NULL,
	.getsockopt = NULL,
	.setsockopt = NULL,
};

static bool offload_is_supported(int family, int type, int proto)
{
	/* offloading always enabled for now. */
	return true;
}

/* register net socket offload */
NET_SOCKET_REGISTER(ublox_sara_n310, AF_UNSPEC, offload_is_supported,
		    offload_socket);

/*
 * Pin functions.
 *
 * The SARA-N310 forces the power pin low to achieve the following:
 * min 1s - max 2.5s: trigger module switch-on
 * min 1s - max 2.5s: trigger module wake-up from PSM
 * min 2.5s: trigger module switch-off
 */

/* turn on module, returns < 0 when module failed to turn on */
static int turn_on_module(void)
{
	int ret = 0, timeout = 0;

	LOG_DBG("MDM_POWER_PIN -> ENABLE");
	modem_pin_write(&mdata.context, MDM_POWER, MDM_POWER_ENABLE);
	k_sleep(K_MSEC(1500));

	LOG_DBG("MDM_POWER_PIN -> DISABLE");
	modem_pin_write(&mdata.context, MDM_POWER, MDM_POWER_DISABLE);

	/* check if module has turned on and return if it is taking too long */
	while (!is_awake() && (timeout++ < 5)) {
		k_sleep(K_MSEC(10));
	}

	if (timeout >= 5) {
		ret = -timeout;

		return ret;
	}

	return 0;
}

static int turn_off_module(void)
{
	LOG_DBG("MDM_POWER_PIN -> ENABLE");
	modem_pin_write(&mdata.context, MDM_POWER, MDM_POWER_ENABLE);
	k_sleep(K_SECONDS(3));

	LOG_DBG("MDM_POWER_PIN -> DISABLE");
	modem_pin_write(&mdata.context, MDM_POWER, MDM_POWER_DISABLE);

	return 0;
}

/* check if module is awake by checking V_INT value */
static int is_awake(void)
{
	return modem_pin_read(&mdata.context, MDM_VINT);
}

/* PSM functions, exposed to application through header */
/* Power mode setting, +NVSETPM */
int n310_psm_set_mode(int psm_mode)
{
	int ret = 0;
	char buf[sizeof("AT+NVSETPM=##\r")] = { 0 };

	snprintk(buf, sizeof(buf), "AT+NVSETPM=%d", psm_mode);

	ret = send_at_command(&mdata.context.iface, &mdata.context.cmd_handler,
			      NULL, 0U, buf, &mdata.sem_response,
			      MDM_CMD_TIMEOUT, true);

	if (ret >= 0) {
		LOG_INF("NVSETPM set with current configuration: %d", psm_mode);
	} else {
		LOG_ERR("Failed to set NVSETPM: %d", ret);
	}

	return ret;
}

/* Low clock mode setting, +CSCLK */
int n310_psm_set_csclk(int setting)
{
	int ret = 0;
	char buf[sizeof("AT+CSCLK=#\r")] = { 0 };

	snprintk(buf, sizeof(buf), "AT+CSCLK=%d", setting);

	ret = send_at_command(&mdata.context.iface, &mdata.context.cmd_handler,
			      NULL, 0U, buf, &mdata.sem_response,
			      MDM_CMD_TIMEOUT, true);

	if (ret >= 0) {
		LOG_INF("CSCLK set with current configuration: %d", setting);
	} else {
		LOG_ERR("Failed to set CSCLK: %d", ret);
	}

	return ret;
}

/* Power Saving Mode setting, +CPSMS */
int n310_psm_config(int mode, char *periodic_TAU, char *active_time)
{
	int ret = 0;
	char buf[sizeof("AT+CPSMS=#,,,\"########\",\"########\"\r")] = { 0 };

	snprintk(buf, sizeof(buf), "AT+CPSMS=%d,,,\"%s\",\"%s\"", mode,
		 periodic_TAU, active_time);

	ret = send_at_command(&mdata.context.iface, &mdata.context.cmd_handler,
			      NULL, 0U, buf, &mdata.sem_response,
			      MDM_CMD_TIMEOUT, true);

	if (ret >= 0) {
		LOG_INF("CPSMS set with current configuration: %d, \"%s\", \"%s\"",
			mode, log_strdup(periodic_TAU),
			log_strdup(active_time));
	} else {
		LOG_ERR("Failed to set CPSMS: %d", ret);
	}

	return ret;
}

/* Pin initialization */
static int pin_init(void)
{
	int ret = 0;

	LOG_INF("Initializing modem pins.");

	turn_off_module();

	/* wait until power is off */
	while (is_awake() > 0) {
		k_sleep(K_MSEC(100));
	}

	ret = turn_on_module();

	if (ret < 0) {
		LOG_ERR("Failed to turn on module.");
		return ret;
	}

	LOG_INF("Done.");
	return 0;
}

/*
 * Modem reset function.
 * Resets the modem, sends setup commands and waits for modem to be functional.
 */
int n310_modem_reset(void)
{
	int ret = 0, counter = 0;

	LOG_INF("Starting modem...");

	static const struct setup_cmd setup_cmds[] = {
		/* turn off echo */
		SETUP_CMD_NOHANDLE("ATE0"),
		/* stop functionality */
		SETUP_CMD_NOHANDLE("AT+CFUN=0"),
		/* extended error numbers */
		SETUP_CMD_NOHANDLE("AT+CMEE=1"),
		/* UNC messages for registration */
		SETUP_CMD_NOHANDLE("AT+CREG=1"),
		/* enable PSM URC for debugging */
		SETUP_CMD_NOHANDLE("AT+NPSMR=1"),
		/* enable PDP context */
		SETUP_CMD_NOHANDLE("AT+CIPCA=1"),
		/* enable HEX mode for +USOWR, +USOST, +USORD and +USORF AT commands. */
		SETUP_CMD_NOHANDLE("AT+UDCONF=1,1"),
		/* get and store modem info */
		SETUP_CMD("AT+CGMI", "", on_cmd_atcmdinfo_manufacturer, 0U, ""),
		SETUP_CMD("AT+CGMM", "", on_cmd_atcmdinfo_model, 0U, ""),
		SETUP_CMD("AT+CGMR", "", on_cmd_atcmdinfo_revision, 0U, ""),
		SETUP_CMD("AT+CGSN", "", on_cmd_atcmdinfo_imei, 0U, ""),
		SETUP_CMD("AT+CCID", "", on_cmd_atcmdinfo_iccid, 0U, ""),
		/* enable functionality */
		SETUP_CMD_NOHANDLE("AT+CFUN=1"),
	};

	/* reset module through pins */
	pin_init();

	/* give modem time to start responding after restart */
	ret = -1;

	while (counter++ < 50 && ret < 0) {
		k_sleep(K_SECONDS(2));
		ret = send_at_command(&mdata.context.iface,
				      &mdata.context.cmd_handler, NULL, 0, "AT",
				      &mdata.sem_response, MDM_CMD_TIMEOUT,
				      true);
		if (ret < 0 && ret != -ETIMEDOUT) {
			break;
		}
	}

	if (ret < 0) {
		LOG_ERR("Modem wait loop error: %d", ret);
		return ret;
	}

	/* send setup commands */
	ret = modem_cmd_handler_setup_cmds(&mdata.context.iface,
					   &mdata.context.cmd_handler,
					   setup_cmds, ARRAY_SIZE(setup_cmds),
					   &mdata.sem_response,
					   MDM_REGISTRATION_TIMEOUT);

	if (ret < 0) {
		LOG_ERR("Modem setup cmds error: %d", ret);
		return ret;
	}

	/* register operator automatically */
	ret = send_at_command(&mdata.context.iface, &mdata.context.cmd_handler,
			      NULL, 0, "AT+COPS=0", &mdata.sem_response,
			      MDM_REGISTRATION_TIMEOUT, true);

	if (ret < 0) {
		LOG_ERR("AT+COPS error: %d", ret);
		return ret;
	}

	/* query for IP address once */
	ret = send_at_command(&mdata.context.iface, &mdata.context.cmd_handler,
			      NULL, 0, "AT+CGPADDR=", &mdata.sem_response,
			      MDM_CMD_TIMEOUT, true);

	if (ret < 0) {
		LOG_ERR("Failed to obtain IP address");
	}

	LOG_INF("Modem is ready.");
	return 0;
}

/*
 * Driver init function that is called on load.
 * Initializes data structs, uart interface, sockets,
 * and modem context.
 */
static int n310_driver_init(const struct device *device)
{
	struct modem_data *mdm = device->data;
	int ret = 0;

	mdata.cmd_handler_data.cmds[CMD_RESP] = response_cmds;
	mdata.cmd_handler_data.cmds_len[CMD_RESP] = ARRAY_SIZE(response_cmds);
	mdata.cmd_handler_data.cmds[CMD_UNSOL] = unsol_cmds;
	mdata.cmd_handler_data.cmds_len[CMD_UNSOL] = ARRAY_SIZE(unsol_cmds);
	mdata.cmd_handler_data.match_buf = &mdm->cmd_match_buf[0];
	mdata.cmd_handler_data.match_buf_len = sizeof(mdm->cmd_match_buf);
	mdata.cmd_handler_data.buf_pool = &mdm_recv_pool;
	mdata.cmd_handler_data.alloc_timeout = K_NO_WAIT;
	mdata.cmd_handler_data.eol = "\r";

	/* init response semaphore with a count limit of 1 */
	k_sem_init(&mdata.sem_response, 0, 1);

	ret = modem_cmd_handler_init(&mdata.context.cmd_handler,
				     &mdata.cmd_handler_data);
	if (ret < 0) {
		LOG_ERR("cmd handler init error: %d", ret);
		return ret;
	}

	mdata.context.pins = modem_pins;
	mdata.context.pins_len = ARRAY_SIZE(modem_pins);

	/* init modem socket */
	mdata.socket_config.sockets = &mdata.sockets[0];
	mdata.socket_config.sockets_len = ARRAY_SIZE(mdata.sockets);
	mdata.socket_config.base_socket_num = 0;
	ret = modem_socket_init(&mdata.socket_config,
				&offload_socket_fd_op_vtable);
	if (ret < 0) {
		LOG_ERR("socket init failed: %d", ret);
		return ret;
	}

	mdata.mdm_data.rx_rb_buf = &mdata.mdm_rx_rb_buf[0];
	mdata.mdm_data.rx_rb_buf_len = sizeof(mdata.mdm_rx_rb_buf);

	/* init iface uart using UART name */
	ret = modem_iface_uart_init(&mdata.context.iface, &mdata.mdm_data,
				    MDM_UART_DEV_NAME);

	if (ret < 0) {
		LOG_ERR("iface uart init error: %d", ret);
		return ret;
	}

	ret = modem_context_register(&mdata.context);
	if (ret < 0) {
		LOG_ERR("modem context register error: %d", ret);
		return ret;
	}

	/* create rx thread */
	k_thread_create(&rx_data, modem_rx_stack,
			K_THREAD_STACK_SIZEOF(modem_rx_stack),
			(k_thread_entry_t)n310_recv, NULL, NULL, NULL,
			RX_PRIORITY, 0, K_NO_WAIT);

	n310_modem_reset();

	return 0;
}

static int net_offload_dummy_get(sa_family_t family, enum net_sock_type type,
				 enum net_ip_protocol ip_proto,
				 struct net_context **context)
{
	LOG_ERR("CONFIG_NET_SOCKETS_OFFLOAD must be enabled for this driver");

	return -ENOTSUP;
}

/* placeholders, until Zepyr IP stack updated to handle a NULL net_offload */
static struct net_offload modem_net_offload = {
	.get = net_offload_dummy_get,
};

#define HASH_MULTIPLIER 37
static uint32_t hash32(char *str, int len)
{
	uint32_t h = 0;
	int i;

	for (i = 0; i < len; ++i) {
		h = (h * HASH_MULTIPLIER) + str[i];
	}

	return h;
}

static inline uint8_t *modem_get_mac(const struct device *dev)
{
	struct modem_data *data = dev->data;
	uint32_t hash_value = 0;

	data->mac_addr[0] = 0x00;
	data->mac_addr[1] = 0x10;

	/* use IMEI for mac_addr */
	hash_value = hash32(minfo.mdm_imei, strlen(minfo.mdm_imei));

	UNALIGNED_PUT(hash_value, (uint32_t *)(data->mac_addr + 2));

	return data->mac_addr;
}

/*
 * Init net interface and use socket offload.
 */
static void modem_net_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct modem_data *data = dev->data;

	/* direct socket offload used instead of net offload: */
	iface->if_dev->offload = &modem_net_offload;
	net_if_set_link_addr(iface, modem_get_mac(dev), sizeof(data->mac_addr),
			     NET_LINK_ETHERNET);
	data->net_iface = iface;
}

/* net_if API struct */
static const struct net_if_api api_funcs = {
	.init = modem_net_iface_init,
};

NET_DEVICE_DT_INST_OFFLOAD_DEFINE(0, n310_driver_init, device_pm_control_nop,
				  &mdata, NULL,
				  CONFIG_MODEM_UBLOX_SARA_N310_INIT_PRIORITY,
				  &api_funcs, MDM_MAX_DATA_LENGTH);
