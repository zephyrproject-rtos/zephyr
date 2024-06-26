/*
 * Copyright (C) 2021 metraTec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT simcom_sim7080

#include <zephyr/logging/log.h>
#include <zephyr/net/offloaded_netdev.h>
LOG_MODULE_REGISTER(modem_simcom_sim7080, CONFIG_MODEM_LOG_LEVEL);

#include <zephyr/drivers/modem/simcom-sim7080.h>
#include "simcom-sim7080.h"

#define SMS_TP_UDHI_HEADER 0x40

static struct k_thread modem_rx_thread;
static struct k_work_q modem_workq;
static struct sim7080_data mdata;
static struct modem_context mctx;
static const struct socket_op_vtable offload_socket_fd_op_vtable;

static struct zsock_addrinfo dns_result;
static struct sockaddr dns_result_addr;
static char dns_result_canonname[DNS_MAX_NAME_SIZE + 1];

static struct sim7080_gnss_data gnss_data;

static K_KERNEL_STACK_DEFINE(modem_rx_stack, CONFIG_MODEM_SIMCOM_SIM7080_RX_STACK_SIZE);
static K_KERNEL_STACK_DEFINE(modem_workq_stack, CONFIG_MODEM_SIMCOM_SIM7080_RX_WORKQ_STACK_SIZE);
NET_BUF_POOL_DEFINE(mdm_recv_pool, MDM_RECV_MAX_BUF, MDM_RECV_BUF_SIZE, 0, NULL);

/* pin settings */
static const struct gpio_dt_spec power_gpio = GPIO_DT_SPEC_INST_GET(0, mdm_power_gpios);

static void socket_close(struct modem_socket *sock);
static const struct socket_dns_offload offload_dns_ops;

static inline uint32_t hash32(char *str, int len)
{
#define HASH_MULTIPLIER 37
	uint32_t h = 0;
	int i;

	for (i = 0; i < len; ++i) {
		h = (h * HASH_MULTIPLIER) + str[i];
	}

	return h;
}

static inline uint8_t *modem_get_mac(const struct device *dev)
{
	struct sim7080_data *data = dev->data;
	uint32_t hash_value;

	data->mac_addr[0] = 0x00;
	data->mac_addr[1] = 0x10;

	/* use IMEI for mac_addr */
	hash_value = hash32(mdata.mdm_imei, strlen(mdata.mdm_imei));

	UNALIGNED_PUT(hash_value, (uint32_t *)(data->mac_addr + 2));

	return data->mac_addr;
}

static int offload_socket(int family, int type, int proto);

/* Setup the Modem NET Interface. */
static void modem_net_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct sim7080_data *data = dev->data;

	net_if_set_link_addr(iface, modem_get_mac(dev), sizeof(data->mac_addr), NET_LINK_ETHERNET);

	data->netif = iface;

	socket_offload_dns_register(&offload_dns_ops);

	net_if_socket_offload_set(iface, offload_socket);
}

/**
 * Changes the operating state of the sim7080.
 *
 * @param state The new state.
 */
static void change_state(enum sim7080_state state)
{
	LOG_DBG("Changing state to (%d)", state);
	mdata.state = state;
}

/**
 * Get the current operating state of the sim7080.
 *
 * @return The current state.
 */
static enum sim7080_state get_state(void)
{
	return mdata.state;
}

/*
 * Parses the +CAOPEN command and gives back the
 * connect semaphore.
 */
MODEM_CMD_DEFINE(on_cmd_caopen)
{
	int result = atoi(argv[1]);

	LOG_INF("+CAOPEN: %d", result);
	modem_cmd_handler_set_error(data, result);
	return 0;
}

/*
 * Unlock the tx ready semaphore if '> ' is received.
 */
MODEM_CMD_DIRECT_DEFINE(on_cmd_tx_ready)
{
	k_sem_give(&mdata.sem_tx_ready);
	return len;
}

/*
 * Connects an modem socket. Protocol can either be TCP or UDP.
 */
static int offload_connect(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	uint16_t dst_port = 0;
	char *protocol;
	struct modem_cmd cmd[] = { MODEM_CMD("+CAOPEN: ", on_cmd_caopen, 2U, ",") };
	char buf[sizeof("AT+CAOPEN: #,#,#####,#xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx#,####")];
	char ip_str[NET_IPV6_ADDR_LEN];
	int ret;

	/* Modem is not attached to the network. */
	if (get_state() != SIM7080_STATE_NETWORKING) {
		return -EAGAIN;
	}

	if (modem_socket_is_allocated(&mdata.socket_config, sock) == false) {
		LOG_ERR("Invalid socket id %d from fd %d", sock->id, sock->sock_fd);
		errno = EINVAL;
		return -1;
	}

	if (sock->is_connected == true) {
		LOG_ERR("Socket is already connected! id: %d, fd: %d", sock->id, sock->sock_fd);
		errno = EISCONN;
		return -1;
	}

	/* get the destination port */
	if (addr->sa_family == AF_INET6) {
		dst_port = ntohs(net_sin6(addr)->sin6_port);
	} else if (addr->sa_family == AF_INET) {
		dst_port = ntohs(net_sin(addr)->sin_port);
	}

	/* Get protocol */
	protocol = (sock->type == SOCK_STREAM) ? "TCP" : "UDP";

	ret = modem_context_sprint_ip_addr(addr, ip_str, sizeof(ip_str));
	if (ret != 0) {
		LOG_ERR("Failed to format IP!");
		errno = ENOMEM;
		return -1;
	}

	ret = snprintk(buf, sizeof(buf), "AT+CAOPEN=%d,%d,\"%s\",\"%s\",%d", 0, sock->id,
		       protocol, ip_str, dst_port);
	if (ret < 0) {
		LOG_ERR("Failed to build connect command. ID: %d, FD: %d", sock->id, sock->sock_fd);
		errno = ENOMEM;
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmd, ARRAY_SIZE(cmd), buf,
			     &mdata.sem_response, MDM_CONNECT_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret: %d", buf, ret);
		socket_close(sock);
		goto error;
	}

	ret = modem_cmd_handler_get_error(&mdata.cmd_handler_data);
	if (ret != 0) {
		LOG_ERR("Closing the socket!");
		socket_close(sock);
		goto error;
	}

	sock->is_connected = true;
	errno = 0;
	return 0;
error:
	errno = -ret;
	return -1;
}

/*
 * Send data over a given socket.
 *
 * First we signal the module that we want to send data over a socket.
 * This is done by sending AT+CASEND=<sockfd>,<nbytes>\r\n.
 * If The module is ready to send data it will send back
 * an UNTERMINATED prompt '> '. After that data can be sent to the modem.
 * As terminating byte a STRG+Z (0x1A) is sent. The module will
 * then send a OK or ERROR.
 */
