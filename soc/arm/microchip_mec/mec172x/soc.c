/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/__assert.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <kernel.h>
#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

#define CLK32K_SIL_OSC_DELAY	256U
#define CLK32K_PLL_LOCK_WAIT	(16U * 1024U)
#define CLK32K_PIN_WAIT		4096U
#define CLK32K_XTAL_WAIT	4096U

#define DEST_PLL	0
#define DEST_PERIPH	1

#define CLK32K_FLAG_CRYSTAL_SE		BIT(0)
#define CLK32K_FLAG_PIN_FB_CRYSTAL	BIT(1)

#define PIN_32KHZ_IN_MODE (MCHP_GPIO_CTRL_PUD_NONE |\
	MCHP_GPIO_CTRL_PWRG_VTR_IO | MCHP_GPIO_CTRL_IDET_DISABLE |\
	MCHP_GPIO_CTRL_MUX_F1)

enum clk32k_src {
	CLK32K_SRC_SIL_OSC = 0,
	CLK32K_SRC_CRYSTAL,
	CLK32K_SRC_PIN,
	CLK32K_SRC_MAX
};

enum clk32k_dest { CLK32K_DEST_PLL = 0, CLK32K_DEST_PERIPH, CLK32K_DEST_MAX };

static uint32_t spin_delay(uint32_t cnt)
{
	struct pcr_regs *pcr = (struct pcr_regs *)(PCR_BASE);
	uint32_t n;

	for (n = 0U; n < cnt; n++) {
		pcr->OSC_ID = n;
	}

	return n;
}

/*
 * Make sure PCR sleep enables are clear except for crypto
 * which do not have internal clock gating and UARTs.
 */
static int soc_pcr_init(void)
{
	struct pcr_regs *pcr = (struct pcr_regs *)(PCR_BASE);

	pcr->SYS_SLP_CTRL = 0U;

	for (int i = 0; i < MCHP_MAX_PCR_SCR_REGS; i++) {
		pcr->SLP_EN[i] = 0U;
	}

	pcr->SLP_EN[3] = MCHP_PCR3_CRYPTO_MASK;

	return 0;
}

/*
 * Initialize MEC172x EC Interrupt Aggregator (ECIA) and external NVIC
 * inputs.
 */
static int soc_ecia_init(void)
{
	struct ecs_regs *ecsr = (struct ecs_regs *)(ECS_BASE);
	struct ecia_ar_regs *eciar = (struct ecia_ar_regs *)(ECIA_BASE);
	uint32_t n;

	mchp_pcr_periph_slp_ctrl(PCR_ECIA, MCHP_PCR_SLEEP_DIS);

	ecsr->INTR_CTRL |= MCHP_ECS_ICTRL_DIRECT_EN;

	/* gate off all aggregated outputs */
	eciar->BLK_EN_CLR = 0xFFFFFFFFul;
	/* gate on GIRQ's that are aggregated only */
	eciar->BLK_EN_SET = MCHP_ECIA_AGGR_BITMAP;

	/* Clear all GIRQn source enables */
	for (n = 0; n < MCHP_GIRQ_IDX_MAX; n++) {
		eciar->GIRQ[n].EN_CLR = 0xFFFFFFFFul;
	}

	/* Clear all external NVIC enables and pending status */
	for (n = 0u; n < MCHP_NUM_NVIC_REGS; n++) {
		NVIC->ICER[n] = 0xFFFFFFFFul;
		NVIC->ICPR[n] = 0xFFFFFFFFul;
	}

	return 0;
}

static bool is_sil_osc_enabled(void)
{
	struct vbatr_regs *vbr = (struct vbatr_regs *)(VBATR_BASE);

	if (vbr->CLK32_SRC & MCHP_VBATR_CS_SO_EN) {
		return true;
	}

	return false;
}

static void enable_sil_osc(uint32_t delay_cnt)
{
	struct vbatr_regs *vbr = (struct vbatr_regs *)(VBATR_BASE);

	vbr->CLK32_SRC |= MCHP_VBATR_CS_SO_EN;
	spin_delay(delay_cnt);
}

