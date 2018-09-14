/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_SEGMENTATION_H_
#define ZEPHYR_INCLUDE_ARCH_X86_SEGMENTATION_H_

#include <zephyr/types.h>

/* Host gen_idt uses this header as well, don't depend on toolchain.h */
#ifndef __packed
#define __packed __attribute__((packed))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Bitmask used to determine which exceptions result in an error code being
 * pushed onto the stack.  The following exception vectors push an error code:
 *
 *  Vector    Mnemonic    Description
 *  ------    -------     ----------------------
 *    8       #DF         Double Fault
 *    10      #TS         Invalid TSS
 *    11      #NP         Segment Not Present
 *    12      #SS         Stack Segment Fault
 *    13      #GP         General Protection Fault
 *    14      #PF         Page Fault
 *    17      #AC         Alignment Check
 */
#define _EXC_ERROR_CODE_FAULTS	0x27d00


/* NOTE: We currently do not have definitions for 16-bit segment, currently
 * assume everything we are working with is 32-bit
 */

#define SEG_TYPE_LDT		0x2
#define SEG_TYPE_TASK_GATE	0x5
#define SEG_TYPE_TSS		0x9
#define SEG_TYPE_TSS_BUSY	0xB
#define SEG_TYPE_CALL_GATE	0xC
#define SEG_TYPE_IRQ_GATE	0xE
#define SEG_TYPE_TRAP_GATE	0xF

#define DT_GRAN_BYTE	0
#define DT_GRAN_PAGE	1

#define DT_READABLE	1
#define DT_NON_READABLE	0

#define DT_WRITABLE	1
#define DT_NON_WRITABLE	0

#define DT_EXPAND_DOWN	1
#define DT_EXPAND_UP	0

#define DT_CONFORM	1
#define DT_NONCONFORM	0

#define DT_TYPE_SYSTEM		0
#define DT_TYPE_CODEDATA	1

#ifndef _ASMLANGUAGE

/* Section 7.2.1 of IA architecture SW developer manual, Vol 3. */
struct __packed task_state_segment {
	u16_t backlink;
	u16_t reserved_1;
	u32_t esp0;
	u16_t ss0;
	u16_t reserved_2;
	u32_t esp1;
	u16_t ss1;
	u16_t reserved_3;
	u32_t esp2;
	u16_t ss2;
	u16_t reserved_4;
	u32_t cr3;
	u32_t eip;
	u32_t eflags;
	u32_t eax;
	u32_t ecx;
	u32_t edx;
	u32_t ebx;
	u32_t esp;
	u32_t ebp;
	u32_t esi;
	u32_t edi;
	u16_t es;
	u16_t reserved_5;
	u16_t cs;
	u16_t reserved_6;
	u16_t ss;
	u16_t reserved_7;
	u16_t ds;
	u16_t reserved_8;
	u16_t fs;
	u16_t reserved_9;
	u16_t gs;
	u16_t reserved_10;
	u16_t ldt_ss;
	u16_t reserved_11;
	u8_t t:1;		/* Trap bit */
	u16_t reserved_12:15;
	u16_t iomap;
};

/* Section 3.4.2 of IA architecture SW developer manual, Vol 3. */
struct __packed segment_selector {
	union {
		struct {
			u8_t rpl:2;
			u8_t table:1; /* 0=gdt 1=ldt */
			u16_t index:13;
		};
		u16_t val;
	};
};

#define SEG_SELECTOR(index, table, dpl) (index << 3 | table << 2 | dpl)

/* References
 *
 * Section 5.8.3 (Call gates)
 * Section 7.2.2 (TSS Descriptor)
 * Section 3.4.5 (Segment descriptors)
 * Section 6.11 (IDT Descriptors)
 *
 * IA architecture SW developer manual, Vol 3.
 */
struct __packed segment_descriptor {

	/* First DWORD: 0-15 */
	union {
		/* IRQ, call, trap gates */
		u16_t limit_low;

		/* Task gates */
		u16_t reserved_task_gate_0;

		/* Everything else */
		u16_t offset_low;
	};

