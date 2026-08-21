/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>

#include <soc.h>
#include <esp_cpu.h>
#include <esp_rom_serial_output.h>

#include <esp_mcuboot_image.h>
#include <esp_memory_utils.h>
#include <zephyr/zsr.h>

#ifdef CONFIG_SMP

#include <esp_rom_sys.h>
#include <xt_instr_macros.h>
#include <zephyr/arch/xtensa/smp.h>

static struct k_spinlock loglock;

/* VECBASE value for whichever secondary core is currently being started.
 * ESP32 only ever starts core 1 (__ASSERT'd in soc_mp_start_core() below),
 * so a single instance is enough -- no need for a per-core array the way
 * the shared layer's own cpustart_rec[] needs one.
 */
static int appcpu_vecbase;

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
		esp_rom_output_tx_one_char(*msg++);
	}
	esp_rom_output_tx_one_char('\r');
	esp_rom_output_tx_one_char('\n');

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
	/* Raise INTLEVEL before clearing INTENABLE below (not just to 0) so
	 * stale INTENABLE bits left over from the ROM handoff can't let a
	 * spurious interrupt fire before the wsr.INTENABLE below.
	 */
	ps |= XCHAL_EXCM_LEVEL;
	__asm__ volatile("wsr.PS %0" : : "r"(ps));

	ie = 0;
	__asm__ volatile("wsr.INTENABLE %0" : : "r"(ie));
	__asm__ volatile("wsr.VECBASE %0" : : "r"(appcpu_vecbase));
	__asm__ volatile("rsync");

	smp_log("ESP32: APPCPU running");

	/* Hands off to the shared Xtensa SMP layer (arch/xtensa/core/smp.c):
	 * sets up the ZSR_CPU pointer, calls soc_mp_startup_self(1) (below)
	 * to register this core's own incoming IPI line, marks this core
	 * active, then calls the fn/arg pair arch_cpu_start() was given.
	 * Never returns.
	 */
	xtensa_smp_secondary_start(1);
}

/* Direct ROM boot target -- an ordinary C function, matching ESP-IDF's
 * call_start_cpu1() shape: runs on whatever SP the ROM handoff left in a1.
 * Region 1 (0x20000000-0x3FFFFFFF) must be unlocked for RW before any code
 * here touches DRAM (e.g. appcpu_entry2()'s locals) -- matches ESP-IDF's own
 * esp_cpu_configure_region_protection(), called at the same point in
 * call_start_cpu1().
 */
void appcpu_entry1(void)
{
	WDTLB(0x0, 0x20000000);

	appcpu_entry2();
}

/* The calls and sequencing here were extracted from the ESP-32
 * FreeRTOS integration with just a tiny bit of cleanup.  None of the
 * calls or registers shown are documented, so treat this code with
 * extreme caution.
 */
/* ESP32 (unlike every later Espressif chip) has two entirely separate flash
 * MMU page-table hardware banks, one per core (DR_REG_FLASH_MMU_TABLE_PRO /
 * _APP). Zephyr's image loader only ever populates PRO's table
 * (mmu_hal_map_region() is hardcoded to core 0); APP's is left empty by
 * reset_mmu()'s mmu_init(1) call and never filled in anywhere else in-tree.
 * Without this copy, APPCPU has no valid mapping for its own flash-cached
 * (IROM) code. Matches ESP-IDF's restore_app_mmu_from_pro_mmu().
 */
static void restore_app_mmu_from_pro_mmu(void)
{
	volatile uint32_t *from = (volatile uint32_t *)DR_REG_FLASH_MMU_TABLE_PRO;
	volatile uint32_t *to = (volatile uint32_t *)DR_REG_FLASH_MMU_TABLE_APP;

	/* Matches ESP-IDF's restore_app_mmu_from_pro_mmu() mmu_reg_num exactly. */
	for (int i = 0; i < 2048; i++) {
		*(to++) = *(from++);
	}
}

