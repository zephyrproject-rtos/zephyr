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

	return 0;
}

SYS_INIT(atmel_same70_config, PRE_KERNEL_1, 1);
