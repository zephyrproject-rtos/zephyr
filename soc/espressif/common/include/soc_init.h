/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

void soc_hw_init(void);

void ana_super_wdt_reset_config(bool enable);

void ana_bod_reset_config(bool enable);

void ana_clock_glitch_reset_config(bool enable);

void ana_reset_config(void);
