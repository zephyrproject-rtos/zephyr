/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SHELL_OPS_H__
#define SHELL_OPS_H__

#include <stdbool.h>
#include <zephyr/shell/shell.h>
#include "shell_vt100.h"
#include "shell_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void z_shell_raw_fprintf(const struct shell_fprintf *const ctx,
				       const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	z_shell_fprintf_fmt(ctx, fmt, args);
	va_end(args);
}

/* Macro to send VT100 command. */
#define Z_SHELL_VT100_CMD(_shell_, ...)					\
	do {								\
		if (!IS_ENABLED(CONFIG_SHELL_VT100_COMMANDS) ||		\
		    !z_flag_use_vt100_get(_shell_))			\
			break;						\
		z_shell_raw_fprintf(_shell_->fprintf_ctx, __VA_ARGS__);	\
	} while (0)

#define Z_SHELL_SET_FLAG_ATOMIC(_shell_, _type_, _flag_, _val_, _ret_)		\
	do {									\
		union shell_backend_##_type_ _internal_;			\
		atomic_t *_dst_ = (atomic_t *)&(_shell_)->ctx->_type_.value;	\
		_internal_.value = 0U;						\
		_internal_.flags._flag_ = 1U;					\
		if (_val_) {							\
			_internal_.value = atomic_or(_dst_,			\
						   _internal_.value);		\
		} else {							\
			_internal_.value = atomic_and(_dst_,			\
						   ~_internal_.value);		\
		}								\
		_ret_ = (_internal_.flags._flag_ != 0);				\
	} while (false)

static inline bool z_flag_insert_mode_get(const struct shell *sh)
{
	return sh->ctx->cfg.flags.insert_mode == 1;
}

static inline bool z_flag_insert_mode_set(const struct shell *sh, bool val)
{
	bool ret;

	Z_SHELL_SET_FLAG_ATOMIC(sh, cfg, insert_mode, val, ret);
	return ret;
}

static inline bool z_flag_use_colors_get(const struct shell *sh)
{
	return sh->ctx->cfg.flags.use_colors == 1;
}

static inline bool z_flag_use_colors_set(const struct shell *sh, bool val)
{
	bool ret;

	Z_SHELL_SET_FLAG_ATOMIC(sh, cfg, use_colors, val, ret);
	return ret;
}

static inline bool z_flag_use_vt100_get(const struct shell *sh)
{
	return sh->ctx->cfg.flags.use_vt100 == 1;
}

static inline bool z_flag_use_vt100_set(const struct shell *sh, bool val)
{
	bool ret;

	Z_SHELL_SET_FLAG_ATOMIC(sh, cfg, use_vt100, val, ret);
	return ret;
}

static inline bool z_flag_echo_get(const struct shell *sh)
{
	return sh->ctx->cfg.flags.echo == 1;
}

static inline bool z_flag_echo_set(const struct shell *sh, bool val)
{
	bool ret;

	Z_SHELL_SET_FLAG_ATOMIC(sh, cfg, echo, val, ret);
	return ret;
}

static inline bool z_flag_obscure_get(const struct shell *sh)
{
	return sh->ctx->cfg.flags.obscure == 1;
}

static inline bool z_flag_obscure_set(const struct shell *sh, bool val)
{
	bool ret;

	Z_SHELL_SET_FLAG_ATOMIC(sh, cfg, obscure, val, ret);
	return ret;
}

static inline bool z_flag_processing_get(const struct shell *sh)
{
	return sh->ctx->ctx.flags.processing == 1;
}

static inline bool z_flag_processing_set(const struct shell *sh, bool val)
{
	bool ret;

	Z_SHELL_SET_FLAG_ATOMIC(sh, ctx, processing, val, ret);
	return ret;
}

static inline bool z_flag_tx_rdy_get(const struct shell *sh)
{
	return sh->ctx->ctx.flags.tx_rdy == 1;
}

