/*
 * Copyright (c) 2017 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Atmel SAMD MCU series initialization code
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>

static void flash_waitstates_init(void)
{
	/* One wait state at 48 MHz. */
	NVMCTRL->CTRLB.bit.RWS = NVMCTRL_CTRLB_RWS_HALF_Val;
}

/* Follows the same structure as the Arduino Zero, namely:
 *  XOSC32K -> GCLK1 -> DFLL48M -> GCLK0
 *  OSC8M -> 8 MHz -> GCLK3
 */

static void xosc_init(void)
{
#ifdef CONFIG_SOC_ATMEL_SAMD_XOSC
#error External oscillator support is not implemented.
#endif
}

static void wait_gclk_synchronization(void)
{
	while (GCLK->STATUS.bit.SYNCBUSY) {
	}
}

static void xosc32k_init(void)
{
#ifdef CONFIG_SOC_ATMEL_SAMD_XOSC32K
	SYSCTRL->XOSC32K.reg = SYSCTRL_XOSC32K_STARTUP(6) |
			       SYSCTRL_XOSC32K_XTALEN | SYSCTRL_XOSC32K_EN32K;

	SYSCTRL->XOSC32K.bit.ENABLE = 1;
	/* Wait for the crystal to stabalise. */
	while (!SYSCTRL->PCLKSR.bit.XOSC32KRDY) {
	}
#endif
}

static void osc32k_init(void)
{
#ifdef FUSES_OSC32K_CAL_ADDR
	uint32_t fuse = *(uint32_t *)FUSES_OSC32K_CAL_ADDR;
	uint32_t calib = (fuse & FUSES_OSC32K_CAL_Msk) >> FUSES_OSC32K_CAL_Pos;
#else
	uint32_t fuse = *(uint32_t *)FUSES_OSC32KCAL_ADDR;
	uint32_t calib = (fuse & FUSES_OSC32KCAL_Msk) >> FUSES_OSC32KCAL_Pos;
#endif

	SYSCTRL->OSC32K.reg = SYSCTRL_OSC32K_CALIB(calib) |
			      SYSCTRL_OSC32K_STARTUP(0x6u) |
			      SYSCTRL_OSC32K_EN32K | SYSCTRL_OSC32K_ENABLE;

	/* Wait for the oscillator to stabilise. */
	while (!SYSCTRL->PCLKSR.bit.OSC32KRDY) {
	}
}

static void dfll_init(void)
{
	/* No prescaler */
	GCLK->GENDIV.reg = GCLK_GENDIV_ID(1) | GCLK_GENDIV_DIV(0);
	wait_gclk_synchronization();


#if defined(CONFIG_SOC_ATMEL_SAMD_XOSC32K_AS_MAIN)
	/* Route XOSC32K to GCLK1 */
	GCLK->GENCTRL.reg =
	    GCLK_GENCTRL_ID(1) | GCLK_GENCTRL_SRC_XOSC32K | GCLK_GENCTRL_GENEN;
#elif defined(CONFIG_SOC_ATMEL_SAMD_OSC8M_AS_MAIN)
	/* Route OSC8M to GCLK1 */
	GCLK->GENCTRL.reg =
	    GCLK_GENCTRL_ID(1) | GCLK_GENCTRL_SRC_OSC8M | GCLK_GENCTRL_GENEN;
#else
#error Unsupported main clock source.
#endif

	wait_gclk_synchronization();

	/* Route GCLK1 to multiplexer 1 */
	GCLK->CLKCTRL.reg =
	    GCLK_CLKCTRL_ID(0) | GCLK_CLKCTRL_GEN_GCLK1 | GCLK_CLKCTRL_CLKEN;
	wait_gclk_synchronization();

	SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_ENABLE;
	while (!SYSCTRL->PCLKSR.bit.DFLLRDY) {
	}

	uint32_t mul = (SOC_ATMEL_SAM0_MCK_FREQ_HZ +
		     SOC_ATMEL_SAM0_GCLK1_FREQ_HZ / 2) /
		    SOC_ATMEL_SAM0_GCLK1_FREQ_HZ;

	SYSCTRL->DFLLMUL.reg = SYSCTRL_DFLLMUL_CSTEP(31) |
			       SYSCTRL_DFLLMUL_FSTEP(511) |
			       SYSCTRL_DFLLMUL_MUL(mul);
	while (!SYSCTRL->PCLKSR.bit.DFLLRDY) {
	}

	SYSCTRL->DFLLCTRL.reg |= SYSCTRL_DFLLCTRL_MODE |
#ifdef SYSCTRL_DFLLCTRL_WAITLOCK
				 SYSCTRL_DFLLCTRL_WAITLOCK |
#endif
				 SYSCTRL_DFLLCTRL_QLDIS;
	while (!SYSCTRL->PCLKSR.bit.DFLLRDY) {
	}

	/* Enable the DFLL */
	SYSCTRL->DFLLCTRL.bit.ENABLE = 1;

	while (!SYSCTRL->PCLKSR.bit.DFLLLCKC || !SYSCTRL->PCLKSR.bit.DFLLLCKF) {
	}

	while (!SYSCTRL->PCLKSR.bit.DFLLRDY) {
	}
}

static void osc8m_init(void)
{
	/* Turn off the prescaler */
	SYSCTRL->OSC8M.bit.PRESC = SYSCTRL_OSC8M_PRESC(0);
	SYSCTRL->OSC8M.bit.ONDEMAND = 0;
}

static void gclks_init(void)
{
	/* DFLL/1 -> GCLK0 */
	GCLK->GENDIV.reg = GCLK_GENDIV_ID(0) | GCLK_GENDIV_DIV(0);
	wait_gclk_synchronization();

	GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(0) | GCLK_GENCTRL_SRC_DFLL48M |
			    GCLK_GENCTRL_IDC | GCLK_GENCTRL_GENEN;
	wait_gclk_synchronization();

	/* OSC8M/1 -> GCLK3 */
	GCLK->GENCTRL.reg =
	    GCLK_GENCTRL_ID(3) | GCLK_GENCTRL_SRC_OSC8M | GCLK_GENCTRL_GENEN;
	wait_gclk_synchronization();

	/* OSCULP32K/32 -> GCLK2 */
	GCLK->GENDIV.reg = GCLK_GENDIV_ID(2) | GCLK_GENDIV_DIV(32 - 1);
	wait_gclk_synchronization();

	GCLK->GENCTRL.reg =
	    GCLK_GENCTRL_ID(2) | GCLK_GENCTRL_SRC_OSC32K | GCLK_GENCTRL_GENEN;
	wait_gclk_synchronization();
}

static void dividers_init(void)
{
	/* Set the CPU, APBA, B, and C dividers */
	PM->CPUSEL.reg = PM_CPUSEL_CPUDIV_DIV1;
	PM->APBASEL.reg = PM_APBASEL_APBADIV_DIV1_Val;
	PM->APBBSEL.reg = PM_APBBSEL_APBBDIV_DIV1_Val;
	PM->APBCSEL.reg = PM_APBCSEL_APBCDIV_DIV1_Val;

	/* TODO(mlhx): enable clock failure detection? */
}

static int atmel_samd_init(const struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

	flash_waitstates_init();
	osc8m_init();
	osc32k_init();
	xosc_init();
	xosc32k_init();
	dfll_init();
	gclks_init();
	dividers_init();

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

SYS_INIT(atmel_samd_init, PRE_KERNEL_1, 0);
