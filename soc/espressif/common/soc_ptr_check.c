/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_memory_utils.h>
#include <zephyr/arch/xtensa/xtensa_ptr.h>
#include <zephyr/sys/util.h>

bool IRAM_ATTR xtensa_soc_ptr_executable(const void *p)
{
	return esp_ptr_executable(p);
}

bool IRAM_ATTR xtensa_soc_stack_ptr_is_sane(uint32_t sp)
{
	if (esp_stack_ptr_is_sane(sp)) {
		return true;
	}

	if (IS_ENABLED(CONFIG_ESP_SPIRAM)) {
		return esp_ptr_external_ram((void *)sp) && ((sp & 0xF) == 0U);
	}

	return false;
}
