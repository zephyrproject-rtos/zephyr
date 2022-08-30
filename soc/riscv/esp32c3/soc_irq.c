/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc/rtc_cntl_reg.h>
#include <soc/timer_group_reg.h>
#include <soc/gpio_reg.h>
#include <soc/syscon_reg.h>
#include <soc/system_reg.h>
#include <soc/cache_memory.h>
#include "hal/soc_ll.h"
#include <riscv/interrupt.h>
#include <soc/interrupt_reg.h>
#include <soc/periph_defs.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32c3.h>

#include <zephyr/kernel_structs.h>
#include <string.h>
#include <zephyr/toolchain/gcc.h>
#include <soc.h>
#include <zephyr/arch/riscv/arch.h>

#define ESP32C3_INTSTATUS_SLOT1_THRESHOLD	32

void arch_irq_enable(unsigned int irq)
{
	esp_intr_enable(irq);
}

void arch_irq_disable(unsigned int irq)
{
	esp_intr_disable(irq);
}

int arch_irq_is_enabled(unsigned int irq)
{
	bool res = false;
	uint32_t key = irq_lock();

	if (irq < 32) {
		res = esp_intr_get_enabled_intmask(0) & BIT(irq);
	} else {
		res = esp_intr_get_enabled_intmask(1) & BIT(irq - 32);
	}

	irq_unlock(key);

	return res;
}

uint32_t soc_intr_get_next_source(void)
{
	uint32_t status;
	uint32_t source;

	status = REG_READ(INTERRUPT_CORE0_INTR_STATUS_0_REG) &
		esp_intr_get_enabled_intmask(0);

	if (status) {
		source = __builtin_ffs(status) - 1;
	} else {
		status = REG_READ(INTERRUPT_CORE0_INTR_STATUS_1_REG) &
			esp_intr_get_enabled_intmask(1);
		source = (__builtin_ffs(status) - 1 + ESP32C3_INTSTATUS_SLOT1_THRESHOLD);
	}

	return source;
}
