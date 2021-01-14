/*
 * Copyright (c) 2020 Analog Life LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT quectel_bg9x

#include <logging/log.h>
LOG_MODULE_REGISTER(modem_quectel_bg9x, CONFIG_MODEM_LOG_LEVEL);

#include "quectel-bg9x.h"

static struct k_thread	       modem_rx_thread;
static struct k_work_q	       modem_workq;
static struct modem_data       mdata;
static struct modem_context    mctx;
static const struct socket_op_vtable offload_socket_fd_op_vtable;

static K_KERNEL_STACK_DEFINE(modem_rx_stack, CONFIG_MODEM_QUECTEL_BG9X_RX_STACK_SIZE);
static K_KERNEL_STACK_DEFINE(modem_workq_stack, CONFIG_MODEM_QUECTEL_BG9X_RX_WORKQ_STACK_SIZE);
NET_BUF_POOL_DEFINE(mdm_recv_pool, MDM_RECV_MAX_BUF, MDM_RECV_BUF_SIZE, 0, NULL);

static inline int digits(int n)
{
	int count = 0;

	while (n != 0) {
		n /= 10;
		++count;
	}

	return count;
}

static inline uint32_t hash32(char *str, int len)
{
#define HASH_MULTIPLIER		37

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
	uint32_t hash_value;

	data->mac_addr[0] = 0x00;
	data->mac_addr[1] = 0x10;

	/* use IMEI for mac_addr */
	hash_value = hash32(mdata.mdm_imei, strlen(mdata.mdm_imei));

	UNALIGNED_PUT(hash_value, (uint32_t *)(data->mac_addr + 2));

	return data->mac_addr;
}

/* Func: modem_atoi
 * Desc: Convert string to long integer, but handle errors
 */
static int modem_atoi(const char *s, const int err_value,
		      const char *desc, const char *func)
{
	int   ret;
	char  *endptr;

	ret = (int)strtol(s, &endptr, 10);
	if (!endptr || *endptr != '\0') {
		LOG_ERR("bad %s '%s' in %s", log_strdup(s), log_strdup(desc),
			log_strdup(func));
		return err_value;
	}

	return ret;
}

static inline int find_len(char *data)
{
	char buf[10] = {0};
	int  i;

	for (i = 0; i < 10; i++) {
		if (data[i] == '\r')
			break;

		buf[i] = data[i];
	}

	return ATOI(buf, 0, "rx_buf");
}

/* Func: modem_at
 * Desc: Send "AT" command to the modem and wait for it to
 * respond. If the modem doesn't respond after some time, give
 * up and kill the driver.
 */
static int modem_at(struct modem_context *mctx, struct modem_data *mdata)
{
	int counter = 0, ret = -1;

	do {

		/* Send "AT" command to the modem. */
		ret = modem_cmd_send(&mctx->iface, &mctx->cmd_handler,
				     NULL, 0, "AT", &mdata->sem_response,
				     MDM_CMD_TIMEOUT);

		/* Check the response from the Modem. */
		if (ret < 0 && ret != -ETIMEDOUT) {
			return ret;
		}

		counter++;
		k_sleep(K_SECONDS(2));
	} while (counter < MDM_MAX_AT_RETRIES && ret < 0);

	return ret;
}

/* Func: on_cmd_sockread_common
 * Desc: Function to successfully read data from the modem on a given socket.
 */
static int on_cmd_sockread_common(int socket_fd,
				  struct modem_cmd_handler_data *data,
				  uint16_t len)
{
	struct modem_socket	 *sock = NULL;
	struct socket_read_data	 *sock_data;
	int ret, i;
	int socket_data_length;
	int bytes_to_skip;

	if (!len) {
		LOG_ERR("Invalid length, Aborting!");
		return -EAGAIN;
	}

	/* Make sure we still have buf data */
	if (!data->rx_buf) {
		LOG_ERR("Incorrect format! Ignoring data!");
		return -EINVAL;
	}

	socket_data_length = find_len(data->rx_buf->data);

	/* No (or not enough) data available on the socket. */
	bytes_to_skip = digits(socket_data_length) + 2 + 4;
	if (socket_data_length <= 0) {
		LOG_ERR("Length problem (%d).  Aborting!", socket_data_length);
		return -EAGAIN;
	}

	/* check to make sure we have all of the data. */
	if (net_buf_frags_len(data->rx_buf) < (socket_data_length + bytes_to_skip)) {
		LOG_DBG("Not enough data -- wait!");
		return -EAGAIN;
	}

	/* Skip "len" and CRLF */
	bytes_to_skip = digits(socket_data_length) + 2;
	for (i = 0; i < bytes_to_skip; i++) {
		net_buf_pull_u8(data->rx_buf);
	}

	if (!data->rx_buf->len) {
		data->rx_buf = net_buf_frag_del(NULL, data->rx_buf);
	}

	sock = modem_socket_from_fd(&mdata.socket_config, socket_fd);
	if (!sock) {
		LOG_ERR("Socket not found! (%d)", socket_fd);
		ret = -EINVAL;
		goto exit;
	}

	sock_data = (struct socket_read_data *)sock->data;
	if (!sock_data) {
		LOG_ERR("Socket data not found! Skip handling (%d)", socket_fd);
		ret = -EINVAL;
		goto exit;
	}

	ret = net_buf_linearize(sock_data->recv_buf, sock_data->recv_buf_len,
				data->rx_buf, 0, (uint16_t)socket_data_length);
	data->rx_buf = net_buf_skip(data->rx_buf, ret);
	sock_data->recv_read_len = ret;
	if (ret != socket_data_length) {
		LOG_ERR("Total copied data is different then received data!"
			" copied:%d vs. received:%d", ret, socket_data_length);
		ret = -EINVAL;
	}

exit:
	/* remove packet from list (ignore errors) */
	(void)modem_socket_packet_size_update(&mdata.socket_config, sock,
					      -socket_data_length);

	/* don't give back semaphore -- OK to follow */
	return ret;
}

