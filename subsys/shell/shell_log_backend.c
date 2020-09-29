/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell_log_backend.h>
#include <shell/shell.h>
#include "shell_ops.h"
#include <logging/log_ctrl.h>

int shell_log_backend_output_func(uint8_t *data, size_t length, void *ctx)
{
	shell_print_stream(ctx, data, length);
	return length;
}

void shell_log_backend_enable(const struct shell_log_backend *backend,
			      void *ctx, uint32_t init_log_level)
{
	int err = 0;

	if (IS_ENABLED(CONFIG_LOG_IMMEDIATE)) {
		const struct shell *shell;

		shell = (const struct shell *)ctx;

		/* Reenable transport in blocking mode */
		err = shell->iface->api->enable(shell->iface, true);
	}

	if (err == 0) {
		log_backend_enable(backend->backend, ctx, init_log_level);
		log_output_ctx_set(backend->log_output, ctx);
	backend->control_block->dropped_cnt = 0;
		backend->control_block->state = SHELL_LOG_BACKEND_ENABLED;
	}
}

static struct log_msg *msg_from_fifo(const struct shell_log_backend *backend)
{
	struct shell_log_backend_msg msg;
	int err;

	err = k_msgq_get(backend->msgq, &msg, K_NO_WAIT);

	return (err == 0) ? msg.msg : NULL;
}

static void fifo_flush(const struct shell_log_backend *backend)
{
	struct log_msg *msg;

	/* Flush log messages. */
	while ((msg = msg_from_fifo(backend)) != NULL) {
		log_msg_put(msg);
	}
}

static void flush_expired_messages(const struct shell *shell)
{
	int err;
	struct shell_log_backend_msg msg;
	struct k_msgq *msgq = shell->log_backend->msgq;
	uint32_t timeout = shell->log_backend->timeout;
	uint32_t now = k_uptime_get_32();

	while (1) {
		err = k_msgq_peek(msgq, &msg);

		if (err == 0 && ((now - msg.timestamp) > timeout)) {
			(void)k_msgq_get(msgq, &msg, K_NO_WAIT);
			log_msg_put(msg.msg);

			if (IS_ENABLED(CONFIG_SHELL_STATS)) {
				atomic_inc(&shell->stats->log_lost_cnt);
			}
		} else {
			break;
		}
	}
}

static void msg_to_fifo(const struct shell *shell,
			struct log_msg *msg)
{
	int err;
	struct shell_log_backend_msg t_msg = {
		.msg = msg,
		.timestamp = k_uptime_get_32()
	};

	err = k_msgq_put(shell->log_backend->msgq, &t_msg,
			 K_MSEC(shell->log_backend->timeout));

	switch (err) {
	case 0:
		break;
	case -EAGAIN:
	case -ENOMSG:
	{
		flush_expired_messages(shell);

		err = k_msgq_put(shell->log_backend->msgq, &msg, K_NO_WAIT);
		if (err) {
			/* Unexpected case as we just freed one element and
			 * there is no other context that puts into the msgq.
			 */
			__ASSERT_NO_MSG(0);
		}
		break;
	}
	default:
		/* Other errors are not expected. */
		__ASSERT_NO_MSG(0);
		break;
	}
}

void shell_log_backend_disable(const struct shell_log_backend *backend)
{
	fifo_flush(backend);
	log_backend_disable(backend->backend);
	backend->control_block->state = SHELL_LOG_BACKEND_DISABLED;
}

static void msg_process(const struct log_output *log_output,
			struct log_msg *msg, bool colors)
{
	uint32_t flags = LOG_OUTPUT_FLAG_LEVEL |
		      LOG_OUTPUT_FLAG_TIMESTAMP |
		      LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;

	if (colors) {
		flags |= LOG_OUTPUT_FLAG_COLORS;
	}

	log_output_msg_process(log_output, msg, flags);
	log_msg_put(msg);
}