void esp_appcpu_start(void *entry_point)
{
	ets_printf("ESP32: starting APPCPU");

	/* Cache must stay disabled while the MMU table below is being
	 * overwritten, under the DPORT_APP_CACHE_MMU_IA_CLR erratum guard --
	 * a real Xtensa/ESP32 hardware erratum (see Zephyr's own reset_mmu(),
	 * which does the same for PRO_CPU). Enabling cache read before the
	 * table write, or skipping the guard, makes APPCPU's first
	 * flash-cached instruction fetch stall forever with no exception.
	 */
	esp_rom_Cache_Read_Disable(1);
	esp_rom_Cache_Flush(1);

	DPORT_SET_PERI_REG_MASK(DPORT_APP_CACHE_CTRL1_REG, DPORT_APP_CACHE_MMU_IA_CLR);
	restore_app_mmu_from_pro_mmu();
	DPORT_CLEAR_PERI_REG_MASK(DPORT_APP_CACHE_CTRL1_REG, DPORT_APP_CACHE_MMU_IA_CLR);

	esp_rom_Cache_Read_Enable(1);

	esp_rom_ets_set_appcpu_boot_addr((void *)0);

	DPORT_SET_PERI_REG_MASK(DPORT_APPCPU_CTRL_B_REG, DPORT_APPCPU_CLKGATE_EN);
	DPORT_CLEAR_PERI_REG_MASK(DPORT_APPCPU_CTRL_C_REG, DPORT_APPCPU_RUNSTALL);
	DPORT_SET_PERI_REG_MASK(DPORT_APPCPU_CTRL_A_REG, DPORT_APPCPU_RESETTING);
	DPORT_CLEAR_PERI_REG_MASK(DPORT_APPCPU_CTRL_A_REG, DPORT_APPCPU_RESETTING);

	/* Seems weird that you set the boot address AFTER starting
	 * the CPU, but this is how they do it...
	 */
	esp_rom_ets_set_appcpu_boot_addr((void *)entry_point);

	/* mcuboot's appcpu_start() follows the boot-address write with a real
	 * 10ms delay plus a wait for the UART to actually finish transmitting,
	 * not just a couple of dummy character writes -- matching that here
	 * instead of guessing at the timing.
	 */
	esp_rom_delay_us(10000);
	esp_rom_output_tx_wait_idle(0);
}

void soc_mp_start_core(int cpu_num)
{
	__ASSERT(cpu_num == 1, "ESP-32 supports only two CPUs");

	__asm__ volatile("rsr.VECBASE %0\n\t" : "=r"(appcpu_vecbase));

	esp_appcpu_start(appcpu_entry1);
}

void soc_mp_startup_self(int cpu_num)
{
	int ret;

	/* ipi0 (FROM_CPU_INTR0_SOURCE, tied to DPORT_CPU_INTR_FROM_CPU_0_REG)
	 * is core 1's own incoming line: it fires when core 0 writes that
	 * register. ipi1/FROM_CPU_1 is the mirror image, core 0's own
	 * incoming line. Each core registers only its own line, on itself
	 * -- this is the structural fix for the bug where both lines used
	 * to be registered while executing on PRO_CPU only, leaving
	 * APPCPU's own line (ipi0) never actually enabled on APPCPU.
	 */
	if (cpu_num == 0) {
		ret = esp_intr_alloc(
			DT_IRQ_BY_IDX(DT_NODELABEL(ipi1), 0, irq),
			ESP_PRIO_TO_FLAGS(DT_IRQ_BY_IDX(DT_NODELABEL(ipi1), 0, priority)) |
				ESP_INT_FLAGS_CHECK(DT_IRQ_BY_IDX(DT_NODELABEL(ipi1), 0, flags)) |
				ESP_INTR_FLAG_IRAM,
			xtensa_smp_ipi_isr, NULL, NULL);
	} else {
		ret = esp_intr_alloc(
			DT_IRQ_BY_IDX(DT_NODELABEL(ipi0), 0, irq),
			ESP_PRIO_TO_FLAGS(DT_IRQ_BY_IDX(DT_NODELABEL(ipi0), 0, priority)) |
				ESP_INT_FLAGS_CHECK(DT_IRQ_BY_IDX(DT_NODELABEL(ipi0), 0, flags)) |
				ESP_INTR_FLAG_IRAM,
			xtensa_smp_ipi_isr, NULL, NULL);

		smp_log("ESP32: APPCPU initialized");
	}

	/* A silently-unregistered IPI line here means this core's own incoming
	 * IPI signal is lost forever -- no exception, no crash, just missed
	 * scheduler wake-ups on this core. Fail loud instead.
	 */
	__ASSERT(ret == 0, "failed to register CPU %d IPI line: %d", cpu_num, ret);
}

void soc_ipi_trigger(int cpu_num)
{
	if (cpu_num == 1) {
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_0_REG, DPORT_CPU_INTR_FROM_CPU_0);
	} else {
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_1_REG, DPORT_CPU_INTR_FROM_CPU_1);
	}
}

void soc_ipi_clear(void)
{
	if (esp_core_id() == 0) {
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_1_REG, 0);
	} else {
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_0_REG, 0);
	}
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

#include <bootloader_flash_priv.h>

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
	const uint32_t img_off = PARTITION_OFFSET(slot0_appcpu_partition);
	const uint32_t fa_size = PARTITION_SIZE(slot0_appcpu_partition);
	const uint8_t fa_id = PARTITION_ID(slot0_appcpu_partition);

	if (entry_addr == NULL) {
		ets_printf("Can't return the entry address. Aborting!\n");
		abort();
		return -1;
	}

	uint32_t mcuboot_header[8] = {0};
	esp_image_load_header_t image_header = {0};

	const uint32_t *data = (const uint32_t *)sys_mmap(img_off, 0x80);

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
	esp_rom_output_tx_wait_idle(0);

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
