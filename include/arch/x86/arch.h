/* arch.h - IA-32 specific nanokernel interface header */

/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
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
This header contains the IA-32 specific nanokernel interface.  It is included
by the generic nanokernel interface header (nanokernel.h)
*/

#ifndef _ARCH_IFACE_H
#define _ARCH_IFACE_H

/* WARNING: must include nanokernel.h before this file */

#ifndef _ASMLANGUAGE
#include <arch/x86/Intelprc.h>
#endif

/*
 * Macro used internally by NANO_CPU_INT_REGISTER and NANO_CPU_INT_REGISTER_ASM.
 * Not meant to be used explicitly by BSP, driver or application code.
 */
#define MK_ISR_NAME(x) __isr__##x

#ifndef _ASMLANGUAGE

/* interrupt/exception/error related definitions */

#define _INT_STUB_SIZE		0x2b
/*
 * Performance optimization
 *
 * Macro PERF_OPT is defined if project is compiled with option other than
 * size optimization ("-Os" for GCC, "-XO -Xsize-opt" for Diab). If the
 * last of the compiler options is the size optimization, PERF_OPT is not
 * defined and the project is optimized for size, hence the stub should be
 * aligned to 1 and not 16.
 */
#ifdef PERF_OPT
#define _INT_STUB_ALIGN	16
#else
#define _INT_STUB_ALIGN	1
#endif

typedef unsigned char __attribute__((aligned(_INT_STUB_ALIGN)))
						NANO_INT_STUB[_INT_STUB_SIZE];


typedef struct s_isrList {
	void		*fnc;    /* Address of ISR/stub */
	unsigned int    vec;    /* Vector number associated with ISR/stub */
	unsigned int    dpl;    /* Privilege level associated with ISR/stub */
} ISR_LIST;

/*******************************************************************************
*
* NANO_CPU_INT_REGISTER - connect a routine to an interrupt vector
*
* This macro "connects" the specified routine, <r>, to the specified interrupt
* vector, <v> using the descriptor privilege level <d>. On the IA-32
* architecture, an interrupt vector is a value from 0 to 255. This macro
* populates the special intList section with the address of the routine, the
* vector number and the descriptor privilege level. The genIdt tool then picks
* up this information and generates an actual IDT entry with this information
* properly encoded. This macro replaces the _IntVecSet () routine in static
* interrupt systems.
*
* The <d> argument specifies the privilege level for the interrupt-gate
* descriptor; (hardware) interrupts and exceptions should specify a level of 0,
* whereas handlers for user-mode software generated interrupts should specify 3.
*
* RETURNS: N/A
*
*/

#define NANO_CPU_INT_REGISTER(r, v, d) \
	 ISR_LIST __attribute__((section(".intList"))) MK_ISR_NAME(r) = {&r, v, d}

/*
 * Macro to declare a dynamic interrupt stub. Using the macro places the stub
 * in the .intStubSection which is located in the image according to the kernel
 * configuration.
 */
  #define NANO_CPU_INT_STUB_DECL(s) \
	_NODATA_SECTION(.intStubSect) NANO_INT_STUB(s)

/*
 * A pointer to an "exception stack frame" (ESF) is passed as an argument
 * to exception handlers registered via nanoCpuExcConnect().  When an exception
 * occurs while PL=0, then only the EIP, CS, and EFLAGS are pushed onto the stack.
 * The least significant pair of bits in the CS value should be examined to
 * determine whether the exception occured while PL=3, in which case the ESP and
 * SS values will also be present in the ESF.  If the exception occurred while
 * in PL=0, neither the SS nor ESP values will be present in the ESF.
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
	unsigned int cr2; /* putting cr2 here allows discarding it and pEsf in
			   * one instruction */
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

/*
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

#define _NANO_ERR_SPURIOUS_INT		 (0)	/* Unhandled exception/interrupt */
#define _NANO_ERR_PAGE_FAULT		 (1)	/* Page fault */
#define _NANO_ERR_GEN_PROT_FAULT	 (2)	/* General protection fault */
#define _NANO_ERR_INVALID_TASK_EXIT  (3)	/* Invalid task exit */
#define _NANO_ERR_STACK_CHK_FAIL	 (4)	/* Stack corruption detected */
#define _NANO_ERR_ALLOCATION_FAIL    (5)    /* Kernel Allocation Failure */

#ifndef _ASMLANGUAGE


#ifdef CONFIG_NO_ISRS

static inline unsigned int irq_lock(void) {return 1;}
static inline void irq_unlock(unsigned int key) {}
#define irq_lock_inline irq_lock
#define irq_unlock_inline irq_unlock

#else /* CONFIG_NO_ISRS */

#ifdef CONFIG_INT_LATENCY_BENCHMARK
void _int_latency_start(void);
void _int_latency_stop(void);
#endif

/*******************************************************************************
*
* irq_lock_inline - disable all interrupts on the CPU (inline)
*
* This routine disables interrupts.  It can be called from either interrupt,
* task or fiber level.  This routine returns an architecture-dependent
* lock-out key representing the "interrupt disable state" prior to the call;
* this key can be passed to irq_unlock_inline() to re-enable interrupts.
*
* The lock-out key should only be used as the argument to the
* irq_unlock_inline() API.  It should never be used to manually re-enable
* interrupts or to inspect or manipulate the contents of the source register.
*
* WARNINGS
* Invoking a VxMicro routine with interrupts locked may result in
* interrupts being re-enabled for an unspecified period of time.  If the
* called routine blocks, interrupts will be re-enabled while another
* context executes, or while the system is idle.
*
* The "interrupt disable state" is an attribute of a context.  Thus, if a
* fiber or task disables interrupts and subsequently invokes a VxMicro
* system routine that causes the calling context to block, the interrupt
* disable state will be restored when the context is later rescheduled
* for execution.
*
* RETURNS: An architecture-dependent lock-out key representing the
* "interrupt disable state" prior to the call.
*
* \NOMANUAL
*/

