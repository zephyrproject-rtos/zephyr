#ifndef _CORE64_H
#define _CORE64_H

#include "shared-page.h"

/*
 * APIs exposed by the core64 layer to OS integration:
 */

static inline void core64_switch(void *switch_to, void **switched_from)
{
	__asm__ volatile("pushfq;"
			 "mov %%cs, %%ecx;"
			 "pushq %%rcx;"
			 "call _switch_top"
			 : : "a"(switch_to), "d"(switched_from) : "ecx");
}

/* Set a single CPU-specific pointer which can be retrieved (on that
 * CPU!) with get_f_ptr()
 */
static inline void set_f_ptr(int cpu, void *p)
{
	_shared.fs_ptrs[cpu] = (long)p;
}

/* Likewise, but "G" */
static inline void set_g_ptr(int cpu, void *p)
{
	_shared.gs_ptrs[cpu] = (long)p;
}

/* Retrieves the pointer set by set_f_ptr() for the current CPU */
static inline void *get_f_ptr()
{
	long long ret, off = 0;

	__asm__("movq %%fs:(%1), %0" : "=r"(ret) : "r"(off));
	return (void *)(long)ret;
}

/* Retrieves the pointer set by set_g_ptr() for the current CPU */
static inline void *get_g_ptr()
{
	long long ret, off = 0;

	__asm__("movq %%gs:(%1), %0" : "=r"(ret) : "r"(off));
	return (void *)(long)ret;
}

/**
 * @brief Sets a global handler for the specified interrupt.
 *
 * Interrupt numbers live in a partitioned space:
 *
 * + Values from 0 - 0xff are mapped to INTIx interrupts in the global
 *   index of IO-APIC inputs, which on many systems correspond to
 *   legacy IRQ0-IRQ15 interrupts at the bottom of the interrupt
 *   range.  These handlers are not passed a meaningful value in their
 *   first argument, though the function pointer type declares one.
 *
 * + Values from 0x100 to 0x1ff are mapped to raw vectors 0x00-0xff
 *   and can be used for handling exceptions, for INT instructions, or
 *   for MSI- or IPI-directed interrupts that specifiy specific
 *   vectors.
 *
 * + Values outside this range may be exposed symbolically for other
 *   interrupts sources, for example local APIC LVT interrupts.
 *
 * If there is a pre-existing handler specified for a specified raw
 * vector, this function will replace it.
 *
 * @param interrupt Interrupt number.  See above for interpretation.
 * @param priority Integer in the range 2-15. Higher-valued interrupts
 *                 can interrupt lower ones.  Ignored for raw vector
 *                 numbers, as their priority is encoded in the top
 *                 four bits of the vector number.  A priority of zero
 *                 is treated as "don't care" and the interrupt will
 *                 be assigned the lowest available vector.
 * @param handler Function pointer to invoke on interrupt receipt.  It
 *                 will be passed the specified argument as the first
 *                 argument and the x86 exception error code (if any)
 *                 in the second.
 * @param arg Opaque value to pass to the handler when invoked.
 *
 */
void set_isr(int interrupt, int priority,
	     void (*handler)(void *, int), void *arg);

#define INT_APIC_LVT_TIMER 0x200

/* Sets up a new stack. The sp argument should point to the quadword
 * above (!) the allocated stack area (i.e. the frame will be pushed
 * below it).  The frame will be set up to enter the function in the
 * specified code segment with the specified flags register.  An array
 * of up to 6 function arguments may also be provided.  Returns a
 * handle suitable for passing to switch() or for returning from
 * isr_exit_restore_stack().
 */
long setup_stack(long sp, void *fn, unsigned int eflags,
		 long *args, int nargs);

/*
 * OS-defined utilities required by the core64 layer:
 */

/* Returns the address of a stack pointer in 32 bit memory to be used
 * by AP processor bootstraping and startup.
 */
unsigned int init_cpu_stack(int cpu);

/* OS CPU startup entry point, running on the stack returned by
 * init_cpu_stack()
 */
void cpu_start(int cpu);

/* Called on receipt of an unregistered interrupt/exception.  Passes
 * the vector number and the CPU error code, if any.
 */
void unhandled_vector(int vector, int err);

/* Called on ISR exit to choose a next thread to run.  The argument is
 * a context pointer to the thread that was interrupted.
 */
void *isr_exit_restore_stack(void *interrupted);

#endif /* _CORE64_H */
