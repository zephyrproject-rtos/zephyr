/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define WM8731_NODE DT_NODELABEL(wm8731)

#if DT_ON_BUS(WM8731_NODE, i2c)
bool init_wm8731_i2c(void);
#endif
