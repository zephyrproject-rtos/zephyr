/*
 * Copyright (c) 2017 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Atmel SAMD MCU series initialization code
 */

#include <init.h>
#include <soc.h>
#include <drivers/clock_control.h>


static void soc_sam0_flash_wait_init(void)
{
	/* SAMD21 Manual: 37.12 wait states for Vdd >= 2.7V */
	if (DT_ARM_CORTEX_M0PLUS_0_CLOCK_FREQUENCY <= MHZ(24)) {
		NVMCTRL->CTRLB.bit.RWS = 0;
	} else {
		NVMCTRL->CTRLB.bit.RWS = 1;
	}
}

static void soc_sam0_wait_gclk_synchronization(void)
{
	while (GCLK->STATUS.bit.SYNCBUSY) {
	}
}

static void soc_sam0_osc8m_init(void)
{
#if DT_FIXED_CLOCK_OSC8M_CLOCK_FREQUENCY == MHZ(8)
	SYSCTRL->OSC8M.bit.PRESC = 0;
#elif DT_FIXED_CLOCK_OSC8M_CLOCK_FREQUENCY == MHZ(4)
	SYSCTRL->OSC8M.bit.PRESC = 1;
#elif DT_FIXED_CLOCK_OSC8M_CLOCK_FREQUENCY == MHZ(2)
	SYSCTRL->OSC8M.bit.PRESC = 2;
#elif DT_FIXED_CLOCK_OSC8M_CLOCK_FREQUENCY == MHZ(1)
	SYSCTRL->OSC8M.bit.PRESC = 3;
#else
#error Unsupported OSC8M prescaler
#endif
	SYSCTRL->OSC8M.bit.ONDEMAND = 0;
}

static void soc_sam0_osc32k_init(void)
{
#ifdef DT_FIXED_CLOCK_OSC32K_CLOCK_FREQUENCY
#ifdef FUSES_OSC32K_CAL_ADDR
	u32_t cal = ((*((u32_t *) FUSES_OSC32K_CAL_ADDR)) &
		     FUSES_OSC32K_CAL_Msk) >> FUSES_OSC32K_CAL_Pos;
#else
	u32_t cal = ((*((u32_t *) FUSES_OSC32KCAL_ADDR)) &
		     FUSES_OSC32KCAL_Msk) >> FUSES_OSC32KCAL_Pos;
#endif
	SYSCTRL->OSC32K.reg = SYSCTRL_OSC32K_CALIB(cal) |
			      SYSCTRL_OSC32K_STARTUP(6) |
			      SYSCTRL_OSC32K_EN32K |
			      SYSCTRL_OSC32K_ENABLE;
#endif
}

static void soc_sam0_xosc_init(void)
{
#ifdef DT_FIXED_CLOCK_XOSC_CLOCK_FREQUENCY
	u32_t reg = SYSCTRL_XOSC_XTALEN |
		    SYSCTRL_XOSC_ENABLE;

	/* SAMD21 Manual 17.8.5 (XOSC register description) */
	if (DT_FIXED_CLOCK_XOSC_CLOCK_FREQUENCY <= MHZ(2)) {
		reg |= SYSCTRL_XOSC_GAIN(0);
	} else if (DT_FIXED_CLOCK_XOSC_CLOCK_FREQUENCY <= MHZ(4)) {
		reg |= SYSCTRL_XOSC_GAIN(1);
	} else if (DT_FIXED_CLOCK_XOSC_CLOCK_FREQUENCY <= MHZ(8)) {
		reg |= SYSCTRL_XOSC_GAIN(2);
	} else if (DT_FIXED_CLOCK_XOSC_CLOCK_FREQUENCY <= MHZ(16)) {
		reg |= SYSCTRL_XOSC_GAIN(3);
	} else {
		reg |= SYSCTRL_XOSC_GAIN(4);
	}

	/*
	 * SAMD21 Manual 37.13.1.2 (Crystal oscillator characteristics,
	 * t_startup).
	 */
	if (DT_FIXED_CLOCK_XOSC_CLOCK_FREQUENCY <= MHZ(2)) {
		/* 48k cycles @ 2MHz */
		reg |= SYSCTRL_XOSC_STARTUP(7);
	} else if (DT_FIXED_CLOCK_XOSC_CLOCK_FREQUENCY <= MHZ(4)) {
		/* 20k cycles @ 4MHz */
		reg |= SYSCTRL_XOSC_STARTUP(8);
	} else if (DT_FIXED_CLOCK_XOSC_CLOCK_FREQUENCY <= MHZ(8)) {
		/* 13k cycles @ 8MHz */
		reg |= SYSCTRL_XOSC_STARTUP(6);
	} else if (DT_FIXED_CLOCK_XOSC_CLOCK_FREQUENCY <= MHZ(16)) {
		/* 15k cycles @ 16MHz */
		reg |= SYSCTRL_XOSC_STARTUP(5);
	} else {
		/* 10k cycles @ 32MHz */
		reg |= SYSCTRL_XOSC_STARTUP(4);
	}

	SYSCTRL->XOSC.reg = reg;
#endif
}

