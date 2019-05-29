/*
 * Copyright (c) 2019 ML!PA Consulting GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Atmel SAMD MCU series initialization code
 */

#include <init.h>
#include <soc.h>
#include <clock_control.h>

#if !defined(DT_FIXED_CLOCK_FDPLL0_CLOCK_CONTROLLER_0) && defined(DT_FIXED_CLOCK_FDPLL0_CLOCK_CONTROLLER)
#define DT_FIXED_CLOCK_FDPLL0_CLOCK_CONTROLLER_0 DT_FIXED_CLOCK_FDPLL0_CLOCK_CONTROLLER
#endif

static void soc_sam0_flash_wait_init(void)
{
	/* SAME D5x/E5x Manual: 54.11 wait states for Vdd >= 2.7V */
	if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= MHZ(24)) {
		NVMCTRL->CTRLA.bit.AUTOWS = 0;
		NVMCTRL->CTRLA.bit.RWS = 0;
	} else {
		NVMCTRL->CTRLA.bit.AUTOWS = 1;
	}
}

static void soc_sam0_wait_gclk_synchronization(void)
{
	while (GCLK->SYNCBUSY.reg) {
	}
}

static ALWAYS_INLINE
void soc_sam0_xosc_init(u8_t idx, u32_t freq)
{
#ifdef DT_FIXED_CLOCK_XOSC0_CLOCK_FREQUENCY
	u32_t reg = OSCCTRL_XOSCCTRL_XTALEN |
		    OSCCTRL_XOSCCTRL_ENALC  |
		    OSCCTRL_XOSCCTRL_ENABLE;

	/* SAM D5x/E5x Manual 54.12.1 (Crystal oscillator characteristics) &
	 * 28.8.6 (External Multipurpose Crystal Oscillator Control)
	 */
	if (freq <= MHZ(8)) {
		/* 72200 cycles @ 8MHz = 9025 µs */
		reg |= OSCCTRL_XOSCCTRL_STARTUP(9);
	} else if (freq <= MHZ(16)) {
		/* 62000 cycles @ 16MHz = 3875 µs */
		reg |= OSCCTRL_XOSCCTRL_STARTUP(7);
	} else if (freq <= MHZ(24)) {
		/* 68500 cycles @ 24MHz = 2854 µs */
		reg |= OSCCTRL_XOSCCTRL_STARTUP(7);
	} else {
		/* 38500 cycles @ 48MHz = 802 µs */
		reg |= OSCCTRL_XOSCCTRL_STARTUP(5);
	}

	OSCCTRL->XOSCCTRL[idx].reg = reg;
#endif
}

static void soc_sam0_xosc32k_init(void)
{
#ifdef DT_FIXED_CLOCK_XOSC32K_CLOCK_FREQUENCY
	OSC32KCTRL->XOSC32K.reg = OSC32KCTRL_XOSC32K_STARTUP(7)
				| OSC32KCTRL_XOSC32K_XTALEN
				| OSC32KCTRL_XOSC32K_EN32K
				| OSC32KCTRL_XOSC32K_ENABLE;
#endif
}

static void soc_sam0_wait_fixed_osc_ready(void)
{
#ifdef DT_FIXED_CLOCK_XOSC0_CLOCK_FREQUENCY
	while (!OSCCTRL->STATUS.bit.XOSCRDY0) {
	}
#endif
#ifdef DT_FIXED_CLOCK_XOSC1_CLOCK_FREQUENCY
	while (!OSCCTRL->STATUS.bit.XOSCRDY1) {
	}
#endif
#ifdef DT_FIXED_CLOCK_XOSC32K_CLOCK_FREQUENCY
	while (!OSC32KCTRL->STATUS.bit.XOSC32KRDY) {
	}
#endif
}


