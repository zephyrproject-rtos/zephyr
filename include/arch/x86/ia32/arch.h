/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IA-32 specific kernel interface header
 * This header contains the IA-32 specific kernel interface.  It is included
 * by the generic kernel interface header (include/arch/cpu.h)
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_IA32_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_X86_IA32_ARCH_H_

#include "sys_io.h"
#include <drivers/interrupt_controller/sysapic.h>
#include <kernel_arch_thread.h>
#include <stdbool.h>
#include <arch/common/ffs.h>
#include <misc/util.h>
#include <arch/x86/ia32/syscall.h>

#ifndef _ASMLANGUAGE
#include <stddef.h>	/* for size_t */

#include <arch/common/addr_types.h>
#include <arch/x86/ia32/segmentation.h>

#endif /* _ASMLANGUAGE */

/* GDT layout */
#define CODE_SEG	0x08
#define DATA_SEG	0x10
#define MAIN_TSS	0x18
#define DF_TSS		0x20

/**
 * Macro used internally by NANO_CPU_INT_REGISTER and NANO_CPU_INT_REGISTER_ASM.
 * Not meant to be used explicitly by platform, driver or application code.
 */
#define MK_ISR_NAME(x) __isr__##x

#define Z_DYN_STUB_SIZE			4
#define Z_DYN_STUB_OFFSET		0
#define Z_DYN_STUB_LONG_JMP_EXTRA_SIZE	3
#define Z_DYN_STUB_PER_BLOCK		32


#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif


/* interrupt/exception/error related definitions */

/*
 * The TCS must be aligned to the same boundary as that used by the floating
 * point register set.  This applies even for threads that don't initially
 * use floating point, since it is possible to enable floating point support
 * later on.
 */

#define STACK_ALIGN  FP_REG_SET_ALIGN

typedef struct s_isrList {
	/** Address of ISR/stub */
	void		*fnc;
	/** IRQ associated with the ISR/stub, or -1 if this is not
	 * associated with a real interrupt; in this case vec must
	 * not be -1
	 */
	unsigned int    irq;
	/** Priority associated with the IRQ. Ignored if vec is not -1 */
	unsigned int    priority;
	/** Vector number associated with ISR/stub, or -1 to assign based
	 * on priority
	 */
	unsigned int    vec;
	/** Privilege level associated with ISR/stub */
	unsigned int    dpl;

	/** If nonzero, specifies a TSS segment selector. Will configure
	 * a task gate instead of an interrupt gate. fnc parameter will be
	 * ignored
	 */
	unsigned int	tss;
} ISR_LIST;


/**
 * @brief Connect a routine to an interrupt vector
 *
 * This macro "connects" the specified routine, @a r, to the specified interrupt
 * vector, @a v using the descriptor privilege level @a d. On the IA-32
 * architecture, an interrupt vector is a value from 0 to 255. This macro
 * populates the special intList section with the address of the routine, the
 * vector number and the descriptor privilege level. The genIdt tool then picks
 * up this information and generates an actual IDT entry with this information
 * properly encoded.
 *
 * The @a d argument specifies the privilege level for the interrupt-gate
 * descriptor; (hardware) interrupts and exceptions should specify a level of 0,
 * whereas handlers for user-mode software generated interrupts should specify 3.
 * @param r Routine to be connected
 * @param n IRQ number
 * @param p IRQ priority
 * @param v Interrupt Vector
 * @param d Descriptor Privilege Level
 *
 * @return N/A
 *
 */

#define NANO_CPU_INT_REGISTER(r, n, p, v, d) \
	 static ISR_LIST __attribute__((section(".intList"))) \
			 __attribute__((used)) MK_ISR_NAME(r) = \
			{ \
				.fnc = &(r), \
				.irq = (n), \
				.priority = (p), \
				.vec = (v), \
				.dpl = (d), \
				.tss = 0 \
			}

/**
 * @brief Connect an IA hardware task to an interrupt vector
 *
 * This is very similar to NANO_CPU_INT_REGISTER but instead of connecting
 * a handler function, the interrupt will induce an IA hardware task
 * switch to another hardware task instead.
 *
 * @param tss_p GDT/LDT segment selector for the TSS representing the task
 * @param irq_p IRQ number
 * @param priority_p IRQ priority
 * @param vec_p Interrupt vector
 * @param dpl_p Descriptor privilege level
 */
