/*
 * Copyright (c) 2020-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ftp_client.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>

#include "ftp_commands.h"

LOG_MODULE_REGISTER(ftp_client, CONFIG_FTP_CLIENT_LOG_LEVEL);

#define INVALID_SOCKET -1
#define FTP_CLIENT_POLL_TIMEOUT_MSEC (MSEC_PER_SEC * CONFIG_FTP_CLIENT_LISTEN_TIME)
#define FTP_CODE_ANY 0

/* FTP parameter length limits */
#define FTP_MAX_USERNAME 64  /* Unix/Linux username typical limit */
#define FTP_MAX_PASSWORD 255 /* Allow longer passwords */
#define FTP_MAX_FILENAME 255 /* Filename length limit */
#define FTP_MAX_PATHNAME 255 /* Standard NAME_MAX */
#define FTP_MAX_OPTIONS  32  /* FTP LIST options are typically short */

#if CONFIG_FTP_CLIENT_KEEPALIVE_TIME > 0
#define FTP_STACK_SIZE KB(2)
#define FTP_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO
static K_THREAD_STACK_DEFINE(ftp_stack_area, FTP_STACK_SIZE);

static struct k_work_q ftp_work_q;
#endif

enum ftp_channel_type {
	FTP_CHANNEL_CTRL,
	FTP_CHANNEL_DATA,
};

#if CONFIG_FTP_CLIENT_KEEPALIVE_TIME > 0
static void keepalive_timer_reset(struct ftp_client *client)
{
	k_work_reschedule_for_queue(&ftp_work_q, &client->keepalive_work,
				    K_SECONDS(CONFIG_FTP_CLIENT_KEEPALIVE_TIME));
}

static void keepalive_timer_cancel(struct ftp_client *client)
{
	struct k_work_sync sync;

	k_work_cancel_delayable_sync(&client->keepalive_work, &sync);
}

static void keepalive_handler(struct k_work *work)
{
	struct k_work_delayable *delayable = k_work_delayable_from_work(work);
	struct ftp_client *client = CONTAINER_OF(delayable, struct ftp_client, keepalive_work);

	if (client->ctrl_sock == INVALID_SOCKET) {
		return;
	}

	(void)ftp_keepalive(client);

	keepalive_timer_reset(client);
}
#else
static void keepalive_timer_reset(struct ftp_client *client) { }
static void keepalive_timer_cancel(struct ftp_client *client) { }
#endif

static int new_ftp_connection(struct ftp_client *client, enum ftp_channel_type channel,
			      uint16_t port)
{
	int proto = client->sec_tag <= 0 ? NET_IPPROTO_TCP : NET_IPPROTO_TLS_1_2;
	socklen_t addrlen;
	int *sock;
	int ret;

	if (channel == FTP_CHANNEL_CTRL) {
		sock = &client->ctrl_sock;
	} else if (channel == FTP_CHANNEL_DATA) {
		sock = &client->data_sock;
	} else {
		return -EINVAL;
	}

	*sock = zsock_socket(client->remote.sa_family, NET_SOCK_STREAM, proto);
	if (*sock < 0) {
		ret = -errno;
		LOG_ERR("socket(data) failed: %d", ret);
		return ret;
	}
	if (client->sec_tag != SEC_TAG_TLS_INVALID) {
		sec_tag_t sec_tag_list[] = { client->sec_tag };

		ret = zsock_setsockopt(*sock, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST,
				       sec_tag_list, sizeof(sec_tag_t));
		if (ret < 0) {
			ret = -errno;
			LOG_ERR("set tag list failed: %d", ret);
			zsock_close(*sock);
			*sock = INVALID_SOCKET;
			return ret;
		}
	}

	/* Connect to remote host */
	if (client->remote.sa_family == NET_AF_INET) {
		net_sin(&client->remote)->sin_port = net_htons(port);
		addrlen = sizeof(struct net_sockaddr_in);
	} else {
		net_sin6(&client->remote)->sin6_port = net_htons(port);
		addrlen = sizeof(struct net_sockaddr_in6);
	}

	ret = zsock_connect(*sock, &client->remote, addrlen);
	if (ret < 0) {
		ret = -errno;
		LOG_ERR("connect(data) failed: %d", ret);
		zsock_close(*sock);
		*sock = INVALID_SOCKET;
		return ret;
	}

	return ret;
}

static int validate_ftp_param(const char *param, size_t max_len)
{
	if (param == NULL) {
		return -EINVAL;
	}

	if (strlen(param) > max_len) {
		return -EINVAL;
	}

	/* Check for FTP command injection - CR/LF can inject additional commands */
	if (strchr(param, '\r') || strchr(param, '\n')) {
		return -EINVAL;
	}

	return 0;
}