static ALWAYS_INLINE
u32_t soc_sam0_get_gclk_input_frequency(const char *const src)
{
#ifdef DT_FIXED_CLOCK_XOSC0_CLOCK_FREQUENCY
	if (__builtin_strcmp(src, DT_FIXED_CLOCK_XOSC0_LABEL) == 0) {
		return DT_FIXED_CLOCK_XOSC0_CLOCK_FREQUENCY;
	}
#endif
#ifdef DT_FIXED_CLOCK_XOSC1_CLOCK_FREQUENCY
	if (__builtin_strcmp(src, DT_FIXED_CLOCK_XOSC1_LABEL) == 0) {
		return DT_FIXED_CLOCK_XOSC1_CLOCK_FREQUENCY;
	}
#endif
#ifdef DT_FIXED_CLOCK_GCLK1_CLOCK_FREQUENCY
	if (__builtin_strcmp(src, DT_FIXED_CLOCK_GCLK1_LABEL) == 0) {
		return DT_FIXED_CLOCK_GCLK1_CLOCK_FREQUENCY;
	}
#endif
#ifdef DT_FIXED_CLOCK_ULPOSC32K_CLOCK_FREQUENCY
	if (__builtin_strcmp(src, DT_FIXED_CLOCK_ULPOSC32K_LABEL) == 0) {
		return DT_FIXED_CLOCK_ULPOSC32K_CLOCK_FREQUENCY;
	}
#endif
#ifdef DT_FIXED_CLOCK_XOSC32K_CLOCK_FREQUENCY
	if (__builtin_strcmp(src, DT_FIXED_CLOCK_XOSC32K_LABEL) == 0) {
		return DT_FIXED_CLOCK_XOSC32K_CLOCK_FREQUENCY;
	}
#endif
#ifdef DT_FIXED_CLOCK_DFLL_CLOCK_FREQUENCY
	if (__builtin_strcmp(src, DT_FIXED_CLOCK_DFLL_LABEL) == 0) {
		return DT_FIXED_CLOCK_DFLL_CLOCK_FREQUENCY;
	}
#endif
#ifdef DT_FIXED_CLOCK_FDPLL0_CLOCK_FREQUENCY
	if (__builtin_strcmp(src, DT_FIXED_CLOCK_FDPLL0_LABEL) == 0) {
		return DT_FIXED_CLOCK_FDPLL0_CLOCK_FREQUENCY;
	}
#endif
#ifdef DT_FIXED_CLOCK_FDPLL1_CLOCK_FREQUENCY
	if (__builtin_strcmp(src, DT_FIXED_CLOCK_FDPLL1_LABEL) == 0) {
		return DT_FIXED_CLOCK_FDPLL1_CLOCK_FREQUENCY;
	}
#endif
	__ASSERT(false, "no clock source selected");
	return 0;
}

static ALWAYS_INLINE
u32_t soc_sam0_get_gclk_input_source(const char *const src)
{
#ifdef DT_FIXED_CLOCK_XOSC0_CLOCK_FREQUENCY
	if (__builtin_strcmp(src, DT_FIXED_CLOCK_XOSC0_LABEL) == 0) {
		return GCLK_GENCTRL_SRC_XOSC0;
	}
#endif
#ifdef DT_FIXED_CLOCK_XOSC1_CLOCK_FREQUENCY
	if (__builtin_strcmp(src, DT_FIXED_CLOCK_XOSC1_LABEL) == 0) {
		return GCLK_GENCTRL_SRC_XOSC1;
	}
#endif
#ifdef DT_FIXED_CLOCK_GCLK1_CLOCK_FREQUENCY
	if (__builtin_strcmp(src, DT_FIXED_CLOCK_GCLK1_LABEL) == 0) {
		return GCLK_GENCTRL_SRC_GCLKGEN1;
	}
#endif
#ifdef DT_FIXED_CLOCK_ULPOSC32K_CLOCK_FREQUENCY
	if (__builtin_strcmp(src, DT_FIXED_CLOCK_ULPOSC32K_LABEL) == 0) {
		return GCLK_GENCTRL_SRC_OSCULP32K;
	}
#endif
#ifdef DT_FIXED_CLOCK_XOSC32K_CLOCK_FREQUENCY
	if (__builtin_strcmp(src, DT_FIXED_CLOCK_XOSC32K_LABEL) == 0) {
		return GCLK_GENCTRL_SRC_XOSC32K;
	}
#endif
#ifdef DT_FIXED_CLOCK_DFLL_CLOCK_FREQUENCY
	if (__builtin_strcmp(src, DT_FIXED_CLOCK_DFLL_LABEL) == 0) {
		return GCLK_GENCTRL_SRC_DFLL;
	}
#endif
#ifdef DT_FIXED_CLOCK_FDPLL0_CLOCK_FREQUENCY
	if (__builtin_strcmp(src, DT_FIXED_CLOCK_FDPLL0_LABEL) == 0) {
		return GCLK_GENCTRL_SRC_DPLL0;
	}
#endif
#ifdef DT_FIXED_CLOCK_FDPLL1_CLOCK_FREQUENCY
	if (__builtin_strcmp(src, DT_FIXED_CLOCK_FDPLL1_LABEL) == 0) {
		return GCLK_GENCTRL_SRC_DPLL1;
	}
#endif
	__ASSERT(false, "no clock source selected");
	return 0;
}

