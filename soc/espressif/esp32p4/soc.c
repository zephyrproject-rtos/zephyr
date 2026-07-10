/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <soc/soc.h>
#include <soc_init.h>
#include <flash_init.h>
#include <esp_err.h>
#include <esp_private/cache_utils.h>
#include <esp_private/system_internal.h>
#include <esp_timer.h>
#include <efuse_virtual.h>
#include <esp_rom_serial_output.h>
#include <stdint.h>
#include <zephyr/init.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/arch/common/init.h>
#include <zephyr/arch/riscv/arch.h>
#include <riscv/csr.h>
#include <hal/interrupt_clic_ll.h>
#include <riscv/interrupt.h>
#include <soc/interrupts.h>
#include <psram.h>
#include <zephyr/sys/printk.h>
#include <zephyr/cache.h>

extern void esp_reset_reason_init(void);
extern FUNC_NORETURN void z_cstart(void);

static void core_intr_matrix_clear(void)
{
	for (int i = 0; i < ETS_MAX_INTR_SOURCE; i++) {
		interrupt_clic_ll_route(0, i, ETS_INVALID_INUM);
	}

	for (int i = 0; i < 32; i++) {
		esprv_int_set_vectored(i, true);
	}
}

#if defined(CONFIG_NOCACHE_MEMORY)
/*
 * The nocache region is carved out of internal SRAM using PMA entries. The
 * region boundaries come from the linker symbols, so they automatically track
 * the usable SRAM window for the target silicon revision: on rev v1.3 the L2
 * cache sits at the top of SRAM, while on rev v3.x it sits at the bottom, but
 * in both cases the linker places the nocache section within the valid RAM and
 * these entries follow it. The chain covers cacheable RAM below the region
 * (entry 0), the noncacheable window itself (entry 1), and the remaining
 * cacheable space up to the peripheral range (entry 2).
 */
static void configure_nocache_region(void)
{
	const uintptr_t nocache_start = (uintptr_t)_nocache_ram_start;
	const uintptr_t nocache_end = (uintptr_t)_nocache_ram_end;

	/* The region is aligned to the L2 cache line size by the linker, so any
	 * cached copies of it can be dropped a whole line at a time.
	 */
	sys_cache_data_flush_and_invd_range(_nocache_ram_start, (size_t)_nocache_ram_size);

	PMA_RESET_AND_ENTRY_SET_TOR(0, nocache_start, PMA_L | PMA_EN | PMA_TOR | PMA_R | PMA_W | PMA_X);
	PMA_RESET_AND_ENTRY_SET_TOR(1, nocache_end,
				    PMA_L | PMA_TOR | PMA_EN | PMA_R | PMA_W | PMA_X | PMA_NONCACHEABLE | PMA_WRITETHROUGH | PMA_WRITEMISSNOALLOC | PMA_READMISSNOALLOC);
	PMA_RESET_AND_ENTRY_SET_TOR(2, SOC_PERIPHERAL_LOW,
				    PMA_L | PMA_TOR | PMA_EN | PMA_R | PMA_W | PMA_X);

	/* Reset PMA entries that were set by the rom */
	PMA_ENTRY_CFG_RESET(15);
}
#endif

void IRAM_ATTR __esp_platform_app_start(void)
{
#ifdef CONFIG_FPU
	/* The Espressif boot path enters via __start, bypassing the
	 * generic riscv reset.S that initialises mstatus.FS. Enable
	 * the FPU here before any float instruction is executed.
	 */
	__asm__ volatile("li t0, %0\n"
			 "csrs mstatus, t0\n"
			 "fscsr zero\n" ::"i"(MSTATUS_FS_INIT)
			 : "t0");
#endif

	core_intr_matrix_clear();

	esp_reset_reason_init();

	esp_timer_early_init();

	esp_flash_config();

	esp_efuse_init_virtual();

#if CONFIG_ESP_SPIRAM
	esp_init_psram();

	int err = esp_psram_smh_init();

	if (err) {
		printk("Failed to initialize PSRAM shared multi heap (%d)\n", err);
	}
#endif

#if defined(CONFIG_NOCACHE_MEMORY)
	configure_nocache_region();
#endif

	z_cstart();

	CODE_UNREACHABLE;
}

void IRAM_ATTR __esp_platform_mcuboot_start(void)
{
	z_cstart();

	CODE_UNREACHABLE;
}

int IRAM_ATTR arch_printk_char_out(int c)
{
	if (c == '\n') {
		esp_rom_output_tx_one_char('\r');
	}
	esp_rom_output_tx_one_char(c);
	return 0;
}

void sys_arch_reboot(int type)
{
	esp_restart();
}
