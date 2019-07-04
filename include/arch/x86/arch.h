/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_X86_ARCH_H_

#include <generated_dts_board.h>
#include <stdbool.h>
#include <irq.h>

#if !defined(_ASMLANGUAGE)

#include <sys/sys_io.h>
#include <zephyr/types.h>
#include <stddef.h>

static ALWAYS_INLINE void sys_out8(u8_t data, io_port_t port)
{
	__asm__ volatile("outb	%b0, %w1;\n\t"
			 :
			 : "a"(data), "Nd"(port));
}

static ALWAYS_INLINE u8_t sys_in8(io_port_t port)
{
	u8_t ret;

	__asm__ volatile("inb	%w1, %b0;\n\t"
			 : "=a"(ret)
			 : "Nd"(port));
	return ret;
}

static ALWAYS_INLINE void sys_out16(u16_t data, io_port_t port)
{
	__asm__ volatile("outw	%w0, %w1;\n\t"
			 :
			 : "a"(data), "Nd"(port));
}

static ALWAYS_INLINE u16_t sys_in16(io_port_t port)
{
	u16_t ret;

	__asm__ volatile("inw	%w1, %w0;\n\t"
			 : "=a"(ret)
			 : "Nd"(port));
	return ret;
}

static ALWAYS_INLINE void sys_out32(u32_t data, io_port_t port)
{
	__asm__ volatile("outl	%0, %w1;\n\t"
			 :
			 : "a"(data), "Nd"(port));
}

static ALWAYS_INLINE u32_t sys_in32(io_port_t port)
{
	u32_t ret;

	__asm__ volatile("inl	%w1, %0;\n\t"
			 : "=a"(ret)
			 : "Nd"(port));
	return ret;
}

static ALWAYS_INLINE void sys_write8(u8_t data, mm_reg_t addr)
{
	__asm__ volatile("movb	%0, %1;\n\t"
			 :
			 : "q"(data), "m" (*(volatile u8_t *)(uintptr_t) addr)
			 : "memory");
}

static ALWAYS_INLINE u8_t sys_read8(mm_reg_t addr)
{
	u8_t ret;

	__asm__ volatile("movb	%1, %0;\n\t"
			 : "=q"(ret)
			 : "m" (*(volatile u8_t *)(uintptr_t) addr)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE void sys_write16(u16_t data, mm_reg_t addr)
{
	__asm__ volatile("movw	%0, %1;\n\t"
			 :
			 : "r"(data), "m" (*(volatile u16_t *)(uintptr_t) addr)
			 : "memory");
}

static ALWAYS_INLINE u16_t sys_read16(mm_reg_t addr)
{
	u16_t ret;

	__asm__ volatile("movw	%1, %0;\n\t"
			 : "=r"(ret)
			 : "m" (*(volatile u16_t *)(uintptr_t) addr)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE void sys_write32(u32_t data, mm_reg_t addr)
{
	__asm__ volatile("movl	%0, %1;\n\t"
			 :
			 : "r"(data), "m" (*(volatile u32_t *)(uintptr_t) addr)
			 : "memory");
}

static ALWAYS_INLINE u32_t sys_read32(mm_reg_t addr)
{
	u32_t ret;

	__asm__ volatile("movl	%1, %0;\n\t"
			 : "=r"(ret)
			 : "m" (*(volatile u32_t *)(uintptr_t) addr)
			 : "memory");

	return ret;
}

#endif /* _ASMLANGUAGE */

#ifdef CONFIG_X86_LONGMODE
#include <arch/x86/intel64/arch.h>
#else
#include <arch/x86/ia32/arch.h>
#endif

#include <arch/common/ffs.h>

#ifndef _ASMLANGUAGE

extern void z_arch_irq_enable(unsigned int irq);
extern void z_arch_irq_disable(unsigned int irq);

extern u32_t z_timer_cycle_get_32(void);
#define z_arch_k_cycle_get_32() z_timer_cycle_get_32()

/**
 * Returns true if interrupts were unlocked prior to the
 * z_arch_irq_lock() call that produced the key argument.
 */
static ALWAYS_INLINE bool z_arch_irq_unlocked(unsigned int key)
{
	return (key & 0x200) != 0;
}

/**
 *  @brief read timestamp register ensuring serialization
 */

static inline u64_t z_tsc_read(void)
{
	union {
		struct  {
			u32_t lo;
			u32_t hi;
		};
		u64_t  value;
	}  rv;

	/* rdtsc & cpuid clobbers eax, ebx, ecx and edx registers */
	__asm__ volatile (/* serialize */
		"xorl %%eax,%%eax;\n\t"
		"cpuid;\n\t"
		:
		:
		: "%eax", "%ebx", "%ecx", "%edx"
		);
	/*
	 * We cannot use "=A", since this would use %rax on x86_64 and
	 * return only the lower 32bits of the TSC
	 */
	__asm__ volatile ("rdtsc" : "=a" (rv.lo), "=d" (rv.hi));

	return rv.value;
}

static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
}

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_ARCH_H_ */
