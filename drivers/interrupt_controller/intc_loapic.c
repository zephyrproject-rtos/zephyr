/*
 * Copyright (c) 1984-2008, 2011-2015 Wind River Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * driver for x86 CPU local APIC (as an interrupt controller)
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/pm/device.h>
#include <zephyr/types.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/arch/x86/msr.h>

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/drivers/interrupt_controller/loapic.h> /* public API declarations */
#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/sysapic.h>
#include <zephyr/drivers/interrupt_controller/ioapic.h>

/* Local APIC Version Register Bits */

#define LOAPIC_VERSION_MASK 0x000000ff /* LO APIC Version mask */
#define LOAPIC_MAXLVT_MASK 0x00ff0000  /* LO APIC Max LVT mask */
#define LOAPIC_PENTIUM4 0x00000014     /* LO APIC in Pentium4 */
#define LOAPIC_LVT_PENTIUM4 5	  /* LO APIC LVT - Pentium4 */
#define LOAPIC_LVT_P6 4		       /* LO APIC LVT - P6 */
#define LOAPIC_LVT_P5 3		       /* LO APIC LVT - P5 */

/* Local APIC Vector Table Bits */

#define LOAPIC_VECTOR 0x000000ff /* vectorNo */
#define LOAPIC_MODE 0x00000700   /* delivery mode */
#define LOAPIC_FIXED 0x00000000  /* delivery mode: FIXED */
#define LOAPIC_SMI 0x00000200    /* delivery mode: SMI */
#define LOAPIC_NMI 0x00000400    /* delivery mode: NMI */
#define LOAPIC_EXT 0x00000700    /* delivery mode: ExtINT */
#define LOAPIC_IDLE 0x00000000   /* delivery status: Idle */
#define LOAPIC_PEND 0x00001000   /* delivery status: Pend */
#define LOAPIC_HIGH 0x00000000   /* polarity: High */
#define LOAPIC_LOW 0x00002000    /* polarity: Low */
#define LOAPIC_REMOTE 0x00004000 /* remote IRR */
#define LOAPIC_EDGE 0x00000000   /* trigger mode: Edge */
#define LOAPIC_LEVEL 0x00008000  /* trigger mode: Level */

/* Local APIC Spurious-Interrupt Register Bits */

#define LOAPIC_ENABLE 0x100	/* APIC Enabled */
#define LOAPIC_FOCUS_DISABLE 0x200 /* Focus Processor Checking */

#if CONFIG_LOAPIC_SPURIOUS_VECTOR_ID == -1
#define LOAPIC_SPURIOUS_VECTOR_ID (CONFIG_IDT_NUM_VECTORS - 1)
#else
#define LOAPIC_SPURIOUS_VECTOR_ID CONFIG_LOAPIC_SPURIOUS_VECTOR_ID
#endif

#define LOPIC_SSPND_BITS_PER_IRQ  1  /* Just the one for enable disable*/
#define LOPIC_SUSPEND_BITS_REQD (ROUND_UP((LOAPIC_IRQ_COUNT * LOPIC_SSPND_BITS_PER_IRQ), 32))
#ifdef CONFIG_PM_DEVICE
#include <zephyr/pm/device.h>
__pinned_bss
uint32_t loapic_suspend_buf[LOPIC_SUSPEND_BITS_REQD / 32] = {0};
#endif

#ifdef DEVICE_MMIO_IS_IN_RAM
__pinned_bss
mm_reg_t z_loapic_regs;
#endif

__pinned_func
void send_eoi(void)
{
	x86_write_xapic(LOAPIC_EOI, 0);
}

/**
 * @brief Enable and initialize the local APIC.
 *
 * Called from early assembly layer (e.g., crt0.S).
 */
