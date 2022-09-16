/* loapic.h - public LOAPIC APIs */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LOAPIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_LOAPIC_H_

#include <zephyr/arch/cpu.h>
#include <zephyr/arch/x86/msr.h>
#include <zephyr/sys/device_mmio.h>

/* Local APIC Register Offset */

#define LOAPIC_ID 0x020		  /* Local APIC ID Reg */
#define LOAPIC_VER 0x030	  /* Local APIC Version Reg */
#define LOAPIC_TPR 0x080	  /* Task Priority Reg */
#define LOAPIC_APR 0x090	  /* Arbitration Priority Reg */
#define LOAPIC_PPR 0x0a0	  /* Processor Priority Reg */
#define LOAPIC_EOI 0x0b0	  /* EOI Reg */
#define LOAPIC_LDR 0x0d0	  /* Logical Destination Reg */
#define LOAPIC_DFR 0x0e0	  /* Destination Format Reg */
#define LOAPIC_SVR 0x0f0	  /* Spurious Interrupt Reg */
#define LOAPIC_ISR 0x100	  /* In-service Reg */
#define LOAPIC_TMR 0x180	  /* Trigger Mode Reg */
#define LOAPIC_IRR 0x200	  /* Interrupt Request Reg */
#define LOAPIC_ESR 0x280	  /* Error Status Reg */
#define LOAPIC_ICRLO 0x300	/* Interrupt Command Reg */
#define LOAPIC_ICRHI 0x310	/* Interrupt Command Reg */
#define LOAPIC_TIMER 0x320	/* LVT (Timer) */
#define LOAPIC_THERMAL 0x330      /* LVT (Thermal) */
#define LOAPIC_PMC 0x340	  /* LVT (PMC) */
#define LOAPIC_LINT0 0x350	/* LVT (LINT0) */
#define LOAPIC_LINT1 0x360	/* LVT (LINT1) */
#define LOAPIC_ERROR 0x370	/* LVT (ERROR) */
#define LOAPIC_TIMER_ICR 0x380    /* Timer Initial Count Reg */
#define LOAPIC_TIMER_CCR 0x390    /* Timer Current Count Reg */
#define LOAPIC_TIMER_CONFIG 0x3e0 /* Timer Divide Config Reg */
#define LOAPIC_SELF_IPI 0x3f0	/* Self IPI Reg, only support in X2APIC mode */

#define LOAPIC_ICR_BUSY		0x00001000	/* delivery status: 1 = busy */

#define LOAPIC_ICR_IPI_OTHERS	0x000C4000U	/* normal IPI to other CPUs */
#define LOAPIC_ICR_IPI_INIT	0x00004500U
#define LOAPIC_ICR_IPI_STARTUP	0x00004600U

#define LOAPIC_LVT_MASKED 0x00010000   /* mask */

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t z_loapic_irq_base(void);
extern void z_loapic_enable(unsigned char cpu_number);
extern void z_loapic_int_vec_set(unsigned int irq, unsigned int vector);
extern void z_loapic_irq_enable(unsigned int irq);
extern void z_loapic_irq_disable(unsigned int irq);

/**
 * @brief Read 64-bit value from the local APIC in x2APIC mode.
 *
 * @param reg the LOAPIC register number to read (LOAPIC_*)
 */
static inline uint64_t x86_read_x2apic(unsigned int reg)
{
	reg >>= 4;
	return z_x86_msr_read(X86_X2APIC_BASE_MSR + reg);
}

/* Defined in intc_loapic.c */
#ifdef DEVICE_MMIO_IS_IN_RAM
extern mm_reg_t z_loapic_regs;
#endif

/**
 * @brief Read 32-bit value from the local APIC in xAPIC (MMIO) mode.
 *
 * @param reg the LOAPIC register number to read (LOAPIC_*)
 */
static inline uint32_t x86_read_xapic(unsigned int reg)
{
	mm_reg_t base;
#ifdef DEVICE_MMIO_IS_IN_RAM
	base = z_loapic_regs;
#else
	base = CONFIG_LOAPIC_BASE_ADDRESS;
#endif
	return sys_read32(base + reg);
}

/**
 * @brief Read value from the local APIC using the default mode.
 *
 * Returns a 32-bit value read from the local APIC, using the access
 * method determined by CONFIG_X2APIC (either xAPIC or x2APIC). Note
 * that 64-bit reads are only allowed in x2APIC mode and can only be
 * done by calling x86_read_x2apic() directly. (This is intentional.)
 *
 * @param reg the LOAPIC register number to read (LOAPIC_*)
 */
static inline uint32_t x86_read_loapic(unsigned int reg)
{
#ifdef CONFIG_X2APIC
	return x86_read_x2apic(reg);
#else
	return x86_read_xapic(reg);
#endif
}

/**
 * @brief Write 64-bit value to the local APIC in x2APIC mode.
 *
 * @param reg the LOAPIC register number to write (one of LOAPIC_*)
 * @param val 64-bit value to write
 */
static inline void x86_write_x2apic(unsigned int reg, uint64_t val)
{
	reg >>= 4;
	z_x86_msr_write(X86_X2APIC_BASE_MSR + reg, val);
}

/**
 * @brief Write 32-bit value to the local APIC in xAPIC (MMIO) mode.
 *
 * @param reg the LOAPIC register number to write (one of LOAPIC_*)
 * @param val 32-bit value to write
 */
static inline void x86_write_xapic(unsigned int reg, uint32_t val)
{
	mm_reg_t base;
#ifdef DEVICE_MMIO_IS_IN_RAM
	base = z_loapic_regs;
#else
	base = CONFIG_LOAPIC_BASE_ADDRESS;
#endif
	sys_write32(val, base + reg);
}

/**
 * @brief Write 32-bit value to the local APIC using the default mode.
 *
 * Write a 32-bit value to the local APIC, using the access method
 * determined by CONFIG_X2APIC (either xAPIC or x2APIC). Note that
 * 64-bit writes are only available in x2APIC mode and can only be
 * done by calling x86_write_x2apic() directly. (This is intentional.)
 *
 * @param reg the LOAPIC register number to write (one of LOAPIC_*)
 * @param val 32-bit value to write
 */
static inline void x86_write_loapic(unsigned int reg, uint32_t val)
{
#ifdef CONFIG_X2APIC
	x86_write_x2apic(reg, val);
#else
	x86_write_xapic(reg, val);
#endif
}

/**
 * @brief Send an IPI.
 *
 * @param apic_id If applicable, the target CPU APIC ID (0 otherwise).
 * @param ipi Type of IPI: one of the LOAPIC_ICR_IPI_* constants.
 * @param vector If applicable, the target vector (0 otherwise).
 */
static inline void z_loapic_ipi(uint8_t apic_id, uint32_t ipi, uint8_t vector)
{
	ipi |= vector;

#ifndef CONFIG_X2APIC
	/*
	 * Legacy xAPIC mode: first wait for any previous IPI to be delivered.
	 */

	while (x86_read_xapic(LOAPIC_ICRLO) & LOAPIC_ICR_BUSY) {
	}

	x86_write_xapic(LOAPIC_ICRHI, apic_id << 24);
	x86_write_xapic(LOAPIC_ICRLO, ipi);
#else
	/*
	 * x2APIC mode is greatly simplified: one write, no delivery status.
	 */

	x86_write_x2apic(LOAPIC_ICRLO, (((uint64_t) apic_id) << 32) | ipi);
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_DRIVERS_LOAPIC_H_ */