static ssize_t offload_sendto(void *obj, const void *buf, size_t len, int flags,
			      const struct sockaddr *dest_addr, socklen_t addrlen)
{
	int ret;
	struct modem_socket *sock = (struct modem_socket *)obj;
	char send_buf[sizeof("AT+CASEND=#,####")] = { 0 };
	char ctrlz = 0x1A;

	/* Modem is not attached to the network. */
	if (get_state() != SIM7080_STATE_NETWORKING) {
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

	ret = snprintk(send_buf, sizeof(send_buf), "AT+CASEND=%d,%ld", sock->id, (long)len);
	if (ret < 0) {
		LOG_ERR("Failed to build send command!!");
		errno = ENOMEM;
		return -1;
	}

	/* Make sure only one send can be done at a time. */
	k_sem_take(&mdata.cmd_handler_data.sem_tx_lock, K_FOREVER);
	k_sem_reset(&mdata.sem_tx_ready);

	/* Send CASEND */
	mdata.current_sock_written = len;
	ret = modem_cmd_send_nolock(&mctx.iface, &mctx.cmd_handler, NULL, 0U, send_buf, NULL,
				    K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to send CASEND!!");
		goto exit;
	}

	/* Wait for '> ' */
	ret = k_sem_take(&mdata.sem_tx_ready, K_SECONDS(2));
	if (ret < 0) {
		LOG_ERR("Timeout while waiting for tx");
		goto exit;
	}

	/* Send data */
	mctx.iface.write(&mctx.iface, buf, len);
	mctx.iface.write(&mctx.iface, &ctrlz, 1);

	/* Wait for the OK */
	k_sem_reset(&mdata.sem_response);
	ret = k_sem_take(&mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Timeout waiting for OK");
	}

exit:
	k_sem_give(&mdata.cmd_handler_data.sem_tx_lock);
	/* Data was successfully sent */

	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	errno = 0;
	return mdata.current_sock_written;
}

/*
 * Read data from a given socket.
 *
 * The response has the form +CARECV: <length>,data\r\nOK\r\n
 */
static int sockread_common(int sockfd, struct modem_cmd_handler_data *data, int socket_data_length,
			   uint16_t len)
{
	struct modem_socket *sock;
	struct socket_read_data *sock_data;
	int ret, packet_size;

	if (!len) {
		LOG_ERR("Invalid length, aborting");
		return -EAGAIN;
	}

	if (!data->rx_buf) {
		LOG_ERR("Incorrect format! Ignoring data!");
		return -EINVAL;
	}

	if (socket_data_length <= 0) {
		LOG_ERR("Length error (%d)", socket_data_length);
		return -EAGAIN;
	}

	if (net_buf_frags_len(data->rx_buf) < socket_data_length) {
		LOG_DBG("Not enough data -- wait!");
		return -EAGAIN;
	}

	sock = modem_socket_from_fd(&mdata.socket_config, sockfd);
	if (!sock) {
		LOG_ERR("Socket not found! (%d)", sockfd);
		ret = -EINVAL;
		goto exit;
	}

	sock_data = (struct socket_read_data *)sock->data;
	if (!sock_data) {
		LOG_ERR("Socket data not found! (%d)", sockfd);
		ret = -EINVAL;
		goto exit;
	}

	ret = net_buf_linearize(sock_data->recv_buf, sock_data->recv_buf_len, data->rx_buf, 0,
				(uint16_t)socket_data_length);
	data->rx_buf = net_buf_skip(data->rx_buf, ret);
	sock_data->recv_read_len = ret;
	if (ret != socket_data_length) {
		LOG_ERR("Total copied data is different then received data!"
			" copied:%d vs. received:%d",
			ret, socket_data_length);
		ret = -EINVAL;
		goto exit;
	}

exit:
	/* Indication only sets length to a dummy value. */
	packet_size = modem_socket_next_packet_size(&mdata.socket_config, sock);
	modem_socket_packet_size_update(&mdata.socket_config, sock, -packet_size);
	return ret;
}

/*
 * Handler for carecv response.
 */
MODEM_CMD_DEFINE(on_cmd_carecv)
{
	return sockread_common(mdata.current_sock_fd, data, atoi(argv[0]), len);
}

/*
 * Read data from a given socket.
 */
static ssize_t offload_recvfrom(void *obj, void *buf, size_t max_len, int flags,
				struct sockaddr *src_addr, socklen_t *addrlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	char sendbuf[sizeof("AT+CARECV=##,####")];
	int ret, packet_size;
	struct socket_read_data sock_data;

	struct modem_cmd data_cmd[] = { MODEM_CMD("+CARECV: ", on_cmd_carecv, 1U, ",") };

	/* Modem is not attached to the network. */
	if (get_state() != SIM7080_STATE_NETWORKING) {
		LOG_ERR("Modem currently not attached to the network!");
		return -EAGAIN;
	}

	if (!buf || max_len == 0) {
		errno = EINVAL;
		return -1;
	}

	if (flags & ZSOCK_MSG_PEEK) {
		errno = ENOTSUP;
		return -1;
	}

	packet_size = modem_socket_next_packet_size(&mdata.socket_config, sock);
	if (!packet_size) {
		if (flags & ZSOCK_MSG_DONTWAIT) {
			errno = EAGAIN;
			return -1;
		}

		modem_socket_wait_data(&mdata.socket_config, sock);
		packet_size = modem_socket_next_packet_size(&mdata.socket_config, sock);
	}

	max_len = (max_len > MDM_MAX_DATA_LENGTH) ? MDM_MAX_DATA_LENGTH : max_len;
	snprintk(sendbuf, sizeof(sendbuf), "AT+CARECV=%d,%zd", sock->id, max_len);

	memset(&sock_data, 0, sizeof(sock_data));
	sock_data.recv_buf = buf;
	sock_data.recv_buf_len = max_len;
	sock_data.recv_addr = src_addr;
	sock->data = &sock_data;
	mdata.current_sock_fd = sock->sock_fd;

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, data_cmd, ARRAY_SIZE(data_cmd),
			     sendbuf, &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
		goto exit;
	}

	/* HACK: use dst address as src */
	if (src_addr && addrlen) {
		*addrlen = sizeof(sock->dst);
		memcpy(src_addr, &sock->dst, *addrlen);
	}

	errno = 0;
	ret = sock_data.recv_read_len;

exit:
	/* clear socket data */
	mdata.current_sock_fd = -1;
	sock->data = NULL;
	return ret;
}

/*
 * Sends messages to the modem.
 */
static ssize_t offload_sendmsg(void *obj, const struct msghdr *msg, int flags)
{
	struct modem_socket *sock = obj;
	ssize_t sent = 0;
	const char *buf;
	size_t len;
	int ret;

	/* Modem is not attached to the network. */
	if (get_state() != SIM7080_STATE_NETWORKING) {
		LOG_ERR("Modem currently not attached to the network!");
		return -EAGAIN;
	}

	if (sock->type == SOCK_DGRAM) {
		/*
		 * Current implementation only handles single contiguous fragment at a time, so
		 * prevent sending multiple datagrams.
		 */
		if (msghdr_non_empty_iov_count(msg) > 1) {
			errno = EMSGSIZE;
			return -1;
		}
	}

	for (int i = 0; i < msg->msg_iovlen; i++) {
		buf = msg->msg_iov[i].iov_base;
		len = msg->msg_iov[i].iov_len;

		while (len > 0) {
			ret = offload_sendto(obj, buf, len, flags, msg->msg_name, msg->msg_namelen);
			if (ret < 0) {
				if (ret == -EAGAIN) {
					k_sleep(K_SECONDS(1));
				} else {
					return ret;
				}
			} else {
				sent += ret;
				buf += ret;
				len -= ret;
			}
		}
	}

	return sent;
}

/*
 * Closes a given socket.
 */
static void socket_close(struct modem_socket *sock)
{
	char buf[sizeof("AT+CACLOSE=##")];
	int ret;

	snprintk(buf, sizeof(buf), "AT+CACLOSE=%d", sock->id);

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buf, &mdata.sem_response,
			     MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret: %d", buf, ret);
	}

	modem_socket_put(&mdata.socket_config, sock->sock_fd);
}

/*
 * Offloads read by reading from a given socket.
 */
static ssize_t offload_read(void *obj, void *buffer, size_t count)
{
	return offload_recvfrom(obj, buffer, count, 0, NULL, 0);
}

/*
 * Offloads write by writing to a given socket.
 */
static ssize_t offload_write(void *obj, const void *buffer, size_t count)
{
	return offload_sendto(obj, buffer, count, 0, NULL, 0);
}

/*
 * Offloads close by terminating the connection and freeing the socket.
 */
static int offload_close(void *obj)
{
	struct modem_socket *sock = (struct modem_socket *)obj;

	/* Modem is not attached to the network. */
	if (get_state() != SIM7080_STATE_NETWORKING) {
		LOG_ERR("Modem currently not attached to the network!");
		return -EAGAIN;
	}

	/* Make sure socket is allocated */
	if (modem_socket_is_allocated(&mdata.socket_config, sock) == false) {
		return 0;
	}

	/* Close the socket only if it is connected. */
	if (sock->is_connected) {
		socket_close(sock);
	}

	return 0;
}

/*
 * Polls a given socket.
 */
static int offload_poll(struct zsock_pollfd *fds, int nfds, int msecs)
{
	int i;
	void *obj;

	/* Modem is not attached to the network. */
	if (get_state() != SIM7080_STATE_NETWORKING) {
		LOG_ERR("Modem currently not attached to the network!");
		return -EAGAIN;
	}

	/* Only accept modem sockets. */
	for (i = 0; i < nfds; i++) {
		if (fds[i].fd < 0) {
			continue;
		}

		/* If vtable matches, then it's modem socket. */
		obj = zvfs_get_fd_obj(fds[i].fd,
				   (const struct fd_op_vtable *)&offload_socket_fd_op_vtable,
				   EINVAL);
		if (obj == NULL) {
			return -1;
		}
	}

	return modem_socket_poll(&mdata.socket_config, fds, nfds, msecs);
}

/*
 * Offloads ioctl. Only supported ioctl is poll_offload.
 */
static int offload_ioctl(void *obj, unsigned int request, va_list args)
{
	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE:
		return -EXDEV;

	case ZFD_IOCTL_POLL_UPDATE:
		return -EOPNOTSUPP;

	case ZFD_IOCTL_POLL_OFFLOAD: {
		/* Poll on the given socket. */
		struct zsock_pollfd *fds;
		int nfds, timeout;

		fds = va_arg(args, struct zsock_pollfd *);
		nfds = va_arg(args, int);
		timeout = va_arg(args, int);

		return offload_poll(fds, nfds, timeout);
	}

	default:
		errno = EINVAL;
		return -1;
	}
}

static const struct socket_op_vtable offload_socket_fd_op_vtable = {
	.fd_vtable = {
		.read	= offload_read,
		.write	= offload_write,
		.close	= offload_close,
		.ioctl	= offload_ioctl,
	},
	.bind		= NULL,
	.connect	= offload_connect,
	.sendto		= offload_sendto,
	.recvfrom	= offload_recvfrom,
	.listen		= NULL,
	.accept		= NULL,
	.sendmsg	= offload_sendmsg,
	.getsockopt	= NULL,
	.setsockopt	= NULL,
};

