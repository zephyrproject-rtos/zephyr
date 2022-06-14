/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell_log_backend.h>
#include <zephyr/shell/shell.h>
#include "shell_ops.h"
#include <zephyr/logging/log_ctrl.h>

static bool process_msg2_from_buffer(const struct shell *shell);

int z_shell_log_backend_output_func(uint8_t *data, size_t length, void *ctx)
{
	z_shell_print_stream(ctx, data, length);
	return length;
}

/* Set fifo clean state (in case of deferred mode). */
static void fifo_reset(const struct shell_log_backend *backend)
{
	mpsc_pbuf_init(backend->mpsc_buffer, backend->mpsc_buffer_config);
}

void z_shell_log_backend_enable(const struct shell_log_backend *backend,
				void *ctx, uint32_t init_log_level)
{
	int err = 0;

	if (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
		const struct shell *shell;

		shell = (const struct shell *)ctx;

		z_flag_sync_mode_set(shell, true);
		/* Reenable transport in blocking mode */
		err = shell->iface->api->enable(shell->iface, true);
	}

	if (err == 0) {
		fifo_reset(backend);
		log_backend_enable(backend->backend, ctx, init_log_level);
		log_output_ctx_set(backend->log_output, ctx);
		backend->control_block->dropped_cnt = 0;
		backend->control_block->state = SHELL_LOG_BACKEND_ENABLED;
	}
}

void z_shell_log_backend_disable(const struct shell_log_backend *backend)
{
	log_backend_disable(backend->backend);
	backend->control_block->state = SHELL_LOG_BACKEND_DISABLED;
}

bool z_shell_log_backend_process(const struct shell_log_backend *backend)
{
	const struct shell *shell =
			(const struct shell *)backend->backend->cb->ctx;
	uint32_t dropped;
	bool colors = IS_ENABLED(CONFIG_SHELL_VT100_COLORS) &&
			z_flag_use_colors_get(shell);

	dropped = atomic_set(&backend->control_block->dropped_cnt, 0);
	if (dropped) {
		struct shell_vt100_colors col;

		if (colors) {
			z_shell_vt100_colors_store(shell, &col);
			z_shell_vt100_color_set(shell, SHELL_VT100_COLOR_RED);
		}

		log_output_dropped_process(backend->log_output, dropped);

		if (colors) {
			z_shell_vt100_colors_restore(shell, &col);
		}
	}

	return process_msg2_from_buffer(shell);
}

static void panic(const struct log_backend *const backend)
{
	const struct shell *shell = (const struct shell *)backend->cb->ctx;
	int err;

	if (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
		return;
	}

	err = shell->iface->api->enable(shell->iface, true);

	if (err == 0) {
		shell->log_backend->control_block->state =
						SHELL_LOG_BACKEND_PANIC;
		z_flag_sync_mode_set(shell, true);

		/* Move to the start of next line. */
		z_shell_multiline_data_calc(&shell->ctx->vt100_ctx.cons,
					    shell->ctx->cmd_buff_pos,
					    shell->ctx->cmd_buff_len);
		z_shell_op_cursor_vert_move(shell, -1);
		z_shell_op_cursor_horiz_move(shell,
					   -shell->ctx->vt100_ctx.cons.cur_x);

		while (process_msg2_from_buffer(shell)) {
			/* empty */
		}
	} else {
		z_shell_log_backend_disable(shell->log_backend);
	}
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	const struct shell *shell = (const struct shell *)backend->cb->ctx;
	const struct shell_log_backend *log_backend = shell->log_backend;

	if (IS_ENABLED(CONFIG_SHELL_STATS)) {
		atomic_add(&shell->stats->log_lost_cnt, cnt);
	}
	atomic_add(&log_backend->control_block->dropped_cnt, cnt);
}

static void copy_to_pbuffer(struct mpsc_pbuf_buffer *mpsc_buffer,
			    union log_msg2_generic *msg, uint32_t timeout)
{
	size_t wlen;
	union mpsc_pbuf_generic *dst;

