/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_UDC_MUSB_MCHP_USBHS_G1_H
#define ZEPHYR_DRIVERS_USB_UDC_UDC_MUSB_MCHP_USBHS_G1_H

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

/*
 * USBHPHY characterization calibration default values.
 *
 * These are the fallback defaults used when the PHY calibration fuse is
 * unprogrammed (engineering sample or blank OTP). On production silicon the
 * values are read at runtime from the fuse; see mchp_usbhs_g1_calotp_read_phy_cal().
 */
#define MCHP_USBHS_G1_PHY_CAL_SQUELCH   0x2U
#define MCHP_USBHS_G1_PHY_CAL_TUNE      0x57U
#define MCHP_USBHS_G1_PHY_CAL_ODT       0x3U
#define MCHP_USBHS_G1_PHY_CAL_HSSLEW    0x2U
#define MCHP_USBHS_G1_PHY_CAL_DISCONDET 0xDU
#define MCHP_USBHS_G1_PHY_CAL_HSDRVCOMP 0x3U

/*
 * USBHPHY calibration fuse register bit field definitions.
 *
 * Bit layout:
 *   [3:0]   CFG_USB_PHY_SQUELCH   4-bit RX squelch threshold
 *   [11:4]  CFG_USB_PHY_TUNE      8-bit HS amplitude tuning
 *   [14:12] CFG_USB_PHY_ODT       3-bit on-die termination
 *   [17:15] CFG_USB_PHY_HSSLEW    3-bit HS slew rate
 *   [21:18] CFG_USB_PHY_DISCONDET 4-bit disconnect detection threshold
 *   [24:22] CFG_USB_PHY_HSDRVCOMP 3-bit HS drive current compensation
 *   [31:25] RESERVED
 *
 * An unprogrammed OTP word reads as 0x00000000 (erased) or 0xFFFFFFFF
 * (blank). Both values are treated as "no fuse data" and the compile-time
 * defaults above are used instead.
 *
 * The fuse address is specified in the device tree via the nvm-phy-calib
 * property and is controller-specific.
 */
/* --- Fuse field: position and mask defines --- */
#define MCHP_USBHS_G1_PHY_CAL_SQUELCH_Pos      0U
#define MCHP_USBHS_G1_PHY_CAL_SQUELCH_Msk      (0xFU  << MCHP_USBHS_G1_PHY_CAL_SQUELCH_Pos)
#define MCHP_USBHS_G1_PHY_CAL_TUNE_Pos         4U
#define MCHP_USBHS_G1_PHY_CAL_TUNE_Msk         (0xFFU << MCHP_USBHS_G1_PHY_CAL_TUNE_Pos)
#define MCHP_USBHS_G1_PHY_CAL_ODT_Pos          12U
#define MCHP_USBHS_G1_PHY_CAL_ODT_Msk          (0x7U  << MCHP_USBHS_G1_PHY_CAL_ODT_Pos)
#define MCHP_USBHS_G1_PHY_CAL_HSSLEW_Pos       15U
#define MCHP_USBHS_G1_PHY_CAL_HSSLEW_Msk       (0x7U  << MCHP_USBHS_G1_PHY_CAL_HSSLEW_Pos)
#define MCHP_USBHS_G1_PHY_CAL_DISCONDET_Pos    18U
#define MCHP_USBHS_G1_PHY_CAL_DISCONDET_Msk    (0xFU  << MCHP_USBHS_G1_PHY_CAL_DISCONDET_Pos)
#define MCHP_USBHS_G1_PHY_CAL_HSDRVCOMP_Pos    22U
#define MCHP_USBHS_G1_PHY_CAL_HSDRVCOMP_Msk    (0x7U  << MCHP_USBHS_G1_PHY_CAL_HSDRVCOMP_Pos)

/*
 * --- PHY calibration field extraction helpers ---
 *
 * Same style as mchp_usbhs_g1_read32() / mchp_usbhs_g1_write32(): static inline functions
 * rather than macros. Each function takes the raw 32-bit fuse word and
 * returns the extracted field value as uint8_t.
 */
static inline uint8_t mchp_usbhs_g1_phy_cal_squelch(uint32_t fuse)
{
	return (uint8_t)((fuse & MCHP_USBHS_G1_PHY_CAL_SQUELCH_Msk)
			 >> MCHP_USBHS_G1_PHY_CAL_SQUELCH_Pos);
}

static inline uint8_t mchp_usbhs_g1_phy_cal_tune(uint32_t fuse)
{
	return (uint8_t)((fuse & MCHP_USBHS_G1_PHY_CAL_TUNE_Msk)
			 >> MCHP_USBHS_G1_PHY_CAL_TUNE_Pos);
}