/*
 * Parses the dns response from the modem.
 *
 * Response on success:
 * +CDNSGIP: 1,<domain name>,<IPv4>[,<IPv6>]
 *
 * Response on failure:
 * +CDNSGIP: 0,<err>
 */
MODEM_CMD_DEFINE(on_cmd_cdnsgip)
{
	int state;
	char ips[256];
	size_t out_len;
	int ret = -1;

	state = atoi(argv[0]);
	if (state == 0) {
		LOG_ERR("DNS lookup failed with error %s", argv[1]);
		goto exit;
	}

	/* Offset to skip the leading " */
	out_len = net_buf_linearize(ips, sizeof(ips) - 1, data->rx_buf, 1, len);
	ips[out_len] = '\0';

	/* find trailing " */
	char *ipv4 = strstr(ips, "\"");

	if (!ipv4) {
		LOG_ERR("Malformed DNS response!!");
		goto exit;
	}

	*ipv4 = '\0';
	net_addr_pton(dns_result.ai_family, ips,
		      &((struct sockaddr_in *)&dns_result_addr)->sin_addr);
	ret = 0;

exit:
	k_sem_give(&mdata.sem_dns);
	return ret;
}

/*
 * Perform a dns lookup.
 */
static int offload_getaddrinfo(const char *node, const char *service,
			       const struct zsock_addrinfo *hints, struct zsock_addrinfo **res)
{
	struct modem_cmd cmd[] = { MODEM_CMD("+CDNSGIP: ", on_cmd_cdnsgip, 2U, ",") };
	char sendbuf[sizeof("AT+CDNSGIP=\"\",##,#####") + 128];
	uint32_t port = 0;
	int ret;

	/* Modem is not attached to the network. */
	if (get_state() != SIM7080_STATE_NETWORKING) {
		LOG_ERR("Modem currently not attached to the network!");
		return DNS_EAI_AGAIN;
	}

	/* init result */
	(void)memset(&dns_result, 0, sizeof(dns_result));
	(void)memset(&dns_result_addr, 0, sizeof(dns_result_addr));

	/* Currently only support IPv4. */
	dns_result.ai_family = AF_INET;
	dns_result_addr.sa_family = AF_INET;
	dns_result.ai_addr = &dns_result_addr;
	dns_result.ai_addrlen = sizeof(dns_result_addr);
	dns_result.ai_canonname = dns_result_canonname;
	dns_result_canonname[0] = '\0';

	if (service) {
		port = atoi(service);
		if (port < 1 || port > USHRT_MAX) {
			return DNS_EAI_SERVICE;
		}
	}

	if (port > 0U) {
		if (dns_result.ai_family == AF_INET) {
			net_sin(&dns_result_addr)->sin_port = htons(port);
		}
	}

	/* Check if node is an IP address */
	if (net_addr_pton(dns_result.ai_family, node,
			  &((struct sockaddr_in *)&dns_result_addr)->sin_addr) == 0) {
		*res = &dns_result;
		return 0;
	}

	/* user flagged node as numeric host, but we failed net_addr_pton */
	if (hints && hints->ai_flags & AI_NUMERICHOST) {
		return DNS_EAI_NONAME;
	}

	snprintk(sendbuf, sizeof(sendbuf), "AT+CDNSGIP=\"%s\",10,20000", node);
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmd, ARRAY_SIZE(cmd), sendbuf,
			     &mdata.sem_dns, MDM_DNS_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	*res = (struct zsock_addrinfo *)&dns_result;
	return 0;
}

/*
 * Free addrinfo structure.
 */
static void offload_freeaddrinfo(struct zsock_addrinfo *res)
{
	/* No need to free static memory. */
	ARG_UNUSED(res);
}

/*
 * DNS vtable.
 */
static const struct socket_dns_offload offload_dns_ops = {
	.getaddrinfo = offload_getaddrinfo,
	.freeaddrinfo = offload_freeaddrinfo,
};

static struct offloaded_if_api api_funcs = {
	.iface_api.init = modem_net_iface_init,
};

static bool offload_is_supported(int family, int type, int proto)
{
	if (family != AF_INET &&
	    family != AF_INET6) {
		return false;
	}

	if (type != SOCK_DGRAM &&
	    type != SOCK_STREAM) {
		return false;
	}

	if (proto != IPPROTO_TCP &&
	    proto != IPPROTO_UDP) {
		return false;
	}

	return true;
}

static int offload_socket(int family, int type, int proto)
{
	int ret;

	ret = modem_socket_get(&mdata.socket_config, family, type, proto);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	errno = 0;
	return ret;
}

/*
 * Process all messages received from the modem.
 */
static void modem_rx(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		/* Wait for incoming data */
		modem_iface_uart_rx_wait(&mctx.iface, K_FOREVER);

		modem_cmd_handler_process(&mctx.cmd_handler, &mctx.iface);
	}
}

MODEM_CMD_DEFINE(on_cmd_ok)
{
	modem_cmd_handler_set_error(data, 0);
	k_sem_give(&mdata.sem_response);
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_error)
{
	modem_cmd_handler_set_error(data, -EIO);
	k_sem_give(&mdata.sem_response);
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_exterror)
{
	modem_cmd_handler_set_error(data, -EIO);
	k_sem_give(&mdata.sem_response);
	return 0;
}

/*
 * Handles pdp context urc.
 *
 * The urc has the form +APP PDP: <index>,<state>.
 * State can either be ACTIVE for activation or
 * DEACTIVE if disabled.
 */
MODEM_CMD_DEFINE(on_urc_app_pdp)
{
	mdata.pdp_active = strcmp(argv[1], "ACTIVE") == 0;
	LOG_INF("PDP context: %u", mdata.pdp_active);
	k_sem_give(&mdata.sem_response);
	return 0;
}

MODEM_CMD_DEFINE(on_urc_sms)
{
	LOG_INF("SMS: %s", argv[0]);
	return 0;
}

/*
 * Handles socket data notification.
 *
 * The sim modem sends and unsolicited +CADATAIND: <cid>
 * if data can be read from a socket.
 */
MODEM_CMD_DEFINE(on_urc_cadataind)
{
	struct modem_socket *sock;
	int sock_fd;

	sock_fd = atoi(argv[0]);

	sock = modem_socket_from_fd(&mdata.socket_config, sock_fd);
	if (!sock) {
		return 0;
	}

	/* Modem does not tell packet size. Set dummy for receive. */
	modem_socket_packet_size_update(&mdata.socket_config, sock, 1);

	LOG_INF("Data available on socket: %d", sock_fd);
	modem_socket_data_ready(&mdata.socket_config, sock);

	return 0;
}

/*
 * Handles the castate response.
 *
 * +CASTATE: <cid>,<state>
 *
 * Cid is the connection id (socket fd) and
 * state can be:
 *  0 - Closed by remote server or error
 *  1 - Connected to remote server
 *  2 - Listening
 */
MODEM_CMD_DEFINE(on_urc_castate)
{
	struct modem_socket *sock;
	int sockfd, state;

	sockfd = atoi(argv[0]);
	state = atoi(argv[1]);

	sock = modem_socket_from_fd(&mdata.socket_config, sockfd);
	if (!sock) {
		return 0;
	}

	/* Only continue if socket was closed. */
	if (state != 0) {
		return 0;
	}

	LOG_INF("Socket close indication for socket: %d", sockfd);

	sock->is_connected = false;
	LOG_INF("Socket closed: %d", sockfd);

	return 0;
}

/**
 * Handles the ftpget urc.
 *
 * +FTPGET: <mode>,<error>
 *
 * Mode can be 1 for opening a session and
 * reporting that data is available or 2 for
 * reading data. This urc handler will only handle
 * mode 1 because 2 will not occur as urc.
 *
 * Error can be either:
 *  - 1 for data available/opened session.
 *  - 0 If transfer is finished.
 *  - >0 for some error.
 */
MODEM_CMD_DEFINE(on_urc_ftpget)
{
	int error = atoi(argv[0]);

	LOG_INF("+FTPGET: 1,%d", error);

	/* Transfer finished. */
	if (error == 0) {
		mdata.ftp.state = SIM7080_FTP_CONNECTION_STATE_FINISHED;
	} else if (error == 1) {
		mdata.ftp.state = SIM7080_FTP_CONNECTION_STATE_CONNECTED;
	} else {
		mdata.ftp.state = SIM7080_FTP_CONNECTION_STATE_ERROR;
	}

	k_sem_give(&mdata.sem_ftp);

	return 0;
}

/*
 * Read manufacturer identification.
 */
MODEM_CMD_DEFINE(on_cmd_cgmi)
{
	size_t out_len = net_buf_linearize(
		mdata.mdm_manufacturer, sizeof(mdata.mdm_manufacturer) - 1, data->rx_buf, 0, len);
	mdata.mdm_manufacturer[out_len] = '\0';
	LOG_INF("Manufacturer: %s", mdata.mdm_manufacturer);
	return 0;
}

