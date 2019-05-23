/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "shared-page.h"

/* Super-primitive VGA text console output-only "terminal" driver */

static inline unsigned short *vga_row(int row)
{
	return ((unsigned short *)0xb8000) + 80 * row;
}

/* Foreground color is four bit, high to low: "intensity", red, green,
 * blue.  Normal text is low intensity, so 0b0111 (7) is standard.
 * The high nybble is the background color.
 */
static inline void vga_put(int ch, int color, int row, int col)
{
	unsigned short *rp = vga_row(row);

	rp[col] = (color << 8) | ch;
}

static inline void vgacon_putc(char c)
{
	if (_shared.vgacol == 80) {
		for (int r = 0; r < 24; r++) {
			for (int c = 0; c < 80; c++) {
				vga_row(r)[c] = vga_row(r+1)[c];
			}
		}
		for (int c = 0; c < 80; c++) {
			vga_row(24)[c] = 0x9000;
		}
		_shared.vgacol = 0;
	}

	if (c == '\n') {
		_shared.vgacol = 80;
	} else if (c == '\r') {
		_shared.vgacol = 0;
	} else {
		vga_put(c, 0x1f, 24, _shared.vgacol++);
	}
}
