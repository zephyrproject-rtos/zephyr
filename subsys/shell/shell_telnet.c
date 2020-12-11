/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>

#include <logging/log.h>
#include <net/net_context.h>
#include <net/net_ip.h>
#include <net/net_pkt.h>
#include <shell/shell_telnet.h>

#include "shell_telnet_protocol.h"

SHELL_TELNET_DEFINE(shell_transport_telnet);
SHELL_DEFINE(shell_telnet, CONFIG_SHELL_PROMPT_TELNET, &shell_transport_telnet,
	     CONFIG_SHELL_TELNET_LOG_MESSAGE_QUEUE_SIZE,
	     CONFIG_SHELL_TELNET_LOG_MESSAGE_QUEUE_TIMEOUT,
	     SHELL_FLAG_OLF_CRLF);

LOG_MODULE_REGISTER(shell_telnet, CONFIG_SHELL_TELNET_LOG_LEVEL);

struct shell_telnet *sh_telnet;

/* Various definitions mapping the TELNET service configuration options */
#define TELNET_PORT      CONFIG_SHELL_TELNET_PORT
#define TELNET_LINE_SIZE CONFIG_SHELL_TELNET_LINE_BUF_SIZE
#define TELNET_TIMEOUT   CONFIG_SHELL_TELNET_SEND_TIMEOUT

#define TELNET_MIN_MSG 2

/* Basic TELNET implmentation. */

static void telnet_end_client_connection(void)
{
	struct net_pkt *pkt;

	(void)net_context_put(sh_telnet->client_ctx);

	sh_telnet->client_ctx = NULL;
	sh_telnet->output_lock = false;

	k_delayed_work_cancel(&sh_telnet->send_work);

	/* Flush the RX FIFO */
	while ((pkt = k_fifo_get(&sh_telnet->rx_fifo, K_NO_WAIT)) != NULL) {
		net_pkt_unref(pkt);
	}
}

static void telnet_sent_cb(struct net_context *client,
			   int status, void *user_data)
{
	if (status < 0) {
		telnet_end_client_connection();
		LOG_ERR("Could not send packet %d", status);
	}
}

static void telnet_command_send_reply(uint8_t *msg, uint16_t len)
{
	int err;

	if (sh_telnet->client_ctx == NULL) {
		return;
	}

	err = net_context_send(sh_telnet->client_ctx, msg, len, telnet_sent_cb,
			       K_FOREVER, NULL);
	if (err < 0) {
		LOG_ERR("Failed to send command %d, shutting down", err);
		telnet_end_client_connection();
	}
}

static void telnet_reply_ay_command(void)
{
	static const char alive[] = "Zephyr at your service\r\n";

	telnet_command_send_reply((uint8_t *)alive, strlen(alive));
}

static void telnet_reply_do_command(struct telnet_simple_command *cmd)
{
	switch (cmd->opt) {
	case NVT_OPT_SUPR_GA:
		cmd->op = NVT_CMD_WILL;
		break;
	default:
		cmd->op = NVT_CMD_WONT;
		break;
	}

	telnet_command_send_reply((uint8_t *)cmd,
				  sizeof(struct telnet_simple_command));
}

static void telnet_reply_command(struct telnet_simple_command *cmd)
{
	if (!cmd->iac) {
		return;
	}

	switch (cmd->op) {
	case NVT_CMD_AO:
		/* OK, no output then */
		sh_telnet->output_lock = true;
		sh_telnet->line_out.len = 0;
		k_delayed_work_cancel(&sh_telnet->send_work);
		break;
	case NVT_CMD_AYT:
		telnet_reply_ay_command();
		break;
	case NVT_CMD_DO:
		telnet_reply_do_command(cmd);
		break;
	default:
		LOG_DBG("Operation %u not handled", cmd->op);
		break;
	}
}

static int telnet_send(void)
{
	int err;

	if (sh_telnet->line_out.len == 0) {
		return 0;
	}

	if (sh_telnet->client_ctx == NULL) {
		return -ENOTCONN;
	}

	err = net_context_send(sh_telnet->client_ctx, sh_telnet->line_out.buf,
			       sh_telnet->line_out.len, telnet_sent_cb,
			       K_FOREVER, NULL);
	if (err < 0) {
		LOG_ERR("Failed to send %d, shutting down", err);
		telnet_end_client_connection();
		return err;
	}

	/* We reinitialize the line buffer */
	sh_telnet->line_out.len = 0;

	return 0;
}

static void telnet_send_prematurely(struct k_work *work)
{
	(void)telnet_send();
}

