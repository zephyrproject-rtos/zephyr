/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_pcr

#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <cmsis_core.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/clock/mchp_xec_pcr.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/irq.h>
#include <zephyr/sys/barrier.h>

/* PCR and Peripheral clock sources can be:
 * VBAT backed selections
 *   Internal silicon 32K OSC
 *   Parallel external XTAL on XTAL1 and XTAL2 pins
 *   Single-ended input on XTAL2 pin (SE crystal circuit or 32KHz square wave)
 * VTR only selections:
 *   32KHZ_IN pin. !!! This pin is an alternate function on a GPIO. Not VBAT backed !!!
 *
 * Driver Init:
 *   Do PINCTRL
 *   Check selections matches current HW config
 *   If match return succes
 *   Else
 *     Configure both to Silicon OSC
 *     Application can request change with optional callback
 */

#define CLK32K_SIL_OSC_DELAY         256
#define CLK32K_PLL_LOCK_WAIT         (16 * 1024)
#define CLK32K_PIN_WAIT              4096
#define CLK32K_XTAL_WAIT             (16 * 1024)
#define CLK32K_XTAL_MON_WAIT         (64 * 1024)
#define XEC_CC_DFLT_PLL_LOCK_WAIT_MS 30

/*
 * Counter checks:
 * 32KHz period counter minimum for pass/fail: 16-bit
 * 32KHz period counter maximum for pass/fail: 16-bit
 * 32KHz duty cycle variation max for pass/fail: 16-bit
 * 32KHz valid count minimum: 8-bit
 *
 * 32768 Hz period is 30.518 us
 * HW count resolution is 48 MHz.
 * One 32KHz clock pulse = 1464.84 48 MHz counts.
 */
#define CNT32K_TMIN      1435u
#define CNT32K_TMAX      1495u
#define CNT32K_DUTY_MAX  212u
#define CNT32K_VALID_MIN 4u

#define DEST_PLL    0
#define DEST_PERIPH 1

#define CLK32K_FLAG_CRYSTAL_SE     BIT(0)
#define CLK32K_FLAG_PIN_FB_CRYSTAL BIT(1)

#define PCR_PERIPH_RESET_SPIN 8u

#define XEC_CC_XTAL_EN_DELAY_MS_DFLT 40u
#define HIBTIMER_MS_TO_CNT(x)        ((uint32_t)(x) * 33U)

/* Maximum number of milliseconds the 16-bit hibernation timer can count */
#define HIBTIMER_MAX_MS 8191u

/* Hibernation timer registers */
#define XEC_HT_PRLD_OFS        0  /* preload, r/w 16-bit. 0 disables timer */
#define XEC_HT_CR_OFS          4u /* control r/w 16-bit */
#define XEC_HT_CR_FREQ_8HZ_POS 0  /* 0=32768 Hz (30.5 us), 1 = 8 Hz (125 ms) */
#define XEC_HT_CNT_POS         8u /* counter r/o 16-bit */

#define XEC_MEC150X_DEV_ID_LSB 0x20u

#define XEC_ECS_FL_OFS         0x68u
#define XEC_ECS_FL_CLK_48M_POS 19

#define XEC_XTAL_SRC_SE_FLAG_POS 0 /* use single-ended crystal connection */

/* vs = VBAT 32KHz clock source
 * ps = PCR PLL 32KHz clock source
 * xse = 0(dual xtal), (1 single-ended xtal)
 */
#define XEC_CLKS_CFG_PLL_SRC_POS          28
#define XEC_CLKS_CFG_PLL_SRC_MSK          GENMASK(29, 28)
#define XEC_CLKS_CFG_PLL_SRC_SET(pll_src) FIELD_PREP(XEC_CLKS_CFG_PLL_SRC_MSK, (pll_src))
#define XEC_CLKS_CFG_PLL_SRC_GET(cc)      FIELD_PREP(XEC_CLKS_CFG_PLL_SRC_MSK, (cc))

#define XEC_CLKS_CFG_SRC_MSK                                                                       \
	(XEC_CLKS_CFG_PLL_SRC_MSK | XEC_VBR_CS_PCS_MSK | BIT(XEC_VBR_CS_XSE_POS))

#define XEC_CLKS_CONFIG(vs, ps, xse)                                                               \
	XEC_VBR_CS_PCS_SET(vs) | XEC_CLKS_CFG_PLL_SRC_SET(ps) |                                    \
		(((uint32_t)xse & BIT(0)) << XEC_VBR_CS_XSE_POS)

#define XEC_DT_CLK_FLAG_XTAL_SE_POS        0
#define XEC_DT_CLK_FLAG_PLL_SRC_LOCK_POS   1
#define XEC_DT_CLK_FLAG_SILOSC_EN_LOCK_POS 2
#define XEC_DT_CLK_FLAG_SILOSC_DIS_POS     3
#define XEC_DT_CLK_FLAG_XTAL_DHSC_POS      4
#define XEC_DT_CLK_FLAG_HAS_XG_POS         5

/* XTAL must be 32768 Hz */
#define XEC_XTAL_FREQ 32768U

#if defined(CONFIG_SOC_SERIES_MEC15XX)
#define XEC_CC15_VBR_CLK32_SRC_OFS 8U

/* MEC152x VBAT CLK32_SRC register defines */
#define XEC_CC15_VBATR_INT_SIL_OSC_VAL 0
#define XEC_CC15_VBATR_32KIN_PIN_POS   1
#define XEC_CC15_VBATR_XTAL_POS        2
#define XEC_CC15_VBATR_XTAL_SE_POS     3

/* MEC150x special requirements */
#define XEC_CC15_GCFG_DID_DEV_ID_MEC150X    0x0020U
#define XEC_CC15_TRIM_ENABLE_INT_OSCILLATOR 0x06U