#define _X86_IDT_TSS_REGISTER(tss_p, irq_p, priority_p, vec_p, dpl_p) \
	static ISR_LIST __attribute__((section(".intList"))) \
			__attribute__((used)) MK_ISR_NAME(r) = \
			{ \
				.fnc = NULL, \
				.irq = (irq_p), \
				.priority = (priority_p), \
				.vec = (vec_p), \
				.dpl = (dpl_p), \
				.tss = (tss_p) \
			}

/**
 * Code snippets for populating the vector ID and priority into the intList
 *
 * The 'magic' of static interrupts is accomplished by building up an array
 * 'intList' at compile time, and the gen_idt tool uses this to create the
 * actual IDT data structure.
 *
 * For controllers like APIC, the vectors in the IDT are not normally assigned
 * at build time; instead the sentinel value -1 is saved, and gen_idt figures
 * out the right vector to use based on our priority scheme. Groups of 16
 * vectors starting at 32 correspond to each priority level.
 *
 * These macros are only intended to be used by IRQ_CONNECT() macro.
 */
#define _VECTOR_ARG(irq_p)	(-1)

/* Internally this function does a few things:
 *
 * 1. There is a declaration of the interrupt parameters in the .intList
 * section, used by gen_idt to create the IDT. This does the same thing
 * as the NANO_CPU_INT_REGISTER() macro, but is done in assembly as we
 * need to populate the .fnc member with the address of the assembly
 * IRQ stub that we generate immediately afterwards.
 *
 * 2. The IRQ stub itself is declared. The code will go in its own named
 * section .text.irqstubs section (which eventually gets linked into 'text')
 * and the stub shall be named (isr_name)_irq(irq_line)_stub
 *
 * 3. The IRQ stub pushes the ISR routine and its argument onto the stack
 * and then jumps to the common interrupt handling code in _interrupt_enter().
 *
 * 4. z_irq_controller_irq_config() is called at runtime to set the mapping
 * between the vector and the IRQ line as well as triggering flags
 */
#define Z_ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
({ \
	__asm__ __volatile__(							\
		".pushsection .intList\n\t" \
		".long %c[isr]_irq%c[irq]_stub\n\t"	/* ISR_LIST.fnc */ \
		".long %c[irq]\n\t"		/* ISR_LIST.irq */ \
		".long %c[priority]\n\t"	/* ISR_LIST.priority */ \
		".long %c[vector]\n\t"		/* ISR_LIST.vec */ \
		".long 0\n\t"			/* ISR_LIST.dpl */ \
		".long 0\n\t"			/* ISR_LIST.tss */ \
		".popsection\n\t" \
		".pushsection .text.irqstubs\n\t" \
		".global %c[isr]_irq%c[irq]_stub\n\t" \
		"%c[isr]_irq%c[irq]_stub:\n\t" \
		"pushl %[isr_param]\n\t" \
		"pushl %[isr]\n\t" \
		"jmp _interrupt_enter\n\t" \
		".popsection\n\t" \
		: \
		: [isr] "i" (isr_p), \
		  [isr_param] "i" (isr_param_p), \
		  [priority] "i" (priority_p), \
		  [vector] "i" _VECTOR_ARG(irq_p), \
		  [irq] "i" (irq_p)); \
	z_irq_controller_irq_config(Z_IRQ_TO_INTERRUPT_VECTOR(irq_p), (irq_p), \
				   (flags_p)); \
	Z_IRQ_TO_INTERRUPT_VECTOR(irq_p); \
})

#define Z_ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p) \
({ \
	NANO_CPU_INT_REGISTER(isr_p, irq_p, priority_p, -1, 0); \
	z_irq_controller_irq_config(Z_IRQ_TO_INTERRUPT_VECTOR(irq_p), (irq_p), \
				   (flags_p)); \
	Z_IRQ_TO_INTERRUPT_VECTOR(irq_p); \
})


#ifdef CONFIG_SYS_POWER_MANAGEMENT
extern void z_arch_irq_direct_pm(void);
#define Z_ARCH_ISR_DIRECT_PM() z_arch_irq_direct_pm()
#else
#define Z_ARCH_ISR_DIRECT_PM() do { } while (false)
#endif

