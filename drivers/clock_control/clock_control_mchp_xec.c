/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_pcr

#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/clock/mchp_xec_pcr.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_xec, LOG_LEVEL_ERR);

#define CLK32K_SIL_OSC_DELAY		256
#define CLK32K_PLL_LOCK_WAIT		(16 * 1024)
#define CLK32K_PIN_WAIT			4096
#define CLK32K_XTAL_WAIT		(16 * 1024)
#define CLK32K_XTAL_MON_WAIT		(64 * 1024)
#define XEC_CC_DFLT_PLL_LOCK_WAIT_MS	30

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
#define CNT32K_TMIN			1435
#define CNT32K_TMAX			1495
#define CNT32K_DUTY_MAX			132
#define CNT32K_VAL_MIN			4

#define DEST_PLL			0
#define DEST_PERIPH			1

#define CLK32K_FLAG_CRYSTAL_SE		BIT(0)
#define CLK32K_FLAG_PIN_FB_CRYSTAL	BIT(1)

#define PCR_PERIPH_RESET_SPIN		8u

#define XEC_CC_XTAL_EN_DELAY_MS_DFLT	300u
#define HIBTIMER_MS_TO_CNT(x)		((uint32_t)(x) * 33U)

#define HIBTIMER_10_MS			328u
#define HIBTIMER_300_MS			9830u

enum pll_clk32k_src {
	PLL_CLK32K_SRC_SO = MCHP_XEC_PLL_CLK32K_SRC_SIL_OSC,
	PLL_CLK32K_SRC_XTAL = MCHP_XEC_PLL_CLK32K_SRC_XTAL,
	PLL_CLK32K_SRC_PIN = MCHP_XEC_PLL_CLK32K_SRC_PIN,
	PLL_CLK32K_SRC_MAX,
};

enum periph_clk32k_src {
	PERIPH_CLK32K_SRC_SO_SO = MCHP_XEC_PERIPH_CLK32K_SRC_SO_SO,
	PERIPH_CLK32K_SRC_XTAL_XTAL = MCHP_XEC_PERIPH_CLK32K_SRC_XTAL_XTAL,
	PERIPH_CLK32K_SRC_PIN_SO = MCHP_XEC_PERIPH_CLK32K_SRC_PIN_SO,
	PERIPH_CLK32K_SRC_PIN_XTAL = MCHP_XEC_PERIPH_CLK32K_SRC_PIN_XTAL,
	PERIPH_CLK32K_SRC_MAX
};

enum clk32k_dest { CLK32K_DEST_PLL = 0, CLK32K_DEST_PERIPH, CLK32K_DEST_MAX };

/* PCR hardware registers for MEC15xx and MEC172x */
#define XEC_CC_PCR_MAX_SCR 5

struct pcr_hw_regs {
	volatile uint32_t SYS_SLP_CTRL;
	volatile uint32_t PROC_CLK_CTRL;
	volatile uint32_t SLOW_CLK_CTRL;
	volatile uint32_t OSC_ID;
	volatile uint32_t PWR_RST_STS;
	volatile uint32_t PWR_RST_CTRL;
	volatile uint32_t SYS_RST;
	volatile uint32_t TURBO_CLK; /* MEC172x only */
	volatile uint32_t TEST20;
	uint32_t RSVD1[3];
	volatile uint32_t SLP_EN[XEC_CC_PCR_MAX_SCR];
	uint32_t RSVD2[3];
	volatile uint32_t CLK_REQ[XEC_CC_PCR_MAX_SCR];
	uint32_t RSVD3[3];
	volatile uint32_t RST_EN[5];
	volatile uint32_t RST_EN_LOCK;
	/* all registers below are MEC172x only */
	volatile uint32_t VBAT_SRST;
	volatile uint32_t CLK32K_SRC_VTR;
	volatile uint32_t TEST90;
	uint32_t RSVD4[(0x00c0 - 0x0094) / 4];
	volatile uint32_t CNT32K_PER;
	volatile uint32_t CNT32K_PULSE_HI;
	volatile uint32_t CNT32K_PER_MIN;
	volatile uint32_t CNT32K_PER_MAX;
	volatile uint32_t CNT32K_DV;
	volatile uint32_t CNT32K_DV_MAX;
	volatile uint32_t CNT32K_VALID;
	volatile uint32_t CNT32K_VALID_MIN;
	volatile uint32_t CNT32K_CTRL;
	volatile uint32_t CLK32K_MON_ISTS;
	volatile uint32_t CLK32K_MON_IEN;
};

#define XEC_CC_PCR_OSC_ID_PLL_LOCK	BIT(8)
#define XEC_CC_PCR_TURBO_CLK_96M	BIT(2)

#define XEC_CC_PCR_CLK32K_SRC_MSK	0x3u
#define XEC_CC_PCR_CLK32K_SRC_SIL	0u
#define XEC_CC_PCR_CLK32K_SRC_XTAL	1
#define XEC_CC_PCR_CLK32K_SRC_PIN	2
#define XEC_CC_PCR_CLK32K_SRC_OFF	3

#ifdef CONFIG_SOC_SERIES_MEC1501X
#define XEC_CC_PCR3_CRYPTO_MASK		(BIT(26) | BIT(27) | BIT(28))
#else
#define XEC_CC_PCR3_CRYPTO_MASK		BIT(26)
#endif

/* VBAT powered hardware registers related to clock configuration */
struct vbatr_hw_regs {
	volatile uint32_t PFRS;
	uint32_t RSVD1[1];
	volatile uint32_t CLK32_SRC;
	uint32_t RSVD2[2];
	volatile uint32_t CLK32_TRIM;
	uint32_t RSVD3[1];
	volatile uint32_t CLK32_TRIM_CTRL;
};