#define XEC_GCFG_DR_ID_OFS     0x1CU
#define XEC_GCFG_DR_REV_POS    0
#define XEC_GCFG_DR_REV_MSK    GENMASK(7, 0)
#define XEC_GCFG_DR_REV_GET(r) FIELD_GET(XEC_GCFG_DR_REV_MSK, (r))
#define XEC_GCFG_DR_DEV_POS    16
#define XEC_GCFG_DR_DEV_MSK    GENMASK(31, 16)
#define XEC_GCFG_DR_DEV_GET(r) FIELD_GET(XEC_GCFG_DR_DEV_MSK, (r))
#endif

/* Driver config */
struct xec_pcr_config {
	uintptr_t pcr_base;
	uintptr_t vbr_base;
	uintptr_t htmr_base;
	const struct pinctrl_dev_config *pincfg;
	uint16_t xtal_enable_delay_ms;
	uint16_t xtal_mon_max_ms;
	uint16_t pll_lock_timeout_ms;
	uint16_t period_min;  /* mix and max 32KHz period range */
	uint16_t period_max;  /* monitor values in units of 48MHz (20.8 ns) */
	uint16_t max_dc_va;   /* 32KHz monitor maximum duty cycle variation */
	uint8_t min_valid;    /* minimum number of valid consecutive 32KHz pulses */
	uint8_t core_clk_div; /* Cortex-M4 clock divider (CPU and NVIC) */
	uint8_t xtal_se;      /* External 32KHz square wave on XTAL2 pin */
	uint8_t pcr_girq;
	uint8_t pcr_girq_pos;
	uint8_t htmr_girq;
	uint8_t htmr_girq_pos;
	uint8_t periph_src;
	uint8_t pll_src;
	uint8_t xtal_gain;
	uint8_t clk_flags;
};

struct xec_pcr_data {
	union clock_mchp_xec_subsys cc_subsys_data;
	uint8_t clkmon_status;
};

static uint32_t get_turbo_clock(const struct device *dev)
{
#if defined(CONFIG_SOC_SERIES_MEC15XX)
	return MHZ(48);
#elif defined(CONFIG_SOC_SERIES_MEC172X)
	const struct xec_pcr_config *drvcfg = dev->config;

	if (sys_test_bit(drvcfg->pcr_base + XEC_CC_TCLK_OFS, XEC_CC_TCLK_FAST_EN_POS) != 0) {
		return MHZ(96);
	}

	return MHZ(48);
#else
	mm_reg_t ecs_base = (mm_reg_t)DT_REG_ADDR(DT_NODELABEL(ecs));

	if (sys_test_bit(ecs_base + XEC_ECS_FL_OFS, XEC_ECS_FL_CLK_48M_POS) == 0) {
		return MHZ(96);
	}

	return MHZ(48);
#endif
}

static int xtal_enabled(const struct device *dev)
{
	const struct xec_pcr_config *drvcfg = dev->config;
#if defined(CONFIG_SOC_SERIES_MEC15XX)
	return sys_test_bit(drvcfg->vbr_base + XEC_CC15_VBR_CLK32_SRC_OFS, XEC_CC15_VBATR_XTAL_POS);
#else
	return sys_test_bit(drvcfg->vbr_base + XEC_VBR_CS_OFS, XEC_VBR_CS_XSTA_POS);
#endif
}

static void ht_clean(const struct device *dev)
{
	const struct xec_pcr_config *drvcfg = dev->config;
	mem_addr_t ht_base = drvcfg->htmr_base;

	sys_write16(0, ht_base);
	sys_write16(0, ht_base + 4u);
	soc_ecia_girq_status_clear(drvcfg->htmr_girq, drvcfg->htmr_girq_pos);
}

static int ht_wait_ready(const struct device *dev, uint16_t counts,
			 int (*is_ready)(const struct device *))
{
	const struct xec_pcr_config *drvcfg = dev->config;
	mem_addr_t ht_base = drvcfg->htmr_base;
	uint32_t ht_girq_status = 0;
	int ret = 0;

	sys_write16(0, ht_base);
	soc_ecia_girq_ctrl(drvcfg->htmr_girq, drvcfg->htmr_girq_pos, 0);
	soc_ecia_girq_status_clear(drvcfg->htmr_girq, drvcfg->htmr_girq_pos);

	sys_write16(0, ht_base + 4u); /* 30.5 us per count */
	sys_write16(counts, ht_base); /* disables if counts is 0 */

	if (counts != 0) {
		for (;;) {
			ht_girq_status = 0;
			soc_ecia_girq_status(drvcfg->htmr_girq, &ht_girq_status);
			if (ht_girq_status & BIT(drvcfg->htmr_girq_pos)) {
				ret = -ETIMEDOUT;
				break;
			}

			if ((is_ready != NULL) && (is_ready(dev) != 0)) {
				break;
			}
		}
	}

	return ret;
}

static int ht_wait_ready_ms(const struct device *dev, uint32_t ms,
			    int (*is_ready)(const struct device *))
{
	int ret = 0;
	uint16_t ht_counts = 0;

	while (ms > 0) {
		if (ms > 1998U) {
			ht_counts = UINT16_MAX;
			ms -= 1998U;
		} else {
			ht_counts = ms * 32u;
			ms = 0;
		}

		ret = ht_wait_ready(dev, ht_counts, is_ready);
		if (ret == 0) {
			break;
		}
	}

	return ret;
}

static int is_pll_locked(const struct device *dev)
{
	const struct xec_pcr_config *drvcfg = dev->config;
	mem_addr_t pcr_lock = drvcfg->pcr_base;

	if (sys_test_bit(pcr_lock + XEC_CC_OSC_ID_OFS, XEC_CC_OSC_ID_PLL_LOCK_POS) != 0) {
		return 1;
	}

	return 0;
}

static int xec_cc_wait_pll_lock(const struct device *dev)
{
	const struct xec_pcr_config *drvcfg = dev->config;
	int ret = 0;

	if (drvcfg->pll_lock_timeout_ms == 0) {
		while (is_pll_locked(dev) == 0) {
			;
		}
	} else {
		ret = ht_wait_ready_ms(dev, drvcfg->pll_lock_timeout_ms, is_pll_locked);
	}

	return 0;
}

