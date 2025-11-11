/*
 * Copyright (C) 2025 metraTec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT simcom_sim7080

#include <zephyr/logging/log.h>
#include <zephyr/net/offloaded_netdev.h>
LOG_MODULE_REGISTER(modem_simcom_sim7080_sock, CONFIG_MODEM_LOG_LEVEL);

#include <zephyr/drivers/modem/simcom-sim7080.h>
#include "sim7080.h"

static void socket_close(struct modem_socket *sock);

/*
 * Parses the +CAOPEN command and gives back the
 * connect semaphore.
 */
MODEM_CMD_DEFINE(on_cmd_caopen)
{
	int result = atoi(argv[1]);

	LOG_INF("+CAOPEN: %d", result);
	mdata.socket_open_rc = result;
	return 0;
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
	char buf[sizeof("AT+CAOPEN: #,#,#####,"
			"#xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxx.xxx.xxx.xxx#,####")];
	char ip_str[NET_IPV6_ADDR_LEN];
	int ret;

	/* Modem is not attached to the network. */
	if (sim7080_get_state() != SIM7080_STATE_NETWORKING) {
		return -EINVAL;
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

	ret = snprintk(buf, sizeof(buf), "AT+CAOPEN=%d,0,\"%s\",\"%s\",%d", sock->id,
			   protocol, ip_str, dst_port);
	if (ret < 0) {
		LOG_ERR("Failed to build connect command. ID: %d, FD: %d", sock->id, sock->sock_fd);
		errno = ENOMEM;
		return -1;
	}

	mdata.socket_open_rc = 1;
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmd, ARRAY_SIZE(cmd), buf,
				 &mdata.sem_response, MDM_CONNECT_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret: %d", buf, ret);
		goto error;
	}

	if (mdata.socket_open_rc != 0) {
		LOG_ERR("Failed to open the socket: %u", mdata.socket_open_rc);
		ret = -ENOTCONN;
		goto error;
	}

	sock->is_connected = true;
	errno = 0;
	return 0;
error:
	errno = -ret;
	return -1;
}

MODEM_CMD_DEFINE(on_cmd_casend)
{
	mdata.tx_space_avail = strtoul(argv[0], NULL, 10);
	LOG_DBG("Available tx space: %zu", mdata.tx_space_avail);
	return 0;
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
	struct modem_cmd cmd[] = { MODEM_CMD("+CASEND: ", on_cmd_casend, 1U, "") };

	/* Modem is not attached to the network. */
	if (sim7080_get_state() != SIM7080_STATE_NETWORKING) {
		LOG_ERR("Modem currently not attached to the network!");
		return -EINVAL;
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

	/* Query the available space in send buffer */
	ret = snprintk(send_buf, sizeof(send_buf), "AT+CASEND=%d", sock->id);
	if (ret < 0) {
		LOG_ERR("Failed to build send query command");
		errno = ENOMEM;
		return -1;
	}

	mdata.tx_space_avail = 0;
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, cmd, ARRAY_SIZE(cmd), send_buf,
		&mdata.sem_response, K_SECONDS(2));
	if (ret < 0) {
		LOG_ERR("Failed to query available tx size: %d", ret);
		errno = EIO;
		return -1;
	}

	if (mdata.tx_space_avail == 0) {
		LOG_WRN("No space left in tx buffer");
		errno = ENOMEM;
		return -1;
	}

	/* Only send up to available tx size bytes. */
	if (len > mdata.tx_space_avail) {
		len = mdata.tx_space_avail;
	}

	ret = snprintk(send_buf, sizeof(send_buf), "AT+CASEND=%d,%zu", sock->id, len);
	if (ret < 0) {
		LOG_ERR("Failed to build send command");
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
		LOG_ERR("Failed to send CASEND");
		goto exit;
	}

	/* Wait for '> ' */
	ret = k_sem_take(&mdata.sem_tx_ready, K_SECONDS(2));
	if (ret < 0) {
		LOG_ERR("Timeout while waiting for tx");
		goto exit;
	}

	/* Send data */
	modem_cmd_send_data_nolock(&mctx.iface, buf, len);

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
	if (sim7080_get_state() != SIM7080_STATE_NETWORKING) {
		LOG_ERR("Modem currently not attached to the network!");
		return -EINVAL;
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
	if (sim7080_get_state() != SIM7080_STATE_NETWORKING) {
		LOG_ERR("Modem currently not attached to the network!");
		return -EINVAL;
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
				return ret;
			}

			sent += ret;
			buf += ret;
			len -= ret;
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
	if (sim7080_get_state() != SIM7080_STATE_NETWORKING) {
		LOG_ERR("Modem currently not attached to the network!");
		return -EINVAL;
	}

	/* Make sure socket is allocated */
	if (modem_socket_is_allocated(&mdata.socket_config, sock) == false) {
		return 0;
	}

	socket_close(sock);

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
	if (sim7080_get_state() != SIM7080_STATE_NETWORKING) {
		LOG_ERR("Modem currently not attached to the network!");
		return -EINVAL;
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

const struct socket_op_vtable offload_socket_fd_op_vtable = {
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

void sim7080_handle_sock_data_indication(int fd)
{
	struct modem_socket *sock = modem_socket_from_fd(&mdata.socket_config, fd);

	if (!sock) {
		LOG_INF("No socket with fd %d", fd);
		return;
	}

	/* Modem does not tell packet size. Set dummy for receive. */
	modem_socket_packet_size_update(&mdata.socket_config, sock, 1);

	LOG_INF("Data available on socket: %d", fd);
	modem_socket_data_ready(&mdata.socket_config, sock);
}

void sim7080_handle_sock_state(int fd, uint8_t state)
{
	struct modem_socket *sock = modem_socket_from_fd(&mdata.socket_config, fd);

	if (!sock) {
		LOG_INF("No socket with fd %d", fd);
		return;
	}

	/* Only continue if socket was closed. */
	if (state != 0) {
		return;
	}

	LOG_INF("Socket close indication for socket: %d", fd);

	sock->is_connected = false;

	/* Unblock potentially waiting socket */
	modem_socket_packet_size_update(&mdata.socket_config, sock, 0);
	modem_socket_data_ready(&mdata.socket_config, sock);
}

int sim7080_offload_socket(int family, int type, int proto)
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

int mdm_sim7080_start_network(void)
{
	int ret = -EALREADY;

	if (sim7080_get_state() == SIM7080_STATE_NETWORKING) {
		LOG_WRN("Network already active");
		goto out;
	} else if (sim7080_get_state() != SIM7080_STATE_IDLE) {
		LOG_WRN("Can only activate networking from idle state");
		ret = -EINVAL;
		goto out;
	}

	ret = sim7080_pdp_activate();

out:
	return ret;
}

int mdm_sim7080_stop_network(void)
{
	int ret = -EINVAL;

	if (sim7080_get_state() != SIM7080_STATE_NETWORKING) {
		LOG_WRN("Modem not in networking state");
		goto out;
	}

	ret = sim7080_pdp_deactivate();

out:
	return ret;
}