/* Func: socket_close
 * Desc: Function to close the given socket descriptor.
 */
static void socket_close(struct modem_socket *sock)
{
	char buf[sizeof("AT+QICLOSE=##")] = {0};
	int  ret;

	snprintk(buf, sizeof(buf), "AT+QICLOSE=%d", sock->sock_fd);

	/* Tell the modem to close the socket. */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			     NULL, 0U, buf,
			     &mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
	}

	modem_socket_put(&mdata.socket_config, sock->sock_fd);
}

/* Handler: OK */
MODEM_CMD_DEFINE(on_cmd_ok)
{
	modem_cmd_handler_set_error(data, 0);
	k_sem_give(&mdata.sem_response);
	return 0;
}

/* Handler: ERROR */
MODEM_CMD_DEFINE(on_cmd_error)
{
	modem_cmd_handler_set_error(data, -EIO);
	k_sem_give(&mdata.sem_response);
	return 0;
}

/* Handler: +CME Error: <err>[0] */
MODEM_CMD_DEFINE(on_cmd_exterror)
{
	modem_cmd_handler_set_error(data, -EIO);
	k_sem_give(&mdata.sem_response);
	return 0;
}

/* Handler: +CSQ: <signal_power>[0], <qual>[1] */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_rssi_csq)
{
	int rssi = ATOI(argv[0], 0, "signal_power");

	/* Check the RSSI value. */
	if (rssi == 31) {
		mctx.data_rssi = -51;
	} else if (rssi >= 0 && rssi <= 31) {
		mctx.data_rssi = -114 + ((rssi * 2) + 1);
	} else {
		mctx.data_rssi = -1000;
	}

	LOG_INF("RSSI: %d", mctx.data_rssi);
	return 0;
}

/* Handler: +QIOPEN: <connect_id>[0], <err>[1] */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_sockopen)
{
	int err = ATOI(argv[1], 0, "sock_err");

	LOG_INF("AT+QIOPEN: %d", err);
	modem_cmd_handler_set_error(data, err);
	k_sem_give(&mdata.sem_sock_conn);

	return 0;
}

/* Handler: <manufacturer> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_manufacturer)
{
	size_t out_len = net_buf_linearize(mdata.mdm_manufacturer,
					   sizeof(mdata.mdm_manufacturer) - 1,
					   data->rx_buf, 0, len);
	mdata.mdm_manufacturer[out_len] = '\0';
	LOG_INF("Manufacturer: %s", log_strdup(mdata.mdm_manufacturer));
	return 0;
}

/* Handler: <model> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_model)
{
	size_t out_len = net_buf_linearize(mdata.mdm_model,
					   sizeof(mdata.mdm_model) - 1,
					   data->rx_buf, 0, len);
	mdata.mdm_model[out_len] = '\0';

	/* Log the received information. */
	LOG_INF("Model: %s", log_strdup(mdata.mdm_model));
	return 0;
}

/* Handler: <rev> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_revision)
{
	size_t out_len = net_buf_linearize(mdata.mdm_revision,
					   sizeof(mdata.mdm_revision) - 1,
					   data->rx_buf, 0, len);
	mdata.mdm_revision[out_len] = '\0';

	/* Log the received information. */
	LOG_INF("Revision: %s", log_strdup(mdata.mdm_revision));
	return 0;
}

/* Handler: <IMEI> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_imei)
{
	size_t out_len = net_buf_linearize(mdata.mdm_imei,
					   sizeof(mdata.mdm_imei) - 1,
					   data->rx_buf, 0, len);
	mdata.mdm_imei[out_len] = '\0';

	/* Log the received information. */
	LOG_INF("IMEI: %s", log_strdup(mdata.mdm_imei));
	return 0;
}

