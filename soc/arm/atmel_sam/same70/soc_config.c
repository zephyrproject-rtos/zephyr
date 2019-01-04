/*
 * Copyright (c) 2016 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief System module to support early Atmel SAM E70 MCU configuration
 */

#include <device.h>
#include <init.h>
#include <soc.h>
#include <arch/cpu.h>

/**
 * @brief Perform SoC configuration at boot.
 *
 * This should be run early during the boot process but after basic hardware
 * initialization is done.
 *
 * @return 0
 */
static int atmel_same70_config(struct device *dev)
{
#ifdef CONFIG_SOC_ATMEL_SAME70_DISABLE_ERASE_PIN
	/* Disable ERASE function on PB12 pin, this is controlled by Bus Matrix */
	MATRIX->CCFG_SYSIO |= CCFG_SYSIO_SYSIO12;
#endif

	/* In Cortex-M based SoCs JTAG interface can be used to perform
	 * IEEE1149.1 JTAG Boundary scan only. It can not be used as a debug
	 * interface therefore there is no harm done by disabling the JTAG TDI
	 * pin by default.
	 */
	/* Disable TDI function on PB4 pin, this is controlled by Bus Matrix */
	MATRIX->CCFG_SYSIO |= CCFG_SYSIO_SYSIO4;

#ifdef CONFIG_LOG_BACKEND_SWO
	/* Disable PCK3 clock used by ETM module */
	PMC->PMC_SCDR = PMC_SCDR_PCK3;
	while ((PMC->PMC_SCSR) & PMC_SCSR_PCK3) {
		;
	}
	/* Select PLLA clock as PCK3 clock */
	PMC->PMC_PCK[3] = PMC_MCKR_CSS_PLLA_CLK;
	/* Enable PCK3 clock */
	PMC->PMC_SCER = PMC_SCER_PCK3;
	/* Wait for PCK3 setup to complete */
	while (!((PMC->PMC_SR) & PMC_SR_PCKRDY3)) {
		;
	}
#endif

	return 0;
}

SYS_INIT(atmel_same70_config, PRE_KERNEL_1, 1);