/*
 * Read model identification.
 */
MODEM_CMD_DEFINE(on_cmd_cgmm)
{
	size_t out_len = net_buf_linearize(mdata.mdm_model, sizeof(mdata.mdm_model) - 1,
					   data->rx_buf, 0, len);
	mdata.mdm_model[out_len] = '\0';
	LOG_INF("Model: %s", mdata.mdm_model);
	return 0;
}

/*
 * Read software release.
 *
 * Response will be in format RESPONSE: <revision>.
 */
MODEM_CMD_DEFINE(on_cmd_cgmr)
{
	size_t out_len;
	char *p;

	out_len = net_buf_linearize(mdata.mdm_revision, sizeof(mdata.mdm_revision) - 1,
				    data->rx_buf, 0, len);
	mdata.mdm_revision[out_len] = '\0';

	/* The module prepends a Revision: */
	p = strchr(mdata.mdm_revision, ':');
	if (p) {
		out_len = strlen(p + 1);
		memmove(mdata.mdm_revision, p + 1, out_len + 1);
	}

	LOG_INF("Revision: %s", mdata.mdm_revision);
	return 0;
}

/*
 * Read serial number identification.
 */
MODEM_CMD_DEFINE(on_cmd_cgsn)
{
	size_t out_len =
		net_buf_linearize(mdata.mdm_imei, sizeof(mdata.mdm_imei) - 1, data->rx_buf, 0, len);
	mdata.mdm_imei[out_len] = '\0';
	LOG_INF("IMEI: %s", mdata.mdm_imei);
	return 0;
}

#if defined(CONFIG_MODEM_SIM_NUMBERS)
/*
 * Read international mobile subscriber identity.
 */
MODEM_CMD_DEFINE(on_cmd_cimi)
{
	size_t out_len =
		net_buf_linearize(mdata.mdm_imsi, sizeof(mdata.mdm_imsi) - 1, data->rx_buf, 0, len);
	mdata.mdm_imsi[out_len] = '\0';

	/* Log the received information. */
	LOG_INF("IMSI: %s", mdata.mdm_imsi);
	return 0;
}

/*
 * Read iccid.
 */
MODEM_CMD_DEFINE(on_cmd_ccid)
{
	size_t out_len = net_buf_linearize(mdata.mdm_iccid, sizeof(mdata.mdm_iccid) - 1,
					   data->rx_buf, 0, len);
	mdata.mdm_iccid[out_len] = '\0';

	/* Log the received information. */
	LOG_INF("ICCID: %s", mdata.mdm_iccid);
	return 0;
}
#endif /* defined(CONFIG_MODEM_SIM_NUMBERS) */

/*
 * Parses the non urc C(E)REG and updates registration status.
 */
MODEM_CMD_DEFINE(on_cmd_cereg)
{
	mdata.mdm_registration = atoi(argv[1]);
	LOG_INF("CREG: %u", mdata.mdm_registration);
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_cpin)
{
	mdata.cpin_ready = strcmp(argv[0], "READY") == 0;
	LOG_INF("CPIN: %d", mdata.cpin_ready);
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_cgatt)
{
	mdata.mdm_cgatt = atoi(argv[0]);
	LOG_INF("CGATT: %d", mdata.mdm_cgatt);
	return 0;
}

/*
 * Handler for RSSI query.
 *
 * +CSQ: <rssi>,<ber>
 *  rssi: 0,-115dBm; 1,-111dBm; 2...30,-110...-54dBm; 31,-52dBm or greater.
 *        99, ukn
 *  ber: Not used.
 */
MODEM_CMD_DEFINE(on_cmd_csq)
{
	int rssi = atoi(argv[0]);

	if (rssi == 0) {
		mdata.mdm_rssi = -115;
	} else if (rssi == 1) {
		mdata.mdm_rssi = -111;
	} else if (rssi > 1 && rssi < 31) {
		mdata.mdm_rssi = -114 + 2 * rssi;
	} else if (rssi == 31) {
		mdata.mdm_rssi = -52;
	} else {
		mdata.mdm_rssi = -1000;
	}

	LOG_INF("RSSI: %d", mdata.mdm_rssi);
	return 0;
}

/*
 * Queries modem RSSI.
 *
 * If a work queue parameter is provided query work will
 * be scheduled. Otherwise rssi is queried once.
 */
static void modem_rssi_query_work(struct k_work *work)
{
	struct modem_cmd cmd[] = { MODEM_CMD("+CSQ: ", on_cmd_csq, 2U, ",") };
	static char *send_cmd = "AT+CSQ";
	int ret;

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmd, ARRAY_SIZE(cmd), send_cmd,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("AT+CSQ ret:%d", ret);
	}

	if (work) {
		k_work_reschedule_for_queue(&modem_workq, &mdata.rssi_query_work,
					    K_SECONDS(RSSI_TIMEOUT_SECS));
	}
}

/*
 * Possible responses by the sim7080.
 */
static const struct modem_cmd response_cmds[] = {
	MODEM_CMD("OK", on_cmd_ok, 0U, ""),
	MODEM_CMD("ERROR", on_cmd_error, 0U, ""),
	MODEM_CMD("+CME ERROR: ", on_cmd_exterror, 1U, ""),
	MODEM_CMD_DIRECT(">", on_cmd_tx_ready),
};

/*
 * Possible unsolicited commands.
 */
static const struct modem_cmd unsolicited_cmds[] = {
	MODEM_CMD("+APP PDP: ", on_urc_app_pdp, 2U, ","),
	MODEM_CMD("SMS ", on_urc_sms, 1U, ""),
	MODEM_CMD("+CADATAIND: ", on_urc_cadataind, 1U, ""),
	MODEM_CMD("+CASTATE: ", on_urc_castate, 2U, ","),
	MODEM_CMD("+FTPGET: 1,", on_urc_ftpget, 1U, ""),
};

/*
 * Activates the pdp context
 */
static int modem_pdp_activate(void)
{
	int counter;
	int ret = 0;
#if defined(CONFIG_MODEM_SIMCOM_SIM7080_RAT_GSM)
	const char *buf = "AT+CREG?";
	struct modem_cmd cmds[] = { MODEM_CMD("+CREG: ", on_cmd_cereg, 2U, ",") };
#else
	const char *buf = "AT+CEREG?";
	struct modem_cmd cmds[] = { MODEM_CMD("+CEREG: ", on_cmd_cereg, 2U, ",") };
#endif /* defined(CONFIG_MODEM_SIMCOM_SIM7080_RAT_GSM) */

	struct modem_cmd cgatt_cmd[] = { MODEM_CMD("+CGATT: ", on_cmd_cgatt, 1U, "") };

	counter = 0;
	while (counter++ < MDM_MAX_CGATT_WAITS && mdata.mdm_cgatt != 1) {
		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cgatt_cmd,
				     ARRAY_SIZE(cgatt_cmd), "AT+CGATT?", &mdata.sem_response,
				     MDM_CMD_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("Failed to query cgatt!!");
			return -1;
		}

		k_sleep(K_SECONDS(1));
	}

	if (counter >= MDM_MAX_CGATT_WAITS) {
		LOG_WRN("Network attach failed!!");
		return -1;
	}

	if (!mdata.cpin_ready || mdata.mdm_cgatt != 1) {
		LOG_ERR("Fatal: Modem is not attached to GPRS network!!");
		return -1;
	}

	LOG_INF("Waiting for network");

	/* Wait until the module is registered to the network.
	 * Registration will be set by urc.
	 */
	counter = 0;
	while (counter++ < MDM_MAX_CEREG_WAITS && mdata.mdm_registration != 1 &&
	       mdata.mdm_registration != 5) {
		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmds, ARRAY_SIZE(cmds), buf,
				     &mdata.sem_response, MDM_CMD_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("Failed to query registration!!");
			return -1;
		}

		k_sleep(K_SECONDS(1));
	}

	if (counter >= MDM_MAX_CEREG_WAITS) {
		LOG_WRN("Network registration failed!");
		ret = -1;
		goto error;
	}

	/* Set dual stack mode (IPv4/IPv6) */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, "AT+CNCFG=0,0",
				&mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Could not configure pdp context!");
		goto error;
	}

	/*
	 * Now activate the pdp context and wait for confirmation.
	 */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, "AT+CNACT=0,1",
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Could not activate PDP context.");
		goto error;
	}

	ret = k_sem_take(&mdata.sem_response, MDM_PDP_TIMEOUT);
	if (ret < 0 || mdata.pdp_active == false) {
		LOG_ERR("Failed to activate PDP context.");
		ret = -1;
		goto error;
	}

	LOG_INF("Network active.");

error:
	return ret;
}

/*
 * Toggles the modems power pin.
 */
static void modem_pwrkey(void)
{
	/* Power pin should be high for 1.5 seconds. */
	gpio_pin_set_dt(&power_gpio, 1);
	k_sleep(K_MSEC(1500));
	gpio_pin_set_dt(&power_gpio, 0);
	k_sleep(K_SECONDS(5));
}

/*
 * Commands to be sent at setup.
 */
