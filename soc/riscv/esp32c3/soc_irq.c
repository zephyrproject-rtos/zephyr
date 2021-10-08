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
#include <drivers/interrupt_controller/intc_esp32c3.h>

#include <kernel_structs.h>
#include <string.h>
#include <toolchain/gcc.h>
#include <soc.h>

#define ESP32C3_INTC_DEFAULT_PRIO			15
#define ESP32C3_INTSTATUS_SLOT1_THRESHOLD	32

void arch_irq_enable(unsigned int irq)
{
	uint32_t key = irq_lock();

	esprv_intc_int_set_priority(irq, ESP32C3_INTC_DEFAULT_PRIO);
	esprv_intc_int_set_type(irq, INTR_TYPE_LEVEL);
	esprv_intc_int_enable(1 << irq);
	irq_unlock(key);
}

void arch_irq_disable(unsigned int irq)
{
	esprv_intc_int_disable(1 << irq);
}

int arch_irq_is_enabled(unsigned int irq)
{
	return (REG_READ(INTERRUPT_CORE0_CPU_INT_ENABLE_REG) & (1 << irq));
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
