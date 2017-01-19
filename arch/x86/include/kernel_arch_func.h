/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* this file is only meant to be included by kernel_structs.h */

#ifndef _kernel_arch_func__h_
#define _kernel_arch_func__h_

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

/* stack alignment related macros: STACK_ALIGN_SIZE is defined above */

#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN_SIZE)

/**
 *
 * @brief Performs architecture-specific initialization
 *
 * This routine performs architecture-specific initialization of the kernel.
 * Trivial stuff is done inline; more complex initialization is done via
 * function calls.
 *
 * @return N/A
 */
static inline void nanoArchInit(void)
{
	extern void *__isr___SpuriousIntHandler;
	extern void *_dummy_spurious_interrupt;
	extern void *_dummy_exception_vector_stub;
	extern char _interrupt_stack[CONFIG_ISR_STACK_SIZE];

	extern void _exception_enter(void);

	_kernel.nested = 0;

	_kernel.irq_stack = _interrupt_stack + CONFIG_ISR_STACK_SIZE;
	/*
	 * Forces the inclusion of the spurious interrupt handlers. If a
	 * reference isn't made then intconnect.o is never pulled in by the
	 * linker.
	 */

	_dummy_spurious_interrupt = &__isr___SpuriousIntHandler;

	/*
	 * Forces the inclusion of the exception vector stub code. If a
	 * reference isn't made then excstubs.o is never pulled in by the
	 * linker.
	 */

	_dummy_exception_vector_stub = &_exception_enter;


}

/**
 *
 * @brief Set the return value for the specified fiber (inline)
 *
 * @param fiber pointer to fiber
 * @param value value to set as return value
 *
 * The register used to store the return value from a function call invocation
 * is set to <value>.  It is assumed that the specified <fiber> is pending, and
 * thus the fibers context is stored in its TCS.
 *
 * @return N/A
 */
static ALWAYS_INLINE void
_set_thread_return_value(struct k_thread *thread, unsigned int value)
{
	/* write into 'eax' slot created in _Swap() entry */

	*(unsigned int *)(thread->callee_saved.esp) = value;
}

extern void k_cpu_atomic_idle(unsigned int imask);

extern void _MsrWrite(unsigned int msr, uint64_t msrData);
extern uint64_t _MsrRead(unsigned int msr);

/*
 * _IntLibInit() is called from the non-arch specific function,
 * prepare_multithreading(). The IA-32 kernel does not require any special
 * initialization of the interrupt subsystem. However, we still need to
 * provide an _IntLibInit() of some sort to prevent build errors.
 */
static inline void _IntLibInit(void)
{
}

/* the _idt_base_address symbol is generated via a linker script */
extern unsigned char _idt_base_address[];

#include <stddef.h> /* For size_t */

#ifdef __cplusplus
}
#endif

#define _is_in_isr() (_kernel.nested != 0)

#endif /* _ASMLANGUAGE */

#endif /* _kernel_arch_func__h_ */
