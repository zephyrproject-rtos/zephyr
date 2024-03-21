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
#include <da1469x_clock.h>

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

#define CALIBRATION_INTERVAL (DT_NODE_HAS_STATUS(DT_NODELABEL(rcx), okay) ?	\
			DT_PROP(DT_NODELABEL(rcx), calibration_interval) :	\
			DT_PROP(DT_NODELABEL(rc32k), calibration_interval))

static void calibration_work_cb(struct k_work *work);
static void xtal32k_settle_work_cb(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(calibration_work, calibration_work_cb);
static K_WORK_DELAYABLE_DEFINE(xtal32k_settle_work, xtal32k_settle_work_cb);

static void calibration_work_cb(struct k_work *work)
{
	if (lpc_clock_state.rcx_started) {
		da1469x_clock_lp_rcx_calibrate();
		lpc_clock_state.rcx_ready = true;
		lpc_clock_state.rcx_freq = da1469x_clock_lp_rcx_freq_get();
		k_work_schedule(&calibration_work,
				K_MSEC(1000 * CALIBRATION_INTERVAL));
		LOG_DBG("RCX calibration done, RCX freq: %d",
			(int)lpc_clock_state.rcx_freq);
	} else if (lpc_clock_state.rc32k_started) {
		da1469x_clock_lp_rc32k_calibrate();
		lpc_clock_state.rc32k_ready = true;
		lpc_clock_state.rc32k_freq = da1469x_clock_lp_rc32k_freq_get();
		k_work_schedule(&calibration_work,
				K_MSEC(1000 * CALIBRATION_INTERVAL));
		LOG_DBG("RC32K calibration done, RC32K freq: %d",
			(int)lpc_clock_state.rc32k_freq);
	}
}

static void xtal32k_settle_work_cb(struct k_work *work)
{
	if (lpc_clock_state.xtal32k_started && !lpc_clock_state.xtal32k_ready) {
		LOG_DBG("XTAL32K settled.");
		lpc_clock_state.xtal32k_ready = true;
	}
}

static void smartbond_start_rc32k(void)
{
	if ((CRG_TOP->CLK_RC32K_REG & CRG_TOP_CLK_RC32K_REG_RC32K_ENABLE_Msk) == 0) {
		CRG_TOP->CLK_RC32K_REG |= CRG_TOP_CLK_RC32K_REG_RC32K_ENABLE_Msk;
	}
	lpc_clock_state.rc32k_started = true;
	if (!lpc_clock_state.rc32k_ready && (CALIBRATION_INTERVAL > 0)) {
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
	if (!lpc_clock_state.rcx_ready && (CALIBRATION_INTERVAL > 0)) {
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

static inline int smartbond_clock_control_on(const struct device *dev,
					     clock_control_subsys_t sub_system)
{
	enum smartbond_clock clk = (enum smartbond_clock)(sub_system);

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
	case SMARTBOND_CLK_PLL96M:
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
	enum smartbond_clock clk = (enum smartbond_clock)(sub_system);

	ARG_UNUSED(dev);

	switch (clk) {
	case SMARTBOND_CLK_RC32K:
		if (((CRG_TOP->CLK_CTRL_REG & CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Msk) >>
			   CRG_TOP_CLK_CTRL_REG_LP_CLK_SEL_Pos) != 0) {
			CRG_TOP->CLK_RC32K_REG &= ~CRG_TOP_CLK_RC32K_REG_RC32K_ENABLE_Msk;
			lpc_clock_state.rc32k_ready = false;
			lpc_clock_state.rc32k_started = false;
		}
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
	case SMARTBOND_CLK_PLL96M:
		da1469x_clock_sys_pll_disable();
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
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

static void
qspi_set_read_pipe_delay(uint8_t delay)
{
	QSPIC->QSPIC_CTRLMODE_REG =
		(QSPIC->QSPIC_CTRLMODE_REG & ~QSPIC_QSPIC_CTRLMODE_REG_QSPIC_PCLK_MD_Msk) |
		(delay << QSPIC_QSPIC_CTRLMODE_REG_QSPIC_PCLK_MD_Pos) |
		QSPIC_QSPIC_CTRLMODE_REG_QSPIC_RPIPE_EN_Msk;
}

static void
qspi_set_cs_delay(uint32_t sys_clock_freq, uint32_t read_delay_ns, uint32_t erase_delay_ns)
{
	sys_clock_freq /= 100000;
	uint32_t read_delay_cyc = ((read_delay_ns * sys_clock_freq) + 9999) / 10000;
	uint32_t erase_delay_cyc = ((erase_delay_ns * sys_clock_freq) + 9999) / 10000;

	QSPIC->QSPIC_BURSTCMDB_REG =
		(QSPIC->QSPIC_BURSTCMDB_REG & ~QSPIC_QSPIC_BURSTCMDB_REG_QSPIC_CS_HIGH_MIN_Msk) |
		read_delay_cyc << QSPIC_QSPIC_BURSTCMDB_REG_QSPIC_CS_HIGH_MIN_Pos;
	QSPIC->QSPIC_ERASECMDB_REG =
		(QSPIC->QSPIC_ERASECMDB_REG & ~QSPIC_QSPIC_ERASECMDB_REG_QSPIC_ERS_CS_HI_Msk) |
		(erase_delay_cyc << QSPIC_QSPIC_ERASECMDB_REG_QSPIC_ERS_CS_HI_Pos);
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
		CRG_TOP->CLK_CTRL_REG = (CRG_TOP->CLK_CTRL_REG & ~clk_sel_msk) | clk_sel;
	}

	return rc;
}

int z_smartbond_select_sys_clk(enum smartbond_clock sys_clk)
{
	uint32_t sys_clock_freq;
	uint32_t clk_sel;
	uint32_t clk_sel_msk = CRG_TOP_CLK_CTRL_REG_SYS_CLK_SEL_Msk;
	int res;

	res = smartbond_clock_get_rate(sys_clk, &sys_clock_freq);
	if (res != 0) {
		return -EINVAL;
	}

	/* When PLL is selected as system clock qspi read pipe delay must be set to 7 */
	if (sys_clock_freq > 32000000) {
		qspi_set_read_pipe_delay(7);
		qspi_set_cs_delay(SystemCoreClock,
				  DT_PROP(DT_NODELABEL(flash_controller),
					  read_cs_idle_delay),
				  DT_PROP(DT_NODELABEL(flash_controller),
					  erase_cs_idle_delay));
	}

	if (sys_clk == SMARTBOND_CLK_RC32M) {
		clk_sel = 1 << CRG_TOP_CLK_CTRL_REG_SYS_CLK_SEL_Pos;
		CRG_TOP->CLK_CTRL_REG = (CRG_TOP->CLK_CTRL_REG & ~clk_sel_msk) | clk_sel;
		SystemCoreClock = sys_clock_freq;
	} else if (sys_clk == SMARTBOND_CLK_PLL96M) {
		da1469x_clock_pll_wait_to_lock();
		da1469x_clock_sys_pll_switch();
	} else if (sys_clk == SMARTBOND_CLK_XTAL32M) {
		da1469x_clock_sys_xtal32m_switch_safe();
	}

	/* When switching back from PLL to 32MHz read pipe delay may be set to 2 */
	if (SystemCoreClock <= 32000000) {
		qspi_set_read_pipe_delay(2);
		qspi_set_cs_delay(SystemCoreClock,
				  DT_PROP(DT_NODELABEL(flash_controller),
					  read_cs_idle_delay),
				  DT_PROP(DT_NODELABEL(flash_controller),
					  erase_cs_idle_delay));
	}

	return 0;
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

#define ENABLE_OSC(clock) smartbond_clock_control_on_by_ord(dev, DT_DEP_ORD(clock))
#define DISABLE_OSC(clock) if (DT_NODE_HAS_STATUS(clock, disabled)) { \
		smartbond_clock_control_off_by_ord(dev, DT_DEP_ORD(clock)); \
	}

	/* Enable all oscillators with status "okay" */
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(crg, osc), ENABLE_OSC, (;));

	/* Make sure that selected sysclock is enabled */
	BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_PROP(DT_NODELABEL(sys_clk), clock_src), okay),
		     "Clock selected as system clock no enabled in DT");
	BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_PROP(DT_NODELABEL(lp_clk), clock_src), okay),
		     "Clock selected as LP clock no enabled in DT");
	BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_NODELABEL(pll), disabled) ||
		     DT_NODE_HAS_STATUS(DT_NODELABEL(xtal32m), okay),
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
