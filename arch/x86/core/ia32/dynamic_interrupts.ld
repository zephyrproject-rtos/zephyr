/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONFIG_DYNAMIC_INTERRUPTS
. = ALIGN(8);
_idt_base_address = .;
#ifdef LINKER_PASS2
KEEP(*(staticIdt))
#else
. += CONFIG_IDT_NUM_VECTORS * 8;
#endif /* LINKER_PASS2 */

. = ALIGN(4);
_irq_to_interrupt_vector = .;
#ifdef LINKER_PASS2
KEEP(*(irq_int_vector_map))
#else
. += CONFIG_MAX_IRQ_LINES;
#endif
#endif /* CONFIG_DYNAMIC_INTERRUPTS */
