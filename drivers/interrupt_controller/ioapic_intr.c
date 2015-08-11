/* ioApicIntr.c - Intel IO APIC/xAPIC driver */

/*
 * Copyright (c) 1997-1998, 2000-2002, 2004, 2006-2008, 2011-2015 Wind River
 *Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module is a driver for the IO APIC/xAPIC (Advanced Programmable
Interrupt Controller) for P6 (PentiumPro, II, III) family processors
and P7 (Pentium4) family processors.  The IO APIC/xAPIC is included
in the Intel's system chip set, such as ICH2.  Software intervention
may be required to enable the IO APIC/xAPIC in some chip sets.
The 8259A interrupt controller is intended for use in a uni-processor
system, IO APIC can be used in either a uni-processor or multi-processor
system.  The IO APIC handles interrupts very differently than the 8259A.
Briefly, these differences are:
 - Method of Interrupt Transmission. The IO APIC transmits interrupts
   through a 3-wire bus and interrupts are handled without the need for
   the processor to run an interrupt acknowledge cycle.
 - Interrupt Priority. The priority of interrupts in the IO APIC is
   independent of the interrupt number.  For example, interrupt 10 can
   be given a higher priority than interrupt 3.
 - More Interrupts. The IO APIC supports a total of 24 interrupts.

The IO APIC unit consists of a set of interrupt input signals, a 24-entry
by 64-bit Interrupt Redirection Table, programmable registers, and a message
unit for sending and receiving APIC messages over the APIC bus or the
Front-Side (system) bus.  IO devices inject interrupts into the system by
asserting one of the interrupt lines to the IO APIC.  The IO APIC selects the
corresponding entry in the Redirection Table and uses the information in that
entry to format an interrupt request message.  Each entry in the Redirection
Table can be individually programmed to indicate edge/level sensitive interrupt
signals, the interrupt vector and priority, the destination processor, and how
the processor is selected (statically and dynamically).  The information in
the table is used to transmit a message to other APIC units (via the APIC bus
or the Front-Side (system) bus).  IO APIC is used in the Symmetric IO Mode.
The base address of IO APIC is determined in loapic_init() and stored in the
global variable ioApicBase and ioApicData.
The lower 32 bit value of the redirection table entries for IRQ 0
to 15 are edge triggered positive high, and for IRQ 16 to 23 are level
triggered positive low.

This implementation doesn't support multiple IO APICs.

INCLUDE FILES: ioapic.h loapic.h

SEE ALSO: loApicIntr.c
 */

#include <nanokernel.h>
#include <arch/cpu.h>

#include "board.h"

#include <toolchain.h>
#include <sections.h>

#include <drivers/ioapic.h> /* public API declarations */
#include <drivers/loapic.h> /* public API declarations and registers */

/* IO APIC direct register offsets */

#define IOAPIC_IND 0x00   /* Index Register */
#define IOAPIC_DATA 0x10  /* IO window (data) - pc.h */
#define IOAPIC_IRQPA 0x20 /* IRQ Pin Assertion Register */
#define IOAPIC_EOI 0x40   /* EOI Register */

#ifdef IOAPIC_MSI_REDIRECT

/* direct addressing of the RTEs; including the "configuration register" */

#define IOAPIC_RTE0_LOW 0x1000
#define IOAPIC_RTE0_HIGH 0x1004
#define IOAPIC_RTE0_CONFIG 0x1008
#define IOAPIC_RTE1_LOW 0x1010
#define IOAPIC_RTE1_HIGH 0x1014
#define IOAPIC_RTE1_CONFIG 0x1018
#define IOAPIC_RTE2_LOW 0x1020
#define IOAPIC_RTE2_HIGH 0x1024
#define IOAPIC_RTE2_CONFIG 0x1028

/*
 * etc., etc. until IOAPIC_RTE63_LOW/IOAPIC_RTE63_HIGH/IOAPIC_RTE63_CONFIG
 *
 *  rteLowOffset    = IOAPIC_RTE0_LOW    + (irq * 0x10)
 *  rteHighOffset   = IOAPIC_RTE0_HIGH   + (irq * 0x10)
 *  rteConfigOffset = IOAPIC_RTE0_CONFIG + (irq * 0x10)
 */

