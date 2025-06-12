/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2025 Croxel Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define WM8731_NODE DT_NODELABEL(wm8731)
#define MAX9867_NODE DT_NODELABEL(max9867)

#if DT_ON_BUS(WM8731_NODE, i2c)
bool init_wm8731_i2c(void);
#endif

#if DT_ON_BUS(MAX9867_NODE, i2c)
bool init_max9867_i2c(void);
#endif
