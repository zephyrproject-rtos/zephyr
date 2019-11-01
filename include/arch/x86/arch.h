/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_X86_ARCH_H_

#include <generated_dts_board.h>

#if !defined(_ASMLANGUAGE)

#include <sys/sys_io.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <irq.h>
#include <arch/x86/mmustructs.h>

static ALWAYS_INLINE void z_arch_irq_unlock(unsigned int key)
{
	if ((key & 0x00000200U) != 0U) { /* 'IF' bit */
		__asm__ volatile ("sti" ::: "memory");
	}
}

static ALWAYS_INLINE void sys_out8(u8_t data, io_port_t port)
{
	__asm__ volatile("outb %b0, %w1" :: "a"(data), "Nd"(port));
}

static ALWAYS_INLINE u8_t sys_in8(io_port_t port)
{
	u8_t ret;

	__asm__ volatile("inb %w1, %b0" : "=a"(ret) : "Nd"(port));

	return ret;
}

static ALWAYS_INLINE void sys_out16(u16_t data, io_port_t port)
{
	__asm__ volatile("outw %w0, %w1" :: "a"(data), "Nd"(port));
}

static ALWAYS_INLINE u16_t sys_in16(io_port_t port)
{
	u16_t ret;

	__asm__ volatile("inw %w1, %w0" : "=a"(ret) : "Nd"(port));

	return ret;
}

static ALWAYS_INLINE void sys_out32(u32_t data, io_port_t port)
{
	__asm__ volatile("outl %0, %w1" :: "a"(data), "Nd"(port));
}

static ALWAYS_INLINE u32_t sys_in32(io_port_t port)
{
	u32_t ret;

	__asm__ volatile("inl %w1, %0" : "=a"(ret) : "Nd"(port));

	return ret;
}

static ALWAYS_INLINE void sys_write8(u8_t data, mm_reg_t addr)
{
	__asm__ volatile("movb %0, %1"
			 :
			 : "q"(data), "m" (*(volatile u8_t *)(uintptr_t) addr)
			 : "memory");
}

static ALWAYS_INLINE u8_t sys_read8(mm_reg_t addr)
{
	u8_t ret;

	__asm__ volatile("movb %1, %0"
			 : "=q"(ret)
			 : "m" (*(volatile u8_t *)(uintptr_t) addr)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE void sys_write16(u16_t data, mm_reg_t addr)
{
	__asm__ volatile("movw %0, %1"
			 :
			 : "r"(data), "m" (*(volatile u16_t *)(uintptr_t) addr)
			 : "memory");
}

static ALWAYS_INLINE u16_t sys_read16(mm_reg_t addr)
{
	u16_t ret;

	__asm__ volatile("movw %1, %0"
			 : "=r"(ret)
			 : "m" (*(volatile u16_t *)(uintptr_t) addr)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE void sys_write32(u32_t data, mm_reg_t addr)
{
	__asm__ volatile("movl %0, %1"
			 :
			 : "r"(data), "m" (*(volatile u32_t *)(uintptr_t) addr)
			 : "memory");
}

static ALWAYS_INLINE u32_t sys_read32(mm_reg_t addr)
{
	u32_t ret;

	__asm__ volatile("movl %1, %0"
			 : "=r"(ret)
			 : "m" (*(volatile u32_t *)(uintptr_t) addr)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE void sys_set_bit(mem_addr_t addr, unsigned int bit)
{
	__asm__ volatile("btsl %1, %0"
			 : "+m" (*(volatile u32_t *) (addr))
			 : "Ir" (bit)
			 : "memory");
}

static ALWAYS_INLINE void sys_clear_bit(mem_addr_t addr, unsigned int bit)
{
	__asm__ volatile("btrl %1, %0"
			 : "+m" (*(volatile u32_t *) (addr))
			 : "Ir" (bit));
}

static ALWAYS_INLINE int sys_test_bit(mem_addr_t addr, unsigned int bit)
{
	int ret;

	__asm__ volatile("btl %2, %1;"
			 "sbb %0, %0"
			 : "=r" (ret), "+m" (*(volatile u32_t *) (addr))
			 : "Ir" (bit));

	return ret;
}

static ALWAYS_INLINE int sys_test_and_set_bit(mem_addr_t addr,
					      unsigned int bit)
{
	int ret;

	__asm__ volatile("btsl %2, %1;"
			 "sbb %0, %0"
			 : "=r" (ret), "+m" (*(volatile u32_t *) (addr))
			 : "Ir" (bit));

	return ret;
}

static ALWAYS_INLINE int sys_test_and_clear_bit(mem_addr_t addr,
						unsigned int bit)
{
	int ret;

	__asm__ volatile("btrl %2, %1;"
			 "sbb %0, %0"
			 : "=r" (ret), "+m" (*(volatile u32_t *) (addr))
			 : "Ir" (bit));

	return ret;
}

#define sys_bitfield_set_bit sys_set_bit
#define sys_bitfield_clear_bit sys_clear_bit
#define sys_bitfield_test_bit sys_test_bit
#define sys_bitfield_test_and_set_bit sys_test_and_set_bit
#define sys_bitfield_test_and_clear_bit sys_test_and_clear_bit

/*
 * Map of IRQ numbers to their assigned vectors. On IA32, this is generated
 * at build time and defined via the linker script. On Intel64, it's an array.
 */

extern unsigned char _irq_to_interrupt_vector[];

#define Z_IRQ_TO_INTERRUPT_VECTOR(irq) \
	((unsigned int) _irq_to_interrupt_vector[irq])


#endif /* _ASMLANGUAGE */

#ifdef CONFIG_X86_64
#include <arch/x86/intel64/arch.h>
#else
#include <arch/x86/ia32/arch.h>
#endif

#include <arch/common/ffs.h>

#ifndef _ASMLANGUAGE

extern void z_arch_irq_enable(unsigned int irq);
extern void z_arch_irq_disable(unsigned int irq);

extern u32_t z_timer_cycle_get_32(void);

static inline u32_t z_arch_k_cycle_get_32(void)
{
	return z_timer_cycle_get_32();
}

static ALWAYS_INLINE bool z_arch_irq_unlocked(unsigned int key)
{
	return (key & 0x200) != 0;
}

/**
 * @brief read timestamp register, 32-bits only, unserialized
 */

static ALWAYS_INLINE u32_t z_do_read_cpu_timestamp32(void)
{
	u32_t rv;

	__asm__ volatile("rdtsc" : "=a" (rv) : : "%edx");

	return rv;
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
		"xorl %%eax,%%eax;"
		"cpuid"
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

static ALWAYS_INLINE void z_arch_nop(void)
{
	__asm__ volatile("nop");
}

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_ARCH_H_ */