static ALWAYS_INLINE
void soc_sam0_get_gclk_configure(const char *const src,
				 u32_t out_frequency, u32_t index)
{
	u32_t in_frequency = soc_sam0_get_gclk_input_frequency(src);
	u32_t reg = soc_sam0_get_gclk_input_source(src)
		  | GCLK_GENCTRL_IDC
		  | GCLK_GENCTRL_GENEN;

	if (in_frequency != out_frequency) {
		u32_t div = (in_frequency + out_frequency / 2U) /
			    out_frequency;
		/* If it's an exact power of two, use DIVSEL */
		if ((div & (div - 1)) == 0) {
			reg |= GCLK_GENCTRL_DIVSEL;
			div = (31U - __builtin_clz(div));
		}
		reg |= GCLK_GENCTRL_DIV(div - 1U);
	}

	GCLK->GENCTRL[index].reg = reg;
	soc_sam0_wait_gclk_synchronization();
}

#define SOC_SAM0_XOSC_SETUP(n)						       \
	soc_sam0_xosc_init(n, DT_FIXED_CLOCK_XOSC##n##_CLOCK_FREQUENCY)

#define SOC_SAM0_GCLK_SETUP(n)						       \
	soc_sam0_get_gclk_configure(DT_FIXED_CLOCK_GCLK##n##_CLOCK_CONTROLLER, \
				    DT_FIXED_CLOCK_GCLK##n##_CLOCK_FREQUENCY,  \
				    n)

/* TODO: fdpll1 */
static void soc_sam0_fdpll0_init(void)
{
#ifdef DT_FIXED_CLOCK_FDPLL0_CLOCK_FREQUENCY
	u32_t reg;
	u32_t in_frequency;
	u32_t mul;

#ifdef DT_FIXED_CLOCK_XOSC32K_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_FIXED_CLOCK_FDPLL0_CLOCK_CONTROLLER_0,
			     DT_FIXED_CLOCK_XOSC32K_LABEL) == 0) {
		in_frequency = DT_FIXED_CLOCK_XOSC32K_CLOCK_FREQUENCY;
		reg = OSCCTRL_DPLLCTRLB_REFCLK_XOSC32;
	} else
#endif
#ifdef DT_FIXED_CLOCK_XOSC0_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_FIXED_CLOCK_FDPLL0_CLOCK_CONTROLLER_0,
			     DT_FIXED_CLOCK_XOSC0_LABEL) == 0) {
		in_frequency = DT_FIXED_CLOCK_XOSC0_CLOCK_FREQUENCY;
		reg = OSCCTRL_DPLLCTRLB_REFCLK_XOSC0;
	} else