static int parse_pasv_msg(const char *pasv_msg, uint16_t *data_port)
{
	char *endptr;
	long port_byte;
	char *tmp1, *tmp2;

	/* Parse Server port from passive message
	 * e.g. "227 Entering Passive Mode (90,130,70,73,86,111)" in case of IPv4
	 * e.g. "227 Entering Passive Mode (0,0,0,0,97,78)" in case of IPv6
	 * NOTE assume no IP address change from the Control channel
	 */
	tmp1 = strrchr(pasv_msg, ')');
	if (tmp1 == NULL) {
		return -EINVAL;
	}
	tmp2 = strrchr(pasv_msg, ',');
	if (tmp2 == NULL) {
		return -EINVAL;
	}
	/* Validate that ')' appears after the last ',' */
	if (tmp1 <= tmp2) {
		return -EINVAL;
	}

	/* Extract low byte of port (p2) */
	port_byte = strtol(tmp2 + 1, &endptr, 10);
	if (endptr != tmp1 || port_byte < 0 || port_byte > 255) {
		return -EINVAL;
	}
	*data_port = (uint16_t)port_byte;

	/* Find previous comma for high byte */
	tmp1 = tmp2 - 1;
	while (tmp1 > pasv_msg && isdigit((int)(*tmp1))) {
		tmp1--;
	}
	if (tmp1 <= pasv_msg || *tmp1 != ',') {
		return -EINVAL;
	}

	/* Extract high byte of port (p1) */
	port_byte = strtol(tmp1 + 1, &endptr, 10);
	if (endptr != tmp2 || port_byte < 0 || port_byte > 255) {
		return -EINVAL;
	}
	*data_port += (uint16_t)(port_byte << 8);
	LOG_DBG("data port: %d", *data_port);

	return 0;
}

static void close_connection(struct ftp_client *client, int code, int error)
{
	keepalive_timer_cancel(client);

	if (FTP_PROPRIETARY(code) != 0) {
		switch (code) {
		case FTP_CODE_901_DISCONNECTED_BY_REMOTE:
			snprintf(client->ctrl_buf, sizeof(client->ctrl_buf),
				 "901 Disconnected(%d).\r\n", error);
			break;
		case FTP_CODE_902_CONNECTION_ABORTED:
			snprintf(client->ctrl_buf, sizeof(client->ctrl_buf),
				 "902 Connection aborted(%d).\r\n", error);
			break;
		case FTP_CODE_903_SOCKET_POLL_ERROR:
			snprintf(client->ctrl_buf, sizeof(client->ctrl_buf),
				 "903 Poll error(%d).\r\n", error);
			break;
		case FTP_CODE_904_UNEXPECTED_POLL_EVENT:
			snprintf(client->ctrl_buf, sizeof(client->ctrl_buf),
				 "904 Unexpected poll event(%d).\r\n", error);
			break;
		case FTP_CODE_905_NETWORK_DOWN:
			snprintf(client->ctrl_buf, sizeof(client->ctrl_buf),
				 "905 Network down (%d).\r\n", error);
			break;
		default:
			snprintf(client->ctrl_buf, sizeof(client->ctrl_buf),
				 "900 Unknown error(%d).\r\n", -ENOEXEC);
			break;
		}
		client->ctrl_callback(client->ctrl_buf, strlen(client->ctrl_buf));
	}
	/* Should be impossible, just in case */
	if (client->data_sock != INVALID_SOCKET) {
		zsock_close(client->data_sock);
		client->data_sock = INVALID_SOCKET;
	}

	if (client->ctrl_sock != INVALID_SOCKET) {
		zsock_close(client->ctrl_sock);
		client->ctrl_sock = INVALID_SOCKET;
		client->connected = false;
		client->sec_tag = SEC_TAG_TLS_INVALID;
	}
}

static int do_ftp_send_ctrl(struct ftp_client *client, const uint8_t *message, int length)
{
	int ret = 0;
	uint32_t offset = 0;

	LOG_DBG("%s", (char *)message);
	while (offset < length) {
		ret = zsock_send(client->ctrl_sock, message + offset, length - offset, 0);
		if (ret < 0) {
			ret = -errno;
			LOG_ERR("send cmd failed: %d", ret);
			break;
		}
		offset += ret;
		ret = 0;
	}
	/* Close connection on errors */
	if (ret < 0) {
		close_connection(client, ret == -ENETDOWN ? FTP_CODE_905_NETWORK_DOWN :
							    FTP_CODE_909_UNEXPECTED_ERROR, ret);
	} else {
		LOG_DBG("CMD sent");
	}

	keepalive_timer_reset(client);

	return ret;
}