static inline uint8_t mchp_usbhs_g1_phy_cal_odt(uint32_t fuse)
{
	return (uint8_t)((fuse & MCHP_USBHS_G1_PHY_CAL_ODT_Msk)
			 >> MCHP_USBHS_G1_PHY_CAL_ODT_Pos);
}

static inline uint8_t mchp_usbhs_g1_phy_cal_hsslew(uint32_t fuse)
{
	return (uint8_t)((fuse & MCHP_USBHS_G1_PHY_CAL_HSSLEW_Msk)
			 >> MCHP_USBHS_G1_PHY_CAL_HSSLEW_Pos);
}

static inline uint8_t mchp_usbhs_g1_phy_cal_discondet(uint32_t fuse)
{
	return (uint8_t)((fuse & MCHP_USBHS_G1_PHY_CAL_DISCONDET_Msk)
			 >> MCHP_USBHS_G1_PHY_CAL_DISCONDET_Pos);
}

static inline uint8_t mchp_usbhs_g1_phy_cal_hsdrvcomp(uint32_t fuse)
{
	return (uint8_t)((fuse & MCHP_USBHS_G1_PHY_CAL_HSDRVCOMP_Msk)
			 >> MCHP_USBHS_G1_PHY_CAL_HSDRVCOMP_Pos);
}

/* Sentinel values that indicate an unprogrammed OTP word. */
#define MCHP_USBHS_G1_CALOTP_ERASED             0x00000000UL
#define MCHP_USBHS_G1_CALOTP_BLANK              0xFFFFFFFFUL

/*
 * struct mchp_usbhs_g1_phy_cal - holds the six PHY calibration values.
 *
 * Populated either from the CALOTP fuse (production silicon) or from the
 * compile-time defaults above (engineering sample / blank OTP).
 */
struct mchp_usbhs_g1_phy_cal {
	uint8_t squelch;    /* 4-bit: PHY04.SQUELCH210 / PHY08.SQUELCH3       */
	uint8_t tune;       /* 8-bit: PHY0C.TUNE210    / PHY10.TUNE76543      */
	uint8_t odt;        /* 3-bit: PHY14.ODT0       / PHY18.ODT21          */
	uint8_t hsslew;     /* 3-bit: PHY20.HSSLEW10   / PHY24.HSSLEW2        */
	uint8_t discondet;  /* 4-bit: PHY28.DISCONDET                         */
	uint8_t hsdrvcomp;  /* 3-bit: PHY28.HSDRVCOMP                         */
};

static inline void mchp_usbhs_g1_phy_cal_defaults(struct mchp_usbhs_g1_phy_cal *cal)
{
	cal->squelch   = MCHP_USBHS_G1_PHY_CAL_SQUELCH;
	cal->tune      = MCHP_USBHS_G1_PHY_CAL_TUNE;
	cal->odt       = MCHP_USBHS_G1_PHY_CAL_ODT;
	cal->hsslew    = MCHP_USBHS_G1_PHY_CAL_HSSLEW;
	cal->discondet = MCHP_USBHS_G1_PHY_CAL_DISCONDET;
	cal->hsdrvcomp = MCHP_USBHS_G1_PHY_CAL_HSDRVCOMP;
}

/* Read calibration from the fuse, or use defaults if unavailable/unprogrammed. */
static inline void mchp_usbhs_g1_calotp_read_phy_cal(uintptr_t fuse_addr,
					       struct mchp_usbhs_g1_phy_cal *cal)
{
#if defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	/* Non-secure images cannot access the calibration fuse. */
	mchp_usbhs_g1_phy_cal_defaults(cal);
#else
	uint32_t fuse;

	/* No DT address means no fuse data; never issue an MMIO read at address 0. */
	if (fuse_addr == 0U) {
		mchp_usbhs_g1_phy_cal_defaults(cal);
		return;
	}

	fuse = sys_read32(fuse_addr);

	if (fuse == MCHP_USBHS_G1_CALOTP_ERASED || fuse == MCHP_USBHS_G1_CALOTP_BLANK) {
		mchp_usbhs_g1_phy_cal_defaults(cal);
		return;
	}

	cal->squelch   = mchp_usbhs_g1_phy_cal_squelch(fuse);
	cal->tune      = mchp_usbhs_g1_phy_cal_tune(fuse);
	cal->odt       = mchp_usbhs_g1_phy_cal_odt(fuse);
	cal->hsslew    = mchp_usbhs_g1_phy_cal_hsslew(fuse);
	cal->discondet = mchp_usbhs_g1_phy_cal_discondet(fuse);
	cal->hsdrvcomp = mchp_usbhs_g1_phy_cal_hsdrvcomp(fuse);
#endif /* CONFIG_TRUSTED_EXECUTION_NONSECURE */
}