static int enable_32k_pin(uint32_t wait_cnt)
{
	struct pcr_regs *pcr = (struct pcr_regs *)(PCR_BASE);
	struct gpio_regs *gpr = (struct gpio_regs *)(GPIO_BASE);

	gpr->CTRL[MCHP_GPIO_0165_ID] = PIN_32KHZ_IN_MODE;

	do {
		if (pcr->PWR_RST_STS & MCHP_PCR_PRS_32K_ACTIVE_RO) {
			pcr->CLK32K_SRC_VTR = MCHP_PCR_VTR_32K_SRC_SILOSC;
			return 0;
		}
	} while (wait_cnt--);

	return -1;
}


/*
 * Do nothing if crystal configuration matches request.
 */
static int enable_32k_crystal(uint32_t flags)
{
	struct vbatr_regs *vbr = (struct vbatr_regs *)(VBATR_BASE);
	uint32_t vbcs = vbr->CLK32_SRC;
	uint32_t cfg = MCHP_VBATR_CS_SO_EN;

	if (flags & CLK32K_FLAG_CRYSTAL_SE) {
		cfg |= MCHP_VBATR_CS_XTAL_SE;
	}

	if ((vbcs & cfg) == cfg) {
		return 0;
	}

	/* parallel and clear high startup current disable */
	vbcs &= ~(MCHP_VBATR_CS_SO_EN | MCHP_VBATR_CS_XTAL_SE |
		  MCHP_VBATR_CS_XTAL_DHC | MCHP_VBATR_CS_XTAL_CNTR_MSK);
	if (flags & CLK32K_FLAG_CRYSTAL_SE) { /* single ended connection? */
		vbcs |= MCHP_VBATR_CS_XTAL_SE;
	}

	/* XTAL mode and enable must be separate writes */
	vbr->CLK32_SRC = vbcs | MCHP_VBATR_CS_XTAL_CNTR_DG;
	vbr->CLK32_SRC |= MCHP_VBATR_CS_SO_EN;
	spin_delay(CLK32K_XTAL_WAIT);
	vbr->CLK32_SRC |= MCHP_VBATR_CS_XTAL_DHC;

	return 0;
}

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
#define CNT32K_TMIN 1435
#define CNT32K_TMAX 1495
#define CNT32K_DUTY_MAX 74
#define CNT32K_VAL_MIN 4

static int check_32k_crystal(void)
{
	struct pcr_regs *pcr = (struct pcr_regs *)(PCR_BASE);

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

	uint32_t status = pcr->CLK32K_MON_ISTS;

	while (status == 0) {
		status = pcr->CLK32K_MON_ISTS;
	}

	pcr->CNT32K_CTRL = 0U;

	if (status ==
	    (MCHP_PCR_CLK32M_ISTS_PULSE_RDY | MCHP_PCR_CLK32M_ISTS_PASS_PER |
	     MCHP_PCR_CLK32M_ISTS_PASS_DC | MCHP_PCR_CLK32M_ISTS_VALID)) {
		return 0;
	}

	return -1;
}

static void connect_32k_source(enum clk32k_src src, enum clk32k_dest dest,
			       uint32_t flags)
{
	struct pcr_regs *pcr = (struct pcr_regs *)(PCR_BASE);
	struct vbatr_regs *vbr = (struct vbatr_regs *)(VBATR_BASE);

	if (dest == CLK32K_DEST_PLL) {
		switch (src) {
		case CLK32K_SRC_SIL_OSC:
			pcr->CLK32K_SRC_VTR = MCHP_PCR_VTR_32K_SRC_SILOSC;
			break;
		case CLK32K_SRC_CRYSTAL:
			pcr->CLK32K_SRC_VTR = MCHP_PCR_VTR_32K_SRC_SILOSC;
			break;
		case CLK32K_SRC_PIN:
			pcr->CLK32K_SRC_VTR = MCHP_PCR_VTR_32K_SRC_PIN;
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
		case CLK32K_SRC_PIN:
			if (flags & CLK32K_FLAG_PIN_FB_CRYSTAL) {
				vbr->CLK32_SRC =
					vbcs | MCHP_VBATR_CS_PCS_VTR_PIN_SO;
			} else {
				vbr->CLK32_SRC =
					vbcs | MCHP_VBATR_CS_PCS_VTR_PIN_XTAL;
			}
			break;
		default: /* do not touch HW */
			break;
		}
	}
}