#if defined(CONFIG_SOC_SERIES_MEC15XX)

/* NOTE: MEC15xx uses the save 32KHz clock source for peripherals and PCR */
static int mec15xx_clk32k_config(const struct device *dev, enum xec_pll_clk32k_src pll_clk_src,
				 enum xec_periph_clk32k_src periph_src, uint32_t flags)
{
	const struct xec_pcr_config *xcfg = dev->config;
	mm_reg_t vbr_base = xcfg->vbr_base;
	mm_reg_t global_cfg_base = DT_REG_ADDR(DT_NODELABEL(glblcfg0));
	uint32_t cken = 0, id = 0;

	id = sys_read32(global_cfg_base + XEC_GCFG_DR_ID_OFS);

	if (XEC_GCFG_DR_DEV_GET(id) == XEC_CC15_GCFG_DID_DEV_ID_MEC150X) {
		if (XEC_GCFG_DR_DEV_GET(id) == MCHP_GCFG_REV_B0) {
			sys_write8(XEC_CC15_TRIM_ENABLE_INT_OSCILLATOR, vbr_base + XEC_VBR_TCR_OFS);
		}
	}

	switch (pll_clk_src) {
	case XEC_PLL_CLK32K_SRC_SI:
		cken = XEC_CC15_VBATR_INT_SIL_OSC_VAL;
		break;
	case XEC_PLL_CLK32K_SRC_XTAL:
		cken = BIT(XEC_CC15_VBATR_XTAL_POS);
		if (flags & CLK32K_FLAG_CRYSTAL_SE) {
			cken |= BIT(XEC_CC15_VBATR_XTAL_SE_POS);
		}
		break;
	case XEC_PLL_CLK32K_SRC_PIN: /* 32KHZ_IN pin falls back to Silicon OSC */
		cken |= BIT(XEC_CC15_VBATR_32KIN_PIN_POS);
		break;
	default: /* do not touch HW */
		return -EINVAL;
	}

	if ((sys_read32(vbr_base + XEC_VBR_CS_OFS) & XEC_VBR_CS_MSK) != cken) {
		soc_mmcr_mask_set(vbr_base + XEC_VBR_CS_OFS, cken, XEC_VBR_CS_MSK);
	}

	return xec_cc_wait_pll_lock(dev);
}

static int mec15xx_cc_init(const struct device *dev)
{
	const struct xec_pcr_config *xcfg = dev->config;
	struct xec_pcr_data *xdat = dev->data;
	uint32_t clk_flags = 0;
	int rc = 0;

	xdat->clkmon_status = 0;

	/* pin configuration lost on chip reset */
	pinctrl_apply_state(xcfg->pincfg, PINCTRL_STATE_DEFAULT);

	rc = mec15xx_clk32k_config(dev, xcfg->pll_src, xcfg->periph_src, clk_flags);
	if (rc != 0) {
		return rc;
	}

	soc_xec_pcr_cpu_clk_div_set(xcfg->core_clk_div);

	ht_clean(dev);

	return 0;
}
#else  /* if CONFIG_SOC_SERIES_MEC15XX */

static void xec_silosc_enable(const struct device *dev, uint8_t enable)
{
	const struct xec_pcr_config *drvcfg = dev->config;
	mem_addr_t vbr_base = drvcfg->vbr_base;

	if (enable != 0) {
		if (sys_test_bit(vbr_base + XEC_VBR_CS_OFS, XEC_VBR_CS_SO_EN_POS) == 0) {
			sys_set_bit(vbr_base + XEC_VBR_CS_OFS, XEC_VBR_CS_SO_EN_POS);
			ht_wait_ready(dev, 10u, NULL); /* ~300 us delay */
		}
	} else {
		sys_clear_bit(vbr_base + XEC_VBR_CS_OFS, XEC_VBR_CS_SO_EN_POS);
	}
}

static bool xec_periph_requires_xtal(uint8_t periph_clk_source)
{
	if ((periph_clk_source == XEC_VBR_CS_PCS_XTAL) ||
	    (periph_clk_source == XEC_VBR_CS_PCS_PIN_XTAL)) {
		return true;
	}

	return false;
}

static bool xec_periph_requires_silosc(uint8_t periph_clk_source)
{
	if ((periph_clk_source == XEC_VBR_CS_PCS_SI) ||
	    (periph_clk_source == XEC_VBR_CS_PCS_PIN_SI)) {
		return true;
	}

	return false;
}

static bool xec_pll_requires_xtal(uint8_t pll_clk_source)
{
	if (pll_clk_source == XEC_CC_VTR_CS_PLL_SEL_XTAL) {
		return true;
	}

	return false;
}

static bool xec_pll_requires_silosc(uint8_t pll_clk_source)
{
	if (pll_clk_source == XEC_CC_VTR_CS_PLL_SEL_SI) {
		return true;
	}

	return false;
}

static bool xec_is_xtal_enabled(const struct device *dev)
{
	const struct xec_pcr_config *drvcfg = dev->config;
	mem_addr_t vbr_base = drvcfg->vbr_base;
	uint32_t vbcs = sys_read32(vbr_base + XEC_VBR_CS_OFS);

	if (vbcs & BIT(XEC_VBR_CS_XSTA_POS)) { /* xtal started? */
		return true;
	}

	return false;
}

static void connect_periph_32k_source(const struct device *dev, uint8_t periph_src)
{
	const struct xec_pcr_config *drvcfg = dev->config;
	mem_addr_t vbr_base = drvcfg->vbr_base;
	uint32_t r = sys_read32(vbr_base + XEC_VBR_CS_OFS);

	if (XEC_VBR_CS_PCS_GET(r) == (uint32_t)periph_src) {
		return;
	}

	r &= ~(XEC_VBR_CS_PCS_MSK);
	r |= XEC_VBR_CS_PCS_SET(periph_src);
	sys_write32(r, vbr_base + XEC_VBR_CS_OFS);
}

