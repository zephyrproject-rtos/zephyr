/*
 * Copyright (c) 2022 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <soc.h>
#include <zephyr/sys/onoff.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/smartbond_clock_control.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <da1469x_clock.h>
#include <da1469x_qspic.h>
#if defined(CONFIG_BT_DA1469X)
#include <shm.h>
#endif
#include <zephyr/drivers/regulator.h>

LOG_MODULE_REGISTER(clock_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define DT_DRV_COMPAT smartbond_clock

struct lpc_clock_state {
	uint8_t rcx_started : 1;
	uint8_t rcx_ready : 1;
	uint8_t rc32k_started : 1;
	uint8_t rc32k_ready : 1;
	uint8_t xtal32k_started : 1;
	uint8_t xtal32k_ready : 1;
	uint32_t rcx_freq;
	uint32_t rc32k_freq;
} lpc_clock_state = {
	.rcx_freq = DT_PROP(DT_NODELABEL(rcx), clock_frequency),
	.rc32k_freq = DT_PROP(DT_NODELABEL(rc32k), clock_frequency),
};

#define CALIBRATION_INTERVAL CONFIG_SMARTBOND_LP_OSC_CALIBRATION_INTERVAL

#ifdef CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME
extern int z_clock_hw_cycles_per_sec;
#endif

static void calibration_work_cb(struct k_work *work);
static void xtal32k_settle_work_cb(struct k_work *work);
static enum smartbond_clock smartbond_source_clock(enum smartbond_clock clk);

static K_WORK_DELAYABLE_DEFINE(calibration_work, calibration_work_cb);
static K_WORK_DELAYABLE_DEFINE(xtal32k_settle_work, xtal32k_settle_work_cb);

/* PLL can be turned on by requesting it explicitly or when USB is attached */
/* PLL requested in DT or manually by application */
#define PLL_REQUEST_PLL		1
/* PLL requested indirectly by USB driver */
#define PLL_REQUEST_USB		2
/* Keeps information about blocks that requested PLL */
static uint8_t pll_requests;

static void calibration_work_cb(struct k_work *work)
{
	if (lpc_clock_state.rcx_started) {
		da1469x_clock_lp_rcx_calibrate();
		lpc_clock_state.rcx_ready = true;
		lpc_clock_state.rcx_freq = da1469x_clock_lp_rcx_freq_get();
		LOG_DBG("RCX calibration done, RCX freq: %d",
			(int)lpc_clock_state.rcx_freq);

#if defined(CONFIG_BT_DA1469X)
		/* Update CMAC sleep clock with calculated frequency if RCX is set as lp_clk */
		if ((CRG_TOP->CLK_CTRL_REG & CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Msk) ==
		    (1 << CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Pos)) {
			cmac_request_lp_clock_freq_set(lpc_clock_state.rcx_freq);
		}
#endif
	}
	if (lpc_clock_state.rc32k_started) {
		da1469x_clock_lp_rc32k_calibrate();
		lpc_clock_state.rc32k_ready = true;
		lpc_clock_state.rc32k_freq = da1469x_clock_lp_rc32k_freq_get();
		LOG_DBG("RC32K calibration done, RC32K freq: %d",
			(int)lpc_clock_state.rc32k_freq);
	}
	k_work_schedule(&calibration_work,
			K_MSEC(1000 * CALIBRATION_INTERVAL));
#ifdef CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME
	switch (smartbond_source_clock(SMARTBOND_CLK_LP_CLK)) {
	case SMARTBOND_CLK_RCX:
		z_clock_hw_cycles_per_sec = lpc_clock_state.rcx_freq;
		break;
	case SMARTBOND_CLK_RC32K:
		z_clock_hw_cycles_per_sec = lpc_clock_state.rc32k_freq;
		break;
	default:
		break;
	}
#endif
}