#define Z_ARCH_ISR_DIRECT_HEADER() z_arch_isr_direct_header()
#define Z_ARCH_ISR_DIRECT_FOOTER(swap) z_arch_isr_direct_footer(swap)

/* FIXME prefer these inline, but see GH-3056 */
extern void z_arch_isr_direct_header(void);
extern void z_arch_isr_direct_footer(int maybe_swap);

#define Z_ARCH_ISR_DIRECT_DECLARE(name) \
	static inline int name##_body(void); \
	__attribute__ ((interrupt)) void name(void *stack_frame) \
	{ \
		ARG_UNUSED(stack_frame); \
		int check_reschedule; \
		ISR_DIRECT_HEADER(); \
		check_reschedule = name##_body(); \
		ISR_DIRECT_FOOTER(check_reschedule); \
	} \
	static inline int name##_body(void)

/**
 * @brief Exception Stack Frame
 *
 * A pointer to an "exception stack frame" (ESF) is passed as an argument
 * to exception handlers registered via nanoCpuExcConnect().  As the system
 * always operates at ring 0, only the EIP, CS and EFLAGS registers are pushed
 * onto the stack when an exception occurs.
 *
 * The exception stack frame includes the volatile registers (EAX, ECX, and
 * EDX) as well as the 5 non-volatile registers (EDI, ESI, EBX, EBP and ESP).
 * Those registers are pushed onto the stack by _ExcEnt().
 */

typedef struct nanoEsf {
	unsigned int esp;
	unsigned int ebp;
	unsigned int ebx;
	unsigned int esi;
	unsigned int edi;
	unsigned int edx;
	unsigned int eax;
	unsigned int ecx;
	unsigned int errorCode;
	unsigned int eip;
	unsigned int cs;
	unsigned int eflags;
} z_arch_esf_t;


struct _x86_syscall_stack_frame {
	u32_t eip;
	u32_t cs;
	u32_t eflags;

	/* These are only present if cs = USER_CODE_SEG */
	u32_t esp;
	u32_t ss;
};

static ALWAYS_INLINE unsigned int z_arch_irq_lock(void)
{
	unsigned int key;

	__asm__ volatile ("pushfl; cli; popl %0" : "=g" (key) :: "memory");

	return key;
}


/**
 * The NANO_SOFT_IRQ macro must be used as the value for the @a irq parameter
 * to NANO_CPU_INT_REGISTER when connecting to an interrupt that does not
 * correspond to any IRQ line (such as spurious vector or SW IRQ)
 */
#define NANO_SOFT_IRQ	((unsigned int) (-1))

/**
 * @defgroup float_apis Floating Point APIs
 * @ingroup kernel_apis
 * @{
 */

struct k_thread;

/**
 * @brief Enable preservation of floating point context information.
 *
 * This routine informs the kernel that the specified thread (which may be
 * the current thread) will be using the floating point registers.
 * The @a options parameter indicates which floating point register sets
 * will be used by the specified thread:
 *
 *  a) K_FP_REGS  indicates x87 FPU and MMX registers only
 *  b) K_SSE_REGS indicates SSE registers (and also x87 FPU and MMX registers)
 *
 * Invoking this routine initializes the thread's floating point context info
 * to that of an FPU that has been reset. The next time the thread is scheduled
 * by z_swap() it will either inherit an FPU that is guaranteed to be in a "sane"
 * state (if the most recent user of the FPU was cooperatively swapped out)
 * or the thread's own floating point context will be loaded (if the most
 * recent user of the FPU was preempted, or if this thread is the first user
 * of the FPU). Thereafter, the kernel will protect the thread's FP context
 * so that it is not altered during a preemptive context switch.
 *
 * @warning
 * This routine should only be used to enable floating point support for a
 * thread that does not currently have such support enabled already.
 *
 * @param thread ID of thread.
 * @param options Registers to be preserved (K_FP_REGS or K_SSE_REGS).
 *
 * @return N/A
 */
extern void k_float_enable(struct k_thread *thread, unsigned int options);

/**
 * @}
 */

