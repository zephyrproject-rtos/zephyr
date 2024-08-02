/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONFIG_DYNAMIC_INTERRUPTS
	. = ALIGN(8);
	_idt_base_address = .;
#ifdef LINKER_ZEPHYR_FINAL
	KEEP(*(staticIdt))
#else
	. += CONFIG_IDT_NUM_VECTORS * 8;
#endif /* LINKER_ZEPHYR_FINAL */

	. = ALIGN(4);
	_irq_to_interrupt_vector = .;
#ifdef LINKER_ZEPHYR_FINAL
	KEEP(*(irq_int_vector_map))
#else
	. += CONFIG_MAX_IRQ_LINES;
#endif
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