static void xtal32k_settle_work_cb(struct k_work *work)
{
	if (lpc_clock_state.xtal32k_started && !lpc_clock_state.xtal32k_ready) {
		LOG_DBG("XTAL32K settled.");
		lpc_clock_state.xtal32k_ready = true;

#if defined(CONFIG_BT_DA1469X)
		/* Update CMAC sleep clock if XTAL32K is set as lp_clk */
		if ((CRG_TOP->CLK_CTRL_REG & CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Msk) ==
		    (2 << CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Pos)) {
			cmac_request_lp_clock_freq_set(32768);
		}
#endif
	}
}

static void smartbond_start_rc32k(void)
{
	if ((CRG_TOP->CLK_RC32K_REG & CRG_TOP_CLK_RC32K_REG_RC32K_ENABLE_Msk) == 0) {
		CRG_TOP->CLK_RC32K_REG |= CRG_TOP_CLK_RC32K_REG_RC32K_ENABLE_Msk;
	}
	lpc_clock_state.rc32k_started = true;
	if (!lpc_clock_state.rc32k_ready) {
		if (!k_work_is_pending(&calibration_work.work)) {
			k_work_schedule(&calibration_work,
					K_MSEC(1000 * CALIBRATION_INTERVAL));
		}
	}
}

static void smartbond_start_rcx(void)
{
	if (!lpc_clock_state.rcx_started) {
		lpc_clock_state.rcx_ready = false;
		da1469x_clock_lp_rcx_enable();
		lpc_clock_state.rcx_started = true;
	}
	if (!lpc_clock_state.rcx_ready) {
		if (!k_work_is_pending(&calibration_work.work)) {
			k_work_schedule(&calibration_work,
					K_MSEC(1000 * CALIBRATION_INTERVAL));
		}
	}
}

static void smartbond_start_xtal32k(void)
{
	if (!lpc_clock_state.xtal32k_started) {
		lpc_clock_state.xtal32k_ready = false;
		da1469x_clock_lp_xtal32k_enable();
		lpc_clock_state.xtal32k_started = true;
		k_work_schedule(&xtal32k_settle_work,
				K_MSEC(DT_PROP(DT_NODELABEL(xtal32k),
					       settle_time)));
	}
}

#ifdef CONFIG_REGULATOR
/*
 * Should be used to control PLL when the regulator driver is available.
 * If the latter is available, then the VDD level should be changed when
 * switching to/from PLL. Otherwise, the VDD level is considered to
 * be fixed @1.2V which should support both XTAL32M and PLL system clocks.
 */
static int smartbond_clock_set_pll_status(bool status)
{
	const struct device *dev  = DEVICE_DT_GET(DT_NODELABEL(vdd));
	int ret;

	if (!device_is_ready(dev)) {
		LOG_ERR("Regulator device is not ready");
		return -ENODEV;
	}

	if (status) {
		/* Enabling PLL requires that VDD be raised to 1.2V */
		if (regulator_set_voltage(dev, 1200000, 1200000) == 0) {
			da1469x_clock_sys_pll_enable();

			/* QSPIC read pipe delay should be updated when switching to PLL */
		} else {
			LOG_ERR("Failed to set VDD_LEVEL to 1.2V");
			return -EIO;
		}
	} else {
		/* Disable PLL and switch back to XTAL32M */
		da1469x_clock_sys_pll_disable();

		/* VDD level can now be switched back to 0.9V */
		ret = regulator_set_voltage(dev, 900000, 900000);
		if (ret < 0) {
			LOG_WRN("Failed to set VDD_LEVEL to 0.9V");
		} else {
			/*
			 * System clock should be switched to XTAL32M and VDD should be set to 0.9.
			 * The QSPIC read pipe delay should be updated.
			 */
			da1469x_qspi_set_read_pipe_delay(QSPIC_ID, 2);
		}
	}

	return 0;
}
#endif

