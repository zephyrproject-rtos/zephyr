/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_H__
#define __SOC_H__

#ifndef _ASMLANGUAGE
#include <soc/soc.h>
#include <rom/ets_sys.h>
#include <esp_rom_sys.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <esp_attr.h>
#include <esp_private/esp_clk.h>
#endif

/* ECALL Exception numbers */
#define SOC_MCAUSE_ECALL_EXP      11
#define SOC_MCAUSE_USER_ECALL_EXP 8

/* Interrupt Mask */
#define SOC_MCAUSE_IRQ_MASK (1 << 31)
/* Exception code Mask */
#define SOC_MCAUSE_EXP_MASK 0x7FFFFFFF

#ifndef _ASMLANGUAGE

void __esp_platform_mcuboot_start(void);
void __esp_platform_app_start(void);

static inline uint32_t esp_core_id(void)
{
	return 0;
}

extern int esp_rom_gpio_matrix_in(uint32_t gpio, uint32_t signal_index, bool inverted);
extern int esp_rom_gpio_matrix_out(uint32_t gpio, uint32_t signal_index, bool out_inverted,
				   bool out_enabled_inverted);

#endif /* _ASMLANGUAGE */

#endif /* __SOC_H__ */
