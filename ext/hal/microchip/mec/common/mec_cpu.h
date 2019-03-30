/**
 *
 * Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

/** @file cpu.h
 *MEC1701 CPU abstractions
 */
/** @defgroup cpu
 */


#ifndef _CPU_H
#define _CPU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __CC_ARM /* Keil ARM MDK */

#ifndef UINT8_C
#define UINT8_C(x) (unsigned char)(x)
#endif

#ifndef UINT16_C
#define UINT16_C(x) (unsigned int)(x)
#endif

#ifndef UINT32_C
#define UINT32_C(x) (unsigned long)(x)
#endif

#define USED __attribute__((used))
#define WEAK __attribute__((weak))
#define INLINE __inline
#define NORETURN __declspec(noreturn)
#define PACKED __packed
#define CPU_GET_INTERRUPT_STATE() __get_PRIMASK()
#define CPU_SET_INTERRUPT_STATE(x) __set_PRIMASK((x))

/*
 * Keil MDK intrisic __disable_irq() returns the value of MSR PRIMASK
 * before disabling interrupts.
*/
#define CPU_DISABLE_INTERRUPTS() __disable_irq()
#define CPU_GET_DISABLE_INTERRUPTS(x) {(x) = __disable_irq();}
#define CPU_RESTORE_INTERRUPTS(x) { if (!(x)) { __enable_irq(); } }
#define CPU_ENABLE_INTERRUPTS() __enable_irq()

#define CPU_NOP() __nop()
#define CPU_WAIT_FOR_INTR() __wfi()

#define CPU_REV(x) __rev(x)

#define CPU_CLZ(x) __clz(x)

/*
 * The microsecond delay register is implemented in a Normal Data Memory
 * Region. Normal regions have relaxed data ordering semantics. This can
 * cause issues because writes to this register can complete before a
 * previous write to Device or Strongly ordered memory. Please use the
 * inline code after this definition. It uses the Data Synchronization
 * Barrier instruction to insure all outstanding writes complete before
 * the instruction after DSB is executed.
 * #define MEC2016_DELAY_REG   *((volatile uint8_t*) MEC2016_DELAY_REG_BASE)
 */

static inline void MICROSEC_DELAY(unsigned char n)
{
	volatile unsigned long *pdly_reg;

	pdly_reg = (volatile unsigned long *)0x10000000ul;

	__asm volatile (
		"\tstrb  n, [pdly_reg]\n"
		"\tldrb  n, [pdly_reg]\n"
		"\tadd   n, #0\n"
		"\tdmb\n"
	);
}

#elif defined(__XC32_PART_SUPPORT_VERSION) /* Microchip XC32 compiler customized GCC */

#error "!!! FORCED BUILD ERROR: compiler.h XC32 support has not been implemented !!!"

#elif defined(__GNUC__) && defined(__ARM_EABI__)    /* GCC for ARM (arm-none-eabi-gcc) */

#include <stdint.h>
#include <stddef.h>

#ifndef __always_inline
#define __always_inline inline __attribute__((always_inline))
#endif

static __always_inline void __NOP_THUMB2(void)
{
	__asm volatile ("nop");
}

#define CPU_NOP() __NOP_THUMB2()

static __always_inline void __WFI_THUMB2(void)
{
	__asm volatile ("dsb");
	__asm volatile ("isb");
	__asm volatile ("wfi");
	__asm volatile ("nop");
	__asm volatile ("nop");
	__asm volatile ("nop");
}

#define CPU_WAIT_FOR_INTR() __WFI_THUMB2()

/* We require user have ARM CMSIS header files available and configured
 * core_cmFunc.h includes inlines for global interrupt control.
 * For some reason, CMSIS __disable_irq for GCC does not return current PRIMASK
 * value. */

static __always_inline uint32_t __get_disable_irq(void)
{
	uint32_t pri_mask;

	__asm volatile (
		"\tmrs %0, primask\n"
		"\tcpsid i\n"
		"\tdsb\n"
		"\tisb\n"
		: "=r" (pri_mask) :: "memory"
	);
	return pri_mask;
}