/*
 * An extention to the "standard" IOAPIC design supports a redirection
 * capability that allows each RTE to specify which of the 8 "redirection
 * registers" to use for determining the MSI address.
 */

#define IOAPIC_REDIR_ADDR0 0x2000 /* Dummy entry; reads return all 0's */
#define IOAPIC_REDIR_ADDR1 0x2004 /* MSI redirection selection reg 1 */
#define IOAPIC_REDIR_ADDR2 0x2008 /* MSI redirection selection reg 2 */
#define IOAPIC_REDIR_ADDR3 0x200c /* MSI redirection selection reg 3 */
#define IOAPIC_REDIR_ADDR4 0x2010 /* MSI redirection selection reg 4 */
#define IOAPIC_REDIR_ADDR5 0x2014 /* MSI redirection selection reg 5 */
#define IOAPIC_REDIR_ADDR6 0x2018 /* MSI redirection selection reg 6 */
#define IOAPIC_REDIR_ADDR7 0x201c /* MSI redirection selection reg 7 */

/* interrupt status for line interrupts generated via RTE0 through RTE31 */

#define IOAPIC_LINE_INT_STAT0 0x2040

/* interrupt status for line interrupts generated via RTE32 through RTE64 */

#define IOAPIC_LINE_INT_STAT1 0x2044

/* interrupt mask for line interrupts generated via RTE0 to RTE31 */

#define IOAPIC_LINE_INT_MASK0 0x2048

/* interrupt mask for line interrupts generated via RTE32 to RTE63 */

#define IOAPIC_LINE_INT_MASK1 0x204c

#endif /* IOAPIC_MSI_REDIRECT */

/* IO APIC indirect register offset */

#define IOAPIC_ID 0x00     /* IOAPIC ID */
#define IOAPIC_VERS 0x01   /* IOAPIC Version */
#define IOAPIC_ARB 0x02    /* IOAPIC Arbitration ID */
#define IOAPIC_BOOT 0x03   /* IOAPIC Boot Configuration */
#define IOAPIC_REDTBL 0x10 /* Redirection Table (24 * 64bit) */

/* Interrupt delivery type */

#define IOAPIC_DT_APIC 0x0 /* APIC serial bus */
#define IOAPIC_DT_FS 0x1   /* Front side bus message*/

/* Version register bits */

#define IOAPIC_MRE_MASK 0x00ff0000 /* Max Red. entry mask */
#define IOAPIC_PRQ 0x00008000      /* this has IRQ reg */
#define IOAPIC_VERSION 0x000000ff  /* version number */

/* Redirection table entry number */

#define MAX_REDTABLE_ENTRIES 24

/* Redirection table entry bits: upper 32 bit */

#define IOAPIC_DESTINATION 0xff000000

/* Redirection table entry bits: lower 32 bit */

#define IOAPIC_VEC_MASK 0x000000ff

#ifdef IOAPIC_MSI_REDIRECT

/* RTE configuration register bits */

#define IOAPIC_RTE_CONFIG_REDIR_SEL 0x7
#define IOAPIC_RTE_CONFIG_LI0EN 0x8
#define IOAPIC_RTE_CONFIG_LI1EN 0x10
#define IOAPIC_RTE_CONFIG_LI2EN 0x20
#define IOAPIC_RTE_CONFIG_BYPASS_MSI_DISABLE 0x40
#define IOAPIC_RTE_CONFIG_DISABLE_INT_EXT 0x80

#endif /* IOAPIC_MSI_REDIRECT */

#ifndef XIOAPIC_DIRECT_ADDRESSING
static uint32_t __IoApicGet(int32_t offset);
static void __IoApicSet(int32_t offset, uint32_t value);
#endif

static void ioApicRedSetHi(unsigned int irq, uint32_t upper32);
static void ioApicRedSetLo(unsigned int irq, uint32_t lower32);
static uint32_t ioApicRedGetLo(unsigned int irq);
static void _IoApicRedUpdateLo(unsigned int irq, uint32_t value,
					uint32_t mask);