/* Timeout loop counts (1 us per iteration) */
#define MCHP_USBHS_G1_SYNC_TIMEOUT_ITER      10000
#define MCHP_USBHS_G1_PHY_READY_TIMEOUT_ITER 100000

#define MCHP_USBHS_G1_AVREGEN_SETTLE_US 55U
#define MCHP_USBHS_G1_PHY_SETTLE_US     10U

static inline usbhs_registers_t *mchp_usbhs_g1_get_regs(const struct device *dev)
{
	const struct udc_musb_config *const config = dev->config;

	return (usbhs_registers_t *)config->wrapper;
}

static inline int mchp_usbhs_g1_wait_swrst(usbhs_registers_t *regs)
{
	int timeout = MCHP_USBHS_G1_SYNC_TIMEOUT_ITER;

	while ((mchp_usbhs_g1_read32(&regs->ENDPOINT0.USBHS_CTRLA) & USBHS_CTRLA_SWRST_Msk) != 0U ||
	       (mchp_usbhs_g1_read32(&regs->ENDPOINT0.USBHS_SYNCBUSY) &
		USBHS_SYNCBUSY_SWRST_Msk) != 0U) {
		if (--timeout == 0) {
			return -ETIMEDOUT;
		}

		k_usleep(1);
	}

	return 0;
}

static inline int mchp_usbhs_g1_wait_enable_sync(usbhs_registers_t *regs)
{
	int timeout = MCHP_USBHS_G1_SYNC_TIMEOUT_ITER;

	while ((mchp_usbhs_g1_read32(&regs->ENDPOINT0.USBHS_SYNCBUSY) &
		USBHS_SYNCBUSY_ENABLE_Msk) != 0U) {
		if (--timeout == 0) {
			return -ETIMEDOUT;
		}

		k_usleep(1);
	}

	return 0;
}

static inline int mchp_usbhs_g1_wait_phy_ready(usbhs_registers_t *regs)
{
	int timeout = MCHP_USBHS_G1_PHY_READY_TIMEOUT_ITER;

	while ((mchp_usbhs_g1_read32(&regs->ENDPOINT0.USBHS_STATUS) &
		USBHS_STATUS_PHYRDY_Msk) == 0U) {
		if (--timeout == 0) {
			return -ETIMEDOUT;
		}

		k_usleep(1);
	}

	return 0;
}

/*
 * mchp_usbhs_g1_phy_calibrate() - program USBHPHY tuning registers.
 *
 * PHY0x registers are only accessible once STATUS.PHYRDY is set.
 *
 * Calibration values are read at runtime from the PHY calibration fuse.
 * If the fuse is unprogrammed (engineering sample or blank OTP) the
 * compile-time defaults defined above are used automatically.
 *
 * Each logical calibration value may be split across two adjacent PHY
 * registers because the PHY interface presents fields at arbitrary bit
 * positions across 32-bit words:
 *
 *   SQUELCH  (4-bit): lower 3 bits -> PHY04.SQUELCH210, bit[3] -> PHY08.SQUELCH3
 *   TUNE     (8-bit): lower 3 bits -> PHY0C.TUNE210,   bits[7:3] -> PHY10.TUNE76543
 *   ODT      (3-bit): bit[0]       -> PHY14.ODT0,      bits[2:1] -> PHY18.ODT21
 *   HSSLEW   (3-bit): bits[1:0]    -> PHY20.HSSLEW10,  bit[2]    -> PHY24.HSSLEW2
 *   DISCONDET(4-bit): PHY28.DISCONDET  (no split)
 *   HSDRVCOMP(3-bit): PHY28.HSDRVCOMP (no split, same register as DISCONDET)
 */