bool shell_log_backend_process(const struct shell_log_backend *backend)
{
	uint32_t dropped;
	const struct shell *shell =
			(const struct shell *)backend->backend->cb->ctx;
	bool colors = IS_ENABLED(CONFIG_SHELL_VT100_COLORS) &&
			shell->ctx->internal.flags.use_colors;
	struct log_msg *msg = msg_from_fifo(backend);

	if (!msg) {
		return false;
	}

	dropped = atomic_set(&backend->control_block->dropped_cnt, 0);
	if (dropped) {
		struct shell_vt100_colors col;

		if (colors) {
			shell_vt100_colors_store(shell, &col);
			shell_vt100_color_set(shell, SHELL_VT100_COLOR_RED);
		}

		log_output_dropped_process(backend->log_output, dropped);

		if (colors) {
			shell_vt100_colors_restore(shell, &col);
		}
	}

	msg_process(shell->log_backend->log_output, msg, colors);

	return true;
}

static void put(const struct log_backend *const backend, struct log_msg *msg)
{
	const struct shell *shell = (const struct shell *)backend->cb->ctx;
	bool colors = IS_ENABLED(CONFIG_SHELL_VT100_COLORS) &&
			shell->ctx->internal.flags.use_colors;
	struct k_poll_signal *signal;

	log_msg_get(msg);

	switch (shell->log_backend->control_block->state) {
	case SHELL_LOG_BACKEND_ENABLED:
		msg_to_fifo(shell, msg);

		if (IS_ENABLED(CONFIG_MULTITHREADING)) {
			signal = &shell->ctx->signals[SHELL_SIGNAL_LOG_MSG];
			k_poll_signal_raise(signal, 0);
		}

		break;
	case SHELL_LOG_BACKEND_PANIC:
		shell_cmd_line_erase(shell);
		msg_process(shell->log_backend->log_output, msg, colors);

		break;

	case SHELL_LOG_BACKEND_DISABLED:
		__fallthrough;
	default:
		/* Discard message. */
		log_msg_put(msg);
	}
}

static void put_sync_string(const struct log_backend *const backend,
			    struct log_msg_ids src_level, uint32_t timestamp,
			    const char *fmt, va_list ap)
{
	const struct shell *shell = (const struct shell *)backend->cb->ctx;
	uint32_t key;
	uint32_t flags = LOG_OUTPUT_FLAG_LEVEL |
		      LOG_OUTPUT_FLAG_TIMESTAMP |
		      LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;

	if (IS_ENABLED(CONFIG_SHELL_VT100_COLORS)) {
		flags |= LOG_OUTPUT_FLAG_COLORS;
	}

	key = irq_lock();
	if (!flag_cmd_ctx_get(shell)) {
		shell_cmd_line_erase(shell);
	}
	log_output_string(shell->log_backend->log_output, src_level, timestamp,
			  fmt, ap, flags);
	if (!flag_cmd_ctx_get(shell)) {
		shell_print_prompt_and_cmd(shell);
	}
	irq_unlock(key);
}

static void put_sync_hexdump(const struct log_backend *const backend,
			 struct log_msg_ids src_level, uint32_t timestamp,
			 const char *metadata, const uint8_t *data, uint32_t length)
{
	const struct shell *shell = (const struct shell *)backend->cb->ctx;
	uint32_t key;
	uint32_t flags = LOG_OUTPUT_FLAG_LEVEL |
		      LOG_OUTPUT_FLAG_TIMESTAMP |
		      LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;

	if (IS_ENABLED(CONFIG_SHELL_VT100_COLORS)) {
		flags |= LOG_OUTPUT_FLAG_COLORS;
	}

	key = irq_lock();
	if (!flag_cmd_ctx_get(shell)) {
		shell_cmd_line_erase(shell);
	}
	log_output_hexdump(shell->log_backend->log_output, src_level, timestamp,
			   metadata, data, length, flags);
	if (!flag_cmd_ctx_get(shell)) {
		shell_print_prompt_and_cmd(shell);
	}
	irq_unlock(key);
}

static void panic(const struct log_backend *const backend)
{
	const struct shell *shell = (const struct shell *)backend->cb->ctx;
	int err;

	if (IS_ENABLED(CONFIG_LOG_IMMEDIATE)) {
		return;
	}

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

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	const struct shell *shell = (const struct shell *)backend->cb->ctx;
	const struct shell_log_backend *log_backend = shell->log_backend;

	atomic_add(&shell->stats->log_lost_cnt, cnt);
	atomic_add(&log_backend->control_block->dropped_cnt, cnt);
}

const struct log_backend_api log_backend_shell_api = {
	.put = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : put,
	.put_sync_string = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			put_sync_string : NULL,
	.put_sync_hexdump = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			put_sync_hexdump : NULL,
	.dropped = dropped,
	.panic = panic,
};