/* When PLL source is changed:
 * 1. HW switches to ring oscillator
 * 2. HW restarts PLL with new source.
 * 3. When PLL lock becomes true HW swithes from ring oscillator to PLL.
 * During 1 & 2 the CPU/PCR clock source is the ring oscillator.
 * This means the clock monitor measurements will not be accurate until PLL is locked.
 */
static void connect_pll_32k_source(const struct device *dev, uint8_t pll_src)
{
	const struct xec_pcr_config *drvcfg = dev->config;
	mem_addr_t pcr_base = drvcfg->pcr_base;
	uint32_t r = sys_read32(pcr_base + XEC_CC_VTR_CS_OFS);

	if (XEC_CC_VTR_CS_PLL_SEL_GET(r) == (uint32_t)pll_src) {
		return;
	}

	r &= ~(XEC_CC_VTR_CS_PLL_SEL_MSK);
	r |= XEC_CC_VTR_CS_PLL_SEL_SET((uint32_t)pll_src);
	sys_write32(r, pcr_base + XEC_CC_VTR_CS_OFS);
}

static void xec_cc_xtal_startup_current(const struct device *dev, uint8_t high_current)
{
	const struct xec_pcr_config *drvcfg = dev->config;
	mem_addr_t vbr_base = drvcfg->vbr_base;

	if (high_current != 0) {
		sys_clear_bit(vbr_base + XEC_VBR_CS_OFS, XEC_VBR_CS_XDHSC_POS);
	} else { /* set high startup current disable bit */
		sys_set_bit(vbr_base + XEC_VBR_CS_OFS, XEC_VBR_CS_XDHSC_POS);
	}
}

static void xec_cc_xtal_init(const struct device *dev)
{
	const struct xec_pcr_config *drvcfg = dev->config;
	mem_addr_t vbr_base = drvcfg->vbr_base;
	uint32_t r = 0;

	/* clkmon HW requires silicon OSC for checking XTAL health */
	xec_silosc_enable(dev, 1u);

	/* Make sure we are using internal silicon 32KHz OSC required by clkmon */
	connect_periph_32k_source(dev, XEC_VBR_CS_PCS_SI);
	connect_pll_32k_source(dev, XEC_CC_VTR_CS_PLL_SEL_SI);

	/* Enable XTAL with high startup current not disabled */
	r = sys_read32(vbr_base + XEC_VBR_CS_OFS);
	r &= ~BIT(XEC_VBR_CS_XDHSC_POS);

	if ((drvcfg->clk_flags & BIT(XEC_DT_CLK_FLAG_XTAL_SE_POS)) != 0) {
		r |= BIT(XEC_VBR_CS_XSE_POS); /* single-ended XTAL connection to XTAL2 pin */
	}

	if ((drvcfg->clk_flags & BIT(XEC_DT_CLK_FLAG_HAS_XG_POS)) != 0) {
		r |= XEC_VBR_CS_XG_SET((uint32_t)drvcfg->xtal_gain);
	}

	r |= BIT(XEC_VBR_CS_XSTA_POS);

	sys_write32(r, vbr_base + XEC_VBR_CS_OFS); /* start XTAL */

	ht_wait_ready_ms(dev, drvcfg->xtal_enable_delay_ms, NULL); /* startup delay */
}

static void xec_clkmon_enable(const struct device *dev, uint8_t chk_msk)
{
	const struct xec_pcr_config *drvcfg = dev->config;
	mem_addr_t pcr_base = drvcfg->pcr_base;

	sys_write32(BIT(XEC_CC_32K_CCR_CLR_POS), pcr_base + XEC_CC_32K_CCR_OFS);
	sys_write32(XEC_CC_32K_SR_IER_ALL_MSK, pcr_base + XEC_CC_32K_SR_OFS);
	sys_write32(chk_msk, pcr_base + XEC_CC_32K_CCR_OFS);
}

static void xec_clkmon_disable(const struct device *dev, uint8_t flags)
{
	const struct xec_pcr_config *drvcfg = dev->config;
	mem_addr_t pcr_base = drvcfg->pcr_base;
	uint32_t ctrl_val = 0;

	if ((flags & BIT(0)) != 0) { /* clear counters? */
		ctrl_val |= BIT(XEC_CC_32K_CCR_CLR_POS);
	}

	sys_write32(ctrl_val, pcr_base + XEC_CC_32K_CCR_OFS);

	if ((flags & BIT(1)) != 0) { /* clear status */
		sys_write32(UINT32_MAX, pcr_base + XEC_CC_32K_SR_OFS);
	}
}

static void xec_clkmon_init(const struct device *dev, uint8_t measure_silosc)
{
	const struct xec_pcr_config *drvcfg = dev->config;
	mem_addr_t pcr_base = drvcfg->pcr_base;
	uint16_t temp = 0;

	/* disable, clear read-only counters, and latched status bits */
	sys_write32(BIT(XEC_CC_32K_CCR_CLR_POS), pcr_base + XEC_CC_32K_CCR_OFS);
	sys_write32(0, pcr_base + XEC_CC_32K_IER_OFS);
	sys_write32(XEC_CC_32K_SR_IER_ALL_MSK, pcr_base + XEC_CC_32K_SR_OFS);

	if (measure_silosc != 0) { /* measure internal silosc instead of XTAL */
		sys_set_bit(pcr_base + XEC_CC_32K_CCR_OFS, XEC_CC_32K_CCR_CLR_POS);
	}

	temp = sys_read16(pcr_base + XEC_CC_32K_MIN_PER_CNT_OFS);
	if (temp == 0) {
		temp = (drvcfg->period_min != 0) ? drvcfg->period_min : CNT32K_TMIN;
		sys_write16(temp, pcr_base + XEC_CC_32K_MIN_PER_CNT_OFS);
	}

	temp = sys_read16(pcr_base + XEC_CC_32K_MAX_PER_CNT_OFS);
	if (temp == 0) {
		temp = (drvcfg->period_max != 0) ? drvcfg->period_max : CNT32K_TMAX;
		sys_write16(temp, pcr_base + XEC_CC_32K_MAX_PER_CNT_OFS);
	}

	temp = sys_read16(pcr_base + XEC_CC_32K_MAX_DC_VAR_OFS);
	if (temp == 0) {
		temp = (drvcfg->max_dc_va != 0) ? drvcfg->max_dc_va : CNT32K_DUTY_MAX;
		sys_write16(temp, pcr_base + XEC_CC_32K_MAX_DC_VAR_OFS);
	}

	temp = sys_read8(pcr_base + XEC_CC_32K_MIN_VAL_CNT_OFS);
	if (temp == 0) {
		temp = (drvcfg->min_valid != 0) ? drvcfg->min_valid : CNT32K_VALID_MIN;
		sys_write8((uint8_t)temp, pcr_base + XEC_CC_32K_MIN_VAL_CNT_OFS);
	}
}