static int handle_ctrl_response(struct ftp_client *client, bool post_result, int success_code)
{
	bool done = false;
	int reply_code = 0;

	if (client->ctrl_len > sizeof(client->ctrl_buf) - 1) {
		/* Shouldn't happen, but make sure we don't operate out of buffer. */
		return -EINVAL;
	}

	while (client->ctrl_len > 0 && !done) {
		size_t line_len;
		uint8_t backup;
		uint8_t *eol;

		/* Make sure buffer is NULL terminated so we can operate as with string. */
		client->ctrl_buf[client->ctrl_len] = 0x00;

		eol = strstr(client->ctrl_buf, "\r\n");
		if (eol == NULL) {
			/* No end line detected, need to read more data from the socket. */
			return -EAGAIN;
		}
		eol += 2;

		line_len = eol - client->ctrl_buf;

		/* Status line starts with a status code followed by space */
		if (line_len >= 4 && isdigit(client->ctrl_buf[0])) {
			char *endptr = NULL;

			reply_code = strtol(client->ctrl_buf, &endptr, 10);
			/* Strict parsing of "%d ". */
			if (reply_code > 0 && endptr != NULL && *endptr == ' ' &&
			    (reply_code == success_code || reply_code >= 400 ||
			     success_code == FTP_CODE_ANY)) {
				/* Stop if got expected code, no specific code was
				 * expected or received an error response (400+)
				 */
				done = true;
			}
		}

		/* Report NULL terminated line. */
		backup = *eol;
		*eol = 0x00;

		if (post_result) {
			client->ctrl_callback(client->ctrl_buf, line_len);
		}

		LOG_DBG("%s", client->ctrl_buf);

		*eol = backup;

		client->ctrl_len -= line_len;
		if (client->ctrl_len > 0) {
			memmove(client->ctrl_buf, client->ctrl_buf + line_len, client->ctrl_len);
		}
	}

	return reply_code;
}

static int recv_ctrl_response(struct ftp_client *client, enum ftp_reply_code *err_code)
{
	struct zsock_pollfd fds[1];
	int ret;

	fds[0].fd = client->ctrl_sock;
	fds[0].events = ZSOCK_POLLIN;

	ret = zsock_poll(fds, 1, FTP_CLIENT_POLL_TIMEOUT_MSEC);
	if (ret < 0) {
		ret = -errno;
		*err_code = FTP_CODE_903_SOCKET_POLL_ERROR;
		LOG_ERR("poll(ctrl) failed: (%d)", ret);
		goto out;
	}

	if (ret == 0) {
		ret = -ETIMEDOUT;
		*err_code = FTP_CODE_903_SOCKET_POLL_ERROR;
		LOG_DBG("poll(ctrl) timeout");
		goto out;
	}

	if ((fds[0].revents & ZSOCK_POLLHUP) == ZSOCK_POLLHUP) {
		ret = -ECONNRESET;
		*err_code = FTP_CODE_901_DISCONNECTED_BY_REMOTE;
		LOG_ERR("POLLHUP");
		goto out;
	}

	if ((fds[0].revents & ZSOCK_POLLIN) != ZSOCK_POLLIN) {
		ret = -EIO;
		*err_code = FTP_CODE_904_UNEXPECTED_POLL_EVENT;
		LOG_ERR("POLL 0x%08x", fds[0].revents);
		goto out;
	}

	ret = zsock_recv(client->ctrl_sock, client->ctrl_buf + client->ctrl_len,
			 sizeof(client->ctrl_buf) - client->ctrl_len - 1, 0);
	if (ret < 0) {
		ret = -errno;
		*err_code = ret == -ENETDOWN ? FTP_CODE_905_NETWORK_DOWN :
					       FTP_CODE_909_UNEXPECTED_ERROR;
		LOG_ERR("recv(ctrl) failed: (%d)", ret);
		goto out;
	}

	if (ret == 0) {
		ret = -ECONNRESET;
		*err_code = FTP_CODE_901_DISCONNECTED_BY_REMOTE;
		LOG_ERR("recv(ctrl) peer closed connection");
	}

out:
	return ret;
}

static int do_ftp_recv_ctrl(struct ftp_client *client, bool post_result, int success_code)
{
	enum ftp_reply_code err_code = FTP_CODE_500_GENERAL_ERROR;
	int ret;

	while (true) {
		/* Receive FTP control message */
		ret = recv_ctrl_response(client, &err_code);
		if (ret <= 0) {
			goto error;
		}

		keepalive_timer_reset(client);

		client->ctrl_len += ret;

		ret = handle_ctrl_response(client, post_result, success_code);
		if (ret != -EAGAIN) {
			break;
		}

		if (client->ctrl_len >= sizeof(client->ctrl_buf) - 1) {
			ret = -ENOMEM;
			err_code = FTP_CODE_909_UNEXPECTED_ERROR;
			LOG_ERR("recv(ctrl) buffer full");
			goto error;
		}
	}

	return ret;

error:
	close_connection(client, err_code, ret);

	return ret;
}