	/* First DWORD: 16-31 */
	union {
		/* Call/Task/Interrupt/Trap gates */
		u16_t segment_selector;

		/* TSS/LDT/Segments */
		u16_t base_low;	/* Bits 0-15 */
	};

	/* Second DWORD: 0-7 */
	union {
		/* TSS/LDT/Segments */
		u8_t base_mid;	/* Bits 16-23 */

		/* Task gates */
		u8_t reserved_task_gate_1;

		/* IRQ/Trap/Call Gates */
		struct {
			/* Reserved except in case of call gates */
			u8_t reserved_or_param:5;

			/* Bits 5-7 0 0 0 per CPU manual */
			u8_t always_0_0:3;
		};
	};

	/* Second DWORD: 8-15 */
	union {
		/* Code or data Segments */
		struct {
			/* Set by the processor, init to 0 */
			u8_t accessed:1;

			/* executable ? readable : writable */
			u8_t rw:1;
			/* executable ? conforming : direction */
			u8_t cd:1;
			/* 1=code 0=data */
			u8_t executable:1;

			/* Next 3 fields actually common to all */

			/* 1=code or data, 0=system type */
			u8_t descriptor_type:1;

			u8_t dpl:2;
			u8_t present:1;
		};

		/* System types */
		struct {
			/* One of the SEG_TYPE_* macros above */
			u8_t type:4;

			/* Alas, C doesn't let you do a union of the first
			 * 4 bits of a bitfield and put the rest outside of it,
			 * it ends up getting padded.
			 */
			u8_t use_other_union:4;
		};
	};

	/* Second DWORD: 16-31 */
	union {
		/* Call/IRQ/trap gates */
		u16_t offset_hi;

		/* Task Gates */
		u16_t reserved_task_gate_2;

		/* segment/LDT/TSS */
		struct {
			u8_t limit_hi:4;

			/* flags */
			u8_t avl:1;		/* CPU ignores this */

			/* 1=Indicates 64-bit code segment in IA-32e mode */
			u8_t flags_l:1; /* L field */

			u8_t db:1; /* D/B field 1=32-bit 0=16-bit*/
			u8_t granularity:1;

			u8_t base_hi;	/* Bits 24-31 */
		};
	};

};


/* Address of this passed to lidt/lgdt.
 * IA manual calls this a 'pseudo descriptor'.
 */
struct __packed pseudo_descriptor {
	u16_t size;
	struct segment_descriptor *entries;
};


/*
 * Full linear address (segment selector+offset), for far jumps/calls
 */
struct __packed far_ptr {
	/** Far pointer offset, unused when invoking a task. */
	void *offset;
	/** Far pointer segment/gate selector. */
	u16_t sel;
};


#define DT_ZERO_ENTRY { { 0 } }

/* NOTE: the below macros only work for fixed addresses provided at build time.
 * Base addresses or offsets cannot be &some_variable, as pointer values are not
 * known until link time and the compiler has to split the address into various
 * fields in the segment selector well before that.
 *
 * If you really need to put &some_variable as the base address in some
 * segment descriptor, you will either need to do the assignment at runtime
 * or implement some tool to populate values post-link like gen_idt does.
 */
#define _LIMIT_AND_BASE(base_p, limit_p, granularity_p) \
	.base_low = (((u32_t)base_p) & 0xFFFF), \
	.base_mid = (((base_p) >> 16) & 0xFF), \
	.base_hi = (((base_p) >> 24) & 0xFF), \
	.limit_low = ((limit_p) & 0xFFFF), \
	.limit_hi = (((limit_p) >> 16) & 0xF), \
	.granularity = (granularity_p), \
	.flags_l = 0, \
	.db = 1, \
	.avl = 0

#define _SEGMENT_AND_OFFSET(segment_p, offset_p) \
	.segment_selector = (segment_p), \
	.offset_low = ((offset_p) & 0xFFFF), \
	.offset_hi = ((offset_p) >> 16)

#define _DESC_COMMON(dpl_p) \
	.dpl = (dpl_p), \
	.present = 1

