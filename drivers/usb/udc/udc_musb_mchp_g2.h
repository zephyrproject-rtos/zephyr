/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_UDC_MUSB_MCHP_G2_H
#define ZEPHYR_DRIVERS_USB_UDC_UDC_MUSB_MCHP_G2_H

#include <soc.h>

#include <zephyr/kernel.h>

/*
 * USBHPHY characterization calibration values (CALOTP FCCFG67 fuse map):
 *
 * Function               Fuse       Value  Register/Bit
 * Squelch                SQUELCH    0x2    PHY04.SQUELCH210 / PHY08.SQUELCH3
 * HS amp tuning          TUNE       0x57   PHY0C.TUNE210 / PHY10.TUNE76543
 * On die term comp       ODT        0x3    PHY14.ODT0 / PHY18.ODT21
 * HS slew rate           HSSLEW     0x2    PHY20.HSSLEW10 / PHY24.HSSLEW2
 * Host disconnect detect DISCONDET  0xD    PHY28.DISCONDET
 * HS drive current comp  HSDRVCOMP  0x3    PHY28.HSDRVCOMP
 */
#define MCHP_G2_PHY_CAL_SQUELCH   0x2U
#define MCHP_G2_PHY_CAL_TUNE      0x57U
#define MCHP_G2_PHY_CAL_ODT       0x3U
#define MCHP_G2_PHY_CAL_HSSLEW    0x2U
#define MCHP_G2_PHY_CAL_DISCONDET 0xDU
#define MCHP_G2_PHY_CAL_HSDRVCOMP 0x3U

/* Timeout loop counts (1 us per iteration) */
#define MCHP_G2_SYNC_TIMEOUT_ITER      10000
#define MCHP_G2_PHY_READY_TIMEOUT_ITER 100000

#define MCHP_G2_AVREGEN_SETTLE_US 55U
#define MCHP_G2_PHY_SETTLE_US     10U

static inline usbhs_registers_t *mchp_g2_get_regs(const struct device *dev)
{
	const struct udc_musb_config *const config = dev->config;

	return (usbhs_registers_t *)config->wrapper;
}

static inline int mchp_g2_wait_swrst(usbhs_registers_t *regs)
{
	int timeout = MCHP_G2_SYNC_TIMEOUT_ITER;

	while ((regs->ENDPOINT0.USBHS_CTRLA & USBHS_CTRLA_SWRST_Msk) != 0U ||
	       (regs->ENDPOINT0.USBHS_SYNCBUSY & USBHS_SYNCBUSY_SWRST_Msk) != 0U) {
		if (--timeout == 0) {
			return -ETIMEDOUT;
		}

		k_usleep(1);
	}

	return 0;
}

static inline int mchp_g2_wait_enable_sync(usbhs_registers_t *regs)
{
	int timeout = MCHP_G2_SYNC_TIMEOUT_ITER;

	while ((regs->ENDPOINT0.USBHS_SYNCBUSY & USBHS_SYNCBUSY_ENABLE_Msk) != 0U) {
		if (--timeout == 0) {
			return -ETIMEDOUT;
		}

		k_usleep(1);
	}

	return 0;
}

static inline int mchp_g2_wait_phy_ready(usbhs_registers_t *regs)
{
	int timeout = MCHP_G2_PHY_READY_TIMEOUT_ITER;

	while ((regs->ENDPOINT0.USBHS_STATUS & USBHS_STATUS_PHYRDY_Msk) == 0U) {
		if (--timeout == 0) {
			return -ETIMEDOUT;
		}

		k_usleep(1);
	}

	return 0;
}

/* PHY0x registers are only accessible once STATUS.PHYRDY is set. */
static inline void mchp_g2_phy_calibrate(usbhs_registers_t *regs)
{
	regs->ENDPOINT0.USBHS_PHY04 = (regs->ENDPOINT0.USBHS_PHY04 & ~USBHS_PHY04_SQUELCH210_Msk) |
				      USBHS_PHY04_SQUELCH210(MCHP_G2_PHY_CAL_SQUELCH);
	regs->ENDPOINT0.USBHS_PHY08 = (regs->ENDPOINT0.USBHS_PHY08 & ~USBHS_PHY08_SQUELCH3_Msk) |
				      USBHS_PHY08_SQUELCH3(MCHP_G2_PHY_CAL_SQUELCH >> 3);

	regs->ENDPOINT0.USBHS_PHY0C = (regs->ENDPOINT0.USBHS_PHY0C & ~USBHS_PHY0C_TUNE210_Msk) |
				      USBHS_PHY0C_TUNE210(MCHP_G2_PHY_CAL_TUNE);
	regs->ENDPOINT0.USBHS_PHY10 = (regs->ENDPOINT0.USBHS_PHY10 & ~USBHS_PHY10_TUNE76543_Msk) |
				      USBHS_PHY10_TUNE76543(MCHP_G2_PHY_CAL_TUNE >> 3);

	regs->ENDPOINT0.USBHS_PHY14 = (regs->ENDPOINT0.USBHS_PHY14 & ~USBHS_PHY14_ODT0_Msk) |
				      USBHS_PHY14_ODT0(MCHP_G2_PHY_CAL_ODT);
	regs->ENDPOINT0.USBHS_PHY18 = (regs->ENDPOINT0.USBHS_PHY18 & ~USBHS_PHY18_ODT21_Msk) |
				      USBHS_PHY18_ODT21(MCHP_G2_PHY_CAL_ODT >> 1);

	regs->ENDPOINT0.USBHS_PHY20 = (regs->ENDPOINT0.USBHS_PHY20 & ~USBHS_PHY20_HSSLEW10_Msk) |
				      USBHS_PHY20_HSSLEW10(MCHP_G2_PHY_CAL_HSSLEW);
	regs->ENDPOINT0.USBHS_PHY24 = (regs->ENDPOINT0.USBHS_PHY24 & ~USBHS_PHY24_HSSLEW2_Msk) |
				      USBHS_PHY24_HSSLEW2(MCHP_G2_PHY_CAL_HSSLEW >> 2);

	regs->ENDPOINT0.USBHS_PHY28 = (regs->ENDPOINT0.USBHS_PHY28 &
				       ~(USBHS_PHY28_DISCONDET_Msk | USBHS_PHY28_HSDRVCOMP_Msk)) |
				      USBHS_PHY28_DISCONDET(MCHP_G2_PHY_CAL_DISCONDET) |
				      USBHS_PHY28_HSDRVCOMP(MCHP_G2_PHY_CAL_HSDRVCOMP);
}