static inline bool telnet_handle_command(struct net_pkt *pkt)
{
	/* Commands are two or three bytes. */
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(cmd_access, uint16_t);
	struct telnet_simple_command *cmd;

	cmd = (struct telnet_simple_command *)net_pkt_get_data(pkt,
							       &cmd_access);
	if (!cmd || cmd->iac != NVT_CMD_IAC) {
		return false;
	}

	if (IS_ENABLED(CONFIG_SHELL_TELNET_SUPPORT_COMMAND)) {
		LOG_DBG("Got a command %u/%u/%u", cmd->iac, cmd->op, cmd->opt);
		telnet_reply_command(cmd);
	}

	return true;
}

static void telnet_recv(struct net_context *client,
			struct net_pkt *pkt,
			union net_ip_header *ip_hdr,
			union net_proto_header *proto_hdr,
			int status,
			void *user_data)
{
	size_t len;

	if (!pkt || status) {
		telnet_end_client_connection();

		LOG_DBG("Telnet client dropped (AF_INET%s) status %d",
			net_context_get_family(client) == AF_INET ?
			"" : "6", status);
		return;
	}

	len = net_pkt_remaining_data(pkt);
	if (len < TELNET_MIN_MSG) {
		LOG_DBG("Packet smaller than minimum length");
		goto unref;
	}

	if (telnet_handle_command(pkt)) {
		LOG_DBG("Handled command");
		goto unref;
	}

	/* Fifo add */
	k_fifo_put(&sh_telnet->rx_fifo, pkt);

	sh_telnet->shell_handler(SHELL_TRANSPORT_EVT_RX_RDY,
				 sh_telnet->shell_context);

	return;

unref:
	net_pkt_unref(pkt);
}

static void telnet_accept(struct net_context *client,
			  struct sockaddr *addr,
			  socklen_t addrlen,
			  int error,
			  void *user_data)
{
	if (error) {
		LOG_ERR("Error %d", error);
		goto error;
	}

	if (sh_telnet->client_ctx) {
		LOG_INF("A telnet client is already in.");
		goto error;
	}

	if (net_context_recv(client, telnet_recv, K_NO_WAIT, NULL)) {
		LOG_ERR("Unable to setup reception (family %u)",
			net_context_get_family(client));
		goto error;
	}

	net_context_set_accepting(client, false);

	LOG_DBG("Telnet client connected (family AF_INET%s)",
		net_context_get_family(client) == AF_INET ? "" : "6");

	sh_telnet->client_ctx = client;

	return;
error:
	net_context_put(client);
}

static void telnet_setup_server(struct net_context **ctx, sa_family_t family,
				struct sockaddr *addr, socklen_t addrlen)
{
	if (net_context_get(family, SOCK_STREAM, IPPROTO_TCP, ctx)) {
		LOG_ERR("No context available");
		goto error;
	}

	if (net_context_bind(*ctx, addr, addrlen)) {
		LOG_ERR("Cannot bind on family AF_INET%s",
			family == AF_INET ? "" : "6");
		goto error;
	}

	if (net_context_listen(*ctx, 0)) {
		LOG_ERR("Cannot listen on");
		goto error;
	}

	if (net_context_accept(*ctx, telnet_accept, K_NO_WAIT, NULL)) {
		LOG_ERR("Cannot accept");
		goto error;
	}

	LOG_DBG("Telnet console enabled on AF_INET%s",
		family == AF_INET ? "" : "6");

	return;
error:
	LOG_ERR("Unable to start telnet on AF_INET%s",
		family == AF_INET ? "" : "6");

	if (*ctx) {
		(void)net_context_put(*ctx);
		*ctx = NULL;
	}
}

static int telnet_init(void)
{
	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		struct sockaddr_in any_addr4 = {
			.sin_family = AF_INET,
			.sin_port = htons(TELNET_PORT),
			.sin_addr = INADDR_ANY_INIT
		};
		static struct net_context *ctx4;

		telnet_setup_server(&ctx4, AF_INET,
				    (struct sockaddr *)&any_addr4,
				    sizeof(any_addr4));
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		struct sockaddr_in6 any_addr6 = {
			.sin6_family = AF_INET6,
			.sin6_port = htons(TELNET_PORT),
			.sin6_addr = IN6ADDR_ANY_INIT
		};
		static struct net_context *ctx6;

		telnet_setup_server(&ctx6, AF_INET6,
				    (struct sockaddr *)&any_addr6,
				    sizeof(any_addr6));
	}

	LOG_INF("Telnet shell backend initialized");

	return 0;
}

/* Shell API */

