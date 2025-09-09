/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mchp_clock_control.h
 * @brief Clock control header file for Microchip soc devices.
 *
 * This file provides clock driver interface definitions and structures
 * for microchip soc families
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_MCHP_CLOCK_CONTROL_H_
#define INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_MCHP_CLOCK_CONTROL_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>

#if CONFIG_CLOCK_CONTROL_MCHP_SAM_D5X_E5X
#include <zephyr/drivers/clock_control/mchp_clock_sam_d5x_e5x.h>
#endif /* CLOCK_CONTROL_MCHP_SAM_D5X_E5X */

#endif /* INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_MCHP_CLOCK_CONTROL_H_ */