static inline int smartbond_clock_control_on(const struct device *dev,
					     clock_control_subsys_t sub_system)
{
	enum smartbond_clock clk = (enum smartbond_clock)(sub_system);
	int ret = 0;

	ARG_UNUSED(dev);

	switch (clk) {
	case SMARTBOND_CLK_RC32K:
		smartbond_start_rc32k();
		break;
	case SMARTBOND_CLK_RCX:
		smartbond_start_rcx();
		break;
	case SMARTBOND_CLK_XTAL32K:
		smartbond_start_xtal32k();
		break;
	case SMARTBOND_CLK_RC32M:
		CRG_TOP->CLK_RC32M_REG |= CRG_TOP_CLK_RC32M_REG_RC32M_ENABLE_Msk;
		break;
	case SMARTBOND_CLK_XTAL32M:
		da1469x_clock_sys_xtal32m_init();
		da1469x_clock_sys_xtal32m_enable();
		break;
	case SMARTBOND_CLK_USB:
	case SMARTBOND_CLK_PLL96M:
		pll_requests = 1 << (clk - SMARTBOND_CLK_PLL96M);
		if ((CRG_TOP->CLK_CTRL_REG & CRG_TOP_CLK_CTRL_REG_RUNNING_AT_PLL96M_Msk) == 0) {
			if ((CRG_TOP->CLK_CTRL_REG &
			     CRG_TOP_CLK_CTRL_REG_RUNNING_AT_XTAL32M_Msk) == 0) {
				da1469x_clock_sys_xtal32m_init();
				da1469x_clock_sys_xtal32m_enable();
				da1469x_clock_sys_xtal32m_wait_to_settle();
			}
#if CONFIG_REGULATOR
			ret = smartbond_clock_set_pll_status(true);
#else
			da1469x_clock_sys_pll_enable();
#endif
			if (pll_requests & PLL_REQUEST_USB) {
				CRG_TOP->CLK_CTRL_REG &= ~CRG_TOP_CLK_CTRL_REG_USB_CLK_SRC_Msk;
			}
		}
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}

static inline int smartbond_clock_control_off(const struct device *dev,
					      clock_control_subsys_t sub_system)
{
	enum smartbond_clock clk = (enum smartbond_clock)(sub_system);
	int ret = 0;

	ARG_UNUSED(dev);

	switch (clk) {
	case SMARTBOND_CLK_RC32K:
		/* RC32K is used by POWERUP and WAKEUP HW FSM */
		BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(rc32k)),
				"RC32K is not allowed to be turned off");
		ret = -EPERM;
		break;
	case SMARTBOND_CLK_RCX:
		if (((CRG_TOP->CLK_CTRL_REG & CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Msk) >>
			   CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Pos) != 1) {
			CRG_TOP->CLK_RCX_REG &= ~CRG_TOP_CLK_RCX_REG_RCX_ENABLE_Msk;
			lpc_clock_state.rcx_ready = false;
			lpc_clock_state.rcx_started = false;
		}
		break;
	case SMARTBOND_CLK_XTAL32K:
		if (((CRG_TOP->CLK_CTRL_REG & CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Msk) >>
			CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Pos) > 1) {
			CRG_TOP->CLK_XTAL32K_REG &= ~CRG_TOP_CLK_XTAL32K_REG_XTAL32K_ENABLE_Msk;
			lpc_clock_state.xtal32k_ready = false;
			lpc_clock_state.xtal32k_started = false;
		}
		break;
	case SMARTBOND_CLK_RC32M:
		/* Disable rc32m only if not used as system clock */
		if ((CRG_TOP->CLK_CTRL_REG & CRG_TOP_CLK_CTRL_REG_RUNNING_AT_RC32M_Msk) == 0) {
			da1469x_clock_sys_rc32m_disable();
		}
		break;
	case SMARTBOND_CLK_XTAL32M:
		da1469x_clock_sys_xtal32m_init();
		da1469x_clock_sys_xtal32m_enable();
		break;
	case SMARTBOND_CLK_USB:
		/* Switch USB clock to HCLK to allow for resume */
		CRG_TOP->CLK_CTRL_REG |= CRG_TOP_CLK_CTRL_REG_USB_CLK_SRC_Msk;
		__fallthrough;
	case SMARTBOND_CLK_PLL96M:
		pll_requests &= ~(1 << (clk - SMARTBOND_CLK_PLL96M));
		if (pll_requests == 0) {
			/*
			 * PLL must not be disabled as long as a peripheral e.g. LCDC is enabled
			 * and clocked by PLL.
			 */
			if (!da1469x_clock_check_device_div1_clock()) {
#if CONFIG_REGULATOR
				ret = smartbond_clock_set_pll_status(false);
#else
				da1469x_clock_sys_pll_disable();
#endif
			} else {
				ret = -EPERM;
			}
		}
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}

