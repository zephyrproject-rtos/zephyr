/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UHC_UHC_MUSB_MCHP_USBHS_G1_H
#define ZEPHYR_DRIVERS_USB_UHC_UHC_MUSB_MCHP_USBHS_G1_H

/*
 * Microchip USBHS G1 glue for the MUSB host controller driver.
 *
 * The USBHS wrapper in front of the MUSB core provides the controller reset
 * and enable, the ID override that forces the host (A-device) role, the
 * wrapper interrupt flags and the USBHPHY control registers.
 */

#include <soc.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>

/*
 * USBHS wrapper and PHY registers are 32-bit MMIO registers. These helpers
 * provide explicit access width and make read-modify-write operations clear.
 */
static inline uint32_t mchp_usbhs_g1_read32(const volatile uint32_t *reg)
{
	return sys_read32((mem_addr_t)reg);
}

static inline void mchp_usbhs_g1_write32(volatile uint32_t *reg, uint32_t value)
{
	sys_write32(value, (mem_addr_t)reg);
}

static inline void mchp_usbhs_g1_set32(volatile uint32_t *reg, uint32_t bits)
{
	mchp_usbhs_g1_write32(reg, mchp_usbhs_g1_read32(reg) | bits);
}

static inline void mchp_usbhs_g1_clear32(volatile uint32_t *reg, uint32_t bits)
{
	mchp_usbhs_g1_write32(reg, mchp_usbhs_g1_read32(reg) & ~bits);
}

/* Replace the bits in mask with value using a single register write. */
static inline void mchp_usbhs_g1_update32(volatile uint32_t *reg, uint32_t mask, uint32_t value)
{
	mchp_usbhs_g1_write32(reg, (mchp_usbhs_g1_read32(reg) & ~mask) | value);
}

/*
 * USBHPHY characterization calibration values (CALOTP FCCFG67 fuse map):
 *
 * Function               Fuse       Size Offset Value  Register/Bit
 * Squelch                SQUELCH    4d   0d     0x2    PHY04.SQUELCH210 / PHY08.SQUELCH3
 * HS amp tuning          TUNE       8d   4d     0x57   PHY0C.TUNE210 / PHY10.TUNE76543
 * On die term comp       ODT        3d   12d    0x3    PHY14.ODT0 / PHY18.ODT21
 * HS slew rate           HSSLEW     3d   15d    0x2    PHY20.HSSLEW10 / PHY24.HSSLEW2
 * Host disconnect detect DISCONDET  4d   18d    0xD    PHY28.DISCONDET
 * HS drive current comp  HSDRVCOMP  3d   22d    0x3    PHY28.HSDRVCOMP
 *
 * The host role programs these fixed values; the nvm-phy-calib fuse is not
 * read in host mode.
 */
#define MCHP_USBHS_G1_PHY_CAL_SQUELCH   0x2U
#define MCHP_USBHS_G1_PHY_CAL_TUNE      0x57U
#define MCHP_USBHS_G1_PHY_CAL_ODT       0x3U
#define MCHP_USBHS_G1_PHY_CAL_HSSLEW    0x2U
#define MCHP_USBHS_G1_PHY_CAL_DISCONDET 0xDU
#define MCHP_USBHS_G1_PHY_CAL_HSDRVCOMP 0x3U

/* PHY24.OTGOFF: powers off the OTG comparators */
#define MCHP_USBHS_G1_PHY24_OTGOFF BIT(1)

/* Timeout loop counts (1 us per iteration) */
#define MCHP_USBHS_G1_SYNC_TIMEOUT_ITER      10000
#define MCHP_USBHS_G1_PHY_READY_TIMEOUT_ITER 100000

#define MCHP_USBHS_G1_AVREGEN_SETTLE_US 55U
#define MCHP_USBHS_G1_PHY_SETTLE_US     10U

static inline usbhs_registers_t *mchp_usbhs_g1_get_regs(const struct device *dev)
{
	const struct uhc_musb_config *const config = dev->config;

	return (usbhs_registers_t *)config->wrapper;
}

