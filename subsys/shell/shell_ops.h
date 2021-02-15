/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SHELL_OPS_H__
#define SHELL_OPS_H__

#include <stdbool.h>
#include <shell/shell.h>
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

/* Macro to send VT100 commands. */
#define Z_SHELL_VT100_CMD(_shell_, _cmd_)				\
	do {								\
		static const char cmd[] = _cmd_;			\
		z_shell_raw_fprintf(_shell_->fprintf_ctx, "%s", cmd);	\
	} while (0)

/* Function sends VT100 command to clear the screen from cursor position to
 * end of the screen.
 */
static inline void z_clear_eos(const struct shell *shell)
{
	Z_SHELL_VT100_CMD(shell, SHELL_VT100_CLEAREOS);
}

/* Function sends VT100 command to save cursor position. */
static inline void z_cursor_save(const struct shell *shell)
{
	Z_SHELL_VT100_CMD(shell, SHELL_VT100_SAVECURSOR);
}

/* Function sends VT100 command to restore saved cursor position. */
static inline void z_cursor_restore(const struct shell *shell)
{
	Z_SHELL_VT100_CMD(shell, SHELL_VT100_RESTORECURSOR);
}

/* Function forcing new line - cannot be replaced with function
 * cursor_down_move.
 */
static inline void z_cursor_next_line_move(const struct shell *shell)
{
	z_shell_raw_fprintf(shell->fprintf_ctx, "\n");
}

static inline bool z_flag_insert_mode_get(const struct shell *shell)
{
	return shell->ctx->internal.flags.insert_mode == 1;
}

static inline void z_flag_insert_mode_set(const struct shell *shell, bool val)
{
	shell->ctx->internal.flags.insert_mode = val ? 1 : 0;
}

static inline bool z_flag_use_colors_get(const struct shell *shell)
{
	return shell->ctx->internal.flags.use_colors == 1;
}

static inline void z_flag_use_colors_set(const struct shell *shell, bool val)
{
	shell->ctx->internal.flags.use_colors = val ? 1 : 0;
}

static inline bool z_flag_echo_get(const struct shell *shell)
{
	return shell->ctx->internal.flags.echo == 1;
}

static inline void z_flag_echo_set(const struct shell *shell, bool val)
{
	shell->ctx->internal.flags.echo = val ? 1 : 0;
}

static inline bool z_flag_processing_get(const struct shell *shell)
{
	return shell->ctx->internal.flags.processing == 1;
}

static inline bool z_flag_tx_rdy_get(const struct shell *shell)
{
	return shell->ctx->internal.flags.tx_rdy == 1;
}

static inline void z_flag_tx_rdy_set(const struct shell *shell, bool val)
{
	shell->ctx->internal.flags.tx_rdy = val ? 1 : 0;
}

static inline bool z_flag_mode_delete_get(const struct shell *shell)
{
	return shell->ctx->internal.flags.mode_delete == 1;
}

static inline void z_flag_mode_delete_set(const struct shell *shell, bool val)
{
	shell->ctx->internal.flags.mode_delete = val ? 1 : 0;
}

static inline bool z_flag_history_exit_get(const struct shell *shell)
{
	return shell->ctx->internal.flags.history_exit == 1;
}

static inline void z_flag_history_exit_set(const struct shell *shell, bool val)
{
	shell->ctx->internal.flags.history_exit = val ? 1 : 0;
}

static inline bool z_flag_cmd_ctx_get(const struct shell *shell)
{
	return shell->ctx->internal.flags.cmd_ctx == 1;
}

static inline void z_flag_cmd_ctx_set(const struct shell *shell, bool val)
{
	shell->ctx->internal.flags.cmd_ctx = val ? 1 : 0;
}

static inline uint8_t z_flag_last_nl_get(const struct shell *shell)
{
	return shell->ctx->internal.flags.last_nl;
}

static inline void z_flag_last_nl_set(const struct shell *shell, uint8_t val)
{
	shell->ctx->internal.flags.last_nl = val;
}

static inline bool z_flag_print_noinit_get(const struct shell *shell)
{
	return shell->ctx->internal.flags.print_noinit == 1;
}

static inline void z_flag_print_noinit_set(const struct shell *shell, bool val)
{
	shell->ctx->internal.flags.print_noinit = val ? 1 : 0;
}

void z_shell_op_cursor_vert_move(const struct shell *shell, int32_t delta);
void z_shell_op_cursor_horiz_move(const struct shell *shell, int32_t delta);

void z_shell_op_cond_next_line(const struct shell *shell);

/* Function will move cursor back to position == cmd_buff_pos. Example usage is
 * when cursor needs to be moved back after printing some text. This function
 * cannot be used to move cursor to new location by manual change of
 * cmd_buff_pos.
 */
void z_shell_op_cursor_position_synchronize(const struct shell *shell);

void z_shell_op_cursor_move(const struct shell *shell, int16_t val);

void z_shell_op_left_arrow(const struct shell *shell);

void z_shell_op_right_arrow(const struct shell *shell);

/* Moves cursor by defined number of words left (val negative) or right. */
void z_shell_op_cursor_word_move(const struct shell *shell, int16_t val);

/*
 *  Removes the "word" to the left of the cursor:
 *  - if there are spaces at the cursor position, remove all spaces to the left
 *  - remove the non-spaces (word) until a space is found or a beginning of
 *    buffer
 */
void z_shell_op_word_remove(const struct shell *shell);

/* Function moves cursor to begin of command position, just after console
 * name.
 */
void z_shell_op_cursor_home_move(const struct shell *shell);

/* Function moves cursor to end of command. */
void z_shell_op_cursor_end_move(const struct shell *shell);

void z_shell_op_char_insert(const struct shell *shell, char data);

void z_shell_op_char_backspace(const struct shell *shell);

void z_shell_op_char_delete(const struct shell *shell);

void z_shell_op_delete_from_cursor(const struct shell *shell);

void z_shell_op_completion_insert(const struct shell *shell,
				  const char *compl,
				  uint16_t compl_len);

bool z_shell_cursor_in_empty_line(const struct shell *shell);

void z_shell_cmd_line_erase(const struct shell *shell);

/**
 * @brief Print command buffer.
 *
 * @param shell Shell instance.
 */
void z_shell_print_cmd(const struct shell *shell);

/**
 * @brief Print prompt followed by command buffer.
 *
 * @param shell Shell instance.
 */
void z_shell_print_prompt_and_cmd(const struct shell *shell);

/* Function sends data stream to the shell instance. Each time before the
 * shell_write function is called, it must be ensured that IO buffer of fprintf
 * is flushed to avoid synchronization issues.
 * For that purpose, use function transport_buffer_flush(shell)
 *
 * This function can be only used by shell module, it shall not be called
 * directly.
 */
void z_shell_write(const struct shell *shell, const void *data, size_t length);

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
void z_shell_vt100_color_set(const struct shell *shell,
			     enum shell_vt100_color color);

static inline void z_shell_vt100_colors_store(const struct shell *shell,
					      struct shell_vt100_colors *color)
{
	memcpy(color, &shell->ctx->vt100_ctx.col, sizeof(*color));
}

void z_shell_vt100_colors_restore(const struct shell *shell,
				  const struct shell_vt100_colors *color);

/* This function can be called only within shell thread but not from command
 * handlers.
 */
void z_shell_fprintf(const struct shell *shell, enum shell_vt100_color color,
		     const char *fmt, ...);

void z_shell_vfprintf(const struct shell *shell, enum shell_vt100_color color,
		      const char *fmt, va_list args);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_OPS_H__ */