/* MEC152x VBAT CLK32_SRC register defines */
#define XEC_CC15_VBATR_USE_SIL_OSC		0u
#define XEC_CC15_VBATR_USE_32KIN_PIN		BIT(1)
#define XEC_CC15_VBATR_USE_PAR_CRYSTAL		BIT(2)
#define XEC_CC15_VBATR_USE_SE_CRYSTAL		(BIT(2) | BIT(3))

/* MEC150x special requirements */
#define XEC_CC15_GCFG_DID_DEV_ID_MEC150x	0x0020U
#define XEC_CC15_TRIM_ENABLE_INT_OSCILLATOR	0x06U


/* MEC172x VBAT CLK32_SRC register defines */
#define XEC_CC_VBATR_CS_SO_EN			BIT(0) /* enable and start silicon OSC */
#define XEC_CC_VBATR_CS_XTAL_EN			BIT(8) /* enable & start external crystal */
#define XEC_CC_VBATR_CS_XTAL_SE			BIT(9) /* crystal XTAL2 used as 32KHz input */
#define XEC_CC_VBATR_CS_XTAL_DHC		BIT(10) /* disable high XTAL startup current */
#define XEC_CC_VBATR_CS_XTAL_CNTR_MSK		0x1800u /* XTAL amplifier gain control */
#define XEC_CC_VBATR_CS_XTAL_CNTR_DG		0x0800u
#define XEC_CC_VBATR_CS_XTAL_CNTR_RG		0x1000u
#define XEC_CC_VBATR_CS_XTAL_CNTR_MG		0x1800u
/* MEC172x Select source of peripheral 32KHz clock */
#define XEC_CC_VBATR_CS_PCS_POS			16
#define XEC_CC_VBATR_CS_PCS_MSK0		0x3u
#define XEC_CC_VBATR_CS_PCS_MSK			0x30000u
#define XEC_CC_VBATR_CS_PCS_VTR_VBAT_SO		0u /* VTR & VBAT use silicon OSC */
#define XEC_CC_VBATR_CS_PCS_VTR_VBAT_XTAL	0x10000u /* VTR & VBAT use crystal */
#define XEC_CC_VBATR_CS_PCS_VTR_PIN_SO		0x20000u /* VTR 32KHZ_IN, VBAT silicon OSC */
#define XEC_CC_VBATR_CS_PCS_VTR_PIN_XTAL	0x30000u /* VTR 32KHZ_IN, VBAT XTAL */
#define XEC_CC_VBATR_CS_DI32_VTR_OFF		BIT(18) /* disable silicon OSC when VTR off */

enum vbr_clk32k_src {
	VBR_CLK32K_SRC_SO_SO = 0,
	VBR_CLK32K_SRC_XTAL_XTAL,
	VBR_CLK32K_SRC_PIN_SO,
	VBR_CLK32K_SRC_PIN_XTAL,
	VBR_CLK32K_SRC_MAX,
};

/* GIRQ23 hardware registers */
#define XEC_CC_HTMR_0_GIRQ23_POS		16

/* Driver config */
struct xec_pcr_config {
	uintptr_t pcr_base;
	uintptr_t vbr_base;
	const struct pinctrl_dev_config *pcfg;
	uint16_t xtal_enable_delay_ms;
	uint16_t pll_lock_timeout_ms;
	uint16_t period_min; /* mix and max 32KHz period range */
	uint16_t period_max; /* monitor values in units of 48MHz (20.8 ns) */
	uint8_t core_clk_div; /* Cortex-M4 clock divider (CPU and NVIC) */
	uint8_t xtal_se; /* External 32KHz square wave on XTAL2 pin */
	uint8_t max_dc_va; /* 32KHz monitor maximum duty cycle variation */
	uint8_t min_valid; /* minimum number of valid consecutive 32KHz pulses */
	enum pll_clk32k_src pll_src;
	enum periph_clk32k_src periph_src;
	uint8_t clkmon_bypass;
	uint8_t dis_internal_osc;
};

/*
 * Make sure PCR sleep enables are clear except for crypto
 * which do not have internal clock gating.
 */
static void pcr_slp_init(struct pcr_hw_regs *pcr)
{
	pcr->SYS_SLP_CTRL = 0U;
	SCB->SCR &= ~BIT(2);

	for (int i = 0; i < XEC_CC_PCR_MAX_SCR; i++) {
		pcr->SLP_EN[i] = 0U;
	}

	pcr->SLP_EN[3] = XEC_CC_PCR3_CRYPTO_MASK;
}

/* MEC172x:
 * Check if PLL is locked with timeout provided by a peripheral clock domain
 * timer. We assume peripheral domain is still using internal silicon OSC as
 * its reference clock. Available peripheral timers using 32KHz are:
 * RTOS timer, hibernation timers, RTC, and week timer. We will use hibernation
 * timer 0 in 30.5 us tick mode. Maximum internal is 2 seconds.
 * A timer count value of 0 is interpreted as no timeout.
 * We use the hibernation timer GIRQ interrupt status bit instead of reading
 * the timer's count register due to race condition of HW taking at least
 * one 32KHz cycle to move pre-load into count register.
 * MEC15xx:
 * Hibernation timer is using the chosen 32KHz source. If the external 32KHz source
 * has a ramp up time, we make not get an accurate delay. This may only occur for
 * the parallel crystal.
 */
static int pll_wait_lock_periph(struct pcr_hw_regs *const pcr, uint16_t ms)
{
	struct htmr_regs *htmr0 = (struct htmr_regs *)DT_REG_ADDR(DT_NODELABEL(hibtimer0));
	struct girq_regs *girq23 = (struct girq_regs *)DT_REG_ADDR(DT_NODELABEL(girq23));
	uint32_t hcount = HIBTIMER_MS_TO_CNT(ms);
	int rc = 0;

	htmr0->PRLD = 0; /* disable */
	htmr0->CTRL = 0; /* 30.5 us units */
	girq23->SRC = BIT(XEC_CC_HTMR_0_GIRQ23_POS);
	htmr0->PRLD = hcount;
	while (!(pcr->OSC_ID & MCHP_PCR_OSC_ID_PLL_LOCK)) {
		if (hcount) {
			if (girq23->SRC & BIT(XEC_CC_HTMR_0_GIRQ23_POS)) {
				rc = -ETIMEDOUT;
			}
		}
	}

	return rc;
}

