/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MicroBlaze V interrupt handling support
 *
 * Connects the RISC-V interrupt architecture layer with the
 * Xilinx AXI Interrupt Controller used as a second-level
 * interrupt controller.
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/arch/riscv/irq.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/irq_nextlevel.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "sw_isr_common.h"

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

/**
 * @brief Enable an interrupt.
 *
 * First-level interrupts are enabled through the RISC-V MIE CSR.
 * Second-level interrupts are enabled through the associated
 * AXI Interrupt Controller instance.
 *
 * @param irq Interrupt number.
 */
void arch_irq_enable(unsigned int irq)
{
	unsigned int level = irq_get_level(irq);

	if (level == 1U) {
		/*
		 * CSR mie register is updated using the atomic CSRRS
		 * instruction (atomic read and set bits in CSR register).
		 */
		(void)csr_read_set(mie, BIT(irq));

	} else if (level == 2U) {
		const struct device *dev = z_get_sw_isr_device_from_irq(irq);
		unsigned int line = irq_from_level_2(irq);

		if (dev == NULL) {
			LOG_ERR("no interrupt controller for irq %u", irq);
			return;
		}

		irq_enable_next_level(dev, line);
	} else {
		LOG_ERR("unsupported IRQ %u level %u", irq, level);
	}
}

/**
 * @brief Disable an interrupt.
 *
 * First-level interrupts are disabled through the RISC-V MIE CSR.
 * Second-level interrupts are disabled through the associated
 * AXI Interrupt Controller instance.
 *
 * @param irq Interrupt number.
 */
void arch_irq_disable(unsigned int irq)
{
	unsigned int level = irq_get_level(irq);

	if (level == 1U) {
		/*
		 * CSR mie register is updated using the atomic CSRRC
		 * instruction (atomic read and clear bits in CSR register).
		 */
		(void)csr_read_clear(mie, BIT(irq));

	} else if (level == 2U) {
		const struct device *dev = z_get_sw_isr_device_from_irq(irq);
		unsigned int line = irq_from_level_2(irq);

		if (dev == NULL) {
			LOG_ERR("no interrupt controller for irq %u", irq);
			return;
		}

		irq_disable_next_level(dev, line);
	} else {
		LOG_ERR("unsupported IRQ %u level %u", irq, level);
	}
}

/**
 * @brief Query interrupt enable state.
 *
 * First-level interrupts are queried from the RISC-V MIE CSR.
 * Second-level interrupts are queried from the associated
 * AXI Interrupt Controller instance.
 *
 * @param irq Interrupt number.
 *
 * @retval 1 Interrupt is enabled.
 * @retval 0 Interrupt is disabled.
 */
int arch_irq_is_enabled(unsigned int irq)
{
	unsigned int level = irq_get_level(irq);

	if (level == 1U) {
		return (int)((csr_read(mie) & BIT(irq)) != 0U);
	}

	if (level == 2U) {
		const struct device *dev = z_get_sw_isr_device_from_irq(irq);
		unsigned int line = irq_from_level_2(irq);

		if (dev == NULL) {
			LOG_ERR("no interrupt controller for irq %u", irq);
			return 0;
		}

		return (int)irq_line_is_enabled_next_level(dev, line);
	}

	LOG_ERR("unsupported IRQ %u level %u", irq, level);

	return 0;
}