__pinned_func
void z_loapic_enable(unsigned char cpu_number)
{
	int32_t loApicMaxLvt; /* local APIC Max LVT */

#ifdef DEVICE_MMIO_IS_IN_RAM
	device_map(&z_loapic_regs, CONFIG_LOAPIC_BASE_ADDRESS, 0x1000,
		   K_MEM_CACHE_NONE);
#endif /* DEVICE_MMIO_IS_IN_RAM */
#ifndef CONFIG_X2APIC
	/*
	 * in xAPIC and flat model, bits 24-31 in LDR (Logical APIC ID) are
	 * bitmap of target logical APIC ID and it supports maximum 8 local
	 * APICs.
	 *
	 * The logical APIC ID could be arbitrarily selected by system software
	 * and is different from local APIC ID in local APIC ID register.
	 *
	 * We choose 0 for BSP, and the index to x86_cpuboot[] for secondary
	 * CPUs.
	 *
	 * in X2APIC, LDR is read-only.
	 */
	x86_write_xapic(LOAPIC_LDR, 1 << (cpu_number + 24));
#endif

	/*
	 * enable the local APIC. note that we use xAPIC mode here, since
	 * x2APIC access is not enabled until the next step (if at all).
	 */

	x86_write_xapic(LOAPIC_SVR,
			x86_read_xapic(LOAPIC_SVR) | LOAPIC_ENABLE);

#ifdef CONFIG_X2APIC
	/*
	 * turn on x2APIC mode. we trust the config option, so
	 * we don't check CPUID to see if x2APIC is supported.
	 */

	uint64_t msr = z_x86_msr_read(X86_APIC_BASE_MSR);
	msr |= X86_APIC_BASE_MSR_X2APIC;
	z_x86_msr_write(X86_APIC_BASE_MSR, msr);
#endif

	loApicMaxLvt = (x86_read_loapic(LOAPIC_VER) & LOAPIC_MAXLVT_MASK) >> 16;

	/* reset the DFR, TPR, TIMER_CONFIG, and TIMER_ICR */

#ifndef CONFIG_X2APIC
	/* Flat model */
	x86_write_loapic(LOAPIC_DFR, 0xffffffff);  /* no DFR in x2APIC mode */
#endif

	x86_write_loapic(LOAPIC_TPR, 0x0);
	x86_write_loapic(LOAPIC_TIMER_CONFIG, 0x0);
	x86_write_loapic(LOAPIC_TIMER_ICR, 0x0);

	/* program Local Vector Table for the Virtual Wire Mode */

	/* set LINT0: extInt, high-polarity, edge-trigger, not-masked */

	x86_write_loapic(LOAPIC_LINT0, (x86_read_loapic(LOAPIC_LINT0) &
		~(LOAPIC_MODE | LOAPIC_LOW |
		  LOAPIC_LEVEL | LOAPIC_LVT_MASKED)) |
		(LOAPIC_EXT | LOAPIC_HIGH | LOAPIC_EDGE));

	/* set LINT1: NMI, high-polarity, edge-trigger, not-masked */

	x86_write_loapic(LOAPIC_LINT1, (x86_read_loapic(LOAPIC_LINT1) &
		~(LOAPIC_MODE | LOAPIC_LOW |
		  LOAPIC_LEVEL | LOAPIC_LVT_MASKED)) |
		(LOAPIC_NMI | LOAPIC_HIGH | LOAPIC_EDGE));

	/* lock the Local APIC interrupts */

	x86_write_loapic(LOAPIC_TIMER, LOAPIC_LVT_MASKED);
	x86_write_loapic(LOAPIC_ERROR, LOAPIC_LVT_MASKED);

	if (loApicMaxLvt >= LOAPIC_LVT_P6) {
		x86_write_loapic(LOAPIC_PMC, LOAPIC_LVT_MASKED);
	}

	if (loApicMaxLvt >= LOAPIC_LVT_PENTIUM4) {
		x86_write_loapic(LOAPIC_THERMAL, LOAPIC_LVT_MASKED);
	}

#if CONFIG_LOAPIC_SPURIOUS_VECTOR
	x86_write_loapic(LOAPIC_SVR, (x86_read_loapic(LOAPIC_SVR) & 0xFFFFFF00) |
		     (LOAPIC_SPURIOUS_VECTOR_ID & 0xFF));
#endif

	/* discard a pending interrupt if any */
	x86_write_loapic(LOAPIC_EOI, 0);
}