static int periph_clk_src_using_pin(enum periph_clk32k_src src)
{
	switch (src) {
	case PERIPH_CLK32K_SRC_PIN_SO:
	case PERIPH_CLK32K_SRC_PIN_XTAL:
		return 1;
	default:
		return 0;
	}
}

#ifdef CONFIG_SOC_SERIES_MEC1501X
/* MEC15xx uses the same 32KHz source for both PLL and Peripheral 32K clock domains.
 * We ignore the peripheral clock source.
 * If XTAL is selected (parallel) or single-ended the external 32KHz MUST stay on
 * even when when VTR goes off.
 * If PIN(32KHZ_IN pin) as the external source, hardware can auto-switch to internal
 * silicon OSC if the signal on the 32KHZ_PIN goes away.
 * We ignore th
 */
static int soc_clk32_init(const struct device *dev,
			  enum pll_clk32k_src pll_clk_src,
			  enum periph_clk32k_src periph_clk_src,
			  uint32_t flags)
{
	const struct xec_pcr_config * const devcfg = dev->config;
	struct pcr_hw_regs *const pcr = (struct pcr_hw_regs *)devcfg->pcr_base;
	struct vbatr_hw_regs *const vbr = (struct vbatr_hw_regs *)devcfg->vbr_base;
	uint32_t cken = 0U;
	int rc = 0;

	if (MCHP_DEVICE_ID() == XEC_CC15_GCFG_DID_DEV_ID_MEC150x) {
		if (MCHP_REVISION_ID() == MCHP_GCFG_REV_B0) {
			vbr->CLK32_TRIM_CTRL = XEC_CC15_TRIM_ENABLE_INT_OSCILLATOR;
		}
	}

	switch (pll_clk_src) {
	case PLL_CLK32K_SRC_SO:
		cken = XEC_CC15_VBATR_USE_SIL_OSC;
		break;
	case PLL_CLK32K_SRC_XTAL:
		if (flags & CLK32K_FLAG_CRYSTAL_SE) {
			cken = XEC_CC15_VBATR_USE_SE_CRYSTAL;
		} else {
			cken = XEC_CC15_VBATR_USE_PAR_CRYSTAL;
		}
		break;
	case PLL_CLK32K_SRC_PIN: /* 32KHZ_IN pin falls back to Silicon OSC */
		cken = XEC_CC15_VBATR_USE_32KIN_PIN;
		break;
	default: /* do not touch HW */
		return -EINVAL;
	}

	if ((vbr->CLK32_SRC & 0xffU) != cken) {
		vbr->CLK32_SRC = cken;
	}

	rc = pll_wait_lock_periph(pcr, devcfg->xtal_enable_delay_ms);

	return rc;
}
#else

static int periph_clk_src_using_si(enum periph_clk32k_src src)
{
	switch (src) {
	case PERIPH_CLK32K_SRC_SO_SO:
	case PERIPH_CLK32K_SRC_PIN_SO:
		return 1;
	default:
		return 0;
	}
}

static int periph_clk_src_using_xtal(enum periph_clk32k_src src)
{
	switch (src) {
	case PERIPH_CLK32K_SRC_XTAL_XTAL:
	case PERIPH_CLK32K_SRC_PIN_XTAL:
		return 1;
	default:
		return 0;
	}
}

static bool is_sil_osc_enabled(struct vbatr_hw_regs *vbr)
{
	if (vbr->CLK32_SRC & XEC_CC_VBATR_CS_SO_EN) {
		return true;
	}

	return false;
}

static void enable_sil_osc(struct vbatr_hw_regs *vbr)
{
	vbr->CLK32_SRC |= XEC_CC_VBATR_CS_SO_EN;
}

/* In early Zephyr initialization we don't have timer services. Also, the SoC
 * may be running on its ring oscillator (+/- 50% accuracy). Configuring the
 * SoC's clock subsystem requires wait/delays. We implement a simple delay
 * by writing to a read-only hardware register in the PCR block.
 */
static uint32_t spin_delay(struct pcr_hw_regs *pcr, uint32_t cnt)
{
	uint32_t n;

	for (n = 0U; n < cnt; n++) {
		pcr->OSC_ID = n;
	}

	return n;
}

/*
 * This routine checks if the PLL is locked to its input source. Minimum lock
 * time is 3.3 ms. Lock time can be larger when the source is an external
 * crystal. Crystal cold start times may vary greatly based on many factors.
 * Crystals do not like being power cycled.
 */
static int pll_wait_lock(struct pcr_hw_regs *const pcr, uint32_t wait_cnt)
{
	while (!(pcr->OSC_ID & MCHP_PCR_OSC_ID_PLL_LOCK)) {
		if (wait_cnt == 0) {
			return -ETIMEDOUT;
		}
		--wait_cnt;
	}

	return 0;
}

/* caller has enabled internal silicon 32 KHz oscillator */
static void hib_timer_delay(uint32_t hib_timer_count)
{
	struct htmr_regs *htmr0 = (struct htmr_regs *)DT_REG_ADDR(DT_NODELABEL(hibtimer0));
	struct girq_regs *girq23 = (struct girq_regs *)DT_REG_ADDR(DT_NODELABEL(girq23));
	uint32_t hcnt;

	while (hib_timer_count) {

		hcnt = hib_timer_count;
		if (hcnt > UINT16_MAX) {
			hcnt -= UINT16_MAX;
		}

		htmr0->PRLD = 0; /* disable */
		while (htmr0->PRLD != 0) {
			;
		}
		htmr0->CTRL = 0; /* 32k time base */
		/* clear hibernation timer 0 status */
		girq23->SRC = BIT(XEC_CC_HTMR_0_GIRQ23_POS);
		htmr0->PRLD = hib_timer_count;
		if (hib_timer_count == 0) {
			return;
		}

		while ((girq23->SRC & BIT(XEC_CC_HTMR_0_GIRQ23_POS)) == 0) {
			;
		}

		htmr0->PRLD = 0; /* disable */
		while (htmr0->PRLD != 0) {
			;
		}
		girq23->SRC = BIT(XEC_CC_HTMR_0_GIRQ23_POS);

		hib_timer_count -= hcnt;
	}
}

