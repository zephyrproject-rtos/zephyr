/*
 * Copyright (c) 2023 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc/timer_group_reg.h>
#include <soc/ext_mem_defs.h>
#include <soc/gpio_reg.h>
#include <soc/system_reg.h>
#include <riscv/interrupt.h>
#include <soc/interrupt_reg.h>
#include <soc/periph_defs.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32c3.h>

#include <zephyr/kernel_structs.h>
#include <string.h>
#include <zephyr/toolchain/gcc.h>
#include <soc.h>
#include <zephyr/arch/riscv/arch.h>

#define ESP32C6_INTSTATUS_REG1_THRESHOLD	32
#define ESP32C6_INTSTATUS_REG2_THRESHOLD	64

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
	} else if (irq < 64) {
		res = esp_intr_get_enabled_intmask(1) & BIT(irq - 32);
	} else {
		res = esp_intr_get_enabled_intmask(2) & BIT(irq - 64);
	}

	irq_unlock(key);

	return res;
}

uint32_t soc_intr_get_next_source(void)
{
	uint32_t status;
	uint32_t source;

	/* Status register for interrupt sources 0 ~ 31 */
	status = REG_READ(INTMTX_CORE0_INT_STATUS_REG_0_REG) &
		esp_intr_get_enabled_intmask(0);

	if (status) {
		source = __builtin_ffs(status) - 1;
		goto ret;
	}

	/* Status register for interrupt sources 32 ~ 63 */
	status = REG_READ(INTMTX_CORE0_INT_STATUS_REG_1_REG) &
		esp_intr_get_enabled_intmask(1);

	if (status) {
		source = (__builtin_ffs(status) - 1 + ESP32C6_INTSTATUS_REG1_THRESHOLD);
		goto ret;
	}

	/* Status register for interrupt sources 64 ~ 76 */
	status = REG_READ(INTMTX_CORE0_INT_STATUS_REG_2_REG) &
		esp_intr_get_enabled_intmask(2);

	source = (__builtin_ffs(status) - 1 + ESP32C6_INTSTATUS_REG2_THRESHOLD);

ret:
	return source;
}