#if defined(CONFIG_MODEM_SIM_NUMBERS)
/* Handler: <IMSI> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_imsi)
{
	size_t	out_len = net_buf_linearize(mdata.mdm_imsi,
					    sizeof(mdata.mdm_imsi) - 1,
					    data->rx_buf, 0, len);
	mdata.mdm_imsi[out_len] = '\0';

	/* Log the received information. */
	LOG_INF("IMSI: %s", log_strdup(mdata.mdm_imsi));
	return 0;
}

/* Handler: <ICCID> */
MODEM_CMD_DEFINE(on_cmd_atcmdinfo_iccid)
{
	size_t out_len;
	char   *p;

	out_len = net_buf_linearize(mdata.mdm_iccid, sizeof(mdata.mdm_iccid) - 1,
				    data->rx_buf, 0, len);
	mdata.mdm_iccid[out_len] = '\0';

	/* Skip over the +CCID bit, which modems omit. */
	if (mdata.mdm_iccid[0] == '+') {
		p = strchr(mdata.mdm_iccid, ' ');
		if (p) {
			out_len = strlen(p + 1);
			memmove(mdata.mdm_iccid, p + 1, len + 1);
		}
	}

	LOG_INF("ICCID: %s", log_strdup(mdata.mdm_iccid));
	return 0;
}
#endif /* #if defined(CONFIG_MODEM_SIM_NUMBERS) */

/* Handler: TX Ready */
MODEM_CMD_DIRECT_DEFINE(on_cmd_tx_ready)
{
	k_sem_give(&mdata.sem_tx_ready);
	return len;
}

/* Handler: SEND OK */
MODEM_CMD_DEFINE(on_cmd_send_ok)
{
	modem_cmd_handler_set_error(data, 0);
	k_sem_give(&mdata.sem_response);

	return 0;
}

/* Handler: SEND FAIL */
MODEM_CMD_DEFINE(on_cmd_send_fail)
{
	mdata.sock_written = 0;
	modem_cmd_handler_set_error(data, -EIO);
	k_sem_give(&mdata.sem_response);

	return 0;
}

/* Handler: Read data */
MODEM_CMD_DEFINE(on_cmd_sock_readdata)
{
	return on_cmd_sockread_common(mdata.sock_fd, data, len);
}

/* Handler: Data receive indication. */
MODEM_CMD_DEFINE(on_cmd_unsol_recv)
{
	struct modem_socket *sock;
	int		     sock_fd;

	sock_fd = ATOI(argv[0], 0, "sock_fd");

	/* Socket pointer from FD. */
	sock = modem_socket_from_fd(&mdata.socket_config, sock_fd);
	if (!sock) {
		return 0;
	}

	/* Data ready indication. */
	LOG_INF("Data Receive Indication for socket: %d", sock_fd);
	modem_socket_data_ready(&mdata.socket_config, sock);

	return 0;
}

/* Handler: Socket Close Indication. */
MODEM_CMD_DEFINE(on_cmd_unsol_close)
{
	struct modem_socket *sock;
	int		     sock_fd;

	sock_fd = ATOI(argv[0], 0, "sock_fd");
	sock	= modem_socket_from_fd(&mdata.socket_config, sock_fd);
	if (!sock) {
		return 0;
	}

	LOG_INF("Socket Close Indication for socket: %d", sock_fd);

	/* Tell the modem to close the socket. */
	socket_close(sock);
	LOG_INF("Socket Closed: %d", sock_fd);
	return 0;
}

/* Func: send_socket_data
 * Desc: This function will send "binary" data over the socket object.
 */