/* Turn off crystal when we are not using it */
static int disable_32k_crystal(const struct device *dev)
{
	const struct xec_pcr_config * const devcfg = dev->config;
	struct vbatr_hw_regs *const vbr = (struct vbatr_hw_regs *)devcfg->vbr_base;
	uint32_t vbcs = vbr->CLK32_SRC;

	vbcs &= ~(XEC_CC_VBATR_CS_XTAL_EN | XEC_CC_VBATR_CS_XTAL_SE | XEC_CC_VBATR_CS_XTAL_DHC);
	vbr->CLK32_SRC = vbcs;

	return 0;
}

/*
 * Start external 32 KHz crystal.
 * Assumes peripheral clocks source is Silicon OSC.
 * If current configuration matches desired crystal configuration do nothing.
 * NOTE: Crystal requires ~300 ms to stabilize.
 */
static int enable_32k_crystal(const struct device *dev, uint32_t flags)
{
	const struct xec_pcr_config * const devcfg = dev->config;
	struct vbatr_hw_regs *const vbr = (struct vbatr_hw_regs *)devcfg->vbr_base;
	uint32_t vbcs = vbr->CLK32_SRC;
	uint32_t cfg = MCHP_VBATR_CS_XTAL_EN;

	if (flags & CLK32K_FLAG_CRYSTAL_SE) {
		cfg |= MCHP_VBATR_CS_XTAL_SE;
	}

	if ((vbcs & cfg) == cfg) {
		return 0;
	}

	/* Configure crystal connection before enabling the crystal. */
	vbr->CLK32_SRC &= ~(MCHP_VBATR_CS_XTAL_SE | MCHP_VBATR_CS_XTAL_DHC |
			    MCHP_VBATR_CS_XTAL_CNTR_MSK);
	if (flags & CLK32K_FLAG_CRYSTAL_SE) {
		vbr->CLK32_SRC |= MCHP_VBATR_CS_XTAL_SE;
	}

	/* Set crystal gain */
	vbr->CLK32_SRC |= MCHP_VBATR_CS_XTAL_CNTR_DG;

	/* enable crystal */
	vbr->CLK32_SRC |= MCHP_VBATR_CS_XTAL_EN;
	/* wait for crystal stabilization */
	hib_timer_delay(HIBTIMER_MS_TO_CNT(devcfg->xtal_enable_delay_ms));
	/* turn off crystal high startup current */
	vbr->CLK32_SRC |= MCHP_VBATR_CS_XTAL_DHC;

	return 0;
}

/*
 * Use PCR clock monitor hardware to test crystal output.
 * Requires crystal to have stabilized after enable.
 * When enabled the clock monitor hardware measures high/low, edges, and
 * duty cycle and compares to programmed limits.
 */
static int check_32k_crystal(const struct device *dev)
{
	const struct xec_pcr_config * const devcfg = dev->config;
	struct pcr_hw_regs *const pcr = (struct pcr_hw_regs *)devcfg->pcr_base;
	struct htmr_regs *htmr0 = (struct htmr_regs *)DT_REG_ADDR(DT_NODELABEL(hibtimer0));
	struct girq_regs *girq23 = (struct girq_regs *)DT_REG_ADDR(DT_NODELABEL(girq23));
	uint32_t status = 0;
	int rc = 0;

	htmr0->PRLD = 0;
	htmr0->CTRL = 0;
	girq23->SRC = BIT(XEC_CC_HTMR_0_GIRQ23_POS);

	pcr->CNT32K_CTRL = 0U;
	pcr->CLK32K_MON_IEN = 0U;
	pcr->CLK32K_MON_ISTS = MCHP_PCR_CLK32M_ISTS_MASK;

	pcr->CNT32K_PER_MIN = devcfg->period_min;
	pcr->CNT32K_PER_MAX = devcfg->period_max;
	pcr->CNT32K_DV_MAX = devcfg->max_dc_va;
	pcr->CNT32K_VALID_MIN = devcfg->min_valid;

	pcr->CNT32K_CTRL =
		MCHP_PCR_CLK32M_CTRL_PER_EN | MCHP_PCR_CLK32M_CTRL_DC_EN |
		MCHP_PCR_CLK32M_CTRL_VAL_EN | MCHP_PCR_CLK32M_CTRL_CLR_CNT;

	rc = -ETIMEDOUT;
	htmr0->PRLD = HIBTIMER_10_MS;
	status = pcr->CLK32K_MON_ISTS;

	while ((girq23->SRC & BIT(XEC_CC_HTMR_0_GIRQ23_POS)) == 0) {
		if (status == (MCHP_PCR_CLK32M_ISTS_PULSE_RDY |
			       MCHP_PCR_CLK32M_ISTS_PASS_PER |
			       MCHP_PCR_CLK32M_ISTS_PASS_DC |
			       MCHP_PCR_CLK32M_ISTS_VALID)) {
			rc = 0;
			break;
		}

		if (status & (MCHP_PCR_CLK32M_ISTS_FAIL |
			      MCHP_PCR_CLK32M_ISTS_STALL)) {
			rc = -EBUSY;
			break;
		}

		status = pcr->CLK32K_MON_ISTS;
	}

	pcr->CNT32K_CTRL = 0u;
	htmr0->PRLD = 0;
	girq23->SRC = BIT(XEC_CC_HTMR_0_GIRQ23_POS);

	return rc;
}