static void soc_sam0_xosc32k_init(void)
{
#ifdef DT_FIXED_CLOCK_XOSC32K_CLOCK_FREQUENCY
	SYSCTRL->XOSC32K.reg = SYSCTRL_XOSC32K_STARTUP(6) |
			       SYSCTRL_XOSC32K_XTALEN |
			       SYSCTRL_XOSC32K_EN32K |
			       SYSCTRL_XOSC32K_ENABLE;
#endif
}

static void soc_sam0_wait_fixed_osc_ready(void)
{
#ifdef DT_FIXED_CLOCK_OSC32K_CLOCK_FREQUENCY
	while (!SYSCTRL->PCLKSR.bit.OSC32KRDY) {
	}
#endif
#ifdef DT_FIXED_CLOCK_XOSC_CLOCK_FREQUENCY
	while (!SYSCTRL->PCLKSR.bit.XOSCRDY) {
	}
#endif
#ifdef DT_FIXED_CLOCK_XOSC32K_CLOCK_FREQUENCY
	while (!SYSCTRL->PCLKSR.bit.XOSC32KRDY) {
	}
#endif
}


static ALWAYS_INLINE
u32_t soc_sam0_get_gclk_input_frequency(const char *const src)
{
#ifdef DT_FIXED_CLOCK_XOSC_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_FIXED_CLOCK_XOSC_CLOCK_OUTPUT_NAMES_0,
			     src) == 0) {
		return DT_FIXED_CLOCK_XOSC_CLOCK_FREQUENCY;
	}
#endif
#ifdef DT_ATMEL_SAM0_GCLK_GCLK1_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_ATMEL_SAM0_GCLK_GCLK1_CLOCK_OUTPUT_NAMES_0,
			     src) == 0) {
		return DT_ATMEL_SAM0_GCLK_GCLK1_CLOCK_FREQUENCY;
	}
#endif
#ifdef DT_FIXED_CLOCK_ULPOSC32K_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_FIXED_CLOCK_ULPOSC32K_CLOCK_OUTPUT_NAMES_0,
			     src) == 0) {
		return DT_FIXED_CLOCK_ULPOSC32K_CLOCK_FREQUENCY;
	}
#endif
#ifdef DT_FIXED_CLOCK_OSC32K_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_FIXED_CLOCK_OSC32K_CLOCK_OUTPUT_NAMES_0,
			     src) == 0) {
		return DT_FIXED_CLOCK_OSC32K_CLOCK_FREQUENCY;
	}
#endif
#ifdef DT_FIXED_CLOCK_XOSC32K_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_FIXED_CLOCK_XOSC32K_CLOCK_OUTPUT_NAMES_0,
			     src) == 0) {
		return DT_FIXED_CLOCK_XOSC32K_CLOCK_FREQUENCY;
	}
#endif
#ifdef DT_FIXED_CLOCK_OSC8M_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_FIXED_CLOCK_OSC8M_CLOCK_OUTPUT_NAMES_0,
			     src) == 0) {
		return DT_FIXED_CLOCK_OSC8M_CLOCK_FREQUENCY;
	}