/*
 * The functions irq_enable() and irq_disable() are implemented in the
 * interrupt controller driver due to the IRQ virtualization imposed by
 * the x86 architecture.
 */

/**
 *
 * @brief Initialize the IO APIC or xAPIC
 *
 * This routine initializes the IO APIC or xAPIC.
 *
 * @return N/A
 */

void _ioapic_init(void)
{
	int32_t ix;	/* redirection table index */
	uint32_t rteValue; /* value to copy into redirection table entry */

#ifdef IOAPIC_MSI_REDIRECT
	_IoApicRedirRegSet(MSI_REDIRECT_SELECT_ID, MSI_REDIRECT_TARGET_ADDR);
#endif

	/*
	 * The platform must set the Kconfig option IOAPIC_NUM_RTES to indicate
	 * the number of redirection table entries supported by the IOAPIC.
	 *
	 * Note: The number of actual IRQs supported by the IOAPIC can be
	 * determined at runtime by computing:
	 *
	 * ((__IoApicGet(IOAPIC_VERS) & IOAPIC_MRE_MASK) >> 16) + 1
	 */

	/*
	 * Initialize the redirection table entries with default settings;
	 * actual interrupt vectors are specified during irq_connect().
	 *
	 * A future enhancement should make this initialization "table driven":
	 * use data provided by the platform to specify the initial state
	 */

	rteValue = IOAPIC_EDGE | IOAPIC_HIGH | IOAPIC_FIXED | IOAPIC_INT_MASK |
		   IOAPIC_PHYSICAL | 0 /* dummy vector */;

	for (ix = 0; ix < CONFIG_IOAPIC_NUM_RTES; ix++) {
		ioApicRedSetHi(ix, 0);
		ioApicRedSetLo(ix, rteValue);
	}
}

/**
 *
 * @brief Send EOI (End Of Interrupt) signal to IO APIC
 *
 * This routine sends an EOI signal to the IO APIC's interrupting source.
 *
 * @return N/A
 */

void _ioapic_eoi(unsigned int irq /* INT number to send EOI */
			  )
{
	*(volatile unsigned int *)(CONFIG_IOAPIC_BASE_ADDRESS + IOAPIC_EOI) = irq;
	*(volatile unsigned int *)(CONFIG_LOAPIC_BASE_ADDRESS + LOAPIC_EOI) = 0;
}

/**
 *
 * @brief Get EOI (End Of Interrupt) information
 *
 * This routine returns EOI signalling information for a specific IRQ.
 *
 * @return address of routine to be called to signal EOI;
 *          as a side effect, also passes back indication if routine requires
 *          an interrupt vector argument and what the argument value should be
 */

void *_ioapic_eoi_get(unsigned int irq,  /* INTIN number of interest */
		      char *argRequired, /* ptr to "argument required" result
					    area */
		      void **arg /* ptr to "argument value" result area */
		      )
{
#ifndef XIOAPIC_DIRECT_ADDRESSING
	if (!(__IoApicGet(IOAPIC_VERS) & IOAPIC_PRQ)) {
		*argRequired = 0;
		return _loapic_eoi;
	}
#endif

	/* indicate that an argument to the EOI handler is required */

	*argRequired = 1;

	/*
	 * The parameter to the ioApicIntEoi() routine is the vector programmed
	 * into the redirection table.  The interrupt controller's
	 * _SysIntVecAlloc() routine must invoke _IoApicIntEoiGet() after
	 * _IoApicRedVecSet() to ensure the redirection table contains the desired
	 * interrupt vector.
	 */

	*arg = (void *)(ioApicRedGetLo(irq) & IOAPIC_VEC_MASK);

	return _ioapic_eoi;
}

/**
 *
 * @brief Enable a specified APIC interrupt input line
 *
 * This routine enables a specified APIC interrupt input line.
 *
 * @return N/A
 */

