/*
 * Copyright (c) 2018 Antmicro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_socks, CONFIG_SOCKS_LOG_LEVEL);

#include <zephyr.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <version.h>

#include <net/socks.h>
#include <net/socket.h>

#include "socks_internal.h"

static struct socks5_ctx context;

static void socks5_client_thread(struct socks5_proxy *proxy);
static void socks5_destination_thread(struct socks5_proxy *proxy);

void dump_pkt(int len, char *buf, const char *label)
{
	int i;

	return;

	if (len <= 0)
		return;

	printk("PKT %s", label);

	for (i = 0; i < len; ++i) {
		if (i % 8 == 0)
			printk("\n");
		printk("%02x ", buf[i] & 0xff);
	}

	if (i % 8)
		printk("\n");

	printk("\n");
}

static const char *socks5_auth_method2str(u8_t method)
{
	switch (method) {
	case SOCKS5_AUTH_METHOD_NOAUTH:
		return "No authentication required";
	case SOCKS5_AUTH_METHOD_GSSAPI:
		return "GSSAPI";
	case SOCKS5_AUTH_METHOD_USERPASS:
		return "User/Password";
	}

	return "Unknown";
}

static const char *socks5_atyp2str(u8_t atyp)
{
	switch (atyp) {
	case SOCKS5_ATYP_IPV4:
		return "IPv4";
	case SOCKS5_ATYP_IPV6:
		return "IPv6";
	case SOCKS5_ATYP_DOMAINNAME:
		return "Domain name";
	}

	return "Unknown";
}

static void socks5_init_method_resp(struct socks5_method_response *resp,
				    u8_t method)
{
	resp->ver = SOCKS5_PKT_MAGIC;
	resp->method = method;
}

static void socks5_handle_negotiation(struct socks5_proxy *proxy)
{
	struct socks5_method_request *method_request;
	struct socks5_method_response *method_response;
	int sent;
	int i;

	method_request =
		(struct socks5_method_request *) proxy->buffer_rcv;
	method_response =
		(struct socks5_method_response *) proxy->buffer_snd;
	u8_t method_negotiated = SOCKS5_AUTH_METHOD_NONEG;

	/* Sanity checks */
	if (method_request->ver != SOCKS5_PKT_MAGIC) {
		NET_DBG("Not a SOCKS5 packet");
		proxy->state = SOCKS5_STATE_ERROR;
		return;
	}

	for (i = 0; i < method_request->nmethods &&
			proxy->state != SOCKS5_STATE_NEGOTIATED; ++i) {

		u8_t method = method_request->methods[i];

		switch (method) {
		/* Supported methods */
		case SOCKS5_AUTH_METHOD_NOAUTH:
			NET_INFO("Authorizing using: %s",
				 socks5_auth_method2str(method));
			method_negotiated = method;
			proxy->state = SOCKS5_STATE_NEGOTIATED;
			break;
		/* Unsupported methods */
		case SOCKS5_AUTH_METHOD_GSSAPI:
		case SOCKS5_AUTH_METHOD_USERPASS:
		default:
			NET_DBG("Skipping authentication method: %s",
				 socks5_auth_method2str(method));
			break;
		}
	}

	if (proxy->state != SOCKS5_STATE_NEGOTIATED) {
		NET_INFO("No authentication method was acceptable");
	}

	socks5_init_method_resp(method_response, method_negotiated);
	sent = send(proxy->client.fd, proxy->buffer_snd,
		    sizeof(struct socks5_method_response), 0);
	dump_pkt(sent, proxy->buffer_snd, "SENT");
}

static void socks5_init_cmd_resp(struct socks5_command_response *resp,
				 u8_t code)
{
	resp->ver = SOCKS5_PKT_MAGIC;
	resp->rep = code;
	resp->rsv = SOCKS5_PKT_RSV;
}