#endif
#ifdef DT_ATMEL_SAM0_DFLL_DFLL_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_ATMEL_SAM0_DFLL_DFLL_CLOCK_OUTPUT_NAMES_0,
			     src) == 0) {
		return DT_ATMEL_SAM0_DFLL_DFLL_CLOCK_FREQUENCY;
	}
#endif
#ifdef DT_ATMEL_SAM0_FDPLL_FDPLL_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_ATMEL_SAM0_FDPLL_FDPLL_CLOCK_OUTPUT_NAMES_0,
			     src) == 0) {
		return DT_ATMEL_SAM0_FDPLL_FDPLL_CLOCK_FREQUENCY;
	}
#endif
	__ASSERT(false, "no clock source selected");
	return 0;
}

static ALWAYS_INLINE
u32_t soc_sam0_get_gclk_input_source(const char *const src)
{
#ifdef DT_FIXED_CLOCK_XOSC_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_FIXED_CLOCK_XOSC_CLOCK_OUTPUT_NAMES_0,
			     src) == 0) {
		return GCLK_GENCTRL_SRC_XOSC;
	}
#endif
#ifdef DT_ATMEL_SAM0_GCLK_GCLK1_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_ATMEL_SAM0_GCLK_GCLK1_CLOCK_OUTPUT_NAMES_0,
			     src) == 0) {
		return GCLK_GENCTRL_SRC_GCLKGEN1;
	}
#endif
#ifdef DT_FIXED_CLOCK_ULPOSC32K_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_FIXED_CLOCK_ULPOSC32K_CLOCK_OUTPUT_NAMES_0,
			     src) == 0) {
		return GCLK_GENCTRL_SRC_OSCULP32K;
	}
#endif
#ifdef DT_FIXED_CLOCK_OSC32K_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_FIXED_CLOCK_OSC32K_CLOCK_OUTPUT_NAMES_0,
			     src) == 0) {
		return GCLK_GENCTRL_SRC_OSC32K;
	}
#endif
#ifdef DT_FIXED_CLOCK_XOSC32K_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_FIXED_CLOCK_XOSC32K_CLOCK_OUTPUT_NAMES_0,
			     src) == 0) {
		return GCLK_GENCTRL_SRC_XOSC32K;
	}
#endif
#ifdef DT_FIXED_CLOCK_OSC8M_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_FIXED_CLOCK_OSC8M_CLOCK_OUTPUT_NAMES_0,
			     src) == 0) {
		return GCLK_GENCTRL_SRC_OSC8M;
	}
#endif
#ifdef DT_ATMEL_SAM0_DFLL_DFLL_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_ATMEL_SAM0_DFLL_DFLL_CLOCK_OUTPUT_NAMES_0,
			     src) == 0) {
		return GCLK_GENCTRL_SRC_DFLL48M;
	}
#endif
#ifdef DT_ATMEL_SAM0_FDPLL_FDPLL_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_ATMEL_SAM0_FDPLL_FDPLL_CLOCK_OUTPUT_NAMES_0,
			     src) == 0) {
		return GCLK_GENCTRL_SRC_FDPLL;
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
	u32_t reg = GCLK_GENCTRL_ID(index) |
		    soc_sam0_get_gclk_input_source(src) |
		    GCLK_GENCTRL_IDC |
		    GCLK_GENCTRL_GENEN;

	if (in_frequency != out_frequency) {
		u32_t div = (in_frequency + out_frequency / 2U) /
			    out_frequency;
		/* If it's an exact power of two, use DIVSEL */
		if ((div & (div - 1)) == 0) {
			reg |= GCLK_GENCTRL_DIVSEL;
			div = (31U - __builtin_clz(div));
		}
		GCLK->GENDIV.reg = GCLK_GENDIV_ID(index) |
				   GCLK_GENDIV_DIV(div - 1U);
		soc_sam0_wait_gclk_synchronization();
	}

	GCLK->GENCTRL.reg = reg;
	soc_sam0_wait_gclk_synchronization();
}