#define _SYS_DESC(type_p) \
	.type = type_p, \
	.descriptor_type = 0

#define DT_CODE_SEG_ENTRY(base_p, limit_p, granularity_p, dpl_p, readable_p, \
		       conforming_p) \
	{ \
		_DESC_COMMON(dpl_p), \
		_LIMIT_AND_BASE(base_p, limit_p, granularity_p), \
		.accessed = 0, \
		.rw = (readable_p), \
		.cd = (conforming_p), \
		.executable = 1, \
		.descriptor_type = 1 \
	}

#define DT_DATA_SEG_ENTRY(base_p, limit_p, granularity_p, dpl_p, writable_p, \
		       direction_p) \
	{ \
		_DESC_COMMON(dpl_p), \
		_LIMIT_AND_BASE(base_p, limit_p, granularity_p), \
		.accessed = 0, \
		.rw = (writable_p), \
		.cd = (direction_p), \
		.executable = 0, \
		.descriptor_type = 1 \
	}

#define DT_LDT_ENTRY(base_p, limit_p, granularity_p, dpl_p) \
	{ \
		_DESC_COMMON(dpl_p), \
		_LIMIT_AND_BASE(base_p, limit_p, granularity_p), \
		_SYS_DESC(SEG_TYPE_LDT) \
	}

#define DT_TSS_ENTRY(base_p, limit_p, granularity_p, dpl_p) \
	{ \
		_DESC_COMMON(dpl_p), \
		_LIMIT_AND_BASE(base_p, limit_p, granularity_p), \
		_SYS_DESC(SEG_TYPE_TSS) \
	}

/* "standard" TSS segments that don't stuff extra data past the end of the
 * TSS struct
 */
#define DT_TSS_STD_ENTRY(base_p, dpl_p) \
	DT_TSS_ENTRY(base_p, sizeof(struct task_state_segment), DT_GRAN_BYTE, \
		     dpl_p)

#define DT_TASK_GATE_ENTRY(segment_p, dpl_p) \
	{ \
		_DESC_COMMON(dpl_p), \
		_SYS_DESC(SEG_TYPE_TASK_GATE), \
		.segment_selector = (segment_p) \
	}

#define DT_IRQ_GATE_ENTRY(segment_p, offset_p, dpl_p) \
	{ \
		_DESC_COMMON(dpl_p), \
		_SEGMENT_AND_OFFSET(segment_p, offset_p), \
		_SYS_DESC(SEG_TYPE_IRQ_GATE), \
		.always_0_0 = 0 \
	}

#define DT_TRAP_GATE_ENTRY(segment_p, offset_p, dpl_p) \
	{ \
		_DESC_COMMON(dpl_p), \
		_SEGMENT_AND_OFFSET(segment_p, offset_p), \
		_SYS_DESC(SEG_TYPE_TRAP_GATE), \
		.always_0_0 = 0 \
	}

#define DT_CALL_GATE_ENTRY(segment_p, offset_p, dpl_p, param_count_p) \
	{ \
		_DESC_COMMON(dpl_p), \
		_SEGMENT_AND_OFFSET(segment_p, offset_p), \
		_SYS_DESC(SEG_TYPE_TRAP_GATE), \
		.reserved_or_param = (param_count_p), \
		.always_0_0 = 0 \
	}

#define DTE_BASE(dt_entry) ((dt_entry)->base_low | \
			    ((dt_entry)->base_mid << 16) | \
			    ((dt_entry)->base_hi << 24))

#define DTE_LIMIT(dt_entry) ((dt_entry)->limit_low | \
			     ((dt_entry)->limit_hi << 16))

#define DTE_OFFSET(dt_entry) ((dt_entry)->offset_low | \
			      ((dt_entry)->offset_hi << 16))

#define DT_INIT(entries) { sizeof(entries) - 1, &entries[0] }

#ifdef CONFIG_SET_GDT
/* This is either the ROM-based GDT in crt0.S or generated by gen_gdt.py,
 * depending on CONFIG_GDT_DYNAMIC
 */
extern struct pseudo_descriptor _gdt;
#endif

