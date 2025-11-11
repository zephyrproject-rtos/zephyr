/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <esp_efuse.h>

static inline void esp_efuse_init_virtual(void)
{
#if CONFIG_ESP32_EFUSE_VIRTUAL
#if CONFIG_ESP32_EFUSE_VIRTUAL_KEEP_IN_FLASH
	esp_efuse_init_virtual_mode_in_flash(CONFIG_ESP32_EFUSE_VIRTUAL_OFFSET,
					     CONFIG_ESP32_EFUSE_VIRTUAL_SIZE);
#else
	esp_efuse_init_virtual_mode_in_ram();
#endif
#endif
}