#define SOC_SAM0_GCLK_SETUP(n)						       \
	soc_sam0_get_gclk_configure(DT_ATMEL_SAM0_GCLK_GCLK##n##_CLOCK_CONTROLLER,\
				    DT_ATMEL_SAM0_GCLK_GCLK##n##_CLOCK_FREQUENCY,\
				    n)

static void soc_sam0_fdpll_init(void)
{
#ifdef DT_ATMEL_SAM0_FDPLL_FDPLL_CLOCK_FREQUENCY
	u32_t reg;
	u32_t in_frequency;
	u32_t mul;

#ifdef DT_FIXED_CLOCK_XOSC32K_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_ATMEL_SAM0_FDPLL_FDPLL_CLOCK_CONTROLLER,
			     DT_FIXED_CLOCK_XOSC32K_CLOCK_OUTPUT_NAMES_0) == 0) {
		in_frequency = DT_FIXED_CLOCK_XOSC32K_CLOCK_FREQUENCY;
		reg = SYSCTRL_DPLLCTRLB_REFCLK_REF0;
	} else
#endif
#ifdef DT_FIXED_CLOCK_XOSC_CLOCK_FREQUENCY
	if (__builtin_strcmp(DT_ATMEL_SAM0_FDPLL_FDPLL_CLOCK_CONTROLLER,
			     DT_FIXED_CLOCK_XOSC_CLOCK_OUTPUT_NAMES_0) == 0) {
		in_frequency = DT_FIXED_CLOCK_XOSC_CLOCK_FREQUENCY;
		reg = SYSCTRL_DPLLCTRLB_REFCLK_REF1;
	} else
#endif
	{
		struct device *clk;

		clk = device_get_binding(DT_ATMEL_SAM0_FDPLL_FDPLL_CLOCK_CONTROLLER);
		__ASSERT(clk != NULL, "invalid FDPLL input clock");

		clock_control_on(clk,
				 (clock_control_subsys_t)SYSCTRL_GCLK_ID_FDPLL);
		clock_control_get_rate(clk,
				       (clock_control_subsys_t)SYSCTRL_GCLK_ID_FDPLL,
				       &in_frequency);
		reg = SYSCTRL_DPLLCTRLB_REFCLK_GCLK;
	}

	__ASSERT(DT_ATMEL_SAM0_FDPLL_FDPLL_CLOCK_FREQUENCY >= in_frequency,
		 "invalid FDPLL input frequency");

	/*
	 * SAMD21 Manual, 36.13.7: Fractional Digital Phase Locked Loop
	 * (FDPLL96M) Characteristics
	 */
	__ASSERT(in_frequency >= KHZ(32),
		 "FDPLL input frequency minimum is 32 kHz");
	__ASSERT(reg == SYSCTRL_DPLLCTRLB_REFCLK_REF1 ||
		 in_frequency <= MHZ(2),
		 "FDPLL input frequency maximum is 2 MHz");
	BUILD_ASSERT_MSG(DT_ATMEL_SAM0_FDPLL_FDPLL_CLOCK_FREQUENCY >= MHZ(48),
			 "FDPLL output frequency minimum is 48 MHz");
	BUILD_ASSERT_MSG(DT_ATMEL_SAM0_FDPLL_FDPLL_CLOCK_FREQUENCY <= MHZ(96),
			 "FDPLL output frequency maximum is 96 MHz");

	/*
	 * Pick the largest possible divisor for XOSC that gives
	 * LDR less than its maximum.  This minimizes the impact of
	 * the fractional part for the highest accuracy.  This also
	 * ensures that the input frequency is less than the limit
	 * (2MHz).
	 */
	if (reg == SYSCTRL_DPLLCTRLB_REFCLK_REF1) {
		u32_t div = in_frequency;

		div /= (2 * DT_ATMEL_SAM0_FDPLL_FDPLL_CLOCK_FREQUENCY);
		div *= 0x1000U;

		/* Ensure it's over the minimum frequency */
		div = MIN(div, in_frequency / (2U * KHZ(32)) - 1U);

		if (div > 0x800U) {
			div = 0x800U;
		} else if (div < 1U) {
			div = 1U;
		}

		in_frequency /= (2U * div);

		reg |= SYSCTRL_DPLLCTRLB_DIV(div - 1U);
	}

	/*
	 * Don't need SYSCTRL_GCLK_ID_FDPLL32K because we're not using
	 * the lock timeout (but it would be easy to put as CLOCK_1).
	 */

	mul = DT_ATMEL_SAM0_FDPLL_FDPLL_CLOCK_FREQUENCY;
	mul <<= 4U;
	mul += in_frequency / 2U;
	mul /= in_frequency;

	u32_t ldr = mul >> 4U;
	u32_t ldrfrac = mul & 0xFU;

	SYSCTRL->DPLLRATIO.reg = SYSCTRL_DPLLRATIO_LDR(ldr - 1U) |
				 SYSCTRL_DPLLRATIO_LDRFRAC(ldrfrac);

	SYSCTRL->DPLLCTRLB.reg = reg;
	SYSCTRL->DPLLCTRLA.reg = SYSCTRL_DPLLCTRLA_ENABLE;
