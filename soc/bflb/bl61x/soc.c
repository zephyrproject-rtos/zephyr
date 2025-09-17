/*
 * Copyright (c) 2024-2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bouffalo Lab RISC-V MCU series initialization code
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>

#include <bflb_soc.h>
#include <glb_reg.h>
#include <hbn_reg.h>
#include <pds_reg.h>

#define SYSMAP_BASE 0xEFFFF000UL
#define SYSMAP_BASE_SHIFT        (12)
#define SYSMAP_ATTR_STRONG_ORDER BIT(4)
#define SYSMAP_ATTR_CACHE_ABLE   BIT(3)
#define SYSMAP_ATTR_BUFFER_ABLE  BIT(2)
#define SYSMAP_ADDR_OFFSET       0x0
#define SYSMAP_FLAGS_OFFSET      0x4
#define SYSMAP_ENTRY_OFFSET      0x8

/* Initialize memory regions */
void system_sysmap_init(void)
{
	uintptr_t sysmap_base = SYSMAP_BASE;

	/* 1. 0x00000000~0x62FC0000: Strong-Order, Non-Cacheable, Non-Bufferable */
	sys_write32(BL616_OCRAM_BUSREMAP_CACHEABLE_BASE >> SYSMAP_BASE_SHIFT,
		    (sysmap_base + SYSMAP_ADDR_OFFSET));
	sys_write32(SYSMAP_ATTR_STRONG_ORDER, (sysmap_base + SYSMAP_FLAGS_OFFSET));
	sysmap_base += SYSMAP_ENTRY_OFFSET;

	/* 2. ocram 0x62FC0000~0x63010000: Weak-Order, Cacheable, Bufferable */
	sys_write32(BL616_WRAM_BUSREMAP_CACHEABLE_BASE >> SYSMAP_BASE_SHIFT,
		    (sysmap_base + SYSMAP_ADDR_OFFSET));
	sys_write32(SYSMAP_ATTR_CACHE_ABLE | SYSMAP_ATTR_BUFFER_ABLE,
		    (sysmap_base + SYSMAP_FLAGS_OFFSET));
	sysmap_base += SYSMAP_ENTRY_OFFSET;

	/* 3. wram 0x63010000~0x63038000: Weak-Order, Cacheable, Bufferable */
	sys_write32(BL616_WRAM_BUSREMAP_CACHEABLE_END
		>> SYSMAP_BASE_SHIFT, (sysmap_base + SYSMAP_ADDR_OFFSET));
	sys_write32(SYSMAP_ATTR_CACHE_ABLE | SYSMAP_ATTR_BUFFER_ABLE,
		    (sysmap_base + SYSMAP_FLAGS_OFFSET));
	sysmap_base += SYSMAP_ENTRY_OFFSET;

	/* 4. rom/empty 0x63038000~0xA0000000: Strong-Order, Non-Cacheable, Non-Bufferable */
	sys_write32(BL616_FLASH_XIP_BASE >> SYSMAP_BASE_SHIFT, (sysmap_base + SYSMAP_ADDR_OFFSET));
	sys_write32(SYSMAP_ATTR_STRONG_ORDER, (sysmap_base + SYSMAP_FLAGS_OFFSET));
	sysmap_base += SYSMAP_ENTRY_OFFSET;

	/* 5. flash(2x32M) 0xA0000000~0xA4000000: Weak-Order, Cacheable, Non-Bufferable */
	sys_write32(BL616_FLASH_XIP_BUSREMAP_END >> SYSMAP_BASE_SHIFT,
		    (sysmap_base + SYSMAP_ADDR_OFFSET));
	sys_write32(SYSMAP_ATTR_CACHE_ABLE, (sysmap_base + SYSMAP_FLAGS_OFFSET));
	sysmap_base += SYSMAP_ENTRY_OFFSET;

	/* 6. empty 0xA2000000~0xA8000000: Strong-Order, Non-Cacheable, Non-Bufferable */
	sys_write32(BL616_PSRAM_BUSREMAP_BASE >> SYSMAP_BASE_SHIFT,
		    (sysmap_base + SYSMAP_ADDR_OFFSET));
	sys_write32(SYSMAP_ATTR_STRONG_ORDER, (sysmap_base + SYSMAP_FLAGS_OFFSET));
	sysmap_base += SYSMAP_ENTRY_OFFSET;

	/* 7. psram(128M (4M)) 0xA8000000~0xB0000000(0xA8400000):
	 * Weak-Order, Cacheable, Bufferable
	 */
	sys_write32(BL616_PSRAM_BUSREMAP_END >> SYSMAP_BASE_SHIFT,
		    (sysmap_base + SYSMAP_ADDR_OFFSET));
	sys_write32(SYSMAP_ATTR_CACHE_ABLE | SYSMAP_ATTR_BUFFER_ABLE,
		    (sysmap_base + SYSMAP_FLAGS_OFFSET));
	sysmap_base += SYSMAP_ENTRY_OFFSET;

	/* 8. others: Strong-Order, Non-Cacheable, Non-Bufferable */
	sys_write32(0xFFFFF000U >> SYSMAP_BASE_SHIFT, (sysmap_base + SYSMAP_ADDR_OFFSET));
	sys_write32(SYSMAP_ATTR_STRONG_ORDER, (sysmap_base + SYSMAP_FLAGS_OFFSET));
}