/* Program the USBHPHY tuning registers. PHY0x is accessible once PHYRDY is set. */
static inline void mchp_usbhs_g1_host_phy_calibrate(usbhs_registers_t *regs)
{
	/* SQUELCH: lower 3 bits -> PHY04, MSB (bit[3]) -> PHY08 */
	mchp_usbhs_g1_update32(&regs->ENDPOINT0.USBHS_PHY04, USBHS_PHY04_SQUELCH210_Msk,
			       USBHS_PHY04_SQUELCH210(MCHP_USBHS_G1_PHY_CAL_SQUELCH));
	mchp_usbhs_g1_update32(&regs->ENDPOINT0.USBHS_PHY08, USBHS_PHY08_SQUELCH3_Msk,
			       USBHS_PHY08_SQUELCH3(MCHP_USBHS_G1_PHY_CAL_SQUELCH >> 3));

	/* TUNE: lower 3 bits -> PHY0C, upper 5 bits (bits[7:3]) -> PHY10 */
	mchp_usbhs_g1_update32(&regs->ENDPOINT0.USBHS_PHY0C, USBHS_PHY0C_TUNE210_Msk,
			       USBHS_PHY0C_TUNE210(MCHP_USBHS_G1_PHY_CAL_TUNE));
	mchp_usbhs_g1_update32(&regs->ENDPOINT0.USBHS_PHY10, USBHS_PHY10_TUNE76543_Msk,
			       USBHS_PHY10_TUNE76543(MCHP_USBHS_G1_PHY_CAL_TUNE >> 3));

	/* ODT: bit[0] -> PHY14, bits[2:1] -> PHY18 */
	mchp_usbhs_g1_update32(&regs->ENDPOINT0.USBHS_PHY14, USBHS_PHY14_ODT0_Msk,
			       USBHS_PHY14_ODT0(MCHP_USBHS_G1_PHY_CAL_ODT));
	mchp_usbhs_g1_update32(&regs->ENDPOINT0.USBHS_PHY18, USBHS_PHY18_ODT21_Msk,
			       USBHS_PHY18_ODT21(MCHP_USBHS_G1_PHY_CAL_ODT >> 1));

	/* HSSLEW: bits[1:0] -> PHY20, bit[2] -> PHY24 */
	mchp_usbhs_g1_update32(&regs->ENDPOINT0.USBHS_PHY20, USBHS_PHY20_HSSLEW10_Msk,
			       USBHS_PHY20_HSSLEW10(MCHP_USBHS_G1_PHY_CAL_HSSLEW));
	mchp_usbhs_g1_update32(&regs->ENDPOINT0.USBHS_PHY24, USBHS_PHY24_HSSLEW2_Msk,
			       USBHS_PHY24_HSSLEW2(MCHP_USBHS_G1_PHY_CAL_HSSLEW >> 2));

	/* DISCONDET and HSDRVCOMP both live in PHY28 */
	mchp_usbhs_g1_update32(&regs->ENDPOINT0.USBHS_PHY28,
			       USBHS_PHY28_DISCONDET_Msk | USBHS_PHY28_HSDRVCOMP_Msk,
			       USBHS_PHY28_DISCONDET(MCHP_USBHS_G1_PHY_CAL_DISCONDET) |
			       USBHS_PHY28_HSDRVCOMP(MCHP_USBHS_G1_PHY_CAL_HSDRVCOMP));
}

static inline int mchp_usbhs_g1_host_init(const struct device *dev)
{
	usbhs_registers_t *const regs = mchp_usbhs_g1_get_regs(dev);
	int timeout = MCHP_USBHS_G1_SYNC_TIMEOUT_ITER;

	/* Enable the analog voltage regulator that powers the USB PHY. */
	mchp_usbhs_g1_set32(&SUPC_REGS->SUPC_VREGCTRL, SUPC_VREGCTRL_AVREGEN_Msk);
	k_usleep(MCHP_USBHS_G1_AVREGEN_SETTLE_US);

	/* Software reset of the controller. */
	mchp_usbhs_g1_write32(&regs->ENDPOINT0.USBHS_CTRLA, USBHS_CTRLA_SWRST_Msk);
	while (((mchp_usbhs_g1_read32(&regs->ENDPOINT0.USBHS_CTRLA) & USBHS_CTRLA_SWRST_Msk) ||
		(mchp_usbhs_g1_read32(&regs->ENDPOINT0.USBHS_SYNCBUSY) &
		 USBHS_SYNCBUSY_SWRST_Msk)) &&
	       timeout--) {
		k_usleep(1);
	}
	if (timeout <= 0) {
		LOG_ERR("USBHS reset timeout");
		return -ETIMEDOUT;
	}

	/*
	 * Force the ID input to the A-device (host) value so the core operates
	 * as a host regardless of the ID pin state.
	 */
	mchp_usbhs_g1_set32(&regs->ENDPOINT0.USBHS_CTRLA, USBHS_CTRLA_IDOVEN(1));
	mchp_usbhs_g1_clear32(&regs->ENDPOINT0.USBHS_CTRLA, USBHS_CTRLA_IDVAL_Msk);

	mchp_usbhs_g1_set32(&regs->ENDPOINT0.USBHS_CTRLA, USBHS_CTRLA_ENABLE_Msk);
	timeout = MCHP_USBHS_G1_SYNC_TIMEOUT_ITER;
	while ((mchp_usbhs_g1_read32(&regs->ENDPOINT0.USBHS_SYNCBUSY) &
		USBHS_SYNCBUSY_ENABLE_Msk) &&
	       timeout--) {
		k_usleep(1);
	}
	if (timeout <= 0) {
		LOG_ERR("USBHS enable sync timeout");
		return -ETIMEDOUT;
	}

	/* Wait for the PHY power-on to complete. */
	timeout = MCHP_USBHS_G1_PHY_READY_TIMEOUT_ITER;
	while ((mchp_usbhs_g1_read32(&regs->ENDPOINT0.USBHS_STATUS) &
		USBHS_STATUS_PHYON_Msk) == 0U) {
		if (timeout-- <= 0) {
			LOG_ERR("USBHS PHY power-on timeout");
			return -ETIMEDOUT;
		}
		k_usleep(1);
	}

	/* Wait for the PHY to be ready before touching MUSB registers. */
	timeout = MCHP_USBHS_G1_PHY_READY_TIMEOUT_ITER;
	while ((mchp_usbhs_g1_read32(&regs->ENDPOINT0.USBHS_STATUS) &
		USBHS_STATUS_PHYRDY_Msk) == 0U) {
		if (timeout-- <= 0) {
			LOG_ERR("USBHS PHY ready timeout");
			return -ETIMEDOUT;
		}
		k_usleep(1);
	}
	k_usleep(MCHP_USBHS_G1_PHY_SETTLE_US);

	mchp_usbhs_g1_host_phy_calibrate(regs);

	/*
	 * Power off the OTG comparators. Required in forced-host mode so that
	 * DEVCTL.VBUS reads VBUS_VALID and a host session can be started.
	 */
	mchp_usbhs_g1_set32(&regs->ENDPOINT0.USBHS_PHY24, MCHP_USBHS_G1_PHY24_OTGOFF);

	return 0;
}

