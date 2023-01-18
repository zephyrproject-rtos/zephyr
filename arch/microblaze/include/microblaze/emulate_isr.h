/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_ARCH_MICROBLAZE_INCLUDE_MICROBLAZE_EMULATE_ISR_H_
#define ZEPHYR_ARCH_MICROBLAZE_INCLUDE_MICROBLAZE_EMULATE_ISR_H_

#include <zephyr/arch/microblaze/arch.h>

#include <microblaze/mb_interface.h>

extern void microblaze_emulate_isr(void);

#define EMULATE_ISR()	__asm__ volatile("\tbralid r14, microblaze_emulate_isr\n\tnop\n")

#define EMULATE_IRQ(irq)                                                                           \
	do {                                                                                       \
		microblaze_disable_interrupts();                                                   \
		arch_irq_set_emulated_pending(irq);                                                \
		__asm__ volatile("\tbralid r14, microblaze_emulate_isr\n\tnop\n");                 \
	} while (0)

#endif /* ZEPHYR_ARCH_MICROBLAZE_INCLUDE_MICROBLAZE_EMULATE_ISR_H_ */