static const struct setup_cmd setup_cmds[] = {
	SETUP_CMD_NOHANDLE("ATH"),
	SETUP_CMD("AT+CGMI", "", on_cmd_cgmi, 0U, ""),
	SETUP_CMD("AT+CGMM", "", on_cmd_cgmm, 0U, ""),
	SETUP_CMD("AT+CGMR", "", on_cmd_cgmr, 0U, ""),
	SETUP_CMD("AT+CGSN", "", on_cmd_cgsn, 0U, ""),
#if defined(CONFIG_MODEM_SIM_NUMBERS)
	SETUP_CMD("AT+CIMI", "", on_cmd_cimi, 0U, ""),
	SETUP_CMD("AT+CCID", "", on_cmd_ccid, 0U, ""),
#endif /* defined(CONFIG_MODEM_SIM_NUMBERS) */
#if defined(CONFIG_MODEM_SIMCOM_SIM7080_RAT_NB1)
	SETUP_CMD_NOHANDLE("AT+CNMP=38"),
	SETUP_CMD_NOHANDLE("AT+CMNB=2"),
	SETUP_CMD_NOHANDLE("AT+CBANDCFG=\"NB-IOT\"," MDM_LTE_BANDS),
#endif /* defined(CONFIG_MODEM_SIMCOM_SIM7080_RAT_NB1) */
#if defined(CONFIG_MODEM_SIMCOM_SIM7080_RAT_M1)
	SETUP_CMD_NOHANDLE("AT+CNMP=38"),
	SETUP_CMD_NOHANDLE("AT+CMNB=1"),
	SETUP_CMD_NOHANDLE("AT+CBANDCFG=\"CAT-M\"," MDM_LTE_BANDS),
#endif /* defined(CONFIG_MODEM_SIMCOM_SIM7080_RAT_M1) */
#if defined(CONFIG_MODEM_SIMCOM_SIM7080_RAT_GSM)
	SETUP_CMD_NOHANDLE("AT+CNMP=13"),
#endif /* defined(CONFIG_MODEM_SIMCOM_SIM7080_RAT_GSM) */
	SETUP_CMD("AT+CPIN?", "+CPIN: ", on_cmd_cpin, 1U, ""),
};

/**
 * Performs the autobaud sequence until modem answers or limit is reached.
 *
 * @return On successful boot 0 is returned. Otherwise <0 is returned.
 */
static int modem_autobaud(void)
{
	int boot_tries = 0;
	int counter = 0;
	int ret;

	while (boot_tries++ <= MDM_BOOT_TRIES) {
		modem_pwrkey();

		/*
		 * The sim7080 has a autobaud function.
		 * On startup multiple AT's are sent until
		 * a OK is received.
		 */
		counter = 0;
		while (counter < MDM_MAX_AUTOBAUD) {
			ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, "AT",
					     &mdata.sem_response, K_MSEC(500));

			/* OK was received. */
			if (ret == 0) {
				/* Disable echo */
				return modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U,
						      "ATE0", &mdata.sem_response, K_SECONDS(2));
			}

			counter++;
		}
	}

	return -1;
}

/**
 * Get the next parameter from the gnss phrase.
 *
 * @param src The source string supported on first call.
 * @param delim The delimiter of the parameter list.
 * @param saveptr Pointer for subsequent parses.
 * @return On success a pointer to the parameter. On failure
 *         or end of string NULL is returned.
 *
 * This function is used instead of strtok because strtok would
 * skip empty parameters, which is not desired. The modem may
 * omit parameters which could lead to a incorrect parse.
 */
static char *gnss_get_next_param(char *src, const char *delim, char **saveptr)
{
	char *start, *del;

	if (src) {
		start = src;
	} else {
		start = *saveptr;
	}

	/* Illegal start string. */
	if (!start) {
		return NULL;
	}

	/* End of string reached. */
	if (*start == '\0' || *start == '\r') {
		return NULL;
	}

	del = strstr(start, delim);
	if (!del) {
		return NULL;
	}

	*del = '\0';
	*saveptr = del + 1;

	if (del == start) {
		return NULL;
	}

	return start;
}

static void gnss_skip_param(char **saveptr)
{
	gnss_get_next_param(NULL, ",", saveptr);
}

/**
 * Splits float parameters of the CGNSINF response on '.'
 *
 * @param src Null terminated string containing the float.
 * @param f1 Resulting number part of the float.
 * @param f2 Resulting fraction part of the float.
 * @return 0 if parsing was successful. Otherwise <0 is returned.
 *
 * If the number part of the float is negative f1 and f2 will be
 * negative too.
 */
static int gnss_split_on_dot(const char *src, int32_t *f1, int32_t *f2)
{
	char *dot = strchr(src, '.');

	if (!dot) {
		return -1;
	}

	*dot = '\0';

	*f1 = (int32_t)strtol(src, NULL, 10);
	*f2 = (int32_t)strtol(dot + 1, NULL, 10);

	if (*f1 < 0) {
		*f2 = -*f2;
	}

	return 0;
}

/**
 * Parses cgnsinf response into the gnss_data structure.
 *
 * @param gps_buf Null terminated buffer containing the response.
 * @return 0 on successful parse. Otherwise <0 is returned.
 */
static int parse_cgnsinf(char *gps_buf)
{
	char *saveptr;
	int ret;
	int32_t number, fraction;

	char *run_status = gnss_get_next_param(gps_buf, ",", &saveptr);

	if (run_status == NULL) {
		goto error;
	} else if (*run_status != '1') {
		goto error;
	}

	char *fix_status = gnss_get_next_param(NULL, ",", &saveptr);

	if (fix_status == NULL) {
		goto error;
	} else if (*fix_status != '1') {
		goto error;
	}

	char *utc = gnss_get_next_param(NULL, ",", &saveptr);

	if (utc == NULL) {
		goto error;
	}

	char *lat = gnss_get_next_param(NULL, ",", &saveptr);

	if (lat == NULL) {
		goto error;
	}

	char *lon = gnss_get_next_param(NULL, ",", &saveptr);

	if (lon == NULL) {
		goto error;
	}

	char *alt = gnss_get_next_param(NULL, ",", &saveptr);
	char *speed = gnss_get_next_param(NULL, ",", &saveptr);
	char *course = gnss_get_next_param(NULL, ",", &saveptr);

	/* discard fix mode and reserved*/
	gnss_skip_param(&saveptr);
	gnss_skip_param(&saveptr);

	char *hdop = gnss_get_next_param(NULL, ",", &saveptr);

	if (hdop == NULL) {
		goto error;
	}

	gnss_data.run_status = 1;
	gnss_data.fix_status = 1;

	strncpy(gnss_data.utc, utc, sizeof(gnss_data.utc) - 1);

	ret = gnss_split_on_dot(lat, &number, &fraction);
	if (ret != 0) {
		goto error;
	}
	gnss_data.lat = number * 10000000 + fraction * 10;

	ret = gnss_split_on_dot(lon, &number, &fraction);
	if (ret != 0) {
		goto error;
	}
	gnss_data.lon = number * 10000000 + fraction * 10;

	if (alt) {
		ret = gnss_split_on_dot(alt, &number, &fraction);
		if (ret != 0) {
			goto error;
		}
		gnss_data.alt = number * 1000 + fraction;
	} else {
		gnss_data.alt = 0;
	}

	ret = gnss_split_on_dot(hdop, &number, &fraction);
	if (ret != 0) {
		goto error;
	}
	gnss_data.hdop = number * 100 + fraction * 10;

	if (course) {
		ret = gnss_split_on_dot(course, &number, &fraction);
		if (ret != 0) {
			goto error;
		}
		gnss_data.cog = number * 100 + fraction * 10;
	} else {
		gnss_data.cog = 0;
	}

	if (speed) {
		ret = gnss_split_on_dot(speed, &number, &fraction);
		if (ret != 0) {
			goto error;
		}
		gnss_data.kmh = number * 10 + fraction / 10;
	} else {
		gnss_data.kmh = 0;
	}

	return 0;
error:
	memset(&gnss_data, 0, sizeof(gnss_data));
	return -1;
}

/*
 * Parses the +CGNSINF Gnss response.
 *
 * The CGNSINF command has the following parameters but
 * not all parameters are set by the module:
 *
 *  +CGNSINF: <GNSS run status>,<Fix status>,<UTC date & Time>,
 *            <Latitude>,<Longitude>,<MSL Altitude>,<Speed Over Ground>,
 *            <Course Over Ground>,<Fix Mode>,<Reserved1>,<HDOP>,<PDOP>,
 *            <VDOP>,<Reserved2>,<GNSS Satellites in View>,<Reserved3>,
 *            <HPA>,<VPA>
 *
 */
MODEM_CMD_DEFINE(on_cmd_cgnsinf)
{
	char gps_buf[MDM_GNSS_PARSER_MAX_LEN];
	size_t out_len = net_buf_linearize(gps_buf, sizeof(gps_buf) - 1, data->rx_buf, 0, len);

	gps_buf[out_len] = '\0';
	return parse_cgnsinf(gps_buf);
}

