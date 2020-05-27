/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include "shell_ops.h"

void shell_op_cursor_vert_move(const struct shell *shell, int32_t delta)
{
	if (delta != 0) {
		shell_raw_fprintf(shell->fprintf_ctx, "\033[%d%c",
				  delta > 0 ? delta : -delta,
				  delta > 0 ? 'A' : 'B');
	}
}

void shell_op_cursor_horiz_move(const struct shell *shell, int32_t delta)
{
	if (delta != 0) {
		shell_raw_fprintf(shell->fprintf_ctx, "\033[%d%c",
				  delta > 0 ? delta : -delta,
				  delta > 0 ? 'C' : 'D');
	}
}

/* Function returns true if command length is equal to multiplicity of terminal
 * width.
 */
static inline bool full_line_cmd(const struct shell *shell)
{
	return ((shell->ctx->cmd_buff_len + shell_strlen(shell->ctx->prompt))
			% shell->ctx->vt100_ctx.cons.terminal_wid == 0U);
}

/* Function returns true if cursor is at beginning of an empty line. */
bool shell_cursor_in_empty_line(const struct shell *shell)
{
	return ((shell->ctx->cmd_buff_pos + shell_strlen(shell->ctx->prompt))
			% shell->ctx->vt100_ctx.cons.terminal_wid == 0U);
}

void shell_op_cond_next_line(const struct shell *shell)
{
	if (shell_cursor_in_empty_line(shell) || full_line_cmd(shell)) {
		cursor_next_line_move(shell);
	}
}

void shell_op_cursor_position_synchronize(const struct shell *shell)
{
	struct shell_multiline_cons *cons = &shell->ctx->vt100_ctx.cons;
	bool last_line;

	shell_multiline_data_calc(cons, shell->ctx->cmd_buff_pos,
				  shell->ctx->cmd_buff_len);
	last_line = (cons->cur_y == cons->cur_y_end);

	/* In case cursor reaches the bottom line of a terminal, it will
	 * be moved to the next line.
	 */
	if (full_line_cmd(shell)) {
		cursor_next_line_move(shell);
	}

	if (last_line) {
		shell_op_cursor_horiz_move(shell, cons->cur_x -
							       cons->cur_x_end);
	} else {
		shell_op_cursor_vert_move(shell, cons->cur_y_end - cons->cur_y);
		shell_op_cursor_horiz_move(shell, cons->cur_x -
							       cons->cur_x_end);
	}
}

void shell_op_cursor_move(const struct shell *shell, int16_t val)
{
	struct shell_multiline_cons *cons = &shell->ctx->vt100_ctx.cons;
	uint16_t new_pos = shell->ctx->cmd_buff_pos + val;
	int32_t row_span;
	int32_t col_span;

	shell_multiline_data_calc(cons, shell->ctx->cmd_buff_pos,
				  shell->ctx->cmd_buff_len);

	/* Calculate the new cursor. */
	row_span = row_span_with_buffer_offsets_get(&shell->ctx->vt100_ctx.cons,
						    shell->ctx->cmd_buff_pos,
						    new_pos);
	col_span = column_span_with_buffer_offsets_get(
						    &shell->ctx->vt100_ctx.cons,
						    shell->ctx->cmd_buff_pos,
						    new_pos);

	shell_op_cursor_vert_move(shell, -row_span);
	shell_op_cursor_horiz_move(shell, col_span);
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

void shell_op_cursor_word_move(const struct shell *shell, int16_t val)
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
		shell_op_cursor_move(shell, sign * shift);
	}
}

void shell_op_word_remove(const struct shell *shell)
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
	shell_op_cursor_move(shell, -chars_to_delete);
	cursor_save(shell);
	shell_internal_fprintf(shell, SHELL_NORMAL, "%s", str + 1);
	clear_eos(shell);
	cursor_restore(shell);
}

void shell_op_cursor_home_move(const struct shell *shell)
{
	shell_op_cursor_move(shell, -shell->ctx->cmd_buff_pos);
}

void shell_op_cursor_end_move(const struct shell *shell)
{
	shell_op_cursor_move(shell, shell->ctx->cmd_buff_len -
						shell->ctx->cmd_buff_pos);
}


void shell_op_left_arrow(const struct shell *shell)
{
	if (shell->ctx->cmd_buff_pos > 0) {
		shell_op_cursor_move(shell, -1);
	}
}

void shell_op_right_arrow(const struct shell *shell)
{
	if (shell->ctx->cmd_buff_pos < shell->ctx->cmd_buff_len) {
		shell_op_cursor_move(shell, 1);
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
		clear_eos(shell);
	}

	shell_internal_fprintf(shell, SHELL_NORMAL, "%s",
		      &shell->ctx->cmd_buff[shell->ctx->cmd_buff_pos]);
	shell->ctx->cmd_buff_pos = shell->ctx->cmd_buff_len;

	if (full_line_cmd(shell)) {
		if (((data_removed) && (diff > 0)) || (!data_removed)) {
			cursor_next_line_move(shell);
		}
	}

	shell_op_cursor_move(shell, -diff);
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

	if (!flag_echo_get(shell)) {
		shell->ctx->cmd_buff_pos += len;
		return;
	}

	reprint_from_cursor(shell, after, false);
}

