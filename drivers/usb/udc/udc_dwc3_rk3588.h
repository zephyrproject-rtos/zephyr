/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC3_RK3588_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC3_RK3588_H

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/*
 * USB 2.0 PHY of the RK3588 USB 3.0 OTG controllers, an Innosilicon PHY
 * controlled through its own GRF, programmed as the Linux
 * phy-rockchip-inno-usb2 driver does for it. Register offsets are relative
 * to the GRF syscon the PHY node sits in; the upper half of each register
 * is the write enable mask of the lower half.
 */
#define RK3588_U2PHY_GRF_CON0 0x0000
#define RK3588_U2PHY_GRF_CON1 0x0004
#define RK3588_U2PHY_GRF_CON2 0x0008
#define RK3588_U2PHY_GRF_CON3 0x000c
#define RK3588_U2PHY_GRF_CON4 0x0010

/* CON0: 480 MHz clock output, 1 turns it off */
#define RK3588_U2PHY_CLKOUT_OFF_BIT 0
/* CON1: HS DC voltage level adjustment */
#define RK3588_U2PHY_HS_DC_LVL_HI   11
#define RK3588_U2PHY_HS_DC_LVL_LO   8
/* CON2: SIDDQ powers the analog block down, and HS pre-emphasis current */
#define RK3588_U2PHY_SIDDQ_BIT      13
#define RK3588_U2PHY_PREEMPH_HI     4
#define RK3588_U2PHY_PREEMPH_LO     3
/* CON3: PHY suspend, and the UTMI configuration applied while suspended */
#define RK3588_U2PHY_SUS_BIT        11
#define RK3588_U2PHY_SUS_CFG_HI     4
#define RK3588_U2PHY_SUS_CFG_LO     0
/* CON4: pull-up of the ID pin */
#define RK3588_U2PHY_IDDIG_PU_HI    1
#define RK3588_U2PHY_IDDIG_PU_LO    0

/* Suspend UTMI configuration: FS terminations and transceiver, opmode */
#define RK3588_U2PHY_SUS_CFG_FS        0x14
#define RK3588_U2PHY_SUS_CFG_NODRIVING 0x01

struct dwc3_rk3588_u2phy {
	const struct device *grf;
	struct reset_dt_spec phy_rst;
	struct dwc3_clk phyclk;
	/* PHY of a USB 3.0 OTG controller rather than of a USB 2.0 host */
	bool usb3otg;
};

static int rk3588_u2phy_write(const struct dwc3_rk3588_u2phy *phy, uint16_t reg, uint8_t hi,
			      uint8_t lo, uint16_t val)
{
	uint32_t mask = GENMASK(hi, lo);

	return syscon_write_reg(phy->grf, reg, (mask << 16) | ((val << lo) & mask));
}

static int rk3588_u2phy_reset(const struct dwc3_rk3588_u2phy *phy)
{
	int ret;

	if (phy->phy_rst.dev == NULL) {
		return 0;
	}

	ret = reset_line_assert_dt(&phy->phy_rst);
	if (ret != 0) {
		return ret;
	}

	k_busy_wait(10);

	ret = reset_line_deassert_dt(&phy->phy_rst);
	if (ret != 0) {
		return ret;
	}

	k_busy_wait(200);

	return 0;
}

static int dwc3_rk3588_phy_init(const struct device *dev)
{
	const struct dwc3_rk3588_u2phy *phy = DEV_CFG(dev)->quirk_data;
	uint16_t sus_cfg = RK3588_U2PHY_SUS_CFG_FS;
	int ret;

	if (!device_is_ready(phy->grf)) {
		return -ENODEV;
	}

	if (phy->phyclk.dev != NULL) {
		ret = clock_control_on(phy->phyclk.dev, phy->phyclk.subsys);
		if (ret != 0 && ret != -EALREADY) {
			return ret;
		}
	}

	/* Power the analog block on, then reset the PHY as it leaves IDDQ */
	ret = rk3588_u2phy_write(phy, RK3588_U2PHY_GRF_CON2, RK3588_U2PHY_SIDDQ_BIT,
				 RK3588_U2PHY_SIDDQ_BIT, 0);
	if (ret != 0) {
		return ret;
	}

	ret = rk3588_u2phy_reset(phy);
	if (ret != 0) {
		return ret;
	}

	/* The PHY of a USB 3.0 controller does not drive the line in suspend */
	if (phy->usb3otg) {
		sus_cfg |= RK3588_U2PHY_SUS_CFG_NODRIVING;
	}

	ret = rk3588_u2phy_write(phy, RK3588_U2PHY_GRF_CON3, RK3588_U2PHY_SUS_CFG_HI,
				 RK3588_U2PHY_SUS_CFG_LO, sus_cfg);
	/* HS DC voltage level +5.89 % */
	ret |= rk3588_u2phy_write(phy, RK3588_U2PHY_GRF_CON1, RK3588_U2PHY_HS_DC_LVL_HI,
				  RK3588_U2PHY_HS_DC_LVL_LO, 0x9);
	/* HS transmitter pre-emphasis current 2x */
	ret |= rk3588_u2phy_write(phy, RK3588_U2PHY_GRF_CON2, RK3588_U2PHY_PREEMPH_HI,
				  RK3588_U2PHY_PREEMPH_LO, 0x2);
	if (phy->usb3otg) {
		ret |= rk3588_u2phy_write(phy, RK3588_U2PHY_GRF_CON4, RK3588_U2PHY_IDDIG_PU_HI,
					  RK3588_U2PHY_IDDIG_PU_LO, 0x3);
	}

	return ret != 0 ? -EIO : 0;
}

