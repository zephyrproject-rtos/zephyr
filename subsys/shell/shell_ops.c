/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include "shell_ops.h"

void z_shell_op_cursor_vert_move(const struct shell *shell, int32_t delta)
{
	if (delta != 0) {
		z_shell_raw_fprintf(shell->fprintf_ctx, "\033[%d%c",
				    delta > 0 ? delta : -delta,
				    delta > 0 ? 'A' : 'B');
	}
}

void z_shell_op_cursor_horiz_move(const struct shell *shell, int32_t delta)
{
	if (delta != 0) {
		z_shell_raw_fprintf(shell->fprintf_ctx, "\033[%d%c",
				    delta > 0 ? delta : -delta,
				    delta > 0 ? 'C' : 'D');
	}
}

/* Function returns true if command length is equal to multiplicity of terminal
 * width.
 */
static inline bool full_line_cmd(const struct shell *shell)
{
	return ((shell->ctx->cmd_buff_len + z_shell_strlen(shell->ctx->prompt))
			% shell->ctx->vt100_ctx.cons.terminal_wid == 0U);
}

/* Function returns true if cursor is at beginning of an empty line. */
bool z_shell_cursor_in_empty_line(const struct shell *shell)
{
	return ((shell->ctx->cmd_buff_pos + z_shell_strlen(shell->ctx->prompt))
			% shell->ctx->vt100_ctx.cons.terminal_wid == 0U);
}

void z_shell_op_cond_next_line(const struct shell *shell)
{
	if (z_shell_cursor_in_empty_line(shell) || full_line_cmd(shell)) {
		z_cursor_next_line_move(shell);
	}
}

void z_shell_op_cursor_position_synchronize(const struct shell *shell)
{
	struct shell_multiline_cons *cons = &shell->ctx->vt100_ctx.cons;
	bool last_line;

	z_shell_multiline_data_calc(cons, shell->ctx->cmd_buff_pos,
				    shell->ctx->cmd_buff_len);
	last_line = (cons->cur_y == cons->cur_y_end);

	/* In case cursor reaches the bottom line of a terminal, it will
	 * be moved to the next line.
	 */
	if (full_line_cmd(shell)) {
		z_cursor_next_line_move(shell);
	}

	if (last_line) {
		z_shell_op_cursor_horiz_move(shell, cons->cur_x -
							       cons->cur_x_end);
	} else {
		z_shell_op_cursor_vert_move(shell, cons->cur_y_end - cons->cur_y);
		z_shell_op_cursor_horiz_move(shell, cons->cur_x -
							       cons->cur_x_end);
	}
}

void z_shell_op_cursor_move(const struct shell *shell, int16_t val)
{
	struct shell_multiline_cons *cons = &shell->ctx->vt100_ctx.cons;
	uint16_t new_pos = shell->ctx->cmd_buff_pos + val;
	int32_t row_span;
	int32_t col_span;

	z_shell_multiline_data_calc(cons, shell->ctx->cmd_buff_pos,
				    shell->ctx->cmd_buff_len);

	/* Calculate the new cursor. */
	row_span = z_row_span_with_buffer_offsets_get(
						&shell->ctx->vt100_ctx.cons,
						shell->ctx->cmd_buff_pos,
						new_pos);
	col_span = z_column_span_with_buffer_offsets_get(
						&shell->ctx->vt100_ctx.cons,
						shell->ctx->cmd_buff_pos,
						new_pos);

	z_shell_op_cursor_vert_move(shell, -row_span);
	z_shell_op_cursor_horiz_move(shell, col_span);
	shell->ctx->cmd_buff_pos = new_pos;
}

static uint16_t shift_calc(const char *str, uint16_t pos, uint16_t len, int16_t sign)
{
	bool found = false;
	uint16_t ret = 0U;
	uint16_t idx;

	while (1) {
		idx = pos + ret * sign;
		if (((idx == 0U) && (sign < 0)) ||
		    ((idx == len) && (sign > 0))) {
			break;
		}
		if (isalnum((int)str[idx]) != 0) {
			found = true;
		} else {
			if (found) {
				break;
			}
		}
		ret++;
	}

	return ret;
}