#ifdef CONFIG_X86_ENABLE_TSS
extern struct task_state_segment _main_tss;
#endif

#ifdef CONFIG_USERSPACE
/* We need a set of page tables for each thread in the system which runs in
 * user mode. For each thread, we have:
 *
 *   - a toplevel PDPT
 *   - a set of page directories for the memory range covered by system RAM
 *   - a set of page tbales for the memory range covered by system RAM
 *
 * Directories and tables for memory ranges outside of system RAM will be
 * shared and not thread-specific.
 *
 * NOTE: We are operating under the assumption that memory domain partitions
 * will not be configured which grant permission to address ranges outside
 * of system RAM.
 *
 * Each of these page tables will be programmed to reflect the memory
 * permission policy for that thread, which will be the union of:
 *
 *   - The boot time memory regions (text, rodata, and so forth)
 *   - The thread's stack buffer
 *   - Partitions in the memory domain configuration (if a member of a
 *     memory domain)
 *
 * The PDPT is fairly small singleton on x86 PAE (32 bytes) and also must
 * be aligned to 32 bytes, so we place it at the highest addresses of the
 * page reserved for the privilege elevation stack.
 *
 * The page directories and tables require page alignment so we put them as
 * additional fields in the stack object, using the below macros to compute how
 * many pages we need.
 */

/* Define a range [Z_X86_PT_START, Z_X86_PT_END) which is the memory range
 * covered by all the page tables needed for system RAM
 */
#define Z_X86_PT_START	((u32_t)ROUND_DOWN(DT_PHYS_RAM_ADDR, Z_X86_PT_AREA))
#define Z_X86_PT_END	((u32_t)ROUND_UP(DT_PHYS_RAM_ADDR + \
					 (DT_RAM_SIZE * 1024U), \
					 Z_X86_PT_AREA))

/* Number of page tables needed to cover system RAM. Depends on the specific
 * bounds of system RAM, but roughly 1 page table per 2MB of RAM */
#define Z_X86_NUM_PT	((Z_X86_PT_END - Z_X86_PT_START) / Z_X86_PT_AREA)

/* Same semantics as above, but for the page directories needed to cover
 * system RAM.
 */
#define Z_X86_PD_START	((u32_t)ROUND_DOWN(DT_PHYS_RAM_ADDR, Z_X86_PD_AREA))
#define Z_X86_PD_END	((u32_t)ROUND_UP(DT_PHYS_RAM_ADDR + \
					 (DT_RAM_SIZE * 1024U), \
					 Z_X86_PD_AREA))
/* Number of page directories needed to cover system RAM. Depends on the
 * specific bounds of system RAM, but roughly 1 page directory per 1GB of RAM */
#define Z_X86_NUM_PD	((Z_X86_PD_END - Z_X86_PD_START) / Z_X86_PD_AREA)

/* Number of pages we need to reserve in the stack for per-thread page tables */
#define Z_X86_NUM_TABLE_PAGES	(Z_X86_NUM_PT + Z_X86_NUM_PD)
#else
/* If we're not implementing user mode, then the MMU tables don't get changed
 * on context switch and we don't need any per-thread page tables
 */
#define Z_X86_NUM_TABLE_PAGES	0U
#endif /* CONFIG_USERSPACE */

#define Z_X86_THREAD_PT_AREA	(Z_X86_NUM_TABLE_PAGES * MMU_PAGE_SIZE)

#if defined(CONFIG_HW_STACK_PROTECTION) || defined(CONFIG_USERSPACE)
#define Z_X86_STACK_BASE_ALIGN	MMU_PAGE_SIZE
#else
#define Z_X86_STACK_BASE_ALIGN	STACK_ALIGN
#endif

#ifdef CONFIG_USERSPACE
/* If user mode enabled, expand any stack size to fill a page since that is
 * the access control granularity and we don't want other kernel data to
 * unintentionally fall in the latter part of the page
 */
#define Z_X86_STACK_SIZE_ALIGN	MMU_PAGE_SIZE
#else
#define Z_X86_STACK_SIZE_ALIGN	1
#endif

struct z_x86_kernel_stack_data {
	/* For 32-bit, a single four-entry page directory pointer table, that
	 * needs to be aligned to 32 bytes.
	 */
	struct x86_page_tables ptables;
} __aligned(0x20);