static void socks5_init_cmd_resp_ipv4(struct socks5_command_response *resp,
				      struct socks5_ipv4_addr *bnd_addr,
				      u8_t code)
{
	socks5_init_cmd_resp(resp, code);

	resp->atyp = SOCKS5_ATYP_IPV4;

	/* Clean address and port data */
	memset(bnd_addr, 0x00, sizeof(struct socks5_ipv4_addr));
}
static void socks5_send_cmd_resp(struct socks5_proxy *proxy,
				 struct socks5_command_response *resp)
{
	int size;

	switch (resp->atyp) {
	case SOCKS5_ATYP_IPV4:
		size = sizeof(struct socks5_ipv4_addr);
		break;
	case SOCKS5_ATYP_IPV6:
		size = sizeof(struct socks5_ipv4_addr);
		break;
	default:
		size = 0;
		break;
	}

	if (size == 0) {
		LOG_ERR("Sending error");
		return;
	}

	size += sizeof(struct socks5_method_response);

	size = send(proxy->client.fd, proxy->buffer_snd, size, 0);

	dump_pkt(size, proxy->buffer_snd, "SENT");
}

static void socks5_handle_connect_command(struct socks5_proxy *proxy)
{
	struct socks5_command_request *command_request;
	struct socks5_command_response *command_response;

	struct socks5_ipv4_addr *dst_addr;
	struct socks5_ipv4_addr *bnd_addr_ipv4;

	u8_t response_code;

	int ret;

	command_request =
		(struct socks5_command_request *) proxy->buffer_rcv;

	command_response =
		(struct socks5_command_response *) proxy->buffer_snd;

	bnd_addr_ipv4 = (struct socks5_ipv4_addr *)
		(proxy->buffer_snd + sizeof(struct socks5_command_response));

	/* Sanity checks */

	if (command_request->ver != SOCKS5_PKT_MAGIC) {
		NET_DBG("Not a SOCKS5 packet");
		proxy->state = SOCKS5_STATE_ERROR;
		return;
	}

	if (command_request->rsv != SOCKS5_PKT_RSV) {
		NET_DBG("Not a SOCKS5 packet");
		proxy->state = SOCKS5_STATE_ERROR;
		return;
	}

	switch (command_request->atyp) {
	case SOCKS5_ATYP_IPV4:
		dst_addr = (struct socks5_ipv4_addr *)
			((u8_t *) command_request +
			 sizeof(struct socks5_command_request));

		NET_ERR("Connecting to %d.%d.%d.%d:%d",
			dst_addr->addr[0],
			dst_addr->addr[1],
			dst_addr->addr[2],
			dst_addr->addr[3],
			ntohs(dst_addr->port));

		inet_pton(AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR,
			  &proxy->destination.addr.sin_addr);

		proxy->destination.addr.sin_family = AF_INET;
		memcpy(&proxy->destination.addr.sin_addr.s_addr, dst_addr, 4);
		proxy->destination.addr.sin_port = dst_addr->port;

		proxy->destination.fd =
			socket(proxy->destination.addr.sin_family,
			       SOCK_STREAM, IPPROTO_TCP);

		if (proxy->destination.fd < 0) {
			NET_ERR("Unable to create socket for destination");
			response_code = SOCKS5_CMD_RESP_FAILURE;
			proxy->state = SOCKS5_STATE_ERROR;
			break;
		}

		ret = connect(proxy->destination.fd,
			      (struct sockaddr *)&proxy->destination.addr,
			      sizeof(struct sockaddr_in));

		if (ret < 0) {
			NET_ERR("Could not connect to destination");
			response_code = SOCKS5_CMD_RESP_HOST_UNREACHABLE;
			proxy->state = SOCKS5_STATE_ERROR;
			break;
		}

		k_thread_create(&proxy->destination.thread,
				proxy->destination.stack,
				K_THREAD_STACK_SIZEOF(proxy->destination.stack),
				(k_thread_entry_t) socks5_destination_thread,
				proxy, NULL, NULL,
				K_PRIO_PREEMPT(6), 0, 0);

		/* Destination thread started, take the semaphore to make the
		 * client thread wait.
		 */
		k_sem_take(&proxy->exit_sem, K_FOREVER);

		k_thread_name_set(&proxy->destination.thread,
				  "socks5-destination");

		NET_DBG("Connected");

		proxy->state = SOCKS5_STATE_CONNECTED;
		response_code = SOCKS5_CMD_RESP_SUCCESS;

		break;
	case SOCKS5_ATYP_IPV6:
	case SOCKS5_ATYP_DOMAINNAME:
	default:
		NET_INFO("Address type not supported: %s",
			  socks5_atyp2str(command_request->atyp));

		proxy->state = SOCKS5_STATE_ERROR;
		response_code = SOCKS5_CMD_RESP_ATYP_NOT_SUPPORTED;
		break;
	}

	socks5_init_cmd_resp_ipv4(command_response, bnd_addr_ipv4,
				  response_code);

	socks5_send_cmd_resp(proxy, command_response);
}