static inline int mchp_g2_init(const struct device *dev)
{
	usbhs_registers_t *const regs = mchp_g2_get_regs(dev);
	int ret;

	/* Enable AVREGEN and wait for it to stabilise. */
	SUPC_REGS->SUPC_VREGCTRL |= SUPC_VREGCTRL_AVREGEN_Msk;
	k_usleep(MCHP_G2_AVREGEN_SETTLE_US);

	regs->ENDPOINT0.USBHS_CTRLA = USBHS_CTRLA_SWRST_Msk;
	ret = mchp_g2_wait_swrst(regs);
	if (ret != 0) {
		return ret;
	}

	/* Configure as B-device (peripheral) and enable the controller. */
	regs->ENDPOINT0.USBHS_CTRLA |= USBHS_CTRLA_IDOVEN(1) | USBHS_CTRLA_IDVAL(1);
	regs->ENDPOINT0.USBHS_CTRLA |= USBHS_CTRLA_ENABLE_Msk;
	ret = mchp_g2_wait_enable_sync(regs);
	if (ret != 0) {
		return ret;
	}

	regs->ENDPOINT0.USBHS_INTENSET = USBHS_INTENSET_PHYRDY_Msk;

	/* The MUSB core registers are accessible only after PHY ready. */
	ret = mchp_g2_wait_phy_ready(regs);
	if (ret != 0) {
		return ret;
	}

	mchp_g2_phy_calibrate(regs);

	/* Open the VBUS detection switch. */
	regs->ENDPOINT0.USBHS_PHY24 |= USBHS_PHY24_VBUSDETEN_Msk;

	return 0;
}

static inline int mchp_g2_post_enable(const struct device *dev)
{
	usbhs_registers_t *const regs = mchp_g2_get_regs(dev);

	regs->ENDPOINT0.USBHS_INTENSET = USBHS_INTENSET_USB_Msk;
	k_usleep(MCHP_G2_PHY_SETTLE_US);

	return 0;
}

static inline int mchp_g2_shutdown(const struct device *dev)
{
	usbhs_registers_t *const regs = mchp_g2_get_regs(dev);
	int ret;

	regs->ENDPOINT0.USBHS_INTENCLR = USBHS_INTENCLR_USB_Msk | USBHS_INTENCLR_PHYRDY_Msk;

	regs->ENDPOINT0.USBHS_CTRLA &= ~USBHS_CTRLA_ENABLE_Msk;
	ret = mchp_g2_wait_enable_sync(regs);
	if (ret != 0) {
		return ret;
	}

	regs->ENDPOINT0.USBHS_CTRLA = USBHS_CTRLA_SWRST_Msk;

	return mchp_g2_wait_swrst(regs);
}

/* Acknowledge wrapper interrupts; return non-zero if no core interrupt is pending. */
static inline int mchp_g2_irq_clear(const struct device *dev)
{
	usbhs_registers_t *const regs = mchp_g2_get_regs(dev);
	const uint32_t intflag = regs->ENDPOINT0.USBHS_INTFLAG;

	if ((intflag & USBHS_INTFLAG_PHYRDY_Msk) != 0U) {
		/* PHY ready is only needed during initialization. */
		regs->ENDPOINT0.USBHS_INTENCLR = USBHS_INTENCLR_PHYRDY_Msk;
	}

	regs->ENDPOINT0.USBHS_INTFLAG = intflag &
					(USBHS_INTFLAG_USB_Msk | USBHS_INTFLAG_PHYRDY_Msk);

	return ((intflag & USBHS_INTFLAG_USB_Msk) != 0U) ? 0 : -ENODATA;
}

#define QUIRK_MCHP_USB_G2_DEFINE(n)						\
	const struct musb_vendor_quirks musb_vendor_quirks_##n = {		\
		.init = mchp_g2_init,					\
		.post_enable = mchp_g2_post_enable,				\
		.shutdown = mchp_g2_shutdown,				\
		.irq_clear = mchp_g2_irq_clear,				\
	};

DT_INST_FOREACH_STATUS_OKAY(QUIRK_MCHP_USB_G2_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UDC_UDC_MUSB_MCHP_G2_H */