static int set_passive_mode(struct ftp_client *client, uint16_t *data_port)
{
	int ret;

	ret = do_ftp_send_ctrl(client, CMD_PASV, sizeof(CMD_PASV) - 1);
	if (ret != 0) {
		goto out;
	}

	ret = do_ftp_recv_ctrl(client, true, FTP_CODE_227_ENTERING_PASSIVE_MODE);
	if (ret != FTP_CODE_227_ENTERING_PASSIVE_MODE) {
		goto out;
	}

	ret = parse_pasv_msg(client->ctrl_buf, data_port);
	if (ret != 0) {
		goto out;
	}

out:
	return ret;
}

static int wait_transfer_complete(struct ftp_client *client)
{
	int ret;

	ret = do_ftp_recv_ctrl(client, true, FTP_CODE_226_CLOSING_DATA_CONN_SUCCESS);
	if (ret == FTP_CODE_226_CLOSING_DATA_CONN_SUCCESS) {
		ret = 0;
	}

	return ret;
}

static int do_ftp_send_data(struct ftp_client *client, uint16_t data_port,
			    const uint8_t *message, uint16_t length)
{
	int ret;
	uint32_t offset = 0;

	/* Establish data channel */
	ret = new_ftp_connection(client, FTP_CHANNEL_DATA, data_port);
	if (ret < 0) {
		return ret;
	}

	if (message == NULL || length == 0) {
		goto out;
	}

	while (offset < length) {
		ret = zsock_send(client->data_sock, message + offset, length - offset, 0);
		if (ret < 0) {
			ret = -errno;
			LOG_ERR("send data failed: %d", ret);
			break;
		}
		LOG_DBG("DATA sent %d", ret);
		offset += ret;
		ret = 0;
	}

out:
	zsock_close(client->data_sock);
	client->data_sock = INVALID_SOCKET;

	if (ret == 0) {
		ret = wait_transfer_complete(client);
	}

	keepalive_timer_reset(client);

	return ret;
}

static int do_ftp_recv_data(struct ftp_client *client, uint16_t data_port)
{
	int ret;
	struct zsock_pollfd fds[1];

	/* Establish data channel */
	ret = new_ftp_connection(client, FTP_CHANNEL_DATA, data_port);
	if (ret < 0) {
		return ret;
	}

	/* Receive FTP data message */
	fds[0].fd = client->data_sock;
	fds[0].events = ZSOCK_POLLIN;
	do {
		ret = zsock_poll(fds, 1, FTP_CLIENT_POLL_TIMEOUT_MSEC);
		if (ret < 0) {
			ret = -errno;
			LOG_ERR("poll(data) failed: (%d)", ret);
			break;
		}

		if (ret == 0) {
			ret = -ETIMEDOUT;
			LOG_DBG("poll(data) timeout");
			break;
		}

		if ((fds[0].revents & ZSOCK_POLLIN) != ZSOCK_POLLIN) {
			LOG_DBG("No more data");
			ret = 0;
			break;
		}

		ret = zsock_recv(client->data_sock, client->data_buf,
				 sizeof(client->data_buf), 0);
		if (ret < 0) {
			ret = -errno;
			LOG_ERR("recv(data) failed: (%d)", ret);
			break;
		}

		if (ret == 0) {
			/* Server close connection */
			break;
		}

		client->data_callback(client->data_buf, ret);
		LOG_DBG("DATA received %d", ret);
	} while (true);

	zsock_close(client->data_sock);
	client->data_sock = INVALID_SOCKET;

	if (ret == 0) {
		ret = wait_transfer_complete(client);
	}

	keepalive_timer_reset(client);

	return ret;
}

