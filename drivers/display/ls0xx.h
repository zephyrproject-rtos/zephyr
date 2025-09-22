/*
 * Copyright (c) 2020 Rohit Gujarathi
 * Copyright (c) 2025 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Supports LS012B7DD01, LS013B7DH03, LS013B7DH05
 * LS027B7DH01A, LS032B7DD02, LS044Q7DH01
 */

/* Note:
 * -> high/1 means white, low/0 means black
 * -> Display expects LSB first
 */

#define LS0XX_PANEL_WIDTH  DT_INST_PROP(0, width)
#define LS0XX_PANEL_HEIGHT DT_INST_PROP(0, height)

#define LS0XX_PIXELS_PER_BYTE 8U
/* Adding 2 for the line number and dummy byte
 * line_buf format for each row.
 * +-------------------+-------------------+----------------+
 * | line num (8 bits) | data (WIDTH bits) | dummy (8 bits) |
 * +-------------------+-------------------+----------------+
 */
#define LS0XX_BYTES_PER_LINE  ((LS0XX_PANEL_WIDTH / LS0XX_PIXELS_PER_BYTE) + 2)

#define LS0XX_BIT_WRITECMD  0x01
#define LS0XX_BIT_VCOM      0x02
#define LS0XX_BIT_CLEAR     0x04
#define LS0XX_MAX_VCOM_MSEC 1000

struct ls0xx_data {
	bool vcom_state;
};