static inline void mchp_usbhs_g1_phy_calibrate(uintptr_t fuse_addr, usbhs_registers_t *regs)
{
	struct mchp_usbhs_g1_phy_cal cal;

	/*
	 * Read calibration from fuse. On engineering samples the
	 * fuse word is unprogrammed and fallback defaults are used instead.
	 */
	mchp_usbhs_g1_calotp_read_phy_cal(fuse_addr, &cal);

	/* SQUELCH: lower 3 bits -> PHY04, MSB (bit[3]) -> PHY08 */
	mchp_usbhs_g1_clear32(&regs->ENDPOINT0.USBHS_PHY04, USBHS_PHY04_SQUELCH210_Msk);
	mchp_usbhs_g1_set32(&regs->ENDPOINT0.USBHS_PHY04,
		      USBHS_PHY04_SQUELCH210(cal.squelch));

	mchp_usbhs_g1_clear32(&regs->ENDPOINT0.USBHS_PHY08, USBHS_PHY08_SQUELCH3_Msk);
	mchp_usbhs_g1_set32(&regs->ENDPOINT0.USBHS_PHY08,
		      USBHS_PHY08_SQUELCH3(cal.squelch >> 3));

	/* TUNE: lower 3 bits -> PHY0C, upper 5 bits (bits[7:3]) -> PHY10 */
	mchp_usbhs_g1_clear32(&regs->ENDPOINT0.USBHS_PHY0C, USBHS_PHY0C_TUNE210_Msk);
	mchp_usbhs_g1_set32(&regs->ENDPOINT0.USBHS_PHY0C,
		      USBHS_PHY0C_TUNE210(cal.tune));

	mchp_usbhs_g1_clear32(&regs->ENDPOINT0.USBHS_PHY10, USBHS_PHY10_TUNE76543_Msk);
	mchp_usbhs_g1_set32(&regs->ENDPOINT0.USBHS_PHY10,
		      USBHS_PHY10_TUNE76543(cal.tune >> 3));

	/* ODT: bit[0] -> PHY14, bits[2:1] -> PHY18 */
	mchp_usbhs_g1_clear32(&regs->ENDPOINT0.USBHS_PHY14, USBHS_PHY14_ODT0_Msk);
	mchp_usbhs_g1_set32(&regs->ENDPOINT0.USBHS_PHY14,
		      USBHS_PHY14_ODT0(cal.odt));

	mchp_usbhs_g1_clear32(&regs->ENDPOINT0.USBHS_PHY18, USBHS_PHY18_ODT21_Msk);
	mchp_usbhs_g1_set32(&regs->ENDPOINT0.USBHS_PHY18,
		      USBHS_PHY18_ODT21(cal.odt >> 1));

	/* HSSLEW: bits[1:0] -> PHY20, bit[2] -> PHY24 */
	mchp_usbhs_g1_clear32(&regs->ENDPOINT0.USBHS_PHY20, USBHS_PHY20_HSSLEW10_Msk);
	mchp_usbhs_g1_set32(&regs->ENDPOINT0.USBHS_PHY20,
		      USBHS_PHY20_HSSLEW10(cal.hsslew));

	mchp_usbhs_g1_clear32(&regs->ENDPOINT0.USBHS_PHY24, USBHS_PHY24_HSSLEW2_Msk);
	mchp_usbhs_g1_set32(&regs->ENDPOINT0.USBHS_PHY24,
		      USBHS_PHY24_HSSLEW2(cal.hsslew >> 2));

	/* DISCONDET and HSDRVCOMP both live in PHY28 — clear both then set */
	mchp_usbhs_g1_clear32(&regs->ENDPOINT0.USBHS_PHY28,
			USBHS_PHY28_DISCONDET_Msk | USBHS_PHY28_HSDRVCOMP_Msk);
	mchp_usbhs_g1_set32(&regs->ENDPOINT0.USBHS_PHY28,
		      USBHS_PHY28_DISCONDET(cal.discondet) |
		      USBHS_PHY28_HSDRVCOMP(cal.hsdrvcomp));
}

static inline int mchp_usbhs_g1_init_with_cal_addr(const struct device *dev, uintptr_t fuse_addr)
{
	usbhs_registers_t *const regs = mchp_usbhs_g1_get_regs(dev);
	int ret;

	/* Enable AVREGEN and wait for it to stabilise. */
	mchp_usbhs_g1_set32(&SUPC_REGS->SUPC_VREGCTRL, SUPC_VREGCTRL_AVREGEN_Msk);
	k_usleep(MCHP_USBHS_G1_AVREGEN_SETTLE_US);

	mchp_usbhs_g1_write32(&regs->ENDPOINT0.USBHS_CTRLA, USBHS_CTRLA_SWRST_Msk);
	ret = mchp_usbhs_g1_wait_swrst(regs);
	if (ret != 0) {
		return ret;
	}

	/* Configure as B-device (peripheral) and enable the controller. */
	mchp_usbhs_g1_set32(&regs->ENDPOINT0.USBHS_CTRLA,
		      USBHS_CTRLA_IDOVEN(1) | USBHS_CTRLA_IDVAL(1));
	mchp_usbhs_g1_set32(&regs->ENDPOINT0.USBHS_CTRLA, USBHS_CTRLA_ENABLE_Msk);
	ret = mchp_usbhs_g1_wait_enable_sync(regs);
	if (ret != 0) {
		return ret;
	}

	/* INTENSET is write-one-to-set: write the mask directly, never RMW it. */
	mchp_usbhs_g1_write32(&regs->ENDPOINT0.USBHS_INTENSET, USBHS_INTENSET_PHYRDY_Msk);

	/* The MUSB core registers are accessible only after PHY ready. */
	ret = mchp_usbhs_g1_wait_phy_ready(regs);
	if (ret != 0) {
		return ret;
	}

	mchp_usbhs_g1_phy_calibrate(fuse_addr, regs);

	/* Open the VBUS detection switch. */
	mchp_usbhs_g1_set32(&regs->ENDPOINT0.USBHS_PHY24, USBHS_PHY24_VBUSDETEN_Msk);

	return 0;
}