#endif
#ifdef DT_FIXED_CLOCK_XOSC1_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_FIXED_CLOCK_FDPLL0_CLOCK_CONTROLLER_0,
			     DT_FIXED_CLOCK_XOSC1_LABEL) == 0) {
		in_frequency = DT_FIXED_CLOCK_XOSC1_CLOCK_FREQUENCY;
		reg = OSCCTRL_DPLLCTRLB_REFCLK_XOSC1;
	} else
#endif
	{
		struct device *clk;
#ifdef DT_FIXED_CLOCK_FDPLL0_CLOCK_CONTROLLER_1
		struct device *clk32;

		clk32 = device_get_binding(DT_FIXED_CLOCK_FDPLL0_CLOCK_CONTROLLER_1);
		__ASSERT(clk != NULL, "invalid FDPLL32 input clock");

		clock_control_on(clk32,
				 (clock_control_subsys_t)OSCCTRL_GCLK_ID_FDPLL032K);

#endif
		clk = device_get_binding(DT_FIXED_CLOCK_FDPLL0_CLOCK_CONTROLLER_0);
		__ASSERT(clk != NULL, "invalid FDPLL input clock");

		clock_control_on(clk,
				 (clock_control_subsys_t)OSCCTRL_GCLK_ID_FDPLL0);
		clock_control_get_rate(clk,
				       (clock_control_subsys_t)OSCCTRL_GCLK_ID_FDPLL0,
				       &in_frequency);
		reg = OSCCTRL_DPLLCTRLB_REFCLK_GCLK;
	}

	__ASSERT(DT_FIXED_CLOCK_FDPLL0_CLOCK_FREQUENCY >= in_frequency,
		 "invalid FDPLL input frequency");

	/*
	 * SAM D5x/E5x Manual, 54.12.5 : Fractional Digital Phase Locked Loop
	 * (FDPLL200M) Characteristics
	 */
	__ASSERT(in_frequency >= KHZ(32),
		 "FDPLL input frequency minimum is 32 kHz");
	__ASSERT(reg == OSCCTRL_DPLLCTRLB_REFCLK_XOSC0 ||
		 reg == OSCCTRL_DPLLCTRLB_REFCLK_XOSC1 ||
		 in_frequency <= MHZ(3.2),
		 "FDPLL input frequency maximum is 3.2 MHz");
	BUILD_ASSERT_MSG(DT_FIXED_CLOCK_FDPLL0_CLOCK_FREQUENCY >= MHZ(96),
			 "FDPLL output frequency minimum is 96 MHz");
	BUILD_ASSERT_MSG(DT_FIXED_CLOCK_FDPLL0_CLOCK_FREQUENCY <= MHZ(200),
			 "FDPLL output frequency maximum is 200 MHz");

	/*
	 * Pick the largest possible divisor for XOSC that gives
	 * LDR less than its maximum.  This minimizes the impact of
	 * the fractional part for the highest accuracy.  This also
	 * ensures that the input frequency is less than the limit
	 * (2MHz).
	 */
	if (reg == OSCCTRL_DPLLCTRLB_REFCLK_XOSC0 ||
	    reg == OSCCTRL_DPLLCTRLB_REFCLK_XOSC1) {
		u32_t div = in_frequency;

		div /= (2 * DT_FIXED_CLOCK_FDPLL0_CLOCK_FREQUENCY);
		div *= 0x1000U;

		/* Ensure it's over the minimum frequency */
		div = MIN(div, in_frequency / (2U * KHZ(32)) - 1U);

		if (div > 0x800U) {
			div = 0x800U;
		} else if (div < 1U) {
			div = 1U;
		}

		in_frequency /= (2U * div);

		reg |= OSCCTRL_DPLLCTRLB_DIV(div - 1U);
	}

	/*
	 * Don't need OSCCTRL_GCLK_ID_FDPLL032K because we're not using
	 * the lock timeout (but it would be easy to put as CLOCK_1).
	 */

	mul = DT_FIXED_CLOCK_FDPLL0_CLOCK_FREQUENCY;
	mul <<= 5U;
	mul += in_frequency / 2U;
	mul /= in_frequency;

	u32_t ldr = mul >> 5U;
	u32_t ldrfrac = mul & 0x1FU;

	reg |= OSCCTRL_DPLLCTRLB_WUF | OSCCTRL_DPLLCTRLB_LBYPASS;

	OSCCTRL->Dpll[0].DPLLRATIO.reg = OSCCTRL_DPLLRATIO_LDR(ldr - 1U) |
					 OSCCTRL_DPLLRATIO_LDRFRAC(ldrfrac);

	OSCCTRL->Dpll[0].DPLLCTRLB.reg = reg;
	OSCCTRL->Dpll[0].DPLLCTRLA.reg = OSCCTRL_DPLLCTRLA_ENABLE;