/*
 * Set the clock source for either PLL or Peripheral-32K clock domain.
 * The source must be a stable 32 KHz input: internal silicon oscillator,
 * external crystal dual-ended crystal, 50% duty cycle waveform on XTAL2 only,
 * or a 50% duty cycles waveform on the 32KHZ_PIN.
 * NOTE: 32KHZ_PIN is an alternate function of a chip specific GPIO.
 * Signal on 32KHZ_PIN may go off when VTR rail go down. MEC172x can automatically
 * switch to silicon OSC or XTAL. At this time we do not support fall back to XTAL
 * when using 32KHZ_PIN.
 * !!! IMPORTANT !!! Fall back from 32KHZ_PIN to SO/XTAL is only for the Peripheral
 * Clock domain. If the PLL is configured to use 32KHZ_PIN as its source then the
 * PLL will shutdown and the PLL clock domain should switch to the ring oscillator.
 * This means the PLL clock domain clock will not longer be accurate and may cause
 * FW malfunction(s).
 */

static void connect_pll_32k(const struct device *dev, enum pll_clk32k_src src, uint32_t flags)
{
	const struct xec_pcr_config * const devcfg = dev->config;
	struct pcr_hw_regs *const pcr = (struct pcr_hw_regs *)devcfg->pcr_base;
	uint32_t pcr_clk_sel;

	switch (src) {
	case PLL_CLK32K_SRC_XTAL:
		pcr_clk_sel = MCHP_PCR_VTR_32K_SRC_XTAL;
		break;
	case PLL_CLK32K_SRC_PIN:
		pcr_clk_sel = MCHP_PCR_VTR_32K_SRC_PIN;
		break;
	default: /* default to silicon OSC */
		pcr_clk_sel = MCHP_PCR_VTR_32K_SRC_SILOSC;
		break;
	}

	pcr->CLK32K_SRC_VTR = pcr_clk_sel;
}

static void connect_periph_32k(const struct device *dev, enum periph_clk32k_src src, uint32_t flags)
{
	const struct xec_pcr_config * const devcfg = dev->config;
	struct vbatr_hw_regs *const vbr = (struct vbatr_hw_regs *)devcfg->vbr_base;
	uint32_t vbr_clk_sel = vbr->CLK32_SRC & ~(MCHP_VBATR_CS_PCS_MSK);

	switch (src) {
	case PERIPH_CLK32K_SRC_XTAL_XTAL:
		vbr_clk_sel |= MCHP_VBATR_CS_PCS_VTR_VBAT_XTAL;
		break;
	case PERIPH_CLK32K_SRC_PIN_SO:
		vbr_clk_sel |= MCHP_VBATR_CS_PCS_VTR_PIN_SO;
		break;
	case PERIPH_CLK32K_SRC_PIN_XTAL:
		vbr_clk_sel |= MCHP_VBATR_CS_PCS_VTR_PIN_XTAL;
		break;
	default: /* default to silicon OSC for VTR/VBAT */
		vbr_clk_sel |= MCHP_VBATR_CS_PCS_VTR_VBAT_SO;
		break;
	}

	vbr->CLK32_SRC = vbr_clk_sel;
}

/* two bit field in PCR VTR 32KHz source register */
enum pll_clk32k_src get_pll_32k_source(const struct device *dev)
{
	const struct xec_pcr_config * const devcfg = dev->config;
	struct pcr_hw_regs *const pcr = (struct pcr_hw_regs *)devcfg->pcr_base;
	enum pll_clk32k_src src = PLL_CLK32K_SRC_MAX;

	switch (pcr->CLK32K_SRC_VTR & XEC_CC_PCR_CLK32K_SRC_MSK) {
	case XEC_CC_PCR_CLK32K_SRC_SIL:
		src = PLL_CLK32K_SRC_SO;
		break;
	case XEC_CC_PCR_CLK32K_SRC_XTAL:
		src = PLL_CLK32K_SRC_XTAL;
		break;
	case XEC_CC_PCR_CLK32K_SRC_PIN:
		src = PLL_CLK32K_SRC_PIN;
		break;
	default:
		src = PLL_CLK32K_SRC_MAX;
		break;
	}

	return src;
}

/* two bit field in VBAT source 32KHz register */
enum periph_clk32k_src get_periph_32k_source(const struct device *dev)
{
	const struct xec_pcr_config * const devcfg = dev->config;
	struct vbatr_hw_regs *const vbr = (struct vbatr_hw_regs *)devcfg->vbr_base;
	enum periph_clk32k_src src = PERIPH_CLK32K_SRC_MAX;
	uint32_t temp;

	temp = (vbr->CLK32_SRC & XEC_CC_VBATR_CS_PCS_MSK) >> XEC_CC_VBATR_CS_PCS_POS;
	if (temp == VBR_CLK32K_SRC_SO_SO) {
		src = PERIPH_CLK32K_SRC_SO_SO;
	} else if (temp == VBR_CLK32K_SRC_XTAL_XTAL) {
		src = PERIPH_CLK32K_SRC_XTAL_XTAL;
	} else if (temp == VBR_CLK32K_SRC_PIN_SO) {
		src = PERIPH_CLK32K_SRC_PIN_SO;
	} else {
		src = PERIPH_CLK32K_SRC_PIN_XTAL;
	}

	return src;
}

/*
 * MEC172x has two 32 KHz clock domains
 *   PLL domain: 32 KHz clock input for PLL to produce 96 MHz and 48 MHz clocks
 *   Peripheral domain: 32 KHz clock for subset of peripherals.
 * Each domain 32 KHz clock input can be from one of the following sources:
 *   Internal Silicon oscillator: +/- 2%
 *   External Crystal connected as parallel or single ended
 *   External 32KHZ_PIN 50% duty cycle waveform with fall back to either
 *     Silicon OSC or crystal when 32KHZ_PIN signal goes away or VTR power rail
 *     goes off.
 * At chip reset the PLL is held in reset and the +/- 50% ring oscillator is
 * the main clock.
 * If no VBAT reset occurs the VBAT 32 KHz source register maintains its state.
 */
