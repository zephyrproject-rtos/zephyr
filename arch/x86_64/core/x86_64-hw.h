/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _X86_64_HW_H
#define _X86_64_HW_H

/*
 * Struct declarations and helper inlines for core x86_64 hardware
 * functionality.  Anything related to ioports, CR's MSR's, I/L/GDTs,
 * PTEs or (IO-)APICs can be found here.  Note that because this
 * header is included in limited stub contexts, it should include
 * declarations and inlines only: no data definitions, even extern
 * ones!
 */

static inline unsigned long eflags(void)
{
	int eflags;

	__asm__ volatile("pushfq; pop %%rax" : "=a"(eflags));
	return eflags;
}

/* PAE page table record.  Note that "addr" is aligned naturally as an
 * address, but of course must be masked to change only significant
 * bits (which depend on whether it's storing a 4k, 2M or 1G memory
 * block) so as to not clobber the bitfields (remember "negative"
 * addresses must mask off the top bits too!).  The natural idiom is
 * to assign addr first, then write the bitfields.
 */
struct pte64 {
	union {
		unsigned long long addr;
		struct {
			unsigned long long present : 1;
			unsigned long long writable : 1;
			unsigned long long usermode : 1;
			unsigned long long writethrough : 1;
			unsigned long long uncached : 1;
			unsigned long long accessed : 1;
			unsigned long long dirty : 1;
			unsigned long long pagesize_pat : 1;
			unsigned long long global : 1;
			unsigned long long _UNUSED1 : 3;
			unsigned long long pat : 1;
			unsigned long long _UNUSED2 : 50;
			unsigned long long exdisable : 1;
		};
	};
};

struct gdt64 {
	union {
		unsigned int dwords[2];
		struct {
			unsigned long long limit_lo16 : 16;
			unsigned long long base_lo16 : 16;
			unsigned long long base_mid8 : 8;
			unsigned long long accessed : 1;
			unsigned long long readable : 1;
			unsigned long long conforming : 1;
			unsigned long long codeseg : 1;
			unsigned long long notsystem : 1;
			unsigned long long ring : 2;
			unsigned long long present : 1;
			unsigned long long limit_hi4 : 4;
			unsigned long long available : 1;
			unsigned long long long64 : 1;
			unsigned long long default_size : 1;
			unsigned long long page_granularity : 1;
			unsigned long long base_hi8 : 8;
		};
	};
};

static inline void gdt64_set_base(struct gdt64 *g, unsigned int base)
{
	g->base_lo16 = base & 0xffff;
	g->base_mid8 = (base >> 16) & 0xff;
	g->base_hi8 = base >> 24;
}

#define GDT_SELECTOR(seg) ((seg) << 3)

struct idt64 {
	unsigned short offset_lo16;
	unsigned short segment;
	unsigned int ist : 3;
	unsigned int _UNUSED1 : 5;
	unsigned int type : 4;
	unsigned int _UNUSED2 : 1;
	unsigned int ring : 2;
	unsigned int present : 1;
	unsigned short offset_mid16;
	unsigned int offset_hi32;
	unsigned int _UNUSED3;
};

static inline void idt64_set_isr(struct idt64 *desc, void *isr)
{
	unsigned long long addr = (unsigned long)isr;

	desc->offset_lo16 = addr & 0xffff;
	desc->offset_mid16 = (addr >> 16) & 0xffff;
	desc->offset_hi32 = addr >> 32;
}

enum apic_delivery_mode {
	FIXED = 0, LOWEST = 1, SMI = 2, NMI = 4,
	INIT = 5, STARTUP = 6, EXTINT = 7,
};

struct apic_icr_lo {
	unsigned int vector : 8;
	enum apic_delivery_mode delivery_mode : 3;
	unsigned int logical : 1;
	unsigned int send_pending : 1;
	unsigned int _unused : 1;
	unsigned int assert : 1;
	unsigned int level_trig : 1;
	unsigned int _unused2 : 2;
	enum { NONE, SELF, ALL, NOTSELF } shorthand : 2;
};

struct apic_icr_hi {
	unsigned int _unused : 24;
	unsigned int destination : 8;
};

/* Generic struct, not all field applicable to all LVT interrupts */
struct apic_lvt {
	unsigned int vector : 8;
	enum apic_delivery_mode delivery_mode : 4;
	unsigned int _UNUSED : 1;
	unsigned int send_pending : 1;
	unsigned int polarity : 1;
	unsigned int remote_irr : 1;
	unsigned int level_trig : 1;
	unsigned int masked : 1;
	enum { ONESHOT, PERIODIC, TSCDEADLINE } mode : 2;
};

/* Memory-mapped local APIC registers.  Note that the registers are
 * always the first dword in a 16 byte block, the other 3 being
 * unused.  So each line represents one of these registers, or an
 * array thereof.  Lots of (_u)nused fields in the layout, but the usage
 * becomes pleasingly clean.
 */
