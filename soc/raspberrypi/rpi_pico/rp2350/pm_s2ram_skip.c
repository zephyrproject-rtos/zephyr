/*
 * SPDX-FileCopyrightText: 2026 Gabriel Germano
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/arch/arm/irq.h>
#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/devicetree.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/sys_io.h>

#include <pico/bootrom.h>
#include <boot/bootrom_constants.h>
#include <pico/runtime_init.h>

#include <hardware/regs/powman.h>

#include <cmsis_core.h>
#include <cortex_m/exception.h>

#define POWMAN_BASE_ADDR     DT_REG_ADDR(DT_NODELABEL(powman))
#define POWMAN_SCRATCH0_ADDR (POWMAN_BASE_ADDR + POWMAN_SCRATCH0_OFFSET)
#define POWMAN_BOOT0_ADDR    (POWMAN_BASE_ADDR + POWMAN_BOOT0_OFFSET)
#define POWMAN_BOOT1_ADDR    (POWMAN_BASE_ADDR + POWMAN_BOOT1_OFFSET)
#define POWMAN_BOOT2_ADDR    (POWMAN_BASE_ADDR + POWMAN_BOOT2_OFFSET)
#define POWMAN_BOOT3_ADDR    (POWMAN_BASE_ADDR + POWMAN_BOOT3_OFFSET)

/* Boot vector format expected by the bootrom in POWMAN BOOT0..3 (datasheet section 5.2.3) */
#define S2RAM_BOOT_MAGIC     0xb007c0d3u
#define S2RAM_BOOT_MAGIC_NEG 0x4ff83f2du
#define S2RAM_MARKER         0x5be25a4du

extern void arch_pm_s2ram_resume(void);

void rp2350_s2ram_resume_trampoline(void);

void pm_s2ram_mark_set(void)
{
	uint32_t entry = ((uint32_t)&rp2350_s2ram_resume_trampoline) | 1u;

	sys_write32(S2RAM_MARKER, POWMAN_SCRATCH0_ADDR);
	sys_write32(S2RAM_BOOT_MAGIC, POWMAN_BOOT0_ADDR);
	sys_write32(entry ^ S2RAM_BOOT_MAGIC_NEG, POWMAN_BOOT1_ADDR);
	sys_write32(__get_MSP(), POWMAN_BOOT2_ADDR);
	sys_write32(entry, POWMAN_BOOT3_ADDR);
}

#if defined(CONFIG_SRAM_VECTOR_TABLE)
extern char _sram_vector_start[];
#define RP2350_ACTIVE_VECTOR_TABLE ((uint32_t)_sram_vector_start)
#else
extern char _vector_table[];
#define RP2350_ACTIVE_VECTOR_TABLE ((uint32_t)_vector_table)
#endif

bool pm_s2ram_mark_check_and_clear(void)
{
	bool resuming = (sys_read32(POWMAN_SCRATCH0_ADDR) == S2RAM_MARKER) &&
			(SCB->VTOR != RP2350_ACTIVE_VECTOR_TABLE);

	sys_write32(0u, POWMAN_SCRATCH0_ADDR);
	sys_write32(0u, POWMAN_BOOT0_ADDR);
	sys_write32(0u, POWMAN_BOOT1_ADDR);
	sys_write32(0u, POWMAN_BOOT2_ADDR);
	sys_write32(0u, POWMAN_BOOT3_ADDR);

	if (resuming) {
		/* SWCORE power-down resets VTOR/CPACR/IRQ priorities; restore them
		 * before the (FP-active) context restore or the kernel faults.
		 */
		SCB->VTOR = RP2350_ACTIVE_VECTOR_TABLE;
		runtime_init_per_core_enable_coprocessors();
		__DSB();
		__ISB();
		z_arm_exc_setup();
		z_arm_interrupt_init();
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
		(rom_flash_enter_cmd_xip_fn)rom_func_lookup_inline(ROM_FUNC_FLASH_ENTER_CMD_XIP);

	connect();
	exit_xip();
	enter_xip();
}

__ramfunc void z_rp2350_s2ram_resume_body(void)
{
	xip_reinit();

	arch_pm_s2ram_resume();

	SCB->AIRCR = (AIRCR_VECT_KEY_PERMIT_WRITE << SCB_AIRCR_VECTKEY_Pos) |
		     SCB_AIRCR_SYSRESETREQ_Msk;
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
		: [boot2_lo] "i"(POWMAN_BOOT2_ADDR & 0xffffu), [boot2_hi] "i"(
								       (POWMAN_BOOT2_ADDR >> 16) &
								       0xffffu));
}
