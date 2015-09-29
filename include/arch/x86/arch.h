/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief IA-32 specific nanokernel interface header
 * This header contains the IA-32 specific nanokernel interface.  It is included
 * by the generic nanokernel interface header (nanokernel.h)
 */

#ifndef _ARCH_IFACE_H
#define _ARCH_IFACE_H

#ifndef _ASMLANGUAGE
#include <arch/x86/asm_inline.h>
#include <arch/x86/addr_types.h>
#endif

/* APIs need to support non-byte addressable architectures */

#define OCTET_TO_SIZEOFUNIT(X) (X)
#define SIZEOFUNIT_TO_OCTET(X) (X)

/**
 * Macro used internally by NANO_CPU_INT_REGISTER and NANO_CPU_INT_REGISTER_ASM.
 * Not meant to be used explicitly by platform, driver or application code.
 */
#define MK_ISR_NAME(x) __isr__##x

#ifdef CONFIG_MICROKERNEL
#define ALL_DYN_IRQ_STUBS (CONFIG_NUM_DYNAMIC_STUBS + CONFIG_MAX_NUM_TASK_IRQS)
#elif defined(CONFIG_NANOKERNEL)
#define ALL_DYN_IRQ_STUBS (CONFIG_NUM_DYNAMIC_STUBS)
#endif

#define ALL_DYN_EXC_STUBS (CONFIG_NUM_DYNAMIC_EXC_STUBS + \
			   CONFIG_NUM_DYNAMIC_EXC_NOERR_STUBS)

#define ALL_DYN_STUBS (ALL_DYN_EXC_STUBS + ALL_DYN_IRQ_STUBS)

/*
 * Synchronize these DYN_STUB_* macros with the generated assembly for
 * _DynIntStubsBegin in intstub.S / _DynExcStubsBegin in excstub.S
 * Assumes all stub types are same size/format
 */

/* Size of each dynamic interrupt/exception stub in bytes */
#define DYN_STUB_SIZE		9

/*
 * Offset from the beginning of a stub to the byte containing the argument
 * to the push instruction, which is the stub index
 */
#define DYN_STUB_IDX_OFFSET	6

/* Size of the periodic jmp instruction to the common handler */
#define DYN_STUB_JMP_SIZE	5

/*
 * How many consecutive stubs we have until we encounter a periodic
 * jump to _DynStubCommon
 */
#define DYN_STUB_PER_BLOCK	8

#ifndef _ASMLANGUAGE

/* interrupt/exception/error related definitions */

/**
 * Floating point register set alignment.
 *
 * If support for SSEx extensions is enabled a 16 byte boundary is required,
 * since the 'fxsave' and 'fxrstor' instructions require this. In all other
 * cases a 4 byte boundary is sufficient.
 */

#ifdef CONFIG_SSE
#define FP_REG_SET_ALIGN  16
#else
#define FP_REG_SET_ALIGN  4
#endif

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
	/** IRQ associated with the ISR/stub */
	unsigned int    irq;
	/** Priority associated with the IRQ */
	unsigned int    priority;
	/** Vector number associated with ISR/stub */
	unsigned int    vec;
	/** Privilege level associated with ISR/stub */
	unsigned int    dpl;
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
 * properly encoded. This macro replaces the _IntVecSet () routine in static
 * interrupt systems.
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
	 ISR_LIST __attribute__((section(".intList"))) MK_ISR_NAME(r) = \
			{&r, n, p, v, d}

/**
 * @brief Connect a routine to interrupt number
 *
 * For the device @a device associates IRQ number @a irq with priority
 * @a priority with the interrupt routine @a isr, that receives parameter
 * @a parameter.
 *
 * @param device Device
 * @param irq IRQ number
 * @param priority IRQ Priority (currently ignored)
 * @param isr Interrupt Service Routine
 * @param parameter ISR parameter
 *
 * @return N/A
 *
 */
