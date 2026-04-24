/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"

extern void _irq_wrapper(void);
extern void _syscall_wrapper(void);
extern void z_tricore_fault(uint8_t trap_class, uint8_t tin);
extern void z_tricore_fault_fcu(void);

void __attribute((naked, section(".vectors.trap"))) __trap_vector_tc0_class0()
{
	__asm volatile(".p2align 5\n");
	__asm volatile("svlcx\n"
		       "mov %d4, 0\n"
		       "mov %d5, %d15\n"
		       "j z_tricore_fault\n");
}

void __attribute((naked, section(".vectors.trap"))) __trap_vector_tc0_class1()
{
	__asm volatile(".p2align 5\n");
	__asm volatile("svlcx\n"
		       "mov %d4, 1\n"
		       "mov %d5, %d15\n"
		       "j z_tricore_fault\n");
}

void __attribute((naked, section(".vectors.trap"))) __trap_vector_tc0_class2()
{
	__asm volatile(".p2align 5\n");
	__asm volatile("svlcx\n"
		       "mov %d4, 2\n"
		       "mov %d5, %d15\n"
		       "j z_tricore_fault\n");
}

void __attribute((naked, section(".vectors.trap"))) __trap_vector_tc0_class3()
{
	__asm volatile(".p2align 5\n");
	__asm volatile("svlcx\n"
		       "mov %d4, 3\n"
		       "mov %d5, %d15\n"
		       "jeq %d5, 4, z_tricore_fault_fcu\n"
		       "j z_tricore_fault\n");
}

void __attribute((naked, section(".vectors.trap"))) __trap_vector_tc0_class4()
{
	__asm volatile(".p2align 5\n");
	__asm volatile("svlcx\n"
		       "mov %d4, 4\n"
		       "mov %d5, %d15\n"
		       "j z_tricore_fault\n");
}

void __attribute((naked, section(".vectors.trap"))) __trap_vector_tc0_class5()
{
	__asm volatile(".p2align 5\n");
	__asm volatile("svlcx\n"
		       "mov %d4, 5\n"
		       "mov %d5, %d15\n"
		       "j z_tricore_fault\n");
}

void __attribute((naked, section(".vectors.trap"))) __trap_vector_tc0_class6()
{
	__asm volatile("   .p2align 5\n");
	__asm volatile("   svlcx\n");
	__asm volatile("   j _syscall_wrapper\n");
}

void __attribute((naked, section(".vectors.trap"))) __trap_vector_tc0_class7()
{
	__asm volatile(".p2align 5\n");
	__asm volatile("svlcx\n"
		       "mov %d4, 7\n"
		       "mov %d5, %d15\n"
		       "j z_tricore_fault\n");
}

#define VECTOR_FUNC(prio, _)                                                                       \
	void __attribute((naked, weak, section(".vectors.irq"))) __irq_vector_tc0_##prio()         \
	{                                                                                          \
		__asm volatile(".p2align 5\n"                                                      \
			       "svlcx\n"                                                           \
			       "j _isr_wrapper\n");                                                \
	}

LISTIFY(256, VECTOR_FUNC, ())

#pragma GCC diagnostic pop
