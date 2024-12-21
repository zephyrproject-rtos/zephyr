/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for nxp_mcxn94x platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the nxp_mcxn94x platform.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

#ifdef CONFIG_SOC_RESET_HOOK

void soc_reset_hook(void)
{
	SystemInit();
}

#endif

#define FLEXCOMM_CHECK_2(n)	\
	BUILD_ASSERT((DT_NODE_HAS_COMPAT(n, nxp_lpuart) == 0) &&		\
		     (DT_NODE_HAS_COMPAT(n, nxp_lpi2c) == 0),			\
		     "Do not enable SPI and UART/I2C on the same Flexcomm node");

/* For SPI node enabled, check if UART or I2C is also enabled on the same parent Flexcomm node */
#define FLEXCOMM_CHECK(n) DT_FOREACH_CHILD_STATUS_OKAY(DT_PARENT(n), FLEXCOMM_CHECK_2)

/* SPI cannot be exist with UART or I2C on the same FlexComm Interface
 * Throw a build error if user is enabling SPI and UART/I2C on a Flexcomm node.
 */
DT_FOREACH_STATUS_OKAY(nxp_lpspi, FLEXCOMM_CHECK)
