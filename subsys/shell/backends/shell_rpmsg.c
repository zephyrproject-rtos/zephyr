/*
 * Copyright (c) 2024 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell_rpmsg.h>

SHELL_RPMSG_DEFINE(shell_transport_rpmsg);
SHELL_DEFINE(shell_rpmsg, CONFIG_SHELL_PROMPT_RPMSG, &shell_transport_rpmsg,
	     CONFIG_SHELL_BACKEND_RPMSG_LOG_MESSAGE_QUEUE_SIZE,
	     CONFIG_SHELL_BACKEND_RPMSG_LOG_MESSAGE_QUEUE_TIMEOUT, SHELL_FLAG_OLF_CRLF);

static int rpmsg_shell_cb(struct rpmsg_endpoint *ept, void *data,
			  size_t len, uint32_t src, void *priv)
{
	const struct shell_transport *transport = (const struct shell_transport *)priv;
	struct shell_rpmsg *sh_rpmsg = (struct shell_rpmsg *)transport->ctx;
	struct shell_rpmsg_rx rx;

	if (len == 0) {
		return RPMSG_ERR_NO_BUFF;
	}

	rx.data = data;
	rx.len = len;
	if (k_msgq_put(&sh_rpmsg->rx_q, &rx, K_NO_WAIT) != 0) {
		return RPMSG_ERR_NO_MEM;
	}

	rpmsg_hold_rx_buffer(ept, data);
	sh_rpmsg->shell_handler(SHELL_TRANSPORT_EVT_RX_RDY, sh_rpmsg->shell_context);

	return RPMSG_SUCCESS;
}

static int uninit(const struct shell_transport *transport)
{
	struct shell_rpmsg *sh_rpmsg = (struct shell_rpmsg *)transport->ctx;

	if (!sh_rpmsg->ready) {
		return -ENODEV;
	}

	rpmsg_destroy_ept(&sh_rpmsg->ept);
	sh_rpmsg->ready = false;

	return 0;
}

static int init(const struct shell_transport *transport,
		const void *config,
		shell_transport_handler_t evt_handler,
		void *context)
{
	struct shell_rpmsg *sh_rpmsg = (struct shell_rpmsg *)transport->ctx;
	struct rpmsg_device *rdev;
	int ret;

	if (sh_rpmsg->ready) {
		return -EALREADY;
	}

	if (config == NULL) {
		return -EINVAL;
	}
	rdev = (struct rpmsg_device *)config;

	k_msgq_init(&sh_rpmsg->rx_q, (char *)sh_rpmsg->rx_buf, sizeof(struct shell_rpmsg_rx),
		    CONFIG_SHELL_RPMSG_MAX_RX);

	ret = rpmsg_create_ept(&sh_rpmsg->ept, rdev, CONFIG_SHELL_RPMSG_SERVICE_NAME,
			       CONFIG_SHELL_RPMSG_SRC_ADDR, CONFIG_SHELL_RPMSG_DST_ADDR,
			       rpmsg_shell_cb, NULL);
	if (ret < 0) {
		return ret;
	}

	sh_rpmsg->ept.priv = (void *)transport;

	sh_rpmsg->shell_handler = evt_handler;
	sh_rpmsg->shell_context = context;
	sh_rpmsg->ready = true;

	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking)
{
	struct shell_rpmsg *sh_rpmsg = (struct shell_rpmsg *)transport->ctx;

	if (!sh_rpmsg->ready) {
		return -ENODEV;
	}

	sh_rpmsg->blocking = blocking;

	return 0;
}

static int write(const struct shell_transport *transport,
		 const void *data, size_t length, size_t *cnt)
{
	struct shell_rpmsg *sh_rpmsg = (struct shell_rpmsg *)transport->ctx;
	int ret;

	*cnt = 0;

	if (!sh_rpmsg->ready) {
		return -ENODEV;
	}

	if (sh_rpmsg->blocking) {
		ret = rpmsg_send(&sh_rpmsg->ept, data, (int)length);
	} else {
		ret = rpmsg_trysend(&sh_rpmsg->ept, data, (int)length);
	}

	/* Set TX ready in any case, as we have no way to recover otherwise */
	sh_rpmsg->shell_handler(SHELL_TRANSPORT_EVT_TX_RDY, sh_rpmsg->shell_context);

	if (ret < 0) {
		return ret;
	}

	*cnt = (size_t)ret;

	return 0;
}

static int read(const struct shell_transport *transport,
		void *data, size_t length, size_t *cnt)
{
	struct shell_rpmsg *sh_rpmsg = (struct shell_rpmsg *)transport->ctx;
	struct shell_rpmsg_rx *rx = &sh_rpmsg->rx_cur;
	size_t read_len;
	bool release = true;

	if (!sh_rpmsg->ready) {
		return -ENODEV;
	}

	/* Check if we still have pending data */
	if (rx->data == NULL) {
		int ret = k_msgq_get(&sh_rpmsg->rx_q, rx, K_NO_WAIT);

		if (ret < 0) {
			rx->data = NULL;
			goto no_data;
		}

		__ASSERT_NO_MSG(rx->len > 0);
		sh_rpmsg->rx_consumed = 0;
	}

	__ASSERT_NO_MSG(rx->len > sh_rpmsg->rx_consumed);
	read_len = rx->len - sh_rpmsg->rx_consumed;
	if (read_len > length) {
		read_len = length;
		release = false;
	}

	*cnt = read_len;
	memcpy(data, &((char *)rx->data)[sh_rpmsg->rx_consumed], read_len);

	if (release) {
		rpmsg_release_rx_buffer(&sh_rpmsg->ept, rx->data);
		rx->data = NULL;
	} else {
		sh_rpmsg->rx_consumed += read_len;
	}

	return 0;

no_data:
	*cnt = 0;
	return 0;
}

const struct shell_transport_api shell_rpmsg_transport_api = {
	.init = init,
	.uninit = uninit,
	.enable = enable,
	.read = read,
	.write = write,
};

int shell_backend_rpmsg_init_transport(struct rpmsg_device *rpmsg_dev)
{
	bool log_backend = CONFIG_SHELL_RPMSG_INIT_LOG_LEVEL > 0;
	uint32_t level = (CONFIG_SHELL_RPMSG_INIT_LOG_LEVEL > LOG_LEVEL_DBG) ?
		      CONFIG_LOG_MAX_LEVEL : CONFIG_SHELL_RPMSG_INIT_LOG_LEVEL;
	static const struct shell_backend_config_flags cfg_flags =
					SHELL_DEFAULT_BACKEND_CONFIG_FLAGS;

	return shell_init(&shell_rpmsg, rpmsg_dev, cfg_flags, log_backend, level);
}

const struct shell *shell_backend_rpmsg_get_ptr(void)
{
	return &shell_rpmsg;
}