static __always_inline uint32_t __get_primask(void)
{
	uint32_t pri_mask;

	__asm volatile (
		"\tmrs %0, primask\n"
		"\tisb\n"
		: "=r" (pri_mask) :: "memory"
	);
	return pri_mask;
}

static __always_inline void __enable_irqs(void)
{
	__asm volatile ("cpsie i" : : : "memory");
}

static __always_inline void __disable_irqs(void)
{
    __asm volatile (
	"\tcpsid i\n"
	"\tdsb\n"
	"\tisb\n" : : : "memory");
}


#define CPU_GET_INTERRUPT_STATE() __get_primask()
#define CPU_GET_DISABLE_INTERRUPTS(x) {(x) = __get_disable_irq();}
#define CPU_RESTORE_INTERRUPTS(x) { if (!(x)) { __enable_irqs(); } }
#define CPU_ENABLE_INTERRUPTS() __enable_irqs()
#define CPU_DISABLE_INTERRUPTS() __disable_irqs()


static __always_inline uint32_t __REV_THUMB2(uint32_t u32)
{
	return __builtin_bswap32(u32);
}

#define CPU_REV(x) __REV_THUMB2(x)

/*
 * __builtin_clz() will not be available if user compiles with built-ins disabled flag.
 */
#define CPU_CLZ(x) __builtin_clz(x)

static inline __attribute__((always_inline, noreturn)) void CPU_JMP(uint32_t addr)
{
	addr |= (1ul << 0);

	__asm volatile (
		"\n\t"
		"\tBX  %0 \n"
		"\tNOP \n"
		:   /* no outputs */
		:"r"(addr)
		:);
	while(1);
}


/*
 * The microsecond delay register is implemented in a Normal Data Memory
 * Region. Normal regions have relaxed data ordering semantics. This can
 * cause issues because writes to this register can complete before a
 * previous write to Device or Strongly ordered memory. Please use the
 * inline code after this definition. It uses the Data Synchronization
 * Barrier instruction to insure all outstanding writes complete before
 * the instruction after DSB is executed.
 * #define MEC2016_DELAY_REG   *((volatile uint8_t*) MEC2016_DELAY_REG_BASE)
 */

static __always_inline void MICROSEC_DELAY(uint8_t n)
{
	uint32_t dly_reg_addr = 0x10000000ul;

	__asm volatile (
		"\tstrb  %0, [%1]\n"
		"\tldrb  %0, [%1]\n"
		"\tadd  %0, #0\n"
		"\tdmb\n"
		:
		: "r" (n), "r" (dly_reg_addr)
		: "memory"
	);
}

static __always_inline void write_read_back8(volatile uint8_t* addr, uint8_t val)
{
	__asm__ __volatile__ (
		"\n\t"
		"strb %1, [%0]      \n\t"
		"ldrb %1, [%0]      \n\t"
		: /* No outputs */
		: "r" (addr), "r" (val)
		: "memory"
	);
}

static __always_inline void write_read_back16(volatile uint16_t* addr, uint16_t val)
{
	__asm__ __volatile__ (
		"\n\t"
		"strh %1, [%0]      \n\t"
		"ldrh %1, [%0]      \n\t"
		: /* No outputs */
		: "r" (addr), "r" (val)
		: "memory"
	);
}

static __always_inline void write_read_back32(volatile uint32_t* addr, uint32_t val)
{
	__asm__ __volatile__ (
		"\n\t"
		"str %1, [%0]       \n\t"
		"ldr %1, [%0]       \n\t"
		: /* No outputs */
		: "r" (addr), "r" (val)
		: "memory"
	);
}

#else  /* Unknown compiler */

#error "!!! FORCED BUILD ERROR: cpu.h Unknown compiler !!!"

#endif

#endif /* #ifndef _CPU_H */
/**   @}
 */
