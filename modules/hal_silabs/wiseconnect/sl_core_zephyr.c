/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr-compatible implementation of Silabs sl_core ATOMIC/CRITICAL section
 * primitives. This replaces sl_core_cortexm.c (which requires em_device.h)
 * for NCP mode where the host MCU is not a Silabs SoC.
 *
 * The CORE_Enter/ExitAtomic and CORE_Enter/ExitCritical functions map to
 * Zephyr's irq_lock()/irq_unlock() which disables/restores interrupts.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <stdint.h>
#include <stdbool.h>

/* These match the sl_core.h typedefs and function signatures */
typedef uint32_t CORE_irqState_t;

CORE_irqState_t CORE_EnterAtomic(void)
{
	return irq_lock();
}

void CORE_ExitAtomic(CORE_irqState_t irqState)
{
	irq_unlock(irqState);
}

CORE_irqState_t CORE_EnterCritical(void)
{
	return irq_lock();
}

void CORE_ExitCritical(CORE_irqState_t irqState)
{
	irq_unlock(irqState);
}

bool CORE_IrqIsDisabled(void)
{
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) || defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	return (__get_PRIMASK() & 0x1) != 0;
#else
	return false;
#endif
}

void CORE_YieldAtomic(void)
{
	/* Brief re-enable of interrupts to let pending IRQs fire */
	unsigned int key = irq_lock();

	irq_unlock(key);
}

void CORE_YieldCritical(void)
{
	CORE_YieldAtomic();
}