static inline bool z_flag_tx_rdy_set(const struct shell *sh, bool val)
{
	bool ret;

	Z_SHELL_SET_FLAG_ATOMIC(sh, ctx, tx_rdy, val, ret);
	return ret;
}

static inline bool z_flag_mode_delete_get(const struct shell *sh)
{
	return sh->ctx->cfg.flags.mode_delete == 1;
}

static inline bool z_flag_mode_delete_set(const struct shell *sh, bool val)
{
	bool ret;

	Z_SHELL_SET_FLAG_ATOMIC(sh, cfg, mode_delete, val, ret);
	return ret;
}

static inline bool z_flag_history_exit_get(const struct shell *sh)
{
	return sh->ctx->ctx.flags.history_exit == 1;
}

static inline bool z_flag_history_exit_set(const struct shell *sh, bool val)
{
	bool ret;

	Z_SHELL_SET_FLAG_ATOMIC(sh, ctx, history_exit, val, ret);
	return ret;
}

static inline bool z_flag_cmd_ctx_get(const struct shell *sh)
{
	return sh->ctx->ctx.flags.cmd_ctx == 1;
}

static inline bool z_flag_cmd_ctx_set(const struct shell *sh, bool val)
{
	bool ret;

	Z_SHELL_SET_FLAG_ATOMIC(sh, ctx, cmd_ctx, val, ret);
	return ret;
}

static inline uint8_t z_flag_last_nl_get(const struct shell *sh)
{
	return sh->ctx->ctx.flags.last_nl;
}

static inline int z_shell_get_return_value(const struct shell *sh)
{
	return sh->ctx->ret_val;
}

static inline void z_flag_last_nl_set(const struct shell *sh, uint8_t val)
{
	sh->ctx->ctx.flags.last_nl = val;
}

static inline bool z_flag_print_noinit_get(const struct shell *sh)
{
	return sh->ctx->ctx.flags.print_noinit == 1;
}

static inline bool z_flag_print_noinit_set(const struct shell *sh, bool val)
{
	bool ret;

	Z_SHELL_SET_FLAG_ATOMIC(sh, ctx, print_noinit, val, ret);
	return ret;
}

static inline bool z_flag_sync_mode_get(const struct shell *sh)
{
	return sh->ctx->ctx.flags.sync_mode == 1;
}

static inline bool z_flag_sync_mode_set(const struct shell *sh, bool val)
{
	bool ret;

	Z_SHELL_SET_FLAG_ATOMIC(sh, ctx, sync_mode, val, ret);
	return ret;
}

static inline bool z_flag_handle_log_get(const struct shell *sh)
{
	return sh->ctx->ctx.flags.handle_log == 1;
}

static inline bool z_flag_handle_log_set(const struct shell *sh, bool val)
{
	bool ret;

	Z_SHELL_SET_FLAG_ATOMIC(sh, ctx, handle_log, val, ret);
	return ret;
}

/* Function sends VT100 command to clear the screen from cursor position to
 * end of the screen.
 */
static inline void z_clear_eos(const struct shell *sh)
{
	Z_SHELL_VT100_CMD(sh, SHELL_VT100_CLEAREOS);
}

/* Function sends VT100 command to save cursor position. */
static inline void z_cursor_save(const struct shell *sh)
{
	Z_SHELL_VT100_CMD(sh, SHELL_VT100_SAVECURSOR);
}

/* Function sends VT100 command to restore saved cursor position. */
static inline void z_cursor_restore(const struct shell *sh)
{
	Z_SHELL_VT100_CMD(sh, SHELL_VT100_RESTORECURSOR);
}

/* Function forcing new line - cannot be replaced with function
 * cursor_down_move.
 */
static inline void z_cursor_next_line_move(const struct shell *sh)
{
	z_shell_raw_fprintf(sh->fprintf_ctx, "\n");
}

void z_shell_op_cursor_vert_move(const struct shell *sh, int32_t delta);

void z_shell_op_cursor_horiz_move(const struct shell *sh, int32_t delta);