int mdm_sim7080_query_gnss(struct sim7080_gnss_data *data)
{
	int ret;
	struct modem_cmd cmds[] = { MODEM_CMD("+CGNSINF: ", on_cmd_cgnsinf, 0U, NULL) };

	if (get_state() != SIM7080_STATE_GNSS) {
		LOG_ERR("GNSS functionality is not enabled!!");
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmds, ARRAY_SIZE(cmds), "AT+CGNSINF",
			     &mdata.sem_response, K_SECONDS(2));
	if (ret < 0) {
		return ret;
	}

	if (!gnss_data.run_status || !gnss_data.fix_status) {
		return -EAGAIN;
	}

	if (data) {
		memcpy(data, &gnss_data, sizeof(gnss_data));
	}

	memset(&gnss_data, 0, sizeof(gnss_data));

	return ret;
}

int mdm_sim7080_start_gnss(void)
{
	int ret;

	change_state(SIM7080_STATE_INIT);
	k_work_cancel_delayable(&mdata.rssi_query_work);

	ret = modem_autobaud();
	if (ret < 0) {
		LOG_ERR("Failed to start modem!!");
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, "AT+CGNSCOLD",
			     &mdata.sem_response, K_SECONDS(2));
	if (ret < 0) {
		return -1;
	}

	change_state(SIM7080_STATE_GNSS);
	return 0;
}

/**
 * Parse the +FTPGET response.
 *
 * +FTPGET: <mode>,<len>
 *
 * Mode is hard set to 2.
 *
 * Length is the number of bytes following (the ftp data).
 */
MODEM_CMD_DEFINE(on_cmd_ftpget)
{
	int nbytes = atoi(argv[0]);
	int bytes_to_skip;
	size_t out_len;

	if (nbytes == 0) {
		mdata.ftp.nread = 0;
		return 0;
	}

	/* Skip length parameter and trailing \r\n */
	bytes_to_skip = strlen(argv[0]) + 2;

	/* Wait until data is ready.
	 * >= to ensure buffer is not empty after skip.
	 */
	if (net_buf_frags_len(data->rx_buf) <= nbytes + bytes_to_skip) {
		return -EAGAIN;
	}

	out_len = net_buf_linearize(mdata.ftp.read_buffer, mdata.ftp.nread, data->rx_buf,
				    bytes_to_skip, nbytes);
	if (out_len != nbytes) {
		LOG_WRN("FTP read size differs!");
	}
	data->rx_buf = net_buf_skip(data->rx_buf, nbytes + bytes_to_skip);

	mdata.ftp.nread = nbytes;

	return 0;
}

int mdm_sim7080_ftp_get_read(char *dst, size_t *size)
{
	int ret;
	char buffer[sizeof("AT+FTPGET=#,######")];
	struct modem_cmd cmds[] = { MODEM_CMD("+FTPGET: 2,", on_cmd_ftpget, 1U, "") };

	/* Some error occurred. */
	if (mdata.ftp.state == SIM7080_FTP_CONNECTION_STATE_ERROR ||
	    mdata.ftp.state == SIM7080_FTP_CONNECTION_STATE_INITIAL) {
		return SIM7080_FTP_RC_ERROR;
	}

	/* Setup buffer. */
	mdata.ftp.read_buffer = dst;
	mdata.ftp.nread = *size;

	/* Read ftp data. */
	ret = snprintk(buffer, sizeof(buffer), "AT+FTPGET=2,%zu", *size);
	if (ret < 0) {
		*size = 0;
		return SIM7080_FTP_RC_ERROR;
	}

	/* Wait for data from the server. */
	k_sem_take(&mdata.sem_ftp, K_MSEC(200));

	if (mdata.ftp.state == SIM7080_FTP_CONNECTION_STATE_FINISHED) {
		*size = 0;
		return SIM7080_FTP_RC_FINISHED;
	} else if (mdata.ftp.state == SIM7080_FTP_CONNECTION_STATE_ERROR) {
		*size = 0;
		return SIM7080_FTP_RC_ERROR;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmds, ARRAY_SIZE(cmds), buffer,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		*size = 0;
		return SIM7080_FTP_RC_ERROR;
	}

	/* Set read size. */
	*size = mdata.ftp.nread;

	return SIM7080_FTP_RC_OK;
}

int mdm_sim7080_ftp_get_start(const char *server, const char *user, const char *passwd,
			      const char *file, const char *path)
{
	int ret;
	char buffer[256];

	/* Start network. */
	ret = mdm_sim7080_start_network();
	if (ret < 0) {
		LOG_ERR("Failed to start network for FTP!");
		return -1;
	}

	/* Set connection id for ftp. */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, "AT+FTPCID=0",
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to set FTP Cid!");
		return -1;
	}

	/* Set ftp server. */
	ret = snprintk(buffer, sizeof(buffer), "AT+FTPSERV=\"%s\"", server);
	if (ret < 0) {
		LOG_WRN("Failed to build command!");
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buffer, &mdata.sem_response,
			     MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to set FTP Cid!");
		return -1;
	}

	/* Set ftp user. */
	ret = snprintk(buffer, sizeof(buffer), "AT+FTPUN=\"%s\"", user);
	if (ret < 0) {
		LOG_WRN("Failed to build command!");
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buffer, &mdata.sem_response,
			     MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to set ftp user!");
		return -1;
	}

	/* Set ftp password. */
	ret = snprintk(buffer, sizeof(buffer), "AT+FTPPW=\"%s\"", passwd);
	if (ret < 0) {
		LOG_WRN("Failed to build command!");
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buffer, &mdata.sem_response,
			     MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to set ftp password!");
		return -1;
	}

	/* Set ftp filename. */
	ret = snprintk(buffer, sizeof(buffer), "AT+FTPGETNAME=\"%s\"", file);
	if (ret < 0) {
		LOG_WRN("Failed to build command!");
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buffer, &mdata.sem_response,
			     MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to set ftp filename!");
		return -1;
	}

	/* Set ftp filename. */
	ret = snprintk(buffer, sizeof(buffer), "AT+FTPGETNAME=\"%s\"", file);
	if (ret < 0) {
		LOG_WRN("Failed to build command!");
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buffer, &mdata.sem_response,
			     MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to set ftp filename!");
		return -1;
	}

	/* Set ftp path. */
	ret = snprintk(buffer, sizeof(buffer), "AT+FTPGETPATH=\"%s\"", path);
	if (ret < 0) {
		LOG_WRN("Failed to build command!");
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, buffer, &mdata.sem_response,
			     MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to set ftp path!");
		return -1;
	}

	/* Initialize ftp variables. */
	mdata.ftp.read_buffer = NULL;
	mdata.ftp.nread = 0;
	mdata.ftp.state = SIM7080_FTP_CONNECTION_STATE_INITIAL;

	/* Start the ftp session. */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, "AT+FTPGET=1",
			     &mdata.sem_ftp, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to start session!");
		return -1;
	}

	if (mdata.ftp.state != SIM7080_FTP_CONNECTION_STATE_CONNECTED) {
		LOG_WRN("Session state is not connected!");
		return -1;
	}

	return 0;
}

/**
 * Decode readable hex to "real" hex.
 */
static uint8_t mdm_pdu_decode_ascii(char byte)
{
	if ((byte >= '0') && (byte <= '9')) {
		return byte - '0';
	} else if ((byte >= 'A') && (byte <= 'F')) {
		return byte - 'A' + 10;
	} else if ((byte >= 'a') && (byte <= 'f')) {
		return byte - 'a' + 10;
	} else {
		return 255;
	}
}

/**
 * Reads "byte" from pdu.
 *
 * @param pdu pdu to read from.
 * @param index index of "byte".
 *
 * Sim module "encodes" one pdu byte as two human readable bytes
 * this functions squashes these two bytes into one.
 */
static uint8_t mdm_pdu_read_byte(const char *pdu, size_t index)
{
	return (mdm_pdu_decode_ascii(pdu[index * 2]) << 4 |
		mdm_pdu_decode_ascii(pdu[index * 2 + 1]));
}

/**
 * Decodes time from pdu.
 *
 * @param pdu pdu to read from.
 * @param index index of "byte".
 */
static uint8_t mdm_pdu_read_time(const char *pdu, size_t index)
{
	return (mdm_pdu_decode_ascii(pdu[index * 2]) +
		mdm_pdu_decode_ascii(pdu[index * 2 + 1]) * 10);
}

/**
 * Decode a sms from pdu mode.
 */