static void socks5_handle_command(struct socks5_proxy *proxy)
{
	struct socks5_command_request *command_request;

	struct socks5_command_response *command_response;
	struct socks5_ipv4_addr *bnd_addr_ipv4;

	command_request =
		(struct socks5_command_request *) proxy->buffer_rcv;
	command_response =
		(struct socks5_command_response *) proxy->buffer_snd;

	bnd_addr_ipv4 = (struct socks5_ipv4_addr *)
		(proxy->buffer_snd + sizeof(struct socks5_command_response));

	switch (command_request->cmd) {
	case SOCKS5_CMD_CONNECT:
		socks5_handle_connect_command(proxy);
		break;
	case SOCKS5_CMD_BIND:
	case SOCKS5_CMD_UDP_ASSOCIATE:
	default:
		NET_DBG("Command not supported: %02x",
			command_request->cmd);

		socks5_init_cmd_resp_ipv4(command_response, bnd_addr_ipv4,
					  SOCKS5_CMD_RESP_CMD_NOT_SUPPORTED);

		socks5_send_cmd_resp(proxy, command_response);
	}
}

static void socks5_handle_pkt(struct socks5_proxy *proxy, int len_rcv)
{
	int sent;

	switch (proxy->state) {
	case SOCKS5_STATE_NEGOTIATING:
		socks5_handle_negotiation(proxy);
		break;
	case SOCKS5_STATE_NEGOTIATED:
		socks5_handle_command(proxy);
		break;
	case SOCKS5_STATE_CONNECTED:
		sent = send(proxy->destination.fd, proxy->buffer_rcv,
			    len_rcv, 0);

		dump_pkt(sent, proxy->buffer_rcv, "SENT TO DST");

		if (sent <= 0) {
			printk("Connection to destination terminated\n");
			proxy->state = SOCKS5_STATE_ERROR;
		} else if (len_rcv != sent) {
			printk("Could not fully proxy data\n");
			proxy->state = SOCKS5_STATE_ERROR;
		}

		break;
	default:
		NET_DBG("Machine state error");
		proxy->state = SOCKS5_STATE_ERROR;
	}
}

static void socks5_client_thread(struct socks5_proxy *proxy)
{
	int len_in;

	k_sem_init(&proxy->exit_sem, 1, 1);

	do {
		len_in = 0;
		memset(proxy->buffer_rcv, 0x0, SOCKS5_BSIZE);
		len_in = recv(proxy->client.fd, proxy->buffer_rcv,
			      SOCKS5_BSIZE - 1, 0);

		if (len_in <= 0) {
			NET_INFO("Connection to client terminated");
			proxy->state = SOCKS5_STATE_ERROR;
			break;
		}

		dump_pkt(len_in, proxy->buffer_rcv, "RECEIVED");

		socks5_handle_pkt(proxy, len_in);
	} while ((len_in > 0) && proxy->state != SOCKS5_STATE_ERROR);

	if (proxy->destination.fd != -1) {
		close(proxy->destination.fd);
	}

	close(proxy->client.fd);

	proxy->state = SOCKS5_STATE_IDLE;

	k_sem_take(&proxy->exit_sem, K_FOREVER);
	k_sem_give(proxy->busy_sem);
	NET_ERR("Exiting client thread");
}