#endif
}

static void soc_sam0_dfll_init(void)
{
#ifdef DT_FIXED_CLOCK_DFLL_CLOCK_FREQUENCY

	/* no manual calibration necessary? */

	u32_t reg = OSCCTRL_DFLLCTRLB_QLDIS
#ifdef OSCCTRL_DFLLCTRLB_WAITLOCK
		  | OSCCTRL_DFLLCTRLB_WAITLOCK
#endif
	;

	/* Use closed loop mode if we have a reference clock */
#ifdef DT_FIXED_CLOCK_DFLL_CLOCK_CONTROLLER
	struct device *clk;
	u32_t clk_freq;

	clk = device_get_binding(DT_FIXED_CLOCK_DFLL_CLOCK_CONTROLLER);
	__ASSERT(clk != NULL, "invalid DFLL clock source");

	reg |= OSCCTRL_DFLLCTRLB_MODE;

	/* ASF doesn't always define the DFLL ID, but it's zero */
	clock_control_on(clk, (clock_control_subsys_t)OSCCTRL_GCLK_ID_DFLL48);
	clock_control_get_rate(clk,
			       (clock_control_subsys_t)OSCCTRL_GCLK_ID_DFLL48,
			       &clk_freq);

	u32_t mul = (DT_FIXED_CLOCK_DFLL_CLOCK_FREQUENCY + clk_freq / 2U) /
		    clk_freq;

	/* Steps are half the maximum value */
	OSCCTRL->DFLLMUL.reg = OSCCTRL_DFLLMUL_CSTEP(31) |
			       OSCCTRL_DFLLMUL_FSTEP(511) |
			       OSCCTRL_DFLLMUL_MUL(mul);

#endif

	OSCCTRL->DFLLCTRLB.reg = reg;
	OSCCTRL->DFLLCTRLA.reg = OSCCTRL_DFLLCTRLA_ENABLE;
#endif
}

static void soc_sam0_wait_programmable_osc_ready(void)
{
#ifdef DT_FIXED_CLOCK_DFLL_CLOCK_FREQUENCY
#ifdef DT_FIXED_CLOCK_DFLL_CLOCK_CONTROLLER
	while (!OSCCTRL->STATUS.bit.DFLLLCKC || !OSCCTRL->STATUS.bit.DFLLLCKF) {
	}
#endif

	while (!OSCCTRL->STATUS.bit.DFLLRDY) {
	}
#endif
#ifdef DT_FIXED_CLOCK_FDPLL0_CLOCK_FREQUENCY
	while (!OSCCTRL->Dpll[0].DPLLSTATUS.bit.LOCK ||
	       !OSCCTRL->Dpll[0].DPLLSTATUS.bit.CLKRDY) {
	}
#endif
#ifdef DT_FIXED_CLOCK_FDPLL1_CLOCK_FREQUENCY
	while (!OSCCTRL->Dpll[1].DPLLSTATUS.bit.LOCK ||
	       !OSCCTRL->Dpll[1].DPLLSTATUS.bit.CLKRDY) {
	}
#endif
}

