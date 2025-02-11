/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(log_backend_net, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/sys/util_macro.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend_net.h>
#include <zephyr/net/hostname.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>

/* Set this to 1 if you want to see what is being sent to server */
#define DEBUG_PRINTING 0

#define DBG(fmt, ...) IF_ENABLED(DEBUG_PRINTING, (printk(fmt, ##__VA_ARGS__)))

#if defined(CONFIG_NET_IPV6) || CONFIG_NET_HOSTNAME_ENABLE
#define MAX_HOSTNAME_LEN NET_IPV6_ADDR_LEN
#else
#define MAX_HOSTNAME_LEN NET_IPV4_ADDR_LEN
#endif

static char dev_hostname[MAX_HOSTNAME_LEN + 1];

static uint8_t output_buf[CONFIG_LOG_BACKEND_NET_MAX_BUF_SIZE];
static bool net_init_done;
struct sockaddr server_addr;
static bool panic_mode;
static uint32_t log_format_current = CONFIG_LOG_BACKEND_NET_OUTPUT_DEFAULT;

static struct log_backend_net_ctx {
	int sock;
	bool is_tcp;
} ctx = {
	.sock = -1,
};

static int line_out(uint8_t *data, size_t length, void *output_ctx)
{
	struct log_backend_net_ctx *ctx = (struct log_backend_net_ctx *)output_ctx;
	int ret = -ENOMEM;
	struct msghdr msg = { 0 };
	struct iovec io_vector[2];
	int pos = 0;

	if (ctx == NULL) {
		return length;
	}

#if defined(CONFIG_NET_TCP)
	char len[sizeof("123456789")];

	if (ctx->is_tcp) {
		(void)snprintk(len, sizeof(len), "%zu ", length);
		io_vector[pos].iov_base = (void *)len;
		io_vector[pos].iov_len = strlen(len);
		pos++;
	}
#else
	if (ctx->is_tcp) {
		return -ENOTSUP;
	}
#endif

	io_vector[pos].iov_base = (void *)data;
	io_vector[pos].iov_len = length;
	pos++;

	msg.msg_iov = io_vector;
	msg.msg_iovlen = pos;

	ret = zsock_sendmsg(ctx->sock, &msg, ctx->is_tcp ? 0 : ZSOCK_MSG_DONTWAIT);
	if (ret < 0) {
		goto fail;
	}

	DBG(data);
fail:
	return length;
}

LOG_OUTPUT_DEFINE(log_output_net, line_out, output_buf, sizeof(output_buf));

static int do_net_init(struct log_backend_net_ctx *ctx)
{
	struct sockaddr *local_addr = NULL;
	struct sockaddr_in6 local_addr6 = {0};
	struct sockaddr_in local_addr4 = {0};
	socklen_t server_addr_len;
	int ret, proto = IPPROTO_UDP, type = SOCK_DGRAM;

	if (IS_ENABLED(CONFIG_NET_IPV4) && server_addr.sa_family == AF_INET) {
		local_addr = (struct sockaddr *)&local_addr4;
		server_addr_len = sizeof(struct sockaddr_in);
		local_addr4.sin_port = 0U;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && server_addr.sa_family == AF_INET6) {
		local_addr = (struct sockaddr *)&local_addr6;
		server_addr_len = sizeof(struct sockaddr_in6);
		local_addr6.sin6_port = 0U;
	}

	if (local_addr == NULL) {
		DBG("Server address unknown\n");
		return -EINVAL;
	}

	local_addr->sa_family = server_addr.sa_family;

	if (ctx->is_tcp) {
		proto = IPPROTO_TCP;
		type = SOCK_STREAM;
	}

	ret = zsock_socket(server_addr.sa_family, type, proto);
	if (ret < 0) {
		ret = -errno;
		DBG("Cannot get socket (%d)\n", ret);
		return ret;
	}

	ctx->sock = ret;

	if (IS_ENABLED(CONFIG_NET_HOSTNAME_ENABLE)) {
		(void)strncpy(dev_hostname, net_hostname_get(), MAX_HOSTNAME_LEN);

	} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
		   server_addr.sa_family == AF_INET6) {
		const struct in6_addr *src;

		src = net_if_ipv6_select_src_addr(
			NULL, &net_sin6(&server_addr)->sin6_addr);
		if (src) {
			net_addr_ntop(AF_INET6, src, dev_hostname,
				      MAX_HOSTNAME_LEN);

			net_ipaddr_copy(&local_addr6.sin6_addr, src);
		} else {
			goto unknown;
		}

	} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
		   server_addr.sa_family == AF_INET) {
		const struct in_addr *src;

		src = net_if_ipv4_select_src_addr(
				  NULL, &net_sin(&server_addr)->sin_addr);

		if (src) {
			net_addr_ntop(AF_INET, src, dev_hostname,
				      MAX_HOSTNAME_LEN);

			net_ipaddr_copy(&local_addr4.sin_addr, src);
		} else {
			goto unknown;
		}

	} else {
	unknown:
		DBG("Cannot setup local socket\n");
		ret = -EINVAL;
		goto err;
	}

	ret = zsock_bind(ctx->sock, local_addr, server_addr_len);
	if (ret < 0) {
		ret = -errno;
		DBG("Cannot bind socket (%d)\n", ret);
		goto err;
	}

	ret = zsock_connect(ctx->sock, &server_addr, server_addr_len);
	if (ret < 0) {
		ret = -errno;
		DBG("Cannot connect socket (%d)\n", ret);
		goto err;
	}

	log_output_ctx_set(&log_output_net, ctx);
	log_output_hostname_set(&log_output_net, dev_hostname);

	return 0;

