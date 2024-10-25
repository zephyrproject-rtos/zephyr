/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>

void flash_update_id(void);

int init_spi_flash(void);

#ifdef CONFIG_SOC_SERIES_ESP32
int flash_get_wp_pin(void);
#endif
