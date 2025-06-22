/*
* SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC51XX_H
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC51XX_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>

#include <adi_tmc_bus.h>
#include "tmc5xxx_bus.h"

#define DT_DRV_COMPAT adi_tmc51xx

/* Check for supported bus types */
#if TMC5XXX_BUS_SPI_CHECK(DT_DRV_COMPAT)
#define TMC51XX_BUS_SPI 1
#endif

#if TMC5XXX_BUS_UART_CHECK(DT_DRV_COMPAT)
#define TMC51XX_BUS_UART 1
#endif

/* Verify at least one bus type is supported */
#if !(TMC51XX_BUS_SPI || TMC51XX_BUS_UART)
#error "No supported bus types available for TMC51xx driver"
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC51XX_H */