static int xec_cc_xtal_health_check(const struct device *dev)
{
	const struct xec_pcr_config *drvcfg = dev->config;
	struct xec_pcr_data *data = dev->data;
	mem_addr_t pcr_base = drvcfg->pcr_base;
	uint32_t r = 0, all_msk = 0, pass_msk = 0, err_msk = 0, ms_count = 0;
	int rc = 0;
	bool xtal_ok = false;
	uint8_t mon_msk = (BIT(XEC_CC_32K_CCR_PER_CNT_EN_POS) | BIT(XEC_CC_32K_VAL_CNT_EN_POS) |
			   BIT(XEC_CC_32K_CCR_DC_CNT_EN_POS));

	all_msk = XEC_CC_32K_SR_IER_ALL_MSK;
	pass_msk = BIT(XEC_CC_32K_SR_PULSE_RDY_POS) | BIT(XEC_CC_32K_SR_PER_PASS_POS) |
		   BIT(XEC_CC_32K_SR_DC_PASS_POS) | BIT(XEC_CC_32K_SR_VALID_POS);
	err_msk = BIT(XEC_CC_32K_SR_FAIL_POS) | BIT(XEC_CC_32K_SR_PER_OVFL_POS) |
		  BIT(XEC_CC_32K_SR_UNWELL_POS);

	xec_clkmon_init(dev, 0); /* init for measuring XTAL */

	xec_cc_xtal_init(dev);

	xec_clkmon_enable(dev, mon_msk);

	while (1) {
		ht_wait_ready_ms(dev, 1u, NULL);
		ms_count++;

		r = sys_read32(pcr_base + XEC_CC_32K_SR_OFS);

		if ((r & BIT(XEC_CC_32K_SR_PULSE_RDY_POS)) != 0) { /* RO counters updated? */
			/* clear latched HW status bits */
			sys_write32(UINT32_MAX, pcr_base + XEC_CC_32K_SR_OFS);

			if (r == pass_msk) {
				xtal_ok = true;
				break;
			}
		}

		if ((drvcfg->xtal_mon_max_ms != 0) && (ms_count >= drvcfg->xtal_mon_max_ms)) {
			rc = -ETIMEDOUT;
			break;
		}
	};

	data->clkmon_status = (uint8_t)(r & 0xffu);

	sys_write32(BIT(XEC_CC_32K_CCR_CLR_POS), pcr_base + XEC_CC_32K_CCR_OFS);

	return rc;
}

/* ---- Custom API Implementation ---- */
/* Configure PLL clock sources and switch to required sources
 * API meant to called in early SoC initialization
 * Scenarios:
 * 1. VBAT POR - lost all clock configuration in VBAT clock select register
 *    If XTAL required
 *      config VBAT clock select for XTAL
 *      enable XTAL
 *      config monitor and check XTAL
 *    else 32KHZ_IN pin required
 *      configure pin via pinctrl
 *      config VBAT clock select for 32KHZ_IN pin
 */
static int z_mchp_xec_pcr_vb_pll_init(void)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	const struct xec_pcr_config *drvcfg = dev->config;
	struct xec_pcr_data *data = dev->data;
	mem_addr_t pcr_base = drvcfg->pcr_base;
	mem_addr_t vbr_base = drvcfg->vbr_base;
	uint8_t pll_src = 0, periph_src = 0;
	bool req_silosc = false;
	bool req_xtal = false;
	bool xtal_start_seq = false;
	int rc = 0;

	data->clkmon_status = 0;

	/* pin configuration lost on chip reset */
	pinctrl_apply_state(drvcfg->pincfg, PINCTRL_STATE_DEFAULT);

	xec_clkmon_disable(dev, 0x3u); /* disable and clear PCR clock monitor */

	pll_src = drvcfg->pll_src;
	periph_src = drvcfg->periph_src;

	if ((xec_periph_requires_xtal(periph_src) == true) ||
	    (xec_pll_requires_xtal(pll_src) == true)) {
		req_xtal = true;
	}

	if ((xec_periph_requires_silosc(periph_src) == true) ||
	    (xec_pll_requires_silosc(pll_src) == true)) {
		req_silosc = true;
		xec_silosc_enable(dev, 1u);
	}

	if ((req_xtal == true) && (xec_is_xtal_enabled(dev) == false)) {
		xtal_start_seq = true;

		rc = xec_cc_xtal_health_check(dev);

		if (rc != 0) { /* XTAL config failed */
			/* XTAL startup failed. Fallback to internal silicon OSC */
			pll_src = XEC_CC_VTR_CS_PLL_SEL_SI;
			periph_src = XEC_VBR_CS_PCS_SI;
			req_silosc = true;
		}
	}

	connect_periph_32k_source(dev, periph_src);
	connect_pll_32k_source(dev, pll_src);

	if ((drvcfg->clk_flags & BIT(XEC_DT_CLK_FLAG_SILOSC_DIS_POS)) != 0) {
		if (req_silosc == false) {
			sys_clear_bit(vbr_base + XEC_VBR_CS_OFS, XEC_VBR_CS_SO_EN_POS);
		}
	}

	if ((drvcfg->clk_flags & BIT(XEC_DT_CLK_FLAG_PLL_SRC_LOCK_POS)) != 0) {
		sys_set_bit(pcr_base + XEC_CC_VTR_CS_OFS, XEC_CC_VTR_CS_PLL_LOCKED_POS);
	}

	if ((drvcfg->clk_flags & BIT(XEC_DT_CLK_FLAG_SILOSC_EN_LOCK_POS)) != 0) {
		sys_set_bit(vbr_base + XEC_VBR_CS_OFS, XEC_VBR_CS_SO_LOCK_POS);
	}

	rc = xec_cc_wait_pll_lock(dev);

	if ((rc == 0) && (xtal_start_seq == true) &&
	    ((drvcfg->clk_flags & BIT(XEC_DT_CLK_FLAG_XTAL_DHSC_POS)) != 0)) {
		xec_cc_xtal_startup_current(dev, 0);
	}

	ht_clean(dev);

	return rc;
}
#endif /* CONFIG_SOC_SERIES_MEC15XX */

