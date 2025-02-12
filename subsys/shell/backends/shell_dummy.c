/*
 * Shell backend used for testing
 *
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell_dummy.h>
#include <zephyr/init.h>

SHELL_DUMMY_DEFINE(shell_transport_dummy);
SHELL_DEFINE(shell_dummy, CONFIG_SHELL_PROMPT_DUMMY, &shell_transport_dummy, 256,
	     0, SHELL_FLAG_OLF_CRLF);

static int init(const struct shell_transport *transport,
		const void *config,
		shell_transport_handler_t evt_handler,
		void *context)
{
	struct shell_dummy *sh_dummy = (struct shell_dummy *)transport->ctx;

	if (sh_dummy->initialized) {
		return -EINVAL;
	}

	sh_dummy->initialized = true;

	return 0;
}

static int uninit(const struct shell_transport *transport)
{
	struct shell_dummy *sh_dummy = (struct shell_dummy *)transport->ctx;

	if (!sh_dummy->initialized) {
		return -ENODEV;
	}

	sh_dummy->initialized = false;

	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking)
{
	struct shell_dummy *sh_dummy = (struct shell_dummy *)transport->ctx;

	if (!sh_dummy->initialized) {
		return -ENODEV;
	}

	return 0;
}

static int write(const struct shell_transport *transport,
		 const void *data, size_t length, size_t *cnt)
{
	struct shell_dummy *sh_dummy = (struct shell_dummy *)transport->ctx;
	size_t store_cnt;

	if (!sh_dummy->initialized) {
		*cnt = 0;
		return -ENODEV;
	}

	store_cnt = length;
	if (sh_dummy->len + store_cnt >= sizeof(sh_dummy->buf)) {
		store_cnt = sizeof(sh_dummy->buf) - sh_dummy->len - 1;
	}
	memcpy(sh_dummy->buf + sh_dummy->len, data, store_cnt);
	sh_dummy->len += store_cnt;

	*cnt = length;

	return 0;
}

static int read(const struct shell_transport *transport,
		void *data, size_t length, size_t *cnt)
{
	struct shell_dummy *sh_dummy = (struct shell_dummy *)transport->ctx;

	if (!sh_dummy->initialized) {
		return -ENODEV;
	}

	*cnt = 0;

	return 0;
}

const struct shell_transport_api shell_dummy_transport_api = {
	.init = init,
	.uninit = uninit,
	.enable = enable,
	.write = write,
	.read = read
};

static int enable_shell_dummy(void)
{
	bool log_backend = CONFIG_SHELL_DUMMY_INIT_LOG_LEVEL > 0;
	uint32_t level = (CONFIG_SHELL_DUMMY_INIT_LOG_LEVEL > LOG_LEVEL_DBG) ?
		      CONFIG_LOG_MAX_LEVEL : CONFIG_SHELL_DUMMY_INIT_LOG_LEVEL;
	static const struct shell_backend_config_flags cfg_flags =
					SHELL_DEFAULT_BACKEND_CONFIG_FLAGS;

	shell_init(&shell_dummy, NULL, cfg_flags, log_backend, level);

	return 0;
}
SYS_INIT(enable_shell_dummy, POST_KERNEL, 0);

const struct shell *shell_backend_dummy_get_ptr(void)
{
	return &shell_dummy;
}

const char *shell_backend_dummy_get_output(const struct shell *sh,
					   size_t *sizep)
{
	struct shell_dummy *sh_dummy = (struct shell_dummy *)sh->iface->ctx;

	sh_dummy->buf[sh_dummy->len] = '\0';
	*sizep = sh_dummy->len;
	sh_dummy->len = 0;

	return sh_dummy->buf;
}

void shell_backend_dummy_clear_output(const struct shell *sh)
{
	struct shell_dummy *sh_dummy = (struct shell_dummy *)sh->iface->ctx;

	sh_dummy->buf[0] = '\0';
	sh_dummy->len = 0;
}