static enum smartbond_clock smartbond_source_clock(enum smartbond_clock clk)
{
	static const enum smartbond_clock lp_clk_src[] = {
		SMARTBOND_CLK_RC32K,
		SMARTBOND_CLK_RCX,
		SMARTBOND_CLK_XTAL32K,
		SMARTBOND_CLK_XTAL32K,
	};
	static const enum smartbond_clock sys_clk_src[] = {
		SMARTBOND_CLK_XTAL32M,
		SMARTBOND_CLK_RC32M,
		SMARTBOND_CLK_LP_CLK,
		SMARTBOND_CLK_PLL96M,
	};

	if (clk == SMARTBOND_CLK_SYS_CLK) {
		clk = sys_clk_src[CRG_TOP->CLK_CTRL_REG &
				  CRG_TOP_CLK_CTRL_REG_SYS_CLK_SEL_Msk];
	}
	/* System clock can be low power clock, so next check is not in else */
	if (clk == SMARTBOND_CLK_LP_CLK) {
		clk = lp_clk_src[(CRG_TOP->CLK_CTRL_REG &
				  CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Msk) >>
				 CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Pos];
	}
	return clk;
}

static int smartbond_clock_get_rate(enum smartbond_clock clk, uint32_t *rate)
{
	clk = smartbond_source_clock(clk);
	switch (clk) {
	case SMARTBOND_CLK_RC32K:
		*rate = lpc_clock_state.rc32k_freq;
		break;
	case SMARTBOND_CLK_RCX:
		*rate = lpc_clock_state.rcx_freq;
		break;
	case SMARTBOND_CLK_XTAL32K:
		*rate = DT_PROP(DT_NODELABEL(xtal32k), clock_frequency);
		break;
	case SMARTBOND_CLK_RC32M:
		*rate = DT_PROP(DT_NODELABEL(rc32m), clock_frequency);
		break;
	case SMARTBOND_CLK_XTAL32M:
		*rate = DT_PROP(DT_NODELABEL(xtal32m), clock_frequency);
		break;
	case SMARTBOND_CLK_PLL96M:
		*rate = DT_PROP(DT_NODELABEL(pll), clock_frequency);
		break;
	case SMARTBOND_CLK_USB:
		*rate = 48000000;
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
	ARG_UNUSED(dev);

	return smartbond_clock_get_rate((enum smartbond_clock)(sub_system), rate);
}

static enum smartbond_clock smartbond_dt_ord_to_clock(uint32_t dt_ord)
{
	switch (dt_ord) {
	case DT_DEP_ORD(DT_NODELABEL(rc32k)):
		return SMARTBOND_CLK_RC32K;
	case DT_DEP_ORD(DT_NODELABEL(rcx)):
		return SMARTBOND_CLK_RCX;
	case DT_DEP_ORD(DT_NODELABEL(xtal32k)):
		return SMARTBOND_CLK_XTAL32K;
	case DT_DEP_ORD(DT_NODELABEL(rc32m)):
		return SMARTBOND_CLK_RC32M;
	case DT_DEP_ORD(DT_NODELABEL(xtal32m)):
		return SMARTBOND_CLK_XTAL32M;
	case DT_DEP_ORD(DT_NODELABEL(pll)):
		return SMARTBOND_CLK_PLL96M;
	default:
		return SMARTBOND_CLK_NONE;
	}
}

static void smartbond_clock_control_on_by_ord(const struct device *dev,
					      uint32_t clock_id)
{
	enum smartbond_clock clk = smartbond_dt_ord_to_clock(clock_id);

	smartbond_clock_control_on(dev, (clock_control_subsys_rate_t)clk);
}

static void smartbond_clock_control_off_by_ord(const struct device *dev,
					       uint32_t clock_id)
{
	enum smartbond_clock clk = smartbond_dt_ord_to_clock(clock_id);

	smartbond_clock_control_off(dev, (clock_control_subsys_rate_t)clk);
}

int z_smartbond_select_lp_clk(enum smartbond_clock lp_clk)
{
	int rc = 0;
	uint32_t clk_sel = 0;
	uint32_t clk_sel_msk = CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Msk;

	switch (lp_clk) {
	case SMARTBOND_CLK_RC32K:
		clk_sel = 0;
		break;
	case SMARTBOND_CLK_RCX:
		clk_sel = 1 << CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Pos;
		break;
	case SMARTBOND_CLK_XTAL32K:
		clk_sel = 2 << CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Pos;
		break;
	default:
		rc = -EINVAL;
	}

	if (rc == 0) {
#ifdef CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME
		switch (lp_clk) {
		case SMARTBOND_CLK_RCX:
			z_clock_hw_cycles_per_sec = lpc_clock_state.rcx_freq;
			break;
		case SMARTBOND_CLK_RC32K:
			z_clock_hw_cycles_per_sec = lpc_clock_state.rc32k_freq;
			break;
		default:
			z_clock_hw_cycles_per_sec = 32768;
			break;
		}
#endif
		CRG_TOP->CLK_CTRL_REG = (CRG_TOP->CLK_CTRL_REG & ~clk_sel_msk) | clk_sel;
	}

	return rc;
}

static void smartbond_clock_control_update_memory_settings(uint32_t sys_clock_freq)
{
	if (sys_clock_freq > 32000000) {
		da1469x_qspi_set_read_pipe_delay(QSPIC_ID, 7);
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(memc))
		da1469x_qspi_set_read_pipe_delay(QSPIC2_ID, 7);
#endif
	} else {
		da1469x_qspi_set_read_pipe_delay(QSPIC_ID, 2);
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(memc))
		da1469x_qspi_set_read_pipe_delay(QSPIC2_ID, 2);
#endif
	}

	da1469x_qspi_set_cs_delay(QSPIC_ID, SystemCoreClock,
		DT_PROP(DT_NODELABEL(flash_controller), read_cs_idle_delay),
		DT_PROP(DT_NODELABEL(flash_controller), erase_cs_idle_delay));
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(memc))
	da1469x_qspi_set_cs_delay(QSPIC2_ID, SystemCoreClock,
		DT_PROP(DT_NODELABEL(memc), read_cs_idle_min_ns),
		DT_PROP_OR(DT_NODELABEL(memc), erase_cs_idle_min_ns, 0));
