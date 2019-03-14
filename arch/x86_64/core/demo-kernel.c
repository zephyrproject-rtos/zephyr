/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "serial.h"
#include "vgacon.h"
#include "printf.h"
#include "xuk.h"

/* Tiny demonstration of the core64 code.  Implements enough of an
 * "OS" layer to do some simple unit testing.
 */

static void putchar(int c)
{
	serial_putc(c);
	vgacon_putc(c);
}

void test_timers(void)
{
	/* Quickly calibrate the timers against each other.  Note that
	 * the APIC is counting DOWN instead of up!  Seems like on
	 * qemu, the APIC base frequency is 3.7x slower than the tsc.
	 * Looking at source, it seems like APIC is uniformly shifted
	 * down from a nominal 1Ghz reference
	 * (i.e. qemu_get_time_ns()), where the TSC is based on
	 * cpu_get_ticks() and thus pulls in wall clock time & such.
	 * If you specify "-icount shift=1", then they synchronize
	 * properly.
	 */
	int tsc0, apic0, tsc1, apic1;

	__asm__ volatile("rdtsc" : "=a"(tsc0) : : "rdx");
	apic0 = _apic.CURR_COUNT;
	do {
		/* Qemu misbehaves if I spam these registers. */
		for (int i = 0; i < 1000; i++) {
			__asm__ volatile("nop");
		}

		__asm__ volatile("rdtsc" : "=a"(tsc1) : : "rdx");
		apic1 = _apic.CURR_COUNT;
	} while ((tsc1 - tsc0) < 10000 || (apic0 - apic1) < 10000);
	printf("tsc %d apic %d\n", tsc1 - tsc0, apic0 - apic1);
}

void handler_timer(void *arg, int err)
{
	printf("Timer expired on CPU%d\n", (int)(long)xuk_get_f_ptr());
}

void handler_f3(void *arg, int err)
{
	printf("f3 handler on cpu%d arg %x, triggering INT 0xff\n",
	       (int)(long)xuk_get_f_ptr(), (int)(long)arg);
	__asm__ volatile("int $0xff");
	printf("end f3 handler\n");
}

void z_unhandled_vector(int vector, int err, struct xuk_entry_frame *f)
{
	(void)f;
	z_putchar = putchar;
	printf("Unhandled vector %d (err %xh) on CPU%d\n",
	       vector, err, (int)(long)xuk_get_f_ptr());
}

void z_isr_entry(void)
{
}

void *z_isr_exit_restore_stack(void *interrupted)
{
	/* Somewhat hacky test of the ISR exit modes.  Two ways of
	 * specifying "this stack", one of which does the full spill
	 * and restore and one shortcuts that due to the NULL
	 * return
	 */
	if (rdtsc() & 1) {
		return interrupted;
	} else {
		return 0;
	}
}

void *switch_back_to;

void switch_back(int arg1, int arg2, int arg3)
{
	printf("Switching back (%d, %d, %d) sbt %xh\n",
	       arg1, arg2, arg3, (int)(long)switch_back_to);
	xuk_switch(switch_back_to, &switch_back_to);
}

void test_switch(void)
{
	static unsigned long long stack[256];
	long args[] = { 5, 4, 3 };
	int eflags = 0x20; /* interrupts disabled */

	long handle = xuk_setup_stack((long)(sizeof(stack) + (char *)stack),
				      switch_back, eflags, args, 3);

	printf("Switching to %xh (stack %xh)\n",
	       (int)handle, (int)(long)&stack[0]);
	__asm__ volatile("cli");
	xuk_switch((void *)handle, &switch_back_to);
	__asm__ volatile("sti");
	printf("Back from switch\n");
}

void local_ipi_handler(void *arg, int err)
{
	printf("local IPI handler on CPU%d\n", (int)(long)xuk_get_f_ptr());
}

/* Sends an IPI to the current CPU and validates it ran */
void test_local_ipi(void)
{
	printf("Testing a local IPI on CPU%d\n", (int)(long)xuk_get_f_ptr());

	_apic.ICR_HI = (struct apic_icr_hi) {};
	_apic.ICR_LO = (struct apic_icr_lo) {
		.delivery_mode = FIXED,
		.vector = 0x90,
		.shorthand = SELF,
	};
}

void z_cpu_start(int cpu)
{
	z_putchar = putchar;
	printf("Entering demo kernel\n");

	/* Make sure the FS/GS pointers work, then set F to store our
	 * CPU ID
	 */
	xuk_set_f_ptr(cpu, (void *)(long)(0x19283700 + cpu));
	xuk_set_g_ptr(cpu, (void *)(long)(0xabacad00 + cpu));
	printf("fptr %p gptr %p\n", xuk_get_f_ptr(), xuk_get_g_ptr());

	xuk_set_f_ptr(cpu, (void *)(long)cpu);

	/* Set up this CPU's timer */
	/* FIXME: this sets up a separate vector for every CPU's
	 * timer, and we'll run out.  They should share the vector but
	 * still have individually-set APIC config.  Probably wants a
	 * "timer" API
	 */
	xuk_set_isr(INT_APIC_LVT_TIMER, 10, handler_timer, 0);
	_apic.INIT_COUNT = 5000000U;
	test_timers();

	if (cpu == 0) {
		xuk_start_cpu(1, (long)alloc_page(0) + 4096);
		xuk_set_isr(0x1f3, 0, (void *)handler_f3, (void *)0x12345678);
	}

	__asm__ volatile("int $0xf3");

	/* Fire it all up */
	printf("Enabling Interrupts\n");
	__asm__ volatile("sti");
	printf("Interrupts are unmasked (eflags %xh), here we go...\n",
	       eflags());

	/* Wait a teeny bit then send an IPI to CPU0, which will hit
	 * the unhandled_vector handler
	 */
	if (cpu == 1) {
		int t0 = rdtsc();

		while (rdtsc() - t0 < 1000000) {
		}

		_apic.ICR_HI = (struct apic_icr_hi) {
			.destination = 0
		};
		_apic.ICR_LO = (struct apic_icr_lo) {
			.delivery_mode = FIXED,
			.vector = 66,
		};
		while (_apic.ICR_LO.send_pending) {
		}
	}

	test_switch();

	xuk_set_isr(XUK_INT_RAW_VECTOR(0x90), -1, local_ipi_handler, 0);
	test_local_ipi();

	printf("CPU%d initialized, sleeping\n", cpu);
	while (1) {
		__asm__ volatile("hlt");
	}
}