void z_shell_op_cursor_word_move(const struct shell *shell, int16_t val)
{
	int16_t shift;
	int16_t sign;

	if (val < 0) {
		val = -val;
		sign = -1;
	} else {
		sign = 1;
	}

	while (val--) {
		shift = shift_calc(shell->ctx->cmd_buff,
				   shell->ctx->cmd_buff_pos,
				   shell->ctx->cmd_buff_len, sign);
		z_shell_op_cursor_move(shell, sign * shift);
	}
}

void z_shell_op_word_remove(const struct shell *shell)
{
	char *str = &shell->ctx->cmd_buff[shell->ctx->cmd_buff_pos - 1];
	char *str_start = &shell->ctx->cmd_buff[0];
	uint16_t chars_to_delete;

	/* Line must not be empty and cursor must not be at 0 to continue. */
	if ((shell->ctx->cmd_buff_len == 0) ||
	    (shell->ctx->cmd_buff_pos == 0)) {
		return;
	}

	/* Start at the current position. */
	chars_to_delete = 0U;

	/* Look back for all spaces then for non-spaces. */
	while ((str >= str_start) && (*str == ' ')) {
		++chars_to_delete;
		--str;
	}

	while ((str >= str_start) && (*str != ' ')) {
		++chars_to_delete;
		--str;
	}

	/* Manage the buffer. */
	memmove(str + 1, str + 1 + chars_to_delete,
		shell->ctx->cmd_buff_len - chars_to_delete);
	shell->ctx->cmd_buff_len -= chars_to_delete;
	shell->ctx->cmd_buff[shell->ctx->cmd_buff_len] = '\0';

	/* Update display. */
	z_shell_op_cursor_move(shell, -chars_to_delete);
	z_cursor_save(shell);
	z_shell_fprintf(shell, SHELL_NORMAL, "%s", str + 1);
	z_clear_eos(shell);
	z_cursor_restore(shell);
}

void z_shell_op_cursor_home_move(const struct shell *shell)
{
	z_shell_op_cursor_move(shell, -shell->ctx->cmd_buff_pos);
}

void z_shell_op_cursor_end_move(const struct shell *shell)
{
	z_shell_op_cursor_move(shell, shell->ctx->cmd_buff_len -
						shell->ctx->cmd_buff_pos);
}

void z_shell_op_left_arrow(const struct shell *shell)
{
	if (shell->ctx->cmd_buff_pos > 0) {
		z_shell_op_cursor_move(shell, -1);
	}
}

void z_shell_op_right_arrow(const struct shell *shell)
{
	if (shell->ctx->cmd_buff_pos < shell->ctx->cmd_buff_len) {
		z_shell_op_cursor_move(shell, 1);
	}
}

static void reprint_from_cursor(const struct shell *shell, uint16_t diff,
				bool data_removed)
{
	/* Clear eos is needed only when newly printed command is shorter than
	 * previously printed command. This can happen when delete or backspace
	 * was called.
	 *
	 * Such condition is useful for Bluetooth devices to save number of
	 * bytes transmitted between terminal and device.
	 */
	if (data_removed) {
		z_clear_eos(shell);
	}

	if (z_flag_obscure_get(shell)) {
		int len = strlen(&shell->ctx->cmd_buff[shell->ctx->cmd_buff_pos]);

		while (len--) {
			z_shell_raw_fprintf(shell->fprintf_ctx, "*");
		}
	} else {
		z_shell_fprintf(shell, SHELL_NORMAL, "%s",
			      &shell->ctx->cmd_buff[shell->ctx->cmd_buff_pos]);
	}
	shell->ctx->cmd_buff_pos = shell->ctx->cmd_buff_len;

	if (full_line_cmd(shell)) {
		if (((data_removed) && (diff > 0)) || (!data_removed)) {
			z_cursor_next_line_move(shell);
		}
	}

	z_shell_op_cursor_move(shell, -diff);
}