static ssize_t send_socket_data(struct modem_socket *sock,
				const struct sockaddr *dst_addr,
				struct modem_cmd *handler_cmds,
				size_t handler_cmds_len,
				const char *buf, size_t buf_len,
				k_timeout_t timeout)
{
	int  ret;
	char send_buf[sizeof("AT+QISEND=##,####")] = {0};
	char ctrlz = 0x1A;

	if (buf_len > MDM_MAX_DATA_LENGTH) {
		buf_len = MDM_MAX_DATA_LENGTH;
	}

	/* Create a buffer with the correct params. */
	mdata.sock_written = buf_len;
	snprintk(send_buf, sizeof(send_buf), "AT+QISEND=%d,%ld", sock->sock_fd, (long) buf_len);

	/* Setup the locks correctly. */
	k_sem_take(&mdata.cmd_handler_data.sem_tx_lock, K_FOREVER);
	k_sem_reset(&mdata.sem_tx_ready);

	/* Send the Modem command. */
	ret = modem_cmd_send_nolock(&mctx.iface, &mctx.cmd_handler,
				    NULL, 0U, send_buf, NULL, K_NO_WAIT);
	if (ret < 0) {
		goto exit;
	}

	/* set command handlers */
	ret = modem_cmd_handler_update_cmds(&mdata.cmd_handler_data,
					    handler_cmds, handler_cmds_len,
					    true);
	if (ret < 0) {
		goto exit;
	}

	/* Wait for '>' */
	ret = k_sem_take(&mdata.sem_tx_ready, K_MSEC(5000));
	if (ret < 0) {
		/* Didn't get the data prompt - Exit. */
		LOG_DBG("Timeout waiting for tx");
		goto exit;
	}

	/* Write all data on the console and send CTRL+Z. */
	mctx.iface.write(&mctx.iface, buf, buf_len);
	mctx.iface.write(&mctx.iface, &ctrlz, 1);

	/* Wait for 'SEND OK' or 'SEND FAIL' */
	k_sem_reset(&mdata.sem_response);
	ret = k_sem_take(&mdata.sem_response, timeout);
	if (ret < 0) {
		LOG_DBG("No send response");
		goto exit;
	}

	ret = modem_cmd_handler_get_error(&mdata.cmd_handler_data);
	if (ret != 0) {
		LOG_DBG("Failed to send data");
	}

exit:
	/* unset handler commands and ignore any errors */
	(void)modem_cmd_handler_update_cmds(&mdata.cmd_handler_data,
					    NULL, 0U, false);
	k_sem_give(&mdata.cmd_handler_data.sem_tx_lock);

	if (ret < 0) {
		return ret;
	}

	/* Return the amount of data written on the socket. */
	return mdata.sock_written;
}

/* Func: offload_sendto
 * Desc: This function will send data on the socket object.
 */
static ssize_t offload_sendto(void *obj, const void *buf, size_t len,
			      int flags, const struct sockaddr *to,
			      socklen_t tolen)
{
	int ret;
	struct modem_socket *sock = (struct modem_socket *) obj;

	/* Here's how sending data works,
	 * -> We firstly send the "AT+QISEND" command on the given socket and
	 *    specify the length of data to be transferred.
	 * -> In response to "AT+QISEND" command, the modem may respond with a
	 *    data prompt (>) or not respond at all. If it doesn't respond, we
	 *    exit. If it does respond with a data prompt (>), we move forward.
	 * -> We plainly write all data on the UART and terminate by sending a
	 *    CTRL+Z. Once the modem receives CTRL+Z, it starts processing the
	 *    data and will respond with either "SEND OK", "SEND FAIL" or "ERROR".
	 *    Here we are registering handlers for the first two responses. We
	 *    already have a handler for the "generic" error response.
	 */
	struct modem_cmd cmd[] = {
		MODEM_CMD_DIRECT(">", on_cmd_tx_ready),
		MODEM_CMD("SEND OK", on_cmd_send_ok,   0, ","),
		MODEM_CMD("SEND FAIL", on_cmd_send_fail, 0, ","),
	};

	/* Ensure that valid parameters are passed. */
	if (!buf || len == 0) {
		errno = EINVAL;
		return -1;
	}

	/* UDP is not supported. */
	if (sock->ip_proto == IPPROTO_UDP) {
		errno = ENOTSUP;
		return -1;
	}

	if (!sock->is_connected) {
		errno = ENOTCONN;
		return -1;
	}

	ret = send_socket_data(sock, to, cmd, ARRAY_SIZE(cmd), buf, len,
			       MDM_CMD_TIMEOUT);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	/* Data was written successfully. */
	errno = 0;
	return ret;
}

/* Func: offload_recvfrom
 * Desc: This function will receive data on the socket object.
 */
static ssize_t offload_recvfrom(void *obj, void *buf, size_t len,
				int flags, struct sockaddr *from,
				socklen_t *fromlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	char   sendbuf[sizeof("AT+QIRD=##,####")] = {0};
	int    ret;
	struct socket_read_data sock_data;

	/* Modem command to read the data. */
	struct modem_cmd data_cmd[] = { MODEM_CMD("+QIRD: ", on_cmd_sock_readdata, 0U, "") };

	if (!buf || len == 0) {
		errno = EINVAL;
		return -1;
	}

	if (flags & ZSOCK_MSG_PEEK) {
		errno = ENOTSUP;
		return -1;
	}

	snprintk(sendbuf, sizeof(sendbuf), "AT+QIRD=%d,%d", sock->sock_fd, len);

	/* Socket read settings */
	(void) memset(&sock_data, 0, sizeof(sock_data));
	sock_data.recv_buf     = buf;
	sock_data.recv_buf_len = len;
	sock_data.recv_addr    = from;
	sock->data	       = &sock_data;
	mdata.sock_fd	       = sock->sock_fd;

	/* Tell the modem to give us data (AT+QIRD=sock_fd,data_len). */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			     data_cmd, ARRAY_SIZE(data_cmd), sendbuf, &mdata.sem_response,
			     MDM_CMD_TIMEOUT);
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