static inline int mchp_usbhs_g1_host_post_enable(const struct device *dev)
{
	usbhs_registers_t *const regs = mchp_usbhs_g1_get_regs(dev);

	/* INTENSET is write-one-to-set: write the mask directly, never RMW it. */
	mchp_usbhs_g1_write32(&regs->ENDPOINT0.USBHS_INTENSET, USBHS_INTENSET_USB_Msk);

	return 0;
}

static inline int mchp_usbhs_g1_host_disable(const struct device *dev)
{
	usbhs_registers_t *const regs = mchp_usbhs_g1_get_regs(dev);

	/* INTENCLR is write-one-to-clear: write the mask directly, never RMW it. */
	mchp_usbhs_g1_write32(&regs->ENDPOINT0.USBHS_INTENCLR, USBHS_INTENCLR_USB_Msk);

	return 0;
}

static inline int mchp_usbhs_g1_host_shutdown(const struct device *dev)
{
	usbhs_registers_t *const regs = mchp_usbhs_g1_get_regs(dev);

	mchp_usbhs_g1_clear32(&regs->ENDPOINT0.USBHS_CTRLA, USBHS_CTRLA_ENABLE_Msk);

	return 0;
}

/* Acknowledge the wrapper interrupt flags observed for this interrupt. */
static inline int mchp_usbhs_g1_host_irq_clear(const struct device *dev)
{
	usbhs_registers_t *const regs = mchp_usbhs_g1_get_regs(dev);
	const uint32_t intflag = mchp_usbhs_g1_read32(&regs->ENDPOINT0.USBHS_INTFLAG);

	/* INTFLAG is write-one-to-clear: acknowledge only flags actually observed. */
	if ((intflag & USBHS_INTFLAG_PHYRDY_Msk) != 0U) {
		mchp_usbhs_g1_write32(&regs->ENDPOINT0.USBHS_INTFLAG, USBHS_INTFLAG_PHYRDY_Msk);
		/* PHY ready is only needed during initialization. */
		mchp_usbhs_g1_write32(&regs->ENDPOINT0.USBHS_INTENCLR, USBHS_INTENCLR_PHYRDY_Msk);
	}

	if ((intflag & USBHS_INTFLAG_USB_Msk) != 0U) {
		mchp_usbhs_g1_write32(&regs->ENDPOINT0.USBHS_INTFLAG, USBHS_INTFLAG_USB_Msk);
	}

	return 0;
}

#define UHC_MUSB_QUIRK_MCHP_USBHS_G1_DEFINE(n)					\
	static const struct uhc_musb_vendor_quirks uhc_musb_vendor_quirks_##n = {\
		.init = mchp_usbhs_g1_host_init,				\
		.post_enable = mchp_usbhs_g1_host_post_enable,			\
		.disable = mchp_usbhs_g1_host_disable,				\
		.shutdown = mchp_usbhs_g1_host_shutdown,			\
		.irq_clear = mchp_usbhs_g1_host_irq_clear,			\
	};

DT_INST_FOREACH_STATUS_OKAY(UHC_MUSB_QUIRK_MCHP_USBHS_G1_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UHC_UHC_MUSB_MCHP_USBHS_G1_H */