err:
	(void)zsock_close(ctx->sock);
	ctx->sock = -1;

	return ret;
}

static void process(const struct log_backend *const backend,
		    union log_msg_generic *msg)
{
	uint32_t flags = LOG_OUTPUT_FLAG_FORMAT_SYSLOG |
			 LOG_OUTPUT_FLAG_TIMESTAMP |
			 LOG_OUTPUT_FLAG_THREAD;

	if (panic_mode) {
		return;
	}

	if (!net_init_done && do_net_init(&ctx) == 0) {
		net_init_done = true;
	}

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	log_output_func(&log_output_net, &msg->log, flags);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	log_format_current = log_type;
	return 0;
}

static bool check_net_init_done(void)
{
	bool ret = false;

	if (net_init_done) {
		/* Release context so it can be recreated with the specified ip address
		 * next time process() is called
		 */
		struct log_backend_net_ctx *ctx = log_output_net.control_block->ctx;
		int released;

		released = zsock_close(ctx->sock);
		if (released < 0) {
			LOG_ERR("Cannot release socket (%d)", ret);
			ret = false;
		} else {
			/* The socket is successfully closed so we flag it
			 * to be recreated with the new ip address
			 */
			net_init_done = false;
			ret = true;
		}

		ctx->sock = -1;

		return ret;
	}

	return true;
}

bool log_backend_net_set_addr(const char *addr)
{
	bool ret = check_net_init_done();

	if (!ret) {
		return ret;
	}

	net_sin(&server_addr)->sin_port = htons(514);

	ret = net_ipaddr_parse(addr, strlen(addr), &server_addr);
	if (!ret) {
		LOG_ERR("Cannot parse syslog server address");
		return ret;
	}

	return ret;
}

bool log_backend_net_set_ip(const struct sockaddr *addr)
{
	bool ret = check_net_init_done();

	if (!ret) {
		return ret;
	}

	if ((IS_ENABLED(CONFIG_NET_IPV4) && addr->sa_family == AF_INET) ||
	    (IS_ENABLED(CONFIG_NET_IPV6) && addr->sa_family == AF_INET6)) {
		memcpy(&server_addr, addr, sizeof(server_addr));

		net_port_set_default(&server_addr, 514);
	} else {
		LOG_ERR("Unknown address family");
		return false;
	}

	return ret;
}

#if defined(CONFIG_NET_HOSTNAME_ENABLE)
void log_backend_net_hostname_set(char *hostname, size_t len)
{
	(void)strncpy(dev_hostname, hostname, MIN(len, MAX_HOSTNAME_LEN));
	log_output_hostname_set(&log_output_net, dev_hostname);
}
#endif

void log_backend_net_start(void)
{
	const struct log_backend *backend = log_backend_net_get();

	if (!log_backend_is_active(backend)) {
		log_backend_activate(backend, backend->cb->ctx);
	}
}

static void init_net(struct log_backend const *const backend)
{
	ARG_UNUSED(backend);

	if (strlen(CONFIG_LOG_BACKEND_NET_SERVER) != 0) {
		const char *server = CONFIG_LOG_BACKEND_NET_SERVER;
		bool ret;

		if (memcmp(server, "tcp://", sizeof("tcp://") - 1) == 0) {
			server += sizeof("tcp://") - 1;
			ctx.is_tcp = true;
		}

		ret = log_backend_net_set_addr(server);
		if (!ret) {
			return;
		}
	}

	log_backend_deactivate(log_backend_net_get());
}

static void panic(struct log_backend const *const backend)
{
	panic_mode = true;
}

const struct log_backend_api log_backend_net_api = {
	.panic = panic,
	.init = init_net,
	.process = process,
	.format_set = format_set,
};

/* Note that the backend can be activated only after we have networking
 * subsystem ready so we must not start it immediately.
 */
LOG_BACKEND_DEFINE(log_backend_net, log_backend_net_api,
		   IS_ENABLED(CONFIG_LOG_BACKEND_NET_AUTOSTART));

const struct log_backend *log_backend_net_get(void)
{
	return &log_backend_net;
}
