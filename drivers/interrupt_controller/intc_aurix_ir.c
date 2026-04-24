/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/intc_aurix_ir.h>
#include "intc_aurix_ir_priv.h"

#define DT_DRV_COMPAT infineon_aurix_ir

void intc_aurix_ir_irq_config(unsigned int irq, unsigned int priority, unsigned int flags)
{
	SRCR reg = {
		.B.SRPN = priority,
		.B.TOS = (flags & IRQ_USE_TOS) ? FIELD_GET(IRQ_TOS, flags) : CONFIG_AURIX_CORE_TOS,
		.B.CLRR = 1,
		.B.IOVCLR = 1,
#if IS_ENABLED(CONFIG_SOC_SERIES_TC4X)
		.B.CS = CONFIG_TRICORE_CORE_ID == 6 ? 1 : 0,
#endif
#if IS_ENABLED(CONFIG_SOC_SERIES_TC3X)
		.B.SWSCLR = 1,
#endif
	};
	sys_write32(reg.U, GET_SRC(irq));
}
void intc_aurix_ir_irq_enable(unsigned int irq)
{
	SRCR reg = {.U = sys_read32(GET_SRC(irq))};

	reg.B.SRE = 1;
	sys_write32(reg.U, GET_SRC(irq));
}

void intc_aurix_ir_irq_disable(unsigned int irq)
{
	SRCR reg = {.U = sys_read32(GET_SRC(irq))};

	reg.B.SRE = 0;
	sys_write32(reg.U, GET_SRC(irq));
}

bool intc_aurix_ir_irq_is_enabled(unsigned int irq)
{
	SRCR reg = {.U = sys_read32(GET_SRC(irq))};

	return (reg.B.SRE == 1);
}

bool intc_aurix_ir_irq_is_pending(unsigned int irq)
{
	SRCR reg = {.U = sys_read32(GET_SRC(irq))};

	return (reg.B.SRR == 1);
}

void intc_aurix_ir_irq_clear_pending(unsigned int irq)
{
	SRCR reg = {.U = sys_read32(GET_SRC(irq))};

	reg.B.CLRR = 1;
	sys_write32(reg.U, GET_SRC(irq));
}

unsigned int intc_aurix_ir_get_active(void)
{
#if defined(CONFIG_SOC_SERIES_TC4X)
	uint32_t lasr = sys_read32(IR_BASE + 0x0C20 + GET_TOS(arch_proc_id()) * 0x34);

	return ((lasr >> 16) & 0x7FF);
#else
	uint32_t lasr = sys_read32(IR_BASE + 0x204 + GET_TOS(arch_proc_id()) * 0x10);

	return ((lasr >> 16) & 0x3FF);
#endif
}

void intc_aurix_ir_irq_raise(unsigned int irq)
{
	SRCR reg = {.U = sys_read32(GET_SRC(irq))};

	reg.B.SETR = 1;
	sys_write32(reg.U, GET_SRC(irq));
}

void intc_aurix_ir_src_raise(uintptr_t src)
{
	SRCR reg = {.U = sys_read32(src)};

	reg.B.SETR = 1;
	sys_write32(reg.U, src);
}

/**
 * @brief Initialize the aurix ir device driver
 */
int intc_aurix_ir_init(const struct device *dev)
{
	return 0;
}

DEVICE_DT_INST_DEFINE(0, intc_aurix_ir_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_INTC_INIT_PRIORITY, NULL);
