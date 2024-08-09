/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#ifndef __DEVICE_POWER_H
#define __DEVICE_POWER_H

void rts5912_light_sleep_close_module(void);
void rts5912_light_sleep_open_module(void);

void rts5912_deep_sleep_close_module(void);
void rts5912_deep_sleep_open_module(void);

#endif