#endif
}

static void soc_sam0_dfll_init(void)
{
#ifdef DT_ATMEL_SAM0_DFLL_DFLL_CLOCK_FREQUENCY
	/*
	 * Errata 1.2.1: the DFLL will freeze the device if is is changed
	 * while in ONDEMAND mode (the default) with no clock requested. So
	 * enable it without ONDEMAND set before doing anything else.
	 */
	SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_ENABLE;

	/* Load the 48MHz calibration */
	u32_t coarse = ((*((u32_t *)FUSES_DFLL48M_COARSE_CAL_ADDR)) &
			FUSES_DFLL48M_COARSE_CAL_Msk) >>
		       FUSES_DFLL48M_COARSE_CAL_Pos;
	u32_t fine = ((*((u32_t *)FUSES_DFLL48M_FINE_CAL_ADDR)) &
		      FUSES_DFLL48M_FINE_CAL_Msk) >>
		     FUSES_DFLL48M_FINE_CAL_Pos;

	SYSCTRL->DFLLVAL.reg = SYSCTRL_DFLLVAL_COARSE(coarse) |
			       SYSCTRL_DFLLVAL_FINE(fine);

	u32_t reg = SYSCTRL_DFLLCTRL_QLDIS |
#ifdef SYSCTRL_DFLLCTRL_WAITLOCK
		    SYSCTRL_DFLLCTRL_WAITLOCK |
#endif
		    SYSCTRL_DFLLCTRL_ENABLE;


	/* Use closed loop mode if we have a reference clock */
#ifdef DT_ATMEL_SAM0_DFLL_DFLL_CLOCK_CONTROLLER
	struct device *clk;
	u32_t clk_freq;

	clk = device_get_binding(DT_ATMEL_SAM0_DFLL_DFLL_CLOCK_CONTROLLER);
	__ASSERT(clk != NULL, "invalid DFLL clock source");

	reg |= SYSCTRL_DFLLCTRL_MODE;

	clock_control_on(clk, (clock_control_subsys_t)SYSCTRL_GCLK_ID_DFLL48);
	clock_control_get_rate(clk,
			       (clock_control_subsys_t)SYSCTRL_GCLK_ID_DFLL48,
			       &clk_freq);

	u32_t mul = (DT_ATMEL_SAM0_DFLL_DFLL_CLOCK_FREQUENCY + clk_freq / 2U) /
		    clk_freq;

	/* Steps are half the maximum value */
	SYSCTRL->DFLLMUL.reg = SYSCTRL_DFLLMUL_CSTEP(31) |
			       SYSCTRL_DFLLMUL_FSTEP(511) |
			       SYSCTRL_DFLLMUL_MUL(mul);

#endif

	SYSCTRL->DFLLCTRL.reg = reg;
#endif
}