/* ---- API implementation ---- */

/* We only allow turning on peripheral slow clock all other domains are always on.
 * SilOSC and XTAL have been enabled if required by DT during driver initialization.
 * We do not recommend touching them.
 * For always on sources return success.
 */
static int xec_cc_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct xec_pcr_config *drvcfg = dev->config;
	mm_reg_t pcr_base = drvcfg->pcr_base;
	union clock_mchp_xec_subsys *xsubsys = (union clock_mchp_xec_subsys *)sys;
	uint32_t clk_div = 0;

	if (xsubsys == NULL) {
		return -EINVAL;
	}

	if (xsubsys->bits.bus == MCHP_XEC_PCR_CLK_PERIPH_SLOW) {
		clk_div = XEC_CC_SCLK_CR_DIV_SET(xsubsys->bits.pcr_data);
		if (clk_div == 0) {
			return -EINVAL;
		}
		soc_mmcr_mask_set(pcr_base + XEC_CC_SCLK_CR_OFS, clk_div, XEC_CC_SCLK_CR_DIV_MSK);
	}

	return 0;
}

/* We only allow turning off peripheral slow clock.
 * Disabling SilOSC and XTAL together will result in SoC running on dumb-ring.
 * If the XTAL is on, PLL and peripheral sources set to XTAL then you could turn off SilOSC.
 * We don't recommend implementing SilOSC and XTAL disable at this time.
 */
static int xec_cc_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct xec_pcr_config *drvcfg = dev->config;
	union clock_mchp_xec_subsys *xsubsys = (union clock_mchp_xec_subsys *)sys;

	if (xsubsys == NULL) {
		return -EINVAL;
	}

	if (xsubsys->bits.bus == MCHP_XEC_PCR_CLK_PERIPH_SLOW) {
		sys_write32(0, drvcfg->pcr_base + XEC_CC_SCLK_CR_OFS);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int xec_cc_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	const struct xec_pcr_config *drvcfg = dev->config;
	union clock_mchp_xec_subsys *xsubsys = (union clock_mchp_xec_subsys *)sys;
	uint32_t r = 0, turbo_clock = 0, clk_div = 0;

	if (rate == NULL) {
		return 0;
	}

	turbo_clock = get_turbo_clock(dev);

	switch (xsubsys->bits.bus) {
	case MCHP_XEC_PCR_CLK_CORE: /* SRAM and input to CPU core */
		__fallthrough;
	case MCHP_XEC_PCR_CLK_PERIPH_FAST: /* QSPI and crypto */
		*rate = turbo_clock;
		break;
	case MCHP_XEC_PCR_CLK_CPU: /* CPU core divided down from CLK_CORE */
		r = sys_read32(drvcfg->pcr_base + XEC_CC_PCLK_CR_OFS);
		clk_div = XEC_CC_PCLK_CR_DIV_GET(r);
		if (clk_div == 0) { /* If this field is 0 the CPU is not running! */
			clk_div = 1U;
		}
		*rate = turbo_clock / clk_div;
		break;
	case MCHP_XEC_PCR_CLK_BUS: /* AHB */
		__fallthrough;
	case MCHP_XEC_PCR_CLK_PERIPH: /* most peripherals */
		*rate = MHZ(48);
		break;
	case MCHP_XEC_PCR_CLK_PERIPH_SLOW:
		r = sys_read32(drvcfg->pcr_base + XEC_CC_SCLK_CR_OFS);
		clk_div = XEC_CC_SCLK_CR_DIV_GET(r);
		if (clk_div != 0) {
			*rate = KHZ(100) / clk_div;
		} else { /* Off */
			*rate = 0;
		}
		break;
	case MCHP_XEC_PCR_CLK_XTAL:
		*rate = XEC_XTAL_FREQ;
		break;
#ifdef CONFIG_HAS_MCHP_MEC_I3C
	case MCHP_XEC_PCR_CLK_I3C:
		*rate = MHZ(192);
		break;
#endif
	default:
		return -EINVAL;
	};

	return 0;
}

/* If PCR PLL is locked then those clocks depending on the PLL are on.
 * Else the internal dumb ring is being used which varies based on process and temperature, report
 * unknown clock status.
 * Slow clock status is on/off based on PCR slow clock divider register
 * XTAL is on/off based on VBAT register bank 32KHz clock control registers.
 * NOTE: we could report XTAL as starting if async API to start it was called
 * and health checks are not complete or bad.
 */