	wlen = log_msg2_generic_get_wlen((union mpsc_pbuf_generic *)msg);
	dst = mpsc_pbuf_alloc(mpsc_buffer, wlen, K_MSEC(timeout));
	if (!dst) {
		/* No space to store the log */
		return;
	}

	/* First word contains internal mpsc packet flags and when copying
	 * those flags must be omitted.
	 */
	uint8_t *dst_data = (uint8_t *)dst + sizeof(struct mpsc_pbuf_hdr);
	uint8_t *src_data = (uint8_t *)msg + sizeof(struct mpsc_pbuf_hdr);
	size_t hdr_wlen = ceiling_fraction(sizeof(struct mpsc_pbuf_hdr),
					   sizeof(uint32_t));

	dst->hdr.data = msg->buf.hdr.data;
	memcpy(dst_data, src_data, (wlen - hdr_wlen) * sizeof(uint32_t));

	mpsc_pbuf_commit(mpsc_buffer, dst);
}

static void process_log_msg2(const struct shell *shell,
			     const struct log_output *log_output,
			     union log_msg2_generic *msg,
			     bool locked, bool colors)
{
	unsigned int key;
	uint32_t flags = LOG_OUTPUT_FLAG_LEVEL |
		      LOG_OUTPUT_FLAG_TIMESTAMP |
		      LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;

	if (colors) {
		flags |= LOG_OUTPUT_FLAG_COLORS;
	}

	if (locked) {
		key = irq_lock();
		if (!z_flag_cmd_ctx_get(shell)) {
			z_shell_cmd_line_erase(shell);
		}
	}

	log_output_msg2_process(log_output, &msg->log, flags);

	if (locked) {
		if (!z_flag_cmd_ctx_get(shell)) {
			z_shell_print_prompt_and_cmd(shell);
		}
		irq_unlock(key);
	}
}

static bool process_msg2_from_buffer(const struct shell *shell)
{
	const struct shell_log_backend *log_backend = shell->log_backend;
	struct mpsc_pbuf_buffer *mpsc_buffer = log_backend->mpsc_buffer;
	const struct log_output *log_output = log_backend->log_output;
	union log_msg2_generic *msg;
	bool colors = IS_ENABLED(CONFIG_SHELL_VT100_COLORS) &&
			z_flag_use_colors_get(shell);

	msg = (union log_msg2_generic *)mpsc_pbuf_claim(mpsc_buffer);
	if (!msg) {
		return false;
	}

	process_log_msg2(shell, log_output, msg, false, colors);

	mpsc_pbuf_free(mpsc_buffer, &msg->buf);

	return true;
}

static void process(const struct log_backend *const backend,
		    union log_msg2_generic *msg)
{
	const struct shell *shell = (const struct shell *)backend->cb->ctx;
	const struct shell_log_backend *log_backend = shell->log_backend;
	struct mpsc_pbuf_buffer *mpsc_buffer = log_backend->mpsc_buffer;
	const struct log_output *log_output = log_backend->log_output;
	bool colors = IS_ENABLED(CONFIG_SHELL_VT100_COLORS) &&
			z_flag_use_colors_get(shell);
	struct k_poll_signal *signal;

	switch (shell->log_backend->control_block->state) {
	case SHELL_LOG_BACKEND_ENABLED:
		if (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
			process_log_msg2(shell, log_output, msg, true, colors);
		} else {
			copy_to_pbuffer(mpsc_buffer, msg,
					log_backend->timeout);

			if (IS_ENABLED(CONFIG_MULTITHREADING)) {
				signal =
				    &shell->ctx->signals[SHELL_SIGNAL_LOG_MSG];
				k_poll_signal_raise(signal, 0);
			}
		}

		break;
	case SHELL_LOG_BACKEND_PANIC:
		z_shell_cmd_line_erase(shell);
		process_log_msg2(shell, log_output, msg, true, colors);

		break;

	case SHELL_LOG_BACKEND_DISABLED:
		__fallthrough;
	default:
		break;
	}
}

const struct log_backend_api log_backend_shell_api = {
	.process = process,
	.dropped = dropped,
	.panic = panic,
};
