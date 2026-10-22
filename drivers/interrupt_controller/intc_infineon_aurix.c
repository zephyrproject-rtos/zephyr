/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/intc_infineon_aurix.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>

#define DT_DRV_COMPAT infineon_aurix_ir

#define GET_SRC(irq) (SRC_BASE + (irq) * 4)

#define SRC_SRPN_MASK GENMASK(7, 0)
#define SRC_SRR       BIT(24)
#define SRC_CLRR      BIT(25)
#define SRC_SETR      BIT(26)
#define SRC_IOVCLR    BIT(28)
#if CONFIG_SOC_SERIES_TC3X
#define SRC_SRE          BIT(10)
#define SRC_TOS_MASK     GENMASK(13, 11)
#define SRC_SWSCLR       BIT(30)
#define INT_LASR(tos)    (IR_BASE + 0x200 + (tos) * 0x10 + 0x4)
#define INT_LASR_ID_MASK GENMASK(25, 16)
#define aurix_coreid_to_tos(core_id) ((core_id) == 0 ? (core_id) : (core_id) + 1)
#elif CONFIG_SOC_SERIES_TC4X
#define SRC_SRE          BIT(23)
#define SRC_TOS_MASK     GENMASK(15, 12)
#define INT_LASR(tos)    (IR_BASE + 0xC00 + (tos) * 0x34 + 0x20)
#define INT_LASR_ID_MASK GENMASK(26, 16)
#define aurix_coreid_to_tos(core_id) (core_id)
#endif

void intc_aurix_ir_irq_config(unsigned int irq, unsigned int priority, unsigned int flags)
{
	__ASSERT(irq < CONFIG_NUM_IRQS, "TriCore SRC index %u out of range", irq);

	uint32_t tos = (flags & IRQ_USE_TOS) ? FIELD_GET(IRQ_TOS, flags)
					     : aurix_coreid_to_tos(arch_proc_id());
	uint32_t reg = FIELD_PREP(SRC_SRPN_MASK, priority) | FIELD_PREP(SRC_TOS_MASK, tos) |
		       SRC_CLRR | SRC_IOVCLR;

#if CONFIG_SOC_SERIES_TC3X
	reg |= SRC_SWSCLR;
#endif
	sys_write32(reg, GET_SRC(irq));
}

void intc_aurix_ir_irq_enable(unsigned int irq)
{
	sys_write32(sys_read32(GET_SRC(irq)) | SRC_SRE, GET_SRC(irq));
}

void intc_aurix_ir_irq_disable(unsigned int irq)
{
	sys_write32(sys_read32(GET_SRC(irq)) & ~SRC_SRE, GET_SRC(irq));
}

bool intc_aurix_ir_irq_is_enabled(unsigned int irq)
{
	return (sys_read32(GET_SRC(irq)) & SRC_SRE) != 0;
}

bool intc_aurix_ir_irq_is_pending(unsigned int irq)
{
	return (sys_read32(GET_SRC(irq)) & SRC_SRR) != 0;
}

void intc_aurix_ir_irq_clear_pending(unsigned int irq)
{
	sys_write32(sys_read32(GET_SRC(irq)) | SRC_CLRR, GET_SRC(irq));
}

unsigned int intc_aurix_ir_get_active(void)
{
	uint32_t tos = aurix_coreid_to_tos(arch_proc_id());

	return FIELD_GET(INT_LASR_ID_MASK, sys_read32(INT_LASR(tos)));
}

void intc_aurix_ir_irq_raise(unsigned int irq)
{
	sys_write32(sys_read32(GET_SRC(irq)) | SRC_SETR, GET_SRC(irq));
}

void intc_aurix_ir_src_raise(uintptr_t src)
{
	sys_write32(sys_read32(src) | SRC_SETR, src);
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