struct apic_regs {
	unsigned int _u1[4][2];
	unsigned int ID, _u2[3];
	unsigned int VER, _u3[3];
	unsigned int _u4[4][4];
	unsigned int TPR, _u5[3];
	unsigned int APR, _u6[3];
	unsigned int PPR, _u7[3];
	unsigned int EOI, _u8[3];
	unsigned int RRD, _u9[3];
	unsigned int LDR, _u10[3];
	unsigned int DFR, _u11[3];
	unsigned int SPURIOUS, _u12[3];
	unsigned int ISR_BITS[4][8];
	unsigned int TMR_BITS[4][8];
	unsigned int IRR_BITS[4][8];
	unsigned int ERR_STATUS, _u13[3];
	unsigned int _u14[4][6];
	struct apic_lvt LVT_CMCI; unsigned int _u15[3];
	struct apic_icr_lo ICR_LO, _u16[3];
	struct apic_icr_hi ICR_HI, _u17[3];
	struct apic_lvt LVT_TIMER; unsigned int _u18[3];
	struct apic_lvt LVT_THERMAL; unsigned int _u19[3];
	struct apic_lvt LVT_PERF; unsigned int _u20[3];
	struct apic_lvt LVT_LINT0; unsigned int _u21[3];
	struct apic_lvt LVT_LINT1; unsigned int _u22[3];
	struct apic_lvt LVT_ERROR; unsigned int _u23[3];
	unsigned int INIT_COUNT, _u24[3];
	unsigned int CURR_COUNT, _u25[3];
	unsigned int _u26[4][4];
	unsigned int DIVIDE_CONF, _u27[3];
};

#define _apic (*((volatile struct apic_regs *)0xfee00000ll))

/* Crazy encoding for this, but susceptable to a formula.  Returns the
 * DIVIDE_CONF register value that divides the input clock by 2^n (n
 * in the range 0-7).
 */
#define APIC_DIVISOR(n) (((((n) - 1) << 1) & 8)|(((n) - 1) & 3))

#define IOREGSEL (*(volatile unsigned int *)0xfec00000l)
#define IOREGWIN (*(volatile unsigned int *)0xfec00010l)

/* Assumes one IO-APIC.  Note that because of the way the register API
 * works, this must be spinlocked or otherwise protected against other
 * CPUs (e.g. do it all on cpu0 at startup, etc...).
 */
static inline unsigned int ioapic_read(int reg)
{
	IOREGSEL = reg;
	return IOREGWIN;
}

static inline void ioapic_write(int reg, unsigned int val)
{
	IOREGSEL = reg;
	IOREGWIN = val;
}

/* IOAPIC redirection table entry */
struct ioapic_red {
	union {
		unsigned int regvals[2];
		struct {
			unsigned int vector : 8;
			enum apic_delivery_mode : 3;
			unsigned int logical : 1;
			unsigned int send_pending : 1;
			unsigned int active_low : 1;
			unsigned int remote_irr : 1;
			unsigned int level_triggered : 1;
			unsigned int masked : 1;
			unsigned int _UNUSED1 : 15;
			unsigned int _UNUSED2 : 24;
			unsigned int destination : 8;
		};
	};
};

#define GET_CR(reg) ({ unsigned int _r;					\
			__asm__ volatile("movl %%" reg ", %0\n\t"	\
					 : "=r"(_r));			\
			_r; })

#define SET_CR(reg, val)						\
	do {								\
		int tmp = val;						\
		__asm__ volatile("movl %0, %%" reg "\n\t" :: "r"(tmp)); \
	} while (0)

#define SET_CR_BIT(reg, bit) SET_CR(reg, GET_CR(reg) | (1 << bit))

static inline void ioport_out8(unsigned short port, unsigned char b)
{
	__asm__ volatile("outb %0, %1;\n\t" : : "a"(b), "d"(port));
}


static inline unsigned char ioport_in8(unsigned short port)
{
	unsigned char ret;

	__asm__ volatile("inb %1, %0;\n\t" : "=a"(ret) : "d"(port));
	return ret;
}

static inline void set_msr_bit(unsigned int msr, int bit)
{
	unsigned int mask = 1 << bit;

	__asm__ volatile("rdmsr; or %0, %%eax; wrmsr"
			 :: "r"(mask), "c"(msr) : "eax", "edx");
}

static inline unsigned int get_msr(unsigned int msr)
{
	unsigned int val;

	__asm__ volatile("rdmsr" : "=a"(val) : "c"(msr) : "edx");
	return val;
}

static inline unsigned long long rdtsc(void)
{
	unsigned long long rax, rdx;

	__asm__ volatile("rdtsc" : "=a"(rax), "=d"(rdx));
	return rdx << 32 | rax;
}

#endif /* _X86_64_HW_H */
