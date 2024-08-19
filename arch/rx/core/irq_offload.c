/*
 * Copyright (c) 2021 KT-Elektronik, Klaucke und Partner GmbH
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Software interrupts utility code - Renesas rx architecture implementation.
 *
 * The code is using the first software interrupt (SWINT) of the RX processor
 * should this interrupt ever be used for something else, this has to be
 * changed - maybe to the second software interrupt (SWINT2).
 */

#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>
#include <zephyr/sys/util.h>

/* vector number of the first software interrupt (SWINT).
 * Alternative (SWINT2): 26
 */
#define IRQ_OFFLOAD_INTNUM  27
/* Interrupt priority for the offload interrupt (set high) */
#define IRQ_OFFLOAD_INTPRIO 14
/* request register for SWINT - the interrupt is triggered if this is set to 1
 * if the offload is ever switched to SWINT2, the SWINT2R register (0x872E1)
 * has to be used.
 */
#define SWINTR_SWINT        *(uint8_t *)(0x872E0)

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
	irq_disable(IRQ_OFFLOAD_INTNUM);
	irq_connect_dynamic(IRQ_OFFLOAD_INTNUM, IRQ_OFFLOAD_INTPRIO, routine, parameter, 0);
	irq_enable(IRQ_OFFLOAD_INTNUM);
	/* set the SWINTR.SWINT bit to trigger the interrupt */
	SWINTR_SWINT = 1;
}
