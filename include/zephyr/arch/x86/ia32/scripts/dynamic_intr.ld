/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_DYNAMIC_INTERRUPTS
	. = ALIGN(4);
	_irq_to_interrupt_vector = .;
#ifdef LINKER_ZEPHYR_FINAL
	KEEP(*(irq_int_vector_map))
#else
	. += CONFIG_MAX_IRQ_LINES;
#endif /* LINKER_ZEPHYR_FINAL */

	z_interrupt_vectors_allocated = .;
#ifdef LINKER_ZEPHYR_FINAL
	KEEP(*(irq_vectors_alloc))
#else
	. += (CONFIG_IDT_NUM_VECTORS + 7) / 8;
#endif /* LINKER_ZEPHYR_FINAL */
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
