/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <soc_init.h>
#include <flash_init.h>
#include <esp_err.h>
#include <esp_private/cache_utils.h>
#include <esp_private/system_internal.h>
#include <esp_timer.h>
#include <efuse_virtual.h>
#include <esp_rom_serial_output.h>
#include <zephyr/init.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/arch/common/init.h>
#include <zephyr/arch/riscv/arch.h>
#include <hal/interrupt_clic_ll.h>
#include <riscv/interrupt.h>
#include <riscv/csr.h>
#include <soc/interrupts.h>
#include <psram.h>
#include <zephyr/sys/printk.h>
#include <zephyr/cache.h>

#if defined(CONFIG_NOCACHE_MEMORY)
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/util.h>
#include "cache.h"
#endif

extern void esp_reset_reason_init(void);
extern FUNC_NORETURN void z_cstart(void);

#if defined(CONFIG_NOCACHE_MEMORY)
void nocache_region_init(void)
{
	const uintptr_t start = (uintptr_t)_nocache_ram_start;
	const size_t used = (size_t)(uintptr_t)_nocache_ram_size;
	size_t size;
	uintptr_t napot_addr;
	uint32_t cfg;

	if (used == 0) {
		return;
	}

	/* A PMA NAPOT entry can only describe a power-of-two region that is
	 * aligned to its own size. The linker sizes the section to its content,
	 * so round the length up to the next power of two and make sure the
	 * base is aligned to it, otherwise the entry would leave part of the
	 * section cached and silently break DMA coherency.
	 */
	size = BIT(LOG2CEIL(used));

	__ASSERT((start & (size - 1)) == 0U,
		 "nocache region 0x%lx not aligned to its NAPOT size 0x%zx", start, size);

	napot_addr = (start | ((size >> 1) - 1)) >> PMA_SHIFT;
	cfg = PMA_NAPOT | PMA_EN | PMA_R | PMA_W | PMA_NONCACHEABLE;

	sys_cache_data_flush_and_invd_range(_nocache_ram_start, used);

	RV_WRITE_CSR(CSR_PMAADDR0 + NOCACHE_PMA_ENTRY, napot_addr);
	RV_WRITE_CSR(CSR_PMACFG0 + NOCACHE_PMA_ENTRY, cfg);
}
#endif /* CONFIG_NOCACHE_MEMORY */

static void core_intr_matrix_clear(void)
{
	for (int i = 0; i < ETS_MAX_INTR_SOURCE; i++) {
		interrupt_clic_ll_route(0, i, ETS_INVALID_INUM);
	}

	for (int i = 0; i < 32; i++) {
		esprv_int_set_vectored(i, true);
	}
}

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

#if defined(CONFIG_NOCACHE_MEMORY)
	nocache_region_init();
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