#if DT_PROP(DT_NODELABEL(memc), is_ram)
	da1469x_qspi_set_tcem(SystemCoreClock, DT_PROP(DT_NODELABEL(memc), tcem_max_us));
#endif
#endif
}

int z_smartbond_select_sys_clk(enum smartbond_clock sys_clk)
{
	uint32_t sys_clock_freq;
	uint32_t clk_sel;
	uint32_t clk_sel_msk = CRG_TOP_CLK_CTRL_REG_SYS_CLK_SEL_Msk;
	int res = 0;

	res = smartbond_clock_get_rate(sys_clk, &sys_clock_freq);
	if (res != 0) {
		return -EINVAL;
	}

	/* When PLL is selected as system clock qspi read pipe delay must be set to 7 */
	if (sys_clock_freq > 32000000) {
		smartbond_clock_control_update_memory_settings(sys_clock_freq);
	}

	if (sys_clk == SMARTBOND_CLK_RC32M) {
		clk_sel = 1 << CRG_TOP_CLK_CTRL_REG_SYS_CLK_SEL_Pos;
		CRG_TOP->CLK_CTRL_REG = (CRG_TOP->CLK_CTRL_REG & ~clk_sel_msk) | clk_sel;
		SystemCoreClock = sys_clock_freq;
	} else if (sys_clk == SMARTBOND_CLK_PLL96M) {
		/* Check that PLL is already enabled, otherwise enable it. */
		if (!da1469x_clock_sys_pll_is_enabled()) {
#if CONFIG_REGULATOR
			res = smartbond_clock_set_pll_status(true);
			if (res != 0) {
				return -EIO;
			}
#else
			da1469x_clock_sys_pll_enable();
#endif
		}
		da1469x_clock_sys_pll_switch();
	} else if (sys_clk == SMARTBOND_CLK_XTAL32M) {
		/*
		 * XTAL32M should be enabled eitherway as it's not allowed
		 * to be turned off by application.
		 */
		da1469x_clock_sys_xtal32m_switch_safe();
	} else {
		return -EINVAL;
	}

	/* When switching back from PLL to 32MHz read pipe delay may be set to 2 */
	if (SystemCoreClock <= 32000000) {
		smartbond_clock_control_update_memory_settings(SystemCoreClock);
	}

	return res;
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
	uint32_t clk_id;
	enum smartbond_clock lp_clk;
	enum smartbond_clock sys_clk;

	ARG_UNUSED(dev);

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(memc))
	/* Make sure QSPIC2 is enabled */
	da1469x_clock_amba_enable(CRG_TOP_CLK_AMBA_REG_QSPI2_ENABLE_Msk);