static int soc_clk32_init(const struct device *dev,
			  enum pll_clk32k_src pll_src,
			  enum periph_clk32k_src periph_src,
			  uint32_t flags)
{
	const struct xec_pcr_config * const devcfg = dev->config;
	struct pcr_hw_regs *const pcr = (struct pcr_hw_regs *)devcfg->pcr_base;
	struct vbatr_hw_regs *const vbr = (struct vbatr_hw_regs *)devcfg->vbr_base;
	int rc = 0;

	/* disable PCR 32K monitor and clear counters */
	pcr->CNT32K_CTRL = MCHP_PCR_CLK32M_CTRL_CLR_CNT;
	pcr->CLK32K_MON_ISTS = MCHP_PCR_CLK32M_ISTS_MASK;
	pcr->CLK32K_MON_IEN = 0;

	if (!is_sil_osc_enabled(vbr)) {
		enable_sil_osc(vbr);
		spin_delay(pcr, CLK32K_SIL_OSC_DELAY);
	}

	/* Default to 32KHz Silicon OSC for PLL and peripherals */
	connect_pll_32k(dev, PLL_CLK32K_SRC_SO, 0);
	connect_periph_32k(dev, PERIPH_CLK32K_SRC_SO_SO, 0);

	rc = pll_wait_lock(pcr, CLK32K_PLL_LOCK_WAIT);
	if (rc) {
		LOG_ERR("XEC clock control: MEC172x lock timeout for internal 32K OSC");
		return rc;
	}

	/* If crystal input required, enable and check. Single-ended 32KHz square wave
	 * on XTAL pin is also handled here.
	 */
	if ((pll_src == PLL_CLK32K_SRC_XTAL) || periph_clk_src_using_xtal(periph_src)) {
		enable_32k_crystal(dev, flags);
		if (!devcfg->clkmon_bypass) {
			rc = check_32k_crystal(dev);
			if (rc) {
				/* disable crystal */
				vbr->CLK32_SRC &= ~(MCHP_VBATR_CS_XTAL_EN);
				LOG_ERR("XEC clock control: MEC172x XTAL check failed: %d", rc);
				return rc;
			}
		}
	} else {
		disable_32k_crystal(dev);
	}

	/* Do PLL first so we can use a peripheral timer still on silicon OSC */
	if (pll_src != PLL_CLK32K_SRC_SO) {
		connect_pll_32k(dev, pll_src, flags);
		rc = pll_wait_lock_periph(pcr, devcfg->pll_lock_timeout_ms);
	}

	if (periph_src != PERIPH_CLK32K_SRC_SO_SO) {
		connect_periph_32k(dev, periph_src, flags);
	}

	/* Configuration requests disabling internal silicon OSC. */
	if (devcfg->dis_internal_osc) {
		if ((get_pll_32k_source(dev) != PLL_CLK32K_SRC_SO)
		    && !periph_clk_src_using_si(get_periph_32k_source(dev))) {
			vbr->CLK32_SRC &= ~(XEC_CC_VBATR_CS_SO_EN);
		}
	}

	/* Configuration requests disabling internal silicon OSC. */
	if (devcfg->dis_internal_osc) {
		if ((get_pll_32k_source(dev) != PLL_CLK32K_SRC_SO)
		    && !periph_clk_src_using_si(get_periph_32k_source(dev))) {
			vbr->CLK32_SRC &= ~(XEC_CC_VBATR_CS_SO_EN);
		}
	}

	return rc;
}
#endif

/*
 * MEC172x Errata document DS80000913C
 * Programming the PCR clock divider that divides the clock input to the ARM
 * Cortex-M4 may cause a clock glitch. The recommended work-around is to
 * issue four NOP instruction before and after the write to the PCR processor
 * clock control register. The final four NOP instructions are followed by
 * data and instruction barriers to flush the Cortex-M4's pipeline.
 * NOTE: Zephyr provides inline functions for Cortex-Mx NOP but not for
 * data and instruction barrier instructions. Caller's should only invoke this
 * function with interrupts locked.
 */
static void xec_clock_control_core_clock_divider_set(uint8_t clkdiv)
{
	struct pcr_hw_regs *const pcr = (struct pcr_hw_regs *)DT_INST_REG_ADDR_BY_IDX(0, 0);

	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	pcr->PROC_CLK_CTRL = (uint32_t)clkdiv;
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	__DSB();
	__ISB();
}

/*
 * PCR peripheral sleep enable allows the clocks to a specific peripheral to
 * be gated off if the peripheral is not requesting a clock.
 * slp_idx = zero based index into 32-bit PCR sleep enable registers.
 * slp_pos = bit position in the register
 * slp_en if non-zero set the bit else clear the bit
 */
int z_mchp_xec_pcr_periph_sleep(uint8_t slp_idx, uint8_t slp_pos,
				uint8_t slp_en)
{
	struct pcr_hw_regs *const pcr = (struct pcr_hw_regs *)DT_INST_REG_ADDR_BY_IDX(0, 0);

	if ((slp_idx >= MCHP_MAX_PCR_SCR_REGS) || (slp_pos >= 32)) {
		return -EINVAL;
	}

	if (slp_en) {
		pcr->SLP_EN[slp_idx] |= BIT(slp_pos);
	} else {
		pcr->SLP_EN[slp_idx] &= ~BIT(slp_pos);
	}

	return 0;
}

/* clock control driver API implementation */

static int xec_cc_on(const struct device *dev,
		     clock_control_subsys_t sub_system,
		     bool turn_on)
{
	struct pcr_hw_regs *const pcr = (struct pcr_hw_regs *)DT_INST_REG_ADDR_BY_IDX(0, 0);
	struct mchp_xec_pcr_clk_ctrl *cc = (struct mchp_xec_pcr_clk_ctrl *)sub_system;
	uint16_t pcr_idx = 0;
	uint16_t bitpos = 0;

	if (!cc) {
		return -EINVAL;
	}

	switch (MCHP_XEC_CLK_SRC_GET(cc->pcr_info)) {
	case MCHP_XEC_PCR_CLK_CORE:
	case MCHP_XEC_PCR_CLK_BUS:
		break;
	case MCHP_XEC_PCR_CLK_CPU:
		if (cc->pcr_info & MCHP_XEC_CLK_CPU_MASK) {
			uint32_t lock = irq_lock();

			xec_clock_control_core_clock_divider_set(
				cc->pcr_info & MCHP_XEC_CLK_CPU_MASK);

			irq_unlock(lock);
		} else {
			return -EINVAL;
		}
		break;
	case MCHP_XEC_PCR_CLK_PERIPH:
	case MCHP_XEC_PCR_CLK_PERIPH_FAST:
		pcr_idx = MCHP_XEC_PCR_SCR_GET_IDX(cc->pcr_info);
		bitpos = MCHP_XEC_PCR_SCR_GET_BITPOS(cc->pcr_info);

		if (pcr_idx >= MCHP_MAX_PCR_SCR_REGS) {
			return -EINVAL;
		}

		if (turn_on) {
			pcr->SLP_EN[pcr_idx] &= ~BIT(bitpos);
		} else {
			pcr->SLP_EN[pcr_idx] |= BIT(bitpos);
		}
		break;
	case MCHP_XEC_PCR_CLK_PERIPH_SLOW:
		if (turn_on) {
			pcr->SLOW_CLK_CTRL =
				cc->pcr_info & MCHP_XEC_CLK_SLOW_MASK;
		} else {
			pcr->SLOW_CLK_CTRL = 0;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*
 * Turn on requested clock source.
 * Core, CPU, and Bus clocks are always on except in deep sleep state.
 * Peripheral clocks can be gated off if the peripheral's PCR sleep enable
 * is set and the peripheral indicates it does not need a clock by clearing
 * its PCR CLOCK_REQ read-only status.
 * Peripheral slow clock my be turned on by writing a non-zero divider value
 * to its PCR control register.
 */
static int xec_clock_control_on(const struct device *dev,
				clock_control_subsys_t sub_system)
{
	return xec_cc_on(dev, sub_system, true);
}

/*
 * Turn off clock source.
 * Core, CPU, and Bus clocks are always on except in deep sleep when PLL is
 * turned off. Exception is 32 KHz clock.
 * Peripheral clocks are gated off when the peripheral's sleep enable is set
 * and the peripheral indicates is no longer needs a clock by de-asserting
 * its read-only PCR CLOCK_REQ bit.
 * Peripheral slow clock can be turned off by writing 0 to its control register.
 */
static inline int xec_clock_control_off(const struct device *dev,
					clock_control_subsys_t sub_system)
{
	return xec_cc_on(dev, sub_system, false);
}

/* MEC172x and future SoC's implement a turbo clock mode where
 * ARM Core, QMSPI, and PK use turbo clock.  All other peripherals
 * use AHB clock or the slow clock.
 */
static uint32_t get_turbo_clock(const struct device *dev)
{
#ifdef CONFIG_SOC_SERIES_MEC1501X
	ARG_UNUSED(dev);

	return MHZ(48);
#else
	const struct xec_pcr_config * const devcfg = dev->config;
	struct pcr_hw_regs *const pcr = (struct pcr_hw_regs *)devcfg->pcr_base;

	if (pcr->TURBO_CLK & XEC_CC_PCR_TURBO_CLK_96M) {
		return MHZ(96);
	}

	return MHZ(48);
#endif
}

/*
 * MEC172x clock subsystem:
 * Two main clock domains: PLL and Peripheral-32K. Each domain's 32 KHz source
 * can be selected from one of three inputs:
 *  internal silicon OSC +/- 2% accuracy
 *  external crystal connected parallel or single ended
 *  external 32 KHz 50% duty cycle waveform on 32KHZ_IN pin.
 * PLL domain supplies 96 MHz, 48 MHz, and other high speed clocks to all
 * peripherals except those in the Peripheral-32K clock domain. The slow clock
 * is derived from the 48 MHz produced by the PLL.
 *   ARM Cortex-M4 core input: 96MHz
 *   AHB clock input: 48 MHz
 *   Fast AHB peripherals: 96 MHz internal and 48 MHz AHB interface.
 *   Slow clock peripherals: PWM,  TACH, PROCHOT
 * Peripheral-32K domain peripherals:
 *   WDT, RTC, RTOS timer, hibernation timers, week timer
 *
 * Peripherals using both PLL and 32K clock domains:
 *   BBLED, RPMFAN
 */
static int xec_clock_control_get_subsys_rate(const struct device *dev,
					     clock_control_subsys_t sub_system,
					     uint32_t *rate)
{
	const struct xec_pcr_config * const devcfg = dev->config;
	struct pcr_hw_regs *const pcr = (struct pcr_hw_regs *)devcfg->pcr_base;
	uint32_t bus = (uint32_t)sub_system;
	uint32_t temp = 0;
	uint32_t ahb_clock = MHZ(48);
	uint32_t turbo_clock = get_turbo_clock(dev);

	switch (bus) {
	case MCHP_XEC_PCR_CLK_CORE:
	case MCHP_XEC_PCR_CLK_PERIPH_FAST:
		*rate = turbo_clock;
		break;
	case MCHP_XEC_PCR_CLK_CPU:
		/* if PCR PROC_CLK_CTRL is 0 the chip is not running */
		*rate = turbo_clock / pcr->PROC_CLK_CTRL;
		break;
	case MCHP_XEC_PCR_CLK_BUS:
	case MCHP_XEC_PCR_CLK_PERIPH:
		*rate = ahb_clock;
		break;
	case MCHP_XEC_PCR_CLK_PERIPH_SLOW:
		temp = pcr->SLOW_CLK_CTRL;
		if (temp) {
			*rate = ahb_clock / temp;
		} else {
			*rate = 0; /* slow clock off */
		}
		break;
	default:
		*rate = 0;
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_PM)
void mchp_xec_clk_ctrl_sys_sleep_enable(bool is_deep)
{
	struct pcr_hw_regs *const pcr = (struct pcr_hw_regs *)DT_INST_REG_ADDR_BY_IDX(0, 0);
	uint32_t sys_sleep_mode = MCHP_PCR_SYS_SLP_CTRL_SLP_ALL;

	if (is_deep) {
		sys_sleep_mode |= MCHP_PCR_SYS_SLP_CTRL_SLP_HEAVY;
	}

	SCB->SCR |= BIT(2);
	pcr->SYS_SLP_CTRL = sys_sleep_mode;
}

void mchp_xec_clk_ctrl_sys_sleep_disable(void)
{
	struct pcr_hw_regs *const pcr = (struct pcr_hw_regs *)DT_INST_REG_ADDR_BY_IDX(0, 0);
	pcr->SYS_SLP_CTRL = 0;
	SCB->SCR &= ~BIT(2);
}
#endif

/* Clock controller driver registration */
static struct clock_control_driver_api xec_clock_control_api = {
	.on = xec_clock_control_on,
	.off = xec_clock_control_off,
	.get_rate = xec_clock_control_get_subsys_rate,
};

static int xec_clock_control_init(const struct device *dev)
{
	const struct xec_pcr_config * const devcfg = dev->config;
	struct pcr_hw_regs *const pcr = (struct pcr_hw_regs *)devcfg->pcr_base;
	enum pll_clk32k_src pll_clk_src = devcfg->pll_src;
	enum periph_clk32k_src periph_clk_src = devcfg->periph_src;
	uint32_t clk_flags = 0U;
	int rc = 0;

	if (devcfg->xtal_se) {
		clk_flags |= CLK32K_FLAG_CRYSTAL_SE;
	}

	pcr_slp_init(pcr);

	rc = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_DEFAULT);
	if ((pll_clk_src == PLL_CLK32K_SRC_PIN) || periph_clk_src_using_pin(periph_clk_src)) {
		if (rc) {
			LOG_ERR("XEC clock control: PINCTRL apply error %d", rc);
			pll_clk_src = PLL_CLK32K_SRC_SO;
			periph_clk_src = PERIPH_CLK32K_SRC_SO_SO;
			clk_flags = 0U;
		}
	}

	/* sleep used as debug */
	rc = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_SLEEP);
	if ((rc != 0) && (rc != -ENOENT)) {
		LOG_ERR("XEC clock control: PINCTRL debug apply error %d", rc);
	}

	rc = soc_clk32_init(dev, pll_clk_src, periph_clk_src, clk_flags);
	if (rc) {
		LOG_ERR("XEC clock control: init error %d", rc);
	}

	xec_clock_control_core_clock_divider_set(devcfg->core_clk_div);

	return rc;
}

#define XEC_PLL_32K_SRC(i)	\
	(enum pll_clk32k_src)DT_INST_PROP_OR(i, pll_32k_src, PLL_CLK32K_SRC_SO)

#define XEC_PERIPH_32K_SRC(i)	\
	(enum periph_clk32k_src)DT_INST_PROP_OR(0, periph_32k_src, PERIPH_CLK32K_SRC_SO_SO)

PINCTRL_DT_INST_DEFINE(0);

const struct xec_pcr_config pcr_xec_config = {
	.pcr_base = DT_INST_REG_ADDR_BY_IDX(0, 0),
	.vbr_base = DT_INST_REG_ADDR_BY_IDX(0, 1),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.xtal_enable_delay_ms =
		(uint16_t)DT_INST_PROP_OR(0, xtal_enable_delay_ms, XEC_CC_XTAL_EN_DELAY_MS_DFLT),
	.pll_lock_timeout_ms =
		(uint16_t)DT_INST_PROP_OR(0, pll_lock_timeout_ms, XEC_CC_DFLT_PLL_LOCK_WAIT_MS),
	.period_min = (uint16_t)DT_INST_PROP_OR(0, clk32kmon_period_min, CNT32K_TMIN),
	.period_max = (uint16_t)DT_INST_PROP_OR(0, clk32kmon_period_max, CNT32K_TMAX),
	.core_clk_div = (uint8_t)DT_INST_PROP_OR(0, core_clk_div, CONFIG_SOC_MEC172X_PROC_CLK_DIV),
	.xtal_se = (uint8_t)DT_INST_PROP_OR(0, xtal_single_ended, 0),
	.max_dc_va = (uint8_t)DT_INST_PROP_OR(0, clk32kmon_duty_cycle_var_max, CNT32K_DUTY_MAX),
	.min_valid = (uint8_t)DT_INST_PROP_OR(0, clk32kmon_valid_min, CNT32K_VAL_MIN),
	.pll_src = XEC_PLL_32K_SRC(0),
	.periph_src = XEC_PERIPH_32K_SRC(0),
	.clkmon_bypass = (uint8_t)DT_INST_PROP_OR(0, clkmon_bypass, 0),
	.dis_internal_osc = (uint8_t)DT_INST_PROP_OR(0, internal_osc_disable, 0),
};

DEVICE_DT_INST_DEFINE(0,
		    &xec_clock_control_init,
		    NULL,
		    NULL, &pcr_xec_config,
		    PRE_KERNEL_1,
		    CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		    &xec_clock_control_api);