int ftp_open(struct ftp_client *client, const char *hostname, uint16_t port, int sec_tag)
{
	int ret;
	struct zsock_addrinfo *ai;

	if (client == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	if (client->connected) {
		LOG_ERR("FTP already connected");
		ret = -EINVAL;
		goto out;
	}

	/* Resolve the hostname in the preferred IP version .*/
	ret = zsock_getaddrinfo(hostname, NULL, NULL, &ai);
	if (ret != 0) {
		LOG_ERR("Failed to resolve hostname (\"%s\"): %s",
			hostname, zsock_gai_strerror(ret));
		ret = -EHOSTUNREACH;
		goto out;
	}

	memcpy(&client->remote, ai->ai_addr, ai->ai_addrlen);
	zsock_freeaddrinfo(ai);

	client->sec_tag = sec_tag;

	/* open control socket */
	ret = new_ftp_connection(client, FTP_CHANNEL_CTRL, port);
	if (ret != 0) {
		goto out;
	}

	/* Receive server greeting */
	ret = do_ftp_recv_ctrl(client, true, FTP_CODE_220_SERVICE_READY);
	if (ret != FTP_CODE_220_SERVICE_READY) {
		zsock_close(client->ctrl_sock);
		client->ctrl_sock = INVALID_SOCKET;
		goto out;
	}

	/* Send UTF8 option */
	snprintf(client->ctrl_buf, sizeof(client->ctrl_buf), CMD_OPTS, "UTF8 ON");
	ret = do_ftp_send_ctrl(client, client->ctrl_buf, strlen(client->ctrl_buf));
	if (ret != 0) {
		zsock_close(client->ctrl_sock);
		client->ctrl_sock = INVALID_SOCKET;
		goto out;
	}

	(void)do_ftp_recv_ctrl(client, true, FTP_CODE_ANY);

	LOG_DBG("FTP opened");

out:
	k_mutex_unlock(&client->lock);

	return ret;
}

int ftp_login(struct ftp_client *client, const char *username, const char *password)
{
	int ret;

	if (client == NULL) {
		return -EINVAL;
	}

	/* Validate inputs */
	ret = validate_ftp_param(username, FTP_MAX_USERNAME);
	if (ret != 0) {
		return ret;
	}

	ret = validate_ftp_param(password, FTP_MAX_PASSWORD);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	/* send username */
	snprintf(client->ctrl_buf, sizeof(client->ctrl_buf), CMD_USER, username);
	ret = do_ftp_send_ctrl(client, client->ctrl_buf, strlen(client->ctrl_buf));
	if (ret != 0) {
		goto out;
	}

	ret = do_ftp_recv_ctrl(client, true, FTP_CODE_331_USERNAME_OK_NEED_PASSWORD);
	if (ret == FTP_CODE_331_USERNAME_OK_NEED_PASSWORD) {
		/* send password if requested */
		snprintf(client->ctrl_buf, sizeof(client->ctrl_buf), CMD_PASS, password);
		ret = do_ftp_send_ctrl(client, client->ctrl_buf, strlen(client->ctrl_buf));
		if (ret != 0) {
			goto out;
		}

		ret = do_ftp_recv_ctrl(client, true, FTP_CODE_230_USER_LOGGED_IN);
	}

	if (ret != FTP_CODE_230_USER_LOGGED_IN) {
		goto out;
	}

	client->connected = true;
	ret = 0;

	/* Start keep alive timer */
	keepalive_timer_reset(client);

out:
	k_mutex_unlock(&client->lock);

	return ret;
}

int ftp_close(struct ftp_client *client)
{
	int ret = 0;

	if (client == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	if (client->connected) {
		ret = do_ftp_send_ctrl(client, CMD_QUIT, sizeof(CMD_QUIT) - 1);
		if (ret != 0) {
			goto out;
		}

		/* Some FTP servers do not reply QUIT */
		(void)do_ftp_recv_ctrl(client, true, FTP_CODE_221_SERVICE_CLOSING_CONN);
	}

	close_connection(client, FTP_CODE_200_OK, 0);

	client->connected = false;
	ret = 0;

out:
	k_mutex_unlock(&client->lock);

	return ret;
}

int ftp_status(struct ftp_client *client)
{
	int ret;

	if (client == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	/* get server system type */
	ret = do_ftp_send_ctrl(client, CMD_SYST, sizeof(CMD_SYST) - 1);
	if (ret != 0) {
		goto out;
	}

	ret = do_ftp_recv_ctrl(client, true, FTP_CODE_215_NAME_SYSTEM_TYPE);
	if (ret != FTP_CODE_215_NAME_SYSTEM_TYPE) {
		goto out;
	}

	/* get server and connection status */
	ret = do_ftp_send_ctrl(client, CMD_STAT, sizeof(CMD_STAT) - 1);
	if (ret != 0) {
		goto out;
	}

	ret = do_ftp_recv_ctrl(client, true, FTP_CODE_211_SYSTEM_STATUS);
	if (ret != FTP_CODE_211_SYSTEM_STATUS) {
		goto out;
	}

	ret = 0;

out:
	k_mutex_unlock(&client->lock);

	return ret;
}

int ftp_type(struct ftp_client *client, enum ftp_transfer_type type)
{
	int ret;

	if (client == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	if (type == FTP_TYPE_ASCII) {
		ret = do_ftp_send_ctrl(client, CMD_TYPE_A, sizeof(CMD_TYPE_A) - 1);
	} else if (type == FTP_TYPE_BINARY) {
		ret = do_ftp_send_ctrl(client, CMD_TYPE_I, sizeof(CMD_TYPE_I) - 1);
	} else {
		ret = -EINVAL;
	}

	if (ret != 0) {
		goto out;
	}

	ret = do_ftp_recv_ctrl(client, true, FTP_CODE_200_OK);
	if (ret != FTP_CODE_200_OK) {
		goto out;
	}

	ret = 0;

out:
	k_mutex_unlock(&client->lock);

	return ret;
}

int ftp_pwd(struct ftp_client *client)
{
	int ret;

	if (client == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	ret = do_ftp_send_ctrl(client, CMD_PWD, sizeof(CMD_PWD) - 1);
	if (ret != 0) {
		goto out;
	}

	ret = do_ftp_recv_ctrl(client, true, FTP_CODE_257_PATHNAME_CREATED);
	if (ret != FTP_CODE_257_PATHNAME_CREATED) {
		goto out;
	}

	ret = 0;

out:
	k_mutex_unlock(&client->lock);

	return ret;
}

int ftp_list(struct ftp_client *client, const char *options, const char *target)
{
	int ret;
	uint16_t data_port;

	if (client == NULL) {
		return -EINVAL;
	}

	/* Validate inputs */
	ret = validate_ftp_param(options, FTP_MAX_OPTIONS);
	if (ret != 0) {
		return ret;
	}

	ret = validate_ftp_param(target, FTP_MAX_PATHNAME);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	/* Always set Passive mode to act as TCP client */
	ret = set_passive_mode(client, &data_port);
	if (ret != 0) {
		goto out;
	}

	/* Send LIST/NLST command in control channel */
	if (strlen(options) != 0) {
		if (strlen(target) != 0) {
			snprintf(client->ctrl_buf, sizeof(client->ctrl_buf),
				 CMD_LIST_OPT_FILE, options, target);
		} else {
			snprintf(client->ctrl_buf, sizeof(client->ctrl_buf),
				 CMD_LIST_OPT, options);
		}
	} else {
		if (strlen(target) != 0) {
			snprintf(client->ctrl_buf, sizeof(client->ctrl_buf),
				 CMD_LIST_FILE, target);
		} else {
			strcpy(client->ctrl_buf, CMD_NLST);
		}
	}

	ret = do_ftp_send_ctrl(client, client->ctrl_buf, strlen(client->ctrl_buf));
	if (ret != 0) {
		goto out;
	}

	/* Wait for file status ok reply from the server. */
	ret = do_ftp_recv_ctrl(client, true, FTP_CODE_150_FILE_STATUS_OK);
	if (ret != FTP_CODE_150_FILE_STATUS_OK) {
		goto out;
	}

	ret = do_ftp_recv_data(client, data_port);

out:
	k_mutex_unlock(&client->lock);

	return ret;
}

int ftp_cwd(struct ftp_client *client, const char *folder)
{
	int ret;

	if (client == NULL) {
		return -EINVAL;
	}

	/* Validate input */
	ret = validate_ftp_param(folder, FTP_MAX_PATHNAME);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	if (strcmp(folder, "..") == 0) {
		ret = do_ftp_send_ctrl(client, CMD_CDUP, sizeof(CMD_CDUP) - 1);
	} else {
		snprintf(client->ctrl_buf, sizeof(client->ctrl_buf), CMD_CWD, folder);
		ret = do_ftp_send_ctrl(client, client->ctrl_buf, strlen(client->ctrl_buf));
	}

	if (ret != 0) {
		goto out;
	}

	ret = do_ftp_recv_ctrl(client, true, FTP_CODE_250_FILE_ACTION_COMPLETED);
	if (ret != FTP_CODE_250_FILE_ACTION_COMPLETED) {
		goto out;
	}

	ret = 0;

out:
	k_mutex_unlock(&client->lock);

	return ret;
}

int ftp_mkd(struct ftp_client *client, const char *folder)
{
	int ret;

	if (client == NULL) {
		return -EINVAL;
	}

	/* Validate input */
	ret = validate_ftp_param(folder, FTP_MAX_PATHNAME);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	snprintf(client->ctrl_buf, sizeof(client->ctrl_buf), CMD_MKD, folder);
	ret = do_ftp_send_ctrl(client, client->ctrl_buf, strlen(client->ctrl_buf));
	if (ret != 0) {
		goto out;
	}

	ret = do_ftp_recv_ctrl(client, true, FTP_CODE_257_PATHNAME_CREATED);
	if (ret != FTP_CODE_257_PATHNAME_CREATED) {
		goto out;
	}

	ret = 0;
out:
	k_mutex_unlock(&client->lock);

	return ret;
}

int ftp_rmd(struct ftp_client *client, const char *folder)
{
	int ret;

	if (client == NULL) {
		return -EINVAL;
	}

	/* Validate input */
	ret = validate_ftp_param(folder, FTP_MAX_PATHNAME);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	snprintf(client->ctrl_buf, sizeof(client->ctrl_buf), CMD_RMD, folder);
	ret = do_ftp_send_ctrl(client, client->ctrl_buf, strlen(client->ctrl_buf));
	if (ret != 0) {
		goto out;
	}

	ret = do_ftp_recv_ctrl(client, true, FTP_CODE_250_FILE_ACTION_COMPLETED);
	if (ret != FTP_CODE_250_FILE_ACTION_COMPLETED) {
		goto out;
	}

	ret = 0;
out:
	k_mutex_unlock(&client->lock);

	return ret;
}

int ftp_rename(struct ftp_client *client, const char *old_name, const char *new_name)
{
	int ret;

	if (client == NULL) {
		return -EINVAL;
	}

	ret = validate_ftp_param(old_name, FTP_MAX_PATHNAME);
	if (ret != 0) {
		return ret;
	}


	ret = validate_ftp_param(new_name, FTP_MAX_PATHNAME);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	snprintf(client->ctrl_buf, sizeof(client->ctrl_buf), CMD_RNFR, old_name);
	ret = do_ftp_send_ctrl(client, client->ctrl_buf, strlen(client->ctrl_buf));
	if (ret != 0) {
		goto out;
	}

	ret = do_ftp_recv_ctrl(client, true, FTP_CODE_350_FILE_ACTION_PENDING);
	if (ret != FTP_CODE_350_FILE_ACTION_PENDING) {
		goto out;
	}

	snprintf(client->ctrl_buf, sizeof(client->ctrl_buf), CMD_RNTO, new_name);
	ret = do_ftp_send_ctrl(client, client->ctrl_buf, strlen(client->ctrl_buf));
	if (ret != 0) {
		goto out;
	}

	ret = do_ftp_recv_ctrl(client, true, FTP_CODE_250_FILE_ACTION_COMPLETED);
	if (ret != FTP_CODE_250_FILE_ACTION_COMPLETED) {
		goto out;
	}

	ret = 0;

out:
	k_mutex_unlock(&client->lock);

	return ret;
}

int ftp_delete(struct ftp_client *client, const char *file)
{
	int ret;

	if (client == NULL) {
		return -EINVAL;
	}

	/* Validate input */
	ret = validate_ftp_param(file, FTP_MAX_PATHNAME);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	snprintf(client->ctrl_buf, sizeof(client->ctrl_buf), CMD_DELE, file);
	ret = do_ftp_send_ctrl(client, client->ctrl_buf, strlen(client->ctrl_buf));
	if (ret != 0) {
		goto out;
	}

	ret = do_ftp_recv_ctrl(client, true, FTP_CODE_250_FILE_ACTION_COMPLETED);
	if (ret != FTP_CODE_250_FILE_ACTION_COMPLETED) {
		goto out;
	}

	ret = 0;

out:
	k_mutex_unlock(&client->lock);

	return ret;
}

int ftp_get(struct ftp_client *client, const char *file)
{
	int ret;
	uint16_t data_port;

	if (client == NULL) {
		return -EINVAL;
	}

	/* Validate input */
	ret = validate_ftp_param(file, FTP_MAX_PATHNAME);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	/* Always set Passive mode to act as TCP client */
	ret = set_passive_mode(client, &data_port);
	if (ret != 0) {
		goto out;
	}

	/* Send RETR command in control channel */
	snprintf(client->ctrl_buf, sizeof(client->ctrl_buf), CMD_RETR, file);
	ret = do_ftp_send_ctrl(client, client->ctrl_buf, strlen(client->ctrl_buf));
	if (ret != 0) {
		goto out;
	}

	/* Wait for file status ok reply from the server. */
	ret = do_ftp_recv_ctrl(client, true, FTP_CODE_150_FILE_STATUS_OK);
	if (ret != FTP_CODE_150_FILE_STATUS_OK) {
		goto out;
	}

	ret = do_ftp_recv_data(client, data_port);

out:
	k_mutex_unlock(&client->lock);

	return ret;
}

int ftp_put(struct ftp_client *client, const char *file, const uint8_t *data,
	    uint16_t length, int type)
{
	int ret;
	uint16_t data_port = 0;

	if (client == NULL) {
		return -EINVAL;
	}

	if (type != FTP_PUT_NORMAL && type != FTP_PUT_UNIQUE && type != FTP_PUT_APPEND) {
		return -EINVAL;

	}
	if ((type == FTP_PUT_NORMAL || type == FTP_PUT_APPEND) && file == NULL) {
		return -EINVAL;
	}

	if (type == FTP_PUT_APPEND && data == NULL) {
		return -EINVAL;
	}

	/* Validate file parameter if provided */
	if (file != NULL) {
		ret = validate_ftp_param(file, FTP_MAX_FILENAME);
		if (ret != 0) {
			return ret;
		}
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	/** Typical sequence:
	 * FTP		51	Request: PASV
	 * FTP		96	Response: 227 Entering Passive Mode (90,130,70,73,105,177).
	 * FTP		63	Request: STOR upload2.txt
	 * FTP-DATA	53	FTP Data: 8 bytes (PASV) (STOR upload2.txt)
	 * FTP		67	Response: 150 Ok to send data.
	 * FTP		69	Response: 226 Transfer complete.
	 */

	/* Always set Passive mode to act as TCP client */
	ret = set_passive_mode(client, &data_port);
	if (ret != 0) {
		goto out;
	}

	if (type == FTP_PUT_NORMAL) {
		/* Send STOR command in control channel */
		snprintf(client->ctrl_buf, sizeof(client->ctrl_buf), CMD_STOR, file);
		ret = do_ftp_send_ctrl(client, client->ctrl_buf, strlen(client->ctrl_buf));
	} else if (type == FTP_PUT_UNIQUE) {
		/* Send STOU command in control channel */
		snprintf(client->ctrl_buf, sizeof(client->ctrl_buf), CMD_STOU);
		ret = do_ftp_send_ctrl(client, client->ctrl_buf, strlen(client->ctrl_buf));
	} else {
		/* Send APPE command in control channel */
		snprintf(client->ctrl_buf, sizeof(client->ctrl_buf), CMD_APPE, file);
		ret = do_ftp_send_ctrl(client, client->ctrl_buf, strlen(client->ctrl_buf));
	}

	if (ret != 0) {
		goto out;
	}

	/* Wait for file status ok reply from the server. */
	ret = do_ftp_recv_ctrl(client, true, FTP_CODE_150_FILE_STATUS_OK);
	if (ret != FTP_CODE_150_FILE_STATUS_OK) {
		goto out;
	}

	/* Now send data if any */
	ret = do_ftp_send_data(client, data_port, data, length);

out:
	k_mutex_unlock(&client->lock);

	return ret;
}

int ftp_keepalive(struct ftp_client *client)
{
	int ret;

	if (client == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	ret = do_ftp_send_ctrl(client, CMD_NOOP, sizeof(CMD_NOOP) - 1);
	if (ret != 0) {
		goto out;
	}

	ret = do_ftp_recv_ctrl(client, false, FTP_CODE_200_OK);
	if (ret != FTP_CODE_200_OK) {
		goto out;
	}

	ret = 0;

out:
	k_mutex_unlock(&client->lock);

	return ret;
}

int ftp_init(struct ftp_client *client, ftp_client_callback_t ctrl_callback,
	     ftp_client_callback_t data_callback)
{
	if (client == NULL || ctrl_callback == NULL || data_callback == NULL) {
		return -EINVAL;
	}

	client->ctrl_sock = INVALID_SOCKET;
	client->data_sock = INVALID_SOCKET;
	client->ctrl_len = 0;
	client->connected = false;
	client->sec_tag = SEC_TAG_TLS_INVALID;
	client->ctrl_callback = ctrl_callback;
	client->data_callback = data_callback;

	memset(&client->remote, 0, sizeof(client->remote));

	k_mutex_init(&client->lock);
#if CONFIG_FTP_CLIENT_KEEPALIVE_TIME > 0
	k_work_init_delayable(&client->keepalive_work, keepalive_handler);
#endif

	return 0;
}

int ftp_uninit(struct ftp_client *client)
{
	int ret = 0;

	if (client == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&client->lock, K_FOREVER);

	if (client->ctrl_sock != INVALID_SOCKET) {
		ret = ftp_close(client);
	}

	k_mutex_unlock(&client->lock);

	return ret;
}

#if CONFIG_FTP_CLIENT_KEEPALIVE_TIME > 0
static int ftp_sys_init(void)
{
	k_work_queue_start(&ftp_work_q, ftp_stack_area,
			   K_THREAD_STACK_SIZEOF(ftp_stack_area),
			   FTP_PRIORITY, NULL);

	return 0;
}

SYS_INIT(ftp_sys_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
