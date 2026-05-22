/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BCM2835 ARMC peripheral interrupt controller. Sits behind the GPU
 * cascade line of the parent BCM2836 ARM-local intc (BCM2710 / Pi
 * Zero 2 W). Aggregates 64 GPU IRQs (PEND1 + PEND2 banks) plus 8
 * "basic" ARMC IRQs into the cascade line.
 *
 * The decode quirks documented in the BCM2835 ARM Peripherals
 * datasheet ch. 7 are encapsulated here:
 *
 *   1. Bank-1/2 IRQs that have a "shortcut" in bank 0 set ONLY their
 *      shortcut bit. Their per-bank PEND register bit is suppressed
 *      AND the bank-0 bits 8/9 ("bank 1/2 has more pending") do NOT
 *      fire. The decode path must therefore check shortcuts before
 *      consulting PEND1/PEND2.
 *   2. Bank-0 bits 8/9 cannot be masked.
 *   3. The shortcut bits in the bank-0 enable/disable registers are
 *      ignored; the actual bank-1/2 enable register must be used.
 *
 * Called from soc/brcm/bcm2710/soc_irq.c via the
 * bcm2835_armctrl_ic_* entry points in
 * <zephyr/drivers/interrupt_controller/intc_bcm283x.h>.
 */

#define DT_DRV_COMPAT brcm_bcm2835_armctrl_ic

#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/intc_bcm283x.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

/* Register offsets from the ARMC base. */
#define ARMC_IRQ_BASIC_PENDING_OFF  0x00
#define ARMC_IRQ_PENDING1_OFF       0x04
#define ARMC_IRQ_PENDING2_OFF       0x08
#define ARMC_FIQ_CONTROL_OFF        0x0c
#define ARMC_ENABLE_IRQS_1_OFF      0x10
#define ARMC_ENABLE_IRQS_2_OFF      0x14
#define ARMC_ENABLE_BASIC_IRQS_OFF  0x18
#define ARMC_DISABLE_IRQS_1_OFF     0x1c
#define ARMC_DISABLE_IRQS_2_OFF     0x20
#define ARMC_DISABLE_BASIC_IRQS_OFF 0x24

/* IRQ_BASIC_PENDING register layout. */
#define BANK0_BASIC_MASK  0xffU       /* bits 0..7 = ARMC basic IRQs */
#define BANK1_PENDING_BIT BIT(8)      /* "bank 1 has more pending" */
#define BANK2_PENDING_BIT BIT(9)      /* "bank 2 has more pending" */
#define SHORTCUT1_MASK    0x00007c00U /* bits 10..14 -> bank 1 IRQs */
#define SHORTCUT2_MASK    0x001f8000U /* bits 15..20 -> bank 2 IRQs */
#define SHORTCUT_SHIFT    10
#define BANK0_VALID_MASK                                                                           \
	(BANK0_BASIC_MASK | BANK1_PENDING_BIT | BANK2_PENDING_BIT | SHORTCUT1_MASK | SHORTCUT2_MASK)

/*
 * Bank-0 shortcut bits 10..20 each map directly to one bank-1/bank-2
 * IRQ. Indices 0..4 hold the bank-1 IRQ numbers reachable via bits
 * 10..14; indices 5..10 the bank-2 IRQ numbers reachable via bits
 * 15..20.
 */
static const uint8_t shortcuts[] = {
	7,  9,  10, 18, 19,    /* SHORTCUT1 (bank 1) */
	21, 22, 23, 24, 25, 30 /* SHORTCUT2 (bank 2) */
};

#define ARMC_IRQ_ENC(bank, n) (BCM283X_ARMC_IRQ_BASE + ((bank) << 5) + (n))
#define ARMC_IRQ_BANK(irq)    (((irq) - BCM283X_ARMC_IRQ_BASE) >> 5)
#define ARMC_IRQ_BIT(irq)     (((irq) - BCM283X_ARMC_IRQ_BASE) & 0x1f)

static mm_reg_t armc_base;