static int pll_wait_lock(uint32_t wait_cnt)
{
	struct pcr_regs *regs = (struct pcr_regs *)(PCR_BASE);

	while (!(regs->OSC_ID & MCHP_PCR_OSC_ID_PLL_LOCK)) {
		if (wait_cnt == 0) {
			return -1;
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
 * If no VBAT reset occurs the VBAT 32 KHz soure register maintains its state.
 */
static int soc_clk32_init(enum clk32k_src pll_clk_src,
			  enum clk32k_src periph_clk_src,
			  uint32_t flags)
{
	if (!is_sil_osc_enabled()) {
		enable_sil_osc(CLK32K_SIL_OSC_DELAY);
	}

	/* Default to 32KHz Silicon OSC for PLL and peripherals */
	connect_32k_source(CLK32K_SRC_SIL_OSC,  CLK32K_DEST_PLL, 0);
	connect_32k_source(CLK32K_SRC_SIL_OSC, CLK32K_DEST_PERIPH, 0);

	int rc = pll_wait_lock(CLK32K_PLL_LOCK_WAIT);

	if (rc) {
		return rc;
	}

	/*
	 * We only allow Silicon OSC or Crystal as PLL source.
	 * Any of the three source is allowed for Peripheral clock
	 */
	if ((pll_clk_src == CLK32K_SRC_CRYSTAL) ||
	    (periph_clk_src == CLK32K_SRC_CRYSTAL)) {
		enable_32k_crystal(flags);
		rc = check_32k_crystal();
		if (rc) {
			return rc;
		}
		if (pll_clk_src == CLK32K_SRC_CRYSTAL) {
			connect_32k_source(CLK32K_SRC_CRYSTAL, CLK32K_DEST_PLL,
					   flags);
		}
		if (periph_clk_src == CLK32K_SRC_CRYSTAL) {
			connect_32k_source(CLK32K_SRC_CRYSTAL,
					   CLK32K_DEST_PERIPH,
					   flags);
		}
		rc = pll_wait_lock(CLK32K_PLL_LOCK_WAIT);
	} else if (periph_clk_src == CLK32K_SRC_PIN) {
		rc = enable_32k_pin(CLK32K_PIN_WAIT);
		if (rc) {
			return rc;
		}
		connect_32k_source(CLK32K_SRC_PIN, CLK32K_DEST_PERIPH, flags);
	}

	return rc;
}

uint32_t soc_get_core_clock(void)
{
	struct pcr_regs *regs = (struct pcr_regs *)(PCR_BASE);

	return (uint32_t)(MCHP_EC_CLOCK_INPUT_HZ) / regs->PROC_CLK_CTRL;
}

/* Implementation based upon MEC172x Errata */
void soc_set_core_clock_div(uint8_t clkdiv)
{
	struct pcr_regs *regs = (struct pcr_regs *)(PCR_BASE);

	__NOP();
	__NOP();
	__NOP();
	__NOP();
	regs->PROC_CLK_CTRL = (uint32_t)clkdiv;
	__NOP();
	__NOP();
	__NOP();
	__NOP();
	__DSB();
	__ISB();
}

static int soc_init(const struct device *dev)
{
	uint32_t key, clk32_flags;
	int rc = 0;
	enum clk32k_src clk_src_pll, clk_src_periph;

	ARG_UNUSED(dev);

	key = irq_lock();

	soc_pcr_init();
	soc_ecia_init();

	clk32_flags = 0U;

	clk_src_pll = DT_PROP_OR(DT_NODELABEL(clock0), pll_32k_src, CLK32K_SRC_SIL_OSC);

	clk_src_periph = DT_PROP_OR(DT_NODELABEL(clock0), periph_32k_src, CLK32K_SRC_SIL_OSC);

	rc = soc_clk32_init(clk_src_pll, clk_src_periph, clk32_flags);
	__ASSERT(rc == 0, "SoC: PLL and 32 KHz clock initialization failed");

	soc_set_core_clock_div(DT_PROP_OR(DT_NODELABEL(clock0), cpu_clk_div, 1U));

	irq_unlock(key);

	return rc;
}

SYS_INIT(soc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
