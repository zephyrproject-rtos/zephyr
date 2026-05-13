/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(shell_ssh, CONFIG_SHELL_SSH_LOG_LEVEL);

#include <mbedtls/platform_util.h>

#include <zephyr/kernel.h>
#include <zephyr/net/ssh/server.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_ssh.h>

#if defined(CONFIG_SSH_SERVER)

static K_MUTEX_DEFINE(ssh_lock);

#define SHELL_SSH_NAME(n, _) &shell_ssh_##n

#define SHELL_SSH_DEFINE_ALL(n, _)					\
	SHELL_SSH_DEFINE(shell_transport_ssh_##n);			\
	SHELL_DEFINE(shell_ssh_##n,					\
		     CONFIG_SHELL_PROMPT_SSH,				\
		     &shell_transport_ssh_##n,				\
		     CONFIG_SHELL_BACKEND_SERIAL_LOG_MESSAGE_QUEUE_SIZE, \
		     CONFIG_SHELL_BACKEND_SERIAL_LOG_MESSAGE_QUEUE_TIMEOUT, \
		     SHELL_FLAG_OLF_CRLF)

LISTIFY(CONFIG_SSH_SERVER_SHELL_COUNT, SHELL_SSH_DEFINE_ALL, (;));

static const struct shell *const ssh_shell_instances[] = {
	LISTIFY(CONFIG_SSH_SERVER_SHELL_COUNT, SHELL_SSH_NAME, (,), shell_ssh)
};

static struct shell_ssh_context ssh_server_contexts[CONFIG_SSH_SERVER_SHELL_COUNT];

static struct shell_ssh_context *ssh_server_context_alloc(void)
{
	k_mutex_lock(&ssh_lock, K_FOREVER);

	for (size_t i = 0; i < ARRAY_SIZE(ssh_server_contexts); i++) {
		if (!ssh_server_contexts[i].in_use) {
			memset(&ssh_server_contexts[i], 0,
			       sizeof(ssh_server_contexts[i]));
			ssh_server_contexts[i].in_use = true;
			k_mutex_unlock(&ssh_lock);
			return &ssh_server_contexts[i];
		}
	}

	k_mutex_unlock(&ssh_lock);

	return NULL;
}

static void ssh_server_context_free(struct shell_ssh_context *ctx)
{
	k_mutex_lock(&ssh_lock, K_FOREVER);
	ctx->in_use = false;
	k_mutex_unlock(&ssh_lock);
}

static const struct shell *ssh_server_shell_instance_alloc(struct shell_ssh_context *ctx)
{
	k_mutex_lock(&ssh_lock, K_FOREVER);

	for (size_t i = 0; i < ARRAY_SIZE(ssh_shell_instances); i++) {
		const struct shell *sh = ssh_shell_instances[i];
		struct shell_ssh *sh_ssh = sh->iface->ctx;

		if (!sh_ssh->in_use) {
			sh_ssh->in_use = true;
			ctx->ssh = sh_ssh;
			k_mutex_unlock(&ssh_lock);
			return sh;
		}
	}

	k_mutex_unlock(&ssh_lock);

	return NULL;
}

static void ssh_server_shell_instance_free(const struct shell *sh)
{
	struct shell_ssh *sh_ssh;

	k_mutex_lock(&ssh_lock, K_FOREVER);

	sh_ssh = sh->iface->ctx;
	sh_ssh->in_use = false;

	k_mutex_unlock(&ssh_lock);
}
#endif

static int shell_ssh_init(const struct shell_transport *transport,
			  const void *config,
			  shell_transport_handler_t evt_handler,
			  void *context)
{
	struct shell_ssh *sh_ssh = (struct shell_ssh *)transport->ctx;

	sh_ssh->handler = evt_handler;
	sh_ssh->context = context;
	sh_ssh->ssh_channel = (void *)config;

	return 0;
}

static int shell_ssh_uninit(const struct shell_transport *transport)
{
	ARG_UNUSED(transport);
	return 0;
}

static int shell_ssh_enable(const struct shell_transport *transport, bool blocking)
{
	ARG_UNUSED(transport);
	ARG_UNUSED(blocking);
	return 0;
}

static int shell_ssh_write(const struct shell_transport *transport,
			   const void *data, size_t length, size_t *cnt)
{
	struct shell_ssh *sh_ssh = (struct shell_ssh *)transport->ctx;
	int ret;

	ret = ssh_channel_write(sh_ssh->ssh_channel, data, length);
	if (ret < 0) {
		return ret;
	}

	*cnt = ret;

	return 0;
}

static int shell_ssh_read(const struct shell_transport *transport,
			  void *data, size_t length, size_t *cnt)
{
	struct shell_ssh *sh_ssh = (struct shell_ssh *)transport->ctx;
	int ret;

	ret = ssh_channel_read(sh_ssh->ssh_channel, data, length);
	if (ret < 0) {
		return ret;
	}

	*cnt = ret;
	return 0;
}

const struct shell_transport_api shell_ssh_transport_api = {
	.init = shell_ssh_init,
	.uninit = shell_ssh_uninit,
	.enable = shell_ssh_enable,
	.write = shell_ssh_write,
	.read = shell_ssh_read
};

#if defined(CONFIG_SSH_SERVER)
int shell_sshd_event_callback(struct ssh_server *sshd,
			      const struct ssh_server_event *event,
			      void *user_data)
{
	ARG_UNUSED(sshd);
	ARG_UNUSED(user_data);

	switch (event->type) {
	case SSH_SERVER_EVENT_CLOSED:
		LOG_INF("Server closed");
		break;
	case SSH_SERVER_EVENT_CLIENT_CONNECTED:
		LOG_INF("Server client connected");
		break;
	case SSH_SERVER_EVENT_CLIENT_DISCONNECTED:
		LOG_INF("Server client disconnected");
		break;
	default:
		return -1;
	}

	return 0;
}

static int sshd_channel_event_callback(struct ssh_channel *channel,
				       const struct ssh_channel_event *event,
				       void *user_data);

int shell_sshd_transport_event_callback(struct ssh_transport *transport,
					const struct ssh_transport_event *event,
					void *user_data)
{
	ARG_UNUSED(transport);
	ARG_UNUSED(user_data);

	switch (event->type) {
	case SSH_TRANSPORT_EVENT_CHANNEL_OPEN: {
		struct shell_ssh_context *ctx;

		ctx = ssh_server_context_alloc();
		if (ctx == NULL) {
			LOG_ERR("No SSH server contexts available");
			return ssh_channel_open_result(
				event->channel_open.channel, false,
				NULL, NULL);
		}

		return ssh_channel_open_result(event->channel_open.channel, true,
					       sshd_channel_event_callback,
					       ctx);
	}
	default:
		break;
	}

	return 0;
}

static int shell_ssh_setup(struct shell_ssh_context *ctx, struct ssh_channel *channel)
{
	static const struct shell_backend_config_flags cfg_flags =
		SHELL_DEFAULT_BACKEND_CONFIG_FLAGS;
	const struct shell *sh;
	int ret;

	sh = ssh_server_shell_instance_alloc(ctx);
	if (sh != NULL) {
		ret = shell_init(sh, channel, cfg_flags, false, LOG_LEVEL_NONE);
		if (ret == 0) {
			ctx->sh = sh;
		} else {
			LOG_DBG("Cannot init ssh shell (%d)", ret);
			ssh_server_shell_instance_free(sh);
		}
	} else {
		LOG_ERR("No SSH server shell instances remaining");
		ret = -ENOMEM;
	}

	return ret;
}

static int sshd_channel_event_callback(struct ssh_channel *channel,
				       const struct ssh_channel_event *event,
				       void *user_data)
{
	struct shell_ssh_context *ctx = user_data;

	switch (event->type) {
	case SSH_CHANNEL_EVENT_REQUEST: {
		bool success = false;

		LOG_DBG("Server channel request (%d)", event->channel_request.type);

		switch (event->channel_request.type) {
		case SSH_CHANNEL_REQUEST_SHELL: {
			int ret;

			ret = shell_ssh_setup(ctx, channel);
			if (ret != 0) {
				LOG_ERR("shell_init error (%d)", ret);
			} else {
				success = true;
			}

			break;
		}

		case SSH_CHANNEL_REQUEST_PSEUDO_TERMINAL:
			LOG_DBG("PTY request received, ignoring");
			success = true;
			break;
		case SSH_CHANNEL_REQUEST_ENV_VAR:
			LOG_DBG("Environment variable request received, ignoring");
			success = true;
			break;
		case SSH_CHANNEL_REQUEST_WINDOW_CHANGE:
			LOG_DBG("Window change request received, ignoring");
			success = true;
			break;

		default:
			break;
		}

		if (event->channel_request.want_reply) {
			return ssh_channel_request_result(channel, success);
		}

		return 0;
	}

	case SSH_CHANNEL_EVENT_RX_DATA_READY: {
		struct shell_ssh *sh_ssh = ctx->ssh;

		if (sh_ssh != NULL && sh_ssh->handler != NULL) {
			sh_ssh->handler(SHELL_TRANSPORT_EVT_RX_RDY, sh_ssh->context);
		}
		break;
	}

	case SSH_CHANNEL_EVENT_TX_DATA_READY: {
		struct shell_ssh *sh_ssh = ctx->ssh;

		if (sh_ssh != NULL && sh_ssh->handler != NULL) {
			sh_ssh->handler(SHELL_TRANSPORT_EVT_TX_RDY, sh_ssh->context);
		}
		break;
	}

	case SSH_CHANNEL_EVENT_RX_STDERR_DATA_READY:
		LOG_INF("Server channel RX ext data ready");
		break;

	case SSH_CHANNEL_EVENT_EOF:
		LOG_INF("Server channel EOF");
		break;

	case SSH_CHANNEL_EVENT_CLOSED:
		LOG_INF("Server channel closed");
		if (ctx->sh != NULL) {
			shell_uninit(ctx->sh, NULL);
			ssh_server_shell_instance_free(ctx->sh);
			ctx->sh = NULL;
			ctx->ssh = NULL;
		}
		ssh_server_context_free(ctx);
		break;

	default:
		return -1;
	}

	return 0;
}
#endif /* CONFIG_SSH_SERVER */