void _ioapic_irq_enable(unsigned int irq /* INTIN number to enable */
				 )
{
	_IoApicRedUpdateLo(irq, 0, IOAPIC_INT_MASK);
}

/**
 *
 * @brief Disable a specified APIC interrupt input line
 *
 * This routine disables a specified APIC interrupt input line.
 *
 * @return N/A
 */

void _ioapic_irq_disable(unsigned int irq /* INTIN number to disable */
				  )
{
	_IoApicRedUpdateLo(irq, IOAPIC_INT_MASK, IOAPIC_INT_MASK);
}

/**
 *
 * @brief Programs the interrupt redirection table
 *
 * This routine sets up the redirection table entry for the specified IRQ
 *
 * @return N/A
 */
void _ioapic_irq_set(unsigned int irq, /* virtualized IRQ */
		     unsigned int vector, /* vector number */
		     uint32_t flags    /* interrupt flags */
		     )
{
	uint32_t rteValue;   /* value to copy into redirection table entry */

	rteValue = IOAPIC_FIXED | IOAPIC_INT_MASK | IOAPIC_PHYSICAL |
		   (vector & IOAPIC_VEC_MASK) | flags;
	ioApicRedSetHi(irq, 0);
	ioApicRedSetLo(irq, rteValue);
}

/**
 *
 * @brief Program interrupt vector for specified irq
 *
 * The routine writes the interrupt vector in the Interrupt Redirection
 * Table for specified irq number
 *
 * @return N/A
 */
void _ioapic_int_vec_set(unsigned int irq, /* INT number */
				  unsigned int vector /* vector number */
				  )
{
	_IoApicRedUpdateLo(irq, vector, IOAPIC_VEC_MASK);
}

#ifndef XIOAPIC_DIRECT_ADDRESSING

/**
 *
 * @brief Read a 32 bit IO APIC register
 *
 * This routine reads the specified IO APIC register using indirect addressing.
 *
 * @return register value
 */

static uint32_t __IoApicGet(
	int32_t offset /* register offset (8 bits) */
	)
{
	uint32_t value; /* value */
	int key;	/* interrupt lock level */

	/* lock interrupts to ensure indirect addressing works "atomically" */

	key = irq_lock();

	*((volatile char *)
		(CONFIG_IOAPIC_BASE_ADDRESS + IOAPIC_IND)) = (char)offset;
	value = *((volatile uint32_t *)(CONFIG_IOAPIC_BASE_ADDRESS + IOAPIC_DATA));

	irq_unlock(key);

	return value;
}

/**
 *
 * @brief Write a 32 bit IO APIC register
 *
 * This routine writes the specified IO APIC register using indirect addressing.
 *
 * @return N/A
 */

static void __IoApicSet(
	int32_t offset, /* register offset (8 bits) */
	uint32_t value  /* value to set the register */
	)
{
	int key; /* interrupt lock level */

	/* lock interrupts to ensure indirect addressing works "atomically" */

	key = irq_lock();

	*(volatile char *)(CONFIG_IOAPIC_BASE_ADDRESS + IOAPIC_IND) = (char)offset;
	*((volatile uint32_t *)(CONFIG_IOAPIC_BASE_ADDRESS + IOAPIC_DATA)) = value;

	irq_unlock(key);
}

#endif

/**
 *
 * @brief Get low 32 bits of Redirection Table entry
 *
 * This routine reads the low-order 32 bits of a Redirection Table entry.
 *
 * @return 32 low-order bits
 */

static uint32_t ioApicRedGetLo(unsigned int irq /* INTIN number */
			       )
{
#ifdef XIOAPIC_DIRECT_ADDRESSING
	volatile uint32_t *pEntry; /* pointer to redirection table entry */

	pEntry = (volatile uint32_t *)(CONFIG_IOAPIC_BASE_ADDRESS + (irq * 0x10) +
				       IOAPIC_RTE0_LOW);

	return *pEntry;
#else
	int32_t offset = IOAPIC_REDTBL + (irq << 1); /* register offset */

	return __IoApicGet(offset);
#endif
}

