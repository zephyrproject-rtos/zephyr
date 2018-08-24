#include "serial.h"
#include "vgacon.h"
#include "printf.h"
#include "core64.h"

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
		for(int i=0; i<1000; i++) __asm__ volatile("nop");

		__asm__ volatile("rdtsc" : "=a"(tsc1) : : "rdx");
		apic1 = _apic.CURR_COUNT;
	} while((tsc1 - tsc0) < 10000 || (apic0 - apic1) < 10000);
	printf("tsc %d apic %d\n", tsc1 - tsc0, apic0 - apic1);
}

unsigned int init_cpu_stack(int cpu)
{
	return (long)alloc_page(0) + 4096;
}

void handler_timer(void *arg, int err)
{
	printf("Timer expired on CPU%d\n", (int)(long)get_f_ptr());
}

void handler_f3(void *arg, int err)
{
	printf("handler_f3 on cpu%d arg %x, triggering INT 0xff\n",
	       (int)(long)get_f_ptr(), (int)(long)arg);
	__asm__ volatile("int $0xff");
	printf("end handler_f3\n");
}

void unhandled_vector(int vector, int err)
{
	_putchar = putchar;
	printf("Unhandled vector %d (err %xh) on CPU%d\n",
	       vector, err, (int)(long)get_f_ptr());
}

void *isr_exit_restore_stack(void *interrupted)
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

void cpu_start(int cpu)
{
	_putchar = putchar;

	/* Make sure the FS/GS pointers work, then set F to store our
	 * CPU ID
	 */
	set_f_ptr(cpu, (void*)(long)(0x19283700 + cpu));
	set_g_ptr(cpu, (void*)(long)(0xabacad00 + cpu));
	printf("fptr %p gptr %p\n", get_f_ptr(), get_g_ptr());

	set_f_ptr(cpu, (void*)(long)cpu);

	/* Set up this CPU's timer */
	// FIXME: this sets up a separate vector for every CPU's
	// timer, and we'll run out.  They should share the vector but
	// still have individually-set APIC config.  Probably wants a
	// "timer" API
	set_isr(INT_APIC_LVT_TIMER, 10, handler_timer, 0);
	_apic.INIT_COUNT = 5000000;
	test_timers();

	if (cpu == 0) {
		set_isr(0x1f3, 0, (void *)handler_f3, (void *)0x12345678);
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

		while(rdtsc() - t0 < 1000000) {
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

	printf("CPU%d initialzed, sleeping\n", cpu);
	while (1) {
		__asm__ volatile("hlt");
	}
}