/**
 * @brief Dummy initialization function.
 *
 * The local APIC is initialized via z_loapic_enable() long before the
 * kernel runs through its device initializations, so this is unneeded.
 */
__boot_func
static int loapic_init(const struct device *unused)
{
	ARG_UNUSED(unused);
	return 0;
}


__pinned_func
uint32_t z_loapic_irq_base(void)
{
	return z_ioapic_num_rtes();
}

/**
 * @brief Set the vector field in the specified RTE
 *
 * This associates an IRQ with the desired vector in the IDT.
 */
__boot_func
void z_loapic_int_vec_set(unsigned int irq, /* IRQ number of the interrupt */
				  unsigned int vector /* vector to copy into the LVT */
				  )
{
	unsigned int oldLevel;   /* previous interrupt lock level */

	/*
	 * The following mappings are used:
	 *
	 *   IRQ0 -> LOAPIC_TIMER
	 *   IRQ1 -> LOAPIC_THERMAL
	 *   IRQ2 -> LOAPIC_PMC
	 *   IRQ3 -> LOAPIC_LINT0
	 *   IRQ4 -> LOAPIC_LINT1
	 *   IRQ5 -> LOAPIC_ERROR
	 *
	 * It's assumed that LVTs are spaced by 0x10 bytes
	 */

	/* update the 'vector' bits in the LVT */

	oldLevel = irq_lock();
	x86_write_loapic(LOAPIC_TIMER + (irq * 0x10),
		     (x86_read_loapic(LOAPIC_TIMER + (irq * 0x10)) &
		      ~LOAPIC_VECTOR) | vector);
	irq_unlock(oldLevel);
}

/**
 * @brief Enable an individual LOAPIC interrupt (IRQ)
 *
 * @param irq the IRQ number of the interrupt
 *
 * This routine clears the interrupt mask bit in the LVT for the specified IRQ
 */
__pinned_func
void z_loapic_irq_enable(unsigned int irq)
{
	unsigned int oldLevel;   /* previous interrupt lock level */

	/*
	 * See the comments in _LoApicLvtVecSet() regarding IRQ to LVT mappings
	 * and ths assumption concerning LVT spacing.
	 */

	/* clear the mask bit in the LVT */

	oldLevel = irq_lock();
	x86_write_loapic(LOAPIC_TIMER + (irq * 0x10),
		     x86_read_loapic(LOAPIC_TIMER + (irq * 0x10)) &
		     ~LOAPIC_LVT_MASKED);
	irq_unlock(oldLevel);
}

/**
 * @brief Disable an individual LOAPIC interrupt (IRQ)
 *
 * @param irq the IRQ number of the interrupt
 *
 * This routine clears the interrupt mask bit in the LVT for the specified IRQ
 */
__pinned_func
void z_loapic_irq_disable(unsigned int irq)
{
	unsigned int oldLevel;   /* previous interrupt lock level */

	/*
	 * See the comments in _LoApicLvtVecSet() regarding IRQ to LVT mappings
	 * and ths assumption concerning LVT spacing.
	 */

	/* set the mask bit in the LVT */

	oldLevel = irq_lock();
	x86_write_loapic(LOAPIC_TIMER + (irq * 0x10),
		     x86_read_loapic(LOAPIC_TIMER + (irq * 0x10)) |
		     LOAPIC_LVT_MASKED);
	irq_unlock(oldLevel);
}


