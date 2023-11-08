/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <soc/dport_reg.h>
#include <soc/gpio_periph.h>
#include <soc/rtc_periph.h>

#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <soc.h>
#include <ksched.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/kernel_structs.h>

#define Z_REG(base, off) (*(volatile uint32_t *)((base) + (off)))

#define RTC_CNTL_BASE             0x3ff48000
#define RTC_CNTL_OPTIONS0     Z_REG(RTC_CNTL_BASE, 0x0)
#define RTC_CNTL_SW_CPU_STALL Z_REG(RTC_CNTL_BASE, 0xac)

#define DPORT_BASE                 0x3ff00000
#define DPORT_APPCPU_CTRL_A    Z_REG(DPORT_BASE, 0x02C)
#define DPORT_APPCPU_CTRL_B    Z_REG(DPORT_BASE, 0x030)
#define DPORT_APPCPU_CTRL_C    Z_REG(DPORT_BASE, 0x034)

#ifdef CONFIG_SMP
struct cpustart_rec {
	int cpu;
	arch_cpustart_t fn;
	char *stack_top;
	void *arg;
	int vecbase;
	volatile int *alive;
};

volatile struct cpustart_rec *start_rec;
static void *appcpu_top;
static bool cpus_active[CONFIG_MP_MAX_NUM_CPUS];
#endif
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
#ifndef CONFIG_SOC_ESP32_PROCPU
	k_spinlock_key_t key = k_spin_lock(&loglock);

	while (*msg) {
		esp_rom_uart_tx_one_char(*msg++);
	}
	esp_rom_uart_tx_one_char('\r');
	esp_rom_uart_tx_one_char('\n');

	k_spin_unlock(&loglock, key);
#endif
}

#ifdef CONFIG_SMP
static void appcpu_entry2(void)
{
	volatile int ps, ie;

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

	smp_log("ESP32: APPCPU running");

	*start_rec->alive = 1;
	start_rec->fn(start_rec->arg);
}

/* Defines a locally callable "function" named _stack-switch().  The
 * first argument (in register a2 post-ENTRY) is the new stack pointer
 * to go into register a1.  The second (a3) is the entry point.
 * Because this never returns, a0 is used as a scratch register then
 * set to zero for the called function (a null return value is the
 * signal for "top of stack" to the debugger).
 */
void z_appcpu_stack_switch(void *stack, void *entry);
__asm__("\n"
	".align 4"		"\n"
	"z_appcpu_stack_switch:"	"\n\t"

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
	z_appcpu_stack_switch(appcpu_top, appcpu_entry2);
}
#endif

/* The calls and sequencing here were extracted from the ESP-32
 * FreeRTOS integration with just a tiny bit of cleanup.  None of the
 * calls or registers shown are documented, so treat this code with
 * extreme caution.
 */
void esp_appcpu_start(void *entry_point)
{
	smp_log("ESP32: starting APPCPU");

	/* These two calls are wrapped in a "stall_other_cpu" API in
	 * esp-idf.  But in this context the appcpu is stalled by
	 * definition, so we can skip that complexity and just call
	 * the ROM directly.
	 */
	esp_rom_Cache_Flush(1);
	esp_rom_Cache_Read_Enable(1);

	esp_rom_ets_set_appcpu_boot_addr((void *)0);

	RTC_CNTL_SW_CPU_STALL &= ~RTC_CNTL_SW_STALL_APPCPU_C1;
	RTC_CNTL_OPTIONS0     &= ~RTC_CNTL_SW_STALL_APPCPU_C0;
	DPORT_APPCPU_CTRL_B   |= DPORT_APPCPU_CLKGATE_EN;
	DPORT_APPCPU_CTRL_C   &= ~DPORT_APPCPU_RUNSTALL;

	/* Pulse the RESETTING bit */
	DPORT_APPCPU_CTRL_A |= DPORT_APPCPU_RESETTING;
	DPORT_APPCPU_CTRL_A &= ~DPORT_APPCPU_RESETTING;


	/* extracted from SMP LOG above, THIS IS REQUIRED FOR AMP RELIABLE
	 * OPERATION AS WELL, PLEASE DON'T touch on the dummy write below!
	 *
	 * Note that the logging done here is ACTUALLY REQUIRED FOR RELIABLE
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
	 */
	esp_rom_uart_tx_one_char('\r');
	esp_rom_uart_tx_one_char('\r');
	esp_rom_uart_tx_one_char('\n');

	/* Seems weird that you set the boot address AFTER starting
	 * the CPU, but this is how they do it...
	 */
	esp_rom_ets_set_appcpu_boot_addr((void *)entry_point);

	smp_log("ESP32: APPCPU start sequence complete");
}

#ifdef CONFIG_SMP
IRAM_ATTR static void esp_crosscore_isr(void *arg)
{
	ARG_UNUSED(arg);

	/* Right now this interrupt is only used for IPIs */
	z_sched_ipi();

	const int core_id = esp_core_id();

	if (core_id == 0) {
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_0_REG, 0);
	} else {
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_1_REG, 0);
	}
}

void arch_cpu_start(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
	volatile struct cpustart_rec sr;
	int vb;
	volatile int alive_flag;

	__ASSERT(cpu_num == 1, "ESP-32 supports only two CPUs");

	__asm__ volatile("rsr.VECBASE %0\n\t" : "=r"(vb));

	alive_flag = 0;

	sr.cpu = cpu_num;
	sr.fn = fn;
	sr.stack_top = Z_KERNEL_STACK_BUFFER(stack) + sz;
	sr.arg = arg;
	sr.vecbase = vb;
	sr.alive = &alive_flag;

	appcpu_top = Z_KERNEL_STACK_BUFFER(stack) + sz;

	start_rec = &sr;

	esp_appcpu_start(appcpu_entry1);

	while (!alive_flag) {
	}

	cpus_active[0] = true;
	cpus_active[cpu_num] = true;

	esp_intr_alloc(DT_IRQN(DT_NODELABEL(ipi0)),
		ESP_INTR_FLAG_IRAM,
		esp_crosscore_isr,
		NULL,
		NULL);

	esp_intr_alloc(DT_IRQN(DT_NODELABEL(ipi1)),
		ESP_INTR_FLAG_IRAM,
		esp_crosscore_isr,
		NULL,
		NULL);

	smp_log("ESP32: APPCPU initialized");
}

void arch_sched_ipi(void)
{
	const int core_id = esp_core_id();

	if (core_id == 0) {
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_0_REG, DPORT_CPU_INTR_FROM_CPU_0);
	} else {
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_1_REG, DPORT_CPU_INTR_FROM_CPU_1);
	}
}

IRAM_ATTR bool arch_cpu_active(int cpu_num)
{
	return cpus_active[cpu_num];
}
#endif /* CONFIG_SMP */