static int init(const struct shell_transport *transport,
		const void *config,
		shell_transport_handler_t evt_handler,
		void *context)
{
	int err;

	sh_telnet = (struct shell_telnet *)transport->ctx;

	err = telnet_init();
	if (err != 0) {
		return err;
	}

	memset(sh_telnet, 0, sizeof(struct shell_telnet));

	sh_telnet->shell_handler = evt_handler;
	sh_telnet->shell_context = context;

	k_fifo_init(&sh_telnet->rx_fifo);
	k_delayed_work_init(&sh_telnet->send_work, telnet_send_prematurely);

	return 0;
}

static int uninit(const struct shell_transport *transport)
{
	if (sh_telnet == NULL) {
		return -ENODEV;
	}

	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking)
{
	if (sh_telnet == NULL) {
		return -ENODEV;
	}

	return 0;
}

static int write(const struct shell_transport *transport,
		 const void *data, size_t length, size_t *cnt)
{
	struct shell_telnet_line_buf *lb;
	size_t copy_len;
	int err;
	uint32_t timeout;

	if (sh_telnet == NULL) {
		*cnt = 0;
		return -ENODEV;
	}

	if (sh_telnet->client_ctx == NULL || sh_telnet->output_lock) {
		*cnt = length;
		return 0;
	}

	*cnt = 0;
	lb = &sh_telnet->line_out;

	/* Stop the transmission timer, so it does not interrupt the operation.
	 */
	timeout = k_delayed_work_remaining_get(&sh_telnet->send_work);
	k_delayed_work_cancel(&sh_telnet->send_work);

	do {
		if (lb->len + length - *cnt > TELNET_LINE_SIZE) {
			copy_len = TELNET_LINE_SIZE - lb->len;
		} else {
			copy_len = length - *cnt;
		}

		memcpy(lb->buf + lb->len, (uint8_t *)data + *cnt, copy_len);
		lb->len += copy_len;

		/* Send the data immediately if the buffer is full or line feed
		 * is recognized.
		 */
		if (lb->buf[lb->len - 1] == '\n' ||
		    lb->len == TELNET_LINE_SIZE) {
			err = telnet_send();
			if (err != 0) {
				*cnt = length;
				return err;
			}
		}

		*cnt += copy_len;
	} while (*cnt < length);

	if (lb->len > 0) {
		/* Check if the timer was already running, initialize otherwise.
		 */
		timeout = (timeout == 0) ? TELNET_TIMEOUT : timeout;

		k_delayed_work_submit(&sh_telnet->send_work, K_MSEC(timeout));
	}

	sh_telnet->shell_handler(SHELL_TRANSPORT_EVT_TX_RDY,
				 sh_telnet->shell_context);

	return 0;
}

static int read(const struct shell_transport *transport,
		void *data, size_t length, size_t *cnt)
{
	struct net_pkt *pkt;
	size_t read_len;
	bool flush = true;

	if (sh_telnet == NULL) {
		return -ENODEV;
	}

	if (sh_telnet->client_ctx == NULL) {
		goto no_data;
	}

	pkt = k_fifo_peek_head(&sh_telnet->rx_fifo);
	if (pkt == NULL) {
		goto no_data;
	}

	read_len = net_pkt_remaining_data(pkt);
	if (read_len > length) {
		read_len = length;
		flush = false;
	}

	*cnt = read_len;
	if (net_pkt_read(pkt, data, read_len) < 0) {
		/* Failed to read, get rid of the faulty packet. */
		LOG_ERR("Failed to read net packet.");
		*cnt = 0;
		flush = true;
	}

	if (flush) {
		(void)k_fifo_get(&sh_telnet->rx_fifo, K_NO_WAIT);
		net_pkt_unref(pkt);
	}

	return 0;

no_data:
	*cnt = 0;
	return 0;
}

const struct shell_transport_api shell_telnet_transport_api = {
	.init = init,
	.uninit = uninit,
	.enable = enable,
	.write = write,
	.read = read
};

static int enable_shell_telnet(const struct device *arg)
{
	ARG_UNUSED(arg);

	bool log_backend = CONFIG_SHELL_TELNET_INIT_LOG_LEVEL > 0;
	uint32_t level = (CONFIG_SHELL_TELNET_INIT_LOG_LEVEL > LOG_LEVEL_DBG) ?
		      CONFIG_LOG_MAX_LEVEL : CONFIG_SHELL_TELNET_INIT_LOG_LEVEL;

	return shell_init(&shell_telnet, NULL, true, log_backend, level);
}

SYS_INIT(enable_shell_telnet, APPLICATION, 0);

const struct shell *shell_backend_telnet_get_ptr(void)
{
	return &shell_telnet;
}
