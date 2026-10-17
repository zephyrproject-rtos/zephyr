/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/display/display_console.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "display_console_renderer.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_console, CONFIG_DISPLAY_CONSOLE_LOG_LEVEL);

#define DISPLAY_CONSOLE_MAX_ROWS   CONFIG_DISPLAY_CONSOLE_MAX_ROWS
#define DISPLAY_CONSOLE_MAX_COLS   CONFIG_DISPLAY_CONSOLE_MAX_COLS
#define DISPLAY_CONSOLE_CSI_PARAMS 4

const struct display_console_rgb display_console_fg_rgb[DISPLAY_CONSOLE_ATTR_COUNT] = {
	[DISPLAY_CONSOLE_ATTR_DEFAULT] = {0xff, 0xff, 0xff},
	[DISPLAY_CONSOLE_ATTR_RED] = {0xff, 0x50, 0x50},
	[DISPLAY_CONSOLE_ATTR_GREEN] = {0x50, 0xe0, 0x50},
	[DISPLAY_CONSOLE_ATTR_YELLOW] = {0xff, 0xd0, 0x40},
	[DISPLAY_CONSOLE_ATTR_CYAN] = {0x40, 0xd0, 0xf0},
};

const struct display_console_rgb display_console_bg_rgb = {0x00, 0x00, 0x00};

/* Parser state for the control sequences. */
enum display_console_esc_state {
	DISPLAY_CONSOLE_ESC_NONE,
	DISPLAY_CONSOLE_ESC_SEEN,
	DISPLAY_CONSOLE_ESC_CSI,
};

/*
 * Text grid shown on the display. Rows form a ring so that scrolling only
 * advances @c top. Cursor coordinates are relative to @c top.
 */
struct display_console {
	struct k_spinlock lock;
	struct k_work_delayable flush_work;
	char text[DISPLAY_CONSOLE_MAX_ROWS][DISPLAY_CONSOLE_MAX_COLS];
	uint8_t attr[DISPLAY_CONSOLE_MAX_ROWS][DISPLAY_CONSOLE_MAX_COLS];
	bool dirty[DISPLAY_CONSOLE_MAX_ROWS];
	uint16_t rows;
	uint16_t cols;
	uint16_t top;
	uint16_t row;
	uint16_t col;
	uint16_t saved_row;
	uint16_t saved_col;
	uint8_t cur_attr;
	enum display_console_esc_state esc;
	uint16_t csi[DISPLAY_CONSOLE_CSI_PARAMS];
	uint8_t csi_n;
	bool csi_private;
	bool ready;
	int init_err;
	uint32_t render_errors;
	/* Set after a fatal error; rows are then drawn as they are written. */
	atomic_t panic;
};

static struct display_console display_console_inst;
/* Copy of the grid taken under the lock so the renderer runs without it. */
static struct display_console_snapshot display_console_snap;

static uint16_t display_console_phys_row(const struct display_console *t, uint16_t row)
{
	return (t->top + row) % t->rows;
}

static void display_console_clear_cells(struct display_console *t, uint16_t row, uint16_t from,
					uint16_t to)
{
	const uint16_t phys = display_console_phys_row(t, row);

	if (from >= to) {
		return;
	}

	memset(&t->text[phys][from], ' ', to - from);
	memset(&t->attr[phys][from], DISPLAY_CONSOLE_ATTR_DEFAULT, to - from);
	t->dirty[phys] = true;
}

static void display_console_clear_row(struct display_console *t, uint16_t row)
{
	display_console_clear_cells(t, row, 0, t->cols);
}

static void display_console_mark_all_dirty(struct display_console *t)
{
	for (uint16_t r = 0; r < t->rows; r++) {
		t->dirty[r] = true;
	}
}

static void display_console_newline(struct display_console *t)
{
	t->col = 0;

	if (t->row + 1U < t->rows) {
		t->row++;
		return;
	}

	t->top = (t->top + 1U) % t->rows;
	display_console_clear_row(t, t->row);
	display_console_mark_all_dirty(t);
}

static uint16_t display_console_csi_param(const struct display_console *t, uint8_t idx,
					  uint16_t def)
{
	if (idx >= t->csi_n || t->csi[idx] == 0U) {
		return def;
	}

	return t->csi[idx];
}

static void display_console_apply_sgr(struct display_console *t)
{
	if (t->csi_n == 0U) {
		t->cur_attr = DISPLAY_CONSOLE_ATTR_DEFAULT;
		return;
	}

	for (uint8_t i = 0; i < t->csi_n; i++) {
		switch (t->csi[i]) {
		case 0:
			t->cur_attr = DISPLAY_CONSOLE_ATTR_DEFAULT;
			break;
		case 31:
			t->cur_attr = DISPLAY_CONSOLE_ATTR_RED;
			break;
		case 32:
			t->cur_attr = DISPLAY_CONSOLE_ATTR_GREEN;
			break;
		case 33:
			t->cur_attr = DISPLAY_CONSOLE_ATTR_YELLOW;
			break;
		case 36:
			t->cur_attr = DISPLAY_CONSOLE_ATTR_CYAN;
			break;
		default:
			break;
		}
	}
}