static void soc_sam0_wait_programmable_osc_ready(void)
{
#ifdef DT_ATMEL_SAM0_DFLL_DFLL_CLOCK_FREQUENCY
#ifdef DT_ATMEL_SAM0_DFLL_DFLL_CLOCK_CONTROLLER
	while (!SYSCTRL->PCLKSR.bit.DFLLLCKC || !SYSCTRL->PCLKSR.bit.DFLLLCKF) {
	}
#endif

	while (!SYSCTRL->PCLKSR.bit.DFLLRDY) {
	}
#endif
#ifdef DT_ATMEL_SAM0_FDPLL_FDPLL_CLOCK_FREQUENCY
	while (!SYSCTRL->DPLLSTATUS.bit.LOCK ||
	       !SYSCTRL->DPLLSTATUS.bit.CLKRDY) {
	}
#endif
}

static void sam0_dividers_init(void)
{
	u32_t div = DT_ATMEL_SAM0_GCLK_GCLK0_CLOCK_FREQUENCY /
		    DT_ARM_CORTEX_M0PLUS_0_CLOCK_FREQUENCY;

	/* Only powers of two up to 128 are available */
	__ASSERT((div & (div - 1)) == 0, "invalid CPU clock frequency");
	div = 31U - __builtin_clz(div);
	__ASSERT(div <= 7, "invalid CPU clock frequency");

	PM->CPUSEL.bit.CPUDIV = div;
	PM->APBASEL.bit.APBADIV = div;
	PM->APBBSEL.bit.APBBDIV = div;
	PM->APBCSEL.bit.APBCDIV = div;
}


static void soc_sam0_osc_ondemand(void)
{
	/*
	 * Set all enabled oscillators to ONDEMAND, so the unneeded ones
	 * can stop.
	 */
	SYSCTRL->OSC8M.bit.ONDEMAND = 1;

#ifdef DT_FIXED_CLOCK_OSC32K_CLOCK_FREQUENCY
	SYSCTRL->OSC32K.bit.ONDEMAND = 1;
#endif
#ifdef DT_FIXED_CLOCK_XOSC32K_CLOCK_FREQUENCY
	SYSCTRL->XOSC32K.bit.ONDEMAND = 1;
#endif
#ifdef DT_FIXED_CLOCK_XOSC_CLOCK_FREQUENCY
	SYSCTRL->XOSC.bit.ONDEMAND = 1;
#endif
#ifdef DT_ATMEL_SAM0_DFLL_DFLL_CLOCK_FREQUENCY
	SYSCTRL->DFLLCTRL.bit.ONDEMAND = 1;
#endif
#ifdef DT_ATMEL_SAM0_FDPLL_FDPLL_CLOCK_FREQUENCY
	SYSCTRL->DPLLCTRLA.bit.ONDEMAND = 1;
#endif
}

static int atmel_samd_init(struct device *arg)
{
	u32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

	soc_sam0_flash_wait_init();

	/* Setup fixed oscillators */
	soc_sam0_osc8m_init();
	soc_sam0_osc32k_init();
	soc_sam0_xosc_init();
	soc_sam0_xosc32k_init();
	soc_sam0_wait_fixed_osc_ready();

	/* Setup GCLKs */
#ifdef DT_ATMEL_SAM0_GCLK_GCLK1_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(1);
#endif

#ifdef DT_ATMEL_SAM0_GCLK_GCLK2_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(2);
#endif

#ifdef DT_ATMEL_SAM0_GCLK_GCLK3_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(3);
#endif

#ifdef DT_ATMEL_SAM0_GCLK_GCLK4_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(4);
#endif

#ifdef DT_ATMEL_SAM0_GCLK_GCLK5_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(5);
#endif

#ifdef DT_ATMEL_SAM0_GCLK_GCLK6_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(6);
#endif

#ifdef DT_ATMEL_SAM0_GCLK_GCLK7_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(7);
#endif

#ifdef DT_ATMEL_SAM0_GCLK_GCLK8_CLOCK_FREQUENCY
	SOC_SAM0_GCLK_SETUP(8);
#endif

	/*
	 * The DFLL/DPLL can take a GCLK input, so do them after the
	 * routing is set up.
	 */
	soc_sam0_dfll_init();
	soc_sam0_fdpll_init();
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