/* Func: offload_read
 * Desc: This function reads data from the given socket object.
 */
static ssize_t offload_read(void *obj, void *buffer, size_t count)
{
	return offload_recvfrom(obj, buffer, count, 0, NULL, 0);
}

/* Func: offload_write
 * Desc: This function writes data to the given socket object.
 */
static ssize_t offload_write(void *obj, const void *buffer, size_t count)
{
	return offload_sendto(obj, buffer, count, 0, NULL, 0);
}

/* Func: offload_poll
 * Desc: This function polls on a given socket object.
 */
static int offload_poll(struct zsock_pollfd *fds, int nfds, int msecs)
{
	int i;
	void *obj;

	/* Only accept modem sockets. */
	for (i = 0; i < nfds; i++) {
		if (fds[i].fd < 0) {
			continue;
		}

		/* If vtable matches, then it's modem socket. */
		obj = z_get_fd_obj(fds[i].fd,
				   (const struct fd_op_vtable *) &offload_socket_fd_op_vtable,
				   EINVAL);
		if (obj == NULL) {
			return -1;
		}
	}

	return modem_socket_poll(&mdata.socket_config, fds, nfds, msecs);
}

/* Func: offload_ioctl
 * Desc: Function call to handle various misc requests.
 */
static int offload_ioctl(void *obj, unsigned int request, va_list args)
{
	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE:
		return -EXDEV;

	case ZFD_IOCTL_POLL_UPDATE:
		return -EOPNOTSUPP;

	case ZFD_IOCTL_POLL_OFFLOAD:
	{
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

/* Func: offload_connect
 * Desc: This function will connect with a provided TCP.
 */
static int offload_connect(void *obj, const struct sockaddr *addr,
						   socklen_t addrlen)
{
	struct modem_socket *sock     = (struct modem_socket *) obj;
	uint16_t	    dst_port  = 0;
	char		    *protocol = "TCP";
	struct modem_cmd    cmd[]     = { MODEM_CMD("+QIOPEN: ", on_cmd_atcmdinfo_sockopen, 2U, ",") };
	char		    buf[sizeof("AT+QIOPEN=#,##,###,####.####.####.####,######")] = {0};
	int		    ret;

	if (sock->id < mdata.socket_config.base_socket_num - 1) {
		LOG_ERR("Invalid socket_id(%d) from fd:%d",
			sock->id, sock->sock_fd);
		errno = EINVAL;
		return -1;
	}

	if (sock->is_connected == true) {
		LOG_ERR("Socket is already connected!! socket_id(%d), socket_fd:%d",
			sock->id, sock->sock_fd);
		errno = EISCONN;
		return -1;
	}

	/* Find the correct destination port. */
	if (addr->sa_family == AF_INET6) {
		dst_port = ntohs(net_sin6(addr)->sin6_port);
	} else if (addr->sa_family == AF_INET) {
		dst_port = ntohs(net_sin(addr)->sin_port);
	}

	/* UDP is not supported. */
	if (sock->ip_proto == IPPROTO_UDP) {
		errno = ENOTSUP;
		return -1;
	}

	k_sem_reset(&mdata.sem_sock_conn);

	/* Formulate the complete string. */
	snprintk(buf, sizeof(buf), "AT+QIOPEN=%d,%d,\"%s\",\"%s\",%d,0,0", 1, sock->sock_fd, protocol,
		 modem_context_sprint_ip_addr(addr), dst_port);

	/* Send out the command. */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			     NULL, 0U, buf,
			     &mdata.sem_response, K_SECONDS(1));
	if (ret < 0) {
		LOG_ERR("%s ret:%d", log_strdup(buf), ret);
		LOG_ERR("Closing the socket!!!");
		socket_close(sock);
		errno = -ret;
		return -1;
	}

	/* set command handlers */
	ret = modem_cmd_handler_update_cmds(&mdata.cmd_handler_data,
					    cmd, ARRAY_SIZE(cmd), true);
	if (ret < 0) {
		goto exit;
	}

	/* Wait for QI+OPEN */
	ret = k_sem_take(&mdata.sem_sock_conn, MDM_CMD_CONN_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Timeout waiting for socket open");
		LOG_ERR("Closing the socket!!!");
		socket_close(sock);
		goto exit;
	}

	ret = modem_cmd_handler_get_error(&mdata.cmd_handler_data);
	if (ret != 0) {
		LOG_ERR("Closing the socket!!!");
		socket_close(sock);
		goto exit;
	}

	/* Connected successfully. */
	sock->is_connected = true;
	errno = 0;
	return 0;

exit:
	(void) modem_cmd_handler_update_cmds(&mdata.cmd_handler_data,
					     NULL, 0U, false);
	errno = -ret;
	return -1;
}

/* Func: offload_close
 * Desc: This function closes the connection with the remote client and
 * frees the socket.
 */
static int offload_close(void *obj)
{
	struct modem_socket *sock = (struct modem_socket *) obj;

	/* Make sure we assigned an id */
	if (sock->id < mdata.socket_config.base_socket_num) {
		return 0;
	}

	/* Close the socket only if it is connected. */
	if (sock->is_connected) {
		socket_close(sock);
	}

	return 0;
}

/* Func: offload_sendmsg
 * Desc: This function sends messages to the modem.
 */
static ssize_t offload_sendmsg(void *obj, const struct msghdr *msg, int flags)
{
	ssize_t sent = 0;
	int rc;

	LOG_DBG("msg_iovlen:%d flags:%d", msg->msg_iovlen, flags);

	for (int i = 0; i < msg->msg_iovlen; i++) {
		const char *buf = msg->msg_iov[i].iov_base;
		size_t len	= msg->msg_iov[i].iov_len;

		while (len > 0) {
			rc = offload_sendto(obj, buf, len, flags,
					    msg->msg_name, msg->msg_namelen);
			if (rc < 0) {
				if (rc == -EAGAIN) {
					k_sleep(MDM_SENDMSG_SLEEP);
				} else {
					sent = rc;
					break;
				}
			} else {
				sent += rc;
				buf += rc;
				len -= rc;
			}
		}
	}

	return (ssize_t) sent;
}

/* Func: modem_rx
 * Desc: Thread to process all messages received from the Modem.
 */
static void modem_rx(void)
{
	while (true) {

		/* Wait for incoming data */
		k_sem_take(&mdata.iface_data.rx_sem, K_FOREVER);

		mctx.cmd_handler.process(&mctx.cmd_handler, &mctx.iface);
	}
}

/* Func: modem_rssi_query_work
 * Desc: Routine to get Modem RSSI.
 */
static void modem_rssi_query_work(struct k_work *work)
{
	struct modem_cmd cmd  = MODEM_CMD("+CSQ: ", on_cmd_atcmdinfo_rssi_csq, 2U, ",");
	static char *send_cmd = "AT+CSQ";
	int ret;

	/* query modem RSSI */
	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			     &cmd, 1U, send_cmd, &mdata.sem_response,
			     MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("AT+CSQ ret:%d", ret);
	}

	/* Re-start RSSI query work */
	if (work) {
		k_delayed_work_submit_to_queue(&modem_workq,
					       &mdata.rssi_query_work,
					       K_SECONDS(RSSI_TIMEOUT_SECS));
	}
}