/* With both hardware stack protection and userspace enabled, stacks are
 * arranged as follows:
 *
 * High memory addresses
 * +-----------------------------------------+
 * | Thread stack (varies)                   |
 * +-----------------------------------------+
 * | PDPT (32 bytes)		             |
 * | Privilege elevation stack (4064 bytes)  |
 * +-----------------------------------------+
 * | Guard page (4096 bytes)                 |
 * +-----------------------------------------+
 * | User page tables (Z_X86_THREAD_PT_AREA) |
 * +-----------------------------------------+
 * Low Memory addresses
 *
 * Privilege elevation stacks are fixed-size. All the pages containing the
 * thread stack are marked as user-accessible. The guard page is marked
 * read-only to catch stack overflows in supervisor mode.
 *
 * If a thread starts in supervisor mode, the page containing the PDPT and
 * privilege elevation stack is also marked read-only.
 *
 * If a thread starts in, or drops down to user mode, the privilege stack page
 * will be marked as present, supervior-only. The PDPT will be initialized and
 * used as the active page tables when that thread is active.
 *
 * If KPTI is not enabled, the _main_tss.esp0 field will always be updated
 * updated to point to the top of the privilege elevation stack. Otherwise
 * _main_tss.esp0 always points to the trampoline stack, which handles the
 * page table switch to the kernel PDPT and transplants context to the
 * privileged mode stack.
 */
struct z_x86_thread_stack_header {
#ifdef CONFIG_USERSPACE
	char page_tables[Z_X86_THREAD_PT_AREA];
#endif

#ifdef CONFIG_HW_STACK_PROTECTION
	char guard_page[MMU_PAGE_SIZE];
#endif

#ifdef CONFIG_USERSPACE
	char privilege_stack[MMU_PAGE_SIZE -
		sizeof(struct z_x86_kernel_stack_data)];

	struct z_x86_kernel_stack_data kernel_data;
#endif
} __packed __aligned(Z_X86_STACK_SIZE_ALIGN);

#define Z_ARCH_THREAD_STACK_RESERVED \
	((u32_t)sizeof(struct z_x86_thread_stack_header))

#define Z_ARCH_THREAD_STACK_DEFINE(sym, size) \
	struct _k_thread_stack_element __noinit \
		__aligned(Z_X86_STACK_BASE_ALIGN) \
		sym[ROUND_UP((size), Z_X86_STACK_SIZE_ALIGN) + \
			Z_ARCH_THREAD_STACK_RESERVED]

#define Z_ARCH_THREAD_STACK_LEN(size) \
		(ROUND_UP((size), \
			  MAX(Z_X86_STACK_BASE_ALIGN, \
			      Z_X86_STACK_SIZE_ALIGN)) + \
		Z_ARCH_THREAD_STACK_RESERVED)

#define Z_ARCH_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	struct _k_thread_stack_element __noinit \
		__aligned(Z_X86_STACK_BASE_ALIGN) \
		sym[nmemb][Z_ARCH_THREAD_STACK_LEN(size)]

#define Z_ARCH_THREAD_STACK_MEMBER(sym, size) \
	struct _k_thread_stack_element __aligned(Z_X86_STACK_BASE_ALIGN) \
		sym[ROUND_UP((size), Z_X86_STACK_SIZE_ALIGN) + \
			Z_ARCH_THREAD_STACK_RESERVED]

#define Z_ARCH_THREAD_STACK_SIZEOF(sym) \
	(sizeof(sym) - Z_ARCH_THREAD_STACK_RESERVED)

#define Z_ARCH_THREAD_STACK_BUFFER(sym) \
	((char *)((sym) + Z_ARCH_THREAD_STACK_RESERVED))

#if CONFIG_X86_KERNEL_OOPS
#define Z_ARCH_EXCEPT(reason_p) do { \
	__asm__ volatile( \
		"push %[reason]\n\t" \
		"int %[vector]\n\t" \
		: \
		: [vector] "i" (CONFIG_X86_KERNEL_OOPS_VECTOR), \
		  [reason] "i" (reason_p)); \
	CODE_UNREACHABLE; \
} while (false)
#endif

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_IA32_ARCH_H_ */
