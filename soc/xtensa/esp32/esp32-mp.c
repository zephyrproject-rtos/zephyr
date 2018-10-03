/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <soc.h>

#include <zephyr.h>
#include <spinlock.h>
#include <kernel_structs.h>

#define _REG(base, off) (*(volatile u32_t *)((base) + (off)))

#define RTC_CNTL_BASE             0x3ff48000
#define RTC_CNTL_OPTIONS0     _REG(RTC_CNTL_BASE, 0x0)
#define RTC_CNTL_SW_CPU_STALL _REG(RTC_CNTL_BASE, 0xac)

#define DPORT_BASE                 0x3ff00000
#define DPORT_APPCPU_CTRL_A    _REG(DPORT_BASE, 0x02C)
#define DPORT_APPCPU_CTRL_B    _REG(DPORT_BASE, 0x030)
#define DPORT_APPCPU_CTRL_C    _REG(DPORT_BASE, 0x034)

struct cpustart_rec {
	int cpu;
	void (*fn)(int, void *);
	char *stack_top;
	void *arg;
	int vecbase;
	volatile int *alive;
};

volatile struct cpustart_rec *start_rec;
static void *appcpu_top;

static struct k_spinlock loglock;

/* Note that the logging done here is ACTUALLY REQUIRED FOR RELIABLE
 * OPERATION!  At least one particular board will experience spurious
 * hangs during initialization (usually the APPCPU fails to start at
 * all) without these calls present.  It's not just time -- careful
 * use of k_busy_wait() (and even hand-crafted timer loops using the
 * Xtensa timer SRs directly) that duplicates the timing exactly still
 * sees hangs.  Something is happening inside the ROM UART code that
 * magically makes the startup sequence reliable.
 *
 * Leave this in place until the sequence is understood better.
 *
 * (Note that the use of the spinlock is cosmetic only -- if you take
 * it out the messages will interleave across the two CPUs but startup
 * will still be reliable.)
 */
void smp_log(const char *msg)
{
	k_spinlock_key_t key = k_spin_lock(&loglock);

	while (*msg) {
		esp32_rom_uart_tx_one_char(*msg++);
	}
	esp32_rom_uart_tx_one_char('\r');
	esp32_rom_uart_tx_one_char('\n');

	k_spin_unlock(&loglock, key);
}

static void appcpu_entry2(void)
{
	volatile int ps, ie;
	smp_log("ESP32: APPCPU running");

	/* Copy over VECBASE from the main CPU for an initial value
	 * (will need to revisit this if we ever allow a user API to
	 * change interrupt vectors at runtime).  Make sure interrupts
	 * are locally disabled, then synthesize a PS value that will
	 * enable them for the user code to pass to irq_unlock()
	 * later.
	 */
	__asm__ volatile("rsr.PS %0" : "=r"(ps));
	ps &= ~(PS_EXCM_MASK | PS_INTLEVEL_MASK);
	__asm__ volatile("wsr.PS %0" : : "r"(ps));

	ie = 0;
	__asm__ volatile("wsr.INTENABLE %0" : : "r"(ie));
	__asm__ volatile("wsr.VECBASE %0" : : "r"(start_rec->vecbase));
	__asm__ volatile("rsync");

	/* Set up the CPU pointer.  Really this should be xtensa arch
	 * code, not in the ESP-32 layer
	 */
	_cpu_t *cpu = &_kernel.cpus[1];

	__asm__ volatile("wsr.MISC0 %0" : : "r"(cpu));

	*start_rec->alive = 1;
	start_rec->fn(ps, start_rec->arg);
}

/* Defines a locally callable "function" named _stack-switch().  The
 * first argument (in register a2 post-ENTRY) is the new stack pointer
 * to go into register a1.  The second (a3) is the entry point.
 * Because this never returns, a0 is used as a scratch register then
 * set to zero for the called function (a null return value is the
 * signal for "top of stack" to the debugger).
 */
