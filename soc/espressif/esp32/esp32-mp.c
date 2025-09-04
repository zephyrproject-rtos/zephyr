/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>

#include <soc.h>
#include <esp_cpu.h>
#include "esp_rom_uart.h"

#include "esp_mcuboot_image.h"
#include "esp_memory_utils.h"

#ifdef CONFIG_SMP

#include <ipi.h>

#ifndef CONFIG_SOC_ESP32_PROCPU
static struct k_spinlock loglock;
#endif

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
		esp_rom_uart_tx_one_char(*msg++);
	}
	esp_rom_uart_tx_one_char('\r');
	esp_rom_uart_tx_one_char('\n');

	k_spin_unlock(&loglock, key);
}

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
	ps &= ~(XCHAL_PS_EXCM_MASK | XCHAL_PS_INTLEVEL_MASK);
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

/* The calls and sequencing here were extracted from the ESP-32
 * FreeRTOS integration with just a tiny bit of cleanup.  None of the
 * calls or registers shown are documented, so treat this code with
 * extreme caution.
 */
void esp_appcpu_start(void *entry_point)
{
	ets_printf("ESP32: starting APPCPU");

	/* These two calls are wrapped in a "stall_other_cpu" API in
	 * esp-idf.  But in this context the appcpu is stalled by
	 * definition, so we can skip that complexity and just call
	 * the ROM directly.
	 */
	esp_rom_Cache_Flush(1);
	esp_rom_Cache_Read_Enable(1);

	esp_rom_ets_set_appcpu_boot_addr((void *)0);

	DPORT_SET_PERI_REG_MASK(DPORT_APPCPU_CTRL_B_REG, DPORT_APPCPU_CLKGATE_EN);
	DPORT_CLEAR_PERI_REG_MASK(DPORT_APPCPU_CTRL_C_REG, DPORT_APPCPU_RUNSTALL);
	DPORT_SET_PERI_REG_MASK(DPORT_APPCPU_CTRL_A_REG, DPORT_APPCPU_RESETTING);
	DPORT_CLEAR_PERI_REG_MASK(DPORT_APPCPU_CTRL_A_REG, DPORT_APPCPU_RESETTING);

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

	ets_printf("ESP32: APPCPU start sequence complete");
}

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
	sr.stack_top = K_KERNEL_STACK_BUFFER(stack) + sz;
	sr.arg = arg;
	sr.vecbase = vb;
	sr.alive = &alive_flag;

	appcpu_top = K_KERNEL_STACK_BUFFER(stack) + sz;

	start_rec = &sr;

	esp_appcpu_start(appcpu_entry1);

	while (!alive_flag) {
	}

	cpus_active[0] = true;
	cpus_active[cpu_num] = true;

	esp_intr_alloc(DT_IRQ_BY_IDX(DT_NODELABEL(ipi0), 0, irq),
		ESP_PRIO_TO_FLAGS(DT_IRQ_BY_IDX(DT_NODELABEL(ipi0), 0, priority)) |
		ESP_INT_FLAGS_CHECK(DT_IRQ_BY_IDX(DT_NODELABEL(ipi0), 0, flags)) |
			ESP_INTR_FLAG_IRAM,
		esp_crosscore_isr,
		NULL,
		NULL);

	esp_intr_alloc(DT_IRQ_BY_IDX(DT_NODELABEL(ipi1), 0, irq),
		ESP_PRIO_TO_FLAGS(DT_IRQ_BY_IDX(DT_NODELABEL(ipi1), 0, priority)) |
		ESP_INT_FLAGS_CHECK(DT_IRQ_BY_IDX(DT_NODELABEL(ipi1), 0, flags)) |
			ESP_INTR_FLAG_IRAM,
		esp_crosscore_isr,
		NULL,
		NULL);

	smp_log("ESP32: APPCPU initialized");
}

void arch_sched_directed_ipi(uint32_t cpu_bitmap)
{
	const int core_id = esp_core_id();

	ARG_UNUSED(cpu_bitmap);

	if (core_id == 0) {
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_0_REG, DPORT_CPU_INTR_FROM_CPU_0);
	} else {
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_1_REG, DPORT_CPU_INTR_FROM_CPU_1);
	}
}

void arch_sched_broadcast_ipi(void)
{
	arch_sched_directed_ipi(IPI_ALL_CPUS_MASK);
}

IRAM_ATTR bool arch_cpu_active(int cpu_num)
{
	return cpus_active[cpu_num];
}
#endif /* CONFIG_SMP */

void esp_appcpu_start2(void *entry_point)
{
	esp_cpu_unstall(1);

	if (!DPORT_GET_PERI_REG_MASK(DPORT_APPCPU_CTRL_B_REG, DPORT_APPCPU_CLKGATE_EN)) {
		DPORT_SET_PERI_REG_MASK(DPORT_APPCPU_CTRL_B_REG, DPORT_APPCPU_CLKGATE_EN);
		DPORT_CLEAR_PERI_REG_MASK(DPORT_APPCPU_CTRL_C_REG, DPORT_APPCPU_RUNSTALL);
		DPORT_SET_PERI_REG_MASK(DPORT_APPCPU_CTRL_A_REG, DPORT_APPCPU_RESETTING);
		DPORT_CLEAR_PERI_REG_MASK(DPORT_APPCPU_CTRL_A_REG, DPORT_APPCPU_RESETTING);
	}

	esp_rom_ets_set_appcpu_boot_addr((void *)entry_point);

	esp_cpu_reset(1);
}

