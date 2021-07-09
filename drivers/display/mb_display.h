/*
 * Copyright (c) 2020 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BOARD_BBC_MICROBIT)
#define GPIO_PORTS      1
#define DISPLAY_ROWS    3
#define DISPLAY_COLS    9
#else
#define GPIO_PORTS      2
#define DISPLAY_ROWS    5
#define DISPLAY_COLS    5
#endif

void mb_start_image(const struct mb_image *img,
		    uint32_t rows[DISPLAY_ROWS][GPIO_PORTS]);
void mb_update_pins(uint8_t cur, const uint32_t val[GPIO_PORTS]);

extern const uint32_t col_mask[GPIO_PORTS];