/**
 *
 * @brief Set low 32 bits of Redirection Table entry
 *
 * This routine writes the low-order 32 bits of a Redirection Table entry.
 *
 * @return N/A
 */

static void ioApicRedSetLo(unsigned int irq, /* INTIN number */
				    uint32_t lower32  /* value to be written */
				    )
{
#ifdef XIOAPIC_DIRECT_ADDRESSING
	volatile uint32_t *pEntry; /* pointer to redirection table entry */

	pEntry = (volatile uint32_t *)(CONFIG_IOAPIC_BASE_ADDRESS + (irq * 0x10) +
				       IOAPIC_RTE0_LOW);

	*pEntry = lower32;
#else
	int32_t offset = IOAPIC_REDTBL + (irq << 1); /* register offset */

	__IoApicSet(offset, lower32);
#endif
}

/**
 *
 * @brief Set high 32 bits of Redirection Table entry
 *
 * This routine writes the high-order 32 bits of a Redirection Table entry.
 *
 * @return N/A
 */

static void ioApicRedSetHi(unsigned int irq, /* INTIN number */
			   uint32_t upper32  /* value to be written */
			   )
{
#ifdef XIOAPIC_DIRECT_ADDRESSING
	volatile uint32_t *pEntry; /* pointer to redirection table entry */

	pEntry = (volatile uint32_t *)(CONFIG_IOAPIC_BASE_ADDRESS + (irq * 0x10) +
				       IOAPIC_RTE0_HIGH);

	*pEntry = upper32;
#else
	int32_t offset = IOAPIC_REDTBL + (irq << 1) + 1; /* register offset */

	__IoApicSet(offset, upper32);
#endif
}

/**
 *
 * @brief Modify low 32 bits of Redirection Table entry
 *
 * This routine modifies selected portions of the low-order 32 bits of a
 * Redirection Table entry, as indicated by the associate bit mask.
 *
 * @return N/A
 */

static void _IoApicRedUpdateLo(
	unsigned int irq, /* INTIN number */
	uint32_t value,   /* value to be written */
	uint32_t mask     /* mask of bits to be modified */
	)
{
	ioApicRedSetLo(irq, (ioApicRedGetLo(irq) & ~mask) | (value & mask));
}

#ifdef IOAPIC_MSI_REDIRECT

/*
 * The platform is responsible for defining the IOAPIC_MSI_REDIRECT
 * macro if the I/O APIC supports the MSI redirect capability.
 */

/**
 *
 * @brief Write to the RTE config register for specified IRQ
 *
 * This routine writes the specified 32-bit <value> into the RTE configuration
 * register for the specified <irq> (0 to (CONFIG_IOAPIC_NUM_RTES - 1))
 *
 * @return void
 */

static void _IoApicRteConfigSet(unsigned int irq, /* INTIN number */
			 uint32_t value    /* value to be written */
			 )
{
	unsigned int offset; /* register offset */

#ifdef CONFIG_IOAPIC_DEBUG
	if (irq >= CONFIG_IOAPIC_NUM_RTES)
		return; /* do nothing if <irq> is invalid */
#endif

	offset = IOAPIC_RTE0_CONFIG + (irq * 0x10);

	/* use direct addressing when writing to RTE config register */

	*((volatile uint32_t *)(CONFIG_IOAPIC_BASE_ADDRESS + offset)) = value;
}

/**
 *
 * @brief Write to the specified MSI redirection register
 *
 * This routine writes the 32-bit <value> into the redirection register
 * specified by <reg>.
 *
 * @return void
 */

static void _IoApicRedirRegSet(unsigned int reg, uint32_t value)
{
	unsigned int offset; /* register offset */

#ifdef CONFIG_IOAPIC_DEBUG
	if ((reg > 7) || (reg == 0))
		return; /* do nothing if <reg> is invalid */
#endif

	offset = IOAPIC_REDIR_ADDR0 + (reg * 4);

	/* use direct addressing when writing to RTE config register */

	*((volatile uint32_t *)(CONFIG_IOAPIC_BASE_ADDRESS + offset)) = value;
}

#endif /* IOAPIC_MSI_REDIRECT */