static void display_console_apply_csi(struct display_console *t, char final)
{
	uint16_t n;

	if (t->csi_private) {
		return;
	}

	switch (final) {
	case 'A':
		n = display_console_csi_param(t, 0, 1);
		t->row = (n > t->row) ? 0 : t->row - n;
		break;
	case 'B':
		n = display_console_csi_param(t, 0, 1);
		t->row = MIN(t->row + n, t->rows - 1U);
		break;
	case 'C':
		n = display_console_csi_param(t, 0, 1);
		t->col = MIN(t->col + n, t->cols - 1U);
		break;
	case 'D':
		n = display_console_csi_param(t, 0, 1);
		t->col = (n > t->col) ? 0 : t->col - n;
		break;
	case 'H':
	case 'f':
		t->row = MIN(display_console_csi_param(t, 0, 1), t->rows) - 1U;
		t->col = MIN(display_console_csi_param(t, 1, 1), t->cols) - 1U;
		break;
	case 'J':
		n = display_console_csi_param(t, 0, 0);
		if (n == 2U) {
			for (uint16_t r = 0; r < t->rows; r++) {
				display_console_clear_row(t, r);
			}
		} else if (n == 0U) {
			display_console_clear_cells(t, t->row, t->col, t->cols);
			for (uint16_t r = t->row + 1U; r < t->rows; r++) {
				display_console_clear_row(t, r);
			}
		}
		break;
	case 'K':
		n = display_console_csi_param(t, 0, 0);
		if (n == 0U) {
			display_console_clear_cells(t, t->row, t->col, t->cols);
		} else if (n == 1U) {
			display_console_clear_cells(t, t->row, 0, MIN(t->col + 1U, t->cols));
		} else if (n == 2U) {
			display_console_clear_row(t, t->row);
		}
		break;
	case 'm':
		display_console_apply_sgr(t);
		break;
	case 's':
		t->saved_row = t->row;
		t->saved_col = t->col;
		break;
	case 'u':
		t->row = t->saved_row;
		t->col = t->saved_col;
		break;
	default:
		break;
	}
}

static void display_console_putc(struct display_console *t, char c)
{
	uint16_t phys;

	switch (t->esc) {
	case DISPLAY_CONSOLE_ESC_SEEN:
		t->esc = DISPLAY_CONSOLE_ESC_NONE;
		if (c == '[') {
			t->esc = DISPLAY_CONSOLE_ESC_CSI;
			t->csi_n = 0;
			t->csi[0] = 0;
			t->csi_private = false;
		} else if (c == '7') {
			t->saved_row = t->row;
			t->saved_col = t->col;
		} else if (c == '8') {
			t->row = t->saved_row;
			t->col = t->saved_col;
		}
		return;
	case DISPLAY_CONSOLE_ESC_CSI:
		if (c >= '0' && c <= '9') {
			if (t->csi_n < DISPLAY_CONSOLE_CSI_PARAMS) {
				t->csi[t->csi_n] = t->csi[t->csi_n] * 10U + (uint16_t)(c - '0');
			}
		} else if (c == ';') {
			if (t->csi_n < DISPLAY_CONSOLE_CSI_PARAMS) {
				t->csi_n++;
			}
			if (t->csi_n < DISPLAY_CONSOLE_CSI_PARAMS) {
				t->csi[t->csi_n] = 0;
			}
		} else if (c >= 0x40 && c <= 0x7e) {
			if (t->csi_n < DISPLAY_CONSOLE_CSI_PARAMS) {
				t->csi_n++;
			}
			t->esc = DISPLAY_CONSOLE_ESC_NONE;
			display_console_apply_csi(t, c);
		} else if (c >= 0x20 && c <= 0x3f) {
			/* Private marker or intermediate byte: not a VT100 sequence. */
			t->csi_private = true;
		} else {
			t->esc = DISPLAY_CONSOLE_ESC_NONE;
		}
		return;
	default:
		break;
	}

	switch (c) {
	case 0x1b:
		t->esc = DISPLAY_CONSOLE_ESC_SEEN;
		break;
	case '\r':
		t->col = 0;
		break;
	case '\n':
		display_console_newline(t);
		break;
	case '\b':
		if (t->col > 0) {
			t->col--;
		}
		break;
	case '\t':
		do {
			display_console_putc(t, ' ');
		} while ((t->col % 8U) != 0U && t->col < t->cols);
		break;
	default:
		if (c < ' ' || c > '~') {
			break;
		}
		if (t->col >= t->cols) {
			display_console_newline(t);
		}
		phys = display_console_phys_row(t, t->row);
		t->text[phys][t->col] = c;
		t->attr[phys][t->col] = t->cur_attr;
		t->dirty[phys] = true;
		t->col++;
		break;
	}
}