static int mdm_decode_pdu(const char *pdu, size_t pdu_len, struct sim7080_sms *target_buf)
{
	size_t index;

	/*
	 * GSM_03.38 to Unicode conversion table
	 */
	const short enc7_basic[128] = {
		'@',	0xA3,	'$',	0xA5,	0xE8,	0xE9,	0xF9,	0xEC,	0xF2,	0xE7,
		'\n',	0xD8,	0xF8,	'\r',	0xC5,	0xF8,	0x0394, '_',	0x03A6, 0x0393,
		0x039B, 0x03A9, 0x03A0, 0x03A8, 0x03A3, 0x0398, 0x039E, '\x1b', 0xC6,	0xE6,
		0xDF,	0xC9,	' ',	'!',	'\"',	'#',	0xA4,	'%',	'&',	'\'',
		'(',	')',	'*',	'+',	',',	'-',	'.',	'/',	'0',	'1',
		'2',	'3',	'4',	'5',	'6',	'7',	'8',	'9',	':',	';',
		'<',	'=',	'>',	'?',	0xA1,	'A',	'B',	'C',	'D',	'E',
		'F',	'G',	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
		'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',	'X',	'Y',
		'Z',	0xC4,	0xD6,	0xD1,	0xDC,	0xA7,	0xBF,	'a',	'b',	'c',
		'd',	'e',	'f',	'g',	'h',	'i',	'j',	'k',	'l',	'm',
		'n',	'o',	'p',	'q',	'r',	's',	't',	'u',	'v',	'w',
		'x',	'y',	'z',	0xE4,	0xF6,	0xF1,	0xFC,	0xE0
	};

	/* two bytes in pdu are on real byte */
	pdu_len = (pdu_len / 2);

	/* first byte of pdu is length of trailing SMSC information
	 * skip it by setting index to SMSC length + 1.
	 */
	index = mdm_pdu_read_byte(pdu, 0) + 1;

	if (index >= pdu_len) {
		return -1;
	}

	/* read first octet */
	target_buf->first_octet = mdm_pdu_read_byte(pdu, index++);

	if (index >= pdu_len) {
		return -1;
	}

	/* pdu_index now points to the address field.
	 * first byte of addr field is the addr length -> skip it.
	 * address type is not included in addr len -> add +1.
	 * address is coded in semi octets
	 *  + addr_len/2 if even
	 *  + addr_len/2 + 1 if odd
	 */
	uint8_t addr_len = mdm_pdu_read_byte(pdu, index);

	index += ((addr_len % 2) == 0) ? (addr_len / 2) + 2 : (addr_len / 2) + 3;

	if (index >= pdu_len) {
		return -1;
	}

	/* read protocol identifier */
	target_buf->tp_pid = mdm_pdu_read_byte(pdu, index++);

	if (index >= pdu_len) {
		return -1;
	}

	/* read coding scheme */
	uint8_t tp_dcs = mdm_pdu_read_byte(pdu, index++);

	/* parse date and time */
	if ((index + 7) >= pdu_len) {
		return -1;
	}

	target_buf->time.year = mdm_pdu_read_time(pdu, index++);
	target_buf->time.month = mdm_pdu_read_time(pdu, index++);
	target_buf->time.day = mdm_pdu_read_time(pdu, index++);
	target_buf->time.hour = mdm_pdu_read_time(pdu, index++);
	target_buf->time.minute = mdm_pdu_read_time(pdu, index++);
	target_buf->time.second = mdm_pdu_read_time(pdu, index++);
	target_buf->time.timezone = mdm_pdu_read_time(pdu, index++);

	/* Read user data length */
	uint8_t tp_udl = mdm_pdu_read_byte(pdu, index++);

	/* Discard header */
	uint8_t header_skip = 0;

	if (target_buf->first_octet & SMS_TP_UDHI_HEADER) {
		uint8_t tp_udhl = mdm_pdu_read_byte(pdu, index);

		index += tp_udhl + 1;
		header_skip = tp_udhl + 1;

		if (index >= pdu_len) {
			return -1;
		}
	}

	/* Read data according to type set in TP-DCS */
	if (tp_dcs == 0x00) {
		/* 7 bit GSM coding */
		uint8_t fill_level = 0;
		uint16_t buf = 0;

		if (target_buf->first_octet & SMS_TP_UDHI_HEADER) {
			/* Initial fill because septets are aligned to
			 * septet boundary after header
			 */
			uint8_t fill_bits = 7 - ((header_skip * 8) % 7);

			if (fill_bits == 7) {
				fill_bits = 0;
			}

			buf = mdm_pdu_read_byte(pdu, index++);

			fill_level = 8 - fill_bits;
		}

		uint16_t data_index = 0;

		for (unsigned int idx = 0; idx < tp_udl; idx++) {
			if (fill_level < 7) {
				uint8_t octet = mdm_pdu_read_byte(pdu, index++);

				buf &= ((1 << fill_level) - 1);
				buf |= (octet << fill_level);
				fill_level += 8;
			}

			/*
			 * Convert 7-bit encoded data to Unicode and
			 * then to UTF-8
			 */
			short letter = enc7_basic[buf & 0x007f];

			if (letter < 0x0080) {
				target_buf->data[data_index++] = letter & 0x007f;
			} else if (letter < 0x0800) {
				target_buf->data[data_index++] = 0xc0 | ((letter & 0x07c0) >> 6);
				target_buf->data[data_index++] = 0x80 | ((letter & 0x003f) >> 0);
			}
			buf >>= 7;
			fill_level -= 7;
		}
		target_buf->data_len = data_index;
	} else if (tp_dcs == 0x04) {
		/* 8 bit binary coding */
		for (int idx = 0; idx < tp_udl - header_skip; idx++) {
			target_buf->data[idx] = mdm_pdu_read_byte(pdu, index++);
		}
		target_buf->data_len = tp_udl;
	} else if (tp_dcs == 0x08) {
		/* Unicode (16 bit per character) */
		for (int idx = 0; idx < tp_udl - header_skip; idx++) {
			target_buf->data[idx] = mdm_pdu_read_byte(pdu, index++);
		}
		target_buf->data_len = tp_udl;
	} else {
		return -1;
	}

	return 0;
}

/**
 * Check if given char sequence is crlf.
 *
 * @param c The char sequence.
 * @param len Total length of the fragment.
 * @return @c true if char sequence is crlf.
 *         Otherwise @c false is returned.
 */
static bool is_crlf(uint8_t *c, uint8_t len)
{
	/* crlf does not fit. */
	if (len < 2) {
		return false;
	}

	return c[0] == '\r' && c[1] == '\n';
}

/**
 * Find terminating crlf in a netbuffer.
 *
 * @param buf The netbuffer.
 * @param skip Bytes to skip before search.
 * @return Length of the returned fragment or 0 if not found.
 */
static size_t net_buf_find_crlf(struct net_buf *buf, size_t skip)
{
	size_t len = 0, pos = 0;
	struct net_buf *frag = buf;

	/* Skip to the start. */
	while (frag && skip >= frag->len) {
		skip -= frag->len;
		frag = frag->frags;
	}

	/* Need to wait for more data. */
	if (!frag) {
		return 0;
	}

	pos = skip;

	while (frag && !is_crlf(frag->data + pos, frag->len - pos)) {
		if (pos + 1 >= frag->len) {
			len += frag->len;
			frag = frag->frags;
			pos = 0U;
		} else {
			pos++;
		}
	}

	if (frag && is_crlf(frag->data + pos, frag->len - pos)) {
		len += pos;
		return len - skip;
	}

	return 0;
}

/**
 * Parses list sms and add them to buffer.
 * Format is:
 *
 * +CMGL: <index>,<stat>,,<length><CR><LF><pdu><CR><LF>
 * +CMGL: <index>,<stat>,,<length><CR><LF><pdu><CR><LF>
 * ...
 * OK
 */
MODEM_CMD_DEFINE(on_cmd_cmgl)
{
	int sms_index, sms_stat, ret;
	char pdu_buffer[256];
	size_t out_len, sms_len, param_len;
	struct sim7080_sms *sms;

	sms_index = atoi(argv[0]);
	sms_stat = atoi(argv[1]);

	/* Get the length of the "length" parameter.
	 * The last parameter will be stuck in the netbuffer.
	 * It is not the actual length of the trailing pdu so
	 * we have to search the next crlf.
	 */
	param_len = net_buf_find_crlf(data->rx_buf, 0);
	if (param_len == 0) {
		LOG_INF("No <CR><LF>");
		return -EAGAIN;
	}

	/* Get actual trailing pdu len. +2 to skip crlf. */
	sms_len = net_buf_find_crlf(data->rx_buf, param_len + 2);
	if (sms_len == 0) {
		return -EAGAIN;
	}

	/* Skip to start of pdu. */
	data->rx_buf = net_buf_skip(data->rx_buf, param_len + 2);

	out_len = net_buf_linearize(pdu_buffer, sizeof(pdu_buffer) - 1, data->rx_buf, 0, sms_len);
	pdu_buffer[out_len] = '\0';

	data->rx_buf = net_buf_skip(data->rx_buf, sms_len);

	/* No buffer specified. */
	if (!mdata.sms_buffer) {
		return 0;
	}

	/* No space left in buffer. */
	if (mdata.sms_buffer_pos >= mdata.sms_buffer->nsms) {
		return 0;
	}

	sms = &mdata.sms_buffer->sms[mdata.sms_buffer_pos];

	ret = mdm_decode_pdu(pdu_buffer, out_len, sms);
	if (ret < 0) {
		return 0;
	}

	sms->stat = sms_stat;
	sms->index = sms_index;
	sms->data[sms->data_len] = '\0';

	mdata.sms_buffer_pos++;

	return 0;
}