static inline int mchp_usbhs_g1_post_enable(const struct device *dev)
{
	usbhs_registers_t *const regs = mchp_usbhs_g1_get_regs(dev);

	/* INTENSET is write-one-to-set: write the mask directly, never RMW it. */
	mchp_usbhs_g1_write32(&regs->ENDPOINT0.USBHS_INTENSET, USBHS_INTENSET_USB_Msk);
	k_usleep(MCHP_USBHS_G1_PHY_SETTLE_US);

	return 0;
}

static inline int mchp_usbhs_g1_shutdown(const struct device *dev)
{
	usbhs_registers_t *const regs = mchp_usbhs_g1_get_regs(dev);
	int ret;

	/* INTENCLR is write-one-to-clear: write the mask directly, never RMW it. */
	mchp_usbhs_g1_write32(&regs->ENDPOINT0.USBHS_INTENCLR,
			USBHS_INTENCLR_USB_Msk | USBHS_INTENCLR_PHYRDY_Msk);

	mchp_usbhs_g1_clear32(&regs->ENDPOINT0.USBHS_CTRLA, USBHS_CTRLA_ENABLE_Msk);
	ret = mchp_usbhs_g1_wait_enable_sync(regs);
	if (ret != 0) {
		return ret;
	}

	mchp_usbhs_g1_write32(&regs->ENDPOINT0.USBHS_CTRLA, USBHS_CTRLA_SWRST_Msk);

	return mchp_usbhs_g1_wait_swrst(regs);
}

/* Acknowledge wrapper interrupts; return non-zero if no core interrupt is pending. */
static inline int mchp_usbhs_g1_irq_clear(const struct device *dev)
{
	usbhs_registers_t *const regs = mchp_usbhs_g1_get_regs(dev);
	const uint32_t intflag = mchp_usbhs_g1_read32(&regs->ENDPOINT0.USBHS_INTFLAG);

	if ((intflag & USBHS_INTFLAG_PHYRDY_Msk) != 0U) {
		/* PHY ready is only needed during initialization. */
		mchp_usbhs_g1_write32(&regs->ENDPOINT0.USBHS_INTENCLR, USBHS_INTENCLR_PHYRDY_Msk);
	}

	/* INTFLAG is write-one-to-clear: acknowledge only flags actually observed. */
	mchp_usbhs_g1_write32(&regs->ENDPOINT0.USBHS_INTFLAG,
			intflag & (USBHS_INTFLAG_USB_Msk | USBHS_INTFLAG_PHYRDY_Msk));

	return ((intflag & USBHS_INTFLAG_USB_Msk) != 0U) ? 0 : -ENODATA;
}

#define QUIRK_MCHP_USBHS_G1_DEFINE(n) \
	static inline int mchp_usbhs_g1_init_##n(const struct device *dev) \
	{ \
		return mchp_usbhs_g1_init_with_cal_addr(dev, \
			COND_CODE_1(DT_INST_NODE_HAS_PROP(n, nvm_phy_calib), \
				    (DT_REG_ADDR(DT_INST_PHANDLE(n, nvm_phy_calib))), \
				    (0U))); \
	} \
	\
	const struct musb_vendor_quirks musb_vendor_quirks_##n = { \
		.init = mchp_usbhs_g1_init_##n, \
		.post_enable = mchp_usbhs_g1_post_enable, \
		.shutdown = mchp_usbhs_g1_shutdown, \
		.irq_clear = mchp_usbhs_g1_irq_clear, \
	};

DT_INST_FOREACH_STATUS_OKAY(QUIRK_MCHP_USBHS_G1_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UDC_UDC_MUSB_MCHP_USBHS_G1_H */