#endif

#define ENABLE_OSC(clock) smartbond_clock_control_on_by_ord(dev, DT_DEP_ORD(clock))
#define DISABLE_OSC(clock) if (DT_NODE_HAS_STATUS(clock, disabled)) { \
		smartbond_clock_control_off_by_ord(dev, DT_DEP_ORD(clock)); \
	}

	/* Enable all oscillators with status "okay" */
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(crg, osc), ENABLE_OSC, (;));

	/* Make sure that selected sysclock is enabled */
	BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_PROP(DT_NODELABEL(sys_clk), clock_src)),
		     "Clock selected as system clock no enabled in DT");
	BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_PROP(DT_NODELABEL(lp_clk), clock_src)),
		     "Clock selected as LP clock no enabled in DT");
	BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_NODELABEL(pll), disabled) ||
		     DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(xtal32m)),
		     "PLL enabled in DT but XTAL32M is disabled");

	clk_id = DT_DEP_ORD(DT_PROP(DT_NODELABEL(lp_clk), clock_src));
	lp_clk = smartbond_dt_ord_to_clock(clk_id);
	z_smartbond_select_lp_clk(lp_clk);

	clk_id = DT_DEP_ORD(DT_PROP(DT_NODELABEL(sys_clk), clock_src));
	sys_clk = smartbond_dt_ord_to_clock(clk_id);
	smartbond_clock_control_on(dev,
				   (clock_control_subsys_rate_t)smartbond_source_clock(sys_clk));
	z_smartbond_select_sys_clk(sys_clk);

	/* Disable unwanted oscillators */
	DT_FOREACH_CHILD_SEP(DT_PATH(crg, osc), DISABLE_OSC, (;));

	return 0;
}

static const struct clock_control_driver_api smartbond_clock_control_api = {
	.on = smartbond_clock_control_on,
	.off = smartbond_clock_control_off,
	.get_rate = smartbond_clock_control_get_rate,
};

#if CONFIG_PM_DEVICE
static int smartbond_clocks_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_RESUME:
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(memc))
		/* Make sure QSPIC2 is enabled */
		da1469x_clock_amba_enable(CRG_TOP_CLK_AMBA_REG_QSPI2_ENABLE_Msk);
#endif
		/*
		 * Make sure the flash controller has correct settings as clock restoration
		 * might have been performed upon waking up.
		 */
		smartbond_clock_control_update_memory_settings(SystemCoreClock);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

PM_DEVICE_DT_DEFINE(DT_NODELABEL(osc), smartbond_clocks_pm_action);

DEVICE_DT_DEFINE(DT_NODELABEL(osc),
		 smartbond_clocks_init,
		 PM_DEVICE_DT_GET(DT_NODELABEL(osc)),
		 NULL, NULL,
		 PRE_KERNEL_1,
		 CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &smartbond_clock_control_api);