void _appcpu_stack_switch(void *stack, void *entry);
__asm__("\n"
	".align 4"		"\n"
	"_appcpu_stack_switch:"	"\n\t"

	"entry a1, 16"		"\n\t"

	/* Subtle: we want the stack to be 16 bytes higher than the
	 * top on entry to the called function, because the ABI forces
	 * it to assume that those bytes are for its caller's A0-A3
	 * spill area.  (In fact ENTRY instructions with stack
	 * adjustments less than 16 are a warning condition in the
	 * assembler). But we aren't a caller, have no bit set in
	 * WINDOWSTART and will never be asked to spill anything.
	 * Those 16 bytes would otherwise be wasted on the stack, so
	 * adjust
	 */
	"addi a1, a2, 16"	"\n\t"

	/* Clear WINDOWSTART so called functions never try to spill
	 * our callers' registers into the now-garbage stack pointers
	 * they contain.  No need to set the bit corresponding to
	 * WINDOWBASE, our C callee will do that when it does an
	 * ENTRY.
	 */
	"movi a0, 0"		"\n\t"
	"wsr.WINDOWSTART a0"	"\n\t"

	/* Clear CALLINC field of PS (you would think it would, but
	 * our ENTRY doesn't actually do that) so the callee's ENTRY
	 * doesn't shift the registers
	 */
	"rsr.PS a0"		"\n\t"
	"movi a2, 0xfffcffff"	"\n\t"
	"and a0, a0, a2"	"\n\t"
	"wsr.PS a0"		"\n\t"

	"rsync"			"\n\t"
	"movi a0, 0"		"\n\t"

	"jx a3"			"\n\t");

/* Carefully constructed to use no stack beyond compiler-generated ABI
 * instructions.  WE DO NOT KNOW WHERE THE STACK FOR THIS FUNCTION IS.
 * The ROM library just picks a spot on its own with no input from our
 * app linkage and tells us nothing about it until we're already
 * running.
 */
static void appcpu_entry1(void)
{
	_appcpu_stack_switch(appcpu_top, appcpu_entry2);
}

/* The calls and sequencing here were extracted from the ESP-32
 * FreeRTOS integration with just a tiny bit of cleanup.  None of the
 * calls or registers shown are documented, so treat this code with
 * extreme caution.
 */
static void appcpu_start(void)
{
	smp_log("ESP32: starting APPCPU");

	/* These two calls are wrapped in a "stall_other_cpu" API in
	 * esp-idf.  But in this context the appcpu is stalled by
	 * definition, so we can skip that complexity and just call
	 * the ROM directly.
	 */
	esp32_rom_Cache_Flush(1);
	esp32_rom_Cache_Read_Enable(1);

	RTC_CNTL_SW_CPU_STALL &= ~RTC_CNTL_SW_STALL_APPCPU_C1;
	RTC_CNTL_OPTIONS0     &= ~RTC_CNTL_SW_STALL_APPCPU_C0;
	DPORT_APPCPU_CTRL_B   |= DPORT_APPCPU_CLKGATE_EN;
	DPORT_APPCPU_CTRL_C   &= ~DPORT_APPCPU_RUNSTALL;

	/* Pulse the RESETTING bit */
	DPORT_APPCPU_CTRL_A |= DPORT_APPCPU_RESETTING;
	DPORT_APPCPU_CTRL_A &= ~DPORT_APPCPU_RESETTING;

	/* Seems weird that you set the boot address AFTER starting
	 * the CPU, but this is how they do it...
	 */
	esp32_rom_ets_set_appcpu_boot_addr((void *)appcpu_entry1);

	smp_log("ESP32: APPCPU start sequence complete");
}

void _arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
		     void (*fn)(int, void *), void *arg)
{
	volatile struct cpustart_rec sr;
	int vb;
	volatile int alive_flag;

	__ASSERT(cpu_num == 1, "ESP-32 supports only two CPUs");

	__asm__ volatile("rsr.VECBASE %0\n\t" : "=r"(vb));

	alive_flag = 0;

	sr.cpu = cpu_num;
	sr.fn = fn;
	sr.stack_top = K_THREAD_STACK_BUFFER(stack) + sz;
	sr.arg = arg;
	sr.vecbase = vb;
	sr.alive = &alive_flag;

	appcpu_top = K_THREAD_STACK_BUFFER(stack) + sz;

	start_rec = &sr;

	appcpu_start();

	while (!alive_flag) {
	}

	smp_log("ESP32: APPCPU initialized");
}
