/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_usbphy_pll

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>

#include <fsl_clock.h>
#include <fsl_modcon.h>
#include <fsl_reset.h>

#define USBPHY_PLL_RATE_HZ 480000000U

/*
 * The HAL derives the PLL multiplier from the usb0_phyclk root rate, so that
 * root is fixed here to the 24 MHz SXOSC rather than left to devicetree.
 */
static int usbphy_pll_on(const struct device *dev, clock_control_subsys_t sys)
{
	clock_root_config_t root_cfg = {
		.clockShutdown = false,
		.mux = (uint8_t)kCLOCK_USB0_PHYCLK_ClockRoot_SXOSC,
		.div = 1U,
		.sndDiv = 1U,
	};

	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	CLOCK_SetRootClock(kCLOCK_Root_COMM_usb0_phyclk, &root_cfg);

	if (!CLOCK_EnableUsbPhyPllClock()) {
		return -EIO;
	}

	/* The HAL only releases the PHY reset, not the controller's. */
	RESET_PeripheralReset(kModCon_COMM_USB0);

	return 0;
}

static int usbphy_pll_off(const struct device *dev, clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	CLOCK_DisableUsbPhyPllClock();

	return 0;
}

/* The shared USB drivers call set_rate() after on(); the PLL has one output. */
static int usbphy_pll_set_rate(const struct device *dev, clock_control_subsys_t sys,
			       clock_control_subsys_rate_t rate)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	if ((uint32_t)(uintptr_t)rate != USBPHY_PLL_RATE_HZ) {
		return -ENOTSUP;
	}

	return 0;
}

static int usbphy_pll_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	*rate = USBPHY_PLL_RATE_HZ;

	return 0;
}

static enum clock_control_status usbphy_pll_get_status(const struct device *dev,
						       clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return ((COMM__USBPHY->PLL_SIC & USBPHY_PLL_SIC_PLL_POWER_MASK) != 0U)
		       ? CLOCK_CONTROL_STATUS_ON
		       : CLOCK_CONTROL_STATUS_OFF;
}

static DEVICE_API(clock_control, usbphy_pll_api) = {
	.on = usbphy_pll_on,
	.off = usbphy_pll_off,
	.set_rate = usbphy_pll_set_rate,
	.get_rate = usbphy_pll_get_rate,
	.get_status = usbphy_pll_get_status,
};

#define USBPHY_PLL_INIT(n)                                                                         \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, PRE_KERNEL_1,                             \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &usbphy_pll_api);

DT_INST_FOREACH_STATUS_OKAY(USBPHY_PLL_INIT)
