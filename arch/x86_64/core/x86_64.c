/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel_internal.h>
#include <kernel_structs.h>
#include <tracing.h>
#include <ksched.h>
#include <irq_offload.h>
#include "xuk.h"

struct device;

struct NANO_ESF {
};

void _new_thread(struct k_thread *t, k_thread_stack_t *stack,
		 size_t sz, k_thread_entry_t entry,
		 void *p1, void *p2, void *p3,
		 int prio, unsigned int opts)
{
	void *args[] = { entry, p1, p2, p3 };
	int nargs = 4;
	int eflags = 0x200;
	char *base = K_THREAD_STACK_BUFFER(stack);
	char *top = base + sz;

	_new_thread_init(t, base, sz, prio, opts);

	t->switch_handle = (void *)xuk_setup_stack((long) top,
						   (void *)_thread_entry,
						   eflags, (long *)args,
						   nargs);
}

void k_cpu_idle(void)
{
	z_sys_trace_idle();
	__asm__ volatile("sti; hlt");
}

void _unhandled_vector(int vector, int err, struct xuk_entry_frame *f)
{
	/* Yes, there are five regsiters missing.  See notes on
	 * xuk_entry_frame/xuk_stack_frame.
	 */
	printk("*** FATAL ERROR vector %d code %d\n", vector, err);
	printk("***  RIP %d:0x%llx RSP %d:0x%llx RFLAGS 0x%llx\n",
	       (int)f->cs, f->rip, (int)f->ss, f->rsp, f->rflags);
	printk("***  RAX 0x%llx RCX 0x%llx RDX 0x%llx RSI 0x%llx RDI 0x%llx\n",
	       f->rax, f->rcx, f->rdx, f->rsi, f->rdi);
	printk("***  R8 0x%llx R9 0x%llx R10 0x%llx R11 0x%llx\n",
	       f->r8, f->r9, f->r10, f->r11);

	_NanoFatalErrorHandler(x86_64_except_reason, NULL);
}

void _isr_entry(void)
{
	_arch_curr_cpu()->nested++;
}

void *_isr_exit_restore_stack(void *interrupted)
{
	bool nested = (--_arch_curr_cpu()->nested) > 0;
	void *next = _get_next_switch_handle(interrupted);

	return (nested || next == interrupted) ? NULL : next;
}

struct {
	void (*fn)(int, void*);
	void *arg;
	unsigned int esp;
} cpu_init[CONFIG_MP_NUM_CPUS];

/* Called from Zephyr initialization */
void _arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
		     void (*fn)(int, void *), void *arg)
{
	cpu_init[cpu_num].arg = arg;
	cpu_init[cpu_num].esp = (int)(long)(sz + (char *)stack);

	/* This is our flag to the spinning CPU.  Do this last */
	cpu_init[cpu_num].fn = fn;
}

#ifdef CONFIG_IRQ_OFFLOAD
static irq_offload_routine_t offload_fn;
static void *offload_arg;

static void irq_offload_handler(void *arg, int err)
{
	ARG_UNUSED(arg);
	ARG_UNUSED(err);
	offload_fn(offload_arg);
}

void irq_offload(irq_offload_routine_t fn, void *arg)
{
	offload_fn = fn;
	offload_arg = arg;
	__asm__ volatile("int %0" : : "i"(CONFIG_IRQ_OFFLOAD_VECTOR));
}
#endif

/* Default.  Can be overridden at link time by a timer driver */
void __weak x86_apic_timer_isr(void *arg, int code)
{
	ARG_UNUSED(arg);
	ARG_UNUSED(code);
}

/* Called from xuk layer on actual CPU start */
void _cpu_start(int cpu)
{
	xuk_set_f_ptr(cpu, &_kernel.cpus[cpu]);

	/* Set up the timer ISR, but ensure the timer is disabled */
	xuk_set_isr(INT_APIC_LVT_TIMER, 13, x86_apic_timer_isr, 0);
	_apic.INIT_COUNT = 0;

#ifdef CONFIG_IRQ_OFFLOAD
	xuk_set_isr(XUK_INT_RAW_VECTOR(CONFIG_IRQ_OFFLOAD_VECTOR),
		    -1, irq_offload_handler, 0);
#endif

	if (cpu <= 0) {
		/* The SMP CPU startup function pointers act as init
		 * flags.  Zero them here because this code is running
		 * BEFORE .bss is zeroed!  Should probably move that
		 * out of _Cstart() for this architecture...
		 */
		for (int i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
			cpu_init[i].fn = 0;
		}

		/* Enter Zephyr */
		_Cstart();

	} else if (cpu < CONFIG_MP_NUM_CPUS) {
		/* SMP initialization.  First spin, waiting for
		 * _arch_start_cpu() to be called from the main CPU
		 */
		while (!cpu_init[cpu].fn) {
		}

		/* Enter Zephyr, which will switch away and never return */
		cpu_init[cpu].fn(0, cpu_init[cpu].arg);
	}

	/* Spin forever as a fallback */
	while (1) {
	}
}

/* Returns the initial stack to use for CPU startup on auxiliary (not
 * cpu 0) processors to the xuk layer, which gets selected by the
 * non-arch Zephyr kernel and stashed by _arch_start_cpu()
 */
unsigned int _init_cpu_stack(int cpu)
{
	return cpu_init[cpu].esp;
}

int _arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			      void (*routine)(void *parameter), void *parameter,
			      u32_t flags)
{
	ARG_UNUSED(flags);
	__ASSERT(priority >= 2 && priority <= 15,
		 "APIC interrupt priority must be 2-15");

	xuk_set_isr(irq, priority, (void *)routine, parameter);
	return 0;
}

void _arch_irq_disable(unsigned int irq)
{
	xuk_set_isr_mask(irq, 1);
}

void _arch_irq_enable(unsigned int irq)
{
	xuk_set_isr_mask(irq, 0);
}

void x86_apic_set_timeout(u32_t cyc_from_now)
{
	_apic.INIT_COUNT = cyc_from_now;
}

const NANO_ESF _default_esf;

int x86_64_except_reason;

void _NanoFatalErrorHandler(unsigned int reason, const NANO_ESF *esf)
{
	_SysFatalErrorHandler(reason, esf);
}

/* App-overridable handler.  Does nothing here */
void __weak _SysFatalErrorHandler(unsigned int reason, const NANO_ESF *esf)
{
	ARG_UNUSED(reason);
	ARG_UNUSED(esf);
	k_thread_abort(_current);
}