static uintptr_t armc_enable_reg(unsigned int bank)
{
	static const uint32_t enable_off[3] = {
		ARMC_ENABLE_BASIC_IRQS_OFF,
		ARMC_ENABLE_IRQS_1_OFF,
		ARMC_ENABLE_IRQS_2_OFF,
	};

	return armc_base + enable_off[bank];
}

static uintptr_t armc_disable_reg(unsigned int bank)
{
	static const uint32_t disable_off[3] = {
		ARMC_DISABLE_BASIC_IRQS_OFF,
		ARMC_DISABLE_IRQS_1_OFF,
		ARMC_DISABLE_IRQS_2_OFF,
	};

	return armc_base + disable_off[bank];
}

void bcm2835_armctrl_ic_init(void)
{
	/* See the comment in bcm2836_l1_intc_init() on why we map here. */
	device_map(&armc_base, DT_INST_REG_ADDR(0), DT_INST_REG_SIZE(0), K_MEM_CACHE_NONE);

	/*
	 * Mask everything. The DISABLE_* registers are write-1-to-clear,
	 * not r/w masks -- there is no separate "mask" register on this
	 * controller.
	 */
	sys_write32(0xffffffffU, armc_base + ARMC_DISABLE_IRQS_1_OFF);
	sys_write32(0xffffffffU, armc_base + ARMC_DISABLE_IRQS_2_OFF);
	sys_write32(0xffU, armc_base + ARMC_DISABLE_BASIC_IRQS_OFF);

	/* Cancel any FIQ left enabled by the boot firmware. */
	sys_write32(0, armc_base + ARMC_FIQ_CONTROL_OFF);
}

void bcm2835_armctrl_ic_irq_enable(unsigned int irq)
{
	if (!BCM283X_IRQ_IS_ARMC(irq)) {
		return;
	}
	sys_write32(BIT(ARMC_IRQ_BIT(irq)), armc_enable_reg(ARMC_IRQ_BANK(irq)));
}

void bcm2835_armctrl_ic_irq_disable(unsigned int irq)
{
	if (!BCM283X_IRQ_IS_ARMC(irq)) {
		return;
	}
	sys_write32(BIT(ARMC_IRQ_BIT(irq)), armc_disable_reg(ARMC_IRQ_BANK(irq)));
}

int bcm2835_armctrl_ic_irq_is_enabled(unsigned int irq)
{
	if (!BCM283X_IRQ_IS_ARMC(irq)) {
		return 0;
	}
	return !!(sys_read32(armc_enable_reg(ARMC_IRQ_BANK(irq))) & BIT(ARMC_IRQ_BIT(irq)));
}

unsigned int bcm2835_armctrl_ic_irq_get_active(void)
{
	uint32_t basic;
	uint32_t p;

	basic = sys_read32(armc_base + ARMC_IRQ_BASIC_PENDING_OFF) & BANK0_VALID_MASK;
	if (basic == 0) {
		return CONFIG_NUM_IRQS;
	}
	if (basic & BANK0_BASIC_MASK) {
		return ARMC_IRQ_ENC(0, __builtin_ctz(basic & BANK0_BASIC_MASK));
	}
	if (basic & SHORTCUT1_MASK) {
		return ARMC_IRQ_ENC(1, shortcuts[__builtin_ctz(basic >> SHORTCUT_SHIFT)]);
	}
	if (basic & SHORTCUT2_MASK) {
		return ARMC_IRQ_ENC(2, shortcuts[__builtin_ctz(basic >> SHORTCUT_SHIFT)]);
	}
	if (basic & BANK1_PENDING_BIT) {
		p = sys_read32(armc_base + ARMC_IRQ_PENDING1_OFF);
		if (p) {
			return ARMC_IRQ_ENC(1, __builtin_ctz(p));
		}
	}
	if (basic & BANK2_PENDING_BIT) {
		p = sys_read32(armc_base + ARMC_IRQ_PENDING2_OFF);
		if (p) {
			return ARMC_IRQ_ENC(2, __builtin_ctz(p));
		}
	}
	return CONFIG_NUM_IRQS;
}