static int dwc3_rk3588_phy_power_on(const struct device *dev)
{
	const struct dwc3_rk3588_u2phy *phy = DEV_CFG(dev)->quirk_data;
	uint32_t con0;
	int ret;

	/* Turn the 480 MHz clock output on and let it settle */
	ret = syscon_read_reg(phy->grf, RK3588_U2PHY_GRF_CON0, &con0);
	if (ret != 0) {
		return ret;
	}

	if (con0 & BIT(RK3588_U2PHY_CLKOUT_OFF_BIT)) {
		ret = rk3588_u2phy_write(phy, RK3588_U2PHY_GRF_CON0, RK3588_U2PHY_CLKOUT_OFF_BIT,
					 RK3588_U2PHY_CLKOUT_OFF_BIT, 0);
		if (ret != 0) {
			return ret;
		}

		k_busy_wait(1300);
	}

	ret = rk3588_u2phy_write(phy, RK3588_U2PHY_GRF_CON3, RK3588_U2PHY_SUS_BIT,
				 RK3588_U2PHY_SUS_BIT, 0);
	if (ret != 0) {
		return ret;
	}

	/*
	 * The PHY leaves suspend with its reference logic, bias and PLL powered
	 * down, so it is reset, then given time for the UTMI clock to settle.
	 */
	ret = rk3588_u2phy_reset(phy);
	if (ret != 0) {
		return ret;
	}

	k_msleep(2);

	return 0;
}

static int dwc3_rk3588_phy_power_off(const struct device *dev)
{
	const struct dwc3_rk3588_u2phy *phy = DEV_CFG(dev)->quirk_data;
	int ret;

	ret = rk3588_u2phy_write(phy, RK3588_U2PHY_GRF_CON3, RK3588_U2PHY_SUS_BIT,
				 RK3588_U2PHY_SUS_BIT, 1);
	if (ret != 0) {
		return ret;
	}

	return rk3588_u2phy_write(phy, RK3588_U2PHY_GRF_CON0, RK3588_U2PHY_CLKOUT_OFF_BIT,
				  RK3588_U2PHY_CLKOUT_OFF_BIT, 1);
}

static const struct dwc3_vendor_quirks dwc3_rk3588_quirks = {
	.phy_init = dwc3_rk3588_phy_init,
	.phy_power_on = dwc3_rk3588_phy_power_on,
	.phy_power_off = dwc3_rk3588_phy_power_off,
};

/* The USB 2.0 PHY port, "usb2-phy" in phys, or the first PHY without names */
#define DWC3_RK3588_U2PHY_PORT(n)                                                                  \
	COND_CODE_1(DT_INST_PROP_HAS_NAME(n, phys, usb2_phy),                                      \
		    (DT_INST_PHANDLE_BY_NAME(n, phys, usb2_phy)),                                  \
		    (DT_INST_PHANDLE_BY_IDX(n, phys, 0)))

#define DWC3_RK3588_U2PHY_RESET(node)                                                              \
	COND_CODE_1(DT_PROP_HAS_NAME(node, resets, phy),                                           \
		    (RESET_DT_SPEC_GET_BY_IDX(node, DT_PHA_ELEM_IDX_BY_NAME(node, resets, phy))),  \
		    ({0}))

#define DWC3_RK3588_U2PHY_CLK(node)                                                                \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_NAME(node, phyclk)),                        \
		.subsys = (clock_control_subsys_t)DT_CLOCKS_CELL_BY_NAME(node, phyclk, clkid),     \
	}

#define DWC3_RK3588_U2PHY_CLK_OR_NONE(node)                                                        \
	COND_CODE_1(DT_CLOCKS_HAS_NAME(node, phyclk), (DWC3_RK3588_U2PHY_CLK(node)), ({0}))

/* The PHYs at 0x0 and 0x4000 serve the USB 3.0 OTG controllers */
#define DWC3_RK3588_U2PHY_USB3OTG(node) (DT_REG_ADDR(node) == 0x0 || DT_REG_ADDR(node) == 0x4000)

#define DWC3_RK3588_U2PHY_DEFINE_NODE(n, node)                                                     \
	BUILD_ASSERT(DT_NODE_HAS_COMPAT(node, rockchip_rk3588_usb2phy),                            \
		     "RK3588 DWC3 needs a rockchip,rk3588-usb2phy USB 2.0 PHY");                   \
	static const struct dwc3_rk3588_u2phy dwc3_rk3588_u2phy_##n = {                            \
		.grf = DEVICE_DT_GET(DT_PARENT(node)),                                             \
		.phy_rst = DWC3_RK3588_U2PHY_RESET(node),                                          \
		.phyclk = DWC3_RK3588_U2PHY_CLK_OR_NONE(node),                                     \
		.usb3otg = DWC3_RK3588_U2PHY_USB3OTG(node),                                        \
	};

#define DWC3_RK3588_U2PHY_DEFINE(n)                                                                \
	COND_CODE_1(DT_NODE_HAS_COMPAT(DT_DRV_INST(n), rockchip_rk3588_dwc3),                      \
		    (DWC3_RK3588_U2PHY_DEFINE_NODE(n, DT_PARENT(DWC3_RK3588_U2PHY_PORT(n)))), ())

DT_INST_FOREACH_STATUS_OKAY(DWC3_RK3588_U2PHY_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC3_RK3588_H */