/* Func: pin_init
 * Desc: Boot up the Modem.
 */
static void pin_init(void)
{
	LOG_INF("Setting Modem Pins");

#if DT_INST_NODE_HAS_PROP(0, mdm_wdisable_gpios)
	LOG_INF("Deactivate W Disable");
	modem_pin_write(&mctx, MDM_WDISABLE, 0);
	k_sleep(K_MSEC(250));
#endif

	/* NOTE: Per the BG95 document, the Reset pin is internally connected to the
	 * Power key pin.
	 */

	/* MDM_POWER -> 1 for 500-1000 msec. */
	modem_pin_write(&mctx, MDM_POWER, 1);
	k_sleep(K_MSEC(750));

	/* MDM_POWER -> 0 and wait for ~2secs as UART remains in "inactive" state
	 * for some time after the power signal is enabled.
	 */
	modem_pin_write(&mctx, MDM_POWER, 0);
	k_sleep(K_SECONDS(2));

	LOG_INF("... Done!");
}

static const struct modem_cmd response_cmds[] = {
	MODEM_CMD("OK", on_cmd_ok, 0U, ""),
	MODEM_CMD("ERROR", on_cmd_error, 0U, ""),
	MODEM_CMD("+CME ERROR: ", on_cmd_exterror, 1U, ""),
};

static const struct modem_cmd unsol_cmds[] = {
	MODEM_CMD("+QIURC: \"recv\",",	   on_cmd_unsol_recv,  1U, ""),
	MODEM_CMD("+QIURC: \"closed\",",   on_cmd_unsol_close, 1U, ""),
};

/* Commands sent to the modem to set it up at boot time. */
static const struct setup_cmd setup_cmds[] = {
	SETUP_CMD_NOHANDLE("ATE0"),
	SETUP_CMD_NOHANDLE("ATH"),
	SETUP_CMD_NOHANDLE("AT+CMEE=1"),

	/* Commands to read info from the modem (things like IMEI, Model etc). */
	SETUP_CMD("AT+CGMI", "", on_cmd_atcmdinfo_manufacturer, 0U, ""),
	SETUP_CMD("AT+CGMM", "", on_cmd_atcmdinfo_model, 0U, ""),
	SETUP_CMD("AT+CGMR", "", on_cmd_atcmdinfo_revision, 0U, ""),
	SETUP_CMD("AT+CGSN", "", on_cmd_atcmdinfo_imei, 0U, ""),
#if defined(CONFIG_MODEM_SIM_NUMBERS)
	SETUP_CMD("AT+CIMI", "", on_cmd_atcmdinfo_imsi, 0U, ""),
	SETUP_CMD("AT+QCCID", "", on_cmd_atcmdinfo_iccid, 0U, ""),
#endif /* #if defined(CONFIG_MODEM_SIM_NUMBERS) */
	SETUP_CMD_NOHANDLE("AT+QICSGP=1,1,\"" MDM_APN "\",\"" MDM_USERNAME "\", \"" MDM_PASSWORD "\",1"),
};