/**
 * @brief Find the currently executing interrupt vector, if any
 *
 * This routine finds the vector of the interrupt that is being processed.
 * The ISR (In-Service Register) register contain the vectors of the interrupts
 * in service. And the higher vector is the identification of the interrupt
 * being currently processed.
 *
 * This function must be called with interrupts locked in interrupt context.
 *
 * ISR registers' offsets:
 * --------------------
 * | Offset | bits    |
 * --------------------
 * | 0100H  |   0:31  |
 * | 0110H  |  32:63  |
 * | 0120H  |  64:95  |
 * | 0130H  |  96:127 |
 * | 0140H  | 128:159 |
 * | 0150H  | 160:191 |
 * | 0160H  | 192:223 |
 * | 0170H  | 224:255 |
 * --------------------
 *
 * @return The vector of the interrupt that is currently being processed, or -1
 * if no IRQ is being serviced.
 */
__pinned_func
int z_irq_controller_isr_vector_get(void)
{
	int pReg, block;

	/* Block 0 bits never lit up as these are all exception or reserved
	 * vectors
	 */
	for (block = 7; likely(block > 0); block--) {
		pReg = x86_read_loapic(LOAPIC_ISR + (block * 0x10));
		if (pReg) {
			return (block * 32) + (find_msb_set(pReg) - 1);
		}

	}
	return -1;
}

#ifdef CONFIG_PM_DEVICE
__pinned_func
static int loapic_suspend(const struct device *port)
{
	volatile uint32_t lvt; /* local vector table entry value */
	int loapic_irq;

	ARG_UNUSED(port);

	(void)memset(loapic_suspend_buf, 0, (LOPIC_SUSPEND_BITS_REQD >> 3));

	for (loapic_irq = 0; loapic_irq < LOAPIC_IRQ_COUNT; loapic_irq++) {

		if (_irq_to_interrupt_vector[z_loapic_irq_base() + loapic_irq]) {

			/* Since vector numbers are already present in RAM/ROM,
			 * We save only the mask bits here.
			 */
			lvt = x86_read_loapic(LOAPIC_TIMER + (loapic_irq * 0x10));

			if ((lvt & LOAPIC_LVT_MASKED) == 0U) {
				sys_bitfield_set_bit((mem_addr_t)loapic_suspend_buf,
					loapic_irq);
			}
		}
	}

	return 0;
}

__pinned_func
int loapic_resume(const struct device *port)
{
	int loapic_irq;

	ARG_UNUSED(port);

	/* Assuming all loapic device registers lose their state, the call to
	 * z_loapic_init(), should bring all the registers to a sane state.
	 */
	loapic_init(NULL);

	for (loapic_irq = 0; loapic_irq < LOAPIC_IRQ_COUNT; loapic_irq++) {

		if (_irq_to_interrupt_vector[z_loapic_irq_base() + loapic_irq]) {
			/* Configure vector and enable the required ones*/
			z_loapic_int_vec_set(loapic_irq,
				_irq_to_interrupt_vector[z_loapic_irq_base() +
							 loapic_irq]);

			if (sys_bitfield_test_bit((mem_addr_t) loapic_suspend_buf,
							loapic_irq)) {
				z_loapic_irq_enable(loapic_irq);
			}
		}
	}

	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
__pinned_func
static int loapic_pm_action(const struct device *dev,
			    enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = loapic_suspend(dev);
		break;
	case PM_DEVICE_ACTION_RESUME:
		ret = loapic_resume(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

PM_DEVICE_DEFINE(loapic, loapic_pm_action);

DEVICE_DEFINE(loapic, "loapic", loapic_init, PM_DEVICE_GET(loapic), NULL, NULL,
	      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);

#if CONFIG_LOAPIC_SPURIOUS_VECTOR
extern void z_loapic_spurious_handler(void);

NANO_CPU_INT_REGISTER(z_loapic_spurious_handler, NANO_SOFT_IRQ,
		      LOAPIC_SPURIOUS_VECTOR_ID >> 4,
		      LOAPIC_SPURIOUS_VECTOR_ID, 0);
#endif
