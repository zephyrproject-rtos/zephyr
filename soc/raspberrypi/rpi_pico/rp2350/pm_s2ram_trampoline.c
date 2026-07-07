/*
 * Copyright (c) 2026 Gabriel Germano
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/linker/sections.h>

#include <pico/bootrom.h>
#include <boot/bootrom_constants.h>

#include <cmsis_core.h>

/* Raw POWMAN register addresses */
#define POWMAN_BASE_ADDR    0x40100000u
#define POWMAN_SCRATCH0     (*(volatile uint32_t *)(POWMAN_BASE_ADDR + 0xb0u))
#define POWMAN_BOOT0        (*(volatile uint32_t *)(POWMAN_BASE_ADDR + 0xd0u))
#define POWMAN_BOOT1        (*(volatile uint32_t *)(POWMAN_BASE_ADDR + 0xd4u))
#define POWMAN_BOOT2        (*(volatile uint32_t *)(POWMAN_BASE_ADDR + 0xd8u))
#define POWMAN_BOOT3        (*(volatile uint32_t *)(POWMAN_BASE_ADDR + 0xdcu))
#define POWMAN_BOOT2_ADDR   (POWMAN_BASE_ADDR + 0xd8u)

/* Boot vector format expected by the bootrom in POWMAN BOOT0..3 (RP2350 datasheet §5.2.3). */
#define S2RAM_BOOT_MAGIC     0xb007c0d3u
#define S2RAM_BOOT_MAGIC_NEG 0x4ff83f2du
#define S2RAM_MARKER         0x5be25a4du

/* Defined in arch/arm/core/cortex_m/pm_s2ram.S; not exported via a header. */
extern void arch_pm_s2ram_resume(void);

void rp2350_s2ram_resume_trampoline(void);

void pm_s2ram_mark_set(void)
{
	uint32_t entry = ((uint32_t)&rp2350_s2ram_resume_trampoline) | 1u;

	POWMAN_SCRATCH0 = S2RAM_MARKER;
	POWMAN_BOOT0 = S2RAM_BOOT_MAGIC;
	POWMAN_BOOT1 = entry ^ S2RAM_BOOT_MAGIC_NEG;
	POWMAN_BOOT2 = __get_MSP();
	POWMAN_BOOT3 = entry;
}

bool pm_s2ram_mark_check_and_clear(void)
{
	bool resuming = (POWMAN_SCRATCH0 == S2RAM_MARKER);

	POWMAN_SCRATCH0 = 0u;
	POWMAN_BOOT0 = 0u;
	POWMAN_BOOT1 = 0u;
	POWMAN_BOOT2 = 0u;
	POWMAN_BOOT3 = 0u;

	if (resuming) {
		/*
		 * This runs (from arch_pm_s2ram_resume) BEFORE z_arm_init_arch_hw_at_boot,
		 * so VTOR and the FPU are still at their post-cold-boot state.  The saved
		 * context is FP-active (CONTROL.SFPA=1); restoring it with the FPU
		 * disabled faults, and with VTOR unset that fault locks up.  Re-establish
		 * both before the context restore.
		 */
		extern char _vector_table[];

		SCB->VTOR = (uint32_t)_vector_table;
#if defined(CONFIG_FPU)
		SCB->CPACR |= (3UL << 20) | (3UL << 22);
		__DSB();
		__ISB();
#endif
	}

	return resuming;
}

/* ROM functions live at 0x0 (always accessible, even with XIP down). */
__ramfunc static void xip_reinit(void)
{
	rom_connect_internal_flash_fn connect =
		(rom_connect_internal_flash_fn)rom_func_lookup_inline(
			ROM_FUNC_CONNECT_INTERNAL_FLASH);
	rom_flash_exit_xip_fn exit_xip =
		(rom_flash_exit_xip_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_EXIT_XIP);
	rom_flash_enter_cmd_xip_fn enter_xip =
		(rom_flash_enter_cmd_xip_fn)rom_func_lookup_inline(
			ROM_FUNC_FLASH_ENTER_CMD_XIP);

	connect();
	exit_xip();
	enter_xip();
}

__ramfunc void z_rp2350_s2ram_resume_body(void)
{
	xip_reinit();

	arch_pm_s2ram_resume();

	SCB->AIRCR = (0x05FAu << SCB_AIRCR_VECTKEY_Pos) | SCB_AIRCR_SYSRESETREQ_Msk;
	__builtin_unreachable();
}

__ramfunc __attribute__((naked)) void rp2350_s2ram_resume_trampoline(void)
{
	__asm__ volatile(
		"movw r0, %[boot2_lo]\n\t"
		"movt r0, %[boot2_hi]\n\t"
		"ldr  r0, [r0]\n\t"
		"mov  sp, r0\n\t"
		"b    z_rp2350_s2ram_resume_body\n\t"
		:
		: [boot2_lo] "i" (POWMAN_BOOT2_ADDR & 0xffffu),
		  [boot2_hi] "i" ((POWMAN_BOOT2_ADDR >> 16) & 0xffffu)
	);
}
