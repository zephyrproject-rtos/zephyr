/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC50XX_H
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC50XX_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>

#include <adi_tmc_bus.h>
#include "tmc5xxx_bus.h"

#define DT_DRV_COMPAT adi_tmc50xx

/* Check for supported bus types - TMC50xx only supports SPI */
#define TMC50XX_BUS_SPI TMC5XXX_BUS_SPI_CHECK(DT_DRV_COMPAT)

/* Verify SPI bus is supported */
#if !TMC50XX_BUS_SPI
#error "SPI bus is required for TMC50xx driver but not available"
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC50XX_H */