/**
 * Properly set the segment descriptor segment and offset
 *
 * Used for call/interrupt/trap gates
 *
 * @param sd Segment descriptor
 * @param offset Offset within segment
 * @param segment_selector Segment selector
 */
static inline void _sd_set_seg_offset(struct segment_descriptor *sd,
				      u16_t segment_selector,
				      u32_t offset)
{
	sd->offset_low = offset & 0xFFFF;
	sd->offset_hi = offset >> 16;
	sd->segment_selector = segment_selector;
	sd->always_0_0 = 0;
}


/**
 * Initialize an segment descriptor to be a 32-bit IRQ gate
 *
 * @param sd Segment descriptor memory
 * @param seg_selector Segment selector of handler
 * @param offset offset of handler
 * @param dpl descriptor privilege level
 */
static inline void _init_irq_gate(struct segment_descriptor *sd,
				  u16_t seg_selector, u32_t offset,
				  u32_t dpl)
{
	_sd_set_seg_offset(sd, seg_selector, offset);
	sd->dpl = dpl;
	sd->descriptor_type = DT_TYPE_SYSTEM;
	sd->present = 1;
	sd->type = SEG_TYPE_IRQ_GATE;
}

/**
 * Set current IA task TSS
 *
 * @param sel Segment selector in GDT for desired TSS
 */
static inline void _set_tss(u16_t sel)
{
	__asm__ __volatile__ ("ltr %0" :: "r" (sel));
}


/**
 * Get the TSS segment selector in the GDT for the current IA task
 *
 * @return Segment selector for current IA task
 */
static inline u16_t _get_tss(void)
{
	u16_t sel;

	__asm__ __volatile__ ("str %0" : "=r" (sel));
	return sel;
}


/**
 * Get the current global descriptor table
 *
 * @param gdt Pointer to memory to receive GDT pseudo descriptor information
 */
static inline void _get_gdt(struct pseudo_descriptor *gdt)
{
	__asm__ __volatile__ ("sgdt %0" : "=m" (*gdt));
}


/**
 * Get the current interrupt descriptor table
 *
 * @param idt Pointer to memory to receive IDT pseudo descriptor information
 */
static inline void _get_idt(struct pseudo_descriptor *idt)
{
	__asm__ __volatile__ ("sidt %0" : "=m" (*idt));
}


/**
 * Get the current local descriptor table (LDT)
 *
 * @return Segment selector in the GDT for the current LDT
 */
static inline u16_t _get_ldt(void)
{
	u16_t ret;

	__asm__ __volatile__ ("sldt %0" : "=m" (ret));
	return ret;
}


/**
 * Set the local descriptor table for the current IA Task
 *
 * @param ldt Segment selector in the GDT for an LDT
 */
static inline void _set_ldt(u16_t ldt)
{
	__asm__ __volatile__ ("lldt %0" :: "m" (ldt));

}

/**
 * Set the global descriptor table
 *
 * You will most likely need to update all the data segment registers
 * and do a far call to the code segment.
 *
 * @param gdt Pointer to GDT pseudo descriptor.
 */
static inline void _set_gdt(const struct pseudo_descriptor *gdt)
{
	__asm__ __volatile__ ("lgdt %0" :: "m" (*gdt));
}


/**
 * Set the interrupt descriptor table
 *
 * @param idt Pointer to IDT pseudo descriptor.
 */
static inline void _set_idt(const struct pseudo_descriptor *idt)
{
	__asm__ __volatile__ ("lidt %0" :: "m" (*idt));
}


/**
 * Get the segment selector for the current code segment
 *
 * @return Segment selector
 */
static inline u16_t _get_cs(void)
{
	u16_t cs = 0;

	__asm__ __volatile__ ("mov %%cs, %0" : "=r" (cs));
	return cs;
}


/**
 * Get the segment selector for the current data segment
 *
 * @return Segment selector
 */
static inline u16_t _get_ds(void)
{
	u16_t ds = 0;

	__asm__ __volatile__ ("mov %%ds, %0" : "=r" (ds));
	return ds;
}


#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_X86_SEGMENTATION_H_ */