static void data_insert(const struct shell *shell, const char *data, uint16_t len)
{
	uint16_t after = shell->ctx->cmd_buff_len - shell->ctx->cmd_buff_pos;
	char *curr_pos = &shell->ctx->cmd_buff[shell->ctx->cmd_buff_pos];

	if ((shell->ctx->cmd_buff_len + len) >= CONFIG_SHELL_CMD_BUFF_SIZE) {
		return;
	}

	memmove(curr_pos + len, curr_pos, after);
	memcpy(curr_pos, data, len);
	shell->ctx->cmd_buff_len += len;
	shell->ctx->cmd_buff[shell->ctx->cmd_buff_len] = '\0';

	if (!z_flag_echo_get(shell)) {
		shell->ctx->cmd_buff_pos += len;
		return;
	}

	reprint_from_cursor(shell, after, false);
}

static void char_replace(const struct shell *shell, char data)
{
	shell->ctx->cmd_buff[shell->ctx->cmd_buff_pos++] = data;

	if (!z_flag_echo_get(shell)) {
		return;
	}
	if (z_flag_obscure_get(shell)) {
		data = '*';
	}

	z_shell_raw_fprintf(shell->fprintf_ctx, "%c", data);
	if (z_shell_cursor_in_empty_line(shell)) {
		z_cursor_next_line_move(shell);
	}
}

void z_shell_op_char_insert(const struct shell *shell, char data)
{
	if (shell->ctx->internal.flags.insert_mode &&
		(shell->ctx->cmd_buff_len != shell->ctx->cmd_buff_pos)) {
		char_replace(shell, data);
	} else {
		data_insert(shell, &data, 1);
	}
}

void z_shell_op_char_backspace(const struct shell *shell)
{
	if ((shell->ctx->cmd_buff_len == 0) ||
	    (shell->ctx->cmd_buff_pos == 0)) {
		return;
	}

	z_shell_op_cursor_move(shell, -1);
	z_shell_op_char_delete(shell);
}

void z_shell_op_char_delete(const struct shell *shell)
{
	uint16_t diff = shell->ctx->cmd_buff_len - shell->ctx->cmd_buff_pos;
	char *str = &shell->ctx->cmd_buff[shell->ctx->cmd_buff_pos];

	if (diff == 0U) {
		return;
	}

	memmove(str, str + 1, diff);
	--shell->ctx->cmd_buff_len;
	reprint_from_cursor(shell, --diff, true);
}

void z_shell_op_delete_from_cursor(const struct shell *shell)
{
	shell->ctx->cmd_buff_len = shell->ctx->cmd_buff_pos;
	shell->ctx->cmd_buff[shell->ctx->cmd_buff_pos] = '\0';

	z_clear_eos(shell);
}

void z_shell_op_completion_insert(const struct shell *shell,
				  const char *compl,
				  uint16_t compl_len)
{
	data_insert(shell, compl, compl_len);
}

void z_shell_cmd_line_erase(const struct shell *shell)
{
	z_shell_multiline_data_calc(&shell->ctx->vt100_ctx.cons,
				    shell->ctx->cmd_buff_pos,
				    shell->ctx->cmd_buff_len);
	z_shell_op_cursor_horiz_move(shell,
				   -(shell->ctx->vt100_ctx.cons.cur_x - 1));
	z_shell_op_cursor_vert_move(shell, shell->ctx->vt100_ctx.cons.cur_y - 1);

	z_clear_eos(shell);
}

static void print_prompt(const struct shell *shell)
{
	z_shell_fprintf(shell, SHELL_INFO, "%s", shell->ctx->prompt);
}

void z_shell_print_cmd(const struct shell *shell)
{
	z_shell_raw_fprintf(shell->fprintf_ctx, "%s", shell->ctx->cmd_buff);
}

void z_shell_print_prompt_and_cmd(const struct shell *shell)
{
	print_prompt(shell);

	if (z_flag_echo_get(shell)) {
		z_shell_print_cmd(shell);
		z_shell_op_cursor_position_synchronize(shell);
	}
}