/* Copy the grid into the snapshot and clear the dirty flags; the caller holds the lock. */
static void display_console_snapshot_take(struct display_console *t)
{
	struct display_console_snapshot *s = &display_console_snap;

	memcpy(s->text, t->text, sizeof(s->text));
	memcpy(s->attr, t->attr, sizeof(s->attr));
	memcpy(s->dirty, t->dirty, sizeof(s->dirty));
	memset(t->dirty, 0, sizeof(t->dirty));
	s->top = t->top;
	s->rows = t->rows;
	s->cols = t->cols;
}

static void display_console_render(struct display_console *t)
{
	k_spinlock_key_t key;
	int err;

	key = k_spin_lock(&t->lock);
	display_console_snapshot_take(t);
	k_spin_unlock(&t->lock, key);

	err = display_console_renderer.render(&display_console_snap);
	if (err != 0) {
		t->render_errors++;
	}
}

/* Draw from a fatal error; the faulting code may hold the lock, so skip it. */
static void display_console_render_panic(struct display_console *t)
{
	int err;

	display_console_snapshot_take(t);

	err = display_console_renderer.render(&display_console_snap);
	if (err != 0) {
		t->render_errors++;
	}
}

static void display_console_flush_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct display_console *t = CONTAINER_OF(dwork, struct display_console, flush_work);

	display_console_render(t);
}

static void display_console_schedule(struct display_console *t)
{
	(void)k_work_schedule(&t->flush_work, K_MSEC(CONFIG_DISPLAY_CONSOLE_FLUSH_DELAY_MS));
}

void display_console_write(const uint8_t *data, size_t len)
{
	struct display_console *t = &display_console_inst;
	k_spinlock_key_t key;

	if (!t->ready) {
		return;
	}

	if (atomic_get(&t->panic) != 0) {
		for (size_t i = 0; i < len; i++) {
			display_console_putc(t, (char)data[i]);
		}

		if (memchr(data, '\n', len) != NULL) {
			display_console_render_panic(t);
		}

		return;
	}

	key = k_spin_lock(&t->lock);
	for (size_t i = 0; i < len; i++) {
		display_console_putc(t, (char)data[i]);
	}
	k_spin_unlock(&t->lock, key);

	display_console_schedule(t);
}

void display_console_redraw(void)
{
	struct display_console *t = &display_console_inst;
	k_spinlock_key_t key;

	if (!t->ready) {
		return;
	}

	key = k_spin_lock(&t->lock);
	display_console_mark_all_dirty(t);
	k_spin_unlock(&t->lock, key);

	display_console_schedule(t);
}

void display_console_flush(void)
{
	struct display_console *t = &display_console_inst;

	if (!t->ready) {
		return;
	}

	if (atomic_get(&t->panic) != 0) {
		display_console_render_panic(t);
		return;
	}

	if (k_is_in_isr()) {
		display_console_schedule(t);
		return;
	}

	display_console_render(t);

	/*
	 * A render in progress on the work queue shares the snapshot with the
	 * one above; have it redraw every row afterwards.
	 */
	display_console_redraw();
}

void display_console_panic(void)
{
	struct display_console *t = &display_console_inst;

	if (!t->ready || !atomic_cas(&t->panic, 0, 1)) {
		return;
	}

	display_console_mark_all_dirty(t);
	display_console_render_panic(t);
}

bool display_console_ready(void)
{
	return display_console_inst.ready;
}

int display_console_size(uint16_t *rows, uint16_t *cols)
{
	struct display_console *t = &display_console_inst;

	if (!t->ready) {
		return -ENODEV;
	}

	*rows = t->rows;
	*cols = t->cols;

	return 0;
}

static int display_console_setup(struct display_console *t)
{
	uint16_t rows = 0;
	uint16_t cols = 0;
	int err;

	err = display_console_renderer.init(&rows, &cols);
	if (err != 0) {
		return err;
	}

	t->rows = MIN(rows, DISPLAY_CONSOLE_MAX_ROWS);
	t->cols = MIN(cols, DISPLAY_CONSOLE_MAX_COLS);
	if (t->rows == 0U || t->cols == 0U) {
		LOG_ERR("display too small for the font");
		return -ENOSPC;
	}

	for (uint16_t r = 0; r < t->rows; r++) {
		display_console_clear_row(t, r);
	}

	k_work_init_delayable(&t->flush_work, display_console_flush_work);

	return 0;
}

int display_console_init(void)
{
	struct display_console *t = &display_console_inst;
	static bool done;

	if (done) {
		return t->init_err;
	}
	done = true;

	t->init_err = display_console_setup(t);
	t->ready = (t->init_err == 0);

	return t->init_err;
}