static void socks5_destination_thread(struct socks5_proxy *proxy)
{
	int ret;

	do {
		ret = recv(proxy->destination.fd, proxy->buffer_dst,
			   SOCKS5_BSIZE - 1, 0);

		if (ret <= 0) {
			ret = 1;
			break;
		}

		dump_pkt(ret, proxy->buffer_dst, "RCV FROM DST");
		ret = send(proxy->client.fd, proxy->buffer_dst, ret, 0);
		dump_pkt(ret, proxy->buffer_dst, "SENT TO CL FROM DST");
	} while (ret > 0);

	if (proxy->client.fd >= 0) {
		close(proxy->client.fd);
		proxy->client.fd = -1;
	}

	if (proxy->destination.fd >= 0) {
		close(proxy->destination.fd);
		proxy->destination.fd = -1;
	}

	LOG_ERR("Exiting destination thread\n");

	k_sem_give(&proxy->exit_sem);
}

int socks_init(struct socks_ctx *ctx, u16_t port)
{
	struct socks5_proxy *proxy;
	struct socks5_connection *client;
	int ret;
	int i;

	NET_INFO("Starting SOCKS proxy server");

	context.fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (context.fd < 0) {
		NET_ERR("Could not open socket");
		return context.fd;
	}

	context.addr.sin_family = AF_INET;
	context.addr.sin_addr.s_addr = INADDR_ANY;
	context.addr.sin_port = htons(port);

	ret = bind(context.fd, (struct sockaddr *) &context.addr,
		   sizeof(struct sockaddr_in));

	if (ret < 0) {
		NET_ERR("Could not bind socket");
		return ret;
	}

	listen(context.fd, 5);

	for (i = 0; i < SOCKS5_MAX_CONNECTIONS; ++i) {
		context.proxy[i].state = SOCKS5_STATE_IDLE;
		context.proxy[i].id = i + 1;
		context.proxy[i].busy_sem = &context.busy_sem;
	}

	k_sem_init(&context.busy_sem, 1, 1);

	NET_DBG("SOCKS5 socket ready");

	while (1) {
		/* Find a free client */
		proxy = NULL;

		for (i = 0; proxy == NULL; i++, i %= SOCKS5_MAX_CONNECTIONS) {
			if (i == 0) {
				k_sem_take(&context.busy_sem, K_FOREVER);
			}

			if (context.proxy[i].state == SOCKS5_STATE_IDLE) {
				proxy = &context.proxy[i];
				client = &proxy->client;
				break;
			}
		}

		NET_INFO("Awaiting connection on %d/%d", i + 1,
			 SOCKS5_MAX_CONNECTIONS);
		client->len = sizeof(client->addr);
		client->fd = accept(context.fd,
				    (struct sockaddr *) &client->addr,
				    &client->len);

		if (client->fd < 0) {
			NET_ERR("Could not accept incoming connection");
			continue;
		}

		NET_INFO("Connection accepted");

		proxy->state = SOCKS5_STATE_NEGOTIATING;
		k_thread_create(&client->thread,
				client->stack,
				K_THREAD_STACK_SIZEOF(client->stack),
				(k_thread_entry_t) socks5_client_thread,
				proxy, NULL, NULL,
				K_PRIO_PREEMPT(5), 0, 0);
		k_thread_name_set(&client->thread, "socks5-client");
	}

	close(context.fd);

	return 0;
}