/* Func: modem_pdp_context_active
 * Desc: This helper function is called from modem_setup, and is
 * used to open the PDP context. If there is trouble activating the
 * PDP context, we try to deactive and reactive MDM_PDP_ACT_RETRY_COUNT times.
 * If it fails, we return an error.
 */
static int modem_pdp_context_activate(void)
{
	int ret;
	int retry_count = 0;

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			     NULL, 0U, "AT+QIACT=1", &mdata.sem_response,
			     MDM_CMD_TIMEOUT);

	/* If there is trouble activating the PDP context, we try to deactivate/reactive it. */
	while (ret == -EIO && retry_count < MDM_PDP_ACT_RETRY_COUNT) {
		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			     NULL, 0U, "AT+QIDEACT=1", &mdata.sem_response,
			     MDM_CMD_TIMEOUT);

		/* If there's any error for AT+QIDEACT, restart the module. */
		if (ret != 0) {
			return ret;
		}

		ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler,
			     NULL, 0U, "AT+QIACT=1", &mdata.sem_response,
			     MDM_CMD_TIMEOUT);

		retry_count++;
	}

	if (ret == -EIO && retry_count >= MDM_PDP_ACT_RETRY_COUNT) {
		LOG_ERR("Retried activating/deactivating too many times.");
	}

	return ret;
}

/* Func: modem_setup
 * Desc: This function is used to setup the modem from zero. The idea
 * is that this function will be called right after the modem is
 * powered on to do the stuff necessary to talk to the modem.
 */
static int modem_setup(void)
{
	int ret = 0, counter;
	int rssi_retry_count = 0, init_retry_count = 0;

	/* Setup the pins to ensure that Modem is enabled. */
	pin_init();

restart:

	counter = 0;

	/* stop RSSI delay work */
	k_delayed_work_cancel(&mdata.rssi_query_work);

	/* Let the modem respond. */
	LOG_INF("Waiting for modem to respond");
	ret = modem_at(&mctx, &mdata);
	if (ret < 0) {
		LOG_ERR("MODEM WAIT LOOP ERROR: %d", ret);
		goto error;
	}

	/* Run setup commands on the modem. */
	ret = modem_cmd_handler_setup_cmds(&mctx.iface, &mctx.cmd_handler,
					   setup_cmds, ARRAY_SIZE(setup_cmds),
					   &mdata.sem_response, MDM_REGISTRATION_TIMEOUT);
	if (ret < 0) {
		goto error;
	}

restart_rssi:

	/* query modem RSSI */
	modem_rssi_query_work(NULL);
	k_sleep(MDM_WAIT_FOR_RSSI_DELAY);

	/* Keep trying to read RSSI until we get a valid value - Eventually, exit. */
	while (counter++ < MDM_WAIT_FOR_RSSI_COUNT &&
	      (mctx.data_rssi >= 0 || mctx.data_rssi <= -1000)) {
		modem_rssi_query_work(NULL);
		k_sleep(MDM_WAIT_FOR_RSSI_DELAY);
	}

	/* Is the RSSI invalid ? */
	if (mctx.data_rssi >= 0 || mctx.data_rssi <= -1000) {
		rssi_retry_count++;

		if (rssi_retry_count >= MDM_NETWORK_RETRY_COUNT) {
			LOG_ERR("Failed network init. Too many attempts!");
			ret = -ENETUNREACH;
			goto error;
		}

		/* Try again! */
		LOG_ERR("Failed network init. Restarting process.");
		counter = 0;
		goto restart_rssi;
	}

	/* Network is ready - Start RSSI work in the background. */
	LOG_INF("Network is ready.");
	k_delayed_work_submit_to_queue(&modem_workq,
				       &mdata.rssi_query_work,
				       K_SECONDS(RSSI_TIMEOUT_SECS));

	/* Once the network is ready, we try to activate the PDP context. */
	ret = modem_pdp_context_activate();
	if (ret < 0 && init_retry_count++ < MDM_INIT_RETRY_COUNT) {
		LOG_ERR("Error activating modem with pdp context");
		goto restart;
	}

error:
	return ret;
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

/* Setup the Modem NET Interface. */
static void modem_net_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct modem_data *data	 = dev->data;

	/* Direct socket offload used instead of net offload: */
	net_if_set_link_addr(iface, modem_get_mac(dev),
			     sizeof(data->mac_addr),
			     NET_LINK_ETHERNET);
	data->net_iface = iface;
}

