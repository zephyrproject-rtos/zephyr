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
#include <stdbool.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/sys/util.h>
#include <zephyr/arch/x86/ia32/gdbstub.h>
#include <zephyr/arch/x86/ia32/thread.h>
#include <zephyr/arch/x86/ia32/syscall.h>

#ifndef _ASMLANGUAGE
#include <stddef.h>	/* for size_t */

#include <zephyr/arch/common/addr_types.h>
#include <zephyr/arch/x86/ia32/segmentation.h>
#include <zephyr/pm/pm.h>

#endif /* _ASMLANGUAGE */

/* GDT layout */
#define CODE_SEG	0x08
#define DATA_SEG	0x10
#define MAIN_TSS	0x18
#define DF_TSS		0x20

/*
 * Use for thread local storage.
 * Match these to gen_gdt.py.
 * The 0x03 is added to limit privilege.
 */
#if defined(CONFIG_USERSPACE)
#define GS_TLS_SEG	(0x38 | 0x03)
#elif defined(CONFIG_HW_STACK_PROTECTION)
#define GS_TLS_SEG	(0x28 | 0x03)
#else
#define GS_TLS_SEG	(0x18 | 0x03)
#endif

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
			__attribute__((used)) MK_ISR_NAME(vec_p) = \
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

#ifdef CONFIG_LINKER_USE_PINNED_SECTION
#define IRQSTUBS_TEXT_SECTION	".pinned_text.irqstubs"
#else
#define IRQSTUBS_TEXT_SECTION	".text.irqstubs"
#endif

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
#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
{ \
	__asm__ __volatile__(							\
		".pushsection .intList\n\t" \
		".long %c[isr]_irq%c[irq]_stub\n\t"	/* ISR_LIST.fnc */ \
		".long %c[irq]\n\t"		/* ISR_LIST.irq */ \
		".long %c[priority]\n\t"	/* ISR_LIST.priority */ \
		".long %c[vector]\n\t"		/* ISR_LIST.vec */ \
		".long 0\n\t"			/* ISR_LIST.dpl */ \
		".long 0\n\t"			/* ISR_LIST.tss */ \
		".popsection\n\t" \
		".pushsection " IRQSTUBS_TEXT_SECTION "\n\t" \
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
}

#ifdef CONFIG_PCIE

#define ARCH_PCIE_IRQ_CONNECT(bdf_p, irq_p, priority_p,			\
			      isr_p, isr_param_p, flags_p)		\
	ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)

#endif /* CONFIG_PCIE */

/* Direct interrupts won't work as expected with KPTI turned on, because
 * all non-user accessible pages in the page table are marked non-present.
 * It's likely possible to add logic to ARCH_ISR_DIRECT_HEADER/FOOTER to do
 * the necessary trampolining to switch page tables / stacks, but this
 * probably loses all the latency benefits that direct interrupts provide
 * and one might as well use a regular interrupt anyway.
 */
#ifndef CONFIG_X86_KPTI
#define ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p) \
{ \
	NANO_CPU_INT_REGISTER(isr_p, irq_p, priority_p, -1, 0); \
	z_irq_controller_irq_config(Z_IRQ_TO_INTERRUPT_VECTOR(irq_p), (irq_p), \
				   (flags_p)); \
}

#ifdef CONFIG_PM
static inline void arch_irq_direct_pm(void)
{
	if (_kernel.idle) {
		_kernel.idle = 0;
		z_pm_save_idle_exit();
	}
}

#define ARCH_ISR_DIRECT_PM() arch_irq_direct_pm()
#else
#define ARCH_ISR_DIRECT_PM() do { } while (false)
#endif

#define ARCH_ISR_DIRECT_HEADER() arch_isr_direct_header()
#define ARCH_ISR_DIRECT_FOOTER(swap) arch_isr_direct_footer(swap)

/* FIXME:
 * tracing/tracing.h cannot be included here due to circular dependency
 */
#if defined(CONFIG_TRACING)
extern void sys_trace_isr_enter(void);
extern void sys_trace_isr_exit(void);
#endif

static inline void arch_isr_direct_header(void)
{
#if defined(CONFIG_TRACING)
	sys_trace_isr_enter();
#endif

	/* We're not going to unlock IRQs, but we still need to increment this
	 * so that arch_is_in_isr() works
	 */
	++_kernel.cpus[0].nested;
}

/*
 * FIXME: z_swap_irqlock is an inline function declared in a private header and
 *	  cannot be referenced from a public header, so we move it to an
 *	  external function.
 */
extern void arch_isr_direct_footer_swap(unsigned int key);

static inline void arch_isr_direct_footer(int swap)
{
	z_irq_controller_eoi();
#if defined(CONFIG_TRACING)
	sys_trace_isr_exit();
#endif
	--_kernel.cpus[0].nested;

	/* Call swap if all the following is true:
	 *
	 * 1) swap argument was enabled to this function
	 * 2) We are not in a nested interrupt
	 * 3) Next thread to run in the ready queue is not this thread
	 */
	if (swap != 0 && _kernel.cpus[0].nested == 0 &&
	    _kernel.ready_q.cache != _current) {
		unsigned int flags;

		/* Fetch EFLAGS argument to z_swap() */
		__asm__ volatile (
			"pushfl\n\t"
			"popl %0\n\t"
			: "=g" (flags)
			:
			: "memory"
			);

		arch_isr_direct_footer_swap(flags);
	}
}

#define ARCH_ISR_DIRECT_DECLARE(name) \
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
#endif /* !CONFIG_X86_KPTI */

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
#ifdef CONFIG_GDBSTUB
	unsigned int ss;
	unsigned int gs;
	unsigned int fs;
	unsigned int es;
	unsigned int ds;
#endif
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

extern unsigned int z_x86_exception_vector;

struct _x86_syscall_stack_frame {
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;

	/* These are only present if cs = USER_CODE_SEG */
	uint32_t esp;
	uint32_t ss;
};

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
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

#ifdef CONFIG_X86_ENABLE_TSS
extern struct task_state_segment _main_tss;
#endif

#define ARCH_EXCEPT(reason_p) do { \
	__asm__ volatile( \
		"push %[reason]\n\t" \
		"int %[vector]\n\t" \
		: \
		: [vector] "i" (Z_X86_OOPS_VECTOR), \
		  [reason] "i" (reason_p)); \
	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */ \
} while (false)

/*
 * Dynamic thread object memory alignment.
 *
 * If support for SSEx extensions is enabled a 16 byte boundary is required,
 * since the 'fxsave' and 'fxrstor' instructions require this. In all other
 * cases a 4 byte boundary is sufficient.
 */
#if defined(CONFIG_EAGER_FPU_SHARING) || defined(CONFIG_LAZY_FPU_SHARING)
#ifdef CONFIG_SSE
#define ARCH_DYNAMIC_OBJ_K_THREAD_ALIGNMENT	16
#else
#define ARCH_DYNAMIC_OBJ_K_THREAD_ALIGNMENT	(sizeof(void *))
#endif
#else
/* No special alignment requirements, simply align on pointer size. */
#define ARCH_DYNAMIC_OBJ_K_THREAD_ALIGNMENT	(sizeof(void *))
#endif /* CONFIG_*_FP_SHARING */


#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_IA32_ARCH_H_ */
