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

/* brown out detection */
void system_BOD_init(void)
{
	uint32_t tmpVal = 0;

	/* disable BOD interrupt */
	tmpVal = sys_read32(HBN_BASE + HBN_IRQ_MODE_OFFSET);
	tmpVal &= ~HBN_IRQ_BOR_EN_MSK;
	sys_write32(tmpVal, HBN_BASE + HBN_IRQ_MODE_OFFSET);

	tmpVal = sys_read32(HBN_BASE + HBN_BOR_CFG_OFFSET);
	/* when brownout threshold, restart*/
	tmpVal |= HBN_BOD_SEL_MSK;
	/* set BOD threshold:
	 * 0:2.05v,1:2.10v,2:2.15v....7:2.4v
	 */
	tmpVal &= ~HBN_BOD_VTH_MSK;
	tmpVal |= (7 << HBN_BOD_VTH_POS);
	/* enable BOD */
	tmpVal |= HBN_PU_BOD_MSK;
	sys_write32(tmpVal, HBN_BASE + HBN_BOR_CFG_OFFSET);
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
	uint32_t tmpVal = 0;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		/* th.icache.iall */
		".insn 0x100000B\n"
	);
	__asm__ volatile(
		"csrr %0, 0x7C1"
		: "=r"(tmpVal));
	tmpVal |= (1 << 0);
	__asm__ volatile(
		"csrw 0x7C1, %0"
		:
		: "r"(tmpVal));
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
	);
}

static void enable_dcache(void)
{
	uint32_t tmpVal = 0;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		/* th.dcache.iall */
		".insn 0x20000B\n"
	);
	__asm__ volatile(
		"csrr %0, 0x7C1"
		: "=r"(tmpVal));
	tmpVal |= (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4);
	__asm__ volatile(
		"csrw 0x7C1, %0"
		:
		: "r"(tmpVal));
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
	);
}

static void enable_branchpred(bool yes)
{
	uint32_t tmpVal = 0;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
	);
	__asm__ volatile(
		"csrr %0, 0x7C1"
		: "=r"(tmpVal));
	if (yes) {
		tmpVal |= (1 << 5) | (1 << 12);
	} else {
		tmpVal &= ~((1 << 5) | (1 << 12));
	}
	__asm__ volatile(
		"csrw 0x7C1, %0"
		:
		: "r"(tmpVal));
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
	);
}

static void enable_thead_isa_ext(void)
{
	uint32_t tmpVal = 0;

	__asm__ volatile(
		"csrr %0, 0x7C0"
		: "=r"(tmpVal));
	tmpVal |= (1 << 22);
	__asm__ volatile(
		"csrw 0x7C0, %0"
		:
		: "r"(tmpVal));
}

static void set_thead_enforce_aligned(bool enable)
{
	uint32_t tmpVal = 0;

	__asm__ volatile(
		"csrr %0, 0x7C0"
		: "=r"(tmpVal));
	if (enable) {
		tmpVal &= ~(1 << 15);
	} else {
		tmpVal |= (1 << 15);
	}
	__asm__ volatile(
		"csrw 0x7C0, %0"
		:
		: "r"(tmpVal));
}

static void disable_interrupt_autostacking(void)
{
	uint32_t tmpVal = 0;

	__asm__ volatile(
		"csrr %0, 0x7E1"
		: "=r"(tmpVal));
	tmpVal &= ~(0x3 << 16);
	__asm__ volatile(
		"csrw 0x7E1, %0"
		:
		: "r"(tmpVal));
}


void soc_early_init_hook(void)
{
	uint32_t key;
	uint32_t tmp;

	key = irq_lock();


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