static struct net_if_api api_funcs = {
	.init = modem_net_iface_init,
};

static bool offload_is_supported(int family, int type, int proto)
{
	return true;
}

static int offload_socket(int family, int type, int proto)
{
	int ret;

	/* defer modem's socket create call to bind() */
	ret = modem_socket_get(&mdata.socket_config, family, type, proto);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	errno = 0;
	return ret;
}

static int modem_init(const struct device *dev)
{
	int ret; ARG_UNUSED(dev);

	k_sem_init(&mdata.sem_response,	 0, 1);
	k_sem_init(&mdata.sem_tx_ready,	 0, 1);
	k_sem_init(&mdata.sem_sock_conn, 0, 1);
	k_work_q_start(&modem_workq, modem_workq_stack,
		       K_KERNEL_STACK_SIZEOF(modem_workq_stack),
		       K_PRIO_COOP(7));

	/* socket config */
	mdata.socket_config.sockets	    = &mdata.sockets[0];
	mdata.socket_config.sockets_len	    = ARRAY_SIZE(mdata.sockets);
	mdata.socket_config.base_socket_num = MDM_BASE_SOCKET_NUM;
	ret = modem_socket_init(&mdata.socket_config, &offload_socket_fd_op_vtable);
	if (ret < 0) {
		goto error;
	}

	/* cmd handler */
	mdata.cmd_handler_data.cmds[CMD_RESP]	   = response_cmds;
	mdata.cmd_handler_data.cmds_len[CMD_RESP]  = ARRAY_SIZE(response_cmds);
	mdata.cmd_handler_data.cmds[CMD_UNSOL]	   = unsol_cmds;
	mdata.cmd_handler_data.cmds_len[CMD_UNSOL] = ARRAY_SIZE(unsol_cmds);
	mdata.cmd_handler_data.match_buf	   = &mdata.cmd_match_buf[0];
	mdata.cmd_handler_data.match_buf_len	   = sizeof(mdata.cmd_match_buf);
	mdata.cmd_handler_data.buf_pool		   = &mdm_recv_pool;
	mdata.cmd_handler_data.alloc_timeout	   = BUF_ALLOC_TIMEOUT;
	mdata.cmd_handler_data.eol		   = "\r\n";
	ret = modem_cmd_handler_init(&mctx.cmd_handler, &mdata.cmd_handler_data);
	if (ret < 0) {
		goto error;
	}

	/* modem interface */
	mdata.iface_data.rx_rb_buf     = &mdata.iface_rb_buf[0];
	mdata.iface_data.rx_rb_buf_len = sizeof(mdata.iface_rb_buf);
	ret = modem_iface_uart_init(&mctx.iface, &mdata.iface_data,
				    MDM_UART_DEV_NAME);
	if (ret < 0) {
		goto error;
	}

	/* modem data storage */
	mctx.data_manufacturer = mdata.mdm_manufacturer;
	mctx.data_model	       = mdata.mdm_model;
	mctx.data_revision     = mdata.mdm_revision;
	mctx.data_imei	       = mdata.mdm_imei;
#if defined(CONFIG_MODEM_SIM_NUMBERS)
	mctx.data_imsi	       = mdata.mdm_imsi;
	mctx.data_iccid	       = mdata.mdm_iccid;
#endif /* #if defined(CONFIG_MODEM_SIM_NUMBERS) */

	/* pin setup */
	mctx.pins	       = modem_pins;
	mctx.pins_len	       = ARRAY_SIZE(modem_pins);
	mctx.driver_data       = &mdata;

	ret = modem_context_register(&mctx);
	if (ret < 0) {
		LOG_ERR("Error registering modem context: %d", ret);
		goto error;
	}

	/* start RX thread */
	k_thread_create(&modem_rx_thread, modem_rx_stack,
			K_KERNEL_STACK_SIZEOF(modem_rx_stack),
			(k_thread_entry_t) modem_rx,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	/* Init RSSI query */
	k_delayed_work_init(&mdata.rssi_query_work, modem_rssi_query_work);
	return modem_setup();

error:
	return ret;
}

/* Register the device with the Networking stack. */
NET_DEVICE_DT_INST_OFFLOAD_DEFINE(0, modem_init, device_pm_control_nop,
				  &mdata, NULL,
				  CONFIG_MODEM_QUECTEL_BG9X_INIT_PRIORITY,
				  &api_funcs, MDM_MAX_DATA_LENGTH);

/* Register NET sockets. */
NET_SOCKET_REGISTER(quectel_bg9x, AF_UNSPEC, offload_is_supported, offload_socket);