static enum clock_control_status xec_cc_get_status(const struct device *dev,
						   clock_control_subsys_t sys)
{
	const struct xec_pcr_config *drvcfg = dev->config;
	uint32_t r = 0, clk_div = 0;
	enum clock_control_status cc_status = CLOCK_CONTROL_STATUS_UNKNOWN;
	union clock_mchp_xec_subsys *xsubsys = (union clock_mchp_xec_subsys *)sys;
	bool pll_locked = false;

	if (xsubsys == NULL) {
		return -EINVAL;
	}

	if (sys_test_bit(drvcfg->pcr_base + XEC_CC_OSC_ID_OFS, XEC_CC_OSC_ID_PLL_LOCK_POS) != 0) {
		pll_locked = true;
	}

	switch (xsubsys->bits.bus) {
	case MCHP_XEC_PCR_CLK_CORE:
		__fallthrough;
	case MCHP_XEC_PCR_CLK_PERIPH_FAST:
		__fallthrough;
	case MCHP_XEC_PCR_CLK_CPU:
		__fallthrough;
	case MCHP_XEC_PCR_CLK_BUS:
		__fallthrough;
#ifdef CONFIG_HAS_MCHP_MEC_I3C
	case MCHP_XEC_PCR_CLK_I3C:
		__fallthrough;
#endif
	case MCHP_XEC_PCR_CLK_PERIPH:
		if (pll_locked) {
			cc_status = CLOCK_CONTROL_STATUS_ON;
		}
		break;
	case MCHP_XEC_PCR_CLK_PERIPH_SLOW:
		cc_status = CLOCK_CONTROL_STATUS_ON;
		r = sys_read32(drvcfg->pcr_base + XEC_CC_SCLK_CR_OFS);
		clk_div = XEC_CC_SCLK_CR_DIV_GET(r);
		if (clk_div == 0) {
			cc_status = CLOCK_CONTROL_STATUS_OFF;
		}
		break;
	case MCHP_XEC_PCR_CLK_XTAL:
		cc_status = CLOCK_CONTROL_STATUS_OFF;
		if (xtal_enabled(dev) != 0) {
			/* TODO - it is on but is it good? */
			cc_status = CLOCK_CONTROL_STATUS_ON;
		}
		break;
	default:
		return -EINVAL;
	};

	return cc_status;
}

/* We can only change frequencies of MCHP_XEC_PCR_CLK_CPU or MCHP_XEC_PCR_CLK_PERIPH_SLOW */
static int xec_cc_set_rate(const struct device *dev, clock_control_subsys_t sys,
			   clock_control_subsys_rate_t rate)
{
	const struct xec_pcr_config *drvcfg = dev->config;
	mm_reg_t pcr_base = drvcfg->pcr_base;
	union clock_mchp_xec_subsys *xsubsys = (union clock_mchp_xec_subsys *)sys;
	uint32_t clk_div = 0;

	if (xsubsys == NULL) {
		return -EINVAL;
	}

	if (xsubsys->bits.bus == MCHP_XEC_PCR_CLK_CPU) {
		clk_div = xsubsys->bits.pcr_data & MCHP_XEC_CLK_CPU_MASK;
		if (clk_div == 0) {
			return -EINVAL;
		}
		clk_div = XEC_CC_PCLK_CR_DIV_SET(clk_div);
		soc_mmcr_mask_set(pcr_base + XEC_CC_PCLK_CR_OFS, clk_div, XEC_CC_PCLK_CR_DIV_MSK);
	} else if (xsubsys->bits.bus == MCHP_XEC_PCR_CLK_PERIPH_SLOW) {
		clk_div = XEC_CC_SCLK_CR_DIV_SET(xsubsys->bits.pcr_data);
		soc_mmcr_mask_set(pcr_base + XEC_CC_SCLK_CR_OFS, clk_div, XEC_CC_SCLK_CR_DIV_MSK);
	} else {
		return -EINVAL;
	}

	return 0;
}

/* The following are kept for now and will be eventually removed when all MEC15xx/172x drivers
 * are converted to call the Microchip SoC layer directly.
 */
/*
 * PCR peripheral sleep enable allows the clocks to a specific peripheral to
 * be gated off if the peripheral is not requesting a clock.
 * slp_idx = zero based index into 32-bit PCR sleep enable registers.
 * slp_pos = bit position in the register
 * slp_en if non-zero set the bit else clear the bit
 */
int z_mchp_xec_pcr_periph_sleep(uint8_t slp_idx, uint8_t slp_pos, uint8_t slp_en)
{
	uint8_t enc_pcr_scr = 0;

	if ((slp_idx >= XEC_CC_PCR_MAX_SCR) || (slp_pos > 31U)) {
		return -EINVAL;
	}

	enc_pcr_scr = MCHP_XEC_ENC_PCR_SCR(slp_idx, slp_pos);

	if (slp_en != 0) {
		soc_xec_pcr_sleep_en_set(enc_pcr_scr);
	} else {
		soc_xec_pcr_sleep_en_clear(enc_pcr_scr);
	}

	return 0;
}

/* Most peripherals have a write only reset bit in the PCR reset enable registers.
 * The layout of these registers is identical to the PCR sleep enable registers.
 * Reset enables are protected by a lock register.
 */
int z_mchp_xec_pcr_periph_reset(uint8_t slp_idx, uint8_t slp_pos)
{
	if ((slp_idx >= XEC_CC_PCR_MAX_SCR) || (slp_pos > 31U)) {
		return -EINVAL;
	}

	soc_xec_pcr_reset_en(MCHP_XEC_ENC_PCR_SCR(slp_idx, slp_pos));

	return 0;
}

/* Driver initialization*/
static int xec_clock_control_init(const struct device *dev)
{
#if defined(CONFIG_SOC_SERIES_MEC15XX)
	return mec15xx_cc_init(dev);

#else
	const struct xec_pcr_config *drvcfg = dev->config;

	sys_write32(0, drvcfg->pcr_base + XEC_CC_32K_IER_OFS);
	soc_ecia_girq_ctrl(drvcfg->pcr_girq, drvcfg->pcr_girq_pos, 0);
	soc_ecia_girq_status_clear(drvcfg->pcr_girq, drvcfg->pcr_girq_pos);

	return z_mchp_xec_pcr_vb_pll_init();
#endif
}

