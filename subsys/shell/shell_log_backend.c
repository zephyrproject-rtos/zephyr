/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell_log_backend.h>
#include <zephyr/shell/shell.h>
#include "shell_ops.h"
#include <zephyr/logging/log_ctrl.h>

static bool process_msg_from_buffer(const struct shell *sh);

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
		const struct shell *sh;

		sh = (const struct shell *)ctx;

		z_flag_sync_mode_set(sh, true);
		/* Reenable transport in blocking mode */
		err = sh->iface->api->enable(sh->iface, true);
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
	const struct shell *sh =
			(const struct shell *)backend->backend->cb->ctx;
	uint32_t dropped;
	bool colors = IS_ENABLED(CONFIG_SHELL_VT100_COLORS) &&
			z_flag_use_colors_get(sh);

	dropped = atomic_set(&backend->control_block->dropped_cnt, 0);
	if (dropped) {
		struct shell_vt100_colors col;

		if (colors) {
			z_shell_vt100_colors_store(sh, &col);
			z_shell_vt100_color_set(sh, SHELL_VT100_COLOR_RED);
		}

		log_output_dropped_process(backend->log_output, dropped);

		if (colors) {
			z_shell_vt100_colors_restore(sh, &col);
		}
	}

	return process_msg_from_buffer(sh);
}

static void panic(const struct log_backend *const backend)
{
	const struct shell *sh = (const struct shell *)backend->cb->ctx;
	int err;

	if (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
		return;
	}

	err = sh->iface->api->enable(sh->iface, true);

	if (err == 0) {
		sh->log_backend->control_block->state =
						SHELL_LOG_BACKEND_PANIC;
		z_flag_sync_mode_set(sh, true);

		/* Move to the start of next line. */
		z_shell_multiline_data_calc(&sh->ctx->vt100_ctx.cons,
					    sh->ctx->cmd_buff_pos,
					    sh->ctx->cmd_buff_len);
		z_shell_op_cursor_vert_move(sh, -1);
		z_shell_op_cursor_horiz_move(sh,
					   -sh->ctx->vt100_ctx.cons.cur_x);

		while (process_msg_from_buffer(sh)) {
			/* empty */
		}
	} else {
		z_shell_log_backend_disable(sh->log_backend);
	}
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	const struct shell *sh = (const struct shell *)backend->cb->ctx;
	const struct shell_log_backend *log_backend = sh->log_backend;

	if (IS_ENABLED(CONFIG_SHELL_STATS)) {
		atomic_add(&sh->stats->log_lost_cnt, cnt);
	}
	atomic_add(&log_backend->control_block->dropped_cnt, cnt);
}

static bool copy_to_pbuffer(struct mpsc_pbuf_buffer *mpsc_buffer,
			    union log_msg_generic *msg, uint32_t timeout)
{
	size_t wlen;
	union mpsc_pbuf_generic *dst;

	wlen = log_msg_generic_get_wlen((union mpsc_pbuf_generic *)msg);
	dst = mpsc_pbuf_alloc(mpsc_buffer, wlen, K_MSEC(timeout));
	if (!dst) {
		/* No space to store the log */
		return false;
	}

	/* First word contains internal mpsc packet flags and when copying
	 * those flags must be omitted.
	 */
	uint8_t *dst_data = (uint8_t *)dst + sizeof(struct mpsc_pbuf_hdr);
	uint8_t *src_data = (uint8_t *)msg + sizeof(struct mpsc_pbuf_hdr);
	size_t hdr_wlen = DIV_ROUND_UP(sizeof(struct mpsc_pbuf_hdr),
					   sizeof(uint32_t));
	if (wlen <= hdr_wlen) {
		return false;
	}

	dst->hdr.data = msg->buf.hdr.data;
	memcpy(dst_data, src_data, (wlen - hdr_wlen) * sizeof(uint32_t));

	mpsc_pbuf_commit(mpsc_buffer, dst);

	return true;
}

static void process_log_msg(const struct shell *sh,
			     const struct log_output *log_output,
			     union log_msg_generic *msg,
			     bool locked, bool colors)
{
	unsigned int key = 0;
	uint32_t flags = LOG_OUTPUT_FLAG_LEVEL |
		      LOG_OUTPUT_FLAG_TIMESTAMP |
		      (IS_ENABLED(CONFIG_SHELL_LOG_FORMAT_TIMESTAMP) ?
			LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP : 0);

	if (colors) {
		flags |= LOG_OUTPUT_FLAG_COLORS;
	}

	if (locked) {
		/*
		 * If running in the thread context, lock the shell mutex to synchronize with
		 * messages printed on the shell thread. In the ISR context, using a mutex is
		 * forbidden so use the IRQ lock to at least synchronize log messages printed
		 * in different contexts.
		 */
		if (k_is_in_isr()) {
			key = irq_lock();
		} else {
			k_mutex_lock(&sh->ctx->wr_mtx, K_FOREVER);
		}
		if (!z_flag_cmd_ctx_get(sh)) {
			z_shell_cmd_line_erase(sh);
		}
	}

	log_output_msg_process(log_output, &msg->log, flags);

	if (locked) {
		if (!z_flag_cmd_ctx_get(sh)) {
			z_shell_print_prompt_and_cmd(sh);
		}
		if (k_is_in_isr()) {
			irq_unlock(key);
		} else {
			k_mutex_unlock(&sh->ctx->wr_mtx);
		}
	}
}

static bool process_msg_from_buffer(const struct shell *sh)
{
	const struct shell_log_backend *log_backend = sh->log_backend;
	struct mpsc_pbuf_buffer *mpsc_buffer = log_backend->mpsc_buffer;
	const struct log_output *log_output = log_backend->log_output;
	union log_msg_generic *msg;
	bool colors = IS_ENABLED(CONFIG_SHELL_VT100_COLORS) &&
			z_flag_use_colors_get(sh);

	msg = (union log_msg_generic *)mpsc_pbuf_claim(mpsc_buffer);
	if (!msg) {
		return false;
	}

	process_log_msg(sh, log_output, msg, false, colors);

	mpsc_pbuf_free(mpsc_buffer, &msg->buf);

	return true;
}

static void process(const struct log_backend *const backend,
		    union log_msg_generic *msg)
{
	const struct shell *sh = (const struct shell *)backend->cb->ctx;
	const struct shell_log_backend *log_backend = sh->log_backend;
	struct mpsc_pbuf_buffer *mpsc_buffer = log_backend->mpsc_buffer;
	const struct log_output *log_output = log_backend->log_output;
	bool colors = IS_ENABLED(CONFIG_SHELL_VT100_COLORS) &&
			z_flag_use_colors_get(sh);
	struct k_poll_signal *signal;

	switch (sh->log_backend->control_block->state) {
	case SHELL_LOG_BACKEND_ENABLED:
		if (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE)) {
			process_log_msg(sh, log_output, msg, true, colors);
		} else {
			if (copy_to_pbuffer(mpsc_buffer, msg, log_backend->timeout)) {
				if (IS_ENABLED(CONFIG_MULTITHREADING)) {
					signal = &sh->ctx->signals[SHELL_SIGNAL_LOG_MSG];
					k_poll_signal_raise(signal, 0);
				}
			} else {
				dropped(backend, 1);
			}
		}

		break;
	case SHELL_LOG_BACKEND_PANIC:
		z_shell_cmd_line_erase(sh);
		process_log_msg(sh, log_output, msg, true, colors);

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