static void char_replace(const struct shell *shell, char data)
{
	shell->ctx->cmd_buff[shell->ctx->cmd_buff_pos++] = data;

	if (!flag_echo_get(shell)) {
		return;
	}

	shell_raw_fprintf(shell->fprintf_ctx, "%c", data);
	if (shell_cursor_in_empty_line(shell)) {
		cursor_next_line_move(shell);
	}
}

void shell_op_char_insert(const struct shell *shell, char data)
{
	if (shell->ctx->internal.flags.insert_mode &&
		(shell->ctx->cmd_buff_len != shell->ctx->cmd_buff_pos)) {
		char_replace(shell, data);
	} else {
		data_insert(shell, &data, 1);
	}
}

void shell_op_char_backspace(const struct shell *shell)
{
	if ((shell->ctx->cmd_buff_len == 0) ||
	    (shell->ctx->cmd_buff_pos == 0)) {
		return;
	}

	shell_op_cursor_move(shell, -1);
	shell_op_char_delete(shell);
}

void shell_op_char_delete(const struct shell *shell)
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

void shell_op_delete_from_cursor(const struct shell *shell)
{
	shell->ctx->cmd_buff_len = shell->ctx->cmd_buff_pos;
	shell->ctx->cmd_buff[shell->ctx->cmd_buff_pos] = '\0';

	clear_eos(shell);
}

void shell_op_completion_insert(const struct shell *shell,
				const char *compl,
				uint16_t compl_len)
{
	data_insert(shell, compl, compl_len);
}

void shell_cmd_line_erase(const struct shell *shell)
{
	shell_multiline_data_calc(&shell->ctx->vt100_ctx.cons,
				  shell->ctx->cmd_buff_pos,
				  shell->ctx->cmd_buff_len);
	shell_op_cursor_horiz_move(shell,
				   -(shell->ctx->vt100_ctx.cons.cur_x - 1));
	shell_op_cursor_vert_move(shell, shell->ctx->vt100_ctx.cons.cur_y - 1);

	clear_eos(shell);
}

static void print_prompt(const struct shell *shell)
{
	shell_internal_fprintf(shell, SHELL_INFO, "%s", shell->ctx->prompt);
}

void shell_print_cmd(const struct shell *shell)
{
	shell_raw_fprintf(shell->fprintf_ctx, "%s", shell->ctx->cmd_buff);
}

void shell_print_prompt_and_cmd(const struct shell *shell)
{
	print_prompt(shell);

	if (flag_echo_get(shell)) {
		shell_print_cmd(shell);
		shell_op_cursor_position_synchronize(shell);
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
		while (!flag_tx_rdy_get(shell)) {
		}
		flag_tx_rdy_set(shell, false);
	}
}

void shell_write(const struct shell *shell, const void *data,
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
void shell_print_stream(const void *user_ctx, const char *data,
			size_t data_len)
{
	shell_write((const struct shell *) user_ctx, data, data_len);
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
	shell_raw_fprintf(shell->fprintf_ctx, "%s", cmd);

}

void shell_vt100_color_set(const struct shell *shell,
			   enum shell_vt100_color color)
{

	if (shell->ctx->vt100_ctx.col.col == color) {
		return;
	}

	shell->ctx->vt100_ctx.col.col = color;

	if (color != SHELL_NORMAL) {

		uint8_t cmd[] = SHELL_VT100_COLOR(color - 1);

		shell_raw_fprintf(shell->fprintf_ctx, "%s", cmd);
	} else {
		static const uint8_t cmd[] = SHELL_VT100_MODESOFF;

		shell_raw_fprintf(shell->fprintf_ctx, "%s", cmd);
	}
}

void shell_vt100_colors_restore(const struct shell *shell,
				       const struct shell_vt100_colors *color)
{
	shell_vt100_color_set(shell, color->col);
	vt100_bgcolor_set(shell, color->bgcol);
}

void shell_internal_vfprintf(const struct shell *shell,
			     enum shell_vt100_color color, const char *fmt,
			     va_list args)
{
	if (IS_ENABLED(CONFIG_SHELL_VT100_COLORS) &&
	    shell->ctx->internal.flags.use_colors &&
	    (color != shell->ctx->vt100_ctx.col.col)) {
		struct shell_vt100_colors col;

		shell_vt100_colors_store(shell, &col);
		shell_vt100_color_set(shell, color);

		shell_fprintf_fmt(shell->fprintf_ctx, fmt, args);

		shell_vt100_colors_restore(shell, &col);
	} else {
		shell_fprintf_fmt(shell->fprintf_ctx, fmt, args);
	}
}

void shell_internal_fprintf(const struct shell *shell,
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
	shell_internal_vfprintf(shell, color, fmt, args);
	va_end(args);
}