static void sam0_dividers_init(void)
{
	u32_t div = DT_FIXED_CLOCK_GCLK0_CLOCK_FREQUENCY /
		    CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

	/* Only powers of two up to 128 are available */
	__ASSERT((div & (div - 1)) == 0, "invalid CPU clock frequency");
	div = 31U - __builtin_clz(div);
	__ASSERT(div <= 7, "invalid CPU clock frequency");

	MCLK->CPUDIV.bit.DIV = div;
}


static void soc_sam0_osc_ondemand(void)
{
	/*
	 * Set all enabled oscillators to ONDEMAND, so the unneeded ones
	 * can stop.
	 */

#ifdef DT_FIXED_CLOCK_XOSC32K_CLOCK_FREQUENCY
	OSC32KCTRL->XOSC32K.bit.ONDEMAND = 1;
#endif
#ifdef DT_FIXED_CLOCK_XOSC0_CLOCK_FREQUENCY
	OSCCTRL->XOSCCTRL[0].bit.ONDEMAND = 1;
#endif
#ifdef DT_FIXED_CLOCK_XOSC1_CLOCK_FREQUENCY
	OSCCTRL->XOSCCTRL[1].bit.ONDEMAND = 1;
#endif
#ifdef DT_FIXED_CLOCK_DFLL_CLOCK_FREQUENCY
	OSCCTRL->DFLLCTRLA.bit.ONDEMAND = 1;
#endif
#ifdef DT_FIXED_CLOCK_FDPLL0_CLOCK_FREQUENCY
	OSCCTRL->Dpll[0].DPLLCTRLA.bit.ONDEMAND = 1;
#endif
#ifdef DT_FIXED_CLOCK_FDPLL1_CLOCK_FREQUENCY
	OSCCTRL->Dpll[1].DPLLCTRLA.bit.ONDEMAND = 1;
#endif
}

static int atmel_samd_init(struct device *arg)
{
	u32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

	soc_sam0_flash_wait_init();

	/* enable the Cortex M Cache Controller */
	CMCC->CTRL.bit.CEN = 1;

	/* Setup fixed oscillators */
#ifdef DT_FIXED_CLOCK_XOSC0_CLOCK_FREQUENCY
	SOC_SAM0_XOSC_SETUP(0);
#endif
#ifdef DT_FIXED_CLOCK_XOSC1_CLOCK_FREQUENCY
	SOC_SAM0_XOSC_SETUP(1);
#endif
	soc_sam0_xosc32k_init();
	soc_sam0_wait_fixed_osc_ready();

	/* Setup GCLKs */
#ifdef DT_FIXED_CLOCK_GCLK1_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(1);
#endif

#ifdef DT_FIXED_CLOCK_GCLK2_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(2);
#endif

#ifdef DT_FIXED_CLOCK_GCLK3_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(3);
#endif

#ifdef DT_FIXED_CLOCK_GCLK4_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(4);
#endif

#ifdef DT_FIXED_CLOCK_GCLK5_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(5);
#endif

#ifdef DT_FIXED_CLOCK_GCLK6_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(6);
#endif

#ifdef DT_FIXED_CLOCK_GCLK7_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(7);
#endif

#ifdef DT_FIXED_CLOCK_GCLK8_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(8);
#endif

#ifdef DT_FIXED_CLOCK_GCLK9_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(9);
#endif

#ifdef DT_FIXED_CLOCK_GCLK10_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(10);
#endif

#ifdef DT_FIXED_CLOCK_GCLK11_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(11);
#endif

	/*
	 * The DFLL/DPLL can take a GCLK input, so do them after the
	 * routing is set up.
	 */
	soc_sam0_dfll_init();
	soc_sam0_fdpll0_init();
	soc_sam0_wait_programmable_osc_ready();

	sam0_dividers_init();

	/*
	 * GCLK0 is the CPU clock, so it must be present.  This is also last
	 * so that everything else is set up beforehand.
	 */
	SOC_SAM0_GCLK_SETUP(0);

	soc_sam0_osc_ondemand();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

SYS_INIT(atmel_samd_init, PRE_KERNEL_1, 0);
