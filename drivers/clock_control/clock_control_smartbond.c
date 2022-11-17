/*
 * Copyright (c) 2022 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <soc.h>
#include <zephyr/sys/onoff.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <da1469x_clock.h>

LOG_MODULE_REGISTER(clock_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define DT_DRV_COMPAT smartbond_clock

static inline int smartbond_clock_control_on(const struct device *dev,
					     clock_control_subsys_t sub_system)
{
	const uint32_t *clk = (const uint32_t *)(sub_system);

	ARG_UNUSED(dev);

	switch (*clk) {
	case DT_DEP_ORD(DT_NODELABEL(rc32k)):
		CRG_TOP->CLK_RC32K_REG |= CRG_TOP_CLK_RC32K_REG_RC32K_ENABLE_Msk;
		break;
	case DT_DEP_ORD(DT_NODELABEL(rcx)):
		da1469x_clock_lp_rcx_enable();
		break;
	case DT_DEP_ORD(DT_NODELABEL(xtal32k)):
		da1469x_clock_lp_xtal32k_enable();
		break;
	case DT_DEP_ORD(DT_NODELABEL(rc32m)):
		CRG_TOP->CLK_RC32M_REG |= CRG_TOP_CLK_RC32M_REG_RC32M_ENABLE_Msk;
		break;
	case DT_DEP_ORD(DT_NODELABEL(xtal32m)):
		da1469x_clock_sys_xtal32m_init();
		da1469x_clock_sys_xtal32m_enable();
		break;
	case DT_DEP_ORD(DT_NODELABEL(pll)):
		if ((CRG_TOP->CLK_CTRL_REG & CRG_TOP_CLK_CTRL_REG_RUNNING_AT_PLL96M_Msk) == 0) {
			if ((CRG_TOP->CLK_CTRL_REG &
			     CRG_TOP_CLK_CTRL_REG_RUNNING_AT_XTAL32M_Msk) == 0) {
				da1469x_clock_sys_xtal32m_init();
				da1469x_clock_sys_xtal32m_enable();
				da1469x_clock_sys_xtal32m_wait_to_settle();
			}
			da1469x_clock_sys_pll_enable();
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline int smartbond_clock_control_off(const struct device *dev,
					      clock_control_subsys_t sub_system)
{
	const uint32_t *clk = (const uint32_t *)(sub_system);

	ARG_UNUSED(dev);

	switch (*clk) {
	case DT_DEP_ORD(DT_NODELABEL(rc32k)):
		if (((CRG_TOP->CLK_CTRL_REG & CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Msk) >>
			   CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Pos) != 0) {
			CRG_TOP->CLK_RC32K_REG &= ~CRG_TOP_CLK_RC32K_REG_RC32K_ENABLE_Msk;
		}
		break;
	case DT_DEP_ORD(DT_NODELABEL(rcx)):
		if (((CRG_TOP->CLK_CTRL_REG & CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Msk) >>
			   CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Pos) != 1) {
			CRG_TOP->CLK_RCX_REG &= ~CRG_TOP_CLK_RCX_REG_RCX_ENABLE_Msk;
		}
		break;
	case DT_DEP_ORD(DT_NODELABEL(xtal32k)):
		if (((CRG_TOP->CLK_CTRL_REG & CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Msk) >>
			CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Pos) > 1) {
			CRG_TOP->CLK_XTAL32K_REG &= ~CRG_TOP_CLK_XTAL32K_REG_XTAL32K_ENABLE_Msk;
		}
		break;
	case DT_DEP_ORD(DT_NODELABEL(rc32m)):
		/* Disable rc32m only if not used as system clock */
		if ((CRG_TOP->CLK_CTRL_REG & CRG_TOP_CLK_CTRL_REG_RUNNING_AT_RC32M_Msk) == 0) {
			da1469x_clock_sys_rc32m_disable();
		}
		break;
	case DT_DEP_ORD(DT_NODELABEL(xtal32m)):
		da1469x_clock_sys_xtal32m_init();
		da1469x_clock_sys_xtal32m_enable();
		break;
	case DT_DEP_ORD(DT_NODELABEL(pll)):
		da1469x_clock_sys_pll_disable();
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int smartbond_clock_control_get_rate(const struct device *dev,
					    clock_control_subsys_t sub_system,
					    uint32_t *rate)
{
	const uint32_t *clk = (const uint32_t *)(sub_system);

	switch (*clk) {
	case DT_DEP_ORD(DT_NODELABEL(rc32k)):
		*rate = da1469x_clock_lp_rc32m_freq_get();
		break;
	case DT_DEP_ORD(DT_NODELABEL(rcx)):
		*rate = da1469x_clock_lp_rcx_freq_get();
		break;
	case DT_DEP_ORD(DT_NODELABEL(xtal32k)):
		*rate = DT_PROP(DT_NODELABEL(xtal32k), clock_frequency);
		break;
	case DT_DEP_ORD(DT_NODELABEL(rc32m)):
		*rate = DT_PROP(DT_NODELABEL(rc32m), clock_frequency);
		break;
	case DT_DEP_ORD(DT_NODELABEL(xtal32m)):
		*rate = DT_PROP(DT_NODELABEL(xtal32m), clock_frequency);
		break;
	case DT_DEP_ORD(DT_NODELABEL(pll)):
		*rate = DT_PROP(DT_NODELABEL(pll), clock_frequency);
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static void smartbond_clock_control_on_by_ord(const struct device *dev,
					      uint32_t clock_id)
{
	smartbond_clock_control_on(dev, &clock_id);
}

static void smartbond_clock_control_off_by_ord(const struct device *dev,
					       uint32_t clock_id)
{
	smartbond_clock_control_off(dev, &clock_id);
}

/**
 * @brief Initialize clocks for the Smartbond
 *
 * This routine is called to enable and configure the clocks and PLL
 * of the soc on the board.
 *
 * @param dev clocks device struct
 *
 * @return 0
 */
int smartbond_clocks_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	uint32_t clk_ctrl;
	uint32_t sys_clk_id;

#define ENABLE_OSC(clock) smartbond_clock_control_on_by_ord(dev, DT_DEP_ORD(clock))
#define DISABLE_OSC(clock) if (DT_NODE_HAS_STATUS(clock, disabled)) { \
		smartbond_clock_control_off_by_ord(dev, DT_DEP_ORD(clock)); \
	}

	/* Enable all oscillators with status "okay" */
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(crg, osc), ENABLE_OSC, (;));

	clk_ctrl = CRG_TOP->CLK_CTRL_REG &
		   ~(CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Msk | CRG_TOP_CLK_CTRL_REG_SYS_CLK_SEL_Msk);

	/* Make sure that selected sysclock is enabled */
	BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_PROP(DT_NODELABEL(sys_clk), clock_src), okay),
		     "Clock selected as system clock no enabled in DT");
	BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_PROP(DT_NODELABEL(lp_clk), clock_src), okay),
		     "Clock selected as LP clock no enabled in DT");
	BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_NODELABEL(pll), disabled) ||
		     DT_NODE_HAS_STATUS(DT_NODELABEL(xtal32m), okay),
		     "PLL enabled in DT but XTAL32M is disabled");

	sys_clk_id = DT_DEP_ORD(DT_NODELABEL(sys_clk));
	smartbond_clock_control_on(dev, &sys_clk_id);
	if (DT_SAME_NODE(DT_PROP(DT_NODELABEL(lp_clk), clock_src),
			 DT_NODELABEL(rc32k))) {
	} else if (DT_SAME_NODE(DT_PROP(DT_NODELABEL(lp_clk), clock_src),
				DT_NODELABEL(rcx))) {
		clk_ctrl |= 1 << CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Pos;
	} else if (DT_SAME_NODE(DT_PROP(DT_NODELABEL(lp_clk), clock_src),
				DT_NODELABEL(xtal32k))) {
		clk_ctrl |= 2 << CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Pos;
	}

	if (DT_SAME_NODE(DT_PROP(DT_NODELABEL(sys_clk), clock_src),
			 DT_NODELABEL(rc32m))) {
		clk_ctrl |= 1 << CRG_TOP_CLK_CTRL_REG_SYS_CLK_SEL_Pos;
		SystemCoreClock = DT_PROP(DT_NODELABEL(rc32m), clock_frequency);
	} else if (DT_SAME_NODE(DT_PROP(DT_NODELABEL(sys_clk), clock_src),
				DT_NODELABEL(pll))) {
		da1469x_clock_pll_wait_to_lock();
		SystemCoreClock = DT_PROP(DT_NODELABEL(pll), clock_frequency);
		clk_ctrl |= 3 << CRG_TOP_CLK_CTRL_REG_SYS_CLK_SEL_Pos;
	} else if (DT_SAME_NODE(DT_PROP(DT_NODELABEL(sys_clk), clock_src),
				DT_NODELABEL(xtal32m))) {
		da1469x_clock_sys_xtal32m_switch_safe();
	}
	CRG_TOP->CLK_CTRL_REG = clk_ctrl;

	/* Disable unwanted oscillators */
	DT_FOREACH_CHILD_SEP(DT_PATH(crg, osc), DISABLE_OSC, (;));

	return 0;
}

static struct clock_control_driver_api smartbond_clock_control_api = {
	.on = smartbond_clock_control_on,
	.off = smartbond_clock_control_off,
	.get_rate = smartbond_clock_control_get_rate,
};

DEVICE_DT_DEFINE(DT_NODELABEL(osc),
		 &smartbond_clocks_init,
		 NULL,
		 NULL, NULL,
		 PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &smartbond_clock_control_api);