static void shell_pend_on_txdone(const struct shell *shell)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING) &&
	    (shell->ctx->state < SHELL_STATE_PANIC_MODE_ACTIVE)) {
		k_poll(&shell->ctx->events[SHELL_SIGNAL_TXDONE], 1, K_FOREVER);
		k_poll_signal_reset(&shell->ctx->signals[SHELL_SIGNAL_TXDONE]);
	} else {
		/* Blocking wait in case of bare metal. */
		while (!z_flag_tx_rdy_get(shell)) {
		}
		z_flag_tx_rdy_set(shell, false);
	}
}

void z_shell_write(const struct shell *shell, const void *data,
		 size_t length)
{
	__ASSERT_NO_MSG(shell && data);

	size_t offset = 0;
	size_t tmp_cnt;

	while (length) {
		int err = shell->iface->api->write(shell->iface,
				&((const uint8_t *) data)[offset], length,
				&tmp_cnt);
		(void)err;
		__ASSERT_NO_MSG(err == 0);
		__ASSERT_NO_MSG(length >= tmp_cnt);
		offset += tmp_cnt;
		length -= tmp_cnt;
		if (tmp_cnt == 0 &&
		    (shell->ctx->state != SHELL_STATE_PANIC_MODE_ACTIVE)) {
			shell_pend_on_txdone(shell);
		}
	}
}

/* Function shall be only used by the fprintf module. */
void z_shell_print_stream(const void *user_ctx, const char *data, size_t len)
{
	z_shell_write((const struct shell *) user_ctx, data, len);
}

static void vt100_bgcolor_set(const struct shell *shell,
			      enum shell_vt100_color bgcolor)
{
	if ((bgcolor == SHELL_NORMAL) ||
	    (shell->ctx->vt100_ctx.col.bgcol == bgcolor)) {
		return;
	}

	/* -1 because default value is first in enum */
	uint8_t cmd[] = SHELL_VT100_BGCOLOR(bgcolor - 1);

	shell->ctx->vt100_ctx.col.bgcol = bgcolor;
	z_shell_raw_fprintf(shell->fprintf_ctx, "%s", cmd);

}

void z_shell_vt100_color_set(const struct shell *shell,
			     enum shell_vt100_color color)
{

	if (shell->ctx->vt100_ctx.col.col == color) {
		return;
	}

	shell->ctx->vt100_ctx.col.col = color;

	if (color != SHELL_NORMAL) {

		uint8_t cmd[] = SHELL_VT100_COLOR(color - 1);

		z_shell_raw_fprintf(shell->fprintf_ctx, "%s", cmd);
	} else {
		static const uint8_t cmd[] = SHELL_VT100_MODESOFF;

		z_shell_raw_fprintf(shell->fprintf_ctx, "%s", cmd);
	}
}

void z_shell_vt100_colors_restore(const struct shell *shell,
				       const struct shell_vt100_colors *color)
{
	z_shell_vt100_color_set(shell, color->col);
	vt100_bgcolor_set(shell, color->bgcol);
}

void z_shell_vfprintf(const struct shell *shell, enum shell_vt100_color color,
		      const char *fmt, va_list args)
{
	if (IS_ENABLED(CONFIG_SHELL_VT100_COLORS) &&
	    shell->ctx->internal.flags.use_colors &&
	    (color != shell->ctx->vt100_ctx.col.col)) {
		struct shell_vt100_colors col;

		z_shell_vt100_colors_store(shell, &col);
		z_shell_vt100_color_set(shell, color);

		z_shell_fprintf_fmt(shell->fprintf_ctx, fmt, args);

		z_shell_vt100_colors_restore(shell, &col);
	} else {
		z_shell_fprintf_fmt(shell->fprintf_ctx, fmt, args);
	}
}

void z_shell_fprintf(const struct shell *shell,
			    enum shell_vt100_color color,
			    const char *fmt, ...)
{
	__ASSERT_NO_MSG(shell);
	__ASSERT(!k_is_in_isr(), "Thread context required.");
	__ASSERT_NO_MSG(shell->ctx);
	__ASSERT_NO_MSG(shell->fprintf_ctx);
	__ASSERT_NO_MSG(fmt);

	va_list args;

	va_start(args, fmt);
	z_shell_vfprintf(shell, color, fmt, args);
	va_end(args);
}