void z_shell_op_cond_next_line(const struct shell *sh);

/* Function will move cursor back to position == cmd_buff_pos. Example usage is
 * when cursor needs to be moved back after printing some text. This function
 * cannot be used to move cursor to new location by manual change of
 * cmd_buff_pos.
 */
void z_shell_op_cursor_position_synchronize(const struct shell *sh);

void z_shell_op_cursor_move(const struct shell *sh, int16_t val);

void z_shell_op_left_arrow(const struct shell *sh);

void z_shell_op_right_arrow(const struct shell *sh);

/* Moves cursor by defined number of words left (val negative) or right. */
void z_shell_op_cursor_word_move(const struct shell *sh, int16_t val);

/*
 *  Removes the "word" to the left of the cursor:
 *  - if there are spaces at the cursor position, remove all spaces to the left
 *  - remove the non-spaces (word) until a space is found or a beginning of
 *    buffer
 */
void z_shell_op_word_remove(const struct shell *sh);

/* Function moves cursor to begin of command position, just after console
 * name.
 */
void z_shell_op_cursor_home_move(const struct shell *sh);

/* Function moves cursor to end of command. */
void z_shell_op_cursor_end_move(const struct shell *sh);

void z_shell_op_char_insert(const struct shell *sh, char data);

void z_shell_op_char_backspace(const struct shell *sh);

void z_shell_op_char_delete(const struct shell *sh);

void z_shell_op_delete_from_cursor(const struct shell *sh);

void z_shell_op_completion_insert(const struct shell *sh,
				  const char *compl,
				  uint16_t compl_len);

bool z_shell_cursor_in_empty_line(const struct shell *sh);

void z_shell_cmd_line_erase(const struct shell *sh);

/**
 * @brief Print command buffer.
 *
 * @param sh Shell instance.
 */
void z_shell_print_cmd(const struct shell *sh);

/**
 * @brief Print prompt followed by command buffer.
 *
 * @param sh Shell instance.
 */
void z_shell_print_prompt_and_cmd(const struct shell *sh);

/* Function sends data stream to the shell instance. Each time before the
 * shell_write function is called, it must be ensured that IO buffer of fprintf
 * is flushed to avoid synchronization issues.
 * For that purpose, use function transport_buffer_flush(shell)
 *
 * This function can be only used by shell module, it shall not be called
 * directly.
 */
void z_shell_write(const struct shell *sh, const void *data, size_t length);

/**
 * @internal @brief This function shall not be used directly, it is required by
 *		    the fprintf module.
 *
 * @param[in] p_user_ctx    Pointer to the context for the shell instance.
 * @param[in] p_data        Pointer to the data buffer.
 * @param[in] len           Data buffer size.
 */
void z_shell_print_stream(const void *user_ctx, const char *data, size_t len);

/** @internal @brief Function for setting font color */
void z_shell_vt100_color_set(const struct shell *sh,
			     enum shell_vt100_color color);

static inline void z_shell_vt100_colors_store(const struct shell *sh,
					      struct shell_vt100_colors *color)
{
	memcpy(color, &sh->ctx->vt100_ctx.col, sizeof(*color));
}

void z_shell_vt100_colors_restore(const struct shell *sh,
				  const struct shell_vt100_colors *color);

/* This function can be called only within shell thread but not from command
 * handlers.
 */
void z_shell_fprintf(const struct shell *sh, enum shell_vt100_color color,
		     const char *fmt, ...);

void z_shell_vfprintf(const struct shell *sh, enum shell_vt100_color color,
		      const char *fmt, va_list args);

/**
 * @brief Flushes the shell backend receive buffer.
 *
 * This function repeatedly reads from the shell interface's receive buffer
 * until it is empty or a maximum number of iterations is reached.
 * It ensures that no additional data is left in the buffer.
 */
void z_shell_backend_rx_buffer_flush(const struct shell *sh);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_OPS_H__ */
