/*
 * Copyright (c) 2023 - 2024 Advanced Micro Devices, Inc.
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/riscv/irq.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/intc_xlnx.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys_clock.h>
#include <zephyr/tracing/tracing.h>

/**
 * @file
 *
 * The file is wiring architecture irq handler with secondary interrupt
 * controller. AXI intc is used as secondary interrupt controller. Another
 * level is not supported now but can be added in future. Maximum amount
 * interrupts handled by AXI intc is 32.
 */

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

/**
 * @brief Override arch_irq_enable with a call to intc driver
 *
 * @param irq irq number to enable
 */
void arch_irq_enable(uint32_t irq)
{
	unsigned int level = irq_get_level(irq);

	if (level == 1) {
		uint32_t mie;
		/*
		 * CSR mie register is updated using atomic instruction csrrs
		 * (atomic read and set bits in CSR register)
		 */
		LOG_DBG("%s: RISCV irq: %d", __func__, irq);
		mie = csr_read_set(mie, 1 << irq);
	} else if (level == 2) {
		irq = irq_from_level_2(irq);
		LOG_DBG("%s: AXI INTC irq: %d", __func__, irq);
		xlnx_intc_irq_enable(irq);
	} else {
		/* This code shoudln't be taken because 3rd level is not enabled */
		LOG_ERR("%s: Unsupported IRQ %d level %d", __func__, irq, level);
	}
}

/**
 * @brief Override arch_irq_disable with a call to intc driver
 *
 * @param irq irq number to disable
 */
void arch_irq_disable(uint32_t irq)
{
	uint32_t mie;
	unsigned int level = irq_get_level(irq);

	if (level == 2) {
		irq = irq_from_level_2(irq);
		LOG_DBG("%s irq: %d, 1 means timer, 2 means uart %d", __func__, irq, level);
		xlnx_intc_irq_disable(irq);

	} else {
		/*
		 * CSR mie register is updated using atomic instruction csrrs
		 * (atomic read and set bits in CSR register)
		 */
		mie = csr_read_clear(mie, 1 << irq);
	}
}

/**
 * @brief Override arch_irq_is_enabled with a call to intc driver
 *
 * @param irq irq number to see if enabled
 */
int arch_irq_is_enabled(uint32_t irq)
{
	uint32_t mie;
	int32_t ret;
	unsigned int level = irq_get_level(irq);

	if (level == 2) {
		irq = irq_from_level_2(irq);
		ret = BIT(irq) & xlnx_intc_irq_get_enabled();
	} else {
		mie = csr_read(mie);

		ret = !!(mie & (1 << irq));
	}
	return ret;
}

/**
 * @brief Returns the currently pending interrupts.
 *
 * @return Pending IRQ bitmask. Pending IRQs will have their bitfield set to 1. 0 if no interrupt is
 * pending.
 */
uint32_t arch_irq_pending(void)
{
	return xlnx_intc_irq_pending();
};

/**
 * @brief Returns the vector for highest pending interrupt.
 *
 * @return Returns the vector for (i.e. index) for highest-prio/lowest-num pending interrupt to be
 * used in a jump table. This is used used for sw_isr_table.
 */
uint32_t arch_irq_pending_vector(uint32_t ipending)
{
	ARG_UNUSED(ipending);
	return xlnx_intc_irq_pending_vector();
}

#ifdef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
void arch_cpu_idle(void)
{
	sys_trace_idle();
	irq_unlock(MSTATUS_IEN);
}
#endif /* CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE */

#ifdef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
void arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	irq_unlock(key);
}
#endif /* CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE */
