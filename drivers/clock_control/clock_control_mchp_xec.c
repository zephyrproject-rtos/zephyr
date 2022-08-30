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
#include <zephyr/dt-bindings/clock/mchp_xec_pcr.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_xec, LOG_LEVEL_ERR);

#define CLK32K_SIL_OSC_DELAY		256
#define CLK32K_PLL_LOCK_WAIT		(16 * 1024)
#define CLK32K_PIN_WAIT			4096
#define CLK32K_XTAL_WAIT		(16 * 1024)
#define CLK32K_XTAL_MON_WAIT		(64 * 1024)

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
#define CNT32K_DUTY_MAX			74
#define CNT32K_VAL_MIN			4

#define DEST_PLL			0
#define DEST_PERIPH			1

#define CLK32K_FLAG_CRYSTAL_SE		BIT(0)
#define CLK32K_FLAG_PIN_FB_CRYSTAL	BIT(1)

#define PCR_PERIPH_RESET_SPIN		8u

#define HIBTIMER_10_MS			328u
#define HIBTIMER_300_MS			9830u

#define PCR_XEC_REG_BASE						\
	((struct pcr_regs *)(DT_REG_ADDR(DT_NODELABEL(pcr))))

#define HIBTIMER_0_XEC_REG_BASE						\
	((struct htmr_regs *)(DT_REG_ADDR(DT_NODELABEL(hibtimer0))))

#define GIRQ23_XEC_REG_BASE						\
	((struct girq_regs *)(DT_REG_ADDR(DT_NODELABEL(girq23))))

enum clk32k_src {
	CLK32K_SRC_SIL_OSC = 0,
	CLK32K_SRC_CRYSTAL,
	CLK32K_SRC_MAX
};

enum clk32k_dest { CLK32K_DEST_PLL = 0, CLK32K_DEST_PERIPH, CLK32K_DEST_MAX };


/* Driver config */
struct xec_pcr_config {
	uintptr_t pcr_base; /* pcr base address */
	uintptr_t vbr_base; /* vbat registers base address */
};

/* Driver convenience defines */

#define PCR_NODE_LBL	DT_NODELABEL(pcr)

#define XEC_CORE_CLK_DIV \
	DT_PROP_OR(PCR_NODE_LBL, core_clk_div, CONFIG_SOC_MEC172X_PROC_CLK_DIV)

#define DRV_CONFIG(dev) \
	((const struct xec_pcr_config *)(dev)->config)

#define XEC_PCR_REGS_BASE(dev) \
	(struct pcr_regs *)(DRV_CONFIG(dev)->pcr_base)

#define XEC_VBATR_REGS_BASE(dev) \
	(struct vbatr_regs *)(DRV_CONFIG(dev)->vbr_base)

/*
 * In early Zephyr initialization we don't have timer services. Also, the SoC
 * may be running on its ring oscillator (+/- 50% accuracy). Configuring the
 * SoC's clock subsystem requires wait/delays. We implement a simple delay
 * by writing to a read-only hardware register in the PCR block.
 */
static uint32_t spin_delay(struct pcr_regs *pcr, uint32_t cnt)
{
	uint32_t n;

	for (n = 0U; n < cnt; n++) {
		pcr->OSC_ID = n;
	}

	return n;
}

/*
 * Make sure PCR sleep enables are clear except for crypto
 * which do not have internal clock gating.
 */
static void pcr_slp_init(struct pcr_regs *pcr)
{
	pcr->SYS_SLP_CTRL = 0U;
	SCB->SCR &= ~BIT(2);

	for (int i = 0; i < MCHP_MAX_PCR_SCR_REGS; i++) {
		pcr->SLP_EN[i] = 0U;
	}

	pcr->SLP_EN[3] = MCHP_PCR3_CRYPTO_MASK;
}

static bool is_sil_osc_enabled(struct vbatr_regs *vbr)
{
	if (vbr->CLK32_SRC & MCHP_VBATR_CS_SO_EN) {
		return true;
	}

	return false;
}

static void enable_sil_osc(struct vbatr_regs *vbr)
{
	vbr->CLK32_SRC |= MCHP_VBATR_CS_SO_EN;
}

/* caller has enabled internal silicon 32 KHz oscillator */
static void hib_timer_delay(uint16_t hib_timer_count)
{
	struct htmr_regs *htmr0 = HIBTIMER_0_XEC_REG_BASE;
	struct girq_regs *girq23 = GIRQ23_XEC_REG_BASE;

	htmr0->PRLD = 0; /* disable */
	htmr0->CTRL = 0; /* 32k time base */
	girq23->SRC = BIT(16); /* clear hibernation timer 0 status */
	htmr0->PRLD = hib_timer_count;
	if (hib_timer_count == 0) {
		return;
	}

	while ((girq23->SRC & BIT(16)) == 0) {
		;
	}

	girq23->SRC = BIT(16);
	htmr0->PRLD = 0; /* disable */
}

/*
 * Start external 32 KHz crystal.
 * Assumes peripheral clocks source is Silicon OSC.
 * If current configuration matches desired crystal configuration do nothing.
 * NOTE: Crystal requires ~300 ms to stabilize.
 */
static int enable_32k_crystal(const struct device *dev, uint32_t flags)
{
	struct vbatr_regs *const vbr = XEC_VBATR_REGS_BASE(dev);
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
	hib_timer_delay(HIBTIMER_300_MS);
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
	struct pcr_regs *const pcr = XEC_PCR_REGS_BASE(dev);
	struct htmr_regs *htmr0 = HIBTIMER_0_XEC_REG_BASE;
	struct girq_regs *girq23 = GIRQ23_XEC_REG_BASE;
	uint32_t status = 0;
	int rc = 0;

	htmr0->PRLD = 0;
	htmr0->CTRL = 0;
	girq23->SRC = BIT(16);

	pcr->CNT32K_CTRL = 0U;
	pcr->CLK32K_MON_IEN = 0U;
	pcr->CLK32K_MON_ISTS = MCHP_PCR_CLK32M_ISTS_MASK;

	pcr->CNT32K_PER_MIN = CNT32K_TMIN;
	pcr->CNT32K_PER_MAX = CNT32K_TMAX;
	pcr->CNT32K_DV_MAX = CNT32K_DUTY_MAX;
	pcr->CNT32K_VALID_MIN = CNT32K_VAL_MIN;

	pcr->CNT32K_CTRL =
		MCHP_PCR_CLK32M_CTRL_PER_EN | MCHP_PCR_CLK32M_CTRL_DC_EN |
		MCHP_PCR_CLK32M_CTRL_VAL_EN | MCHP_PCR_CLK32M_CTRL_CLR_CNT;

	rc = -ETIMEDOUT;
	htmr0->PRLD = HIBTIMER_10_MS;
	status = pcr->CLK32K_MON_ISTS;

	while ((girq23->SRC & BIT(16)) == 0) {
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
	girq23->SRC = BIT(16);

	return rc;
}

/*
 * Set the clock source for either PLL or Peripheral-32K clock domain.
 * The source must be a stable 32 KHz input: internal silicon oscillator,
 * external crystal (parallel or single ended connection), or a 50% duty cycle
 * waveform on the 32KHZ_PIN. The driver does not implement 32KHZ_PIN support
 * at this time.
 */
static void connect_32k_source(const struct device *dev, enum clk32k_src src,
			       enum clk32k_dest dest, uint32_t flags)
{
	struct pcr_regs *const pcr = XEC_PCR_REGS_BASE(dev);
	struct vbatr_regs *const vbr = XEC_VBATR_REGS_BASE(dev);

	if (dest == CLK32K_DEST_PLL) {
		switch (src) {
		case CLK32K_SRC_SIL_OSC:
			pcr->CLK32K_SRC_VTR = MCHP_PCR_VTR_32K_SRC_SILOSC;
			break;
		case CLK32K_SRC_CRYSTAL:
			pcr->CLK32K_SRC_VTR = MCHP_PCR_VTR_32K_SRC_XTAL;
			break;
		default: /* do not touch HW */
			break;
		}
	} else if (dest == CLK32K_DEST_PERIPH) {
		uint32_t vbcs = vbr->CLK32_SRC & ~(MCHP_VBATR_CS_PCS_MSK);

		switch (src) {
		case CLK32K_SRC_SIL_OSC:
			vbr->CLK32_SRC = vbcs | MCHP_VBATR_CS_PCS_VTR_VBAT_SO;
			break;
		case CLK32K_SRC_CRYSTAL:
			vbr->CLK32_SRC = vbcs | MCHP_VBATR_CS_PCS_VTR_VBAT_XTAL;
			break;
		default: /* do not touch HW */
			break;
		}
	}
}

/*
 * This routine checks if the PLL is locked to its input source. Minimum lock
 * time is 3.3 ms. Lock time can be larger when the source is an external
 * crystal. Crystal cold start times may vary greatly based on many factors.
 * Crystals do not like being power cycled.
 */
static int pll_wait_lock(struct pcr_regs *const pcr, uint32_t wait_cnt)
{
	while (!(pcr->OSC_ID & MCHP_PCR_OSC_ID_PLL_LOCK)) {
		if (wait_cnt == 0) {
			return -ETIMEDOUT;
		}
		--wait_cnt;
	}

	return 0;
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
			  enum clk32k_src pll_clk_src,
			  enum clk32k_src periph_clk_src,
			  uint32_t flags)
{
	struct pcr_regs *const pcr = XEC_PCR_REGS_BASE(dev);
	struct vbatr_regs *const vbr = XEC_VBATR_REGS_BASE(dev);
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
	connect_32k_source(dev, CLK32K_SRC_SIL_OSC,  CLK32K_DEST_PLL, 0);
	connect_32k_source(dev, CLK32K_SRC_SIL_OSC, CLK32K_DEST_PERIPH, 0);

	rc = pll_wait_lock(pcr, CLK32K_PLL_LOCK_WAIT);
	if (rc) {
		return rc;
	}

	/* We only allow Silicon OSC or Crystal as a source. */
	if ((pll_clk_src == CLK32K_SRC_CRYSTAL) ||
	    (periph_clk_src == CLK32K_SRC_CRYSTAL)) {
		enable_32k_crystal(dev, flags);
		rc = check_32k_crystal(dev);
		if (rc) {
			/* disable crystal */
			vbr->CLK32_SRC &= ~(MCHP_VBATR_CS_XTAL_EN);
			return rc;
		}
		if (pll_clk_src == CLK32K_SRC_CRYSTAL) {
			connect_32k_source(dev, CLK32K_SRC_CRYSTAL,
					   CLK32K_DEST_PLL, flags);
		}
		if (periph_clk_src == CLK32K_SRC_CRYSTAL) {
			connect_32k_source(dev, CLK32K_SRC_CRYSTAL,
					   CLK32K_DEST_PERIPH, flags);
		}
		rc = pll_wait_lock(pcr, CLK32K_PLL_LOCK_WAIT);
	}

	return rc;
}

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
	struct pcr_regs *const pcr =
		(struct pcr_regs *)(DT_REG_ADDR(DT_NODELABEL(pcr)));

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
	struct pcr_regs *regs = PCR_XEC_REG_BASE;

	if ((slp_idx >= MCHP_MAX_PCR_SCR_REGS) || (slp_pos >= 32)) {
		return -EINVAL;
	}

	if (slp_en) {
		regs->SLP_EN[slp_idx] |= BIT(slp_pos);
	} else {
		regs->SLP_EN[slp_idx] &= ~BIT(slp_pos);
	}

	return 0;
}

/* clock control driver API implementation */

static int xec_cc_on(const struct device *dev,
		     clock_control_subsys_t sub_system,
		     bool turn_on)
{
	struct pcr_regs *const pcr = XEC_PCR_REGS_BASE(dev);
	struct mchp_xec_pcr_clk_ctrl *cc =
		(struct mchp_xec_pcr_clk_ctrl *)sub_system;
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
	struct pcr_regs *const pcr = XEC_PCR_REGS_BASE(dev);
	uint32_t bus = (uint32_t)sub_system;
	uint32_t temp = 0;

	switch (bus) {
	case MCHP_XEC_PCR_CLK_CORE:
	case MCHP_XEC_PCR_CLK_PERIPH_FAST:
		*rate = MHZ(96);
		break;
	case MCHP_XEC_PCR_CLK_CPU:
		/* if PCR PROC_CLK_CTRL is 0 the chip is not running */
		*rate = MHZ(96) / pcr->PROC_CLK_CTRL;
		break;
	case MCHP_XEC_PCR_CLK_BUS:
	case MCHP_XEC_PCR_CLK_PERIPH:
		*rate = MHZ(48);
		break;
	case MCHP_XEC_PCR_CLK_PERIPH_SLOW:
		temp = pcr->SLOW_CLK_CTRL;
		if (pcr->SLOW_CLK_CTRL) {
			*rate = MHZ(48) / temp;
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
	struct pcr_regs *const pcr =
		(struct pcr_regs *)(DT_REG_ADDR(DT_NODELABEL(pcr)));
	uint32_t sys_sleep_mode = MCHP_PCR_SYS_SLP_CTRL_SLP_ALL;

	if (is_deep) {
		sys_sleep_mode |= MCHP_PCR_SYS_SLP_CTRL_SLP_HEAVY;
	}

	SCB->SCR |= BIT(2);
	pcr->SYS_SLP_CTRL = sys_sleep_mode;
}

void mchp_xec_clk_ctrl_sys_sleep_disable(void)
{
	struct pcr_regs *const pcr =
		(struct pcr_regs *)(DT_REG_ADDR(DT_NODELABEL(pcr)));

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
	int rc = 0;
	uint32_t clk32_flags = 0;
	struct pcr_regs *const pcr = XEC_PCR_REGS_BASE(dev);
	enum clk32k_src clk_src_pll =
		DT_PROP_OR(PCR_NODE_LBL, pll_32k_src, CLK32K_SRC_SIL_OSC);
	enum clk32k_src clk_src_periph =
		DT_PROP_OR(PCR_NODE_LBL, periph_32k_src, CLK32K_SRC_SIL_OSC);

	pcr_slp_init(pcr);

	rc = soc_clk32_init(dev, clk_src_pll, clk_src_periph, clk32_flags);
	__ASSERT(rc == 0, "XEC: PLL and 32 KHz clock initialization failed");

	xec_clock_control_core_clock_divider_set(XEC_CORE_CLK_DIV);

	return rc;
}

const struct xec_pcr_config xec_config = {
	.pcr_base = DT_INST_REG_ADDR_BY_IDX(0, 0),
	.vbr_base = DT_INST_REG_ADDR_BY_IDX(0, 1),
};

DEVICE_DT_INST_DEFINE(0,
		    &xec_clock_control_init,
		    NULL,
		    NULL, &xec_config,
		    PRE_KERNEL_1,
		    CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		    &xec_clock_control_api);