int mdm_sim7080_read_sms(struct sim7080_sms_buffer *buffer)
{
	int ret;
	struct modem_cmd cmds[] = { MODEM_CMD("+CMGL: ", on_cmd_cmgl, 4U, ",\r") };

	mdata.sms_buffer = buffer;
	mdata.sms_buffer_pos = 0;

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmds, ARRAY_SIZE(cmds), "AT+CMGL=4",
			     &mdata.sem_response, K_SECONDS(20));
	if (ret < 0) {
		return -1;
	}

	return mdata.sms_buffer_pos;
}

int mdm_sim7080_delete_sms(uint16_t index)
{
	int ret;
	char buf[sizeof("AT+CMGD=#####")] = { 0 };

	ret = snprintk(buf, sizeof(buf), "AT+CMGD=%u", index);
	if (ret < 0) {
		return -1;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, buf, &mdata.sem_response,
			     K_SECONDS(5));
	if (ret < 0) {
		return -1;
	}

	return 0;
}

/*
 * Does the modem setup by starting it and
 * bringing the modem to a PDP active state.
 */
static int modem_setup(void)
{
	int ret = 0;
	int counter = 0;

	k_work_cancel_delayable(&mdata.rssi_query_work);

	ret = modem_autobaud();
	if (ret < 0) {
		LOG_ERR("Booting modem failed!!");
		goto error;
	}

	ret = modem_cmd_handler_setup_cmds(&mctx.iface, &mctx.cmd_handler, setup_cmds,
					   ARRAY_SIZE(setup_cmds), &mdata.sem_response,
					   MDM_REGISTRATION_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Failed to send init commands!");
		goto error;
	}

	k_sleep(K_SECONDS(3));

	/* Wait for acceptable rssi values. */
	modem_rssi_query_work(NULL);
	k_sleep(MDM_WAIT_FOR_RSSI_DELAY);

	counter = 0;
	while (counter++ < MDM_WAIT_FOR_RSSI_COUNT &&
	       (mdata.mdm_rssi >= 0 || mdata.mdm_rssi <= -1000)) {
		modem_rssi_query_work(NULL);
		k_sleep(MDM_WAIT_FOR_RSSI_DELAY);
	}

	if (mdata.mdm_rssi >= 0 || mdata.mdm_rssi <= -1000) {
		LOG_ERR("Network not reachable!!");
		ret = -ENETUNREACH;
		goto error;
	}

	ret = modem_pdp_activate();
	if (ret < 0) {
		goto error;
	}

	k_work_reschedule_for_queue(&modem_workq, &mdata.rssi_query_work,
				    K_SECONDS(RSSI_TIMEOUT_SECS));

	change_state(SIM7080_STATE_NETWORKING);

error:
	return ret;
}

int mdm_sim7080_start_network(void)
{
	change_state(SIM7080_STATE_INIT);
	return modem_setup();
}

int mdm_sim7080_power_on(void)
{
	return modem_autobaud();
}

int mdm_sim7080_power_off(void)
{
	int tries = 5;
	int autobaud_tries;
	int ret = 0;

	k_work_cancel_delayable(&mdata.rssi_query_work);

	/* Check if module is already off. */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, "AT", &mdata.sem_response,
			     K_MSEC(1000));
	if (ret < 0) {
		change_state(SIM7080_STATE_OFF);
		return 0;
	}

	while (tries--) {
		modem_pwrkey();

		autobaud_tries = 5;

		while (autobaud_tries--) {
			ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0U, "AT",
					     &mdata.sem_response, K_MSEC(500));
			if (ret == 0) {
				break;
			}
		}

		if (ret < 0) {
			change_state(SIM7080_STATE_OFF);
			return 0;
		}
	}

	return -1;
}

const char *mdm_sim7080_get_manufacturer(void)
{
	return mdata.mdm_manufacturer;
}

const char *mdm_sim7080_get_model(void)
{
	return mdata.mdm_model;
}

const char *mdm_sim7080_get_revision(void)
{
	return mdata.mdm_revision;
}

const char *mdm_sim7080_get_imei(void)
{
	return mdata.mdm_imei;
}

/*
 * Initializes modem handlers and context.
 * After successful init this function calls
 * modem_setup.
 */
static int modem_init(const struct device *dev)
{
	int ret;

	ARG_UNUSED(dev);

	k_sem_init(&mdata.sem_response, 0, 1);
	k_sem_init(&mdata.sem_tx_ready, 0, 1);
	k_sem_init(&mdata.sem_dns, 0, 1);
	k_sem_init(&mdata.sem_ftp, 0, 1);
	k_work_queue_start(&modem_workq, modem_workq_stack,
			   K_KERNEL_STACK_SIZEOF(modem_workq_stack), K_PRIO_COOP(7), NULL);

	/* Assume the modem is not registered to the network. */
	mdata.mdm_registration = 0;
	mdata.cpin_ready = false;
	mdata.pdp_active = false;

	mdata.sms_buffer = NULL;
	mdata.sms_buffer_pos = 0;

	/* Socket config. */
	ret = modem_socket_init(&mdata.socket_config, &mdata.sockets[0], ARRAY_SIZE(mdata.sockets),
				MDM_BASE_SOCKET_NUM, true, &offload_socket_fd_op_vtable);
	if (ret < 0) {
		goto error;
	}

	change_state(SIM7080_STATE_INIT);

	/* Command handler. */
	const struct modem_cmd_handler_config cmd_handler_config = {
		.match_buf = &mdata.cmd_match_buf[0],
		.match_buf_len = sizeof(mdata.cmd_match_buf),
		.buf_pool = &mdm_recv_pool,
		.alloc_timeout = BUF_ALLOC_TIMEOUT,
		.eol = "\r\n",
		.user_data = NULL,
		.response_cmds = response_cmds,
		.response_cmds_len = ARRAY_SIZE(response_cmds),
		.unsol_cmds = unsolicited_cmds,
		.unsol_cmds_len = ARRAY_SIZE(unsolicited_cmds),
	};

	ret = modem_cmd_handler_init(&mctx.cmd_handler, &mdata.cmd_handler_data,
				     &cmd_handler_config);
	if (ret < 0) {
		goto error;
	}

	/* Uart handler. */
	const struct modem_iface_uart_config uart_config = {
		.rx_rb_buf = &mdata.iface_rb_buf[0],
		.rx_rb_buf_len = sizeof(mdata.iface_rb_buf),
		.dev = MDM_UART_DEV,
		.hw_flow_control = DT_PROP(MDM_UART_NODE, hw_flow_control),
	};

	ret = modem_iface_uart_init(&mctx.iface, &mdata.iface_data, &uart_config);
	if (ret < 0) {
		goto error;
	}

	mdata.current_sock_fd = -1;
	mdata.current_sock_written = 0;

	mdata.ftp.read_buffer = NULL;
	mdata.ftp.nread = 0;
	mdata.ftp.state = SIM7080_FTP_CONNECTION_STATE_INITIAL;

	/* Modem data storage. */
	mctx.data_manufacturer = mdata.mdm_manufacturer;
	mctx.data_model = mdata.mdm_model;
	mctx.data_revision = mdata.mdm_revision;
	mctx.data_imei = mdata.mdm_imei;
#if defined(CONFIG_MODEM_SIM_NUMBERS)
	mctx.data_imsi = mdata.mdm_imsi;
	mctx.data_iccid = mdata.mdm_iccid;
#endif /* #if defined(CONFIG_MODEM_SIM_NUMBERS) */
	mctx.data_rssi = &mdata.mdm_rssi;

	ret = gpio_pin_configure_dt(&power_gpio, GPIO_OUTPUT_LOW);
	if (ret < 0) {
		LOG_ERR("Failed to configure %s pin", "power");
		goto error;
	}

	mctx.driver_data = &mdata;

	memset(&gnss_data, 0, sizeof(gnss_data));

	ret = modem_context_register(&mctx);
	if (ret < 0) {
		LOG_ERR("Error registering modem context: %d", ret);
		goto error;
	}

	k_thread_create(&modem_rx_thread, modem_rx_stack, K_KERNEL_STACK_SIZEOF(modem_rx_stack),
			modem_rx, NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	/* Init RSSI query */
	k_work_init_delayable(&mdata.rssi_query_work, modem_rssi_query_work);

	return modem_setup();
error:
	return ret;
}

/* Register device with the networking stack. */
NET_DEVICE_DT_INST_OFFLOAD_DEFINE(0, modem_init, NULL, &mdata, NULL,
				  CONFIG_MODEM_SIMCOM_SIM7080_INIT_PRIORITY, &api_funcs,
				  MDM_MAX_DATA_LENGTH);

NET_SOCKET_OFFLOAD_REGISTER(simcom_sim7080, CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY,
			    AF_UNSPEC, offload_is_supported, offload_socket);