/* brown out detection */
void system_BOD_init(void)
{
	uint32_t tmp;

	/* disable BOD interrupt */
	tmp = sys_read32(HBN_BASE + HBN_IRQ_MODE_OFFSET);
	tmp &= ~HBN_IRQ_BOR_EN_MSK;
	sys_write32(tmp, HBN_BASE + HBN_IRQ_MODE_OFFSET);

	tmp = sys_read32(HBN_BASE + HBN_BOR_CFG_OFFSET);
	/* when brownout threshold, restart*/
	tmp |= HBN_BOD_SEL_MSK;
	/* set BOD threshold:
	 * 0:2.05v,1:2.10v,2:2.15v....7:2.4v
	 */
	tmp &= ~HBN_BOD_VTH_MSK;
	tmp |= (7 << HBN_BOD_VTH_POS);
	/* enable BOD */
	tmp |= HBN_PU_BOD_MSK;
	sys_write32(tmp, HBN_BASE + HBN_BOR_CFG_OFFSET);
}

static void clean_dcache(void)
{
	__asm__ volatile (
		"fence\n"
		/* th.dcache.call*/
		".insn 0x10000B\n"
		"fence\n"
	);
}

static void clean_icache(void)
{
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		/* th.icache.iall */
		".insn 0x100000B\n"
		"fence\n"
		"fence.i\n"
	);
}

static void enable_icache(void)
{
	uint32_t tmp;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		/* th.icache.iall */
		".insn 0x100000B\n"
	);
	__asm__ volatile(
		"csrr %0, 0x7C1"
		: "=r"(tmp));
	tmp |= (1 << 0);
	__asm__ volatile(
		"csrw 0x7C1, %0"
		:
		: "r"(tmp));
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
	);
}

static void enable_dcache(void)
{
	uint32_t tmp;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		/* th.dcache.iall */
		".insn 0x20000B\n"
	);
	__asm__ volatile(
		"csrr %0, 0x7C1"
		: "=r"(tmp));
	tmp |= (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4);
	__asm__ volatile(
		"csrw 0x7C1, %0"
		:
		: "r"(tmp));
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
	);
}

static void enable_branchpred(bool yes)
{
	uint32_t tmp;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
	);
	__asm__ volatile(
		"csrr %0, 0x7C1"
		: "=r"(tmp));
	if (yes) {
		tmp |= (1 << 5) | (1 << 12);
	} else {
		tmp &= ~((1 << 5) | (1 << 12));
	}
	__asm__ volatile(
		"csrw 0x7C1, %0"
		:
		: "r"(tmp));
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
	);
}

static void enable_thead_isa_ext(void)
{
	uint32_t tmp;

	__asm__ volatile(
		"csrr %0, 0x7C0"
		: "=r"(tmp));
	tmp |= (1 << 22);
	__asm__ volatile(
		"csrw 0x7C0, %0"
		:
		: "r"(tmp));
}

static void set_thead_enforce_aligned(bool enable)
{
	uint32_t tmp;

	__asm__ volatile(
		"csrr %0, 0x7C0"
		: "=r"(tmp));
	if (enable) {
		tmp &= ~(1 << 15);
	} else {
		tmp |= (1 << 15);
	}
	__asm__ volatile(
		"csrw 0x7C0, %0"
		:
		: "r"(tmp));
}

static void disable_interrupt_autostacking(void)
{
	uint32_t tmp;

	__asm__ volatile(
		"csrr %0, 0x7E1"
		: "=r"(tmp));
	tmp &= ~(0x3 << 16);
	__asm__ volatile(
		"csrw 0x7E1, %0"
		:
		: "r"(tmp));
}


void soc_early_init_hook(void)
{
	uint32_t key;
	uint32_t tmp;

	key = irq_lock();

	system_sysmap_init();

	/* turn off USB power */
	sys_write32((1 << 5), PDS_BASE + PDS_USB_CTL_OFFSET);
	sys_write32(0, PDS_BASE + PDS_USB_PHY_CTRL_OFFSET);

	enable_thead_isa_ext();
	set_thead_enforce_aligned(false);
	enable_dcache();
	/* branch prediction can cause major slowdowns (250ms -> 2 seconds)
	 * in some applications
	 */
	enable_branchpred(true);
	enable_icache();
	disable_interrupt_autostacking();
	clean_dcache();
	clean_icache();

	/* reset uart signals */
	sys_write32(0xffffffffU, GLB_BASE + GLB_UART_CFG1_OFFSET);
	sys_write32(0x0000ffffU, GLB_BASE + GLB_UART_CFG2_OFFSET);

	/* reset wrongful AON control set by Bootrom on BL618 */
	tmp = sys_read32(HBN_BASE + HBN_PAD_CTRL_0_OFFSET);
	tmp &= ~HBN_REG_EN_AON_CTRL_GPIO_MSK;
	sys_write32(tmp, HBN_BASE + HBN_PAD_CTRL_0_OFFSET);

	/* TODO: 'em' config for ble goes here */

	system_BOD_init();

	irq_unlock(key);
}