static DEVICE_API(clock_control, xec_clock_control_api) = {
	.on = xec_cc_on,
	.off = xec_cc_off,
	.get_rate = xec_cc_get_rate,
	.get_status = xec_cc_get_status,
	.set_rate = xec_cc_set_rate,
};

#define XEC_PLL_32K_SRC(i) (enum pll_clk32k_src) DT_INST_PROP_OR(i, pll_32k_src, PLL_CLK32K_SRC_SO)

#define XEC_PERIPH_32K_SRC(i)                                                                      \
	(enum periph_clk32k_src) DT_INST_PROP_OR(0, periph_32k_src, PERIPH_CLK32K_SRC_SO_SO)

#ifdef CONFIG_SOC_SERIES_MEC15XX
#define XEC_PCR_GIRQ_DT(inst, idx)     255
#define XEC_PCR_GIRQ_POS_DT(inst, idx) 255
#else
#define XEC_PCR_GIRQ_DT(inst, idx)                                                                 \
	(uint8_t)MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(inst, girqs, idx))
#define XEC_PCR_GIRQ_POS_DT(inst, idx)                                                             \
	(uint8_t)MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(inst, girqs, idx))
#endif

#define HT_NODE     DT_INST_PHANDLE(0, hibtimer_handle)
#define HT_REG_BASE DT_REG_ADDR(HT_NODE)
#define HT_GIRQ     (uint8_t)MCHP_XEC_ECIA_GIRQ(DT_PROP_BY_IDX(HT_NODE, girqs, 0))
#define HT_GIRQ_POS (uint8_t)MCHP_XEC_ECIA_GIRQ_POS(DT_PROP_BY_IDX(HT_NODE, girqs, 0))

#ifdef CONFIG_SOC_SERIES_MEC15XX
#define XTAL_GAIN(inst) 0u

#define XEC_DT_CLK_FLAGS(i) (DT_INST_PROP_OR(i, xtal_single_ended, 0) & BIT(0))

#else
#define XEC_HAS_XTAL_GAIN(inst) DT_INST_NODE_HAS_PROP(inst, xtal_gain)

#define XEC_XTAL_GAIN_FLAG(inst)                                                                   \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, xtal_gain), (BIT(XEC_DT_CLK_FLAG_HAS_XG_POS)), (0))

#define XTAL_GAIN(inst) (uint32_t)DT_INST_ENUM_IDX_OR(inst, xtal_gain, XEC_VBR_CS_XG_2X)

#define XEC_DT_CLK_FLAGS(i)                                                                        \
	((DT_INST_PROP_OR(i, xtal_single_ended, 0) & BIT(0)) |                                     \
	 ((DT_INST_PROP_OR(i, pll_src_lock, 0) & BIT(0)) << 1) |                                   \
	 ((DT_INST_PROP_OR(i, silosc_en_lock, 0) & BIT(0)) << 2) |                                 \
	 ((DT_INST_PROP_OR(i, internal_osc_disable, 0) & BIT(0)) << 3) |                           \
	 ((DT_INST_PROP_OR(i, xtal_hi_startup_current_disable, 0) & BIT(0)) << 4) |                \
	 (XEC_XTAL_GAIN_FLAG(i)))
#endif

static struct xec_pcr_data pcr_xec_data;

PINCTRL_DT_INST_DEFINE(0);

const struct xec_pcr_config pcr_xec_config = {
	.pcr_base = DT_INST_REG_ADDR_BY_IDX(0, 0),
	.vbr_base = DT_INST_REG_ADDR_BY_IDX(0, 1),
	.htmr_base = HT_REG_BASE,
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.xtal_enable_delay_ms =
		(uint16_t)DT_INST_PROP_OR(0, xtal_enable_delay_ms, XEC_CC_XTAL_EN_DELAY_MS_DFLT),
	.xtal_mon_max_ms = (uint16_t)DT_INST_PROP_OR(0, xtal_monitor_max_ms, 0),
	.pll_lock_timeout_ms =
		(uint16_t)DT_INST_PROP_OR(0, pll_lock_timeout_ms, XEC_CC_DFLT_PLL_LOCK_WAIT_MS),
	.period_min = (uint16_t)DT_INST_PROP_OR(0, clk32kmon_period_min, CNT32K_TMIN),
	.period_max = (uint16_t)DT_INST_PROP_OR(0, clk32kmon_period_max, CNT32K_TMAX),
	.max_dc_va = (uint16_t)DT_INST_PROP_OR(0, clk32kmon_duty_cycle_var_max, CNT32K_DUTY_MAX),
	.min_valid = (uint8_t)DT_INST_PROP_OR(0, clk32kmon_valid_min, CNT32K_VAL_MIN),
	.core_clk_div = (uint8_t)DT_INST_PROP_OR(0, core_clk_div, CONFIG_SOC_MEC_PROC_CLK_DIV),
	.pcr_girq = XEC_PCR_GIRQ_DT(0, 0),
	.pcr_girq_pos = XEC_PCR_GIRQ_POS_DT(0, 0),
	.htmr_girq = HT_GIRQ,
	.htmr_girq_pos = HT_GIRQ_POS,
	.periph_src = (uint8_t)DT_INST_PROP_OR(0, periph_32k_src, XEC_VBR_CS_PCS_SI),
	.pll_src = (uint8_t)DT_INST_PROP_OR(0, pll_32k_src, XEC_CC_VTR_CS_PLL_SEL_SI),
	.xtal_gain = (uint8_t)XTAL_GAIN(0),
	.clk_flags = XEC_DT_CLK_FLAGS(0),
};

DEVICE_DT_INST_DEFINE(0, xec_clock_control_init, NULL, &pcr_xec_data, &pcr_xec_config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &xec_clock_control_api);