/* AMP support */
#ifdef CONFIG_SOC_ENABLE_APPCPU

#include "bootloader_flash_priv.h"

#define sys_mmap   bootloader_mmap
#define sys_munmap bootloader_munmap

static int load_segment(uint32_t src_addr, uint32_t src_len, uint32_t dst_addr)
{
	const uint32_t *data = (const uint32_t *)sys_mmap(src_addr, src_len);

	if (!data) {
		ets_printf("%s: mmap failed", __func__);
		return -1;
	}

	volatile uint32_t *dst = (volatile uint32_t *)dst_addr;

	for (int i = 0; i < src_len / 4; i++) {
		dst[i] = data[i];
	}

	sys_munmap(data);

	return 0;
}

int IRAM_ATTR esp_appcpu_image_load(unsigned int hdr_offset, unsigned int *entry_addr)
{
	const uint32_t img_off = FIXED_PARTITION_OFFSET(slot0_appcpu_partition);
	const uint32_t fa_size = FIXED_PARTITION_SIZE(slot0_appcpu_partition);
	const uint8_t fa_id = FIXED_PARTITION_ID(slot0_appcpu_partition);

	if (entry_addr == NULL) {
		ets_printf("Can't return the entry address. Aborting!\n");
		abort();
		return -1;
	}

	uint32_t mcuboot_header[8] = {0};
	esp_image_load_header_t image_header = {0};

	const uint32_t *data = (const uint32_t *)sys_mmap(img_off, 0x40);

	memcpy((void *)&mcuboot_header, data, sizeof(mcuboot_header));
	memcpy((void *)&image_header, data + (hdr_offset / sizeof(uint32_t)),
	       sizeof(esp_image_load_header_t));

	sys_munmap(data);

	if (image_header.header_magic == ESP_LOAD_HEADER_MAGIC) {
		ets_printf("APPCPU image, area id: %d, offset: 0x%x, hdr.off: 0x%x, size: %d kB\n",
			   fa_id, img_off, hdr_offset, fa_size / 1024);
	} else if ((image_header.header_magic & 0xff) == 0xE9) {
		ets_printf("ESP image format is not supported\n");
		abort();
	} else {
		ets_printf("Unknown or empty image detected. Aborting!\n");
		abort();
	}

	if (!esp_ptr_in_iram((void *)image_header.iram_dest_addr) ||
	    !esp_ptr_in_iram((void *)(image_header.iram_dest_addr + image_header.iram_size))) {
		ets_printf("IRAM region in load header is not valid. Aborting");
		abort();
	}

	if (!esp_ptr_in_dram((void *)image_header.dram_dest_addr) ||
	    !esp_ptr_in_dram((void *)(image_header.dram_dest_addr + image_header.dram_size))) {
		ets_printf("DRAM region in load header is not valid. Aborting");
		abort();
	}

	if (!esp_ptr_in_iram((void *)image_header.entry_addr)) {
		ets_printf("Application entry point (%xh) is not in IRAM. Aborting",
			   image_header.entry_addr);
		abort();
	}

	ets_printf("IRAM segment: paddr=%08xh, vaddr=%08xh, size=%05xh (%6d) load\n",
		   (img_off + image_header.iram_flash_offset), image_header.iram_dest_addr,
		   image_header.iram_size, image_header.iram_size);

	load_segment(img_off + image_header.iram_flash_offset, image_header.iram_size,
		     image_header.iram_dest_addr);

	ets_printf("DRAM segment: paddr=%08xh, vaddr=%08xh, size=%05xh (%6d) load\n",
		   (img_off + image_header.dram_flash_offset), image_header.dram_dest_addr,
		   image_header.dram_size, image_header.dram_size);

	load_segment(img_off + image_header.dram_flash_offset, image_header.dram_size,
		     image_header.dram_dest_addr);

	ets_printf("Application start=%xh\n\n", image_header.entry_addr);
	esp_rom_uart_tx_wait_idle(0);

	assert(entry_addr != NULL);
	*entry_addr = image_header.entry_addr;

	return 0;
}

void esp_appcpu_image_stop(void)
{
	esp_cpu_stall(1);
}

void esp_appcpu_image_start(unsigned int hdr_offset)
{
	static int started;
	unsigned int entry_addr = 0;

	if (started) {
		printk("APPCPU already started.\r\n");
		return;
	}

	/* Input image meta header, output appcpu entry point */
	esp_appcpu_image_load(hdr_offset, &entry_addr);

	esp_appcpu_start2((void *)entry_addr);
}

int esp_appcpu_init(void)
{
	/* Load APPCPU image using image header offset
	 * (skipping the MCUBoot header)
	 */
	esp_appcpu_image_start(0x20);

	return 0;
}

#if !defined(CONFIG_MCUBOOT)
extern int esp_appcpu_init(void);
SYS_INIT(esp_appcpu_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif

#endif /* CONFIG_SOC_ENABLE_APPCPU */
