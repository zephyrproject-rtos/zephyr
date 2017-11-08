#include <zephyr.h>
#include <misc/printk.h>

#define _REG(base, off) (*(volatile u32_t *)((base) + (off)))

#define RTC_CNTL_BASE             0x3ff48000
#define RTC_CNTL_OPTIONS0     _REG(RTC_CNTL_BASE, 0x0)
#define RTC_CNTL_SW_CPU_STALL _REG(RTC_CNTL_BASE, 0xac)

#define DPORT_BASE                 0x3ff00000
#define DPORT_APPCPU_CTRL_A    _REG(DPORT_BASE, 0x02C)
#define DPORT_APPCPU_CTRL_B    _REG(DPORT_BASE, 0x030)
#define DPORT_APPCPU_CTRL_C    _REG(DPORT_BASE, 0x034)

/* Two fields with same naming conventions live in two different
 * registers and have different widths...
 */
#define RTC_CNTL_SW_STALL_APPCPU_C0  (0x03 << 0)  /* reg: OPTIONS0 */
#define RTC_CNTL_SW_STALL_APPCPU_C1  (0x3f << 20) /* reg: SW_CPU_STALL */

#define DPORT_APPCPU_CLKGATE_EN  BIT(0)
#define DPORT_APPCPU_RUNSTALL    BIT(0)
#define DPORT_APPCPU_RESETTING   BIT(0)


/* These calls are ROM-resident and have fixed addresses. No, I don't
 * know how they plan on updating these portably either.
 */
typedef void (*esp32rom_call_t)(int);
static const esp32rom_call_t esp32rom_Cache_Flush = (void*)0x40009a14;
static const esp32rom_call_t esp32rom_Cache_Read_Enable = (void*)0x40009a84;
static const esp32rom_call_t esp32rom_ets_set_appcpu_boot_addr = (void*)0x4000689c;

volatile int counter;
volatile int ccount1;
volatile void *stack1;

/* Carefully constructed to use no stack beyond compiler-generated ABI
 * instructions.  WE DO NOT KNOW WHERE THE STACK FOR THIS FUNCTION IS.
 * The ROM library just picks a spot on its own with no input from our
 * app linkage and tells us nothing about it until we're already
 * running.
 *
 * This is also a problem for stack switching: we don't know what the
 * register window state might be either.  Remember Xtensa registers
 * are lazy-filled: it's not impossible (though in practice probably
 * not likely) that access to a4-a15 in the swapped stack will cause
 * an exception that tries to "fill" them from a stack that no longer
 * exists.  Need to research how to properly reset the register window
 * (which is different from existing context switch code, which can
 * presume a working stack on which to spill!).
 */
void appcpu_entry(void)
{
	__asm__ volatile("s32i sp, %0, 0" : "=a"(stack1));
	counter = 1000;
	while (1) {
		static volatile int dummy;
		for (dummy = 0; dummy < 1000000; dummy++);
		__asm__ volatile("rsr.ccount %0" : "=a"(ccount1));
		counter++;
	}
}

/* The calls and sequencing here were extracted from the ESP-32
 * FreeRTOS integration with just a tiny bit of cleanup.  None of the
 * calls or registers shown are documented, so treat this code with
 * extreme caution.
 */
void appcpu_start(void)
{
	/* These two calls are wrapped in a "stall_other_cpu" API in
	 * esp-idf.  But in this context the appcpu is stalled by
	 * definition, so we can skip that complexity and just call
	 * the ROM directly.
	 */
	esp32rom_Cache_Flush(1);
	esp32rom_Cache_Read_Enable(1);

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
	esp32rom_ets_set_appcpu_boot_addr((uint32_t)appcpu_entry);
}

u32_t read_ccount(void)
{
	u32_t val;
	__asm__ volatile("rsr.ccount %0" : "=a"(val));
	return val;
}

void main(void)
{
	int now, last = counter;
	u32_t cc0, last_cc0, last_cc1=0; /* benchmarking the CCOUNT timers vs. each other */

	printk("Starting app cpu...\n");

	int start = read_ccount();
	appcpu_start();
	int mid = read_ccount();
	while (!counter) {}
	int end = read_ccount();
	printk("App CPU alive, took %d cycles total, waited %d\n", end - start, end - mid);
	printk("App CPU stack pointer: %p\n", stack1);

	/* Spin forever watching the "counter" variable for changes from the other side */
	last_cc0 = end;
	while (1) {
		do {
			now = counter;
		} while (now == last);

		cc0 = read_ccount();
		printk("app cpu set counter to %d (dt0 %d dt1 %d)\n", now, cc0-last_cc0, ccount1-last_cc1);
		last_cc0 = cc0;
		last_cc1 = ccount1;

		last = now;
	}
}
