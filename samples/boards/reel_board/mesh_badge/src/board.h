/*
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void board_refresh_display(void);
void board_show_text(const char *text, bool center, s32_t duration);
void board_blink_leds(void);
int board_init(void);