static inline __attribute__((always_inline))
	unsigned int irq_lock_inline(void)
{
	unsigned int key = _do_irq_lock_inline();

#ifdef CONFIG_INT_LATENCY_BENCHMARK
	_int_latency_start();
#endif

	return key;
}


/*******************************************************************************
*
* irq_unlock_inline - enable all interrupts on the CPU (inline)
*
* This routine re-enables interrupts on the CPU.  The <key> parameter
* is an architecture-dependent lock-out key that is returned by a previous
* invocation of irq_lock_inline().
*
* This routine can be called from either interrupt, task or fiber level.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static inline __attribute__((always_inline))
	void irq_unlock_inline(unsigned int key)
{
	if (!(key & 0x200)) {
		return;
	}
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	_int_latency_stop();
#endif
	_do_irq_unlock_inline();
	return;
}
#endif /* CONFIG_NO_ISRS */

/* interrupt/exception/error related definitions */

typedef void (*NANO_EOI_GET_FUNC) (void *);

/*
 * The NANO_SOFT_IRQ macro must be used as the value for the <irq> parameter
 * to irq_connect() when connecting to a software generated interrupt.
 */

#define NANO_SOFT_IRQ	((unsigned int) (-1))

#ifdef CONFIG_FP_SHARING
/* Definitions for the 'options' parameter to the nanoFiberStart() API */

#define USE_FP		0x10	/* context uses floating point unit */
#ifdef CONFIG_SSE
#define USE_SSE		0x20	/* context uses SSEx instructions */
#endif /* CONFIG_SSE */
#endif /* CONFIG_FP_SHARING */

extern void	_nano_fiber_swap(void);

extern unsigned int find_first_set(unsigned int op);

extern unsigned int find_last_set(unsigned int op);

extern void	irq_handler_set(unsigned int vector,
					 void (*oldRoutine)(void *parameter),
					 void (*newRoutine)(void *parameter),
					 void *parameter);

extern int	irq_connect(unsigned int irq,
					 unsigned int priority,
					 void (*routine)(void *parameter),
					 void *parameter,
					 NANO_INT_STUB pIntStubMem);

/*
 * irq_enable()  : enable a specific IRQ
 * irq_disable() : disable a specific IRQ
 * irq_lock()	: lock out all interrupts
 * irq_unlock()  : unlock all interrupts
 */

extern void	irq_enable(unsigned int irq);
extern void	irq_disable(unsigned int irq);

#ifndef CONFIG_NO_ISRS
extern int	irq_lock(void);
extern void	irq_unlock(int key);
#endif /* CONFIG_NO_ISRS */

#ifdef CONFIG_FP_SHARING
/*
 * Dynamically enable/disable the capability of a context to share floating
 * point hardware resources.  The same "floating point" options accepted by
 * nanoFiberStart() are accepted by these APIs (i.e. USE_FP and USE_SSE).
 */

extern void	fiber_float_enable(nano_context_id_t ctx, unsigned int options);
extern void	task_float_enable(nano_context_id_t ctx, unsigned int options);
extern void	fiber_float_disable(nano_context_id_t ctx);
extern void	task_float_disable(nano_context_id_t ctx);
#endif /* CONFIG_FP_SHARING */

#include <stddef.h>	/* for size_t */

#ifdef CONFIG_NANOKERNEL
extern void	nano_cpu_idle(void);
#endif

/* Nanokernel provided routine to report any detected fatal error. */
extern FUNC_NORETURN void _NanoFatalErrorHandler(unsigned int reason,
						 const NANO_ESF *pEsf);
/* User provided routine to handle any detected fatal error post reporting. */
extern FUNC_NORETURN void _SysFatalErrorHandler(unsigned int reason,
						const NANO_ESF *pEsf);
/* Dummy ESF for fatal errors that would otherwise not have an ESF */
extern const NANO_ESF _default_esf;

/*
 * BSP provided routine which kernel invokes to configure an interrupt vector
 * of the specified priority; the BSP allocates an interrupt vector, programs
 * hardware to route interrupt requests on the specified irq to that vector,
 * and returns the vector number along with its associated BOI/EOI information
 */

extern int	_SysIntVecAlloc(unsigned int irq,
					 unsigned int priority,
					 NANO_EOI_GET_FUNC *boiRtn,
					 NANO_EOI_GET_FUNC *eoiRtn,
					 void **boiRtnParm,
					 void **eoiRtnParm,
					 unsigned char *boiParamRequired,
					 unsigned char *eoiParamRequired
					 );

/* functions provided by the kernel for usage by the BSP's _SysIntVecAlloc() */

extern int	_IntVecAlloc(unsigned int priority);

extern void	_IntVecMarkAllocated(unsigned int vector);

extern void	_IntVecMarkFree(unsigned int vector);

#endif /* !_ASMLANGUAGE */

/* Segment selector defintions are shared */
#include "segselect.h"

#endif /* _ARCH_IFACE_H */
