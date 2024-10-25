/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/init.h>

#define SHELL_TELINK_W91_RX_BUF                 CONFIG_SHELL_BACKEND_TELINK_W91_RX_BUF_SIZE

K_MSGQ_DEFINE(backend_telink_w91_rx_queue, sizeof(char), SHELL_TELINK_W91_RX_BUF, 1);

struct backend_telink_w91_data {
	struct k_msgq * const queue;
	shell_transport_handler_t handler;
	void *ctx;
};

extern void telink_w91_debug_isr_set(bool enabled, void (*on_rx)(char c, void *ctx), void *ctx);
extern int arch_printk_char_out(int c);

static void backend_telink_w91_data_received(char c, void *ctx)
{
	struct backend_telink_w91_data *backend = (struct backend_telink_w91_data *)ctx;

	if (!k_msgq_put(backend->queue, &c, K_NO_WAIT)) {
		backend->handler(SHELL_TRANSPORT_EVT_RX_RDY, backend->ctx);
	}
}

static int backend_telink_w91_telink_w91_init(const struct shell_transport *transport,
	const void *config, shell_transport_handler_t evt_handler, void *context)
{
	ARG_UNUSED(config);

	struct backend_telink_w91_data *backend = (struct backend_telink_w91_data *)transport->ctx;

	backend->handler = evt_handler;
	backend->ctx = context;
	telink_w91_debug_isr_set(true, backend_telink_w91_data_received, backend);
	return 0;
}

static int backend_telink_w91_telink_w91_uninit(const struct shell_transport *transport)
{
	ARG_UNUSED(transport);

	telink_w91_debug_isr_set(false, NULL, NULL);
	return 0;
}

static int backend_telink_w91_telink_w91_enable(const struct shell_transport *transport,
	bool blocking)
{
	ARG_UNUSED(transport);
	ARG_UNUSED(blocking);

	return 0;
}

static int backend_telink_w91_telink_w91_write(const struct shell_transport *transport,
	const void *data, size_t length, size_t *cnt)
{
	struct backend_telink_w91_data *backend = (struct backend_telink_w91_data *)transport->ctx;

	for (size_t i = 0; i < length; ++i) {
		(void) arch_printk_char_out(((uint8_t *)data)[i]);
	}
	*cnt = length;
	backend->handler(SHELL_TRANSPORT_EVT_TX_RDY, backend->ctx);

	return 0;
}

static int backend_telink_w91_telink_w91_read(const struct shell_transport *transport,
	void *data, size_t length, size_t *cnt)
{
	struct backend_telink_w91_data *backend = (struct backend_telink_w91_data *)transport->ctx;

	*cnt = length;
	for (size_t i = 0; i < length; ++i) {
		if (k_msgq_get(backend->queue, &((char *)data)[i], K_NO_WAIT)) {
			*cnt = i;
			break;
		}
	}
	if (k_msgq_num_used_get(backend->queue)) {
		backend->handler(SHELL_TRANSPORT_EVT_RX_RDY, backend->ctx);
	}
	return 0;
}

static const struct shell_transport backend_telink_w91_transport = {
	.api = &(const struct shell_transport_api) {
		.init = backend_telink_w91_telink_w91_init,
		.uninit = backend_telink_w91_telink_w91_uninit,
		.enable = backend_telink_w91_telink_w91_enable,
		.write = backend_telink_w91_telink_w91_write,
		.read = backend_telink_w91_telink_w91_read
	},
	.ctx = &(struct backend_telink_w91_data) {
		.queue = &backend_telink_w91_rx_queue
	}
};

SHELL_DEFINE(shell_telink_w91, CONFIG_SHELL_PROMPT_TELINK_W91,
	&backend_telink_w91_transport, 0, 0, SHELL_FLAG_OLF_CRLF);

static int start_shell_telink_w91(void)
{
	bool log_backend = CONFIG_SHELL_TELINK_W91_INIT_LOG_LEVEL > 0;
	uint32_t level = (CONFIG_SHELL_TELINK_W91_INIT_LOG_LEVEL > LOG_LEVEL_DBG) ?
		CONFIG_LOG_MAX_LEVEL : CONFIG_SHELL_TELINK_W91_INIT_LOG_LEVEL;
	static const struct shell_backend_config_flags cfg_flags =
		SHELL_DEFAULT_BACKEND_CONFIG_FLAGS;

	shell_init(&shell_telink_w91, NULL, cfg_flags, log_backend, level);
	return 0;
}

SYS_INIT(start_shell_telink_w91, POST_KERNEL, 0);
