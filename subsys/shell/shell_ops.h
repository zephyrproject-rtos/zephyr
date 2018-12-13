/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SHELL_OPS_H__
#define SHELL_OPS_H__

#include <shell/shell.h>
#include "shell_vt100.h"
#include "shell_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SHELL_DEFAULT_TERMINAL_WIDTH	(80u) /* Default PuTTY width. */
#define SHELL_DEFAULT_TERMINAL_HEIGHT	(24u) /* Default PuTTY height. */

static inline void shell_raw_fprintf(const struct shell_fprintf *const ctx,
				     const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	shell_fprintf_fmt(ctx, fmt, args);
	va_end(args);
}

/* Macro to send VT100 commands. */
#define SHELL_VT100_CMD(_shell_, _cmd_) \
	do {								\
		static const char cmd[] = _cmd_;			\
		shell_raw_fprintf(_shell_->fprintf_ctx, "%s", cmd);	\
	} while (0)

/* Function sends VT100 command to clear the screen from cursor position to
 * end of the screen.
 */
static inline void clear_eos(const struct shell *shell)
{
	SHELL_VT100_CMD(shell, SHELL_VT100_CLEAREOS);
}

/* Function sends VT100 command to save cursor position. */
static inline void cursor_save(const struct shell *shell)
{
	SHELL_VT100_CMD(shell, SHELL_VT100_SAVECURSOR);
}

/* Function sends VT100 command to restore saved cursor position. */
static inline void cursor_restore(const struct shell *shell)
{
	SHELL_VT100_CMD(shell, SHELL_VT100_RESTORECURSOR);
}

/* Function forcing new line - cannot be replaced with function
 * cursor_down_move.
 */
static inline void cursor_next_line_move(const struct shell *shell)
{
	shell_raw_fprintf(shell->fprintf_ctx, "\n");
}

/* Function sends 1 character to the shell instance. */
static inline void shell_putc(const struct shell *shell, char ch)
{
	shell_raw_fprintf(shell->fprintf_ctx, "%c", ch);
}

static inline bool flag_echo_is_set(const struct shell *shell)
{
	return shell->ctx->internal.flags.echo == 1 ? true : false;
}

void shell_op_cursor_vert_move(const struct shell *shell, s32_t delta);
void shell_op_cursor_horiz_move(const struct shell *shell, s32_t delta);

void shell_op_cond_next_line(const struct shell *shell);

/* Function will move cursor back to position == cmd_buff_pos. Example usage is
 * when cursor needs to be moved back after printing some text. This function
 * cannot be used to move cursor to new location by manual change of
 * cmd_buff_pos.
 */
void shell_op_cursor_position_synchronize(const struct shell *shell);

void shell_op_cursor_move(const struct shell *shell, s16_t val);

void shell_op_left_arrow(const struct shell *shell);

void shell_op_right_arrow(const struct shell *shell);

/*
 *  Removes the "word" to the left of the cursor:
 *  - if there are spaces at the cursor position, remove all spaces to the left
 *  - remove the non-spaces (word) until a space is found or a beginning of
 *    buffer
 */
void shell_op_word_remove(const struct shell *shell);

/* Function moves cursor to begin of command position, just after console
 * name.
 */
void shell_op_cursor_home_move(const struct shell *shell);

/* Function moves cursor to end of command. */
void shell_op_cursor_end_move(const struct shell *shell);

void char_replace(const struct shell *shell, char data);

void shell_op_char_insert(const struct shell *shell, char data);

void shell_op_char_backspace(const struct shell *shell);

void shell_op_char_delete(const struct shell *shell);

void shell_op_completion_insert(const struct shell *shell,
				const char *compl,
				u16_t compl_len);

bool shell_cursor_in_empty_line(const struct shell *shell);

/* Function sends data stream to the shell instance. Each time before the
 * shell_write function is called, it must be ensured that IO buffer of fprintf
 * is flushed to avoid synchronization issues.
 * For that purpose, use function transport_buffer_flush(shell)
 *
 * This function can be only used by shell module, it shall not be called
 * directly.
 */
void shell_write(const struct shell *shell, const void *data,
		 size_t length);

/**
 * @internal @brief This function shall not be used directly, it is required by
 *		    the fprintf module.
 *
 * @param[in] p_user_ctx    Pointer to the context for the shell instance.
 * @param[in] p_data        Pointer to the data buffer.
 * @param[in] data_len      Data buffer size.
 */
void shell_print_stream(const void *user_ctx, const char *data,
			size_t data_len);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_OPS_H__ */
