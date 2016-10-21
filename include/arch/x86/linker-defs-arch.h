/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Intel commonly used macro and defines for linker script
 *
 * Commonly used macros and defines for linker script.
 */
#ifndef _LINKERDEFSARCH_H
#define _LINKERDEFSARCH_H

#include <toolchain.h>

#define INT_STUB_NOINIT KEEP(*(.intStubSect))

#define STATIC_IDT KEEP(*(staticIdt))

#define INTERRUPT_VECTORS_ALLOCATED KEEP(*(int_vector_alloc))

#define IRQ_TO_INTERRUPT_VECTOR KEEP(*(irq_int_vector_map))

/*
 * The x86 manual recommends aligning the IDT on 8 byte boundary. This also
 * ensures that individual entries do not span a page boundary which the
 * interrupt management code relies on.
 */
#define IDT_MEMORY \
	. = ALIGN(8);\
	_idt_base_address = .;\
	STATIC_IDT

#define INTERRUPT_VECTORS_ALLOCATED_MEMORY \
	. = ALIGN(4); \
	_interrupt_vectors_allocated = .; \
	INTERRUPT_VECTORS_ALLOCATED

#define IRQ_TO_INTERRUPT_VECTOR_MEMORY \
	. = ALIGN(4); \
	_irq_to_interrupt_vector = .; \
	IRQ_TO_INTERRUPT_VECTOR

#endif   /* _LINKERDEFSARCH_H */
