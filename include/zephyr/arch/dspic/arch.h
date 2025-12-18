/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief dsPIC33 specific kernel interface header
 *
 * This header contains the dsPIC33 specific kernel interface. It is
 * included by the kernel interface architecture-abstraction header
 * (include/zephyr/arch/cpu.h).
 */

#ifndef ZEPHYR_INCLUDE_ARCH_DSPIC_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_DSPIC_ARCH_H_

#include <zephyr/arch/dspic/thread.h>
#include <zephyr/arch/dspic/exception.h>
#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/arch/common/ffs.h>

#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/devicetree.h>
#include <stdbool.h>

/** @brief Required stack pointer alignment in bytes */
#define ARCH_STACK_PTR_ALIGN 4

/** @brief Mask for CPU context level bits in the STATUS register */
#define STATUS_CTX_MASK      0x00070000u

/** @brief Mask for the interrupt level bits in an IRQ key */
#define IRQ_KEY_ILR_IRQ_MASK 0x7u

/** @brief Number of priority bits per interrupt in the IPC registers */
#define DSPIC_PRIORITY_BITS  3u

/** @brief Total bit width of one interrupt priority field (priority + enable) */
#define DSPIC_PRIORITY_WIDTH DSPIC_PRIORITY_BITS + 1u

/** @brief Number of interrupt priority fields per IPC register */
#define DSPIC_IRQ_PER_REG    8u

/** @brief Bitmask covering all priority bits of one interrupt field */
#define DSPIC_PRIORITY_MASK  ((1u << DSPIC_PRIORITY_BITS) - 1u)

/**
 * @brief Generate the jump instruction for an IRQ vector table entry.
 *
 * @param v ISR function name (without leading underscore)
 */
#define ARCH_IRQ_VECTOR_JUMP_CODE(v) "call _" STRINGIFY(v)

#ifndef _ASMLANGUAGE
#include <xc.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enable an interrupt line.
 *
 * @param irq IRQ line number to enable
 */
void arch_irq_enable(unsigned int irq);

/**
 * @brief Disable an interrupt line.
 *
 * @param irq IRQ line number to disable
 */
void arch_irq_disable(unsigned int irq);

/**
 * @brief Check whether an interrupt line is enabled.
 *
 * @param irq IRQ line number to check
 * @return Non-zero if the IRQ is enabled, zero otherwise
 */
int arch_irq_is_enabled(unsigned int irq);

/**
 * @brief Spurious interrupt handler.
 *
 * @param unused Unused parameter
 */
void z_irq_spurious(const void *unused);

/**
 * @brief Check whether an IRQ is currently set (pending or active).
 *
 * @param irq IRQ line number to check
 * @return true if the IRQ flag is set, false otherwise
 */
bool arch_dspic_irq_isset(unsigned int irq);

/**
 * @brief Software-trigger an IRQ for testing purposes.
 *
 * @param irq IRQ line number to trigger
 */
void z_dspic_enter_irq(int irq);

/**
 * @brief Map a physical device address to a virtual address.
 *
 * dsPIC33 has no MMU; the virtual address is set equal to the physical address.
 *
 * @param virt Pointer to the variable that receives the mapped address
 * @param phys Physical base address of the device
 * @param size Size of the mapping in bytes (unused)
 * @param flags Mapping flags (unused)
 */
#define device_map(virt, phys, size, flags) *(virt) = (phys)

/**
 * Configure a static interrupt.
 *
 * All arguments must be computable by the compiler at build time.
 *
 * @param irq_p IRQ line number
 * @param priority_p Interrupt priority
 * @param isr_p Interrupt service routine
 * @param isr_param_p ISR parameter
 * @param flags_p IRQ options
 *
 * @return The vector assigned to this interrupt
 */
#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)                           \
	{                                                                                          \
		Z_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p);                                       \
		z_dspic_irq_priority_set(irq_p, priority_p, flags_p);                              \
	}

/**
 * @brief Configure a direct static interrupt.
 *
 * @param irq_p IRQ line number
 * @param priority_p Interrupt priority
 * @param isr_p Interrupt service routine
 * @param flags_p IRQ options
 */
#define ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p)                                 \
	{                                                                                          \
		Z_ISR_DECLARE_DIRECT(irq_p, ISR_FLAG_DIRECT, isr_p);                               \
		z_dspic_irq_priority_set(irq_p, priority_p, flags_p);                              \
	}

/**
 * @brief Set the priority of an interrupt in the IPC registers.
 *
 * @param irq  IRQ line number
 * @param prio Interrupt priority level
 * @param flags Interrupt flags (unused)
 */
static ALWAYS_INLINE void z_dspic_irq_priority_set(unsigned int irq, unsigned int prio,
						   uint32_t flags)
{
	ARG_UNUSED(flags);
	volatile unsigned int reg_index = irq / DSPIC_IRQ_PER_REG;
	volatile unsigned int bit_pos = (irq % DSPIC_IRQ_PER_REG) * (DSPIC_PRIORITY_WIDTH);
	volatile uint32_t *prior_reg = (volatile uint32_t *)(&IPC0 + reg_index);
	*prior_reg &= ~(DSPIC_PRIORITY_MASK << bit_pos);
	*prior_reg |= (prio & DSPIC_PRIORITY_MASK) << bit_pos;
}

/**
 * @brief Unlock interrupts using the provided key.
 *
 * @param key Key returned by arch_irq_lock()
 */
static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	if (key == 0U) {
		/* Unmask interrupts (clear DISI) */
		__asm__ volatile("DISICTL %0\n\t" : : "r"(key) :);
	}
}

/**
 * @brief Check whether a key represents an unlocked interrupt state.
 *
 * @param key Key returned by arch_irq_lock()
 * @return true if interrupts were unlocked when the key was acquired
 */
static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	return (key == 0);
}

/**
 * @brief Lock interrupts and return a key representing the prior state.
 *
 * @return Key to be passed to arch_irq_unlock()
 */
static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	volatile unsigned int key;

	/* Mask all interrupts using DISI */
	__asm__ volatile("DISICTL #7, %0\n\t" : "=r"(key) : :);
	return key;
}

/**
 * @brief Check whether the CPU is currently executing in ISR context.
 *
 * @return true if executing in interrupt context, false otherwise
 */
static ALWAYS_INLINE bool arch_is_in_isr(void)
{
	uint32_t status_reg;

	__asm__ volatile("mov.l sr, %0" : "=r"(status_reg)::);
	return ((status_reg & STATUS_CTX_MASK) ? (true) : (false));
}

/**
 * @brief Execute a no-operation instruction.
 */
static ALWAYS_INLINE void arch_nop(void)
{
	__asm__ volatile("nop");
}

/**
 * @brief Get the current 32-bit system clock cycle count.
 *
 * @return Current cycle count as a 32-bit value
 */
extern uint32_t sys_clock_cycle_get_32(void);

/**
 * @brief Read the 32-bit hardware cycle counter.
 *
 * @return Current cycle count as a 32-bit value
 */
static inline uint32_t arch_k_cycle_get_32(void)
{
	return sys_clock_cycle_get_32();
}

/**
 * @brief Get the current 64-bit system clock cycle count.
 *
 * @return Current cycle count as a 64-bit value
 */
extern uint64_t sys_clock_cycle_get_64(void);

/**
 * @brief Read the 64-bit hardware cycle counter.
 *
 * @return Current cycle count as a 64-bit value
 */
static inline uint64_t arch_k_cycle_get_64(void)
{
	return sys_clock_cycle_get_64();
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_DSPIC_ARCH_H_ */
