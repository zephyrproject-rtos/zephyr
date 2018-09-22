/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell_log_backend.h>
#include <shell/shell.h>
#include "shell_ops.h"
#include <logging/log_ctrl.h>

int shell_log_backend_output_func(u8_t *data, size_t length, void *ctx)
{
	shell_print_stream(ctx, data, length);
	return length;
}

void shell_log_backend_enable(const struct shell_log_backend *backend,
			      void *ctx, u32_t init_log_level)
{
	log_backend_enable(backend->backend, ctx, init_log_level);
	log_output_ctx_set(backend->log_output, ctx);
	backend->control_block->cnt = 0;
	backend->control_block->state = SHELL_LOG_BACKEND_ENABLED;
}

static struct log_msg *msg_from_fifo(const struct shell_log_backend *backend)
{
	struct log_msg *msg = k_fifo_get(backend->fifo, K_NO_WAIT);

	if (msg) {
		atomic_dec(&backend->control_block->cnt);
	}

	return msg;
}

static void fifo_flush(const struct shell_log_backend *backend)
{
	struct log_msg *msg = msg_from_fifo(backend);

	/* Flush log messages. */
	while (msg) {
		log_msg_put(msg);
		msg = msg_from_fifo(backend);
	}
}

static void msg_to_fifo(const struct shell *shell,
			struct log_msg *msg)
{
	atomic_val_t cnt;

	k_fifo_put(shell->log_backend->fifo, msg);

	cnt = atomic_inc(&shell->log_backend->control_block->cnt);

	/* If there is too much queued free the oldest one. */
	if (cnt >= CONFIG_SHELL_MAX_LOG_MSG_BUFFERED) {
		log_msg_put(msg_from_fifo(shell->log_backend));
		if (IS_ENABLED(CONFIG_SHELL_STATS)) {
			shell->stats->log_lost_cnt++;
		}
	}
}

void shell_log_backend_disable(const struct shell_log_backend *backend)
{
	fifo_flush(backend);
	log_backend_disable(backend->backend);
	backend->control_block->state = SHELL_LOG_BACKEND_DISABLED;
}

static void msg_process(const struct log_output *log_output,
			struct log_msg *msg)
{
	u32_t flags = 0;

	if (IS_ENABLED(CONFIG_SHELL)) {
		flags |= LOG_OUTPUT_FLAG_COLORS;
	}

	if (IS_ENABLED(CONFIG_SHELL)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	log_output_msg_process(log_output, msg, flags);
	log_msg_put(msg);
}

bool shell_log_backend_process(const struct shell_log_backend *backend)
{
	const struct shell *shell =
			(const struct shell *)backend->backend->cb->ctx;
	struct log_msg *msg = msg_from_fifo(backend);

	if (!msg) {
		return false;
	}

	msg_process(shell->log_backend->log_output, msg);

	return true;
}

static void put(const struct log_backend *const backend, struct log_msg *msg)
{
	const struct shell *shell = (const struct shell *)backend->cb->ctx;
	struct k_poll_signal *signal;

	log_msg_get(msg);

	switch (shell->log_backend->control_block->state) {
	case SHELL_LOG_BACKEND_ENABLED:
		msg_to_fifo(shell, msg);

		if (IS_ENABLED(CONFIG_MULTITHREADING)) {
			signal = &shell->ctx->signals[SHELL_SIGNAL_LOG_MSG];
			k_poll_signal(signal, 0);
		}

		break;

	case SHELL_LOG_BACKEND_PANIC:
		msg_process(shell->log_backend->log_output, msg);
		break;

	case SHELL_LOG_BACKEND_DISABLED:
		/* fall through */
		/* no break */
	default:
		/* Discard message. */
		log_msg_put(msg);
	}
}

static void panic(const struct log_backend *const backend)
{
	const struct shell *shell = (const struct shell *)backend->cb->ctx;
	int err;

	err = shell->iface->api->enable(shell->iface, true);

	if (err == 0) {
		shell->log_backend->control_block->state =
							SHELL_LOG_BACKEND_PANIC;

		/* Move to the start of next line. */
		shell_multiline_data_calc(&shell->ctx->vt100_ctx.cons,
						  shell->ctx->cmd_buff_pos,
						  shell->ctx->cmd_buff_len);
		shell_op_cursor_vert_move(shell, -1);
		shell_op_cursor_horiz_move(shell,
					   -shell->ctx->vt100_ctx.cons.cur_x);

		while (shell_log_backend_process(shell->log_backend)) {
			/* empty */
		}
	} else {
		shell_log_backend_disable(shell->log_backend);
	}
}

const struct log_backend_api log_backend_shell_api = {
	.put = put,
	.panic = panic
};