#define IRQ_CONNECT_STATIC(device, irq, priority, isr, parameter)	   \
	extern void *_##device##_##isr##_stub;				               \
	NANO_CPU_INT_REGISTER(_##device##_##isr##_stub, (irq), (priority), -1, 0)


extern unsigned char _irq_to_interrupt_vector[];
/**
 * @brief Convert a statically connected IRQ to its interrupt vector number
 *
 * @param irq IRQ number
 */
#define _IRQ_TO_INTERRUPT_VECTOR(irq)                       \
			((unsigned int) _irq_to_interrupt_vector[irq])

/**
 *
 * @brief Configure interrupt for the device
 *
 * For the given device do the necessary configuration steps.
 * For x86 platform configure APIC and mark interrupt vector allocated
 * @param device Device - not used by macro
 * @param irq IRQ
 * @param priority IRQ priority, unused on this platform
 *
 * @return N/A
 *
 */
#define IRQ_CONFIG(device, irq, priority)			\
	do {							\
		_SysIntVecProgram(_IRQ_TO_INTERRUPT_VECTOR(irq), irq); \
	} while (0)


/**
 * @brief Nanokernel Exception Stack Frame
 *
 * A pointer to an "exception stack frame" (ESF) is passed as an argument
 * to exception handlers registered via nanoCpuExcConnect().  When an exception
 * occurs while PL=0, then only the EIP, CS, and EFLAGS are pushed onto the stack.
 * The least significant pair of bits in the CS value should be examined to
 * determine whether the exception occurred while PL=3, in which case the ESP
 * and SS values will also be present in the ESF.  If the exception occurred
 * while in PL=0, neither the SS nor ESP values will be present in the ESF.
 *
 * The exception stack frame includes the volatile registers EAX, ECX, and EDX
 * pushed on the stack by _ExcEnt().
 *
 * It also contains the value of CR2, used when the exception is a page fault.
 * Since that register is shared amongst threads of execution, it might get
 * overwritten if another thread is context-switched in and then itself
 * page-faults before the first thread has time to read CR2.
 *
 * If configured for host-based debug tools such as GDB, the 4 non-volatile
 * registers (EDI, ESI, EBX, EBP) are also pushed by _ExcEnt()
 * for use by the debug tools.
 */

typedef struct nanoEsf {
	/** putting cr2 here allows discarding it and pEsf in one instruction */
	unsigned int cr2;
#ifdef CONFIG_GDB_INFO
	unsigned int ebp;
	unsigned int ebx;
	unsigned int esi;
	unsigned int edi;
#endif /* CONFIG_GDB_INFO */
	unsigned int edx;
	unsigned int ecx;
	unsigned int eax;
	unsigned int errorCode;
	unsigned int eip;
	unsigned int cs;
	unsigned int eflags;
	unsigned int esp;
	unsigned int ss;
} NANO_ESF;

/**
 * @brief Nanokernel "interrupt stack frame" (ISF)
 *
 * An "interrupt stack frame" (ISF) as constructed by the processor
 * and the interrupt wrapper function _IntExit().  When an interrupt
 * occurs while PL=0, only the EIP, CS, and EFLAGS are pushed onto the stack.
 * The least significant pair of bits in the CS value should be examined to
 * determine whether the exception occurred while PL=3, in which case the ESP
 * and SS values will also be present in the ESF.  If the exception occurred
 * while in PL=0, neither the SS nor ESP values will be present in the ISF.
 *
 * The interrupt stack frame includes the volatile registers EAX, ECX, and EDX
 * pushed on the stack by _IntExit()..
 *
 * The host-based debug tools such as GDB do not require the 4 non-volatile
 * registers (EDI, ESI, EBX, EBP) to be preserved during an interrupt.
 * The register values saved/restored by _Swap() called from _IntExit() are
 * sufficient.
 */

typedef struct nanoIsf {
	unsigned int edx;
	unsigned int ecx;
	unsigned int eax;
	unsigned int eip;
	unsigned int cs;
	unsigned int eflags;
	unsigned int esp;
	unsigned int ss;
} NANO_ISF;

#endif /* !_ASMLANGUAGE */

/*
 * Reason codes passed to both _NanoFatalErrorHandler()
 * and _SysFatalErrorHandler().
 */

/** Unhandled exception/interrupt */
#define _NANO_ERR_SPURIOUS_INT		 (0)
/** Page fault */
#define _NANO_ERR_PAGE_FAULT		 (1)
/** General protection fault */
#define _NANO_ERR_GEN_PROT_FAULT	 (2)
/** Invalid task exit */
#define _NANO_ERR_INVALID_TASK_EXIT  (3)
/** Stack corruption detected */
#define _NANO_ERR_STACK_CHK_FAIL	 (4)
/** Kernel Allocation Failure */
#define _NANO_ERR_ALLOCATION_FAIL    (5)

#ifndef _ASMLANGUAGE


#ifdef CONFIG_INT_LATENCY_BENCHMARK
void _int_latency_start(void);
void _int_latency_stop(void);
#endif

/**
 * @brief Disable all interrupts on the CPU (inline)
 *
 * This routine disables interrupts.  It can be called from either interrupt,
 * task or fiber level.  This routine returns an architecture-dependent
 * lock-out key representing the "interrupt disable state" prior to the call;
 * this key can be passed to irq_unlock() to re-enable interrupts.
 *
 * The lock-out key should only be used as the argument to the irq_unlock()
 * API.  It should never be used to manually re-enable interrupts or to inspect
 * or manipulate the contents of the source register.
 *
 * This function can be called recursively: it will return a key to return the
 * state of interrupt locking to the previous level.
 *
 * WARNINGS
 * Invoking a kernel routine with interrupts locked may result in
 * interrupts being re-enabled for an unspecified period of time.  If the
 * called routine blocks, interrupts will be re-enabled while another
 * thread executes, or while the system is idle.
 *
 * The "interrupt disable state" is an attribute of a thread.  Thus, if a
 * fiber or task disables interrupts and subsequently invokes a kernel
 * routine that causes the calling thread to block, the interrupt
 * disable state will be restored when the thread is later rescheduled
 * for execution.
 *
 * @return An architecture-dependent lock-out key representing the
 * "interrupt disable state" prior to the call.
 *
 */

static inline __attribute__((always_inline)) unsigned int irq_lock(void)
{
	unsigned int key = _do_irq_lock();

#ifdef CONFIG_INT_LATENCY_BENCHMARK
	_int_latency_start();
#endif

	return key;
}


/**
 *
 * @brief Enable all interrupts on the CPU (inline)
 *
 * This routine re-enables interrupts on the CPU.  The @a key parameter
 * is an architecture-dependent lock-out key that is returned by a previous
 * invocation of irq_lock().
 *
 * This routine can be called from either interrupt, task or fiber level.
 *
 * @return N/A
 *
 */

static inline __attribute__((always_inline)) void irq_unlock(unsigned int key)
{
	if (!(key & 0x200)) {
		return;
	}
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	_int_latency_stop();
#endif
	_do_irq_unlock();
}

/** interrupt/exception/error related definitions */
typedef void (*NANO_EOI_GET_FUNC) (void *);

/**
 * The NANO_SOFT_IRQ macro must be used as the value for the @a irq parameter
 * to irq_connect() when connecting to a software generated interrupt.
 */
#define NANO_SOFT_IRQ	((unsigned int) (-1))

#ifdef CONFIG_FP_SHARING
/* Definitions for the 'options' parameter to the fiber_fiber_start() API */

/** thread uses floating point unit */
#define USE_FP		0x10
#ifdef CONFIG_SSE
/** thread uses SSEx instructions */
#define USE_SSE		0x20
#endif /* CONFIG_SSE */
#endif /* CONFIG_FP_SHARING */

extern int	irq_connect(unsigned int irq,
					 unsigned int priority,
					 void (*routine)(void *parameter),
					 void *parameter);

/**
 * @brief Enable a specific IRQ
 * @param irq IRQ
 */
extern void	irq_enable(unsigned int irq);
/**
 * @brief Disable a specific IRQ
 * @param irq IRQ
 */
extern void	irq_disable(unsigned int irq);

#ifdef CONFIG_FP_SHARING
/**
 * @brief Enable floating point hardware resources sharing
 * Dynamically enable/disable the capability of a thread to share floating
 * point hardware resources.  The same "floating point" options accepted by
 * fiber_fiber_start() are accepted by these APIs (i.e. USE_FP and USE_SSE).
 */
extern void	fiber_float_enable(nano_thread_id_t thread_id,
								unsigned int options);
extern void	task_float_enable(nano_thread_id_t thread_id,
								unsigned int options);
extern void	fiber_float_disable(nano_thread_id_t thread_id);
extern void	task_float_disable(nano_thread_id_t thread_id);
#endif /* CONFIG_FP_SHARING */

#include <stddef.h>	/* for size_t */

extern void	nano_cpu_idle(void);

/** Nanokernel provided routine to report any detected fatal error. */
extern FUNC_NORETURN void _NanoFatalErrorHandler(unsigned int reason,
						 const NANO_ESF * pEsf);
/** User provided routine to handle any detected fatal error post reporting. */
extern FUNC_NORETURN void _SysFatalErrorHandler(unsigned int reason,
						const NANO_ESF * pEsf);
/** Dummy ESF for fatal errors that would otherwise not have an ESF */
extern const NANO_ESF _default_esf;

/**
 * @brief Configure an interrupt vector of the specified priority
 *
 * This routine is invoked by the kernel to configure an interrupt vector of
 * the specified priority.  To this end, it allocates an interrupt vector,
 * programs hardware to route interrupt requests on the specified IRQ to that
 * vector, and returns the vector number
 */
extern int _SysIntVecAlloc(unsigned int irq,
			   unsigned int priority);

/* functions provided by the kernel for usage by _SysIntVecAlloc() */

extern int	_IntVecAlloc(unsigned int priority);

extern void	_IntVecMarkAllocated(unsigned int vector);

extern void	_IntVecMarkFree(unsigned int vector);

#endif /* !_ASMLANGUAGE */

/* Segment selector definitions are shared */
#include "segselect.h"

#endif /* _ARCH_IFACE_H */
