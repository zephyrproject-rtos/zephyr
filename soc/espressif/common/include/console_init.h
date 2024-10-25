/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

void esp_console_init(void);

void esp_console_deinit(void);

void esp_console_write_char_usb(char c);
