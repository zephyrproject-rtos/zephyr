/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell_adsp_memory_window.h>
#include <zephyr/sys/winstream.h>
#include <zephyr/logging/log.h>

#include <adsp_memory.h>
#include <adsp_debug_window.h>

SHELL_ADSP_MEMORY_WINDOW_DEFINE(shell_transport_adsp_memory_window);
SHELL_DEFINE(shell_adsp_memory_window, CONFIG_SHELL_BACKEND_ADSP_MEMORY_WINDOW_PROMPT,
	     &shell_transport_adsp_memory_window, 256, 0, SHELL_FLAG_OLF_CRLF);

LOG_MODULE_REGISTER(shell_adsp_memory_window);

#define RX_WINDOW_SIZE		256
#define POLL_INTERVAL		K_MSEC(CONFIG_SHELL_BACKEND_ADSP_MEMORY_WINDOW_POLL_INTERVAL)

BUILD_ASSERT(RX_WINDOW_SIZE < ADSP_DW_SLOT_SIZE);

struct adsp_debug_slot_shell {
	uint8_t rx_window[RX_WINDOW_SIZE];
	uint8_t tx_window[ADSP_DW_SLOT_SIZE - RX_WINDOW_SIZE];
} __packed;

static void timer_handler(struct k_timer *timer)
{
	const struct shell_adsp_memory_window *sh_adsp_mw = k_timer_user_data_get(timer);

	__ASSERT_NO_MSG(sh_adsp_mw->shell_handler && sh_adsp_mw->shell_context);

	sh_adsp_mw->shell_handler(SHELL_TRANSPORT_EVT_RX_RDY,
				    sh_adsp_mw->shell_context);
}

static int init(const struct shell_transport *transport,
		const void *config,
		shell_transport_handler_t evt_handler,
		void *context)
{
	struct shell_adsp_memory_window *sh_adsp_mw =
		(struct shell_adsp_memory_window *)transport->ctx;
	struct adsp_debug_slot_shell *dw_slot;

	if (ADSP_DW->descs[ADSP_DW_SLOT_NUM_SHELL].type &&
	    (ADSP_DW->descs[ADSP_DW_SLOT_NUM_SHELL].type !=
		    ADSP_DW_SLOT_SHELL)) {
		LOG_WRN("Possible conflict with debug window slot for shell, key %#x\n",
			ADSP_DW->descs[ADSP_DW_SLOT_NUM_SHELL].type);
	}

	ADSP_DW->descs[ADSP_DW_SLOT_NUM_SHELL].type = ADSP_DW->descs[ADSP_DW_SLOT_NUM_SHELL].type;

	sh_adsp_mw->shell_handler = evt_handler;
	sh_adsp_mw->shell_context = context;

	dw_slot = (struct adsp_debug_slot_shell *)ADSP_DW->slots[ADSP_DW_SLOT_NUM_SHELL];

	sh_adsp_mw->ws_rx = sys_winstream_init(&dw_slot->rx_window[0], sizeof(dw_slot->rx_window));
	sh_adsp_mw->ws_tx = sys_winstream_init(&dw_slot->tx_window[0], sizeof(dw_slot->tx_window));

	LOG_DBG("shell with ADSP debug window rx/tx %u/%u\n",
		sizeof(dw_slot->rx_window), sizeof(dw_slot->tx_window));

	k_timer_init(&sh_adsp_mw->timer, timer_handler, NULL);
	k_timer_user_data_set(&sh_adsp_mw->timer, (void *)sh_adsp_mw);
	k_timer_start(&sh_adsp_mw->timer, POLL_INTERVAL, POLL_INTERVAL);

	return 0;
}

static int uninit(const struct shell_transport *transport)
{
	struct shell_adsp_memory_window *sh_adsp_mw =
		(struct shell_adsp_memory_window *)transport->ctx;

	k_timer_stop(&sh_adsp_mw->timer);

	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking)
{
	return 0;
}

static int write(const struct shell_transport *transport,
		 const void *data, size_t length, size_t *cnt)
{
	struct shell_adsp_memory_window *sh_adsp_mw =
		(struct shell_adsp_memory_window *)transport->ctx;

	sys_winstream_write(sh_adsp_mw->ws_tx, data, length);
	*cnt = length;

	return 0;
}

static int read(const struct shell_transport *transport,
		void *data, size_t length, size_t *cnt)
{
	struct shell_adsp_memory_window *sh_adsp_mw =
		(struct shell_adsp_memory_window *)transport->ctx;
	uint32_t res;

	res = sys_winstream_read(sh_adsp_mw->ws_rx, &sh_adsp_mw->read_seqno, data, length);
	*cnt = res;

	return 0;
}

const struct shell_transport_api shell_adsp_memory_window_transport_api = {
	.init = init,
	.uninit = uninit,
	.enable = enable,
	.write = write,
	.read = read
};

static int enable_shell_adsp_memory_window(void)
{
	bool log_backend = true;
	uint32_t level = CONFIG_LOG_MAX_LEVEL;
	static const struct shell_backend_config_flags cfg_flags =
		SHELL_DEFAULT_BACKEND_CONFIG_FLAGS;

	shell_init(&shell_adsp_memory_window, NULL, cfg_flags, log_backend, level);

	return 0;
}
SYS_INIT(enable_shell_adsp_memory_window, POST_KERNEL, 0);

const struct shell *shell_backend_adsp_memory_window_get_ptr(void)
{
	return &shell_adsp_memory_window;
}
